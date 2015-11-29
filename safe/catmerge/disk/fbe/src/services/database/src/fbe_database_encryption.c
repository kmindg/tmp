/***************************************************************************
* Copyright (C) EMC Corporation 2010-2014
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_database_encryption.c
***************************************************************************
*
* @brief
*  This file contains database encryption related functions 
*  
* @version
*  10/03/2013 - Created - Ashok Tamilarasan
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_database_private.h"
#include "fbe_database_cmi_interface.h"
#include "fbe_database_config_tables.h"
#include "fbe_cmi.h"
#include "fbe/fbe_registry.h"

#define FBE_ENCRYPT_READ_BUFFER 10

typedef enum fbe_database_key_entry_state_s{
    FBE_DATABASE_KEY_ENTRY_STATE_INVALID,
    FBE_DATABASE_KEY_ENTRY_STATE_VALID
}fbe_database_key_entry_state_t;

/* Table entry to describe the keys */
typedef struct fbe_database_key_table_s{
    fbe_database_key_entry_t *handle;
    fbe_database_key_entry_state_t entry_state;
}fbe_database_key_table_t;


/* Queue to manage the Key related information*/
fbe_queue_head_t fbe_database_active_keys_queue;
fbe_queue_head_t fbe_database_free_keys_queue;
fbe_spinlock_t fbe_database_key_queue_lock;

/* For easier access, the key handles are stored in a table
 * indexed by object id
 */
fbe_database_key_table_t key_table[FBE_MAX_SEP_OBJECTS];

static fbe_u8_t  *fbe_database_encryption_key_virutal_memory_start = NULL;
static fbe_u32_t  fbe_database_encryption_key_entries = 0;
static fbe_u64_t  fbe_database_encryption_peer_physical_memory_start = 0;

static fbe_database_system_encryption_progress_t encryption_progress_info;
static fbe_time_t   encryption_progress_poll_time_stamp = 0;
static fbe_u32_t    encryption_poll_interval_in_secs = 60; /* 1 Minute */
static fbe_database_system_scrub_progress_t scrub_progress_info;
static fbe_time_t   scrub_progress_poll_time_stamp = 0;
static fbe_u32_t    scrub_poll_interval_in_secs = 60; /* 1 Minute */


static fbe_database_system_encryption_info_t system_encryption_info;
static fbe_time_t   encryption_info_poll_time_stamp = 0;

extern fbe_database_service_t fbe_database_service;

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/
static fbe_bool_t fbe_database_is_zeroing_in_progress(void);
static fbe_status_t fbe_database_is_system_drive_encrypted(fbe_system_encryption_state_t *sys_drive_enc);

/*!****************************************************************************
 * fbe_database_encryption_init()
 ******************************************************************************
 * @brief
 *  This function inits the memory and tables that are used to store the key
 *  related information.
 * 
 * @param  total_mem_mb      - Total Memory that we have in MB
 * @param  vitual_addr       - Pointer to the memory that was provided
 *
 * @return status               - status of the operation.
 *
 * @author
 *  10/05/2013 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_database_encryption_init(fbe_u32_t total_mem_mb, fbe_u8_t *virtual_addr)
{
    fbe_u32_t index = 0;
    fbe_u32_t total_mem_size = 0;
    fbe_u32_t num_of_entries = 0;
    fbe_u32_t entries = 0;
    fbe_database_key_entry_t *key_entry = NULL;

    fbe_queue_init(&fbe_database_active_keys_queue);
    fbe_queue_init(&fbe_database_free_keys_queue);
    fbe_spinlock_init(&fbe_database_key_queue_lock);

    for(index = 0; index < FBE_MAX_SEP_OBJECTS; index++)
    {
        key_table[index].handle = NULL;
        key_table[index].entry_state = FBE_DATABASE_KEY_ENTRY_STATE_INVALID;
    }
    total_mem_size = total_mem_mb * 1024 *1024;
    num_of_entries = total_mem_size / sizeof(fbe_database_key_entry_t);

    fbe_database_encryption_key_virutal_memory_start = virtual_addr;
    fbe_database_encryption_key_entries              = total_mem_size;

    /* Carve memory for each entry and push all the entries into the free queue */
    for(entries = 0; entries < num_of_entries; entries++)
    {
        key_entry = (fbe_database_key_entry_t *) virtual_addr;
        key_entry->key_info.object_id = FBE_OBJECT_ID_INVALID;
        fbe_queue_push(&fbe_database_free_keys_queue, &key_entry->queue_element);
        virtual_addr += sizeof(fbe_database_key_entry_t);
    }
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_database_encryption_reconstruct_key_tables()
 ******************************************************************************
 * @brief
 *  This function reconstruct key_table.
 *  This function reference fbe_database_encryption_key_virutal_memory_start
 *  and fbe_database_encryption_key_entries directly.
 * 
 * @param  void
 *
 * @return status            - status of the operation.
 *
 * @author
 *
 ******************************************************************************/
fbe_status_t fbe_database_encryption_reconstruct_key_tables(void)
{
    fbe_u8_t *virtual_addr = fbe_database_encryption_key_virutal_memory_start;
    fbe_u32_t entries = 0, total_entries = 0;
    fbe_database_key_entry_t *key_entry = NULL;

    if (virtual_addr == NULL)
    {
	    database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s uninitialized key memory \n", 
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_lock(&fbe_database_key_queue_lock);

    fbe_queue_init(&fbe_database_active_keys_queue);
    fbe_queue_init(&fbe_database_free_keys_queue);

    total_entries = fbe_database_encryption_key_entries / sizeof(fbe_database_key_entry_t);
    for(entries = 0; entries < total_entries; entries++)
    {
        key_entry = (fbe_database_key_entry_t *) virtual_addr;
        
        if ((key_entry->key_info.object_id != FBE_OBJECT_ID_INVALID)&&
            (key_entry->key_info.object_id != 0) &&
            (key_entry->key_info.object_id < FBE_MAX_SEP_OBJECTS))
        {
            key_table[key_entry->key_info.object_id].handle = key_entry;
            key_table[key_entry->key_info.object_id].entry_state = FBE_DATABASE_KEY_ENTRY_STATE_VALID;
            fbe_queue_push(&fbe_database_active_keys_queue, &key_entry->queue_element);
        }
        else
        {
            fbe_queue_push(&fbe_database_free_keys_queue, &key_entry->queue_element);
        }
        virtual_addr += sizeof(fbe_database_key_entry_t);
    }
    fbe_spinlock_unlock(&fbe_database_key_queue_lock);

    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_database_encryption_get_local_table_virtual_addr()
 ******************************************************************************
 * @brief
 *  This function report the start of virtual address for key memory
 * 
 * @param  void
 *
 * @return vitual_addr   - Pointer to the memory that was provided
 *
 * @author
 *
 ******************************************************************************/
fbe_u8_t * fbe_database_encryption_get_local_table_virtual_addr(void)
{
    return fbe_database_encryption_key_virutal_memory_start;
}

/*!****************************************************************************
 * fbe_database_encryption_get_local_table_size()
 ******************************************************************************
 * @brief
 *  This function report total number of key entries suppported.
 * 
 * @param  void 
 *
 * @return            - total number of entries
 *
 * @author
 *
 ******************************************************************************/
fbe_u32_t  fbe_database_encryption_get_local_table_size(void)
{
    return fbe_database_encryption_key_entries;
}

/*!****************************************************************************
 * fbe_database_encryption_set_peer_table_addr()
 ******************************************************************************
 * @brief
 *  This function records peer physical memory for the key table
 * 
 * @param  peer_physical_address       - the memory that was provided by peer
 *
 * @return status            - status of the operation.
 *
 * @author
 *
 ******************************************************************************/
fbe_status_t fbe_database_encryption_set_peer_table_addr(fbe_u64_t peer_physical_address)
{
    fbe_database_encryption_peer_physical_memory_start = peer_physical_address;
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_database_encryption_get_peer_table_addr()
 ******************************************************************************
 * @brief
 *  This function retrieves peer physical memory for the key table
 * 
 * @param void
 *
 * @return status            - the memory that was provided by peer
 *
 * @author
 *
 ******************************************************************************/
fbe_u64_t fbe_database_encryption_get_peer_table_addr(void)
{
    return fbe_database_encryption_peer_physical_memory_start;
}

/*!****************************************************************************
 * fbe_database_encryption_get_next_active_key_entry()
 ******************************************************************************
 * @brief
 *  This function retrieves the next active key
 * 
 * @param  current_entry      - point to current entry,
 *                              NULL indicates the beginning of the queue,
 *
 * @return next               - pointer to the next key,
 *                              NULL indicates it's the end of
 *
 * @author
 *
 ******************************************************************************/
fbe_database_key_entry_t *fbe_database_encryption_get_next_active_key_entry(fbe_database_key_entry_t *current_entry)
{
    fbe_database_key_entry_t *next_entry = NULL;

    fbe_spinlock_lock(&fbe_database_key_queue_lock);
    if (current_entry == NULL)
    {
        next_entry = (fbe_database_key_entry_t *)fbe_queue_front(&fbe_database_active_keys_queue);
    }
    else
    {
        next_entry = (fbe_database_key_entry_t *)fbe_queue_next(&fbe_database_active_keys_queue, (fbe_queue_element_t *)current_entry);
    }
    fbe_spinlock_unlock(&fbe_database_key_queue_lock);

    return next_entry;
}

/*!****************************************************************************
 * fbe_database_encryption_get_next_active_key_entry()
 ******************************************************************************
 * @brief
 *  This function retrieves the next active key
 * 
 * @param  current_entry      - point to current entry,
 *                              NULL indicates the beginning of the queue,
 *
 * @return next               - pointer to the next key,
 *                              NULL indicates it's the end of
 *
 * @author
 *
 ******************************************************************************/
fbe_status_t fbe_database_encryption_store_key_entry(fbe_encryption_setup_encryption_keys_t *encryption_key)
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t                 object_id;
    fbe_database_key_entry_t       *key_entry;

    object_id = encryption_key->object_id;

    fbe_spinlock_lock(&fbe_database_key_queue_lock);
    /* Get memory to store the Key information that was passed in */
    key_entry = (fbe_database_key_entry_t *)fbe_queue_pop(&fbe_database_free_keys_queue);

    if (key_entry != NULL)
    {
        /* record the new key */
        fbe_copy_memory(&key_entry->key_info, encryption_key, sizeof(fbe_encryption_setup_encryption_keys_t));
        fbe_queue_push(&fbe_database_active_keys_queue, &key_entry->queue_element);
        /* update the key_table */
        key_table[object_id].handle = key_entry;
        key_table[object_id].entry_state = FBE_DATABASE_KEY_ENTRY_STATE_VALID;

        status = FBE_STATUS_OK;
    }

    fbe_spinlock_unlock(&fbe_database_key_queue_lock);

    return status;
}

/*!****************************************************************************
 * fbe_database_control_setup_encryption_key()
 ******************************************************************************
 * @brief
 *  This control entry function to setup the encryption keys and sync with peer
 * 
 * @param  packet            - Pointer to the packet 
 * 
 * @return status               - status of the operation.
 *
 * @author
 *  10/05/2013 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_database_control_setup_encryption_key(fbe_packet_t * packet)
{
    fbe_payload_ex_t *                  payload;
    fbe_payload_control_operation_t*    control_operation;
    fbe_database_control_setup_encryption_key_t *encryption_info;
    fbe_status_t                        status;
    fbe_database_key_entry_t            *key_entry;
    fbe_object_id_t                     object_id;
    fbe_u32_t                           index;
    fbe_queue_head_t                    local_free_entry_queue_head;

    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "%s Entry ... \n", __FUNCTION__);

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&encryption_info, sizeof(fbe_database_control_setup_encryption_key_t));
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(status != FBE_STATUS_OK)
    {
	    database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s Invalid payload \n", 
                       __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0); /* There is nothing wrong with a packet */
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    /* make sure all requests are valid */
    for (index = 0 ; index < encryption_info->key_setup.num_of_entries; index ++)
    {
        object_id = encryption_info->key_setup.key_table_entry[index].object_id;

        if(key_table[object_id].entry_state != FBE_DATABASE_KEY_ENTRY_STATE_INVALID)
        {
	        database_trace(FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s Invalid entry key_table[0x%X] state %d \n", 
                           __FUNCTION__, 
                           object_id,
                           key_table[object_id].entry_state);
            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet, FBE_STATUS_OK, 0); /* There is nothing wrong with a packet */
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_OK;
        }
    }

    /* make sure we have enough free entries */
    fbe_queue_init(&local_free_entry_queue_head);
    index = 0;
    fbe_spinlock_lock(&fbe_database_key_queue_lock);
    do
    {
        if(fbe_queue_is_empty(&fbe_database_free_keys_queue))
        {
            /* stop the loop */
            break;
        }
        else
        {
            key_entry = (fbe_database_key_entry_t *)fbe_queue_pop(&fbe_database_free_keys_queue);
            fbe_queue_push(&local_free_entry_queue_head, &key_entry->queue_element);
            index ++ ;
        }
    }
    while (index < encryption_info->key_setup.num_of_entries);

    /* push everything back for now */
    if(!fbe_queue_is_empty(&local_free_entry_queue_head))
    {
        fbe_queue_merge(&fbe_database_free_keys_queue, &local_free_entry_queue_head);
    }
    fbe_spinlock_unlock(&fbe_database_key_queue_lock);

    if (index < encryption_info->key_setup.num_of_entries)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s No enough free space for storing the keys, request %d, available %d\n",
                        __FUNCTION__, encryption_info->key_setup.num_of_entries, index);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }


    /* we need to update peer first */
    if (is_peer_update_allowed(&fbe_database_service)) {
        /*synch to the peer*/
        status = fbe_database_cmi_send_general_command_to_peer_sync(&fbe_database_service,
                                                             (void *)encryption_info,
                                                             FBE_DATABASE_CMI_MSG_TYPE_SETUP_ENCRYPTION_KEYS);
        if(status != FBE_STATUS_OK)
        {
	        database_trace(FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s update peer failed \n", 
                           __FUNCTION__);
            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet, FBE_STATUS_OK, 0); /* There is nothing wrong with a packet */
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_OK;
        }
    }

    /* now update local */
    status = database_setup_encryption_key(encryption_info);

    if (status == FBE_STATUS_OK)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    else
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * database_setup_encryption_key()
 ******************************************************************************
 * @brief
 *  This control entry function to setup the encryption keys for the objects
 * 
 * @param  encryption_info      - Pointer to encryption_info 
 * 
 * @return status               - status of the operation.
 *
 * @author
 *
 ******************************************************************************/
fbe_status_t database_setup_encryption_key(fbe_database_control_setup_encryption_key_t *encryption_info)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_database_key_entry_t            *key_entry;
    fbe_base_config_init_key_handle_t   init_key_handle;
    fbe_object_id_t                     object_id;
    fbe_u32_t                           table_index;

    /* repeat for each key entry */
    for (table_index = 0 ; table_index < encryption_info->key_setup.num_of_entries; table_index ++)
    {
        object_id = encryption_info->key_setup.key_table_entry[table_index].object_id;

        fbe_spinlock_lock(&fbe_database_key_queue_lock);
        /* Get memory to store the Key information that was passed in */
        key_entry = (fbe_database_key_entry_t *)fbe_queue_pop(&fbe_database_free_keys_queue);

        fbe_copy_memory(&key_entry->key_info, &encryption_info->key_setup.key_table_entry[table_index], sizeof(fbe_encryption_setup_encryption_keys_t));

        fbe_queue_push(&fbe_database_active_keys_queue, &key_entry->queue_element);
        fbe_spinlock_unlock(&fbe_database_key_queue_lock);

#if 0
        /* Temporarily until tests and key manager know how to push down key masks,
         * just mark one key valid if neither are valid.
         */ 
        for (index = 0; index < key_entry->key_info.num_of_keys; index++) {
            if (key_entry->key_info.encryption_keys[index].key_mask == FBE_ENCRYPTION_KEY_MASK_NONE){
                fbe_encryption_key_mask_set(&key_entry->key_info.encryption_keys[index].key_mask,
                                            0, /* position 0 */
                                            FBE_ENCRYPTION_KEY_MASK_VALID);
            }
        }
#endif
        key_table[object_id].handle = key_entry;
        key_table[object_id].entry_state = FBE_DATABASE_KEY_ENTRY_STATE_VALID;

        database_trace (FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: setup keys for object ID:0x%08x\n", __FUNCTION__, object_id);

        init_key_handle.key_handle = &key_entry->key_info;
        /* send to object */
        status = fbe_database_send_packet_to_object(FBE_BASE_CONFIG_CONTROL_CODE_INIT_KEY_HANDLE,
                                                  &init_key_handle, /* No payload for this control code*/
                                                  sizeof(fbe_base_config_init_key_handle_t),  /* No payload for this control code*/
                                                  object_id, 
                                                  NULL,  /* no sg list*/
                                                  0,  /* no sg list*/
                                                  FBE_PACKET_FLAG_NO_ATTRIB,
                                                  FBE_PACKAGE_ID_SEP_0);
        if (status != FBE_STATUS_OK)
        {
	        database_trace(FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s setup key to object 0x%x failed \n", 
                           __FUNCTION__, object_id);
        }

    }
    return FBE_STATUS_OK;
}

fbe_status_t fbe_database_encryption_destroy(void)
{
    /**** AT-TODO NEED TO PERFORM ALL PROPER DESTROY here once we have 
     * the unregister function
     ****/

    fbe_spinlock_destroy(&fbe_database_key_queue_lock);
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_database_control_setup_kek()
 ******************************************************************************
 * @brief
 *  This control entry function to setup the key encryption keys for the objects
 * 
 * @param  packet            - Pointer to the packet 
 * 
 * @return status               - status of the operation.
 *
 * @author
 *  10/05/2013 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_database_control_setup_kek(fbe_packet_t * packet)
{
    fbe_payload_ex_t *                  payload;
    fbe_payload_control_operation_t*    control_operation;
    fbe_database_control_setup_kek_t *key_encryption_info;
    fbe_cmi_sp_id_t                     my_sp_id;
    fbe_status_t                        status;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&key_encryption_info, sizeof(fbe_database_control_setup_kek_t));

    if (status != FBE_STATUS_OK)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: unable to get payload memory for operation\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, status);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    
    fbe_cmi_get_sp_id(&my_sp_id);

    if(key_encryption_info->sp_id != my_sp_id) {
        /* This command is for the peer and so send it to the peer*/
        status = fbe_database_cmi_send_setup_kek_to_peer(&fbe_database_service,
                                                         key_encryption_info);
    }
    else
    {
        status = fbe_database_setup_kek(key_encryption_info);
    }

    if (status != FBE_STATUS_OK)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    else
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return status;
}

fbe_status_t fbe_database_setup_kek(fbe_database_control_setup_kek_t *key_encryption_info)
{
    fbe_status_t                        status;
    fbe_base_port_mgmt_register_kek_t kek_info;
    fbe_cmi_sp_id_t                     my_sp_id;

    fbe_cmi_get_sp_id(&my_sp_id);
    if(key_encryption_info->sp_id != my_sp_id) 
    {
        /* At this point, the desitnation should be this SP */
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Incorrect SP destination. Expected:%d actual:%d\n", 
                        __FUNCTION__, my_sp_id, key_encryption_info->sp_id);
    	return FBE_STATUS_GENERIC_FAILURE;
    }

    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "setup KEK for port 0x%08x\n", key_encryption_info->port_object_id);

    fbe_zero_memory(&kek_info, sizeof(fbe_base_port_mgmt_register_kek_t));

    /* Send the keys over */
    kek_info.key_size = key_encryption_info->key_size;
    kek_info.kek_ptr =  (fbe_key_handle_t *) &key_encryption_info->kek[0];
    kek_info.kek_kek_handle = key_encryption_info->kek_kek_handle;

    status = fbe_database_send_packet_to_object(FBE_BASE_PORT_CONTROL_CODE_REGISTER_KEK,
                                                &kek_info,
                                                sizeof(fbe_base_port_mgmt_register_kek_t),
                                                key_encryption_info->port_object_id,
                                                NULL,  /* no sg list*/
                                                0,  /* no sg list*/
                                                FBE_PACKET_FLAG_EXTERNAL,
                                                FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
            database_trace (FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: unable to send to object 0x%x\n", 
                            __FUNCTION__, key_encryption_info->port_object_id);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    key_encryption_info->kek_handle = kek_info.kek_handle;
    return status;
        
}

/*!****************************************************************************
 * fbe_database_control_destroy_kek()
 ******************************************************************************
 * @brief
 *  This control entry function to destroy the key encryption keys for the objects
 * 
 * @param  packet            - Pointer to the packet 
 * 
 * @return status               - status of the operation.
 *
 * @author
 *  10/05/2013 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_database_control_destroy_kek(fbe_packet_t * packet)
{
    fbe_payload_ex_t *                  payload;
    fbe_payload_control_operation_t*    control_operation;
    fbe_database_control_destroy_kek_t *key_encryption_info;
    fbe_cmi_sp_id_t                     my_sp_id;
    fbe_status_t                        status;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&key_encryption_info, sizeof(fbe_database_control_destroy_kek_t));

    if (status != FBE_STATUS_OK)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: unable to get payload memory for operation\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, status);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    
    fbe_cmi_get_sp_id(&my_sp_id);

    if(key_encryption_info->sp_id != my_sp_id) {
        /* This command is for the peer and so send it to the peer*/
        status = fbe_database_cmi_send_destroy_kek_to_peer(&fbe_database_service,
                                                           key_encryption_info);
    }
    else
    {
        status = fbe_database_destroy_kek(key_encryption_info);
    }

    if (status != FBE_STATUS_OK)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    else
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return status;
}

fbe_status_t fbe_database_destroy_kek(fbe_database_control_destroy_kek_t *key_encryption_info)
{
    fbe_status_t                        status;
    fbe_cmi_sp_id_t                     my_sp_id;

    fbe_cmi_get_sp_id(&my_sp_id);
    if(key_encryption_info->sp_id != my_sp_id) 
    {
        /* At this point, the desitnation should be this SP */
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Incorrect SP destination. Expected:%d actual:%d\n", 
                        __FUNCTION__, my_sp_id, key_encryption_info->sp_id);
    	return FBE_STATUS_GENERIC_FAILURE;
    }

    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "Destroy KEK for port 0x%08x\n", key_encryption_info->port_object_id);

    status = fbe_database_send_packet_to_object(FBE_BASE_PORT_CONTROL_CODE_UNREGISTER_KEK,
                                                NULL,
                                                0,
                                                key_encryption_info->port_object_id,
                                                NULL,  /* no sg list*/
                                                0,  /* no sg list*/
                                                FBE_PACKET_FLAG_EXTERNAL,
                                                FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
            database_trace (FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: unable to send to object 0x%x\n", 
                            __FUNCTION__, key_encryption_info->port_object_id);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
        
}

/*!****************************************************************************
 * fbe_database_control_setup_kek()
 ******************************************************************************
 * @brief
 *  This control entry function to setup the key encryption keys for the objects
 * 
 * @param  packet            - Pointer to the packet 
 * 
 * @return status               - status of the operation.
 *
 * @author
 *  10/05/2013 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_database_control_setup_kek_kek(fbe_packet_t * packet)
{
    fbe_payload_ex_t *                  payload;
    fbe_payload_control_operation_t*    control_operation;
    fbe_database_control_setup_kek_kek_t *key_encryption_info;
    fbe_cmi_sp_id_t                     my_sp_id;
    fbe_status_t                        status;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&key_encryption_info, sizeof(fbe_database_control_setup_kek_kek_t));

    if (status != FBE_STATUS_OK)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: unable to get payload memory for operation\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, status);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    
    fbe_cmi_get_sp_id(&my_sp_id);

    if(key_encryption_info->sp_id != my_sp_id) {
        /* This command is for the peer and so send it to the peer*/
        status = fbe_database_cmi_send_setup_kek_kek_to_peer(&fbe_database_service,
                                                             key_encryption_info);
    }
    else
    {
        status = fbe_database_setup_kek_kek(key_encryption_info);
    }

    if (status != FBE_STATUS_OK)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    else
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return status;
}

fbe_status_t fbe_database_setup_kek_kek(fbe_database_control_setup_kek_kek_t *key_encryption_info)
{
    fbe_status_t                        status;
    fbe_base_port_mgmt_register_kek_kek_t kek_info;
    fbe_cmi_sp_id_t                     my_sp_id;

    fbe_cmi_get_sp_id(&my_sp_id);
    if(key_encryption_info->sp_id != my_sp_id) 
    {
        /* At this point, the desitnation should be this SP */
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Incorrect SP destination. Expected:%d actual:%d\n", 
                        __FUNCTION__, my_sp_id, key_encryption_info->sp_id);
    	return FBE_STATUS_GENERIC_FAILURE;
    }

    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "setup KEK KEK for port 0x%08x\n", key_encryption_info->port_object_id);

    fbe_zero_memory(&kek_info, sizeof(fbe_base_port_mgmt_register_kek_kek_t));

    /* Send the keys over */
    kek_info.key_size = key_encryption_info->key_size;
    kek_info.kek_ptr =  (fbe_key_handle_t *) &key_encryption_info->kek_kek[0];
    
    status = fbe_database_send_packet_to_object(FBE_BASE_PORT_CONTROL_CODE_REGISTER_KEK_KEK,
                                                &kek_info,
                                                sizeof(fbe_base_port_mgmt_register_kek_kek_t),
                                                key_encryption_info->port_object_id,
                                                NULL,  /* no sg list*/
                                                0,  /* no sg list*/
                                                FBE_PACKET_FLAG_EXTERNAL,
                                                FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
            database_trace (FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: unable to send to object 0x%x\n", 
                            __FUNCTION__, key_encryption_info->port_object_id);
            return FBE_STATUS_GENERIC_FAILURE;
    }
    
    key_encryption_info->kek_kek_handle = kek_info.mp_key_handle;
    return status;
        
}

/*!****************************************************************************
 * fbe_database_control_destroy_kek_kek()
 ******************************************************************************
 * @brief
 *  This control entry function to destroy the key encryption keys for the objects
 * 
 * @param  packet            - Pointer to the packet 
 * 
 * @return status               - status of the operation.
 *
 * @author
 *  10/05/2013 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_database_control_destroy_kek_kek(fbe_packet_t * packet)
{
    fbe_payload_ex_t *                  payload;
    fbe_payload_control_operation_t*    control_operation;
    fbe_database_control_destroy_kek_kek_t *key_encryption_info;
    fbe_cmi_sp_id_t                     my_sp_id;
    fbe_status_t                        status;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&key_encryption_info, sizeof(fbe_database_control_destroy_kek_kek_t));

    if (status != FBE_STATUS_OK)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: unable to get payload memory for operation\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, status);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    
    fbe_cmi_get_sp_id(&my_sp_id);

    if(key_encryption_info->sp_id != my_sp_id) {
        /* This command is for the peer and so send it to the peer*/
        status = fbe_database_cmi_send_destroy_kek_kek_to_peer(&fbe_database_service,
                                                               key_encryption_info);
    }
    else
    {
        status = fbe_database_destroy_kek_kek(key_encryption_info);
    }

    if (status != FBE_STATUS_OK)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    else
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return status;
}

fbe_status_t fbe_database_destroy_kek_kek(fbe_database_control_destroy_kek_kek_t *key_encryption_info)
{
    fbe_status_t                        status;
    fbe_cmi_sp_id_t                     my_sp_id;
    fbe_base_port_mgmt_unregister_keys_t unregister_info;

    fbe_cmi_get_sp_id(&my_sp_id);
    if(key_encryption_info->sp_id != my_sp_id) 
    {
        /* At this point, the desitnation should be this SP */
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Incorrect SP destination. Expected:%d actual:%d\n", 
                        __FUNCTION__, my_sp_id, key_encryption_info->sp_id);
    	return FBE_STATUS_GENERIC_FAILURE;
    }

    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "Destroy KEK KEK for port 0x%08x\n", key_encryption_info->port_object_id);

    unregister_info.num_of_keys = 1;
    unregister_info.mp_key_handle[0] = key_encryption_info->kek_kek_handle;

    status = fbe_database_send_packet_to_object(FBE_BASE_PORT_CONTROL_CODE_UNREGISTER_KEK_KEK,
                                                &unregister_info,
                                                sizeof(fbe_base_port_mgmt_unregister_keys_t),
                                                key_encryption_info->port_object_id,
                                                NULL,  /* no sg list*/
                                                0,  /* no sg list*/
                                                FBE_PACKET_FLAG_EXTERNAL,
                                                FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
            database_trace (FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: unable to send to object 0x%x\n", 
                            __FUNCTION__, key_encryption_info->port_object_id);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
        
}

/*!****************************************************************************
 * fbe_database_control_destroy_kek_kek()
 ******************************************************************************
 * @brief
 *  This control entry function to destroy the key encryption keys for the objects
 * 
 * @param  packet            - Pointer to the packet 
 * 
 * @return status               - status of the operation.
 *
 * @author
 *  10/05/2013 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_database_control_reestablish_kek_kek(fbe_packet_t * packet)
{
    fbe_payload_ex_t *                  payload;
    fbe_payload_control_operation_t*    control_operation;
    fbe_database_control_reestablish_kek_kek_t *key_encryption_info;
    fbe_cmi_sp_id_t                     my_sp_id;
    fbe_status_t                        status;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&key_encryption_info, sizeof(fbe_database_control_reestablish_kek_kek_t));

    if (status != FBE_STATUS_OK)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: unable to get payload memory for operation\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, status);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    
    fbe_cmi_get_sp_id(&my_sp_id);

    if(key_encryption_info->sp_id != my_sp_id) {
        /* This command is for the peer and so send it to the peer*/
        status = fbe_database_cmi_send_reestablish_kek_kek_to_peer(&fbe_database_service,
                                                                   key_encryption_info);
    }
    else
    {
        status = fbe_database_reestablish_kek_kek(key_encryption_info);
    }

    if (status != FBE_STATUS_OK)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    else
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return status;
}

fbe_status_t fbe_database_reestablish_kek_kek(fbe_database_control_reestablish_kek_kek_t *key_encryption_info)
{
    fbe_status_t                        status;
    fbe_cmi_sp_id_t                     my_sp_id;
    fbe_base_port_mgmt_reestablish_key_handle_t reestablish_info;

    fbe_cmi_get_sp_id(&my_sp_id);
    if(key_encryption_info->sp_id != my_sp_id) 
    {
        /* At this point, the desitnation should be this SP */
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Incorrect SP destination. Expected:%d actual:%d\n", 
                        __FUNCTION__, my_sp_id, key_encryption_info->sp_id);
    	return FBE_STATUS_GENERIC_FAILURE;
    }

    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "Reestablish KEK KEK for port 0x%08x\n", key_encryption_info->port_object_id);

    reestablish_info.mp_key_handle = key_encryption_info->kek_kek_handle;

    status = fbe_database_send_packet_to_object(FBE_BASE_PORT_CONTROL_CODE_REESTABLISH_KEY_HANDLE,
                                                &reestablish_info,
                                                sizeof(fbe_base_port_mgmt_reestablish_key_handle_t),
                                                key_encryption_info->port_object_id,
                                                NULL,  /* no sg list*/
                                                0,  /* no sg list*/
                                                FBE_PACKET_FLAG_EXTERNAL,
                                                FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
            database_trace (FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: unable to send to object 0x%x\n", 
                            __FUNCTION__, key_encryption_info->port_object_id);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
        
}

/*!****************************************************************************
 * fbe_database_control_setup_encryption_rekey()
 ******************************************************************************
 * @brief
 *  This control entry function to start a rekey on an object.
 * 
 * @param  packet            - Pointer to the packet 
 * 
 * @return status               - status of the operation.
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t fbe_database_control_setup_encryption_rekey(fbe_packet_t * packet)
{
    fbe_payload_ex_t *                  payload;
    fbe_payload_control_operation_t*    control_operation;
    fbe_database_control_setup_encryption_key_t *encryption_info;
    fbe_status_t                        status;
    fbe_database_key_entry_t            *key_entry;
    fbe_object_id_t                     object_id;
    fbe_queue_head_t                    local_free_entry_queue_head;
    fbe_u32_t                           index;

    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "%s Entry ... \n", __FUNCTION__);

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&encryption_info, sizeof(fbe_database_control_setup_encryption_key_t));
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(status != FBE_STATUS_OK)
    {
	    database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s Invalid payload \n", 
                       __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0); /* There is nothing wrong with a packet */
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    /* make sure we have enough free entries */
    fbe_queue_init(&local_free_entry_queue_head);
    index = 0;
    fbe_spinlock_lock(&fbe_database_key_queue_lock);
    do
    {
        object_id = encryption_info->key_setup.key_table_entry[index].object_id;

        /* need new entry */
        if(key_table[object_id].entry_state == FBE_DATABASE_KEY_ENTRY_STATE_INVALID)
        {
            if(fbe_queue_is_empty(&fbe_database_free_keys_queue))
            {
                /* stop the loop */
                break;
            }
            else
            {
                key_entry = (fbe_database_key_entry_t *)fbe_queue_pop(&fbe_database_free_keys_queue);
                fbe_queue_push(&local_free_entry_queue_head, &key_entry->queue_element);
                index ++ ;
            }
        }
        else
        {
            index ++;
        }
    }
    while (index < encryption_info->key_setup.num_of_entries);

    /* push everything back for now */
    if(!fbe_queue_is_empty(&local_free_entry_queue_head))
    {
        fbe_queue_merge(&fbe_database_free_keys_queue, &local_free_entry_queue_head);
    }
    fbe_spinlock_unlock(&fbe_database_key_queue_lock);

    if (index < encryption_info->key_setup.num_of_entries)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s No enough free space for storing the new keys, request %d, stop after %d\n",
                        __FUNCTION__, encryption_info->key_setup.num_of_entries, index);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    /* we need to update peer first */
    if (is_peer_update_allowed(&fbe_database_service)) {
        /*synch to the peer*/
        status = fbe_database_cmi_send_general_command_to_peer_sync(&fbe_database_service,
                                                             (void *)encryption_info,
                                                             FBE_DATABASE_CMI_MSG_TYPE_REKEY_ENCRYPTION_KEYS);
        if(status != FBE_STATUS_OK)
        {
	        database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s update peer failed \n", 
                           __FUNCTION__);
            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet, FBE_STATUS_OK, 0); /* There is nothing wrong with a packet */
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_OK;
        }
    }

    /* now update local */
    status = database_rekey_encryption_key(encryption_info);

    if (status == FBE_STATUS_OK)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    else
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * database_rekek()
 ******************************************************************************
 * @brief
 *  This control entry function to rekey the encryption keys for the objects
 * 
 * @param  encryption_info      - Pointer to encryption_info 
 * 
 * @return status               - status of the operation.
 *
 * @author
 *
 ******************************************************************************/
fbe_status_t database_rekey_encryption_key(fbe_database_control_setup_encryption_key_t *encryption_info)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_database_key_entry_t            *key_entry;
    fbe_base_config_init_key_handle_t   init_key_handle;
    fbe_object_id_t                     object_id;
    fbe_u32_t                           table_index;

    /* repeat for each key entry */
    for (table_index = 0 ; table_index < encryption_info->key_setup.num_of_entries; table_index ++)
    {
        object_id = encryption_info->key_setup.key_table_entry[table_index].object_id;

        if(key_table[object_id].entry_state != FBE_DATABASE_KEY_ENTRY_STATE_INVALID)
        {
            /* Use existing entry.
             */
            key_entry = key_table[object_id].handle;
            fbe_copy_memory(&key_entry->key_info, &encryption_info->key_setup.key_table_entry[table_index], sizeof(fbe_encryption_setup_encryption_keys_t));
        }
        else {
            fbe_spinlock_lock(&fbe_database_key_queue_lock);

            /* Get memory to store the Key information that was passed in */
            key_entry = (fbe_database_key_entry_t *)fbe_queue_pop(&fbe_database_free_keys_queue);

            fbe_copy_memory(&key_entry->key_info, &encryption_info->key_setup.key_table_entry[table_index], sizeof(fbe_encryption_setup_encryption_keys_t));

            fbe_queue_push(&fbe_database_active_keys_queue, &key_entry->queue_element);
            fbe_spinlock_unlock(&fbe_database_key_queue_lock);

            key_table[object_id].handle = key_entry;
            key_table[object_id].entry_state = FBE_DATABASE_KEY_ENTRY_STATE_VALID;
        }

        database_trace (FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: setup keys for object ID:0x%08x\n", __FUNCTION__, object_id);


        init_key_handle.key_handle = &key_entry->key_info;
        status = fbe_database_send_packet_to_object(FBE_BASE_CONFIG_CONTROL_CODE_INIT_KEY_HANDLE,
                                                  &init_key_handle, /* No payload for this control code*/
                                                  sizeof(fbe_base_config_init_key_handle_t),  /* No payload for this control code*/
                                                  object_id, 
                                                  NULL,  /* no sg list*/
                                                  0,  /* no sg list*/
                                                  FBE_PACKET_FLAG_NO_ATTRIB,
                                                  FBE_PACKAGE_ID_SEP_0);
        if (status != FBE_STATUS_OK)
        {
	        database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s setup key to object 0x%x failed \n", 
                           __FUNCTION__, object_id);
        }

    }
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_database_get_list_be_port_object_id()
 ******************************************************************************
 * @brief
 *  This function returns the list of all port object IDs
 * 
 * @param  port_obj_id_buffer            - Buffer to store the port Object IDs
 * @param  port_count                    - NUmber of port that we can see
 * @param  buffer_count                  - NUmber of port that we have allocated buffer
 * 
 * @return status               - status of the operation.
 *
 * @author
 *  10/05/2013 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_database_get_list_be_port_object_id(fbe_object_id_t *port_obj_id_buffer, 
                                                     fbe_u32_t *port_count, 
                                                     fbe_u32_t buffer_count)
{
    fbe_topology_control_enumerate_ports_by_role_t topology_port_role;
    fbe_topology_control_get_total_objects_t             get_total_objects;/* this structure is small enough to be on the stack*/
    fbe_sg_element_t                                *sg_list;
    fbe_sg_element_t                                *temp_sg_list;
    fbe_u32_t                                       i;
    fbe_object_id_t                                 *object_id_list;
    fbe_status_t                                    status;

    *port_count = 0;

    /* First get the total number of objects in physical package */
    fbe_database_send_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_GET_TOTAL_OBJECTS,
                                        &get_total_objects,
                                        sizeof(fbe_topology_control_get_total_objects_t),
                                        FBE_SERVICE_ID_TOPOLOGY,
                                        NULL,  /* no sg list*/
                                        0,  /* no sg list*/
                                        FBE_PACKET_FLAG_EXTERNAL,
                                        FBE_PACKAGE_ID_PHYSICAL);

    object_id_list = (fbe_object_id_t *) fbe_allocate_contiguous_memory((sizeof(fbe_object_id_t) * get_total_objects.total_objects));

    if(object_id_list == NULL)
    {
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    sg_list = (fbe_sg_element_t *)fbe_allocate_contiguous_memory(sizeof(fbe_sg_element_t) * 2);/*one for the entry and one for the NULL termination*/

    if(sg_list == NULL)
    {
        fbe_release_contiguous_memory(object_id_list);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    temp_sg_list = sg_list;
    temp_sg_list->address = (fbe_u8_t *)object_id_list;
    temp_sg_list->count = get_total_objects.total_objects * sizeof(fbe_object_id_t);
    temp_sg_list ++;
    temp_sg_list->address = NULL;
    temp_sg_list->count = 0;

    topology_port_role.number_of_objects = get_total_objects.total_objects ;
    topology_port_role.port_role = FBE_PORT_ROLE_BE;
    topology_port_role.number_of_objects_copied = 0;

    /* First get the total number of objects in physical package */
    status = fbe_database_send_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_ENUMERATE_PORTS_BY_ROLE,
                                                 &topology_port_role,
                                                 sizeof(fbe_topology_control_enumerate_ports_by_role_t),
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 sg_list,  /* no sg list*/
                                                 2,  /* no sg list*/
                                                 FBE_PACKET_FLAG_EXTERNAL,
                                                 FBE_PACKAGE_ID_PHYSICAL);


    for (i = 0; i < topology_port_role.number_of_objects_copied; i++)
    {
        *port_obj_id_buffer = object_id_list[i];
        (*port_count)++;
        port_obj_id_buffer++;
    }

    topology_port_role.number_of_objects = get_total_objects.total_objects ;
    topology_port_role.port_role = FBE_PORT_ROLE_UNC;
    topology_port_role.number_of_objects_copied = 0;

    /* First get the total number of objects in physical package */
    status = fbe_database_send_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_ENUMERATE_PORTS_BY_ROLE,
                                                 &topology_port_role,
                                                 sizeof(fbe_topology_control_enumerate_ports_by_role_t),
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 sg_list,  /* no sg list*/
                                                 2,  /* no sg list*/
                                                 FBE_PACKET_FLAG_EXTERNAL,
                                                 FBE_PACKAGE_ID_PHYSICAL);


    for (i = 0; i < topology_port_role.number_of_objects_copied; i++)
    {
        *port_obj_id_buffer = object_id_list[i];
        (*port_count)++;
        port_obj_id_buffer++;
    }

    fbe_release_contiguous_memory(object_id_list);
    fbe_release_contiguous_memory(sg_list);
    return FBE_STATUS_OK;
}


fbe_status_t fbe_database_invalidate_key(fbe_object_id_t object_id)
{
    fbe_spinlock_lock(&fbe_database_key_queue_lock);
	if(key_table[object_id].entry_state == FBE_DATABASE_KEY_ENTRY_STATE_VALID){
        key_table[object_id].handle->key_info.object_id = FBE_OBJECT_ID_INVALID;
		fbe_queue_remove(&key_table[object_id].handle->queue_element);
		fbe_queue_push(&fbe_database_free_keys_queue, &key_table[object_id].handle->queue_element);
		key_table[object_id].handle = NULL;
		key_table[object_id].entry_state = FBE_DATABASE_KEY_ENTRY_STATE_INVALID;
	}
    fbe_spinlock_unlock(&fbe_database_key_queue_lock);

	return FBE_STATUS_OK;
}


/*!***************************************************************
 * @fn fbe_database_control_get_object_encryption_key_handle()
 *****************************************************************
 * @brief
 *   This functions retrieves the encryption key handle for the
 *  specified object
 *
 * @param packet - packet
 *
 * @return fbe_status_t - FBE_STATUS_OK if key ready
 *
 * @version
 *
 ****************************************************************/
fbe_status_t fbe_database_control_get_object_encryption_key_handle(fbe_object_id_t	object_id, fbe_encryption_setup_encryption_keys_t **key_handle)
{
    fbe_status_t                     		status = FBE_STATUS_GENERIC_FAILURE;

    fbe_spinlock_lock(&fbe_database_key_queue_lock);

	if(key_table[object_id].entry_state == FBE_DATABASE_KEY_ENTRY_STATE_VALID){
        *key_handle = &key_table[object_id].handle->key_info;
        status = FBE_STATUS_OK;
    }
    else
    {
        *key_handle = NULL;
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_spinlock_unlock(&fbe_database_key_queue_lock);


    return status;
}


/*!****************************************************************************
 * fbe_database_control_remove_encryption_keys()
 ******************************************************************************
 * @brief
 *  This control entry function to delete the encryption keys and sync with peer
 * 
 * @param  packet            - Pointer to the packet 
 * 
 * @return status               - status of the operation.
 *
 * @author
 *  12/13/2013 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t fbe_database_control_remove_encryption_keys(fbe_packet_t * packet)
{
    fbe_payload_ex_t *                  payload;
    fbe_payload_control_operation_t*    control_operation;
    fbe_database_control_remove_encryption_keys_t *encryption_info;
    fbe_status_t                        status;

    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "%s Entry ... \n", __FUNCTION__);

    /* Verify packet contents */
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    status = fbe_database_get_payload(packet, (void **)&encryption_info, sizeof(fbe_database_control_remove_encryption_keys_t));
    if(status != FBE_STATUS_OK)
    {
	    database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s Invalid payload \n", 
                       __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0); /* There is nothing wrong with a packet */
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    /* now update local */
    status = database_remove_encryption_keys(encryption_info);
    if (status == FBE_STATUS_OK)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    else
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * database_remove_encryption_keys()
 ******************************************************************************
 * @brief
 *  This control entry function to delete the encryption keys for the objects
 * 
 * @param  encryption_info      - Pointer to encryption_info 
 * 
 * @return status               - status of the operation.
 *
 * @author
 *  12/13/2013 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t database_remove_encryption_keys(fbe_database_control_remove_encryption_keys_t *encryption_info)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_object_id_t                     object_id;

    object_id = encryption_info->object_id;
    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: delete keys for object ID:0x%08x\n", __FUNCTION__, object_id);

    /* First send request to object */
    status = fbe_database_send_packet_to_object(FBE_BASE_CONFIG_CONTROL_CODE_REMOVE_KEY_HANDLE,
                                              NULL, /* No payload for this control code*/
                                              0,  /* No payload for this control code*/
                                              object_id, 
                                              NULL,  /* no sg list*/
                                              0,  /* no sg list*/
                                              FBE_PACKET_FLAG_DESTROY_ENABLED,
                                              FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s destroy key to object 0x%x failed \n", 
                       __FUNCTION__, object_id);
    }

    /* Second, invalidate keys in DB */
    fbe_database_invalidate_key(object_id);

    return FBE_STATUS_OK;
}
/*!***************************************************************
 * @fn fbe_database_control_set_poll_interval()
 *****************************************************************
 * @brief
 *   This functions sets the poll interval.
 *
 * @param packet - packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @author
 *  02/01/2014  NCHIU  - Created.
 *
 ****************************************************************/
fbe_status_t fbe_database_control_set_poll_interval(fbe_packet_t * packet)
{
    fbe_status_t                     		status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_system_poll_interval_t *	new_poll_interval = NULL;  

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, 
                                      (void **)&new_poll_interval, 
                                      sizeof(fbe_database_system_poll_interval_t));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }
    scrub_poll_interval_in_secs = new_poll_interval->poll_interval;

    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: new poll interval %d\n",__FUNCTION__, scrub_poll_interval_in_secs);
        
    fbe_database_complete_packet(packet, FBE_STATUS_OK);
    return FBE_STATUS_OK;

}

/*!***************************************************************
 * @fn fbe_database_control_get_system_scrub_progress()
 *****************************************************************
 * @brief
 *   This functions returns information about the scrubbing progress
 *   on the system as a whole
 *
 * @param packet - packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @author
 *  02/01/2014  NCHIU  - Created.
 *
 ****************************************************************/
fbe_status_t fbe_database_control_get_system_scrub_progress(fbe_packet_t * packet)
{
    fbe_status_t                     		status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_system_scrub_progress_t *	scrub_progress = NULL;  
    database_object_entry_t *	            object_entry = NULL;
    fbe_object_id_t							object_id;
    fbe_provision_drive_info_t              pvd_info;
    fbe_system_encryption_mode_t            encryption_mode;
    fbe_lifecycle_state_t                   obj_lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;


    /* Verify packet contents */
    status = fbe_database_get_payload(packet, 
                                      (void **)&scrub_progress, 
                                      sizeof(fbe_database_system_scrub_progress_t));
    if ( status != FBE_STATUS_OK ) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to get fbe_database_get_payload %p\n",__FUNCTION__, packet);

        fbe_database_complete_packet(packet, status);
        return status;
    }

    scrub_progress->blocks_already_scrubbed = 0;
    scrub_progress->total_capacity_in_blocks = 0;

    fbe_database_get_system_encryption_mode(&encryption_mode);

    /* We dont want to retrieve the information very frequently. So check if the expected time has elapsed */
    if(fbe_get_elapsed_seconds(scrub_progress_poll_time_stamp) < scrub_poll_interval_in_secs )
    {
        /* Less than the poll interval and so Return previous data */
        scrub_progress->blocks_already_scrubbed  = scrub_progress_info.blocks_already_scrubbed;
        scrub_progress->total_capacity_in_blocks = scrub_progress_info.total_capacity_in_blocks;
         
        fbe_database_complete_packet(packet, FBE_STATUS_OK);
        return FBE_STATUS_OK;
    }

    /*go over the tables, get objects that are rgs and get the RG information */
    for (object_id = 0; object_id < fbe_database_service.object_table.table_size; object_id++) {

        status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                                  object_id,
                                                                  &object_entry);  

        if ( status != FBE_STATUS_OK){
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to get object entry for object 0x%X\n",__FUNCTION__, object_id);
            continue;
        }

        if ((object_entry->header.state == DATABASE_CONFIG_ENTRY_VALID) &&
            (object_entry->db_class_id == DATABASE_CLASS_ID_PROVISION_DRIVE))
        {
            // get object lifecycle state
            status = fbe_database_generic_get_object_state(object_id, &obj_lifecycle_state, FBE_PACKAGE_ID_SEP_0);

            if (status != FBE_STATUS_OK) 
            {
                // fail to get lifecycle state
                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                           "_get_system_scrub_progress: get Obj:0x%X state failed.\n", object_id);
                continue;
            }

            // If object is in the FAIL or SPECL state, then continue.  There is no need to scrub it. 
            if ((obj_lifecycle_state == FBE_LIFECYCLE_STATE_FAIL) || (obj_lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE))
            {
                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                           "_get_system_scrub_progress: OBJ:0x%X in %s (%d) State. Continue.\n", object_id, 
                           (obj_lifecycle_state == FBE_LIFECYCLE_STATE_FAIL) ? "FBE_LIFECYCLE_STATE_FAIL":"FBE_LIFECYCLE_STATE_SPECIALIZE",
                               obj_lifecycle_state);
                continue;
            }

            scrub_progress->total_capacity_in_blocks += object_entry->set_config_union.pvd_config.configured_capacity;

            if ((object_entry->set_config_union.pvd_config.update_type_bitmask & FBE_UPDATE_PVD_AFTER_ENCRYPTION_ENABLED) != FBE_UPDATE_PVD_AFTER_ENCRYPTION_ENABLED)
            {
                status = fbe_database_send_packet_to_object(FBE_PROVISION_DRIVE_CONTROL_CODE_GET_PROVISION_DRIVE_INFO,
                                                            &pvd_info, 
                                                            sizeof(fbe_provision_drive_info_t),  
                                                            object_id, 
                                                            NULL,  /* no sg list*/
                                                            0,  /* no sg list*/
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            FBE_PACKAGE_ID_SEP_0);
                
               if ( status != FBE_STATUS_OK){
                   /* pvd destroyed */
                   scrub_progress->total_capacity_in_blocks -= object_entry->set_config_union.pvd_config.configured_capacity;

                   database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: failed to get pvd info for object 0x%X\n",__FUNCTION__, object_id);
                   continue;
               }

               if(pvd_info.scrubbing_in_progress == FBE_FALSE)
               {
                   scrub_progress->blocks_already_scrubbed += object_entry->set_config_union.pvd_config.configured_capacity;
               }
            }
        }
    }

    /* Save the information so that we can return back the previous data if poll happens too frequently */
    scrub_progress_info.blocks_already_scrubbed  = scrub_progress->blocks_already_scrubbed;
    scrub_progress_info.total_capacity_in_blocks = scrub_progress->total_capacity_in_blocks;
    scrub_progress_poll_time_stamp = fbe_get_time();

    fbe_database_complete_packet(packet, FBE_STATUS_OK);
    return FBE_STATUS_OK;
}
/*!***************************************************************
 * @fn fbe_database_is_zeroing_in_progress()
 *****************************************************************
 * @brief
 *   This function checks for zeroing in progress in the system.
 *
 * @param packet - packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    04/03/2014: nchiu - Created
 *
 ****************************************************************/
static fbe_bool_t fbe_database_is_zeroing_in_progress(void)
{
    fbe_status_t                     		status = FBE_STATUS_GENERIC_FAILURE;
    fbe_bool_t                           	zeroing_in_progress = FBE_FALSE;  
    database_object_entry_t *	            object_entry = NULL;
    fbe_object_id_t							object_id;
    fbe_provision_drive_info_t              pvd_info;
    fbe_lifecycle_state_t                   obj_lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;

    /*go over the tables, get objects that are rgs and get the RG information */
    for (object_id = 0; object_id < fbe_database_service.object_table.table_size; object_id++) {

        status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                                  object_id,
                                                                  &object_entry);  

        if ( status != FBE_STATUS_OK){
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to get object entry for object 0x%X\n",__FUNCTION__, object_id);
            continue;
        }

        if ((object_entry->header.state == DATABASE_CONFIG_ENTRY_VALID) &&
            (object_entry->db_class_id == DATABASE_CLASS_ID_PROVISION_DRIVE))
        {
            // get object lifecycle state
            status = fbe_database_generic_get_object_state(object_id, &obj_lifecycle_state, FBE_PACKAGE_ID_SEP_0);

            if (status != FBE_STATUS_OK) 
            {
                // fail to get lifecycle state
                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                           "_is_zeroing_in_progress: get OBJ:0x%X State failed\n", object_id);
                continue;
            }

            // If the object is in the FAIL or SPECL state, then continue. There is no need to scrub it.
            if ((obj_lifecycle_state == FBE_LIFECYCLE_STATE_FAIL) || (obj_lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE))
            {
                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                           "_is_zeroing_in_progress: OBJ:0x%X in State:%d. Continue.\n", object_id, obj_lifecycle_state);
                continue;
            }

            status = fbe_database_send_packet_to_object(FBE_PROVISION_DRIVE_CONTROL_CODE_GET_PROVISION_DRIVE_INFO,
                                                        &pvd_info, 
                                                        sizeof(fbe_provision_drive_info_t),  
                                                        object_id, 
                                                        NULL,  /* no sg list*/
                                                        0,  /* no sg list*/
                                                        FBE_PACKET_FLAG_NO_ATTRIB,
                                                        FBE_PACKAGE_ID_SEP_0);
                
           if ( status != FBE_STATUS_OK){
               database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: failed to get pvd info for object 0x%X\n",__FUNCTION__, object_id);
               continue;
           }

           if(pvd_info.zero_checkpoint <= pvd_info.capacity) {
               database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: pvd object 0x%X zeroing\n",__FUNCTION__, object_id);
               zeroing_in_progress = FBE_TRUE;
               break;
           }
        }
    }

    return zeroing_in_progress;
}


/*!***************************************************************
 * @fn fbe_database_control_get_system_encryption_progress()
 *****************************************************************
 * @brief
 *   This functions returns information about the encryption progress
 *   on the system as a whole
 *
 * @param packet - packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    01/06/2014: Ashok Tamilarasan - Created
 *
 ****************************************************************/
fbe_status_t fbe_database_control_get_system_encryption_progress(fbe_packet_t * packet)
{
    fbe_status_t                     		status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_system_encryption_progress_t *	encryption_progress = NULL;  
    database_object_entry_t *	            object_entry = NULL;
    fbe_object_id_t							object_id;
	fbe_raid_group_get_info_t       rg_info;
    fbe_system_encryption_mode_t encryption_mode;
    database_transaction_t *transaction;
    fbe_bool_t              rg_found = FBE_FALSE;
    fbe_base_config_control_encryption_mode_t encryption_mode_info;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, 
                                      (void **)&encryption_progress, 
                                      sizeof(fbe_database_system_encryption_progress_t));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    encryption_progress->blocks_already_encrypted = 0;
    encryption_progress->total_capacity_in_blocks = 0;

    fbe_database_get_system_encryption_mode(&encryption_mode);

    if(encryption_mode != FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED)
    {
        /* It is not in encrypted mode. So just return with initialized data */
       fbe_database_complete_packet(packet, FBE_STATUS_OK);
       return FBE_STATUS_OK;
    }
    
    /* We dont want to retrieve the information very frequently. So check if the expected time has elapsed */
    if(fbe_get_elapsed_seconds(encryption_progress_poll_time_stamp) < encryption_poll_interval_in_secs )
    {
        /* Less than the poll interval and so Return previous data */
        encryption_progress->blocks_already_encrypted = encryption_progress_info.blocks_already_encrypted;
        encryption_progress->total_capacity_in_blocks = encryption_progress_info.total_capacity_in_blocks;
         
        fbe_database_complete_packet(packet, FBE_STATUS_OK);
        return FBE_STATUS_OK;
    }

   /* Report nothing if we are in the middle of transaction */
    /* It is not safe to do it without proper locking, but may work for now */
	transaction = fbe_database_service.transaction_ptr;
	if(transaction != NULL){
		if(transaction->state != DATABASE_TRANSACTION_STATE_INACTIVE){
            encryption_progress->blocks_already_encrypted = encryption_progress_info.blocks_already_encrypted;
            encryption_progress->total_capacity_in_blocks = encryption_progress_info.total_capacity_in_blocks;
		    fbe_database_complete_packet(packet, FBE_STATUS_OK);
			return FBE_STATUS_OK;
		}
	}

    /*go over the tables, get objects that are rgs and get the RG information */
    for (object_id = 0; object_id < fbe_database_service.object_table.table_size; object_id++) {

       status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                                  object_id,
                                                                  &object_entry);  

        if ( status != FBE_STATUS_OK){
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to get object entry for object 0x%X\n",__FUNCTION__, object_id);
            continue;
        }

        if(fbe_private_space_layout_object_id_is_system_object(object_id))
        {
            if(object_id != FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG)
            {
                /* we want to ignore private RGs for now */
                continue;
            }
        }

        if (database_is_entry_valid(&object_entry->header)) 
        {
            if((object_entry->db_class_id > DATABASE_CLASS_ID_RAID_START && 
                object_entry->db_class_id < DATABASE_CLASS_ID_RAID_END)) 
            {
                status = fbe_database_send_packet_to_object(FBE_RAID_GROUP_CONTROL_CODE_GET_INFO,
                                                            &rg_info, 
                                                            sizeof(fbe_raid_group_get_info_t),  
                                                            object_id, 
                                                            NULL,  /* no sg list*/
                                                            0,  /* no sg list*/
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            FBE_PACKAGE_ID_SEP_0);
                
               if ( status != FBE_STATUS_OK){
                   database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: failed to get rg info for object 0x%X\n",__FUNCTION__, object_id);
                   continue;
               }     
               if(rg_info.raid_type == FBE_RAID_GROUP_TYPE_RAID10)
               {
                   /* We want to skip stripper because all encryption work is done at mirror level */
                   continue;
               }

               status = fbe_database_send_packet_to_object(FBE_BASE_CONFIG_CONTROL_CODE_GET_ENCRYPTION_MODE,
                                                            &encryption_mode_info, 
                                                            sizeof(fbe_base_config_control_encryption_mode_t),  
                                                            object_id, 
                                                            NULL,  /* no sg list*/
                                                            0,  /* no sg list*/
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            FBE_PACKAGE_ID_SEP_0);

               rg_found = FBE_TRUE;

               if((encryption_mode_info.encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED) ||
                  (encryption_mode_info.encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_INVALID)){
                   /* we have not really started the encryption */
                   encryption_progress->blocks_already_encrypted += 0;
               }
               else if ((encryption_mode_info.encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS) ||
                        (encryption_mode_info.encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS)){
                        /* Encryption is going on, use the checkpoint */
                        encryption_progress->blocks_already_encrypted += (rg_info.rekey_checkpoint * rg_info.num_data_disk);
               }
               else if(encryption_mode_info.encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED){
                   /* Encryption is completed */
                   encryption_progress->blocks_already_encrypted += rg_info.capacity;
               }

               encryption_progress->total_capacity_in_blocks += rg_info.capacity;
            }
        }
    }

    if(!rg_found)
    {
        encryption_progress->blocks_already_encrypted = FBE_LBA_INVALID;
        encryption_progress->total_capacity_in_blocks = FBE_LBA_INVALID;
    }
    /* Save the information so that we can return back the previous data if poll happens too frequently */
    encryption_progress_info.blocks_already_encrypted = encryption_progress->blocks_already_encrypted;
    encryption_progress_info.total_capacity_in_blocks = encryption_progress->total_capacity_in_blocks;
    encryption_progress_poll_time_stamp = fbe_get_time();

    fbe_database_complete_packet(packet, FBE_STATUS_OK);
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_control_get_system_encryption_info()
 *****************************************************************
 * @brief
 *   This functions returns information about the encryption state
 *   on the system as a whole
 *
 * @param packet - packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    01/06/2014: Ashok Tamilarasan - Created
 *
 ****************************************************************/
fbe_status_t fbe_database_control_get_system_encryption_info(fbe_packet_t * packet)
{
    fbe_status_t                     		status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_system_encryption_info_t *	encryption_info = NULL;  
    database_object_entry_t *	            object_entry = NULL;
    fbe_object_id_t							object_id;
    fbe_system_encryption_mode_t encryption_mode;
    fbe_base_config_control_encryption_mode_t encryption_mode_info;
    database_transaction_t *transaction;
    fbe_bool_t          rg_found = FBE_FALSE;
    fbe_system_encryption_state_t system_drive_status = FBE_SYSTEM_ENCRYPTION_STATE_INVALID;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&encryption_info, sizeof(fbe_database_system_encryption_info_t));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    /* Check if system drive is not encrypted and set the status to un-supported if yes*/
    status = fbe_database_is_system_drive_encrypted(&system_drive_status);
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: sys drive enc value %d\n",__FUNCTION__, system_drive_status);
    if(status == FBE_STATUS_OK && system_drive_status == FBE_SYSTEM_ENCRYPTION_STATE_UNSUPPORTED)
    {
	encryption_info->system_encryption_state = system_drive_status;
	fbe_database_complete_packet(packet, FBE_STATUS_OK);
	return FBE_STATUS_OK;
    }

    fbe_database_get_system_encryption_mode(&encryption_mode);
    if(encryption_mode != FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED)
    {
        /* It is not in encrypted mode. */
       encryption_info->system_encryption_state = FBE_SYSTEM_ENCRYPTION_STATE_UNENCRYPTED;
       encryption_info->system_scrubbing_state = FBE_SYSTEM_SCRUBBING_STATE_INVALID;
       fbe_database_complete_packet(packet, FBE_STATUS_OK);
       return FBE_STATUS_OK;
    }
    
    /* We dont want to retrieve the information very frequently. So check if the expected time has elapsed */
    if(fbe_get_elapsed_seconds(encryption_info_poll_time_stamp) < encryption_poll_interval_in_secs )
    {
        /* Less than the poll interval and so Return previous data */
        encryption_info->system_encryption_state = system_encryption_info.system_encryption_state;
        encryption_info->system_scrubbing_state = system_encryption_info.system_scrubbing_state;
         
        fbe_database_complete_packet(packet, FBE_STATUS_OK);
        return FBE_STATUS_OK;
    }

   /* Report nothing if we are in the middle of transaction */
    /* It is not safe to do it without proper locking, but may work for now */
	transaction = fbe_database_service.transaction_ptr;
	if(transaction != NULL){
		if(transaction->state != DATABASE_TRANSACTION_STATE_INACTIVE){
            encryption_info->system_encryption_state = system_encryption_info.system_encryption_state;
            encryption_info->system_scrubbing_state = system_encryption_info.system_scrubbing_state;
		    fbe_database_complete_packet(packet, FBE_STATUS_OK);
			return FBE_STATUS_OK;
		}
	}

    encryption_info->system_encryption_state = FBE_SYSTEM_ENCRYPTION_STATE_INVALID;
    encryption_info->system_scrubbing_state = FBE_SYSTEM_SCRUBBING_STATE_INVALID;

    /*go over the tables, get objects that are rgs and get the encryption mode */
    for (object_id = 0; object_id < fbe_database_service.object_table.table_size; object_id++) {

        if(fbe_private_space_layout_object_id_is_system_object(object_id))
        {
            if(object_id != FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG)
            {
                /* Except Vault other RGs are unencrypted */
                continue;
            }
        }

        status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                                  object_id,
                                                                  &object_entry);  

        if ( status != FBE_STATUS_OK){
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to get object entry for object 0x%X\n",__FUNCTION__, object_id);
            continue;
        }

        if (database_is_entry_valid(&object_entry->header)) 
        {
            if((object_entry->db_class_id > DATABASE_CLASS_ID_RAID_START && 
                object_entry->db_class_id < DATABASE_CLASS_ID_RAID_END)) 
            {
                status = fbe_database_send_packet_to_object(FBE_BASE_CONFIG_CONTROL_CODE_GET_ENCRYPTION_MODE,
                                                            &encryption_mode_info, 
                                                            sizeof(fbe_base_config_control_encryption_mode_t),  
                                                            object_id, 
                                                            NULL,  /* no sg list*/
                                                            0,  /* no sg list*/
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            FBE_PACKAGE_ID_SEP_0);
                
               if ( status != FBE_STATUS_OK){
                   database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: failed to get rg info for object 0x%X\n",__FUNCTION__, object_id);
                   continue;
               }     

               rg_found = FBE_TRUE;

               if(encryption_mode_info.encryption_mode != FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED)
               {
                   /* There is atleast one RG that is in progress and so return the system state as in progress*/
                   /* Note: We treat unencrypted or invalid also as in progress because it is possible that
                    * the mode change might be delayed due to other process that are running on the system
                    */
                   encryption_info->system_encryption_state = FBE_SYSTEM_ENCRYPTION_STATE_IN_PROGRESS;
                   break;
               }
               else
               {
                   encryption_info->system_encryption_state = FBE_SYSTEM_ENCRYPTION_STATE_ENCRYPTED;
               }
            }
        }
    }

    if(!rg_found)
    {
        /* if there are no RGs, the system is in encrypted mode and so treat this also the same */
        encryption_info->system_encryption_state = FBE_SYSTEM_ENCRYPTION_STATE_ENCRYPTED;
    }

    /* now check for scrubbing/zeroing */
    if (encryption_info->system_encryption_state == FBE_SYSTEM_ENCRYPTION_STATE_ENCRYPTED)
    {
        if (fbe_database_is_zeroing_in_progress() == FBE_TRUE)
        {
            encryption_info->system_encryption_state = FBE_SYSTEM_ENCRYPTION_STATE_SCRUBBING;
        }
    }

    /* Save the information so that we can return back the previous data if poll happens too frequently */
    system_encryption_info.system_encryption_state = encryption_info->system_encryption_state;
            
    encryption_info_poll_time_stamp = fbe_get_time();

    fbe_database_complete_packet(packet, FBE_STATUS_OK);
    return FBE_STATUS_OK;
}

//check if system drive encrypted registry is set and retrieve its value
fbe_status_t fbe_database_is_system_drive_encrypted(fbe_system_encryption_state_t *sys_drive_enc)
{
    fbe_status_t status;
    fbe_char_t defaultValue[FBE_ENCRYPT_READ_BUFFER];
    fbe_u8_t system_drive_encrypted[FBE_ENCRYPT_READ_BUFFER] = {0};
    const fbe_u8_t UNENCRYPTED[FBE_ENCRYPT_READ_BUFFER] = "0";
    status = fbe_registry_read(NULL,
                               GLOBAL_REG_PATH,
                               SYSTEM_DRIVE_ENCRYPTED,
                               &system_drive_encrypted,
                               FBE_ENCRYPT_READ_BUFFER,
                               FBE_REGISTRY_VALUE_SZ,
                               &defaultValue,
                               0,
                               FALSE);
    system_drive_encrypted[FBE_ENCRYPT_READ_BUFFER-1] = '\0';
    if(status != FBE_STATUS_OK)
    {
	database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s SystemDriveEncrypted read failed \n", __FUNCTION__);
	return status;
    }

    if(strncmp(system_drive_encrypted, UNENCRYPTED,1) == 0)
    {
	*sys_drive_enc = FBE_SYSTEM_ENCRYPTION_STATE_UNSUPPORTED;
    }
    return status;
}

/*!****************************************************************************
 * fbe_database_control_update_drive_keys()
 ******************************************************************************
 * @brief
 *  This control entry function updates the encryption keys and sync with peer
 * 
 * @param  packet            - Pointer to the packet 
 * 
 * @return status               - status of the operation.
 *
 * @author
 *  01/06/2013 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t fbe_database_control_update_drive_keys(fbe_packet_t * packet)
{
    fbe_payload_ex_t *                  payload;
    fbe_payload_control_operation_t*    control_operation;
    fbe_database_control_update_drive_key_t *encryption_info;
    fbe_status_t                        status;
    fbe_object_id_t                     object_id;

    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "%s Entry ... \n", __FUNCTION__);

    /* Verify packet contents */
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    status = fbe_database_get_payload(packet, (void **)&encryption_info, sizeof(fbe_database_control_update_drive_key_t));
    if(status != FBE_STATUS_OK)
    {
	    database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s Invalid payload \n", 
                       __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0); /* There is nothing wrong with a packet */
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    object_id = encryption_info->key_setup.key_table_entry[0].object_id;
    if (key_table[object_id].entry_state == FBE_DATABASE_KEY_ENTRY_STATE_INVALID)
    {
        /* It is possible that the usurper is sent before the RG gets the initial key setup.
         * We still need to inform VD about it. 
         */
        database_trace(FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s No key entry for object 0x%x\n",
                        __FUNCTION__, object_id);
#if 0
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
#endif
    }

    /* We need to update peer now */
    if (is_peer_update_allowed(&fbe_database_service)) 
    {
        status = fbe_database_cmi_send_general_command_to_peer_sync(&fbe_database_service,
                                                                    (void *)encryption_info,
                                                                     FBE_DATABASE_CMI_MSG_TYPE_UPDATE_DRIVE_KEYS);
        if(status != FBE_STATUS_OK)
        {
	        database_trace(FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s update peer failed \n", 
                           __FUNCTION__);
            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet, FBE_STATUS_OK, 0); /* There is nothing wrong with a packet */
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_OK;
        }
    }

    /* Update local */
    status = database_update_drive_keys(encryption_info);
    if(status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s update peer failed \n", 
                       __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0); /* There is nothing wrong with a packet */
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

fbe_bool_t database_update_drive_keys_validate_keys(fbe_encryption_setup_encryption_keys_t *db_keys, 
                                                    fbe_database_control_update_drive_key_t *encryption_info)
{
    fbe_encryption_setup_encryption_keys_t *kms_keys = &encryption_info->key_setup.key_table_entry[0];
    fbe_u32_t i;
    fbe_bool_t keys_valid = FBE_TRUE;

    for (i = 0; i < FBE_RAID_MAX_DISK_ARRAY_WIDTH; i++)
    {
        if (!(encryption_info->mask & (1 << i)))
        {
            if (!fbe_equal_memory(&db_keys->encryption_keys[i], &kms_keys->encryption_keys[i], sizeof(fbe_encryption_key_info_t)))
            {
                keys_valid = FBE_FALSE;

                database_trace (FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: keys %d changed object ID:0x%08x\n", __FUNCTION__, i, db_keys->object_id);
            }
        }
    }
    return (keys_valid);
}

/*!****************************************************************************
 * database_update_drive_keys()
 ******************************************************************************
 * @brief
 *  This function updates the encryption keys for the RG object
 * 
 * @param  encryption_info      - Pointer to encryption_info 
 * 
 * @return status               - status of the operation.
 *
 * @author
 *  01/06/2013 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t database_update_drive_keys(fbe_database_control_update_drive_key_t *encryption_info)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_object_id_t                     object_id;
    fbe_base_config_update_drive_keys_t       update_keys;
    fbe_database_key_entry_t            *key_entry;

    object_id = encryption_info->key_setup.key_table_entry[0].object_id;
    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: update keys for object ID:0x%08x\n", __FUNCTION__, object_id);

    if(key_table[object_id].entry_state != FBE_DATABASE_KEY_ENTRY_STATE_INVALID)
    {
        /* Use existing entry.
         */
        key_entry = key_table[object_id].handle;
        database_update_drive_keys_validate_keys(&key_entry->key_info, encryption_info);
        fbe_copy_memory(&key_entry->key_info, &encryption_info->key_setup.key_table_entry[0], sizeof(fbe_encryption_setup_encryption_keys_t));
        update_keys.key_handle = &key_entry->key_info;
    }
    else
    {
        update_keys.key_handle = NULL;
    }

    update_keys.mask = encryption_info->mask;
    /* Send request to object */
    status = fbe_database_send_packet_to_object(FBE_BASE_CONFIG_CONTROL_CODE_UPDATE_DRIVE_KEYS,
                                                  &update_keys, /* No payload for this control code*/
                                                  sizeof(fbe_base_config_update_drive_keys_t),  /* No payload for this control code*/
                                                  object_id, 
                                                  NULL,  /* no sg list*/
                                                  0,  /* no sg list*/
                                                  FBE_PACKET_FLAG_NO_ATTRIB,
                                                  FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s update key to object 0x%x failed \n", 
                       __FUNCTION__, object_id);
    }

    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_database_control_set_port_encryption_mode()
 ******************************************************************************
 * @brief
 *  This control entry function to set the encryption mode of the port 
 * 
 * @param  packet            - Pointer to the packet 
 * 
 * @return status               - status of the operation.
 *
 * @author
 *  10/05/2013 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_database_control_set_port_encryption_mode(fbe_packet_t * packet)
{
    fbe_payload_ex_t *                  payload;
    fbe_payload_control_operation_t*    control_operation;
    fbe_database_control_port_encryption_mode_t *port_encryption_info;
    fbe_cmi_sp_id_t                     my_sp_id;
    fbe_status_t                        status;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&port_encryption_info, sizeof(fbe_database_control_port_encryption_mode_t));

    if (status != FBE_STATUS_OK)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: unable to get payload memory for operation\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, status);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    
    fbe_cmi_get_sp_id(&my_sp_id);

    if(port_encryption_info->sp_id != my_sp_id) {
        /* This command is for the peer and so send it to the peer*/
        status = fbe_database_cmi_send_port_encryption_mode_to_peer(&fbe_database_service,
                                                                    port_encryption_info,
                                                                    FBE_DATABASE_PORT_ENCRYPTION_OP_MODE_SET);
    }
    else {
        status = fbe_database_set_port_encryption_mode(port_encryption_info);
    }

    if (status != FBE_STATUS_OK){
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    else {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return status;
}

fbe_status_t fbe_database_set_port_encryption_mode(fbe_database_control_port_encryption_mode_t *port_encryption_info)
{
    fbe_status_t                        status;
    fbe_base_port_mgmt_set_encryption_mode_t mode_info;
    fbe_cmi_sp_id_t                     my_sp_id;

    fbe_cmi_get_sp_id(&my_sp_id);
    if(port_encryption_info->sp_id != my_sp_id) 
    {
        /* At this point, the desitnation should be this SP */
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Incorrect SP destination. Expected:%d actual:%d\n", 
                        __FUNCTION__, my_sp_id, port_encryption_info->sp_id);
    	return FBE_STATUS_GENERIC_FAILURE;
    }

    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "set port %d encryption mode %d \n", port_encryption_info->port_object_id,
                    port_encryption_info->mode );

    fbe_zero_memory(&mode_info, sizeof(fbe_base_port_mgmt_set_encryption_mode_t));

    mode_info.encryption_mode = port_encryption_info->mode;
    status = fbe_database_send_packet_to_object(FBE_BASE_PORT_CONTROL_CODE_SET_ENCRYPTION_MODE,
                                                &mode_info,
                                                sizeof(fbe_base_port_mgmt_set_encryption_mode_t),
                                                port_encryption_info->port_object_id,
                                                NULL,  /* no sg list*/
                                                0,  /* no sg list*/
                                                FBE_PACKET_FLAG_EXTERNAL,
                                                FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
            database_trace (FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: unable to send to object 0x%x\n", 
                            __FUNCTION__, port_encryption_info->port_object_id);
            return FBE_STATUS_GENERIC_FAILURE;
    }
    
    return status;
        
}

/*!****************************************************************************
 * fbe_database_control_get_port_encryption_mode()
 ******************************************************************************
 * @brief
 *  This control entry function to get the encryption mode of the port 
 * 
 * @param  packet            - Pointer to the packet 
 * 
 * @return status               - status of the operation.
 *
 * @author
 *  10/05/2013 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_database_control_get_port_encryption_mode(fbe_packet_t * packet)
{
    fbe_payload_ex_t *                  payload;
    fbe_payload_control_operation_t*    control_operation;
    fbe_database_control_port_encryption_mode_t *port_encryption_info;
    fbe_cmi_sp_id_t                     my_sp_id;
    fbe_status_t                        status;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&port_encryption_info, sizeof(fbe_database_control_port_encryption_mode_t));

    if (status != FBE_STATUS_OK)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: unable to get payload memory for operation\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, status);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    
    fbe_cmi_get_sp_id(&my_sp_id);

    if(port_encryption_info->sp_id != my_sp_id) {
        /* This command is for the peer and so send it to the peer*/
        status = fbe_database_cmi_send_port_encryption_mode_to_peer(&fbe_database_service,
                                                                     port_encryption_info,
                                                                     FBE_DATABASE_PORT_ENCRYPTION_OP_MODE_GET);
    }
    else
    {
        status = fbe_database_get_port_encryption_mode(port_encryption_info);
    }

    if (status != FBE_STATUS_OK)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    else
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return status;
}

fbe_status_t fbe_database_get_port_encryption_mode(fbe_database_control_port_encryption_mode_t *port_encryption_info)
{
    fbe_status_t                        status;
    fbe_port_info_t                     mode_info;
    fbe_cmi_sp_id_t                     my_sp_id;

    fbe_cmi_get_sp_id(&my_sp_id);
    if(port_encryption_info->sp_id != my_sp_id) 
    {
        /* At this point, the desitnation should be this SP */
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Incorrect SP destination. Expected:%d actual:%d\n", 
                        __FUNCTION__, my_sp_id, port_encryption_info->sp_id);
    	return FBE_STATUS_GENERIC_FAILURE;
    }

    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "get port %d encryption mode %d \n", port_encryption_info->port_object_id,
                    port_encryption_info->mode );

    fbe_zero_memory(&mode_info, sizeof(fbe_port_info_t));

    status = fbe_database_send_packet_to_object(FBE_BASE_PORT_CONTROL_CODE_GET_PORT_INFO,
                                                &mode_info,
                                                sizeof(fbe_port_info_t),
                                                port_encryption_info->port_object_id,
                                                NULL,  /* no sg list*/
                                                0,  /* no sg list*/
                                                FBE_PACKET_FLAG_EXTERNAL,
                                                FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
            database_trace (FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: unable to send to object 0x%x\n", 
                            __FUNCTION__, port_encryption_info->port_object_id);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    port_encryption_info->mode = mode_info.enc_mode;
    return status;
        
}

