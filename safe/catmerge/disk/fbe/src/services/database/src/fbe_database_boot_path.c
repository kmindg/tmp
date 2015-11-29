/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_database_boot_path.c
***************************************************************************
*
* @brief
*  This file contains database service boot path routines
*  
* @version
*  4/12/2011 - Created. 
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_database_private.h"
#include "fbe_database_config_tables.h"
#include "fbe_imaging_structures.h"
#include "fbe_fru_descriptor_structure.h"
#include "fbe_private_space_layout.h"
#include "fbe_database_system_db_interface.h"
#include "fbe_database_metadata_interface.h"
#include "fbe_database_homewrecker_fru_descriptor.h"
#include "fbe_database_homewrecker_db_interface.h"
#include "fbe_database_persist_interface.h"
#include "fbe/fbe_event_log_api.h"
#include "fbe/fbe_event_log_utils.h"
#include "fbe_fru_signature.h"
#include "fbe_database_fru_signature_interface.h"
#include "fbe_database_drive_connection.h"
#include "fbe_raid_library.h"

#ifdef C4_INTEGRATED
#include "stdio.h"
#include "stdlib.h"
#endif

extern fbe_database_service_t fbe_database_service;


typedef struct database_init_persist_context_s
{
    fbe_status_t commit_status;
    fbe_semaphore_t sem;
}database_init_persist_context_t;

typedef struct database_pdo_io_context_s{
    fbe_semaphore_t semaphore;
    fbe_atomic_t    pdo_io_complete;
}database_pdo_io_context_t;

static database_init_persist_context_t     init_persist_context;

/* Note: This global variable won't be initialized in passive SP during booting.
 * If you want to use it, it's better reading it from disk firstly */
static fbe_imaging_flags_t                 fbe_database_imaging_flags;    /*store ica flags*/

#define MIRROR_SPA_BOOT_PRIMARY_TARGET_ID   (0x0)
#define MIRROR_SPB_BOOT_PRIMARY_TARGET_ID   (0x1)
#define MIRROR_SPA_BOOT_SECONDARY_TARGET_ID (0x2)
#define MIRROR_SPB_BOOT_SECONDARY_TARGET_ID (0x3)

/*************************
*   INCLUDE FUNCTIONS
*************************/
static fbe_status_t database_set_db_lun_completion(fbe_status_t commit_status, fbe_persist_completion_context_t context);
static fbe_status_t fbe_database_load_from_persist_read_completion_function(fbe_status_t read_status, 
                                                                            fbe_persist_entry_id_t next_entry, 
                                                                            fbe_u32_t entries_read, 
                                                                            fbe_persist_completion_context_t context);
static fbe_status_t fbe_database_load_from_persist(void);
static fbe_status_t fbe_database_load_from_persist_single_entry(fbe_database_service_t *database_service_ptr);
static fbe_status_t fbe_database_load_from_persist_per_type(database_config_table_type_t table_type);
static fbe_status_t fbe_database_load_from_persist_single_entry_per_type(database_config_table_type_t table_type);

static fbe_status_t database_copy_entries_from_persist_read(fbe_u8_t * read_buffer, 
                                                            fbe_u32_t elements_read, 
                                                            database_config_table_type_t table_type);
static fbe_status_t database_copy_single_entry_from_persist_read(fbe_u8_t * read_buffer, 
                                                                 database_config_table_type_t table_type);
static void fbe_database_carve_memory_for_packet_and_sgl(fbe_packet_t **packet_aray,
                                                         fbe_sg_element_t **sg_list_array,
                                                         fbe_u32_t number_of_drives_for_operation,
                                                         fbe_memory_request_t *memory_request);

static fbe_status_t fbe_database_carve_memory_for_packet_and_sgl_extend(fbe_packet_t **packet_aray,
                                                                        fbe_sg_element_t **sg_list_array,
                                                                        fbe_u32_t number_of_drives_for_operation,
                                                                        fbe_u32_t sg_list_length_for_each_drive,
                                                                        fbe_memory_request_t *memory_request);


/* Completion functions of flag io*/

static fbe_status_t
fbe_database_read_flags_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context);

static fbe_status_t
fbe_database_write_flags_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context);
/*********************************************************************************/


/* The following functions are related to IO to PDOs*/
static fbe_status_t fbe_database_send_io_packets_to_pdos_completion(fbe_packet_t * packet_p,
                                                                       fbe_packet_completion_context_t context);

static fbe_status_t fbe_database_clean_destroy_packet(fbe_packet_t * packet);

static fbe_status_t fbe_database_send_read_packets_to_pdos(fbe_packet_t **packet_array,
                                                           fbe_sg_element_t **sg_list_array,
                                                           database_physical_drive_info_t* drive_locations,
                                                           fbe_u32_t  number_of_drives_for_operation,
                                                           fbe_block_count_t   block_count,
                                                           fbe_private_space_layout_region_id_t  pls_id,
                                                           fbe_u32_t*   complete_packet_count);

static fbe_status_t fbe_database_send_write_packets_to_pdos(fbe_packet_t **packet_array,
                                                            fbe_sg_element_t **sg_list_array,
                                                            database_physical_drive_info_t* drive_locations,
                                                            fbe_u32_t  number_of_drives_for_operation,
                                                            fbe_block_count_t block_count,
                                                            fbe_private_space_layout_region_id_t  pls_id,
                                                            fbe_u32_t*   complete_packet_count);
/*********************************************************************************/


/* The following functions are auxiliary functions for single raw drive io*/

static fbe_status_t
fbe_database_single_raw_drive_io_allocation_completion(fbe_memory_request_t* mem_request, 
                                                       fbe_memory_completion_context_t context);
static fbe_status_t
fbe_database_copy_data_from_read_sg_list(fbe_u8_t* out_buffer, fbe_u64_t buffer_length, 
                                         fbe_sg_element_t* sg_list);

static fbe_status_t
fbe_database_copy_data_to_write_sg_list(fbe_u8_t* in_data_buffer, fbe_u64_t data_length,
                                        fbe_sg_element_t* sg_list);

/********************************************************************************/


/*FUNCTIONS RELATED WITH ICA FLAG PROCESSING*/
static fbe_status_t fbe_database_read_ica_flags(void);
static fbe_status_t fbe_database_validate_ica_stamp_read(fbe_u8_t *flags_array, fbe_bool_t all_drives_read);

/********************************************************************************/



/********************************************************************************/

static fbe_status_t database_system_copy_entries_from_persist_read(fbe_u8_t *read_buffer,
                                                                   fbe_u32_t count,
                                                                   database_config_table_type_t table_type);
static fbe_status_t database_system_copy_entries_to_write_buffer(fbe_u8_t *write_buffer,
                                                                 fbe_u32_t start_index,
                                                                 fbe_u32_t count,
                                                                 database_config_table_type_t table_type);


static fbe_status_t fbe_database_refactor_tables_for_bad_entry(fbe_database_service_t *database_service_ptr);
static fbe_status_t fbe_database_refactor_tables_for_bad_entry_corrupt_clients(fbe_u32_t object_id);
static void fbe_database_refactor_tables_for_bad_object_entry(database_object_entry_t* object_entry_ptr,
                                                              database_user_entry_t* user_entry_ptr, 
                                                              database_edge_entry_t* edge_entry_ptr, 
                                                              fbe_u32_t object_id,
                                                              fbe_database_service_t *database_service_ptr);
static void fbe_database_refactor_tables_for_bad_user_entry(database_object_entry_t* object_entry_ptr, 
                                                            database_user_entry_t* user_entry_ptr,
                                                            database_edge_entry_t* edge_entry_ptr, 
                                                            fbe_u32_t object_id,
                                                            fbe_database_service_t *database_service_ptr);
static void fbe_database_refactor_tables_for_bad_edge_entry(database_object_entry_t* object_entry_ptr, 
                                                            database_edge_entry_t* edge_entry_ptr, 
                                                            fbe_u32_t object_id,
                                                            fbe_database_service_t *database_service_ptr);

static fbe_status_t database_homewrecker_invalidate_pvd_sn(fbe_database_service_t* database_service_ptr, fbe_object_id_t  pvd_object_id);

static fbe_char_t* database_homewrecker_disk_type_str(fbe_homewrecker_system_disk_type_t *disk_type_enum);

static void database_homewrecker_bootup_with_event(homewrecker_system_disk_table_t* sdt);
static fbe_status_t fbe_database_system_lun_set_ndb_flags(fbe_object_id_t object_id, fbe_bool_t ndb);

/*!**************************************************************************************************
@fn fbe_database_init_persist()
*****************************************************************************************************

* @brief
*  This function sets the LU the persist service will use. it waits for the completion before returns. 
*  todo: may want to make it asynch later. 
*
* @return
*  fbe_status_t
*
*****************************************************************************************************/
fbe_status_t fbe_database_init_persist(void)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_object_id_t db_lun_id = FBE_OBJECT_ID_INVALID;
    fbe_lba_t       required_lun_size;
    fbe_u32_t       wait_count = 0;

    /*we'll initialize it only on the active side and if the active side dies, we'll call it when we get the event on the peer*/
    if (!database_common_cmi_is_active()) {
        return FBE_STATUS_OK;
    }
    /* Validate the lun size */
    status = fbe_database_persist_get_required_lun_size(&required_lun_size);
    if (status != FBE_STATUS_OK) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: get lun size failure %d \n",
                        __FUNCTION__, status);
        return status;
    }
    if (database_system_objects_get_db_lun_capacity() < required_lun_size) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: DB lun is too small, 0x%llX < 0x%llX blocks (required by persist)\n",
                        __FUNCTION__, 
                        database_system_objects_get_db_lun_capacity(),
                        required_lun_size);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_semaphore_init(&init_persist_context.sem, 0, 1);
    database_system_objects_get_db_lun_id(&db_lun_id);
    
    /* Retry set-persistent LUN up to 0.5 minutes when failure occurred. */
    do {
        fbe_database_service.journal_replayed = FBE_TRI_STATE_INVALID;
        fbe_database_persist_set_lun(db_lun_id,
                                     database_set_db_lun_completion, 
                                     &init_persist_context);
    
        /*
          set LUN not only set LUN, but also loads entries and replay journal. 
          So consider it a IO operation and the timeout should be handled like a 
          IO hang:
          1) if timeout,  return directly, don't release resources (void panic)
          2) if other error, cleanup and retry the set LUN
        */
        /* Extend the waiting time from 4 minutes to 6 minutes. 
         * We encoutnered a case which last about 5 minutes to finish the set LUN operation.
         * Refer to AR 572360.
         * */
        status = fbe_semaphore_wait_ms(&init_persist_context.sem, 360000); 

        if (status != FBE_STATUS_OK) {
            /*
             Timeout means we don't want to continue wait IO come back, Not mean it is guranteed 
             not come back;  So just return failure, don't release any resources that the IO
             complete routine will access.  This may cuase panic or write other's buffer.
             we don't retry a timeout IO either here
            */
            database_trace (FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "db_init_persist: waiting semaphore failed\n");
                            
            return FBE_STATUS_GENERIC_FAILURE;
        } else {
            /*wait success, so the init_persist_context has the correct state info*/
        if (init_persist_context.commit_status != FBE_STATUS_OK) 
        {
            /* Increment wait count */
            wait_count++;

                /* Wait 4 second before retry again */
                fbe_thread_delay(4000); 
                
                /*cleanup the effects of setting LUN operation here and retry it*/                                   
                status = fbe_database_persist_unset_lun();
                if (status != FBE_STATUS_OK) {
                    /*unset LUN failed, so break the retry loop to return failure*/
                    database_trace (FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "db_init_persist: Failed unset-lun %d(0x%x). Sts=0x%x, wait_count=%d.\n", 
                                    db_lun_id, db_lun_id, init_persist_context.commit_status, wait_count);
                    break;
                }

                database_trace (FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "db_init_persist: Retry. Failed set-lun %d(0x%x). Sts=0x%x, wait_count=%d.\n", 
                                db_lun_id, db_lun_id, init_persist_context.commit_status, wait_count);

            }
        }

    } while ((init_persist_context.commit_status != FBE_STATUS_OK) && (wait_count < 10));

    /* Only when the completion funciton is called, we can release the semaphore.
       The timeout case will not reach this line, so destroy the semaphore safely here. 
     */
    fbe_semaphore_destroy(&init_persist_context.sem);

    if (init_persist_context.commit_status != FBE_STATUS_OK)
    {
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "db_init_persist: Failed Retry Set-lun %d(0x%x). Sts=0x%x, wait_count=%d.\n", 
                        db_lun_id, db_lun_id, init_persist_context.commit_status, wait_count);
    }

    database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_INFO, 
                   "%s: end\n", __FUNCTION__); 

    return init_persist_context.commit_status;
}

static fbe_status_t database_set_db_lun_completion(fbe_status_t commit_status, fbe_persist_completion_context_t context)
{
    database_init_persist_context_t *        init_persist = (database_init_persist_context_t *)context;

    if (init_persist != NULL) {
        init_persist->commit_status = commit_status;
        fbe_semaphore_release(&init_persist->sem, 0, 1, FBE_FALSE);
    }
    return FBE_STATUS_OK;
}

/*!**************************************************************************************************
@fn fbe_database_load_from_persist()
*****************************************************************************************************

* @brief
*  This function reads from persist service and polulate the service tables. 
*
* @return
*  fbe_status_t
*
*****************************************************************************************************/
static fbe_status_t fbe_database_load_from_persist(void)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    database_config_table_type_t    table_type;

    for(table_type = DATABASE_CONFIG_TABLE_INVALID+1; table_type < DATABASE_CONFIG_TABLE_LAST; table_type ++) {
        status = fbe_database_load_from_persist_per_type(table_type);
        if (status != FBE_STATUS_OK) {
            break;
        }
    }

    return status;
}

/*!**************************************************************************************************
@fn fbe_database_load_from_persist()
*****************************************************************************************************

* @brief
*  This function reads from persist service and polulate the service tables per specified type. 
*
* @return
*  fbe_status_t
*
*****************************************************************************************************/
static fbe_status_t fbe_database_load_from_persist_per_type(database_config_table_type_t table_type)
{
    fbe_status_t                     status = FBE_STATUS_OK;
    fbe_status_t                     wait_status = FBE_STATUS_OK;
    fbe_u8_t                       * read_buffer;
    database_init_read_context_t   * read_context;
    fbe_u32_t                        total_entries_read = 0;
    fbe_persist_sector_type_t        persist_sector_type;

    read_context = (database_init_read_context_t *)fbe_memory_native_allocate(sizeof(database_init_read_context_t));
    if (read_context == NULL) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Can't allocate read_context\n",
            __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_semaphore_init(&read_context->sem, 0, 1);
 
    read_buffer = (fbe_u8_t *)fbe_memory_ex_allocate(FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE);
    if (read_buffer == NULL) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Can't allocate read_buffer\n",
            __FUNCTION__);
        fbe_memory_native_release(read_context);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    database_trace (FBE_TRACE_LEVEL_INFO,
        FBE_TRACE_MESSAGE_ID_INFO,
        "%s: Load Table 0x%x\n",
        __FUNCTION__,
        table_type);

    /* look up the sector type */
    persist_sector_type = database_common_map_table_type_to_persist_sector(table_type);
    if (persist_sector_type != FBE_PERSIST_SECTOR_TYPE_INVALID) {
        /*start to read from the first one*/
        read_context->next_entry_id = FBE_PERSIST_SECTOR_START_ENTRY_ID;
        do {
            status = fbe_database_persist_read_sector(persist_sector_type,
                read_buffer,
                FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE,
                read_context->next_entry_id,
                fbe_database_load_from_persist_read_completion_function,
                (fbe_persist_completion_context_t)read_context);
            if(status == FBE_STATUS_GENERIC_FAILURE)
            {
                /* log the error */
                database_trace (FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "Read from sector: %d failed! next_entry 0x%llX\n",
                                persist_sector_type,
                                (unsigned long long)read_context->next_entry_id );
            }

            wait_status = fbe_semaphore_wait_ms(&read_context->sem, 30000);
            if(wait_status == FBE_STATUS_TIMEOUT)
            {
                /* log the error */
                database_trace (FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "Read from sector: %d TIMEOUT! next_entry 0x%llX\n",
                                persist_sector_type,
                                (unsigned long long)read_context->next_entry_id );
            }
            if (read_context->status == FBE_STATUS_IO_FAILED_RETRYABLE) 
            {
                fbe_semaphore_destroy(&read_context->sem);
                fbe_memory_native_release(read_context);
                fbe_memory_ex_release(read_buffer);
                return FBE_STATUS_IO_FAILED_RETRYABLE;
            }
            if (read_context->next_entry_id != FBE_PERSIST_NO_MORE_ENTRIES_TO_READ) {
                database_trace (FBE_TRACE_LEVEL_DEBUG_LOW,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "Read %d entries from sector: %d up to entry 0x%llX\n",
                                read_context->entries_read,
                                persist_sector_type,
                                (unsigned long long)(read_context->next_entry_id-1));
            }else{
                database_trace (FBE_TRACE_LEVEL_DEBUG_LOW,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "Read %d entries from sector: %d up to sector end\n",
                                read_context->entries_read,
                                persist_sector_type);
            }

            /*now let's copy the read buffer content to the service table */
            status = database_copy_entries_from_persist_read(read_buffer, read_context->entries_read, table_type);
            if (status != FBE_STATUS_OK) {
                /* Error happened when loading the database, quit and hault the system in this state*/
                fbe_semaphore_destroy(&read_context->sem);
                fbe_memory_native_release(read_context);
                fbe_memory_ex_release(read_buffer);
                return status;
            }

            /*increment only AFTER copying*/
            total_entries_read+= read_context->entries_read;
        } while (read_context->next_entry_id != FBE_PERSIST_NO_MORE_ENTRIES_TO_READ);
    }

    fbe_semaphore_destroy(&read_context->sem);
    fbe_memory_native_release(read_context);
    fbe_memory_ex_release(read_buffer);

    return status;
}

static fbe_status_t fbe_database_load_from_persist_read_completion_function(fbe_status_t read_status, 
                                                                            fbe_persist_entry_id_t next_entry, 
                                                                            fbe_u32_t entries_read, 
                                                                            fbe_persist_completion_context_t context)
{
    database_init_read_context_t    * read_context = (database_init_read_context_t    *)context;

    if (read_status != FBE_STATUS_OK)
    {    
        /* log error*/
        if(read_status == FBE_STATUS_IO_FAILED_RETRYABLE) {
            read_context->status = read_status;
            fbe_semaphore_release(&read_context->sem, 0, 1, FALSE);
            return FBE_STATUS_OK;
        }  
    }
    read_context->next_entry_id= next_entry;
    read_context->entries_read = entries_read;
    read_context->status = FBE_STATUS_OK;
    fbe_semaphore_release(&read_context->sem, 0, 1, FALSE);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_database_load_from_persist_read_single_entry_completion_function(fbe_status_t read_status,
                                                                                         fbe_persist_completion_context_t context,
                                                                                         fbe_persist_entry_id_t next_entry)
{
    database_init_read_context_t	* read_context = (database_init_read_context_t	*)context;
    
    if (read_status != FBE_STATUS_OK)
    {	
        /* log error*/
        if(read_status == FBE_STATUS_IO_FAILED_RETRYABLE) {
            read_context->status = read_status;
            read_context->next_entry_id= next_entry;
            fbe_semaphore_release(&read_context->sem, 0, 1, FALSE);
            return FBE_STATUS_OK;
        }       
    }
    read_context->next_entry_id= next_entry;
    read_context->entries_read = 1;
    read_context->status = FBE_STATUS_OK;
    fbe_semaphore_release(&read_context->sem, 0, 1, FALSE);
    return FBE_STATUS_OK;
}


/*!**************************************************************************************************
@fn fbe_database_load_from_persist_single_entry()
*****************************************************************************************************

* @brief
*  This function reads from persist service entry by entry and polulate the service tables. It will be 
*  called when fbe_database_load_from_persist() failed with block error.
*
* @return
*  fbe_status_t
*
*****************************************************************************************************/
static fbe_status_t fbe_database_load_from_persist_single_entry(fbe_database_service_t *database_service_ptr)
{
    fbe_status_t 					status = FBE_STATUS_OK;
    database_config_table_type_t    table_type;

    for(table_type = DATABASE_CONFIG_TABLE_INVALID+1; table_type < DATABASE_CONFIG_TABLE_LAST; table_type ++){
         status = fbe_database_load_from_persist_single_entry_per_type(table_type);
         if (status != FBE_STATUS_OK) {
             /* stop here */
             break;
         }
    } 

    /*after load from persist with bad entry, we need to refactor db tables*/
    fbe_database_refactor_tables_for_bad_entry(database_service_ptr);

    return status; 
}

/*!**************************************************************************************************
@fn fbe_database_load_from_persist_single_entry_per_type()
*****************************************************************************************************

* @brief
*  This function reads from persist service by entry and polulate the service table. 
*
* @return
*  fbe_status_t
*
*****************************************************************************************************/
static fbe_status_t fbe_database_load_from_persist_single_entry_per_type(database_config_table_type_t table_type)
{
    fbe_status_t 					status = FBE_STATUS_OK;
    fbe_status_t                    wait_status = FBE_STATUS_OK;
    fbe_u8_t 					  * read_buffer;
    database_init_read_context_t  * read_context;
    fbe_u32_t					    total_entries_read = 0;
    fbe_persist_sector_type_t		persist_sector_type;

    read_context = (database_init_read_context_t *)fbe_memory_native_allocate(sizeof(database_init_read_context_t));
    if (read_context == NULL) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Can't allocate read_context\n",
                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_semaphore_init(&read_context->sem, 0, 1); 

    read_buffer = (fbe_u8_t *)fbe_memory_ex_allocate(FBE_PERSIST_DATA_BYTES_PER_ENTRY);
    if (read_buffer == NULL) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Can't allocate read_buffer\n",
                        __FUNCTION__);
        fbe_memory_native_release(read_context);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: Load Table 0x%x\n",
                    __FUNCTION__,
                    table_type);

    /* look up the sector type */
    persist_sector_type = database_common_map_table_type_to_persist_sector(table_type);
    if (persist_sector_type != FBE_PERSIST_SECTOR_TYPE_INVALID) {
        /* start to read from the first one */
        read_context->next_entry_id = FBE_PERSIST_SECTOR_START_ENTRY_ID;
        do {
            status = fbe_database_persist_read_entry(read_context->next_entry_id,
                                                     persist_sector_type,
                                                     read_buffer,
                                                     FBE_PERSIST_DATA_BYTES_PER_ENTRY,
                                                     fbe_database_load_from_persist_read_single_entry_completion_function,
                                                     (fbe_persist_completion_context_t)read_context);
            if(status == FBE_STATUS_GENERIC_FAILURE) 
            {
                /* log the error */
                database_trace (FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Read from sector: %d failed! next_entry 0x%llX\n",
                                __FUNCTION__,
                                persist_sector_type,
                                (unsigned long long)read_context->next_entry_id );
            }

            wait_status = fbe_semaphore_wait_ms(&read_context->sem, 30000);
            if(wait_status == FBE_STATUS_TIMEOUT)
            {
                /* log the error */
                database_trace (FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Read from sector: %d TIMEOUT! next_entry 0x%llX\n",
                                __FUNCTION__,
                                persist_sector_type,
                                (unsigned long long)read_context->next_entry_id );
            }   
            /* if we met an entry with bad blcok, we will continue to read other good entries */  
            if (read_context->status == FBE_STATUS_IO_FAILED_RETRYABLE) 
            {
                continue; 
            }   
            if (read_context->next_entry_id != FBE_PERSIST_NO_MORE_ENTRIES_TO_READ) {
                database_trace (FBE_TRACE_LEVEL_DEBUG_LOW,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Read %d entries from sector: %d up to entry 0x%llX\n",
                                __FUNCTION__,
                                read_context->entries_read,
                                persist_sector_type,
                                (unsigned long long)(read_context->next_entry_id-1));
            }else{
                database_trace (FBE_TRACE_LEVEL_DEBUG_LOW,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Read %d entries from sector: %d up to sector end\n",
                                __FUNCTION__,
                                read_context->entries_read,
                                persist_sector_type);
            }
    
            /*now let's copy the read buffer content to the service table */
            status = database_copy_single_entry_from_persist_read(read_buffer, table_type);
            if (status != FBE_STATUS_OK) {

                database_trace (FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: failed copy entry to tbale, next entry:0x%llX\n",
                                __FUNCTION__,
                (unsigned long long)read_context->next_entry_id);

                /* Error happened when loading the database, quit and hault the system in this state*/
                fbe_semaphore_destroy(&read_context->sem);
                fbe_memory_native_release(read_context);
                fbe_memory_ex_release(read_buffer);
                return status;
            }  

            /*increment only AFTER copying*/
            total_entries_read+= read_context->entries_read;
        } while (read_context->next_entry_id != FBE_PERSIST_NO_MORE_ENTRIES_TO_READ);
    }

    fbe_semaphore_destroy(&read_context->sem);
    fbe_memory_native_release(read_context);
    fbe_memory_ex_release(read_buffer);

    return status; 
}


/*!**************************************************************************************************
@fn fbe_database_refactor_tables_for_bad_entry()
*****************************************************************************************************

* @brief
*  This function refactors the database tables while finding that there exists bad entries with 
*  bad block. 
*
* @return
*  fbe_status_t
*
*****************************************************************************************************/
static fbe_status_t fbe_database_refactor_tables_for_bad_entry(fbe_database_service_t *database_service_ptr){
    fbe_status_t 					status = FBE_STATUS_OK;
    fbe_object_id_t                 object_id;
    database_config_table_type_t	table_type;
    database_table_t              * user_table_ptr = NULL;
    database_table_t              * object_table_ptr = NULL;
    database_table_t              * edge_table_ptr = NULL;
    database_object_entry_t       * object_entry_ptr = NULL;
    database_edge_entry_t         * edge_entry_ptr = NULL;
    database_user_entry_t         * user_entry_ptr = NULL;
    //database_global_info_type_t     global_info_type;

    /* get object, edge, user table pointer */
    for(table_type = DATABASE_CONFIG_TABLE_INVALID+1; table_type < DATABASE_CONFIG_TABLE_LAST; table_type++){
        switch (table_type) {
        case DATABASE_CONFIG_TABLE_USER:
            status = fbe_database_get_service_table_ptr(&user_table_ptr, table_type);
            break;
        case DATABASE_CONFIG_TABLE_OBJECT:
            status = fbe_database_get_service_table_ptr(&object_table_ptr, table_type);
            break;
        case DATABASE_CONFIG_TABLE_EDGE:
            status = fbe_database_get_service_table_ptr(&edge_table_ptr, table_type);
            break;
        default:
            database_trace(FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "%s: skip table = 0x%x\n",
                      __FUNCTION__, table_type);
            status = FBE_STATUS_OK;
            break;
        }
        if ((status != FBE_STATUS_OK)) {
            database_trace(FBE_TRACE_LEVEL_ERROR,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "%s: get_service_table_ptr failed, status = 0x%x\n",
                      __FUNCTION__, status);
             return status;
        }
    }

    /*refactor the object, user, edge tables.
      No need to refactor the global info table since it will be set to system default value if global info table corrupt*/ 
    for (object_id = FBE_RESERVED_OBJECT_IDS; object_id < (object_table_ptr->table_size - FBE_RESERVED_OBJECT_IDS); object_id++ ) {
        /* get the current object, user, edge entry pointer */
        object_entry_ptr = &object_table_ptr->table_content.object_entry_ptr[object_id];
        user_entry_ptr = &user_table_ptr->table_content.user_entry_ptr[object_id];
        edge_entry_ptr = &edge_table_ptr->table_content.edge_entry_ptr[object_id*DATABASE_MAX_EDGE_PER_OBJECT];

        /* detect bad object entry*/
        if (object_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_INVALID){
            if ((user_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID) || (object_entry_ptr->db_class_id == DATABASE_CLASS_ID_VIRTUAL_DRIVE)){   
                fbe_database_refactor_tables_for_bad_object_entry(object_entry_ptr, user_entry_ptr, edge_entry_ptr, object_id,database_service_ptr);
            }
        }
        /* detect the potential bad user entry or edge entry */
        else if ((object_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID) && (user_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_INVALID)){
            if (object_entry_ptr->db_class_id == DATABASE_CLASS_ID_VIRTUAL_DRIVE) {
                fbe_database_refactor_tables_for_bad_edge_entry(object_entry_ptr, edge_entry_ptr, object_id,database_service_ptr);
            }
            else{
                fbe_database_refactor_tables_for_bad_user_entry(object_entry_ptr,user_entry_ptr, edge_entry_ptr, object_id,database_service_ptr);
            }      
        }
        /* detect the potential bad edge entry */
        else if ((object_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID) && (user_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID)){
            fbe_database_refactor_tables_for_bad_edge_entry(object_entry_ptr, edge_entry_ptr, object_id,database_service_ptr);
        }
    }
    
    return status; 
} 

static void fbe_database_refactor_tables_for_bad_object_entry(database_object_entry_t* object_entry_ptr,
                                                              database_user_entry_t* user_entry_ptr, 
                                                              database_edge_entry_t* edge_entry_ptr, 
                                                              fbe_u32_t object_id,
                                                              fbe_database_service_t *database_service_ptr){ 
    fbe_u32_t                       edge_index;                                                                                     

    /* pvd doesn't have edges.*/ 
    if (user_entry_ptr->db_class_id == DATABASE_CLASS_ID_PROVISION_DRIVE) {
        database_trace(FBE_TRACE_LEVEL_INFO,                                                                                                                                                  
                       FBE_TRACE_MESSAGE_ID_INFO,                                                                                                                                             
                       "%s: Find the corrupt object 0x%x in object table, then update the corresponding user table and edge table\n",                                                                              
                       __FUNCTION__,object_id); 
        object_entry_ptr->header.state = DATABASE_CONFIG_ENTRY_CORRUPT;
        user_entry_ptr->header.state = DATABASE_CONFIG_ENTRY_CORRUPT;  
        set_database_service_state(database_service_ptr, FBE_DATABASE_STATE_CORRUPT);                                                                                       
    } 
    else{ 
        for (edge_index = 0; edge_index < DATABASE_MAX_EDGE_PER_OBJECT; edge_index++) {                                                                                                               
            if (edge_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID) {                                              
                // the invalid object, edge entry combination which is that                                                                                                                           
                //object state is invalid while user and edge states are valid                                                                                                                        
                database_trace(FBE_TRACE_LEVEL_INFO,                                                                                                                                                  
                               FBE_TRACE_MESSAGE_ID_INFO,                                                                                                                                             
                               "%s: Find the corrupt object 0x%x in object table.\n",                                                                              
                               __FUNCTION__,object_id);                                                                                                                                                         
                object_entry_ptr->header.state = DATABASE_CONFIG_ENTRY_CORRUPT;
                user_entry_ptr->header.state = DATABASE_CONFIG_ENTRY_CORRUPT; 
                set_database_service_state(database_service_ptr, FBE_DATABASE_STATE_CORRUPT);     
                break;                                                                                                                                                                                                                                  
            }
            edge_entry_ptr++;                                                                                                                                                                                         
        }  
    }            
}

static void fbe_database_refactor_tables_for_bad_user_entry(database_object_entry_t* object_entry_ptr,
                                                            database_user_entry_t* user_entry_ptr, 
                                                            database_edge_entry_t* edge_entry_ptr, 
                                                            fbe_u32_t object_id,
                                                            fbe_database_service_t *database_service_ptr){
    fbe_u32_t                       edge_index;
    /* pvd doesn't have edges.*/ 
    if (object_entry_ptr->db_class_id == DATABASE_CLASS_ID_PROVISION_DRIVE) {
        database_trace(FBE_TRACE_LEVEL_INFO,                                                                                                                                                  
                       FBE_TRACE_MESSAGE_ID_INFO,                                                                                                                                             
                       "%s: Find the corrupt object 0x%x in user table. We create the object, while not commit it.\n",                                                                              
                       __FUNCTION__,object_id);
        user_entry_ptr->header.state = DATABASE_CONFIG_ENTRY_CORRUPT;
        object_entry_ptr->header.state = DATABASE_CONFIG_ENTRY_CORRUPT;   
        set_database_service_state(database_service_ptr, FBE_DATABASE_STATE_CORRUPT);                                                                                         
    }
    else if (object_entry_ptr->db_class_id != DATABASE_CLASS_ID_VIRTUAL_DRIVE){ 
        for (edge_index = 0; edge_index < DATABASE_MAX_EDGE_PER_OBJECT; edge_index++) {                                                                                                               
            if (edge_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID) {                                              
                // the invalid object, edge entry combination which is that                                                                                                                           
                //object state is invalid while user and edge states are valid                                                                                                                        
                database_trace(FBE_TRACE_LEVEL_INFO,                                                                                                                                                  
                               FBE_TRACE_MESSAGE_ID_INFO,                                                                                                                                             
                               "%s: Find the corrupt object 0x%x in user table. We create the object, while not commit it.\n",                                                                              
                               __FUNCTION__,object_id);                                                                                                                                                                          
                user_entry_ptr->header.state = DATABASE_CONFIG_ENTRY_CORRUPT;   
                object_entry_ptr->header.state = DATABASE_CONFIG_ENTRY_CORRUPT;   
                set_database_service_state(database_service_ptr, FBE_DATABASE_STATE_CORRUPT);                                                                                                                                                                                                                                
            }
            edge_entry_ptr++;                                                                                                                                                                                         
        }  
    }  
}

static void fbe_database_refactor_tables_for_bad_edge_entry(database_object_entry_t* object_entry_ptr, 
                                                            database_edge_entry_t* edge_entry_ptr, 
                                                            fbe_u32_t object_id,
                                                            fbe_database_service_t *database_service_ptr){
    fbe_u32_t                       edge_index;
    fbe_u32_t                       edge_count = 0;

    if (object_entry_ptr->db_class_id != DATABASE_CLASS_ID_PROVISION_DRIVE){
        for (edge_index = 0; edge_index < DATABASE_MAX_EDGE_PER_OBJECT; edge_index++) {
            if(edge_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID) {
                edge_count++;
            }
            edge_entry_ptr++;
        }
        if (object_entry_ptr->db_class_id == DATABASE_CLASS_ID_MIRROR ||
            object_entry_ptr->db_class_id == DATABASE_CLASS_ID_STRIPER ||
            object_entry_ptr->db_class_id == DATABASE_CLASS_ID_PARITY){   
            /* raid group's edge # should be rg_config.width if not, this raid group is degraded.
               even if there are no edges connect to rg, rg will take care of itself. */
            if(edge_count != object_entry_ptr->set_config_union.rg_config.width) {
                database_trace(FBE_TRACE_LEVEL_INFO,                                                                                                                                                  
                               FBE_TRACE_MESSAGE_ID_INFO,                                                                                                                                             
                               "%s: Find the degraded raid group 0x%x with %d edges, while it's supposed to have %d edges.\n",                                                                              
                               __FUNCTION__,object_id,edge_count, object_entry_ptr->set_config_union.rg_config.width);   
                set_database_service_state(database_service_ptr, FBE_DATABASE_STATE_CORRUPT);    
            }
        }
        /* VD or LUN should have one edge. if there is no edge, this VD or LUN is bad.*/
        if (object_entry_ptr->db_class_id == DATABASE_CLASS_ID_LUN || 
            object_entry_ptr->db_class_id == DATABASE_CLASS_ID_VIRTUAL_DRIVE){
            if(edge_count != 1) {
                database_trace(FBE_TRACE_LEVEL_INFO,                                                                                                                                                  
                               FBE_TRACE_MESSAGE_ID_INFO,                                                                                                                                             
                               "%s: Find the object 0x%x without enough edge in edge table.\n",                                                                              
                               __FUNCTION__,object_id);  
                set_database_service_state(database_service_ptr, FBE_DATABASE_STATE_CORRUPT);       
            }
        }
    }
}

/*!**************************************************************************************************
@fn fbe_database_refactor_tables_for_bad_entry_corrupt_clients()
*****************************************************************************************************

* @brief
*  This function find the upstream edge of the given object and corrupt its client objects
*  bad block. 
* 
* @param  database_table_t* object_table_ptr  
*         database_table_t* edge_table_ptr
*         fbe_u32_t         object_id        
* 
* @return
*  fbe_status_t
*
*****************************************************************************************************/
static fbe_status_t fbe_database_refactor_tables_for_bad_entry_corrupt_clients(fbe_u32_t object_id){
    fbe_status_t 					status = FBE_STATUS_OK;
    fbe_u32_t                       edge_index; 
    fbe_object_id_t                 i;
    database_config_table_type_t	table_type;
    database_table_t              * object_table_ptr = NULL;
    database_table_t              * edge_table_ptr = NULL;


    /* get object, edge table pointer */
    for(table_type = DATABASE_CONFIG_TABLE_INVALID+1; table_type < DATABASE_CONFIG_TABLE_LAST; table_type++){
        switch (table_type) {
        case DATABASE_CONFIG_TABLE_OBJECT:
            status = fbe_database_get_service_table_ptr(&object_table_ptr, table_type);
            break;
        case DATABASE_CONFIG_TABLE_EDGE:
            status = fbe_database_get_service_table_ptr(&edge_table_ptr, table_type);
            break;
        }
        if ((status != FBE_STATUS_OK)) {
            database_trace(FBE_TRACE_LEVEL_ERROR,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "%s: get_service_table_ptr failed, status = 0x%x\n",
                      __FUNCTION__, status);
            return status;
        }
    }

    /* For lun object or virtual drive, there are no clients to corrupt */
    if (object_table_ptr->table_content.object_entry_ptr[object_id].db_class_id == DATABASE_CLASS_ID_LUN) {
        return status;
    } 
    else {
        for (i = FBE_RESERVED_OBJECT_IDS; i < (object_table_ptr->table_size - FBE_RESERVED_OBJECT_IDS); i++) { 
            for (edge_index = 0; edge_index < DATABASE_MAX_EDGE_PER_OBJECT; edge_index++) {
                // Find the upstream edge of the object with object id and corrupt the client objects.                                                                               
                if (object_id == edge_table_ptr->table_content.edge_entry_ptr[i*DATABASE_MAX_EDGE_PER_OBJECT + edge_index].server_id) {
                    if (object_table_ptr->table_content.object_entry_ptr[object_id].db_class_id == DATABASE_CLASS_ID_VIRTUAL_DRIVE)
                    {
                        edge_table_ptr->table_content.edge_entry_ptr[i*DATABASE_MAX_EDGE_PER_OBJECT + edge_index].header.state = DATABASE_CONFIG_ENTRY_CORRUPT;
                    }
                    else{
                        database_trace(FBE_TRACE_LEVEL_INFO,                                                                                                                                          
                                       FBE_TRACE_MESSAGE_ID_INFO,                                                                                                                                     
                                   "%s: Find the object 0x%x which are the client of the corrupt object 0x%x in object table, corrupt object 0x%x.\n",                                          
                                       __FUNCTION__, i, object_id, i);                                                                                                                                                 
                    
                        object_table_ptr->table_content.object_entry_ptr[i].header.state = DATABASE_CONFIG_ENTRY_CORRUPT;   
                        break; 
                    }
                }                                                                                                                                                                                  
            }
        }
    }
    
    
    return status;
}


static fbe_status_t database_copy_entries_from_persist_read(fbe_u8_t * read_buffer, 
                                                            fbe_u32_t elements_read, 
                                                            database_config_table_type_t table_type)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t *                             current_data = read_buffer;
    fbe_u32_t                            elements = 0;
    fbe_persist_user_header_t *            user_header = NULL;
    database_table_header_t *            database_table_header = NULL;
    database_table_t *                     table_ptr = NULL;
    database_object_entry_t *             object_entry_ptr = NULL;
    database_edge_entry_t *             edge_entry_ptr = NULL;
    database_user_entry_t *             user_entry_ptr = NULL;
    database_global_info_entry_t *         global_info_entry_ptr = NULL;
    database_system_spare_entry_t *     system_spare_entry_ptr = NULL;


    status = fbe_database_get_service_table_ptr(&table_ptr, table_type);
    if ((status != FBE_STATUS_OK)||(table_ptr == NULL)) {
        return status;
    }

    /*let's go over the read buffer and copy the data to service table */
    for (elements = 0; elements < elements_read; elements++) {
        user_header = (fbe_persist_user_header_t *)current_data;
        current_data += sizeof(fbe_persist_user_header_t);
        /* Sanity check the entry id */
        database_table_header = (database_table_header_t *)current_data;
        if (user_header->entry_id != database_table_header->entry_id) {
            /* We expect persist entry id to be the same as the entry id stored in the database table.
            * If not, log an ERROR as the DB is corrupted. 
            * Add an event log and set the database as corrupted.
            * To fix the corrupted database, find out the corrupted entry. 
            * We can try to fix it fbecli: odt or rdt to modify the data on disk.
            */ 
            database_trace (FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: PersistId 0x%llx != TableId 0x%llx. We have a corrupted database!\n",
                __FUNCTION__,
                (unsigned long long)user_header->entry_id,
                (unsigned long long)database_table_header->entry_id);
            set_database_service_state(&fbe_database_service, FBE_DATABASE_STATE_CORRUPT);                                                                                       
            /*go to next entry*/
            current_data += (FBE_PERSIST_USER_DATA_BYTES_PER_ENTRY - sizeof(fbe_persist_user_header_t));
            continue;
        }

        if (database_table_header->state != DATABASE_CONFIG_ENTRY_VALID) {
            /* we expect a valid entry. The trace level is low 
             * because many enties will go through this switch*/
            database_trace (FBE_TRACE_LEVEL_DEBUG_HIGH,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: elem: %d) Entry id 0x%llx has unexpected state 0x%x. Skip it!\n",
                __FUNCTION__, elements,
                (unsigned long long)database_table_header->entry_id,
                database_table_header->state);
            /*go to next entry*/
            current_data += (FBE_PERSIST_USER_DATA_BYTES_PER_ENTRY - sizeof(fbe_persist_user_header_t));
            continue;
        }
        /* copy the entry to service tables*/
        database_trace (FBE_TRACE_LEVEL_DEBUG_LOW,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Copy Entry id 0x%llx state 0x%x to Table 0x%x!\n",
            __FUNCTION__,
            (unsigned long long)database_table_header->entry_id,
            database_table_header->state,
            table_type);

        switch(table_type) {
        case DATABASE_CONFIG_TABLE_OBJECT:
            object_entry_ptr = (database_object_entry_t *)current_data;
            status = fbe_database_config_table_update_object_entry(table_ptr, object_entry_ptr);
            break;
        case DATABASE_CONFIG_TABLE_EDGE:
            edge_entry_ptr = (database_edge_entry_t *)current_data;
            status = fbe_database_config_table_update_edge_entry(table_ptr, edge_entry_ptr);
            break;
        case DATABASE_CONFIG_TABLE_USER:
            user_entry_ptr = (database_user_entry_t *)current_data;
            status = fbe_database_config_table_update_user_entry(table_ptr, user_entry_ptr);
            break;
        case DATABASE_CONFIG_TABLE_GLOBAL_INFO:
            global_info_entry_ptr = (database_global_info_entry_t *)current_data;
            status = fbe_database_config_table_update_global_info_entry(table_ptr, global_info_entry_ptr); 
            break;
        case DATABASE_CONFIG_TABLE_SYSTEM_SPARE:
            system_spare_entry_ptr = (database_system_spare_entry_t *)current_data;
            status = fbe_database_config_table_update_system_spare_entry(table_ptr, system_spare_entry_ptr);
            break;

        default:
            database_trace (FBE_TRACE_LEVEL_WARNING,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: Table type:%d is not supported!\n",
                            __FUNCTION__,
                            table_type);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
        }
        if (status != FBE_STATUS_OK) {
            database_trace (FBE_TRACE_LEVEL_WARNING,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: Failed to update Entry id 0x%llx of table type:%d, status 0x%x!\n",
                __FUNCTION__,
                (unsigned long long)database_table_header->entry_id,
                table_type,
                status);
        }
        /*go to next entry*/
        current_data += (FBE_PERSIST_USER_DATA_BYTES_PER_ENTRY - sizeof(fbe_persist_user_header_t));
    }
    return FBE_STATUS_OK;
}

/*!**************************************************************************************************
@fn database_homewrecker_enter_service_mode()
*****************************************************************************************************

* @brief
*  Enter service mode with different event log printed in Homewrecker.
* 
* @param msg_id- event log Message ID.
* @param context- input pointer for additional parameter depend on different event.
* 
* 
* @return
*  fbe_status_t
*
*****************************************************************************************************/

fbe_status_t database_homewrecker_send_event_log(fbe_u32_t msg_id, event_log_param_selector_t* param)
{
    /*send correspond event log*/
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    
    switch (msg_id)
    {
        case SEP_WARN_SYSTEM_DISK_NEED_OPERATION:
            if (param == NULL || param->normal_boot_with_warning == NULL || 
                param->normal_boot_with_warning->event_str == NULL )
            {
                database_trace (FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: Parameter error\n",
                                __FUNCTION__);
                break;
            }
            status = fbe_event_log_write(SEP_WARN_SYSTEM_DISK_NEED_OPERATION,
                                         NULL, 0, "%d %s", param->normal_boot_with_warning->slot,
                                         param->normal_boot_with_warning->event_str);
            break;
            
        case SEP_ERROR_CODE_DB_DRIVES_DISORDERED_ERROR:
            if (param == NULL)
            {
                database_trace (FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: Parameter error\n",
                                __FUNCTION__);
                break;
            }
            status = fbe_event_log_write(SEP_ERROR_CODE_DB_DRIVES_DISORDERED_ERROR,
                                         NULL, 0, "%d %d %d %d", 
                                         param->disordered_slot[0], param->disordered_slot[1], 
                                         param->disordered_slot[2], param->disordered_slot[3]);
            break;
            
        case SEP_ERROR_CODE_INTEGRITY_BROKEN_ERROR:
            if (param == NULL || param->integrity_disk_type->slot_type[0] == NULL ||
                param->integrity_disk_type->slot_type[1] == NULL ||
                param->integrity_disk_type->slot_type[2] == NULL ||
                param->integrity_disk_type->slot_type[3] == NULL)
            {
                database_trace (FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: Parameter error\n",
                                __FUNCTION__);
                break;
            }
            status = fbe_event_log_write(SEP_ERROR_CODE_INTEGRITY_BROKEN_ERROR,
                                        NULL, 0, "%s %s %s %s", 
                                        param->integrity_disk_type->slot_type[0],
                                        param->integrity_disk_type->slot_type[1],
                                        param->integrity_disk_type->slot_type[2],
                                        param->integrity_disk_type->slot_type[3]
                                        );
            break;
        
        case SEP_ERROR_CODE_CHASSIS_MISMATCHED_ERROR:
            status = fbe_event_log_write(SEP_ERROR_CODE_CHASSIS_MISMATCHED_ERROR,
                                         NULL, 0, NULL);
            break;

        case SEP_ERROR_CODE_OPERATION_ON_WWNSEED_CHAOS:
            status = fbe_event_log_write(SEP_ERROR_CODE_OPERATION_ON_WWNSEED_CHAOS,
                                         NULL, 0, NULL);
            break;

        case SEP_ERROR_CODE_BIND_USER_DRIVE_IN_SYSTEM_SLOT:
            if (param == NULL || param->disk_in_wrong_slot == NULL)
            {
                database_trace (FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: Parameter error\n",
                                __FUNCTION__);
                break;
            }
            status = fbe_event_log_write(SEP_ERROR_CODE_BIND_USER_DRIVE_IN_SYSTEM_SLOT,
                                                                       NULL,
                                                                       0,
                                                                       "%d %d %d %d",
                                                                       param->disk_in_wrong_slot->disk_pvd_id,
                                                                       param->disk_in_wrong_slot->bus,
                                                                       param->disk_in_wrong_slot->enclosure,
                                                                       param->disk_in_wrong_slot->slot);
            break;

        case SEP_ERROR_CODE_SYSTEM_DRIVE_IN_WRONG_SLOT:
            if (param == NULL || param->disk_in_wrong_slot == NULL)
            {
                database_trace (FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: Parameter error\n",
                                __FUNCTION__);
                break;
            }
	    status = fbe_event_log_write(SEP_ERROR_CODE_SYSTEM_DRIVE_IN_WRONG_SLOT,
                                                                       NULL,
                                                                       0,
                                                                       "%d %d %d %d %d %d %d",
                                                                       param->disk_in_wrong_slot->disk_pvd_id,
                                                                       param->disk_in_wrong_slot->bus,
                                                                       param->disk_in_wrong_slot->enclosure,
                                                                       param->disk_in_wrong_slot->slot,
                                                                       param->disk_in_wrong_slot->its_correct_bus,
                                                                       param->disk_in_wrong_slot->its_correct_encl,
                                                                       param->disk_in_wrong_slot->its_correct_slot);
            break;
        case SEP_ERROR_CODE_TWO_INVALID_SYSTEM_DRIVE_AND_ONE_MORE_OF_THEM_IN_USER_SLOT:
            status = fbe_event_log_write(SEP_ERROR_CODE_TWO_INVALID_SYSTEM_DRIVE_AND_ONE_MORE_OF_THEM_IN_USER_SLOT,
                                         NULL, 0, NULL);
            break;

            
        default:
        
            database_trace (FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: Should not be here\n",
                            __FUNCTION__);
            break;
    }

    if (status != FBE_STATUS_OK)
    {    
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Failed to write event log, msg id: %d\n",
                        __FUNCTION__, msg_id);
    }
    return status;

}

fbe_status_t database_homewrecker_enter_service_mode(fbe_database_service_mode_reason_t service_mode_reason)
{
    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "Homewrecker: enter service mode\n");
    fbe_database_enter_service_mode(service_mode_reason);
    return FBE_STATUS_SERVICE_MODE;
}

fbe_bool_t database_homewrecker_descriptor_order_check(fbe_homewrecker_fru_descriptor_t* fru_descriptor_standard,
                                                                  fbe_homewrecker_fru_descriptor_t* fru_descriptor_disk,
                                                                  fbe_bool_t invalid_disk[])
{
    fbe_u32_t       disk_index = 0;
    fbe_bool_t      sys_disk_disordered = FBE_FALSE;

    if (invalid_disk == NULL)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: illegal parameter\n",
                        __FUNCTION__);
        return sys_disk_disordered = FBE_TRUE;
    }
    
    /*check DB_DRIVES_DISORDERED_ERROR*/
    for (disk_index = 0; disk_index < FBE_FRU_DESCRIPTOR_DB_DRIVE_NUMBER; disk_index++)
    {
        if (invalid_disk[disk_index] == FBE_TRUE)
            continue;
            
        if (fbe_compare_string(fru_descriptor_disk->system_drive_serial_number[disk_index],
                                sizeof(serial_num_t),
                                fru_descriptor_standard->system_drive_serial_number[disk_index],
                                sizeof(serial_num_t), FBE_TRUE))
        {   
            /*not identical*/
            sys_disk_disordered = FBE_TRUE;
            break;
        }
    }

    return sys_disk_disordered;
}


static fbe_bool_t
database_homewrecker_check_path_pvd_pdo(fbe_object_id_t pvd_object_id)
{
    fbe_status_t    status = FBE_STATUS_INVALID;
    fbe_base_config_downstream_edge_list_t          down_edges;
    /* check for connection to LDO*/
    status = fbe_database_send_packet_to_object(FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_EDGE_LIST,
                                                 &down_edges,
                                                 sizeof(down_edges),
                                                 pvd_object_id,
                                                 NULL,  /* no sg list*/
                                                 0,  /* no sg list*/
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 FBE_PACKAGE_ID_SEP_0);
    if (FBE_STATUS_OK != status)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Get pvd 0x%x downstream edge error\n",
                        __FUNCTION__, pvd_object_id);
    }    

    if (FBE_STATUS_OK == status && down_edges.number_of_downstream_edges == 0)
    {
        database_trace (FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Pvd 0x%x has no need to disconnect\n",
                        __FUNCTION__, pvd_object_id);
        return FBE_FALSE;
    }
    else
    {
        
        database_trace (FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Pvd 0x%x has edge connection or some other issue\n",
                        __FUNCTION__, pvd_object_id);
        return FBE_TRUE;
    }
    

}

/*!**************************************************************************************************
@fn database_homewrecker_disconnect_pvd_pdo()
*****************************************************************************************************

* @brief
*  Disconnect the pvd and pdo for specific PVD.
* 
* @param pvd_object_id- the PVD need to disconnect .
*
*
* @return
*  fbe_status_t
*
*****************************************************************************************************/

static fbe_status_t
database_homewrecker_disconnect_pvd_pdo(fbe_object_id_t pvd_object_id)
{
    fbe_status_t    status = FBE_STATUS_INVALID;
    fbe_base_config_downstream_edge_list_t          down_edges;
    /* check for connection to LDO*/
    status = fbe_database_send_packet_to_object(FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_EDGE_LIST,
                                                 &down_edges,
                                                 sizeof(down_edges),
                                                 pvd_object_id,
                                                 NULL,  /* no sg list*/
                                                 0,  /* no sg list*/
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 FBE_PACKAGE_ID_SEP_0);
    if (FBE_STATUS_OK == status && down_edges.number_of_downstream_edges != 1)
    {
        database_trace (FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Pvd 0x%x has no need to disconnect\n",
                        __FUNCTION__, pvd_object_id);
        return FBE_STATUS_OK;
    }
    
    if (FBE_STATUS_OK != status)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Get pvd 0x%x downstream edge error\n",
                        __FUNCTION__, pvd_object_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    status = fbe_database_destroy_edge(pvd_object_id, 0);
    if (FBE_STATUS_OK != status)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Disconnect PVD: 0x%x failed\n",
                        __FUNCTION__, pvd_object_id);
        return status;
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************************************************
@fn database_homewrecker_max_seq_num_disk()
*****************************************************************************************************

* @brief
*  Find the DB disks which has the maximum  sequence number in raw mirror reading.
* 
* @param err_report- status information gathered in raw mirror reading .
* @param invalid_disk- giving the disks which should be ignored.
* @param max_seq_num- return the maximum sequence number, if it's not passing NULL.
*
*
* @return
*  fbe_u32_t- the disk index of maximum sequence number one.
*
*****************************************************************************************************/

static fbe_s32_t 
database_homewrecker_valid_disk_max_seq_num(homewrecker_system_disk_table_t* sdt,
                                                                 fbe_u64_t* max_seq_num)
{
    fbe_s32_t max_index = 0;
    fbe_u32_t index = 0;
    fbe_u64_t seq_num = 0;
    fbe_bool_t all_same = FBE_TRUE; /*True means all valid position have the same seq num*/

    if (sdt == NULL)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: illegal NULL pointer\n",
                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    for (index = 0; index < FBE_FRU_DESCRIPTOR_DB_DRIVE_NUMBER; index++)
    {
        if (sdt[index].is_invalid_system_drive == FBE_TRUE)
            continue;
            
        if (seq_num != 0 && sdt[index].dp_report.seq_number != seq_num)
            all_same = FBE_FALSE;
            
        if (seq_num < sdt[index].dp_report.seq_number)
        {
            seq_num = sdt[index].dp_report.seq_number;
            max_index = index;
        }
    }
    
    if (max_seq_num != NULL)
        *max_seq_num = seq_num;

    if (all_same == FBE_TRUE)
        max_index = -1;
        
    return max_index;
}

/*!**************************************************************************************************
@fn database_homewrecker_verify_magic_str()
*****************************************************************************************************

* @brief
*  Verify signature or descriptor magic string. 
*
* 
* @param sector- choose the magic string to verify, signature or descriptor.
* @param struct_p- pointer to magic string.
*
* @return
*  fbe_bool_t: true for verify ok, false for verify failed.
*
*****************************************************************************************************/

static fbe_bool_t database_homewrecker_verify_magic_str(homewrecker_selector_t sector,
                                                                        void* struct_p)
{
    /*return true means verify ok*/
    if (struct_p == NULL || sector < HOMEWRECKER_SELECTOR_START || sector > HOMEWRECER_SELECTOR_END)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: illegal parameter\n",
                        __FUNCTION__);
        return FBE_FALSE;
     }
    switch(sector)
    {
        case HOMEWRECER_DISK_DESCRIPTOR:
            return !fbe_compare_string(((fbe_homewrecker_fru_descriptor_t*)struct_p)->magic_string, 
                                      FBE_FRU_DESCRIPTOR_MAGIC_STRING_LENGTH,
                                      FBE_FRU_DESCRIPTOR_MAGIC_STRING, FBE_FRU_DESCRIPTOR_MAGIC_STRING_LENGTH, FBE_TRUE);
        case HOMEWRECER_DISK_SIGNATURE:
            return !fbe_compare_string(((fbe_fru_signature_t*)struct_p)->magic_string, 
                                        FBE_FRU_SIGNATURE_MAGIC_STRING_SIZE,
                                        FBE_FRU_SIGNATURE_MAGIC_STRING, FBE_FRU_SIGNATURE_MAGIC_STRING_SIZE, FBE_TRUE);
        default:
            break;
    }

    return FBE_FALSE;
}

static fbe_bool_t database_homewrecker_dp_verify_order(fbe_u32_t slot,
                                                                        fbe_homewrecker_fru_descriptor_t* dp,
                                                                        fbe_homewrecker_fru_descriptor_t* dp_mem)
{
    if (dp == NULL || dp_mem == NULL)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: illegal NULL pointer\n",
                __FUNCTION__);
        return FBE_FALSE;
    }

    /*In order return FBE_TRUE*/
    if (fbe_compare_string(dp_mem->system_drive_serial_number[slot],
                            sizeof(serial_num_t),
                            dp->system_drive_serial_number[slot],
                            sizeof(serial_num_t), FBE_TRUE))
    {   
        /*not identical*/
        return FBE_FALSE;    
    }else
    {
        return FBE_TRUE;
    }
}

static void database_homewrecker_get_integrity_event(homewrecker_system_disk_table_t* sdt,
                                                                    homewrecker_system_integrity_event_t* event)
{
    fbe_u32_t i = 0;

    if (sdt == NULL || event == NULL)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: illegal NULL pointer\n",
                        __FUNCTION__);
        return;
    }

    for (i=0; i<FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER; i++)
    {
        event->slot_type[i] = database_homewrecker_disk_type_str(&sdt[i].disk_type);
    }
}


static fbe_bool_t database_homewrecker_sig_verify_order(fbe_u32_t bus, fbe_u32_t enclosure, fbe_u32_t slot,
                                                                       fbe_fru_signature_t* sig)
{
    if (sig == NULL)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: illegal NULL pointer\n",
                        __FUNCTION__);
        return FBE_FALSE;
    }

    /*In order return FBE_TRUE*/
    if (sig->slot == slot && sig->bus == bus && sig->enclosure == enclosure)
    {   
        return FBE_TRUE;    
    }else
    {
        /*not identical*/
        return FBE_FALSE;
    }
}

/*!**************************************************************************************************
@fn database_homewrecker_sig_verify_location()
*****************************************************************************************************

* @brief
*  Determine if it is a system slot disk or not  according to fru signature. 
*
* 
* @param sig_p- pointer to fru signature read from disk.
*
* @return
*  homewrecker_location_t- disk location
*
*****************************************************************************************************/
static homewrecker_location_t database_homewrecker_sig_verify_location(fbe_fru_signature_t *sig_p)
{
    if (sig_p == NULL)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: illegal NULL pointer\n",
                        __FUNCTION__);
        return HOMEWRECER_LOCATION_UNKNOW;
    }
    
    if (sig_p->bus == 0 &&
        sig_p->enclosure == 0 &&
        (sig_p->slot < FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER))
    {
        return HOMEWRECER_IN_SYS_SLOT;
    }
    else
    {
        return HOMEWRECER_IN_USR_SLOT;
    }
}

/*!**************************************************************************************************
@fn database_homewrecker_get_disk_type()
*****************************************************************************************************

* @brief
*  Determine the disk type according to read datas and status. 
*
* 
* @param fru_mem_dp- pointer to descriptor which was built according current system.
* @param system_disk_table- pointer to a internal table for Homewrecker use
*
* @return
*  fbe_status_t
*
*****************************************************************************************************/

static fbe_status_t database_homewrecker_get_disk_type(homewrecker_system_disk_table_t* system_disk_table,
                                                       fbe_homewrecker_fru_descriptor_t* fru_mem_dp,
                                                       homewrecker_system_disk_table_t* standard_disk)
{
    homewrecker_system_disk_table_t* sdt = system_disk_table;
    fbe_u32_t disk_index = 0;
    fbe_u32_t i = 0;

    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "Homewrecker: determine disk type.\n");

                        
    if (sdt == NULL || fru_mem_dp == NULL || standard_disk == NULL)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: illegal NULL pointer.\n",
                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*first three DB drives*/
    for (disk_index=0; disk_index<FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER; disk_index++)
    {
        if (database_homewrecker_verify_magic_str(HOMEWRECER_DISK_SIGNATURE, &sdt[disk_index].sig))
        {
            sdt[disk_index].disk_type = FBE_HW_DISK_TYPE_UNKNOWN;
            for (i=0; i<FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER; i++)
            {
                if (!fbe_compare_string(standard_disk->dp.system_drive_serial_number[i], 
                                        sizeof(serial_num_t),
                                        fru_mem_dp->system_drive_serial_number[disk_index], 
                                        sizeof(serial_num_t), FBE_TRUE))
                {
                    /*equal*/
                    sdt[disk_index].disk_type = FBE_HW_CUR_ARR_SYS_DISK;
                    break;
                }
            }
            /* Already got result for this drive, so continue to next drive */
            if (FBE_HW_CUR_ARR_SYS_DISK == sdt[disk_index].disk_type)
                continue;
        }

        if (sdt[disk_index].path_enable == FBE_FALSE ||
             sdt[disk_index].sig_report.call_status != FBE_STATUS_OK)
        {
            sdt[disk_index].disk_type = FBE_HW_NO_DISK;
            continue;
        }
        
        if (sdt[disk_index].sig_report.call_status == FBE_STATUS_OK &&
            !database_homewrecker_verify_magic_str(HOMEWRECER_DISK_SIGNATURE, &sdt[disk_index].sig) &&
            sdt[disk_index].path_enable == FBE_TRUE)
        {
            sdt[disk_index].disk_type = FBE_HW_NEW_DISK;
            continue;
        }

        if (database_homewrecker_verify_magic_str(HOMEWRECER_DISK_DESCRIPTOR, &sdt[disk_index].dp) &&
             sdt[disk_index].sig.system_wwn_seed!= standard_disk->dp.wwn_seed &&
             database_homewrecker_sig_verify_location(&sdt[disk_index].sig) == HOMEWRECER_IN_SYS_SLOT)
        {
            sdt[disk_index].disk_type = FBE_HW_OTHER_ARR_SYS_DISK;
            continue;
        }

        if (database_homewrecker_verify_magic_str(HOMEWRECER_DISK_SIGNATURE, &sdt[disk_index].sig) &&
             (sdt[disk_index].sig.system_wwn_seed != standard_disk->dp.wwn_seed) &&
            database_homewrecker_sig_verify_location(&sdt[disk_index].sig) == HOMEWRECER_IN_USR_SLOT)
        {
            sdt[disk_index].disk_type = FBE_HW_OTHER_ARR_USR_DISK;
            continue;
        }

        if (database_homewrecker_verify_magic_str(HOMEWRECER_DISK_SIGNATURE, &sdt[disk_index].sig) &&
             (sdt[disk_index].sig.system_wwn_seed == standard_disk->dp.wwn_seed) &&
            database_homewrecker_sig_verify_location(&sdt[disk_index].sig) == HOMEWRECER_IN_USR_SLOT)
        {
            sdt[disk_index].disk_type = FBE_HW_CUR_ARR_USR_DISK;
            continue;
        }

        if (sdt[disk_index].disk_type == FBE_HW_DISK_TYPE_INVALID ||
            sdt[disk_index].disk_type == FBE_HW_DISK_TYPE_UNKNOWN)
        {
            sdt[disk_index].disk_type = FBE_HW_NEW_DISK;     
            continue;
        }
    }
    /* output the disk type to trace */
    for (disk_index = 0; disk_index < 4; disk_index++)
    {
        database_trace (FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                         "Homewrecker: disk 0_0_%d type: 0x%x.\n",
                                         disk_index,
                                         sdt[disk_index].disk_type);
    }
    
    return FBE_STATUS_OK;
}

static fbe_status_t database_homewrecker_check_wwn_seed(fbe_database_service_t *database_service_ptr,
                                                        homewrecker_system_disk_table_t* sdt,
                                                        fbe_homewrecker_fru_descriptor_t* in_memory_descriptor,
                                                        homewrecker_system_disk_table_t* standard_disk,
                                                        homewrecker_service_mode_type_t* service_mode_type) 
                                                                          
{   
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_homewrecker_fru_descriptor_t fru_descriptor = {0};
    fbe_bool_t user_modified_flag = FBE_FALSE;

    if (database_service_ptr == NULL || sdt == NULL||
        in_memory_descriptor == NULL|| standard_disk == NULL|| service_mode_type == NULL)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: illegal NULL pointer\n",
                __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    *service_mode_type = NO_SERVICE_MODE;

    /*Get usermodifiedWwnSeed set by ESP*/
    status = fbe_database_get_user_modified_wwn_seed_flag_with_retry(&user_modified_flag);
    if (FBE_STATUS_OK != status)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Failed to get user modified wwn seed flag\n",
                        __FUNCTION__);
        /*Failed to get it, then we considered it as unexpected*/
        /*user_modified_flag = FBE_FALSE;*/
        return status;
    }
    
    /*check chassis replacement*/
    if (in_memory_descriptor->wwn_seed != standard_disk->dp.wwn_seed)
    {
        /*check chasis replacement planned or not*/
        if (standard_disk->dp.chassis_replacement_movement == FBE_CHASSIS_REPLACEMENT_FLAG_TRUE
            && user_modified_flag == FBE_FALSE)
        {
            database_trace (FBE_TRACE_LEVEL_INFO, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "Homewrecker: array is under planned chassis replacement case.\n");  
            /*planned chassis replacement */
            database_trace (FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "Homewrecker: Persist wwn seed to Resume PROM\n");
            status = fbe_database_set_board_resume_prom_wwnseed_with_retry(standard_disk->dp.wwn_seed);
            if (FBE_STATUS_OK != status)
            {
                database_trace (FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: Failed to persist chassis wwn_seed into board PROM resume\n",
                                __FUNCTION__);
                return status;
            }
    
            /* clear the chassis replacement flag, update fru descriptor in disks */
            fbe_copy_memory(&fru_descriptor, &standard_disk->dp, sizeof(fbe_homewrecker_fru_descriptor_t));
            fru_descriptor.chassis_replacement_movement = FBE_CHASSIS_REPLACEMENT_FLAG_INVALID;
    
            status = fbe_database_set_homewrecker_fru_descriptor(database_service_ptr, &fru_descriptor);
            if (FBE_STATUS_OK != status)
            {
                database_trace (FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: Failed to persist fru descriptors to disk\n",
                                __FUNCTION__);
    
                return status; 
            }else
            {
                standard_disk->dp.chassis_replacement_movement = FBE_CHASSIS_REPLACEMENT_FLAG_INVALID;
                            
                database_trace (FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: wwn seed mismatched, persist work done\n",
                                __FUNCTION__);
            }
        }
        else if (standard_disk->dp.chassis_replacement_movement != FBE_CHASSIS_REPLACEMENT_FLAG_TRUE
                 && user_modified_flag == FBE_TRUE)
        {
            fbe_copy_memory(&fru_descriptor, &standard_disk->dp, sizeof(fbe_homewrecker_fru_descriptor_t));
            fru_descriptor.wwn_seed = in_memory_descriptor->wwn_seed;
            standard_disk->dp.wwn_seed = fru_descriptor.wwn_seed;    /*make it consistent in real disk and standard_disk*/
            
            status = fbe_database_set_homewrecker_fru_descriptor(database_service_ptr, &fru_descriptor);
            if (FBE_STATUS_OK != status)
            {
                database_trace (FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: Failed to persist new wwn seed to fru descriptor\n",
                                __FUNCTION__);
            
                return status; 
            }else
            {
                database_trace (FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "Persist new wwn seed %d to fru descriptor\n", fru_descriptor.wwn_seed);
            }
            status = fbe_database_clear_user_modified_wwn_seed_flag_with_retry();
            if (FBE_STATUS_OK != status)
            {
                database_trace (FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: Failed to clean usermodifiedWwnSeed flag\n",
                                __FUNCTION__);
            
                return status; 
            }else
            {
                database_trace (FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "Cleaned usermodifiedWwnSeed flag\n");
            }
        }
        else if (standard_disk->dp.chassis_replacement_movement == FBE_CHASSIS_REPLACEMENT_FLAG_TRUE
                 && user_modified_flag == FBE_TRUE)
        {
            /* chaos now, homewrecker is not able to determine 
            update wwn seed in fru descrioptor or in chassis PROM*/
            database_trace (FBE_TRACE_LEVEL_WARNING, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "Homewrecker: Illegal: Both user modified and chassis replacement flag was set\n");  
            *service_mode_type = OPERATION_ON_WWNSEED_CHAOS;
        }
        else
        {
            /* unplanned chassis replaced*/
            database_trace (FBE_TRACE_LEVEL_WARNING, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "Homewrecker: array is under unplanned chassis replacement case.\n");  
            *service_mode_type = CHASSIS_MISMATCHED_ERROR;
        }
    }else
    {   /*wwn seed in PROM is same as the one in fru descriptor*/
        if (standard_disk->dp.chassis_replacement_movement == FBE_CHASSIS_REPLACEMENT_FLAG_TRUE)
        {
            /* clear the chassis replacement flag*/
            fbe_copy_memory(&fru_descriptor, &standard_disk->dp, sizeof(fbe_homewrecker_fru_descriptor_t));
            fru_descriptor.chassis_replacement_movement = FBE_CHASSIS_REPLACEMENT_FLAG_INVALID;
    
            status = fbe_database_set_homewrecker_fru_descriptor(database_service_ptr, &fru_descriptor);
            if (FBE_STATUS_OK != status)
            {
                database_trace (FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: Failed to persist fru descriptors to disk\n",
                                __FUNCTION__);
    
                /*return status; */ /*we can clean it again in next reboot*/
            }else
            {
                standard_disk->dp.chassis_replacement_movement = FBE_CHASSIS_REPLACEMENT_FLAG_INVALID;
                database_trace (FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "Chassis wasn't changed, cleaned the flag\n");
            }
        }
        
        if (user_modified_flag == FBE_TRUE)
        {
            status = fbe_database_clear_user_modified_wwn_seed_flag_with_retry();
            if (FBE_STATUS_OK != status)
            {
                database_trace (FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: Failed to clean usermodifiedWwnSeed flag\n",
                                __FUNCTION__);
            
                /*return status; */ /*we can clean it again in next reboot*/
            }else
            {
                database_trace (FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "WWN seed wasn't changed, cleaned the flag\n");
            }
        }
        
    }
    return FBE_STATUS_OK;
}


static fbe_status_t database_homewrecker_get_standard_disk(fbe_database_service_t *database_service_ptr,
                                                          homewrecker_system_disk_table_t* sdt,
                                                          fbe_homewrecker_fru_descriptor_t* dp,
                                                          homewrecker_system_disk_table_t** standard_disk) 
{
    /*Choose standard fru*/
    /*Choose the disk as standard disk which return maximum sequence number*/
    fbe_u32_t       disk_index = 0;
    fbe_s32_t       max_seq_num_slot = 0;
    *standard_disk = &sdt[0];
    max_seq_num_slot = database_homewrecker_valid_disk_max_seq_num(sdt, NULL);
    if (max_seq_num_slot != -1)
    {
        *standard_disk = &sdt[max_seq_num_slot];
    }
    else
    {
        for (disk_index = 0; disk_index < FBE_FRU_DESCRIPTOR_DB_DRIVE_NUMBER; disk_index++)
        {
            if (sdt[disk_index].is_invalid_system_drive == FBE_FALSE)
            {
                *standard_disk = &sdt[disk_index];
                break;
            }
        }
    
        if (*standard_disk == NULL) {
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: standard_disk is not initialized\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE; 
        }
    }

    return FBE_STATUS_OK;
}

static fbe_status_t database_homewrecker_disconnect_pvd(fbe_database_service_t *database_service_ptr,
                                                                        homewrecker_system_disk_table_t* sdt)
{
    fbe_u32_t   disk_index = 0;
    fbe_status_t    status = FBE_STATUS_INVALID;

    if (database_service_ptr == NULL || sdt == NULL)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: illegal parameter\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                     "Homewrecker: %s entry.\n",
                     __FUNCTION__);

    /*Disconnect the fru_invalid disks pvd from pdo and continue booting*/
    for (disk_index = 0; disk_index < FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER; disk_index++)
    {
        if (sdt[disk_index].is_invalid_system_drive == FBE_FALSE)
            continue;
        /*Homewrecker detects it a new drive and set a flag for database not to load
          database configuration and system nonpaged metadata from the drive
        */
        if (disk_index < FBE_FRU_DESCRIPTOR_DB_DRIVE_NUMBER)
            fbe_database_raw_mirror_set_new_drive(0x1 << disk_index);
            
        status = database_homewrecker_disconnect_pvd_pdo(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + disk_index);
        if (FBE_STATUS_OK != status)
        {
            database_trace (FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                             "%s: Disconnect edge for PVD: 0x%x failed\n", __FUNCTION__, 
                             FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + disk_index);
            return status; 
        }
        else{        
            database_trace(FBE_TRACE_LEVEL_INFO, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "Homewrecker: Disconnected edge for PVD:  0x%x\n", 
                            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + disk_index);
        }
    
         /* New comments added by Song,Yingying BEGIN:
         *
         * In original code, this area has code to call database_homewrecker_invalidate_pvd_sn(), 
         * we removed invalidate serial num now. Below is the reason:
         * 1. If a bound user drive is inserted to a system slot, homewrecker will not invalidate the 
         *    fru_descriptor in memory and in disk. Only when a totally new/unbound drive is inserted 
         *    to a system slot, homewrecker will update the fru_descriptor through the reinitialize job.
         *    This design is to ensure if the original system drive is inserted back to the right system
         *    slot, homewrecker will take the drive as the original system drive based on the fru_descriptor.
         * 2. If a bound user drive is inserted to a system slot, in original code, the DB will invalidate
         *    the serial_num of system PVD in database table memory. However, if we invalidate the serial num, 
         *    when we inserted the original system drive back to the right system slot online, the system drive 
         *    cannot connect to right system PVD. Although Homewrecker consider it is a right system drive 
         *    based on fru_descriptor, but then it will fail to connect drive to right system PVD based on serial num. 
         *    Since in original code the PVD's serial num has been invalidated, the sytem drive cannot be 
         *    connected to right system PVD.
         * 3. Since above, to make the DB and homewrecker behavior in accordance and avoid above *2 issue, 
         *    we romoved the database_homewrecker_invalidate_pvd_sn().
         *    
         * New comments added by Song,Yingying END*/ 
        
    }
    return FBE_STATUS_OK;
}
static fbe_status_t database_homewrecker_check_disorder(homewrecker_system_disk_table_t* sdt,
                                                                        fbe_homewrecker_fru_descriptor_t* dp,
                                                                        homewrecker_system_disk_table_t* standard_disk,
                                                                        homewrecker_service_mode_type_t* service_mode_type) 
{
    
    fbe_u32_t       disk_index = 0;
    if (service_mode_type == NULL || standard_disk == NULL ||
        sdt == NULL || dp == NULL)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: illegal parameter\n",
                        __FUNCTION__);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *service_mode_type = NO_SERVICE_MODE;

    /*Check disorder situation*/
    for (disk_index=0; disk_index<FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER; disk_index++)
    {
        /* because fru_descriptor is stored in raw_mirror
          * However, raw_mirror only resides in first 3 drive,
          * on 0_0_3 there is no raw_mirror region, so when put
          * 0_0_3 drive into first 3 slots by some stupid drive movement
          * Homewrecker would think it is an invalid drive, because
          * reading fru_descriptor to 0_0_3 drive(it is in 0_0_0/1/2) slot
          * would fail(CRC error). Because it is in first 3 slots, so raw_mirror 
          * IO would access it.
          * meantime it is system drive for this array, we must count it when
          * HW check system drive disorder. So added the second condition
          * in below "if", we could not bypass it in above situation.
          */
        if (sdt[disk_index].is_invalid_system_drive == FBE_TRUE 
            && sdt[disk_index].disk_type != FBE_HW_CUR_ARR_SYS_DISK)
        {
            database_trace (FBE_TRACE_LEVEL_INFO, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "Homewrecker: drive in 0_0_%d slot is not current array system drive.\n",
                            disk_index);
            continue;
        }
        
        if (database_homewrecker_dp_verify_order(disk_index, &standard_disk->dp, dp) &&
            database_homewrecker_sig_verify_order(0, 0, disk_index, &sdt[disk_index].sig))
        {
            /*in order*/
            database_trace (FBE_TRACE_LEVEL_INFO, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "Homewrecker: check system drive 0_0_%d is in right slot.\n",
                            disk_index);  
        }else
        {
            /*service mode*/
            database_trace (FBE_TRACE_LEVEL_WARNING, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "Homewrecker: check system drive 0_0_%d is in wrong slot.\n",
                            disk_index);  
            *service_mode_type = SYS_DRIVES_DISORDERED_ERROR;
            sdt[disk_index].is_invalid_system_drive = FBE_TRUE;
        }
    }
    
    return FBE_STATUS_OK;
}

/*!**************************************************************************************************
@fn database_homewrecker_get_system_drive_actual_location()
*****************************************************************************************************
* @brief
*  Scan all the pdos to find out the actual system drive location. 
*  This function is mostly copied from "fbe_database_drive_discover".
*
* @param sdt (in). The system disk array which contains the system drive info. 
*                         The array number is FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER.
*
* @return
*  fbe_status_t 
*
*****************************************************************************************************/
static fbe_status_t database_homewrecker_get_system_drive_actual_location(homewrecker_system_disk_table_t *sdt,
                                                                        homewrecker_system_disk_table_t *standard_disk)
{
    fbe_status_t                                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_topology_control_get_total_objects_of_class_t   get_total_objects;
    fbe_topology_control_enumerate_class_t              enumerate_class;
    fbe_object_id_t                     *obj_array = NULL;
    fbe_u32_t                           i;
    fbe_u32_t                           disk_index;
    fbe_sg_element_t                    *sg_list;
    fbe_sg_element_t                    *sg_element;
    fbe_u32_t                           alloc_data_size;
    fbe_u32_t                           alloc_sg_size;
    fbe_database_physical_drive_info_t              pdo_info;

    database_trace (FBE_TRACE_LEVEL_INFO, 
                                     FBE_TRACE_MESSAGE_ID_INFO,
                                     "Homewrecker: %s entry.\n",
                                     __FUNCTION__);  
    
    // get total # of PDOs
    get_total_objects.class_id = FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST;
    status = fbe_database_send_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_GET_TOTAL_OBJECTS_OF_CLASS,
                                             &get_total_objects,
                                             sizeof(fbe_topology_control_get_total_objects_of_class_t),
                                             FBE_SERVICE_ID_TOPOLOGY,
                                             NULL,  /* no sg list*/
                                             0,  /* no sg list*/
                                             FBE_PACKET_FLAG_EXTERNAL,
                                             FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: failed to get total # of PDOs\n", __FUNCTION__);
        return status;
    }

    database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: found %d object\n", __FUNCTION__, 
                   get_total_objects.total_objects);

    if (get_total_objects.total_objects == 0) {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                 "%s: found %d object\n", __FUNCTION__, get_total_objects.total_objects);
        return status;
    }
    // allocate buffer and sg_list
    alloc_data_size = get_total_objects.total_objects*sizeof (fbe_object_id_t);
    alloc_sg_size   = FBE_TOPLOGY_CONTROL_ENUMERATE_CLASS_SG_COUNT*sizeof (fbe_sg_element_t);
    sg_list = (fbe_sg_element_t *)fbe_memory_ex_allocate(alloc_data_size + alloc_sg_size);
    if (sg_list == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "%s: fail to allocate memory for %d objects\n", __FUNCTION__, get_total_objects.total_objects);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    sg_element = sg_list;
    fbe_sg_element_init(sg_element, alloc_data_size, (fbe_u8_t *)sg_list + alloc_sg_size);
    fbe_sg_element_increment(&sg_element);
    fbe_sg_element_terminate(sg_element);
    obj_array = (fbe_object_id_t *)(sg_list->address);

    enumerate_class.number_of_objects = get_total_objects.total_objects;
    enumerate_class.class_id = FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST;
    enumerate_class.number_of_objects_copied = 0;

    status = fbe_database_send_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_ENUMERATE_CLASS,
                                             &enumerate_class,
                                             sizeof(fbe_topology_control_enumerate_class_t),
                                             FBE_SERVICE_ID_TOPOLOGY,
                                             sg_list,  
                                             FBE_TOPLOGY_CONTROL_ENUMERATE_CLASS_SG_COUNT,  
                                             FBE_PACKET_FLAG_EXTERNAL,
                                             FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: fail to enumerate PDOs, status = %d\n", __FUNCTION__, status);
        
        fbe_memory_ex_release(sg_list);
        return status;
    }

    /*  Init the (bus, encl, slot) and Count the number of invalid system drive */
    for (disk_index = 0; disk_index < FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER; disk_index++) {
        sdt[disk_index].actual_bus = FBE_INVALID_PORT_NUM;
        sdt[disk_index].actual_encl = FBE_INVALID_ENCL_NUM;
        sdt[disk_index].actual_slot = FBE_INVALID_SLOT_NUM;
    }
    
    /*  go through all the PDOs */
    for (i = 0; i < enumerate_class.number_of_objects_copied; ++i) {
        fbe_zero_memory(&pdo_info, sizeof(fbe_database_physical_drive_info_t));
        status = fbe_database_get_pdo_info(obj_array[i], &pdo_info);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: failed to get pdo info for %d, status 0x%x\n", __FUNCTION__, obj_array[i], status);
            continue;
        }       
        for (disk_index = 0; disk_index < FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER; disk_index++) {
            /*If the serial number matched, it is a system drive inserted in user slot */
            if (!fbe_compare_string(standard_disk->dp.system_drive_serial_number[disk_index],
                            sizeof(serial_num_t),
                            pdo_info.serial_num,
                            sizeof(serial_num_t), FBE_TRUE)) {
                sdt[disk_index].actual_bus = pdo_info.bus;
                sdt[disk_index].actual_encl = pdo_info.enclosure;
                sdt[disk_index].actual_slot = pdo_info.slot;
                database_trace (FBE_TRACE_LEVEL_INFO, 
                                                 FBE_TRACE_MESSAGE_ID_INFO,
                                                 "Homewrecker: system drive with SN: %s in slot: %d_%d_%d.\n",
                                                 pdo_info.serial_num,
                                                 pdo_info.bus,
                                                 pdo_info.enclosure,
                                                 pdo_info.slot);  
            }
        }
    }

    fbe_memory_ex_release(sg_list);
    return FBE_STATUS_OK;
}


/*!**************************************************************************************************
@fn database_homewrecker_check_system_drive_in_user_slot()
*****************************************************************************************************

* @brief
*
* @param sdt (in). The system disk array which contains the system drive info. 
*                         The array number is FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER.
* @param service_mode_type (out). it is used for returning the service mode type.
*
* @return
*  fbe_status_t 
*
*****************************************************************************************************/
static fbe_status_t database_homewrecker_check_system_drive_in_user_slot(homewrecker_system_disk_table_t* sdt,
                                                                        homewrecker_system_disk_table_t *standard_disk,
                                                                        homewrecker_service_mode_type_t* service_mode_type) 
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                           disk_index;
    fbe_u32_t                           system_drive_in_user_slot_num = 0;

    database_trace (FBE_TRACE_LEVEL_INFO, 
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "Homewrecker: %s entry.\n", __FUNCTION__);  

    if (service_mode_type == NULL || standard_disk == NULL || sdt == NULL)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: illegal parameter\n",
                        __FUNCTION__);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = database_homewrecker_get_system_drive_actual_location(sdt, standard_disk);
    if (status != FBE_STATUS_OK)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR, 
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "Homewrecker: fail to get system drive actual location.\n");  
        return status;
    }

    for (disk_index = 0; disk_index < FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER; disk_index++) 
    {

        /* The actual slot is in a user slot */
        if (sdt[disk_index].actual_bus == 0 &&
            sdt[disk_index].actual_encl == 0 &&
            sdt[disk_index].actual_slot < FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER)
        {
            continue;
        }
        if (sdt[disk_index].actual_bus == FBE_INVALID_PORT_NUM ||
            sdt[disk_index].actual_encl == FBE_INVALID_ENCL_NUM ||
            sdt[disk_index].actual_slot == FBE_INVALID_SLOT_NUM)
        {
            database_trace (FBE_TRACE_LEVEL_WARNING, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "Homewrecker: system disk[%d] has invalid location(%d_%d_%d)\n",
                            disk_index, sdt[disk_index].actual_bus, sdt[disk_index].actual_encl, sdt[disk_index].actual_slot);  
            continue;
        }

        system_drive_in_user_slot_num++;
    }

    if (system_drive_in_user_slot_num > 0) {
        database_trace (FBE_TRACE_LEVEL_WARNING, 
                                         FBE_TRACE_MESSAGE_ID_INFO,
                                         "Homewrecker: Doulbe system drives invalid however found system drive in user slot.\n");  
        *service_mode_type = SYS_DRIVES_DOUBLE_INVALID_WITH_SYSTEM_DRIVE_IN_USER_SLOT;
    } else {
        *service_mode_type = NO_SERVICE_MODE;
    }

    return status;
}

static void database_homewrecker_get_disorder_status(homewrecker_system_disk_table_t* sdt,
                                                     fbe_u32_t  *slot,
                                                     fbe_u32_t  count)
{
    /*return slot location for sending event log*/
    /*the event log is like : disk 1 in slot 2, disk 2 in slot 1, disk 3 in slot3, disk 4 in slot4*/
    fbe_u32_t       disk_index = 0;

    if (sdt == NULL || slot == NULL)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: illegal parameter\n",
                        __FUNCTION__);
        return;
    }
    
    for (disk_index=0; disk_index<count; disk_index++)
    {
            slot[disk_index] = sdt[disk_index].sig.slot;
    }
}


static fbe_bool_t database_homewrecker_invalid_disk_type(fbe_homewrecker_system_disk_type_t  disk_type) 
{
    if (disk_type == FBE_HW_DISK_TYPE_UNKNOWN   ||
        disk_type == FBE_HW_DISK_TYPE_INVALID   ||
        disk_type == FBE_HW_OTHER_ARR_USR_DISK  ||
        disk_type == FBE_HW_CUR_ARR_USR_DISK    ||
        disk_type == FBE_HW_OTHER_ARR_SYS_DISK  ||
        disk_type == FBE_HW_NEW_DISK            ||
        disk_type == FBE_HW_NO_DISK)
        return FBE_TRUE;
    else
        return FBE_FALSE;
}

static fbe_char_t* database_homewrecker_disk_type_str(fbe_homewrecker_system_disk_type_t *disk_type_enum)
{
    switch (*disk_type_enum)
    {
        case FBE_HW_NO_DISK:   
            return  "NO_DISK";
            break;
        case FBE_HW_NEW_DISK:
            return  "NEW_DISK";
            break;
        case FBE_HW_OTHER_ARR_SYS_DISK:
            return  "OTHER_ARRAY_SYSTEM_DISK";
            break;
        case FBE_HW_OTHER_ARR_USR_DISK:
            return  "OTHER_ARRAY_USER_DISK";
            break;
        case FBE_HW_CUR_ARR_USR_DISK:
            return  "CURRENT_ARRAY_USER_DISK";
            break;
        case FBE_HW_CUR_ARR_SYS_DISK:  
            return  "CURRENT_ARRAY_SYSTEM_DISK";
            break;
        case FBE_HW_DISK_TYPE_UNKNOWN: 
            return  "DISK_TYPE_UNKNOWN";
            break;
        case FBE_HW_DISK_TYPE_INVALID: 
            return  "DISK_TYPE_INVALID";
            break;
        default:
            return  "DISK_TYPE_INVALID";
    }
}

static void database_homewrecker_bootup_with_event(homewrecker_system_disk_table_t* sdt)
{
    fbe_u32_t       disk_index = 0;
    homewrecker_boot_with_event_t event;
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    event_log_param_selector_t sel;
    
    for (disk_index=0; disk_index<FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER; disk_index++)
    {
        if (sdt[disk_index].is_invalid_system_drive == FBE_TRUE && 
            sdt[disk_index].disk_type != FBE_HW_NEW_DISK &&
            sdt[disk_index].disk_type != FBE_HW_OTHER_ARR_SYS_DISK &&
            sdt[disk_index].disk_type != FBE_HW_OTHER_ARR_USR_DISK)
        {
            event.event_str = database_homewrecker_disk_type_str(&sdt[disk_index].disk_type);
            event.slot = disk_index;
            sel.normal_boot_with_warning = &event;
            database_homewrecker_send_event_log(SEP_WARN_SYSTEM_DISK_NEED_OPERATION, &sel);
            if (status != FBE_STATUS_OK)
            {
                database_trace (FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: Send event log failed\n", __FUNCTION__);
            }
        }
    }
}

/*!**************************************************************************************************
@fn database_homewrecker_logic()
*****************************************************************************************************

* @brief
*  implement homewrecker boot up logic. The input is internal table. This function determine whether let
*  system get into service mode or normally boot up.
* 
* @param database_service_ptr- pointer to database service
* @param sdt- pointer to a internal table for Homewrecker use
* @param dp- pointer to fru descriptor built from current system
*
* @return
*  fbe_status_t
*
*****************************************************************************************************/

static fbe_status_t database_homewrecker_logic(fbe_database_service_t *database_service_ptr,
                                                          homewrecker_system_disk_table_t* sdt,
                                                          homewrecker_system_disk_table_t* standard_disk,
                                                          fbe_homewrecker_fru_descriptor_t* dp) 
{
    fbe_status_t    status = FBE_STATUS_INVALID;
    fbe_u32_t       disk_index = 0;
    fbe_u32_t       invalid_disk = 0;
    homewrecker_service_mode_type_t service_mode_type = NO_SERVICE_MODE;
    fbe_u32_t       slot[FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER];
    event_log_param_selector_t sel;
    homewrecker_system_integrity_event_t integrity_event;
    homewrecker_disk_in_wrong_slot_event_t wrong_slot;

    database_trace (FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                     "Homewrecker: %s entry.\n", __FUNCTION__);
    
    if (sdt == NULL || database_service_ptr == NULL || dp == NULL || standard_disk == NULL)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: illegal parameter\n",
                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    /*according to descriptor status reported, mark disks invalid*/
    for (disk_index=0; disk_index<FBE_FRU_DESCRIPTOR_DB_DRIVE_NUMBER; disk_index++)
    {
        if (sdt[disk_index].dp.wwn_seed != standard_disk->dp.wwn_seed ||
            sdt[disk_index].dp.sequence_number!= standard_disk->dp.sequence_number)
        {
            sdt[disk_index].is_invalid_system_drive = FBE_TRUE;
        }
    }

    /*according to signature status reported, mark disks invalid*/
    for (disk_index=0; disk_index<FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER; disk_index++)
    {   /*read signature status*/
        if (sdt[disk_index].sig_report.call_status != FBE_STATUS_OK)
        {
            sdt[disk_index].is_invalid_system_drive = FBE_TRUE;
        }

        /*mark disk invalid according to disk types*/
        if (database_homewrecker_invalid_disk_type(sdt[disk_index].disk_type))
            sdt[disk_index].is_invalid_system_drive = FBE_TRUE;
        /*invalid disk count.*/
        if (sdt[disk_index].is_invalid_system_drive == FBE_TRUE)
            invalid_disk++;
    }   


    do{
        /*If more than 2 invalid disks, enter service mode */
        if (invalid_disk > 2) 
        {
            database_trace (FBE_TRACE_LEVEL_WARNING, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "Homewrecker: More than 2 system drives are invalid.\n");
            /*system drives integrienter broken, enter service mode*/
            service_mode_type = SYS_DRIVES_INTEGRALITY_BROKEN;
            break;
        }else
        {
            database_trace (FBE_TRACE_LEVEL_INFO, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "Homewrecker: check system drives are disorder or not.\n");        
            status = database_homewrecker_check_disorder(sdt, dp, standard_disk, &service_mode_type); 
            if (FBE_STATUS_OK != status)
            {
                return status;
            }
            if (service_mode_type == SYS_DRIVES_DISORDERED_ERROR)
            {
                break;
            }

            database_trace (FBE_TRACE_LEVEL_INFO, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "Homewrecker: check system wwn seed match or not.\n");      
            status = database_homewrecker_check_wwn_seed(database_service_ptr, sdt, dp, 
                                                         standard_disk, &service_mode_type);
            if (FBE_STATUS_OK != status)
            {
                return status;
            }
            if (service_mode_type == CHASSIS_MISMATCHED_ERROR ||
                service_mode_type == OPERATION_ON_WWNSEED_CHAOS)
            {
                break;
            }

            /* When two system drives are invalid, check if there is one or two system drives is inserted in a user slot;
              * If there is, enter service mode. So user can recover the system by removin the drive. 
              * Else we let the system boot up.
              */
            if (invalid_disk == 2)
            {
                fbe_event_log_write(SEP_WARN_TWO_OF_THE_FIRST_THREE_SYSTEM_DRIVES_FAILED,
                                            NULL, 0, NULL);
                status = database_homewrecker_check_system_drive_in_user_slot(sdt, standard_disk, &service_mode_type);
                if (FBE_STATUS_OK != status)
                {
                    return status;
                }
                break;
            }        
        }
    }while(0);

    status = database_homewrecker_disconnect_pvd(database_service_ptr, sdt);
    if (FBE_STATUS_OK != status)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: Disconnect edge for PVDs failed\n", __FUNCTION__);
        return status;
    }

    switch (service_mode_type)
    {
        case NO_SERVICE_MODE:
            database_trace (FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "Homewrecker: let array boot up\n");
            /*now we can securely initialize the in-memory fru descriptor with the correct data*/
            fbe_spinlock_lock(&database_service_ptr->pvd_operation.system_fru_descriptor_lock);
            fbe_copy_memory(&database_service_ptr->pvd_operation.system_fru_descriptor, &standard_disk->dp, sizeof(fbe_homewrecker_fru_descriptor_t));
            fbe_spinlock_unlock(&database_service_ptr->pvd_operation.system_fru_descriptor_lock);
            database_homewrecker_bootup_with_event(sdt);
            return status;

            break;
            
        case SYS_DRIVES_DISORDERED_ERROR:
            database_trace (FBE_TRACE_LEVEL_WARNING, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "Homewrecker: DB would enter service mode with SYS_DRIVES_DISORDERED_ERROR.\n");
            database_homewrecker_get_disorder_status(sdt, slot, FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER);
            sel.disordered_slot = slot;
            database_homewrecker_send_event_log(SEP_ERROR_CODE_DB_DRIVES_DISORDERED_ERROR, &sel);
            return database_homewrecker_enter_service_mode(FBE_DATABASE_SERVICE_MODE_REASON_HOMEWRECKER_SYSTEM_DISK_DISORDER);
            
            break;
            
        case CHASSIS_MISMATCHED_ERROR:
            database_trace (FBE_TRACE_LEVEL_WARNING, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "Homewrecker: DB would enter service mode with CHASSIS_MISMATCHED_ERROR.\n");
            database_homewrecker_send_event_log(SEP_ERROR_CODE_CHASSIS_MISMATCHED_ERROR, NULL);
            return database_homewrecker_enter_service_mode(FBE_DATABASE_SERVICE_MODE_REASON_HOMEWRECKER_CHASSIS_MISMATCHED);

            break;

        case OPERATION_ON_WWNSEED_CHAOS:
            database_trace (FBE_TRACE_LEVEL_WARNING, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "Homewrecker: DB will enter service mode with OPERATION_ON_WWNSEED_CHAOS.\n");
            database_homewrecker_send_event_log(SEP_ERROR_CODE_OPERATION_ON_WWNSEED_CHAOS, NULL);
            return database_homewrecker_enter_service_mode(FBE_DATABASE_SERVICE_MODE_REASON_HOMEWRECKER_OPERATION_ON_WWNSEED_CHAOS);

            break;
            
        case SYS_DRIVES_INTEGRALITY_BROKEN:
            database_trace (FBE_TRACE_LEVEL_WARNING, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "Homewrecker: DB would enter service mode with SYS_DRIVES_INTEGRALITY_BROKEN.\n");
            database_homewrecker_get_integrity_event(sdt, &integrity_event);
            sel.integrity_disk_type = &integrity_event;
            database_homewrecker_send_event_log(SEP_ERROR_CODE_INTEGRITY_BROKEN_ERROR, &sel);
            return database_homewrecker_enter_service_mode(FBE_DATABASE_SERVICE_MODE_REASON_HOMEWRECKER_INTEGRITY_BROKEN);

            break;
        case SYS_DRIVES_DOUBLE_INVALID_WITH_SYSTEM_DRIVE_IN_USER_SLOT:
            database_trace (FBE_TRACE_LEVEL_WARNING, 
                                             FBE_TRACE_MESSAGE_ID_INFO,
                                             "Homewrecker: DB would enter SYS_DRIVES_DOUBLE_INVALID_WITH_SYSTEM_DRIVE_IN_USER_SLOT.\n");
            database_homewrecker_send_event_log(SEP_ERROR_CODE_TWO_INVALID_SYSTEM_DRIVE_AND_ONE_MORE_OF_THEM_IN_USER_SLOT, NULL);
            sel.disk_in_wrong_slot = &wrong_slot;
            for (disk_index = 0; disk_index < FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER; disk_index++) {
                if (sdt[disk_index].actual_slot != FBE_INVALID_SLOT_NUM) {
                    fbe_zero_memory(&wrong_slot, sizeof(homewrecker_disk_in_wrong_slot_event_t));
                    sel.disk_in_wrong_slot->disk_pvd_id = (fbe_object_id_t)(disk_index + 1); /* The system PVD object id is hardcoded.*/

                    /* The system drive is inserted in user slot, its correct location should be system slot */
                    sel.disk_in_wrong_slot->bus = sdt[disk_index].actual_bus;
                    sel.disk_in_wrong_slot->enclosure = sdt[disk_index].actual_encl;
                    sel.disk_in_wrong_slot->slot = sdt[disk_index].actual_slot;
                    sel.disk_in_wrong_slot->its_correct_bus = 0;
                    sel.disk_in_wrong_slot->its_correct_encl = 0;
                    sel.disk_in_wrong_slot->its_correct_slot = disk_index;
                    database_homewrecker_send_event_log(SEP_ERROR_CODE_SYSTEM_DRIVE_IN_WRONG_SLOT, &sel);
                }
            }
            return database_homewrecker_enter_service_mode(FBE_DATABASE_SERVICE_MODE_REASON_HOMEWRECKER_DOUBLE_INVALID_DRIVE_WITH_DRIVE_IN_USER_SLOT);
        default:
            break;
    }

    database_trace (FBE_TRACE_LEVEL_ERROR,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: Should not be here.\n",
                    __FUNCTION__);
    return status;
}

/*!**************************************************************************************************
@fn database_homewrecker_build_internal_table()
*****************************************************************************************************

* @brief
*  Read fru descriptor and signature from disk to internal table. This table records all the data read and status. 
*  Data are well prepared for next stage process in Homewrecker.
* 
* @param database_service_ptr- pointer to database service
* @param system_disk_table- pointer to a internal table for Homewrecker use
*
* @return
*  fbe_status_t
*
*****************************************************************************************************/

static fbe_status_t database_homewrecker_build_internal_table(homewrecker_system_disk_table_t* system_disk_table,
                                                                             homewrecker_system_disk_table_t* standard_disk)
{
    fbe_status_t    status = FBE_STATUS_INVALID;
    fbe_homewrecker_fru_descriptor_t fru_descriptor_disk = {0};
    fbe_homewrecker_get_fru_disk_mask_t disk_mask = FBE_FRU_DISK_ALL;
    fbe_homewrecker_get_raw_mirror_info_t dp_report;
    fbe_u32_t       disk_index = 0;
    fbe_bool_t      path_enable = FBE_TRUE;
#ifdef C4_INTEGRATED
    FILE * tmp_err_file;
#endif

    if (system_disk_table == NULL || standard_disk == NULL)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: illegal parameter\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "Homewrecker: Read system disk FRU descriptor and signature, build internal table\n");

    /* initialize system_disk_table with default values */
    for (disk_index=0; disk_index<FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER; disk_index++)
    {
            system_disk_table[disk_index].path_enable = FBE_FALSE;
            system_disk_table[disk_index].disk_type = FBE_HW_DISK_TYPE_UNKNOWN;
            system_disk_table[disk_index].is_invalid_system_drive = FBE_FALSE;
            fbe_zero_memory(&system_disk_table[disk_index].sig, sizeof(system_disk_table[disk_index].sig));
            system_disk_table[disk_index].sig_report.call_status = FBE_STATUS_GENERIC_FAILURE;
    
            fbe_zero_memory(&system_disk_table[disk_index].dp, sizeof(system_disk_table[disk_index].dp));
            system_disk_table[disk_index].dp_report.call_status = FBE_STATUS_GENERIC_FAILURE;
            system_disk_table[disk_index].dp_report.status = FBE_GET_HW_STATUS_ERROR;
    }

    /* get each system pvd edge state*/
    for (disk_index = 0;                   
        disk_index < FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER; disk_index++)
    {
        status = database_homewrecker_get_system_pvd_edge_state(disk_index, &path_enable);
        if (FBE_STATUS_OK != status)
        {
            database_trace (FBE_TRACE_LEVEL_WARNING,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: Failed to get edge state\n",
                            __FUNCTION__);
            path_enable = FBE_FALSE;
        }
        else
        {
            database_trace (FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: Got pvd: 0x%x edge state, state: %d \n",
                            __FUNCTION__,
                            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + disk_index,
                            path_enable);
        }
        system_disk_table[disk_index].path_enable = path_enable;
    }

    /*Read fru descriptor region from disk*/
    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: Read FRU descriptor from raw mirror\n",
                    __FUNCTION__);
    status = fbe_database_get_homewrecker_fru_descriptor(&fru_descriptor_disk, &dp_report, disk_mask);
    dp_report.call_status = status;
    if (FBE_STATUS_OK != status)
    {
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Failed to read fru descriptor\n",
                        __FUNCTION__);
       
        for (disk_index=0; disk_index<FBE_FRU_DESCRIPTOR_DB_DRIVE_NUMBER; disk_index++)
        {
            system_disk_table[disk_index].is_invalid_system_drive = FBE_TRUE;
            fbe_copy_memory(&system_disk_table[disk_index].dp_report, &dp_report, sizeof(dp_report));
        }
    } else
    {
        /* Get the FRU Descriptor from raw_mirror successfully,
          * however this does not mean every drive in raw_mirror is good,
          * maybe one/two drives out of raw_mirror are bad.
          * Copy the fru_descriptor_disk to standard_disk->dp, this means
          * we trust raw_mirror, let raw_mirror to determine which copy data
          * from first 3 drives(they are in raw_mirror) is legal/valid/updated, 
          * this leverages some functionalities of raw_mirror, however the raw_mirror
          * would make wrong decision under some cases.
          */

        /* Comments added at 2013 Feb 27th
          * Above comments are older ones, leave them there for future reference.
          *
          * Raw_mirror can not be trusted in... even raw_mirror return Data with warning
          * If we replace 0_0_0 with a foreign 0_0_0 drive, raw_mirror may think data on 0_0_0
          * is valid out of 3 copies in raw_mirror. It is wrong.
          * So Homewrecker needs to do FRU_Descriptor validation by itself, before it complete 
          * the validation, Homewrecker can not tell which is standard FRU_Descriptor.
          */
    }

    if (dp_report.status == FBE_GET_HW_STATUS_OK)
    {
        /* dp_report.status == OK shows every drive in raw_mirror is OK */

        /* Then we can set standard_disk->dp directly, we choose to trust in raw_mirror 
          * when it tells Homewrecker, all disks are OK.
          */
        fbe_database_display_fru_descriptor(&fru_descriptor_disk);
        fbe_copy_memory(&(standard_disk->dp), &fru_descriptor_disk, sizeof(fru_descriptor_disk));

        /* After we set standard_disk->dp, we should set global_fru_descriptor_sequence_number.
          * In order to let Homewrecker get correct seq_num in possible following FRU_Descriptor update
          */
        status = database_set_fru_descriptor_sequence_number(standard_disk->dp.sequence_number);
        if (status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "Homewrecker: Set fru descriptor sequence number failed.\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }
        
        /* so all of system_disk.dp and dp_report are OK */        
        for (disk_index=0; disk_index<FBE_FRU_DESCRIPTOR_DB_DRIVE_NUMBER; disk_index++)
        {
            fbe_copy_memory(&system_disk_table[disk_index].dp, &fru_descriptor_disk, sizeof(fru_descriptor_disk));
            fbe_copy_memory(&system_disk_table[disk_index].dp_report, &dp_report, sizeof(dp_report));
        }

    }else
    {
        /* Has ERROR when read fru descriptor from raw mirror
          * to handle magic number unmatched problem
          * use error_disk_bitmap from raw_mirror and DISK bitmask to
          * find the error disk and set its state in system_disk_talbe
          */

        /* Comments added @2013 Feb 27th 
          * remove the code about set is_invald_system_drive here
          * because raw_mirror may return wrong data to Homewrekcer
          * when there is invalid disk. 
          * Only read on-disk fru_descriptor seperately here, then run Homewrecker
          * logic for validating standard fru_descriptor.
          * After that, set is invalid system drive based on the standard fru_descriptor.
          */

        /* Read all the fru descriptor seperatly from disks */
        database_trace(FBE_TRACE_LEVEL_INFO, 
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "Homewrecker: Read fru descriptor seperatly from each disk in raw_mirror.\n");

        for (disk_index = 0;     
            disk_index < FBE_FRU_DESCRIPTOR_DB_DRIVE_NUMBER; disk_index++)
        {
            disk_mask = 1 << disk_index;
            status = fbe_database_get_homewrecker_fru_descriptor(&system_disk_table[disk_index].dp, 
                                                                  &system_disk_table[disk_index].dp_report, disk_mask);
            system_disk_table[disk_index].dp_report.call_status = status;

            /* Comments added @ 2013 Feb 27th
              * Removed dp_report.status check, because this status is got from raw_mirror
              * we can not trust in raw_mirror here. So the status return from raw_mirror should 
              * be removed
              */
            if (FBE_STATUS_OK != status)
            {
                system_disk_table[disk_index].is_invalid_system_drive = FBE_TRUE;
                database_trace(FBE_TRACE_LEVEL_INFO, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "Homewrecker: Read fru descriptor from disk 0_0_%d failed.\n",
                                disk_index);
                continue;
            }

            /* output the FRU Descriptor got from this disk into trace */
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                           "Homewrecker: on-disk FRU_Descriptor from disk 0_0_%d:\n",
                                           disk_index);
            fbe_database_display_fru_descriptor(&(system_disk_table[disk_index].dp));
        }
            
        /* all 3 copies of fru_descriptor on each DB drive are got
          * it is time to determine the standard fru_descriptor
          */
        status = fbe_database_homewrecker_determine_standard_fru_descriptor(system_disk_table, standard_disk);    
        if (FBE_STATUS_OK != status)
        {
            database_trace (FBE_TRACE_LEVEL_WARNING,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: Failed to get standard fru descriptor\n",
                            __FUNCTION__);
            return status;
        }

        /* After we set standard_disk->dp, we should set global_fru_descriptor_sequence_number.
          * In order to let Homewrecker get correct seq_num in possible following FRU_Descriptor update
          */
        status = database_set_fru_descriptor_sequence_number(standard_disk->dp.sequence_number);
        if (status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            "Homewrecker: Set fru descriptor sequence number failed.\n");
            return FBE_STATUS_GENERIC_FAILURE;
         }
    }
    
    /* Read the FRU Signature from each of first four disks */
    database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "Homewrecker: Read FRU Signature  from each disk in first 4 drives.\n");

    for (disk_index=0; disk_index<FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER; disk_index++)
    {
        status = fbe_database_get_homewrecker_fru_signature(&system_disk_table[disk_index].sig,
                                                            &system_disk_table[disk_index].sig_report,
                                                            0, 0, disk_index);
                                                      
        system_disk_table[disk_index].sig_report.call_status = status;
        if (status != FBE_STATUS_OK)
        {
            system_disk_table[disk_index].is_invalid_system_drive = FBE_TRUE;
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            "Homewrecker: Read FRU Signature  from disk 0_0_%d failed.\n",
                                            disk_index);
        }
        else
        {
            /* Log the trace when read FRU Signature successully */
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                            "Homewrecker: Got FRU Signature  from disk 0_0_%d.\n",
                                            disk_index);
            if ( 0 != system_disk_table[disk_index].sig.bus ||
                 0 != system_disk_table[disk_index].sig.enclosure ||
                 disk_index != system_disk_table[disk_index].sig.slot)
            {
                database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                               "MIR: MirrorReadMcrFruSignature: Wrong slot. Want: %d_%d_%d Found: %d_%d_%d\n",
                               0, 0, disk_index,
                               system_disk_table[disk_index].sig.bus,
                               system_disk_table[disk_index].sig.enclosure,
                               system_disk_table[disk_index].sig.slot);
#ifdef C4_INTEGRATED
                if (standard_disk->dp.wwn_seed == system_disk_table[disk_index].sig.system_wwn_seed)
                {
					tmp_err_file = fopen("/tmp/mirr_vault_wrong_slot","a+");
					fprintf(tmp_err_file,
							"MIR: MirrorReadMcrFruSignature: Wrong slot. Want: %d_%d_%d Found: %d_%d_%d\n",
                                0, 0, disk_index,
                                system_disk_table[disk_index].sig.bus,
                                system_disk_table[disk_index].sig.enclosure,
                                system_disk_table[disk_index].sig.slot);
					fclose(tmp_err_file);
                }
#endif
            }

            /* display fru_signature */
            fbe_database_display_fru_signature(&(system_disk_table[disk_index].sig));
        }
    }

    return FBE_STATUS_OK;
   
}


/*!**************************************************************************************************
@fn fbe_database_homewrecker_process()
*****************************************************************************************************

* @brief
*  This function reads DB drives' FRU descriptor by raw mirror interface and FRU signature. 
*  Then check what's the combination of these data, they show what's the situation system is in. 
*  Homewrecker runs corresponding logic to resolve this situation: go on booting or enter service mode.
* 
* @param database_service_ptr- pointer to database service
* 
* @return
*  fbe_status_t
*
*****************************************************************************************************/

fbe_status_t fbe_database_homewrecker_process(fbe_database_service_t *database_service_ptr)
{
    fbe_status_t    status = FBE_STATUS_INVALID;
    fbe_homewrecker_fru_descriptor_t fru_descriptor_mem = {0};
    homewrecker_system_disk_table_t system_disk_table[FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER] = {0};
    homewrecker_system_disk_table_t    standard_disk = {0};

    database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_INFO, 
                   "%s: begin homewrecker check\n", __FUNCTION__);

    /*Generate disk fru descriptor structure in memory according to actual disks*/
    status = fbe_database_system_disk_fru_descriptor_inmemory_init(&fru_descriptor_mem);
    if (FBE_STATUS_OK != status)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Failed to compose fru descriptor\n",
                        __FUNCTION__);
        return status;
    }

    status = database_homewrecker_build_internal_table(system_disk_table, &standard_disk);
    if (FBE_STATUS_OK != status)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Failed to build intermediate table in Homewrecker logic\n",
                        __FUNCTION__);
        return status;
    }
    
    status = database_homewrecker_get_disk_type(system_disk_table, &fru_descriptor_mem, &standard_disk);
    if (FBE_STATUS_OK != status)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Failed to determine disk type\n",
                        __FUNCTION__);
        return status;
    }
    
    return database_homewrecker_logic(database_service_ptr, system_disk_table, 
                                      &standard_disk, &fru_descriptor_mem);
    
}

static void database_mini_homewrecker_get_two_invalid_system_drive_index(homewrecker_system_disk_table_t *sdt, 
                                                                         fbe_u32_t *first_invalid_index, 
                                                                         fbe_u32_t *second_invalid_index)
{
    fbe_u32_t   disk_index = 0;

    *first_invalid_index = FBE_U32_MAX;
    *second_invalid_index = FBE_U32_MAX;
    for (disk_index = 0; disk_index < FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER; disk_index++)
    {
        if (sdt[disk_index].is_invalid_system_drive == FBE_TRUE)
        {
            if (*first_invalid_index == FBE_U32_MAX)
            {
                *first_invalid_index = disk_index;
            } else if (*second_invalid_index == FBE_U32_MAX)
            {
                *second_invalid_index = disk_index;
            } else 
            {
                break;
            }
        }
    }

    return;
}

static fbe_u32_t database_homewrecker_peer_disk_slot(fbe_u32_t drive_slot)
{
    fbe_u32_t peer_slot = FBE_U32_MAX;

    switch (drive_slot)
    {
    case MIRROR_SPA_BOOT_PRIMARY_TARGET_ID:
        peer_slot =  MIRROR_SPA_BOOT_SECONDARY_TARGET_ID;
        break;

    case MIRROR_SPB_BOOT_PRIMARY_TARGET_ID:
        peer_slot =  MIRROR_SPB_BOOT_SECONDARY_TARGET_ID;
        break;

    case MIRROR_SPA_BOOT_SECONDARY_TARGET_ID:
        peer_slot =  MIRROR_SPA_BOOT_PRIMARY_TARGET_ID;
        break;

    case MIRROR_SPB_BOOT_SECONDARY_TARGET_ID:
        peer_slot =  MIRROR_SPB_BOOT_PRIMARY_TARGET_ID;
        break;
    default:
        break;
    }

    return peer_slot;
}

static fbe_status_t database_mini_homewrecker_logic(fbe_database_service_t *database_service_ptr,
                                                    homewrecker_system_disk_table_t* sdt,
                                                    homewrecker_system_disk_table_t* standard_disk,
                                                    fbe_homewrecker_fru_descriptor_t* dp)
{
    fbe_status_t    status = FBE_STATUS_INVALID;
    fbe_u32_t       disk_index = 0;
    fbe_u32_t       invalid_disk = 0;
    homewrecker_service_mode_type_t service_mode_type = NO_SERVICE_MODE;
    fbe_u32_t   first_invalid_index = FBE_U32_MAX;
    fbe_u32_t   second_invalid_index = FBE_U32_MAX;
    fbe_u32_t   invalid_disks[FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER];
    fbe_u32_t   invalid_disk_num = 0;

#ifdef C4_INTEGRATED
    FILE * tmp_err_file;
#endif

    database_trace (FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                    "Mini Homewrecker: %s entry.\n", __FUNCTION__);

    if (sdt == NULL || database_service_ptr == NULL || dp == NULL || standard_disk == NULL)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: illegal parameter\n",
                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*according to descriptor status reported, mark disks invalid*/
    for (disk_index=0; disk_index<FBE_FRU_DESCRIPTOR_DB_DRIVE_NUMBER; disk_index++)
    {
        if (sdt[disk_index].dp.wwn_seed != standard_disk->dp.wwn_seed ||
            sdt[disk_index].dp.sequence_number!= standard_disk->dp.sequence_number)
        {
            sdt[disk_index].is_invalid_system_drive = FBE_TRUE;
        }
    }

    /*according to signature status reported, mark disks invalid*/
    for (disk_index=0; disk_index<FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER; disk_index++)
    {   /*read signature status*/
        if (sdt[disk_index].sig_report.call_status != FBE_STATUS_OK)
        {
            sdt[disk_index].is_invalid_system_drive = FBE_TRUE;
        }

        /*mark disk invalid according to disk types*/
        if (database_homewrecker_invalid_disk_type(sdt[disk_index].disk_type))
            sdt[disk_index].is_invalid_system_drive = FBE_TRUE;

        /* If the system drive is not in its correct slot, set it invalid */
        if ( 0 != sdt[disk_index].sig.bus || 0 != sdt[disk_index].sig.enclosure ||
             disk_index != sdt[disk_index].sig.slot) 
        { 
            sdt[disk_index].is_invalid_system_drive = FBE_TRUE;
        }

        /*invalid disk count.*/
        if (sdt[disk_index].is_invalid_system_drive == FBE_TRUE)
            invalid_disk++;

    }   
    /* Get the drive actual location */
    if (invalid_disk > 0)
    {
        status = database_homewrecker_get_system_drive_actual_location(sdt, standard_disk);
        if (status != FBE_STATUS_OK)
        {
            database_trace (FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: fail to get system drives actual location\n", __FUNCTION__);
            return status;
        }
    }

    do{
//begin - add for Replace Chassis purpose
        if (dp->wwn_seed != standard_disk->dp.wwn_seed)
        {
            database_trace (FBE_TRACE_LEVEL_WARNING, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "Homewrecker: Chassis wwnseed 0x%x is mismatched with standard disk wwnseed 0x%x .\n",dp->wwn_seed,standard_disk->dp.wwn_seed);
            set_database_service_state(database_service_ptr, FBE_DATABASE_STATE_BOOTFLASH_SERVICE_MODE);
            service_mode_type = BOOTFLASH_SERVICE_MODE;
#ifdef C4_INTEGRATED
	        tmp_err_file = fopen("/tmp/chassis_and_systemdrive_wwnseed_mismatched_fault","a+");
						
	        fprintf(tmp_err_file,"Chassis wwnseed 0x%x is mismatched with standard disk wwnseed 0x%x.\n",dp->wwn_seed, standard_disk->dp.wwn_seed);
					
	        fclose(tmp_err_file);
#endif
	        break;
        }
//end -add for Replace Chassis purpose

        /*If more than 2 invalid disks, enter service mode */
        if (invalid_disk > 2) 
        {
            database_trace (FBE_TRACE_LEVEL_WARNING, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "Homewrecker: More than 2 system drives are invalid.\n");
            /*system drives integrienter broken, enter service mode*/
            set_database_service_state(database_service_ptr, FBE_DATABASE_STATE_BOOTFLASH_SERVICE_MODE);
            service_mode_type = BOOTFLASH_SERVICE_MODE;

            for (disk_index = 0; disk_index < FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER; disk_index++)
            {
                invalid_disks[disk_index] = FBE_U32_MAX;
            }
            invalid_disk_num = 0;
            for (disk_index = 0; disk_index < FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER; disk_index++)
            {
                if (sdt[disk_index].is_invalid_system_drive == FBE_TRUE)
                {
                    invalid_disks[invalid_disk_num++] = disk_index;
                }
            }

#ifdef C4_INTEGRATED
            tmp_err_file = fopen("/tmp/three_or_more_vault_drives_fault","a+");
            if (invalid_disk_num == 3)
            {
				fprintf(tmp_err_file,
						"MIR: system drive: %d_%d_%d and %d_%d_%d and %d_%d_%d are invalid drives\n",
                            0, 0, invalid_disks[0],
                            0, 0, invalid_disks[1],
                            0, 0, invalid_disks[2]);
            }
            if (invalid_disk_num == 4)
            {
				fprintf(tmp_err_file,
						"MIR: system drive: %d_%d_%d and %d_%d_%d and %d_%d_%d and %d_%d_%d are invalid drives\n",
                            0, 0, invalid_disks[0],
                            0, 0, invalid_disks[1],
                            0, 0, invalid_disks[2],
                            0, 0, invalid_disks[3]);
            
            }
			fclose(tmp_err_file);
#endif
            break;
        } else if (invalid_disk == 2)
        {
            database_mini_homewrecker_get_two_invalid_system_drive_index(sdt, &first_invalid_index, &second_invalid_index);
            if (first_invalid_index == FBE_U32_MAX || second_invalid_index == FBE_U32_MAX)
            {
                /* Expect 2 system drive are invalid, but we didn't found enough invalid disk,
                 * In general, we won't enter this case.
                 */ 
                database_trace (FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "MiniHomewrecker: expect 2 invalid disks, but didn't get them\n");
                service_mode_type = NO_SERVICE_MODE;
                break;
            }

            /* Two invalid system slot are user drives */
            if (sdt[first_invalid_index].disk_type == FBE_HW_CUR_ARR_USR_DISK &&
                sdt[second_invalid_index].disk_type == FBE_HW_CUR_ARR_USR_DISK)
            {
                /* Two invalid system slots belong to the same mirror */
                if (first_invalid_index == database_homewrecker_peer_disk_slot(second_invalid_index))
                {
                    /* We return error which will cause SEP driver panic. 
                     * Bootflash's bringup_backend script will check the /tmp/mirr_vault_wrong_slot file
                     * It will drop to service mode there.
                     */
                    database_trace (FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: Two system drive slot(0_0_%d, 0_0_%d) are user drives\n",
                                    __FUNCTION__,
                                    first_invalid_index, second_invalid_index);
                    /* Set database state as bootflash service mode, sep_init will return error when detect this mode*/
                    set_database_service_state(database_service_ptr, FBE_DATABASE_STATE_BOOTFLASH_SERVICE_MODE);
                    service_mode_type = BOOTFLASH_SERVICE_MODE;
                    break;
                } else {
                    /* Two invalid system slots belong to different mirror
                     * In this case, continue to boot up in bootflash. SEP driver Homewrecker 
                     * will detect the issue later.
                     * */
                    // database_homewrecker_send_event_log();
                    database_trace (FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: Two system drive slot(0_0_%d, 0_0_%d) are user drives\n",
                                    __FUNCTION__,
                                    first_invalid_index, second_invalid_index);
                    service_mode_type = NO_SERVICE_MODE;
                    break;
                }
            }

            else if (sdt[first_invalid_index].disk_type == FBE_HW_NEW_DISK &&
                     sdt[second_invalid_index].disk_type == FBE_HW_NEW_DISK)
            {
                if (first_invalid_index == database_homewrecker_peer_disk_slot(second_invalid_index))
                {
                    database_trace (FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: Two system drive (0_0_%d, 0_0_%d) are new drives\n",
                                    __FUNCTION__,
                                    first_invalid_index, second_invalid_index);
                    /* Set database state as bootflash service mode, sep_init will return error when detect this mode*/
                    set_database_service_state(database_service_ptr, FBE_DATABASE_STATE_BOOTFLASH_SERVICE_MODE);
                    service_mode_type = BOOTFLASH_SERVICE_MODE;
#ifdef C4_INTEGRATED
					tmp_err_file = fopen("/tmp/mirr_two_new_vault_drive","a+");
					fprintf(tmp_err_file,
							"MIR: system drive: %d_%d_%d and %d_%d_%d are new drives\n",
                                0, 0, first_invalid_index,
                                0, 0, second_invalid_index);
					fclose(tmp_err_file);
#endif
                } else {
                    database_trace (FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: Two system drive (0_0_%d, 0_0_%d) are new drives\n",
                                    __FUNCTION__,
                                    first_invalid_index, second_invalid_index);
                    service_mode_type = NO_SERVICE_MODE;
                }
                break;
            }

            /* Two system drives are two drives from other array */
            else if (sdt[first_invalid_index].disk_type == FBE_HW_OTHER_ARR_SYS_DISK &&
                     sdt[second_invalid_index].disk_type == FBE_HW_OTHER_ARR_SYS_DISK)
            {
                if (first_invalid_index == database_homewrecker_peer_disk_slot(second_invalid_index))
                {
                    database_trace (FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: Two system drive (0_0_%d, 0_0_%d) are system drives from other array\n",
                                    __FUNCTION__,
                                    first_invalid_index, second_invalid_index);
                    /* Set database state as bootflash service mode, sep_init will return error when detect this mode*/
                    set_database_service_state(database_service_ptr, FBE_DATABASE_STATE_BOOTFLASH_SERVICE_MODE);
                    service_mode_type = BOOTFLASH_SERVICE_MODE;
#ifdef C4_INTEGRATED
                    tmp_err_file = fopen("/tmp/mirr_two_system_vault_drive_from_other_array","a+");
                    fprintf(tmp_err_file,
                            "MIR: system drive: %d_%d_%d and %d_%d_%d are system drives from other array\n",
                                0, 0, first_invalid_index,
                                0, 0, second_invalid_index);
                    fclose(tmp_err_file);
#endif
                } else {
                    database_trace (FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: Two system drive (0_0_%d, 0_0_%d) are system drives from other array\n",
                                    __FUNCTION__,
                                    first_invalid_index, second_invalid_index);
                    service_mode_type = NO_SERVICE_MODE;
                }
                break;

            }

            /* Two system drive are missing. NOTE: we need to verify if any driver before SEP will block the booting progress */
            else if (sdt[first_invalid_index].path_enable == FBE_FALSE &&
                     sdt[second_invalid_index].path_enable == FBE_FALSE)
            {
                if (first_invalid_index == database_homewrecker_peer_disk_slot(second_invalid_index))
                {
                    database_trace (FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: Two system drive (0_0_%d, 0_0_%d) are missing\n",
                                    __FUNCTION__,
                                    first_invalid_index, second_invalid_index);
                    /* Set database state as bootflash service mode, sep_init will return error when detect this mode*/
                    set_database_service_state(database_service_ptr, FBE_DATABASE_STATE_BOOTFLASH_SERVICE_MODE);
                    service_mode_type = BOOTFLASH_SERVICE_MODE;
#ifdef C4_INTEGRATED
					tmp_err_file = fopen("/tmp/mirr_two_missing_vault_drive","a+");
					fprintf(tmp_err_file,
							"MIR: system drive: %d_%d_%d and %d_%d_%d are missing\n",
                                0, 0, first_invalid_index,
                                0, 0, second_invalid_index);
					fclose(tmp_err_file);
#endif
                } else {
                    database_trace (FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: Two system drive (0_0_%d, 0_0_%d) are missing\n",
                                    __FUNCTION__,
                                    first_invalid_index, second_invalid_index);
                    service_mode_type = NO_SERVICE_MODE;
                }
                break;
            }
            else {
                if (first_invalid_index == database_homewrecker_peer_disk_slot(second_invalid_index))
                {
                    database_trace (FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: Two system drive (0_0_%d, 0_0_%d) are invalid drives\n",
                                    __FUNCTION__,
                                    first_invalid_index, second_invalid_index);
                    /* Set database state as bootflash service mode, sep_init will return error when detect this mode*/
                    set_database_service_state(database_service_ptr, FBE_DATABASE_STATE_BOOTFLASH_SERVICE_MODE);
                    service_mode_type = BOOTFLASH_SERVICE_MODE;
#ifdef C4_INTEGRATED
					tmp_err_file = fopen("/tmp/mirr_two_invalid_vault_drive","a+");
					fprintf(tmp_err_file,
							"MIR: system drive: %d_%d_%d and %d_%d_%d are invalid\n",
                            0, 0, first_invalid_index,
                            0, 0, second_invalid_index);
					fclose(tmp_err_file);
#endif
                } else {
                    database_trace (FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: Two system drive (0_0_%d, 0_0_%d) are invalid drives\n",
                                    __FUNCTION__,
                                    first_invalid_index, second_invalid_index);
                    service_mode_type = NO_SERVICE_MODE;
                }
                break;

            }
        } else {
            /* One drive invalid or no drive invalid case. MiniHomewrecker doesn't process it */
        }
    }while(0);

    status = database_homewrecker_disconnect_pvd(database_service_ptr, sdt);
    if (FBE_STATUS_OK != status)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Disconnect edge for PVDs failed\n", __FUNCTION__);
        return status;
    }

    switch (service_mode_type)
    {
    case NO_SERVICE_MODE:
        database_trace (FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "Homewrecker: let array boot up\n");
        /*now we can securely initialize the in-memory fru descriptor with the correct data*/
        fbe_spinlock_lock(&database_service_ptr->pvd_operation.system_fru_descriptor_lock);
        fbe_copy_memory(&database_service_ptr->pvd_operation.system_fru_descriptor, &standard_disk->dp, sizeof(fbe_homewrecker_fru_descriptor_t));
        fbe_spinlock_unlock(&database_service_ptr->pvd_operation.system_fru_descriptor_lock);
        return status;

        break;

    case BOOTFLASH_SERVICE_MODE:
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "Homewrecker: DB would enter bootflash service mode\n");
        return FBE_STATUS_SERVICE_MODE;
        break;

    default:
        break;
    }

    return status;
}

fbe_status_t fbe_database_mini_homewrecker_process(fbe_database_service_t *database_service_ptr, 
                                                   homewrecker_system_disk_table_t *system_disk_table)
    
{
    fbe_status_t    status = FBE_STATUS_INVALID;
    fbe_homewrecker_fru_descriptor_t fru_descriptor_mem = {0};
    homewrecker_system_disk_table_t    standard_disk = {0};

    database_trace(FBE_TRACE_LEVEL_INFO,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: begin mini homewrecker check\n", __FUNCTION__);

    if (system_disk_table == NULL)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: illegal parameter\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*Generate disk fru descriptor structure in memory according to actual disks*/
    status = fbe_database_system_disk_fru_descriptor_inmemory_init(&fru_descriptor_mem);
    if (FBE_STATUS_OK != status)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Failed to compose fru descriptor\n",
                        __FUNCTION__);
        return status;
    }

    status = database_homewrecker_build_internal_table(system_disk_table, &standard_disk);
    if (FBE_STATUS_OK != status)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Failed to build intermediate table in Homewrecker logic\n",
                        __FUNCTION__);
        return status;
    }

    status = database_homewrecker_get_disk_type(system_disk_table, &fru_descriptor_mem, &standard_disk);
    if (FBE_STATUS_OK != status)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Failed to determine disk type\n",
                        __FUNCTION__);
        return status;
    }

    return database_mini_homewrecker_logic(database_service_ptr, system_disk_table,
                                           &standard_disk, &fru_descriptor_mem);

}


/*this function will read the ICA flags and based on that zero some PVDs if needed*/
fbe_status_t fbe_database_get_ica_flags(fbe_bool_t *invalidate_configuration)
{
    fbe_status_t    status;

    if (invalidate_configuration == NULL) {
        database_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: input argument is NULL\n",
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*first let's read in the data from the first 3 drives*/
    status = fbe_database_read_ica_flags( );
    if (status != FBE_STATUS_OK) {
        database_trace (FBE_TRACE_LEVEL_WARNING,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Failed to read ICA flags\n",
            __FUNCTION__);
        return status;
    }

#ifndef ALAMOSA_WINDOWS_ENV
    {
        EMCUTIL_STATUS Status;
        csx_u32_t Invalidate = 0;
        Status = EmcutilGetRegistryValueAsUint32("\\Registry\\Machine\\SOFTWARE\\EMC\\K10\\Global", "MCRInvalidate", &Invalidate);
        if (Status == EMCUTIL_STATUS_OK && Invalidate != 0) {
            EmcutilSetRegistryValueAsUint32("\\Registry\\Machine\\SOFTWARE\\EMC\\K10\\Global", "MCRInvalidate", 0);
            fbe_database_imaging_flags.invalidate_configuration = FBE_IMAGING_FLAGS_TRUE;
        }
    }
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - back door way to set this in Linux use case */

    if (fbe_database_imaging_flags.invalidate_configuration == FBE_IMAGING_FLAGS_TRUE) {
        *invalidate_configuration = FBE_TRUE;
    }else{
        *invalidate_configuration = FBE_FALSE;
    }

    return FBE_STATUS_OK;
}

fbe_status_t fbe_database_get_imaging_flags(fbe_imaging_flags_t *imaging_flags)
{
    fbe_status_t status;

    if (imaging_flags == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Every time we get the imaging flags, we read it from the disks. 
     * */
    status = fbe_database_read_ica_flags();
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to read ica flags, status %d\n", __FUNCTION__, status);
        return status;
    }

    fbe_copy_memory(imaging_flags, &fbe_database_imaging_flags, sizeof(fbe_imaging_flags_t));

    return FBE_STATUS_OK;
}


/*this function read in the ICA flgs from the first 3 drives*/
static fbe_status_t fbe_database_read_ica_flags( )
{
    database_physical_drive_info_t drive_location_info[FBE_DB_NUMBER_OF_RESOURCES_FOR_ICA_OPERATION];
    fbe_u32_t       count;
    fbe_u32_t       retry_count = 0;
    fbe_status_t 	status;

    for(count = 0 ; count < FBE_DB_NUMBER_OF_RESOURCES_FOR_ICA_OPERATION; count++)
    {
        drive_location_info[count].enclosure_number = 0;
        drive_location_info[count].port_number = 0;
        drive_location_info[count].slot_number = count;
        /*! @todo The systems drives cannot be 4160-bps */
        drive_location_info[count].block_geometry = FBE_BLOCK_EDGE_GEOMETRY_520_520;
        drive_location_info[count].logical_drive_object_id = FBE_OBJECT_ID_INVALID;
    }

    fbe_zero_memory(&fbe_database_imaging_flags, sizeof fbe_database_imaging_flags);

    status = fbe_database_read_flags_from_raw_drive(&drive_location_info[0], 
                                                    FBE_DB_NUMBER_OF_RESOURCES_FOR_ICA_OPERATION,
                                                    FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_ICA_FLAGS, 
                                                    fbe_database_validate_ica_stamp_read,
                                                    FBE_FALSE);

    while((status != FBE_STATUS_OK && status != FBE_STATUS_SERVICE_MODE) && (retry_count < FBE_DB_FAILED_RAW_IO_RETRIES)){
        /* Wait 1 second */
        database_trace(FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_INFO, 
            "%s: failed to read ica flags, status %d\n", __FUNCTION__, status);
        fbe_thread_delay(FBE_DB_FAILED_RAW_IO_RETRY_INTERVAL);
        retry_count++;
        status = fbe_database_read_flags_from_raw_drive(&drive_location_info[0], 
                                                        FBE_DB_NUMBER_OF_RESOURCES_FOR_ICA_OPERATION,
                                                        FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_ICA_FLAGS, 
                                                        fbe_database_validate_ica_stamp_read, 
                                                        FBE_FALSE);
    }
    if (status != FBE_STATUS_OK && status != FBE_STATUS_SERVICE_MODE) {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_INFO, 
            "%s: failed to read ica flags\n", __FUNCTION__);
        status = fbe_database_read_flags_from_raw_drive(&drive_location_info[0], 
                                                        FBE_DB_NUMBER_OF_RESOURCES_FOR_ICA_OPERATION,
                                                        FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_ICA_FLAGS, 
                                                        fbe_database_validate_ica_stamp_read, 
                                                        FBE_TRUE);  // force to validate with whatever we got back
    }
    return status;
}


/*this function takes the memory chinks and carves them into packets and SG lists*/
static void fbe_database_carve_memory_for_packet_and_sgl(fbe_packet_t **packet_aray,
                                                         fbe_sg_element_t **sg_list_array,
                                                         fbe_u32_t number_of_drives_for_operation,
                                                         fbe_memory_request_t *memory_request)
{
    fbe_u32_t                    count = 0;
    fbe_memory_header_t *       next_memory_header_p = NULL;
    fbe_memory_header_t *        master_memory_header = NULL;
    fbe_u8_t *                    chunk_start_address = NULL;

    master_memory_header = fbe_memory_get_first_memory_header(memory_request);

    /*let's start with the master packet in the first chunk*/
    *packet_aray = (fbe_packet_t *)fbe_memory_header_to_data(master_memory_header);
    fbe_zero_memory(*packet_aray, sizeof(fbe_packet_t));

    packet_aray++;


    while (count < number_of_drives_for_operation) {

        /*let's get the start of the next chunk*/
        fbe_memory_get_next_memory_header(master_memory_header, &next_memory_header_p);
        master_memory_header = next_memory_header_p;/*for next round*/

        /*and at the start of it put a packet*/
        chunk_start_address  = (fbe_u8_t *)fbe_memory_header_to_data(next_memory_header_p);
        *packet_aray = (fbe_packet_t *)chunk_start_address;
        fbe_zero_memory(*packet_aray, sizeof(fbe_packet_t));
        chunk_start_address += sizeof(fbe_packet_t);/*roll it forward so we can carve memory for the sg list*/
        packet_aray++;/*go to next packet*/

        /*and then an SG list*/
        *sg_list_array = (fbe_sg_element_t *)chunk_start_address;
        fbe_zero_memory(*sg_list_array, sizeof(fbe_sg_element_t) * 2);  /*the second sg elment is terminator*/
        sg_list_array++;/*go to next sg element*/

        count++;
    }

}


/*see if we need to invalidate the LUNs or not*/
fbe_status_t fbe_database_reset_ica_flags(void)
{
    fbe_status_t     status;
    fbe_u32_t        wait_ms = 0;

    if (fbe_database_imaging_flags.invalidate_configuration == FBE_IMAGING_FLAGS_TRUE) {
        database_trace (FBE_TRACE_LEVEL_INFO,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: ICA invalidate_configuration flag is true, will zero all private LUNs\n", __FUNCTION__ );
        /* invalidate the ICA stamp on disk*/
        do {
            status = fbe_database_set_invalidate_configuration_flag(FBE_IMAGING_FLAGS_FALSE);
            if (status != FBE_STATUS_OK) {
                database_trace (FBE_TRACE_LEVEL_WARNING,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: Failed to clear invalidate_configuration flag on PVDs, retrying...\n", __FUNCTION__ );

                wait_ms = 5000;
            }

            fbe_thread_delay(wait_ms);

        } while (status != FBE_STATUS_OK);

    }

    return FBE_STATUS_OK;
}


/* send write packets to the specified pdos in input parameter drive_locations
* if a drive is not available (missed or is not in READY state), the drive_location's
* logical_drive_object_id would be set FBE_OBJECT_ID_INVALID
*/
static fbe_status_t fbe_database_send_read_packets_to_pdos(fbe_packet_t **packet_array,
                                                    fbe_sg_element_t **sg_list_array,
                                                    database_physical_drive_info_t* drive_locations,
                                                    fbe_u32_t  number_of_drives_for_operation,
                                                    fbe_block_count_t   block_count_for_io,
                                                    fbe_private_space_layout_region_id_t  pls_id,
                                                    fbe_u32_t*   complete_packet_count)
{
    fbe_packet_t                        *master_packet_p = NULL;
    fbe_packet_t                        *sub_packet = NULL;
    fbe_packet_t                        **io_packet = packet_array;
    fbe_payload_ex_t                    *sep_payload = NULL;
    fbe_payload_block_operation_t    *    block_operation = NULL;
    fbe_sg_element_t*                        sg_list;
    fbe_u32_t                            count = 0;
    fbe_u32_t                                                    sg_list_count;
    fbe_private_space_layout_region_t    psl_region_info;
    fbe_status_t                        status;
    fbe_bool_t*                                          is_drive_missing;
    fbe_u32_t                           miss_drive_count = 0;
    fbe_cpu_id_t                        cpu_id;
    fbe_topology_control_get_physical_drive_by_location_t    topology_drive_location;
    database_pdo_io_context_t   io_context;
    fbe_lifecycle_state_t lifecycle_state;

    if(NULL == packet_array || NULL == sg_list_array || NULL == drive_locations)
        return FBE_STATUS_GENERIC_FAILURE;

    master_packet_p = *packet_array;  /*first packet is the master one*/

    /*get some information about the flag region*/
    fbe_private_space_layout_get_region_by_region_id(pls_id, &psl_region_info);

    is_drive_missing = (fbe_bool_t*)fbe_memory_native_allocate(number_of_drives_for_operation * sizeof(fbe_bool_t));
    if(!is_drive_missing)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_send_read_2_pdo: can not alloc memory for is_drive_missing\n" );

        return FBE_STATUS_GENERIC_FAILURE;
    }
    for(count = 0; count < number_of_drives_for_operation; count++)
        is_drive_missing[count] = FBE_FALSE;

    fbe_transport_initialize_sep_packet(master_packet_p);/*nothing else to do here*/
    cpu_id = fbe_get_cpu_id();
    fbe_transport_set_cpu_id(master_packet_p, cpu_id);

    packet_array++;

    fbe_semaphore_init(&(io_context.semaphore), 0, 1);
    io_context.pdo_io_complete = 0;

    /*now let's use the other packets*/
    for (count = 0; count < number_of_drives_for_operation; count++) {
        sub_packet = *packet_array;

        /*copy database physical drive info into topology drive location structure*/
        topology_drive_location.enclosure_number = drive_locations[count].enclosure_number;
        topology_drive_location.slot_number= drive_locations[count].slot_number;
        topology_drive_location.port_number= drive_locations[count].port_number;
        topology_drive_location.physical_drive_object_id= FBE_OBJECT_ID_INVALID;

        /*get logical drive object*/
        status = fbe_database_send_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_GET_PHYSICAL_DRIVE_BY_LOCATION,
                                                     &topology_drive_location, 
                                                     sizeof(topology_drive_location),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     NULL,    /* no sg list*/
                                                     0,  /* no sg list*/
                                                     FBE_PACKET_FLAG_EXTERNAL,
                                                     FBE_PACKAGE_ID_PHYSICAL);
        if(status != FBE_STATUS_OK && status != FBE_STATUS_NO_OBJECT){
                drive_locations[count].logical_drive_object_id = FBE_OBJECT_ID_INVALID;
                fbe_semaphore_destroy(&(io_context.semaphore));
                fbe_memory_native_release(is_drive_missing);
                fbe_database_clean_destroy_packet(master_packet_p);
                return status;
        }

        /* Keep track of the missing drives */
        if(FBE_STATUS_NO_OBJECT == status) {
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: fail to get pdo objid by location(%d_%d_%d). status:0x%x\n", 
                           __FUNCTION__, 
                           topology_drive_location.port_number,
                           topology_drive_location.enclosure_number,
                           topology_drive_location.slot_number,
                           status);
            drive_locations[count].logical_drive_object_id = FBE_OBJECT_ID_INVALID;
            is_drive_missing[count] = FBE_TRUE;
            miss_drive_count++;
            packet_array++;/*move to next packet*/
            sg_list_array++;/*move to new sg list*/
            continue;
        }

        /* Fix AR 698912. If the pdo is not ready, we shouldn't send packet to read ica flag */
        status = fbe_database_generic_get_object_state(topology_drive_location.physical_drive_object_id, &lifecycle_state, FBE_PACKAGE_ID_PHYSICAL);
        if (FBE_STATUS_OK != status || FBE_LIFECYCLE_STATE_READY != lifecycle_state)
        {
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s:fail get pdo 0x%x lifecycle, not ready. status:0x%x, state:%d\n", 
                           __FUNCTION__, 
                           topology_drive_location.physical_drive_object_id,
                           status, lifecycle_state);
            drive_locations[count].logical_drive_object_id = FBE_OBJECT_ID_INVALID;
            is_drive_missing[count] = FBE_TRUE;
            miss_drive_count++;
            packet_array++;/*move to next packet*/
            sg_list_array++;/*move to new sg list*/
            continue;
        }

        drive_locations[count].logical_drive_object_id = topology_drive_location.physical_drive_object_id;

        fbe_transport_initialize_sep_packet(sub_packet);

        /* set block operation same as master packet's prev block operation. */
        sep_payload = fbe_transport_get_payload_ex(sub_packet);
        block_operation =  fbe_payload_ex_allocate_block_operation(sep_payload);

        /* set sg list with new packet. */
        sg_list = *sg_list_array;
        sg_list_count = 0;
        while(sg_list->address != NULL)
        {
            sg_list++;
            sg_list_count++;
        }
        fbe_payload_ex_set_sg_list(sep_payload, *sg_list_array, sg_list_count + 1); /*one for terminator*/

        fbe_payload_block_build_operation(block_operation,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
            psl_region_info.starting_block_address,
            block_count_for_io,
            FBE_BE_BYTES_PER_BLOCK,
            1,
            NULL);

        /* set completion function */
        fbe_transport_set_completion_function(sub_packet,
            fbe_database_send_io_packets_to_pdos_completion,
            &io_context);

        /* we need to assosiate newly allocated packet with original one */
        fbe_transport_add_subpacket(master_packet_p, sub_packet);

        /*set address correctly*/
        fbe_transport_set_address(sub_packet,
            FBE_PACKAGE_ID_PHYSICAL,
            FBE_SERVICE_ID_TOPOLOGY,
            FBE_CLASS_ID_INVALID,
            topology_drive_location.physical_drive_object_id);

        fbe_payload_ex_increment_block_operation_level(sep_payload);

        packet_array++;/*move to next packet*/
        sg_list_array++;/*move to new sg list*/
    }

    if(miss_drive_count == number_of_drives_for_operation)
    {
        fbe_semaphore_destroy(&(io_context.semaphore));
        fbe_database_clean_destroy_packet(master_packet_p);
        fbe_memory_native_release(is_drive_missing);
        database_trace (FBE_TRACE_LEVEL_WARNING,
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_send_read_2_pdo: all drives are missed, no read packet is sent out.\n" );

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now let's send the IO packets one by one */
    io_packet++;
    for (count = 0; count < number_of_drives_for_operation; count++)
    {
        if(is_drive_missing[count])
        {
            io_packet++;
            continue;
        }

        sub_packet = *io_packet;
        status = fbe_topology_send_io_packet(sub_packet);

        /*The drive may not be READY to process io request. In this case, the return status
        * is not OK. We do not need to remove and destroy the subpacket here because
        * this is done in the completion function of the packet*/
        if(FBE_STATUS_OK != status)
        {
            database_trace (FBE_TRACE_LEVEL_WARNING,
                FBE_TRACE_MESSAGE_ID_INFO,
                "db_send_read_2_pdo: send packet to pdo 0x%x failed.\n", 
                drive_locations[count].logical_drive_object_id);
        }

        io_packet++;/*move to next packet*/
    }

    /*let's wait for the semaphore to complete*/
    status = fbe_semaphore_wait_ms(&(io_context.semaphore), 0);
    
    /* see if there's problem reported by the lower leverl */
    status = fbe_transport_get_status_code(master_packet_p);

    fbe_semaphore_destroy(&(io_context.semaphore));
    fbe_memory_native_release(is_drive_missing);
    fbe_database_clean_destroy_packet(master_packet_p);

    if(NULL != complete_packet_count)
        *complete_packet_count = (fbe_u32_t)(io_context.pdo_io_complete);

    if(FBE_STATUS_OK != status) 
    {                
        database_trace (FBE_TRACE_LEVEL_WARNING,
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_send_read_2_pdo failed, need to retry\n" );

        return status;/*user will want to retry the write*/
    }

    return FBE_STATUS_OK;
}


fbe_status_t 
fbe_database_send_io_packets_to_pdos_completion(fbe_packet_t * packet,
                                                  fbe_packet_completion_context_t context)
{
    database_pdo_io_context_t*              io_context = (database_pdo_io_context_t *)context;
    fbe_payload_ex_t *                      sep_payload = NULL;
    fbe_payload_block_operation_t    *      block_operation = NULL;
    fbe_packet_t                     *      master_packet_p = NULL;
    fbe_payload_block_operation_status_t    block_status;
    fbe_payload_block_operation_qualifier_t block_qualifier; 
    fbe_bool_t                              is_empty;
    fbe_status_t                            packet_status;

    /* get the master packet. */
    master_packet_p = fbe_transport_get_master_packet(packet);

    packet_status = fbe_transport_get_status_code(packet);

    sep_payload = fbe_transport_get_payload_ex(packet);
    block_operation = fbe_payload_ex_get_block_operation(sep_payload);  
    fbe_payload_block_get_status(block_operation, &block_status);
    fbe_payload_block_get_qualifier(block_operation, &block_qualifier );

    fbe_payload_ex_release_block_operation(sep_payload, block_operation);

    if ((FBE_STATUS_OK != packet_status) ||
        (FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS != block_status))
    {
        database_trace (FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_INFO, 
            "db  read pdos compl: failed packet 0x%x, block 0x%x/0x%x\n",
            packet_status, block_status, block_qualifier);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
    }
    else
    {
        /* Don't increment if the block status is not OK. */
        fbe_atomic_increment(&(io_context->pdo_io_complete));
    }

    /* Fix AR 595773: This is a critical section from where we decrease 
     * the subpacket_count of master_packet to where we check if the sub packet is empty.
     * If there are more operations happened between these two points, must be careful about it.
     * We must check if we need to protect it with lock.  
     * In this case, we don't need to protect it with lock because there is no more operations 
     * between them. It is an atomical operation.
     **/
    fbe_transport_remove_subpacket_is_queue_empty(packet, &is_empty);
    if(is_empty){
        fbe_transport_destroy_subpackets(master_packet_p);
        packet_status = fbe_transport_get_status_code(master_packet_p);
        if (FBE_STATUS_INVALID == packet_status) {
            fbe_transport_set_status(master_packet_p, FBE_STATUS_OK, 0);
        }
        /* If we are done with all drives just bang the semaphore and the blocking code will continue*/
        fbe_semaphore_release(&(io_context->semaphore), 0, 1, FALSE);
    }

    return FBE_STATUS_OK;
}


/*check the signature and ICA values we got from one or more drives*/
static fbe_status_t fbe_database_validate_ica_stamp_read(fbe_u8_t *flags_array, 
                                                                       fbe_bool_t all_drives_read)
{
    fbe_imaging_flags_t*    imaging_flags_pvd[FBE_DB_NUMBER_OF_RESOURCES_FOR_ICA_OPERATION];
    fbe_s32_t               magic_string_valid[FBE_DB_NUMBER_OF_RESOURCES_FOR_ICA_OPERATION];
    fbe_u32_t               imaging_flags_state[FBE_DB_NUMBER_OF_RESOURCES_FOR_ICA_OPERATION];
    fbe_bool_t              all_magic_string_valid = FBE_TRUE; /*Initial value is FBE_TRUE */
    fbe_bool_t              all_imaging_flags_set = FBE_TRUE; /*Initial value is FBE_TRUE */
    fbe_bool_t              at_least_one_imaging_flags_set = FBE_FALSE;
    fbe_database_state_t    db_state;
    fbe_u32_t               diskcnt = 0;

    for (diskcnt = 0; diskcnt < FBE_DB_NUMBER_OF_RESOURCES_FOR_ICA_OPERATION; diskcnt++)
    {
        imaging_flags_pvd[diskcnt] = (fbe_imaging_flags_t *)
                                     (flags_array + diskcnt * FBE_BE_BYTES_PER_BLOCK);
        /* Check if the magic string is valid */
        magic_string_valid[diskcnt] = !fbe_compare_string(imaging_flags_pvd[diskcnt]->magic_string,
                                                 FBE_IMAGING_FLAGS_MAGIC_STRING_LENGTH,
                                                 FBE_IMAGING_FLAGS_MAGIC_STRING,
                                                 FBE_IMAGING_FLAGS_MAGIC_STRING_LENGTH, FBE_TRUE);

        if (magic_string_valid[diskcnt] == 0 ) { /*If any one is invalid, set all_magic_string_valid as false */
            all_magic_string_valid = FBE_FALSE;
            imaging_flags_state[diskcnt] = 1;  /*1 means magic string is not valid*/
        } else { /* When the magic string is valid, judge if the imaging flags is set */
            if (imaging_flags_pvd[diskcnt]->invalidate_configuration == FBE_IMAGING_FLAGS_TRUE) {
                imaging_flags_state[diskcnt] = 2;  /*2 means imaging flags is set*/
                at_least_one_imaging_flags_set = FBE_TRUE;
            } else {
                all_imaging_flags_set = FBE_FALSE;
                imaging_flags_state[diskcnt] = 3;  /*3 mean magic string is valid but imaging flags is not set*/
            }

        }

        /* Debug info*/
        database_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "ICA Flags Disk %d magic_string: %s\n", 
                       diskcnt, imaging_flags_pvd[diskcnt]->magic_string);

        database_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "ICA Flags Disk %d structure_version: 0x%08X\n", 
                       diskcnt, imaging_flags_pvd[diskcnt]->structure_version);

        database_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "ICA Flags Disk %d invalidate_configuration: 0x%08X\n", 
                       diskcnt, imaging_flags_pvd[diskcnt]->invalidate_configuration);

        database_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "ICA Flags Disk %d imaging_stamp: 0x%08X\n", 
                       diskcnt, imaging_flags_pvd[diskcnt]->imaging_stamp);

        database_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "ICA Flags Disk %d imaging_started: 0x%08X\n", 
                       diskcnt, imaging_flags_pvd[diskcnt]->imaging_started);

        database_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "ICA Flags Disk %d imaging_completed: 0x%08X\n", 
                       diskcnt, imaging_flags_pvd[diskcnt]->imaging_completed);

        database_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "ICA Flags Disk %d imaging_mechanism: 0x%08X\n", 
                       diskcnt, imaging_flags_pvd[diskcnt]->imaging_mechanism);
    }

    if (all_magic_string_valid && all_imaging_flags_set ) { /*Only when all magic string is valid and all imaging flags are set, ICA flags is thought as set*/
        fbe_copy_memory(&fbe_database_imaging_flags, imaging_flags_pvd[0], 
                        sizeof(fbe_imaging_flags_t));
        fbe_database_imaging_flags.invalidate_configuration = FBE_IMAGING_FLAGS_TRUE;
    } else {
        /*If Not all magic string are valid and at least one imaging flags is set -> enter degraged mode */
        if (!all_magic_string_valid && at_least_one_imaging_flags_set) {
            database_trace(FBE_TRACE_LEVEL_WARNING,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "ICA flags is set, but not all drives are valid\n");
            fbe_database_get_state(&db_state);
            if (db_state != FBE_DATABASE_STATE_SERVICE_MODE) {
                fbe_database_enter_service_mode(FBE_DATABASE_SERVICE_MODE_REASON_NOT_ALL_DRIVE_SET_ICA_FLAGS);
                fbe_event_log_write(SEP_ERROR_CODE_NOT_ALL_DRIVES_SET_ICA_FLAGS, 
                                    NULL,
                                    0,
                                    "%x %x %x", 
                                    imaging_flags_state[0], imaging_flags_state[1], imaging_flags_state[2]);
            }
            database_trace(FBE_TRACE_LEVEL_WARNING,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "DB enter degraded mode. Imaging state of drive 0,1,2 are (%d, %d, %d) \n",
                           imaging_flags_state[0], imaging_flags_state[1], imaging_flags_state[2]);
            /*Although we don't know whether ica flags is set or not, we still initialize the glocal variable as false */
            fbe_zero_memory(&fbe_database_imaging_flags, sizeof(fbe_imaging_flags_t));
            for (diskcnt = 0; diskcnt < FBE_DB_NUMBER_OF_RESOURCES_FOR_ICA_OPERATION; diskcnt++)
            {
                if(magic_string_valid[diskcnt] != 0)
                {
                    fbe_copy_memory(&fbe_database_imaging_flags, imaging_flags_pvd[diskcnt], 
                                    sizeof(fbe_imaging_flags_t));
                    break;
                }
            }
            fbe_database_imaging_flags.invalidate_configuration = FBE_IMAGING_FLAGS_FALSE;
            return FBE_STATUS_SERVICE_MODE;
        } else {
            fbe_zero_memory(&fbe_database_imaging_flags, sizeof(fbe_imaging_flags_t));
            for (diskcnt = 0; diskcnt < FBE_DB_NUMBER_OF_RESOURCES_FOR_ICA_OPERATION; diskcnt++)
            {
                if(magic_string_valid[diskcnt] != 0)
                {
                    fbe_copy_memory(&fbe_database_imaging_flags, imaging_flags_pvd[diskcnt], 
                                    sizeof(fbe_imaging_flags_t));
                    break;
                }
            }
            fbe_database_imaging_flags.invalidate_configuration = FBE_IMAGING_FLAGS_FALSE;
        }
    }

    return FBE_STATUS_OK;
}

fbe_status_t fbe_database_load_global_info_table(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_database_load_from_persist_per_type(DATABASE_CONFIG_TABLE_GLOBAL_INFO);
    if (status != FBE_STATUS_OK) {
        if (status == FBE_STATUS_IO_FAILED_RETRYABLE) {
            database_trace (FBE_TRACE_LEVEL_WARNING,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: Can't load global info table, read single blocks\n", __FUNCTION__ );

            status = fbe_database_load_from_persist_single_entry_per_type(DATABASE_CONFIG_TABLE_GLOBAL_INFO);
            if (status != FBE_STATUS_OK) { 
                database_trace (FBE_TRACE_LEVEL_CRITICAL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: Can't load entry by entry\n", __FUNCTION__ );
            }
        }
    }
    return status;   
}

fbe_status_t fbe_database_load_topology(fbe_database_service_t *database_service_ptr, fbe_bool_t read_from_disk)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    database_table_t * table_ptr = NULL;
    fbe_system_encryption_mode_t system_encryption_mode;
    database_object_entry_t *   object_entry_ptr = NULL;
    fbe_u32_t index;

    /*data may be in the tables if we are the passive side and we got updated by the peer*/
    if (read_from_disk) {
        status = fbe_database_load_from_persist();
        if (status != FBE_STATUS_OK) {
            if (status == FBE_STATUS_IO_FAILED_RETRYABLE) {
                database_trace (FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: Can't load sector, LU degraded, read single blocks\n", __FUNCTION__ );

                status = fbe_database_load_from_persist_single_entry(database_service_ptr);
                if (status != FBE_STATUS_OK) { 
                    database_trace (FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: Can't load entry by entry\n", __FUNCTION__ );
                }
            } else{
                return status;   
            }
        }
    }

    /* create the global info. */
    status = fbe_database_get_service_table_ptr(&table_ptr, DATABASE_CONFIG_TABLE_GLOBAL_INFO);
    if ((status != FBE_STATUS_OK)||(table_ptr == NULL)) {
        return status;
    }
    status = database_common_create_global_info_from_table(table_ptr->table_content.global_info_ptr, table_ptr->table_size);
    if (status != FBE_STATUS_OK){
        return status;
    }

    /* create the objects */
    status = fbe_database_get_service_table_ptr(&table_ptr, DATABASE_CONFIG_TABLE_OBJECT);
    if ((status != FBE_STATUS_OK)||(table_ptr == NULL)) {
        return status;
    }

    /* If the system is not encrypted, cleanup the config type */
    fbe_database_get_system_encryption_mode(&system_encryption_mode);
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "db_load_topology: System Encryption Mode:0x%x\n", system_encryption_mode);

    if (system_encryption_mode != FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED)
    {
        object_entry_ptr = table_ptr->table_content.object_entry_ptr;
        for(index = 0; index < table_ptr->table_size; index++)
        {
            if (object_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID)
            {
                object_entry_ptr->base_config.encryption_mode = FBE_BASE_CONFIG_ENCRYPTION_MODE_INVALID;
            }
            object_entry_ptr++;
        }
    }
    if (system_encryption_mode == FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED){
        /* Notify raid library that the system is encrypted.
         */
        fbe_raid_library_set_encrypted();
    }
    status = database_common_create_object_from_table(table_ptr, 
        FBE_RESERVED_OBJECT_IDS, 
        (table_ptr->table_size - FBE_RESERVED_OBJECT_IDS));
    if (status != FBE_STATUS_OK){
        return status;
    } 

    status = database_common_set_config_from_table(table_ptr, 
        FBE_RESERVED_OBJECT_IDS, 
        (table_ptr->table_size - FBE_RESERVED_OBJECT_IDS));

    if (status != FBE_STATUS_OK){
        return status;
    }

    status = database_common_connect_pvd_from_table(table_ptr, 
        FBE_RESERVED_OBJECT_IDS, 
        (table_ptr->table_size - FBE_RESERVED_OBJECT_IDS)); 
    if (status != FBE_STATUS_OK){
        return status;
    }

    /* create the edges */
    status = fbe_database_get_service_table_ptr(&table_ptr, DATABASE_CONFIG_TABLE_EDGE);
    if ((status != FBE_STATUS_OK)||(table_ptr == NULL)) {
        return status;
    }
    status = database_common_create_edge_from_table(table_ptr, 
        FBE_RESERVED_OBJECT_IDS*DATABASE_MAX_EDGE_PER_OBJECT, 
        (table_ptr->table_size - FBE_RESERVED_OBJECT_IDS*DATABASE_MAX_EDGE_PER_OBJECT));
    if (status != FBE_STATUS_OK){
        return status;
    }

    status = fbe_database_get_service_table_ptr(&table_ptr, DATABASE_CONFIG_TABLE_OBJECT);
    if ((status != FBE_STATUS_OK)||(table_ptr == NULL)) {
        return status;
    }

    status = database_common_commit_object_from_table(table_ptr, 
        FBE_RESERVED_OBJECT_IDS, 
        (table_ptr->table_size - FBE_RESERVED_OBJECT_IDS),
        FBE_FALSE);
    if (status != FBE_STATUS_OK){
        return status;
    } 
   
    return status;

}


/*!***************************************************************
* @fn fbe_database_wait_for_ready_system_drives
*****************************************************************
* @brief
*   This function sets the system power saving information in the
*   base_config.
*
* @param  wait_for_all- do we want to wait for all or at least one
*
* @return FBE_STATUS_GENERIC_FAILURE if any issues.

*
****************************************************************/
fbe_status_t fbe_database_wait_for_ready_system_drives(fbe_bool_t wait_for_all)
{
    fbe_bool_t                                    *drive_ready = NULL; 
    fbe_u32_t                                    drive_count;
    fbe_base_object_mgmt_get_lifecycle_state_t    get_lifecycle;
    fbe_bool_t                                    need_to_retry = FBE_FALSE;
    fbe_status_t                                status;

    /*system PVD object id starts from 1, so the memory size is one more larger than PVD counts*/
    drive_ready = 
        (fbe_bool_t*)fbe_memory_native_allocate((FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_LAST + 1) * sizeof(fbe_bool_t));

    if (drive_ready == NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: memory allocate failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    do {

        need_to_retry = FBE_FALSE;/*rease for each attempt*/

        for (drive_count = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST; drive_count <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_LAST; drive_count ++) {

            status  = fbe_database_send_packet_to_object(FBE_BASE_OBJECT_CONTROL_CODE_GET_LIFECYCLE_STATE,
                &get_lifecycle,
                sizeof(fbe_base_object_mgmt_get_lifecycle_state_t),
                drive_count,
                NULL,
                0,
                FBE_PACKET_FLAG_NO_ATTRIB,
                FBE_PACKAGE_ID_SEP_0);

            if (status != FBE_STATUS_OK) {
                drive_ready[drive_count] = FBE_FALSE;
            }else if (get_lifecycle.lifecycle_state == FBE_LIFECYCLE_STATE_READY) {
                drive_ready[drive_count] = FBE_TRUE;
            }else{
                drive_ready[drive_count] = FBE_FALSE;
            }
        }

        /*search for at least one drive to be at ready state*/
        if (!wait_for_all) {
            for (drive_count = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST; drive_count <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_LAST; drive_count ++) {
                if ((drive_ready[drive_count] == FBE_TRUE)) {
                    fbe_memory_native_release(drive_ready);
                    return FBE_STATUS_OK;/*we are ready*/
                }
            }

            /*if we got here, it means we wait for at least one drive but non are ready*/
            need_to_retry = FBE_TRUE;
        }else{

            /*if we are here, it means we wait for all drives to be ready*/
            for (drive_count = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST; drive_count <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_LAST; drive_count ++) {
                if ((drive_ready[drive_count] == FBE_FALSE)) {
                    need_to_retry = FBE_TRUE;
                    break;
                }
            }
        }

        if (!need_to_retry) {
            break;
        }else{
            fbe_thread_delay(1000);
        }

    }while(need_to_retry);

    /*we are ready with all system drives and ready to go*/
    fbe_memory_native_release(drive_ready);
    return FBE_STATUS_OK;

}

/*let's lern from the PVD what's the object ID of the LOD connected to it since we want to read the RAW lba from the drive */
fbe_status_t fbe_database_get_system_pdo_object_id_by_pvd_id(fbe_object_id_t pvd_object_id,fbe_object_id_t *pdo_object_id)
{
    fbe_block_transport_control_get_edge_info_t block_edge_info;
    fbe_status_t                                status;

    block_edge_info.base_edge_info.client_index = 0;

    status = fbe_database_send_packet_to_object(FBE_BLOCK_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO,
        &block_edge_info, 
        sizeof(fbe_block_transport_control_get_edge_info_t),
        pvd_object_id,
        NULL,  /* no sg list*/
        0,  /* no sg list*/
        FBE_PACKET_FLAG_NO_ATTRIB,
        FBE_PACKAGE_ID_SEP_0);

    if( status != FBE_STATUS_OK ){
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: can't get edge info for PVD:%d\n", __FUNCTION__, pvd_object_id);

        return status;
    }

    *pdo_object_id = block_edge_info.base_edge_info.server_id;

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_database_wait_for_object_state(fbe_object_id_t object_id, fbe_lifecycle_state_t lifecycle_state, fbe_u32_t timeout_ms)
{
    fbe_base_object_mgmt_get_lifecycle_state_t    get_lifecycle;
    fbe_u32_t to = 0;
    fbe_status_t status;

    while(to < timeout_ms || timeout_ms == 0) {    
        status  = fbe_database_send_packet_to_object(FBE_BASE_OBJECT_CONTROL_CODE_GET_LIFECYCLE_STATE,
            &get_lifecycle,
            sizeof(fbe_base_object_mgmt_get_lifecycle_state_t),
            object_id,
            NULL,
            0,
            FBE_PACKET_FLAG_NO_ATTRIB,
            FBE_PACKAGE_ID_SEP_0);

        if (status == FBE_STATUS_OK && get_lifecycle.lifecycle_state == lifecycle_state) {
            return FBE_STATUS_OK;
        }

        fbe_thread_delay(200);
        to += 200;
    }

    return FBE_STATUS_TIMEOUT;
}

fbe_status_t fbe_database_system_create_topology(fbe_database_service_t *fbe_database_service)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_bool_t        active_side = database_common_cmi_is_active();
    fbe_database_state_t db_state;

    /* currently the configuration has been set by the caller, so no need to set again here.
    if (active_side) {

        // Set the config for PVDs using the info we read from persisted tables to overwrite the default values.
        status = database_common_set_config_from_table(&fbe_database_service->object_table, 
                                                          FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST, 
                                                          (FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_LAST-FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST+1));
        if (status != FBE_STATUS_OK){
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_INFO, 
                "%s: failed to set config for system drives. status: 0x%x\n", __FUNCTION__, status);
            return status;
        }

        status = database_common_set_config_from_table(&fbe_database_service->object_table, 
                                                       FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_SPARE_FIRST, 
                                                       (FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_SPARE_LAST-FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_SPARE_FIRST+1));
                                                       
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: set config to spare pvds failed. Status 0x%x \n",
                           __FUNCTION__,
                           status);
            return status;
        }
    }    
    */

    /* create sep internal objects */
    status = database_system_objects_create_sep_internal_objects_from_persist(fbe_database_service);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_WARNING,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: failed to create sep internal objects. status = 0x%x\n",
            __FUNCTION__, status);
        fbe_event_log_write(SEP_ERROR_CODE_SYSTEM_DB_CONTENT_CORRUPTED,
                            NULL, 0, NULL);
        fbe_database_enter_service_mode(FBE_DATABASE_SERVICE_MODE_REASON_INTERNAL_OBJS);
        return FBE_STATUS_SERVICE_MODE;
    }

    /* set metadata lun and retry getting the DB state for up to 4 minutes for it to finish loading */
    status = fbe_database_wait_for_object_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAID_METADATA, FBE_LIFECYCLE_STATE_READY, DATABASE_WAIT_FOR_OBJECT_STATE_TIME);    

    if (status  == FBE_STATUS_TIMEOUT)
    {
        get_database_service_state(fbe_database_service, &db_state);

        database_trace(FBE_TRACE_LEVEL_WARNING,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
            "%s: Metadata LUN NOT RDY! Set degraded mode. DBState=0x%x.\n", 
            __FUNCTION__, db_state);

        fbe_event_log_write(SEP_ERROR_CODE_SYSTEM_DB_CONTENT_CORRUPTED,
                                    NULL, 0, NULL);
        fbe_database_enter_service_mode(FBE_DATABASE_SERVICE_MODE_REASON_INTERNAL_OBJS);

        return FBE_STATUS_SERVICE_MODE;
    }

    /* database metadata nopaged load */
    if (active_side) {
        status = fbe_database_metadata_nonpaged_load();
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: failed to load nonpaged metadata. status = 0x%x\n",
                __FUNCTION__, status);
            return status;
        }
    }

    /* create export system object */
    status = database_system_objects_create_export_objects_from_persist(fbe_database_service);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: failed to create export objects. status = 0x%x\n",
            __FUNCTION__, status);
        return status;
    }
    /* wait for db lun before setting up persist service */
    status = fbe_database_wait_for_object_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MCR_DATABASE,  FBE_LIFECYCLE_STATE_READY, DATABASE_WAIT_FOR_OBJECT_STATE_TIME);
    if (status == FBE_STATUS_TIMEOUT) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
            "%s: DB Lun not ready in 240 sec!\n", 
            __FUNCTION__);
        fbe_event_log_write(SEP_ERROR_CODE_SYSTEM_DB_CONTENT_CORRUPTED,
                                    NULL, 0, NULL);
        fbe_database_enter_service_mode(FBE_DATABASE_SERVICE_MODE_REASON_INTERNAL_OBJS);

        return FBE_STATUS_SERVICE_MODE;
    } else if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
            "%s: Failed to wait for DB Lun ready!\n", 
            __FUNCTION__);
        return status;
    }

    database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_INFO, 
                   "%s: end of the entry\n", __FUNCTION__); 

    return status;
}


/* fbe_database_system_entry_utils.c */

/*!**************************************************************************************************
@fn fbe_database_system_object_entries_load_from_persist()
*****************************************************************************************************

* @brief
*  This function reads system object entries from raw 3way mirror disk and polulates the service tables. 
*
* @return
*  fbe_status_t
* @version
* 5/5/2012: Modified by Hongpo Gao (changed to 4 blocks per entry)
*
*****************************************************************************************************/

fbe_status_t fbe_database_system_entries_load_from_persist(void)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t            *read_buffer, *p;
    fbe_object_id_t     object_id;
    fbe_lba_t           lba;
    fbe_u64_t           read_count = 0;
    fbe_u32_t           system_object_count = 0;
    fbe_u32_t           index, j;
    fbe_u32_t           valid_db_drives = 0;
    fbe_u32_t           system_db_entry_data_size = 0;

    database_system_objects_get_reserved_count(&system_object_count);

    /* Allocate a buffer which size can get FBE_DATABASE_SYSTEM_EDGE_ENTRIES_NUM entries in one read */
    read_buffer = (fbe_u8_t *)fbe_memory_ex_allocate(system_object_count * DATABASE_MAX_EDGE_PER_OBJECT * DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE);
    if (read_buffer == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Can't allocate read buffer\n",
            __FUNCTION__);
        return status;
    }

    system_db_entry_data_size = DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE * DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY; 

    
    object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FIRST;
    /* First, load the system object entries and user enties */
    status = database_system_db_get_entry_lba(FBE_DATABASE_SYSTEM_DB_OBJECT_ENTRY, object_id, 0, &lba);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: failed to calculate lba, status = 0x%x \n",
            __FUNCTION__,
            status);
        fbe_memory_ex_release(read_buffer);
        return status;
    }

    status = fbe_database_raw_mirror_get_valid_drives(&valid_db_drives);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_INFO, 
                "Database service failed to get valid raw-mirror drives, instead load from all drives \n");
        valid_db_drives = DATABASE_SYSTEM_RAW_MIRROR_ALL_DB_DRIVE_MASK;
    }

    /* Load system object entries and system user entries in one read*/
    fbe_zero_memory(read_buffer, system_object_count * DATABASE_MAX_EDGE_PER_OBJECT * DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE);
    status = database_system_db_interface_read(read_buffer,
        lba,
        system_object_count * 2 * system_db_entry_data_size,
        valid_db_drives,
        &read_count);
    /* If load system object with big read failed, then load the entry one by one.  */
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: fail to load system object and user entries, status = 0x%x\n",
            __FUNCTION__,
            status);
        database_trace(FBE_TRACE_LEVEL_INFO,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: try to load system object and user entry one by one\n",
            __FUNCTION__);
        /* read the entry one by one */
        p = read_buffer;
        for (j = 0; j < system_object_count * 2; j++) {
            /* Don't check the status, if we can't read out data, the the read_buffer will be zero data , 
              *  If the db entry is not read out, the system will enter service mode when create the object or edge because of uncorrect db entry.
              */
            status = database_system_db_interface_read(p + j * system_db_entry_data_size,
                                        lba + j * DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY,
                                        system_db_entry_data_size,
                                        valid_db_drives,
                                        &read_count);            
        }                   

    }
    /* Copy the system object entries */
    status = database_system_copy_entries_from_persist_read(read_buffer,
        system_object_count,
        DATABASE_CONFIG_TABLE_OBJECT); 
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: copy system object enties failed, status = 0x%x\n",
            __FUNCTION__,
            status);
        fbe_memory_ex_release(read_buffer);
        return status;
    }
    /* Copy the user entries */
    status = database_system_copy_entries_from_persist_read(read_buffer + system_object_count*DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE*DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY,
        system_object_count,
        DATABASE_CONFIG_TABLE_USER); 
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: copy system user enties from buffer failed, status = 0x%x\n",
            __FUNCTION__,
            status);
        fbe_memory_ex_release(read_buffer);
        return status;
    }

    /* Load system edge entries */
    fbe_zero_memory(read_buffer, system_object_count * DATABASE_MAX_EDGE_PER_OBJECT * DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE);
    read_count = 0;
    status = database_system_db_get_entry_lba(FBE_DATABASE_SYSTEM_DB_EDGE_ENTRY, object_id, 0, &lba);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: failed to calculate lba, status = 0x%x \n",
            __FUNCTION__,
            status);
        fbe_memory_ex_release(read_buffer);
        return status;
    }

    /* Read edge entry with 4 loops because it is too large to get in one read*/
    for (index = 0; index < DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY; index++) {
        status = database_system_db_interface_read(read_buffer,
                    lba + index * system_object_count * DATABASE_MAX_EDGE_PER_OBJECT,
                    system_object_count * DATABASE_MAX_EDGE_PER_OBJECT * DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE,
                    valid_db_drives,
                    &read_count);
        /*If the big read failed, read the edge entry one by one */
        if (status != FBE_STATUS_OK){
            database_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: database_system_db_interface_read failed, status = 0x%x\n",
                __FUNCTION__,
                status);
            database_trace(FBE_TRACE_LEVEL_INFO,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: try to load edge entry one by one\n",
                __FUNCTION__);
            /*If the big read failed, read the edge entry one by one */
            p = read_buffer;
            for (j = 0; j < (system_object_count * DATABASE_MAX_EDGE_PER_OBJECT) / DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY; j++) {
                /* Don't check the status, if we can't read out data, the the read_buffer will be zero data ,
                            *  If the db entry is not read out, the system will enter service mode when create the object or edge because of uncorrect db entry.
                            */
                status = database_system_db_interface_read(p + j * system_db_entry_data_size,
                            lba + index * (system_object_count * DATABASE_MAX_EDGE_PER_OBJECT) + j * DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY,
                            system_db_entry_data_size,
                            valid_db_drives,
                            &read_count);
            }
        }
    
        /* copy the edge entries */
        status = database_system_copy_entries_from_persist_read(read_buffer,
                (system_object_count * DATABASE_MAX_EDGE_PER_OBJECT)/DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY,
                DATABASE_CONFIG_TABLE_EDGE); 
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: copy system edge entries from buffer failed, status = 0x%x\n",
                __FUNCTION__,
                status);
            fbe_memory_ex_release(read_buffer);
            return status;
        }
    }

    fbe_memory_ex_release(read_buffer);
    return FBE_STATUS_OK;
}

/*****************************************************************
 * @brief 
 *      copy the entry from read buffer to database table
 *  
 * @param read_buffer : the pointer to the read buffer
 * @param count :       the block number
 * @param table_type :  the type of the table
 * 
 * @return fbe_status_t : if success, return FBE_STATUS_OK
 * 
 * @version 
 *    modified by gaoh1 (5/5/2012)
 *****************************************************************/

static fbe_status_t
database_system_copy_entries_from_persist_read(fbe_u8_t *read_buffer,
                                               fbe_u32_t count,
                                               database_config_table_type_t table_type)
{
    fbe_u8_t *          current_data = NULL;
    database_table_t *  table_ptr = NULL;
    fbe_u32_t           i;
    fbe_status_t        status;
    database_object_entry_t *   object_entry_ptr = NULL;
    database_edge_entry_t *     edge_entry_ptr = NULL;
    database_user_entry_t *     user_entry_ptr = NULL;
        
    if (read_buffer == NULL){
        database_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Null pointer\n",
            __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    current_data = read_buffer;
    status = fbe_database_get_service_table_ptr(&table_ptr, table_type);
    if ((status != FBE_STATUS_OK) || table_ptr == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: fbe_database_get_service_table_ptr failed, status = 0x%x\n",
            __FUNCTION__, status);
        return status;
    }

    for (i = 0; i < count; i++) {
        // entry_header_ptr = (database_table_header_t *)current_data;
        /* Skip the system PVD, can not update the system PVD object/user/edge entries into DB table.  
          * Because, the system drives maybe removed or replaced by other drives before array
          * booting. So the system PVD configurations in DB table can not be overwritten by system
          * PVD configurations loaded from disks. 
          */
        /* if (object_entry_ptr->header.object_id >= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST &&
              object_entry_ptr->header.object_id <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_LAST)
        {
            continue;   
        }*/
         
        switch (table_type) {
            case DATABASE_CONFIG_TABLE_OBJECT:
                object_entry_ptr = (database_object_entry_t *)current_data;
                /* check if the entry load from persist is valid */
                if(database_system_object_entry_valid((database_table_header_t *)object_entry_ptr)) {
                    status = fbe_database_config_table_update_object_entry(table_ptr, object_entry_ptr);
                }
                break;
            case DATABASE_CONFIG_TABLE_USER:
                user_entry_ptr = (database_user_entry_t *)current_data;
                /* check if the entry load from persist is valid */
                if (database_system_object_entry_valid((database_table_header_t *)user_entry_ptr)) {
                    status = fbe_database_config_table_update_user_entry(table_ptr, user_entry_ptr);
                }
                break;
            case DATABASE_CONFIG_TABLE_EDGE:
                edge_entry_ptr = (database_edge_entry_t *)current_data;
                /* check if the entry load from persist is valid */
                if (database_system_object_entry_valid((database_table_header_t *)edge_entry_ptr)) {
                    status = fbe_database_config_table_update_edge_entry(table_ptr, edge_entry_ptr);
                }
                break;
            default:
                database_trace(FBE_TRACE_LEVEL_WARNING,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: Table type:%d is not supported!\n",
                    __FUNCTION__,
                    table_type);
                status = FBE_STATUS_GENERIC_FAILURE;
                break;
        }
        if(status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_WARNING,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: failed to update system entry. table type: %d, status 0x%x\n",
                __FUNCTION__,
                table_type,
                status);
            return status;
        }
        /* goto next entry. use 4 blocks to save one entry*/
        current_data += DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE * DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY;
    }
    return FBE_STATUS_OK;
}

fbe_status_t fbe_database_system_db_persist_entries(fbe_database_service_t *database_service_ptr)
{
    fbe_u8_t *          write_buffer;
    fbe_object_id_t     object_id;
    fbe_lba_t           lba;
    fbe_u64_t           write_count = 0;
    fbe_u32_t           system_object_count = 0;
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    database_system_db_header_t system_db_header;

    if (database_service_ptr == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: function argument is NULL\n",
                       __FUNCTION__);
        return status;
    }

    database_system_objects_get_reserved_count(&system_object_count);

    write_buffer = (fbe_u8_t *)fbe_memory_ex_allocate(system_object_count * DATABASE_MAX_EDGE_PER_OBJECT * DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE * DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY);
    if (write_buffer == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Can't allocate write buffer\n",
            __FUNCTION__);
        return status;
    }

    
    /* This is the first system object id */
    object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FIRST;
    /* First, persist the system object entries and user enties */
    /* Copy the system object entries to buffer */
    status = database_system_copy_entries_to_write_buffer(write_buffer,
        object_id,
        system_object_count,
        DATABASE_CONFIG_TABLE_OBJECT); 
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: copy system object enties to buffer failed, status = 0x%x\n",
            __FUNCTION__,
            status);
        fbe_memory_ex_release(write_buffer);
        return status;
    }
    /* Copy the user entries */
    status = database_system_copy_entries_to_write_buffer(write_buffer + system_object_count * DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE * DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY,
        object_id,
        system_object_count,
        DATABASE_CONFIG_TABLE_USER); 
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: copy system user enties to buffer failed, status = 0x%x\n",
            __FUNCTION__,
            status);
        fbe_memory_ex_release(write_buffer);
        return status;
    }
    status = database_system_db_get_entry_lba(FBE_DATABASE_SYSTEM_DB_OBJECT_ENTRY, object_id, 0, &lba);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: fail to get entry's lba, status = 0x%x\n",
            __FUNCTION__,
            status);
        fbe_memory_ex_release(write_buffer);
        return status;
    }
    /* write system object entries and system user entries */
    status = database_system_db_interface_write(write_buffer,
        lba,
        system_object_count * 2 * DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE * DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY,
        &write_count);
    if (status != FBE_STATUS_OK || (write_count != system_object_count * 2 * DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE*DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY)) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: database_system_db_interface_write failed, status = 0x%x, write_count = 0x%llx\n",
            __FUNCTION__,
            status,
            (unsigned long long)write_count);
        fbe_memory_ex_release(write_buffer);
        /* make sure it returns unsuccessfully */
        if (status == FBE_STATUS_OK) {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        return status;
    }

    /* persist system edge entries */
    fbe_zero_memory(write_buffer, system_object_count * DATABASE_MAX_EDGE_PER_OBJECT * DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE);
    write_count = 0;
    /* copy the edge entries */
    status = database_system_copy_entries_to_write_buffer(write_buffer,
        object_id,
        system_object_count,
        DATABASE_CONFIG_TABLE_EDGE); 
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: copy system edge entries to buffer failed, status = 0x%x\n",
            __FUNCTION__,
            status);
        fbe_memory_ex_release(write_buffer);
        return status;
    }

    status = database_system_db_get_entry_lba(FBE_DATABASE_SYSTEM_DB_EDGE_ENTRY, object_id, 0, &lba);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: fail to get entry's lba, status = 0x%x\n",
            __FUNCTION__,
            status);
        fbe_memory_ex_release(write_buffer);
        return status;
    }
    status = database_system_db_interface_write(write_buffer,
        lba,
        system_object_count * DATABASE_MAX_EDGE_PER_OBJECT * DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE * DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY,
        &write_count);
    if (status != FBE_STATUS_OK || (write_count != system_object_count * DATABASE_MAX_EDGE_PER_OBJECT * DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE * DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY)) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: database_system_db_interface_write failed, status = 0x%x, write_count = 0x%llx\n",
            __FUNCTION__,
            status,
            (unsigned long long)write_count);
        fbe_memory_ex_release(write_buffer);
        /* make sure it returns unsuccessfully */
        if (status == FBE_STATUS_OK) {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        return status;
    }

    /*before persist valid system db header, persist shared emv info first*/
    status = fbe_database_system_db_persist_semv_info(&database_service_ptr->shared_expected_memory_info);
    if(FBE_STATUS_OK != status)
    {
        fbe_memory_ex_release(write_buffer);
        database_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: persist shared emv info failed\n",
            __FUNCTION__);
        return status;
    }

        fbe_spinlock_lock(&database_service_ptr->system_db_header_lock);
        fbe_copy_memory(&system_db_header, &database_service_ptr->system_db_header, sizeof(database_system_db_header_t));
        fbe_spinlock_unlock(&database_service_ptr->system_db_header_lock);
        status = fbe_database_system_db_persist_header(&system_db_header);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: database_system_db_persist_header failed, status = 0x%x\n",
                __FUNCTION__,
                status);
            fbe_memory_ex_release(write_buffer);
            return status;
        }
   
    fbe_memory_ex_release(write_buffer);

    return status;
}

static fbe_status_t database_system_copy_entries_to_write_buffer(fbe_u8_t *write_buffer,
                                                                 fbe_u32_t start_index,
                                                                 fbe_u32_t count,
                                                                 database_config_table_type_t table_type)
{
    fbe_u8_t *          current_data = NULL;
    database_table_t *  table_ptr = NULL;
    fbe_u32_t           i, j;
    fbe_status_t        status;
    database_object_entry_t *   object_entry_ptr = NULL;
    database_edge_entry_t *     edge_entry_ptr = NULL;
    database_user_entry_t *     user_entry_ptr = NULL;
    fbe_u32_t                   system_object_count = 0;

    database_system_objects_get_reserved_count(&system_object_count);
    if (write_buffer == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_database_get_service_table_ptr(&table_ptr, table_type);
    if ((status != FBE_STATUS_OK) || table_ptr == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: get_service_table_ptr failed, status = 0x%x\n",
            __FUNCTION__, status);
        return status;
    }

    /* copy the database table entries to buffer */
    current_data = write_buffer;
    fbe_spinlock_lock(&table_ptr->table_lock);
    switch (table_type) {
    case DATABASE_CONFIG_TABLE_OBJECT:
        for (i = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FIRST; i < system_object_count; i++) {
            status = fbe_database_config_table_get_object_entry_by_id(table_ptr,
                i,
                &object_entry_ptr);
            if (status != FBE_STATUS_OK) {
                database_trace(FBE_TRACE_LEVEL_ERROR,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: failed to get object entry, status = 0x%x\n",
                    __FUNCTION__, status);
                fbe_spinlock_unlock(&table_ptr->table_lock);
                return status;
            }

            if(object_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID) {
                fbe_copy_memory(current_data, object_entry_ptr, sizeof(database_object_entry_t));
            }
            /* goto next entry */
            current_data += DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE * DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY;
        }
        break;
    case DATABASE_CONFIG_TABLE_USER:
        for (i = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FIRST; i < system_object_count; i++) {

            status = fbe_database_config_table_get_user_entry_by_object_id(table_ptr,
                i,
                &user_entry_ptr);
            if (status != FBE_STATUS_OK) {
                database_trace(FBE_TRACE_LEVEL_ERROR,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: failed to get user entry, status = 0x%x\n",
                    __FUNCTION__, status);
                fbe_spinlock_unlock(&table_ptr->table_lock);
                return status;
            }

            if (user_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID) {
                fbe_copy_memory(current_data, user_entry_ptr, sizeof(database_user_entry_t));
            }
            /* goto next entry */
            current_data += DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE * DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY;
        }
        break;
    case DATABASE_CONFIG_TABLE_EDGE:
        for (i = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FIRST; i < system_object_count; i++) {

            for (j = 0; j < DATABASE_MAX_EDGE_PER_OBJECT; j++ ) {
                status = fbe_database_config_table_get_edge_entry(table_ptr,
                    i,
                    j,
                    &edge_entry_ptr);
                if (status != FBE_STATUS_OK) {
                    database_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: failed to get edge entry, status = 0x%x\n",
                        __FUNCTION__, status);
                    fbe_spinlock_unlock(&table_ptr->table_lock);
                    return status;
                }

                if (edge_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID) {
                    fbe_copy_memory(current_data, edge_entry_ptr, sizeof(database_edge_entry_t));
                }
                /* goto next entry */
                current_data += DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE * DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY;
            }
        }
        break;
    default:
        database_trace(FBE_TRACE_LEVEL_WARNING,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Table type:%d is not supported!\n",
            __FUNCTION__,
            table_type);
        status = FBE_STATUS_GENERIC_FAILURE;
        break;
    }
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_WARNING,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Failed to update write buffer with system entries. table type: %d, status 0x%x\n",
            __FUNCTION__,
            table_type,
            status);
        fbe_spinlock_unlock(&table_ptr->table_lock);
        return status;
    }
    fbe_spinlock_unlock(&table_ptr->table_lock);

    return status;
}

/* read and check the database system db header.
 * If it is valid, 
 *    copy the database system db header to database_service_ptr->system_db_header
 *    return true;
 * else 
 *    return false; 
 */
fbe_status_t fbe_database_system_db_interface_read_and_check_system_db_header(fbe_database_service_t *database_service_ptr, 
                                                                            fbe_u32_t valid_db_drive_mask)
{
    fbe_status_t    status;
    database_system_db_header_t* system_db_header = NULL;

    system_db_header = (database_system_db_header_t*)fbe_transport_allocate_buffer(); /*8 blocks are enough for us*/
    if(NULL == system_db_header)
        return FBE_STATUS_GENERIC_FAILURE;

    fbe_zero_memory(system_db_header, sizeof(database_system_db_header_t));
    status = fbe_database_system_db_read_header(system_db_header, valid_db_drive_mask);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Failed to read system db header from persist, status 0x%x\n",
            __FUNCTION__,
            status);
        fbe_transport_release_buffer((void*)system_db_header);
        fbe_event_log_write(SEP_ERROR_CODE_SYSTEM_DB_HEADER_RW_ERROR,
                             NULL, 0, NULL);						
        fbe_database_enter_service_mode(FBE_DATABASE_SERVICE_MODE_REASON_SYSTEM_DB_HEADER_IO_ERROR);
        status = FBE_STATUS_SERVICE_MODE;
        return status;
    }

    if (system_db_header->magic_number == DATABASE_SYSTEM_DB_HEADER_MAGIC_NUMBER) {
        
        if(system_db_header->version_header.size > sizeof(database_system_db_header_t))
        {
            /* the disk version size should never be larger than the software version size */
            database_trace(FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: the disk version size(0x%x) is larger than software version size(0x%x)\n",
                                    __FUNCTION__,
                                    system_db_header->version_header.size,
                                    (unsigned int)sizeof(database_system_db_header_t));
            fbe_transport_release_buffer((void*)system_db_header);
			fbe_event_log_write(SEP_ERROR_CODE_NEWER_CONFIG_FROM_PEER,
								NULL, 0, "%x %x",
								system_db_header->version_header.size,
								(int)sizeof(database_system_db_header_t));
			fbe_database_enter_service_mode(FBE_DATABASE_SERVICE_MODE_REASON_SYSTEM_DB_HEADER_TOO_LARGE);
            return FBE_STATUS_SERVICE_MODE;
        }

        fbe_spinlock_lock(&database_service_ptr->system_db_header_lock);
        fbe_zero_memory(&database_service_ptr->system_db_header, sizeof(database_system_db_header_t));        
        fbe_copy_memory(&database_service_ptr->system_db_header, 
                                    system_db_header, system_db_header->version_header.size);
        fbe_spinlock_unlock(&database_service_ptr->system_db_header_lock);

        fbe_transport_release_buffer((void*)system_db_header);        
        return FBE_STATUS_OK;
    } else {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: the magic number(0x%x)of system db header from disk is mismatched\n",
                        __FUNCTION__, (unsigned int)system_db_header->magic_number);
        fbe_transport_release_buffer((void*)system_db_header);
        fbe_event_log_write(SEP_ERROR_CODE_SYSTEM_DB_HEADER_CORRUPTED,
                            NULL, 0, NULL);
        fbe_database_enter_service_mode(FBE_DATABASE_SERVICE_MODE_REASON_SYSTEM_DB_HEADER_DATA_CORRUPT);
        return FBE_STATUS_SERVICE_MODE;
    }

}

fbe_status_t database_copy_single_entry_from_persist_read(fbe_u8_t * read_buffer, database_config_table_type_t table_type)
{
    fbe_status_t						status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t * 							current_data = read_buffer;
    database_table_header_t *			database_table_header = NULL;
    database_table_t * 					table_ptr = NULL;
    database_object_entry_t * 			object_entry_ptr = NULL;
    database_edge_entry_t * 			edge_entry_ptr = NULL;
    database_user_entry_t * 			user_entry_ptr = NULL;
    database_global_info_entry_t * 		global_info_entry_ptr = NULL;
    database_system_spare_entry_t * 	system_spare_entry_ptr = NULL;


    status = fbe_database_get_service_table_ptr(&table_ptr, table_type);
    if ((status != FBE_STATUS_OK)||(table_ptr == NULL)) {
        return status;
    }

    database_table_header = (database_table_header_t *)current_data;

    if (database_table_header->state != DATABASE_CONFIG_ENTRY_VALID) {
        /* we expect a valid entry */
        database_trace (FBE_TRACE_LEVEL_DEBUG_HIGH,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Entry id 0x%llx has unexpected state 0x%x. Skip it!\n",
                        __FUNCTION__,
                        (unsigned long long)database_table_header->entry_id,
                        database_table_header->state);
        return FBE_STATUS_OK;
    }

    switch(table_type) {
        case DATABASE_CONFIG_TABLE_OBJECT:
            object_entry_ptr = (database_object_entry_t *)current_data;
            status = fbe_database_config_table_update_object_entry(table_ptr, object_entry_ptr);
            break;
        case DATABASE_CONFIG_TABLE_EDGE:
            edge_entry_ptr = (database_edge_entry_t *)current_data;
            status = fbe_database_config_table_update_edge_entry(table_ptr, edge_entry_ptr);
            break;
        case DATABASE_CONFIG_TABLE_USER:
            user_entry_ptr = (database_user_entry_t *)current_data;
            status = fbe_database_config_table_update_user_entry(table_ptr, user_entry_ptr);
            break;
        case DATABASE_CONFIG_TABLE_GLOBAL_INFO:
            global_info_entry_ptr = (database_global_info_entry_t *)current_data;
            status = fbe_database_config_table_update_global_info_entry(table_ptr, global_info_entry_ptr);
            break;
        case DATABASE_CONFIG_TABLE_SYSTEM_SPARE:
            system_spare_entry_ptr = (database_system_spare_entry_t *)current_data;
            status = fbe_database_config_table_update_system_spare_entry(table_ptr, system_spare_entry_ptr);
            break;

        default:
            database_trace (FBE_TRACE_LEVEL_WARNING,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: Table type:%d is not supported!\n",
                            __FUNCTION__,
                            table_type);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
        }
    if (status != FBE_STATUS_OK) {
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Failed to update Entry id 0x%llx of table type:%d, status 0x%x!\n",
                        __FUNCTION__,
                        (unsigned long long)database_table_header->entry_id,
                        table_type,
                        status);
        return status;
    }
        
   
    return FBE_STATUS_OK;
}


/*!***************************************************************
 * @fn fbe_database_generic_wait_for_object_state
 ****************************************************************
 * @brief
 *    Get an object's lifecycle state
 *
 * @param[in] object_id     - object ID
 * @param[out] lifecycle_state - the lifecycle state of the object
 * @param[in] package_id - the package where the object lies in
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if the lifecycle state is obtained.
 *
 * @version
 *  11/25/2011 - Created. Zhipeng Hu.
 *
 ****************************************************************/
fbe_status_t fbe_database_generic_get_object_state(fbe_object_id_t object_id, 
                                                               fbe_lifecycle_state_t* lifecycle_state, 
                                                               fbe_package_id_t package_id)
{
    fbe_base_object_mgmt_get_lifecycle_state_t    get_lifecycle;
    fbe_status_t status;
    fbe_packet_attr_t packet_attr = (package_id == FBE_PACKAGE_ID_SEP_0?FBE_PACKET_FLAG_NO_ATTRIB:FBE_PACKET_FLAG_EXTERNAL);

    if(NULL == lifecycle_state)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_database_send_packet_to_object(FBE_BASE_OBJECT_CONTROL_CODE_GET_LIFECYCLE_STATE,
            &get_lifecycle,
            sizeof(fbe_base_object_mgmt_get_lifecycle_state_t),
            object_id,
            NULL,
            0,
            packet_attr,
            package_id);

    if(FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Failed to get object state, status 0x%x\n",
            __FUNCTION__,
            status);
        return status;    
    }

    *lifecycle_state    =   get_lifecycle.lifecycle_state;

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_read_flags_from_raw_drive
 ****************************************************************
 * @brief
 *  this function read flags from the private layout space on the specified raw drives
 *  the size of flag should be no more than 1 blk. If this condition can not be satisfied,
 *  pls use more generic function "fbe_database_read_data_from_single_raw_drive"
 *
 * @param[in] drive_locations     - array to indicate which drives do we want to perform 
 *                     read. the "logical_drive_object_id" member is a returned value.
 * @param[in] number_of_drives_for_operation - how many drives do we want to read from.
 *                   This is the size of the drive_locations array
 * @param[in] pls_id - the private space layout region id, which specify the read is performed
 *                    on which region of the drives
 * @param[in] validation_function - a pointer pointing to the function which verifies and
 *                   stores the read flags
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no errror.
 *
 * @version
 *  11/24/2011 - Created. zphu.
 *
 ****************************************************************/
fbe_status_t fbe_database_read_flags_from_raw_drive(database_physical_drive_info_t* drive_locations,
                                                    fbe_u32_t number_of_drives_for_operation,
                                                    fbe_private_space_layout_region_id_t  pls_id,
                                                    fbe_database_validate_read_flags_funcion validation_function,
                                                    fbe_bool_t force_validation)
{
    fbe_memory_request_t     memory_request;/*we are doing everything sync, no need to allocate from heap*/
    fbe_semaphore_t            semaphore;
    fbe_status_t                   status;
    fbe_packet_t **            packet_array = NULL;
    fbe_sg_element_t **        sg_list_array = NULL;/*each sub needs an sg list*/
    fbe_u8_t    *            flags_array = NULL, * tmp_flags_aray = NULL;
    fbe_u32_t                count = 0;
    fbe_u32_t                     complete_packet_cnt = 0;
    fbe_u32_t                         number_of_packets_for_operation = number_of_drives_for_operation + 1;
    

    /*some sanity check*/
    if((FBE_MEMORY_CHUNK_SIZE_FOR_PACKET * number_of_packets_for_operation) < 
        (sizeof(fbe_sg_element_t) * number_of_drives_for_operation + 
        sizeof(fbe_packet_t) * number_of_packets_for_operation))
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_read_flags_from_drv: we can not allocate enough resources for read\n" );

        return FBE_STATUS_GENERIC_FAILURE;
    }

    packet_array = fbe_memory_native_allocate(sizeof(fbe_packet_t*) * number_of_packets_for_operation);
    if (!packet_array) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_read_flags_from_drv: failed to allocate memory for packet_array\n" );

        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_zero_memory(packet_array, sizeof(fbe_packet_t*) * number_of_packets_for_operation);

    sg_list_array = fbe_memory_native_allocate(sizeof(fbe_sg_element_t*) * number_of_drives_for_operation);
    if (!sg_list_array) {
        fbe_memory_native_release(packet_array);
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_read_flags_from_drv: failed to allocate memory for sg_list_array\n" );

        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_zero_memory(sg_list_array, sizeof(fbe_sg_element_t*) * number_of_drives_for_operation);


    flags_array = fbe_memory_native_allocate(FBE_BE_BYTES_PER_BLOCK * number_of_drives_for_operation);/*each sub needs a memory to read to*/
    if (!flags_array) {
        fbe_memory_native_release(packet_array);
        fbe_memory_native_release(sg_list_array);
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_read_flags_from_drv: failed to allocate memory for read buffers\n" );

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_semaphore_init(&semaphore, 0, 1);


    fbe_memory_request_init(&memory_request);
    fbe_memory_build_chunk_request(&memory_request, 
        FBE_MEMORY_CHUNK_SIZE_FOR_PACKET,
        number_of_packets_for_operation, /* one master and other sub packets*/
        0, /* Lowest resource priority */
        FBE_MEMORY_REQUEST_IO_STAMP_INVALID,
        (fbe_memory_completion_function_t)fbe_database_read_flags_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
        &semaphore);

    status = fbe_memory_request_entry(&memory_request);

    if ((FBE_STATUS_OK != status) && (FBE_STATUS_PENDING != status)) {
        fbe_semaphore_destroy(&semaphore);
        fbe_memory_native_release(flags_array);
        fbe_memory_native_release(packet_array);
        fbe_memory_native_release(sg_list_array);
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_read_flags_from_drv: failed to get memory\n" );

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*technically this is not very safe for DPC and we should continue procesing on completion.
    However, this is the DB int code which is always called on a passive thread context and not at DPC*/
    status = fbe_semaphore_wait_ms(&semaphore, 30000);
    if (FBE_STATUS_OK != status) {
        fbe_semaphore_destroy(&semaphore);
        fbe_memory_native_release(packet_array);
        fbe_memory_native_release(sg_list_array);
        fbe_memory_native_release(flags_array);
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_read_flags_from_drv: timed out waiting to get memory\n" );

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (FBE_FALSE == fbe_memory_request_is_allocation_complete(&memory_request)){
        /* Currently this callback doesn't expect any aborted requests */
        fbe_semaphore_destroy(&semaphore);
        fbe_memory_native_release(packet_array);
        fbe_memory_native_release(sg_list_array);
        fbe_memory_native_release(flags_array);
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_read_flags_from_drv: memory allocation failed\n" );

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*let's carve the memory and sg lists*/
    fbe_database_carve_memory_for_packet_and_sgl(packet_array, sg_list_array, number_of_drives_for_operation, &memory_request);

    /*point the sg elements to the read buffers*/
    fbe_zero_memory(flags_array, FBE_BE_BYTES_PER_BLOCK * number_of_drives_for_operation);

    tmp_flags_aray = flags_array;/*we need to spin on it so we don't want to use the head*/
    for (count = 0; count < number_of_drives_for_operation; count++) {
        sg_list_array[count]->address = tmp_flags_aray;
        sg_list_array[count]->count = FBE_BE_BYTES_PER_BLOCK;
        tmp_flags_aray += FBE_BE_BYTES_PER_BLOCK;
    }

    /*now let's send the packets and wait for them to return with the final answer
    * we just read one block from the disk as flag always occupy one block on the disk.
    */
    status  = fbe_database_send_read_packets_to_pdos(packet_array, sg_list_array, drive_locations,
        number_of_drives_for_operation, 1, pls_id, &complete_packet_cnt);
    if (FBE_STATUS_OK != status)
    {
        /* we will return failure if no subpacket completed successfully, even we are forced to do validation */
        if ((force_validation != FBE_TRUE) || (complete_packet_cnt == 0))
        {
            //fbe_memory_request_get_entry_mark_free(&memory_request, &memory_ptr);
            //fbe_memory_free_entry(memory_ptr);
            fbe_memory_free_request_entry(&memory_request);
            fbe_semaphore_destroy(&semaphore);
            fbe_memory_native_release(packet_array);
            fbe_memory_native_release(sg_list_array);
            fbe_memory_native_release(flags_array);
            database_trace (FBE_TRACE_LEVEL_WARNING,
                FBE_TRACE_MESSAGE_ID_INFO,
                "db_read_flags_from_drv: send_read_packets_to_pdos failed\n" );

            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* By this time, all the 3 subpackets have completed processing and
    * the semaphore has been released.
    */ 
    /* We are done reading from all drives, let's check the flags. */
    if(NULL != validation_function) 
    {
            status = validation_function(flags_array, (complete_packet_cnt == number_of_drives_for_operation));
    }
    /*free resources*/
    //fbe_memory_request_get_entry_mark_free(&memory_request, &memory_ptr);
    //fbe_memory_free_entry(memory_ptr);
    fbe_memory_free_request_entry(&memory_request);

    fbe_semaphore_destroy(&semaphore);
    fbe_memory_native_release(packet_array);
    fbe_memory_native_release(sg_list_array);
    fbe_memory_native_release(flags_array);

    return status;

}


/*!***************************************************************
 * @fn fbe_database_write_flags_on_raw_drive
 ****************************************************************
 * @brief
 *  this function write flags to the private layout space on the specified raw drives
 *  the size of flag should be no more than 1 blk. If this condition can not be satisfied,
 *  pls use more generic function "fbe_database_write_data_to_single_raw_drive"
 *
 * @param[in] drive_locations     - array to indicate which drives do we want to perform 
 *                     write. the "logical_drive_object_id" member is a returned value.
 * @param[in] number_of_drives_for_operation - how many drives do we want to write to.
 *                   This is the size of the drive_locations array
 * @param[in] pls_id - the private space layout region id, which specify the write is performed
 *                    on which region of the drives
 * @param[in] flag_buffer - the pointer pointing to the flag which we want to write
 * @param[in] flag_length - the size of the flag_buffer
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no errror.
 *
 * @version
 *  11/24/2011 - Created. zphu.
 *
 ****************************************************************/
fbe_status_t fbe_database_write_flags_on_raw_drive(database_physical_drive_info_t* drive_locations,
                                                   fbe_u32_t number_of_drives_for_operation,
                                                   fbe_private_space_layout_region_id_t  pls_id,
                                                   fbe_u8_t*  flag_buffer, fbe_u32_t flag_length)
{
    fbe_memory_request_t     memory_request;/*we are doing everything sync, no need to allocate from heap*/
    fbe_semaphore_t        semaphore;
    fbe_status_t                    status;
    fbe_packet_t **            packet_array = NULL;
    fbe_sg_element_t **        sg_list_array = NULL;/*each sub needs an sg list*/
    fbe_u8_t    *            flags_array = NULL, * tmp_flags_aray = NULL;
    fbe_block_count_t                 count = 0;
    fbe_u32_t                         number_of_packets_for_operation = number_of_drives_for_operation + 1;
    //fbe_memory_ptr_t        memory_ptr;
    fbe_u32_t                 block_count = 0;
    fbe_u32_t                 i;        

    /*some sanity check*/
    if((FBE_MEMORY_CHUNK_SIZE_FOR_PACKET * number_of_packets_for_operation) < 
        (sizeof(fbe_sg_element_t) * number_of_drives_for_operation + 
        sizeof(fbe_packet_t) * number_of_packets_for_operation))
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_write_flags_on_drv: we can not allocate enough resources for writing flags\n" );

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    packet_array = fbe_memory_native_allocate(sizeof(fbe_packet_t*) * number_of_packets_for_operation);
    if (!packet_array) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_write_flags_on_drv: failed to allocate memory for packet_array\n" );

        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_zero_memory(packet_array, sizeof(fbe_packet_t*) * number_of_packets_for_operation);

    sg_list_array = fbe_memory_native_allocate(sizeof(fbe_sg_element_t*) * number_of_drives_for_operation);
    if (!sg_list_array) {
        fbe_memory_native_release(packet_array);
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_write_flags_on_drv: failed to allocate memory for sg_list_array\n" );

        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_zero_memory(sg_list_array, sizeof(fbe_sg_element_t*) * number_of_drives_for_operation);

    for (i = 0; i < number_of_drives_for_operation; i++)
    {
        /* Determine the block count.
        */
        status = fbe_database_determine_block_count_for_io(drive_locations + i,
                                                           flag_length,
                                                           &count);
        if (status != FBE_STATUS_OK) {
            fbe_memory_native_release(packet_array);
            fbe_memory_native_release(sg_list_array);
            database_trace (FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "db_write_data_on_single_drive: failed to get block count\n" );

            return FBE_STATUS_GENERIC_FAILURE;
        }
        /* Set the block count to max value
        */
        block_count = (fbe_u32_t)FBE_MAX(block_count, count);
    }

    flags_array = fbe_memory_native_allocate(FBE_BE_BYTES_PER_BLOCK * number_of_drives_for_operation * block_count);
    if (!flags_array) {
        fbe_memory_native_release(packet_array);
        fbe_memory_native_release(sg_list_array);
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_write_flags_on_drv: failed to allocate memory for read buffers\n" );

        return FBE_STATUS_GENERIC_FAILURE;
    }


    fbe_semaphore_init(&semaphore, 0, 1);

    /*we'll allocate number_of_drives_for_operation packets and send them to the database drives for the write*/
    fbe_memory_request_init(&memory_request);
    fbe_memory_build_chunk_request(&memory_request, 
        FBE_MEMORY_CHUNK_SIZE_FOR_PACKET,
        number_of_packets_for_operation, /* we need number_of_drives_for_operation+1 packets*/
        0, /* Lowest resource priority */
        FBE_MEMORY_REQUEST_IO_STAMP_INVALID,
        (fbe_memory_completion_function_t)fbe_database_write_flags_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
        &semaphore);

    status = fbe_memory_request_entry(&memory_request);

    if ((FBE_STATUS_OK != status) && (FBE_STATUS_PENDING != status)) {
        fbe_semaphore_destroy(&semaphore);
        fbe_memory_native_release(flags_array);
        fbe_memory_native_release(packet_array);
        fbe_memory_native_release(sg_list_array);
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_write_flags_on_drv: failed to get memory\n" );

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*technically this is not very safe for DPC and we should continue procesing on completion.
    However, this is the DB int code which is always called on a passive thread context and not at DPC*/
    status = fbe_semaphore_wait_ms(&semaphore, 30000);
    if (FBE_STATUS_OK != status) {
        fbe_semaphore_destroy(&semaphore);
        fbe_memory_native_release(flags_array);
        fbe_memory_native_release(packet_array);
        fbe_memory_native_release(sg_list_array);
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_write_flags_on_drv: timed out waiting to get memory\n" );

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (FBE_FALSE == fbe_memory_request_is_allocation_complete(&memory_request)){
        /* Currently this callback doesn't expect any aborted requests */
        fbe_semaphore_destroy(&semaphore);
        fbe_memory_native_release(flags_array);
        fbe_memory_native_release(packet_array);
        fbe_memory_native_release(sg_list_array);
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_write_flags_on_drv: memory allocation failed\n" );

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*let's carve the memory and sg lists*/
    fbe_database_carve_memory_for_packet_and_sgl(packet_array, sg_list_array, number_of_drives_for_operation, &memory_request);

    /*point the sg elements to the write buffers*/
    tmp_flags_aray = flags_array;/*we need to spin on it so we don't want to use the head*/
    for (i = 0; i < number_of_drives_for_operation; i++) {
        sg_list_array[i]->address = tmp_flags_aray;
        sg_list_array[i]->count = FBE_BE_BYTES_PER_BLOCK * block_count;

        /*copy data to the sgl*/
        fbe_zero_memory(tmp_flags_aray, FBE_BE_BYTES_PER_BLOCK * block_count);
        status = fbe_database_copy_data_to_write_sg_list(flag_buffer, flag_length, sg_list_array[i]);
        if(FBE_STATUS_OK != status)
        {
            database_trace (FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "db_write_flags_on_drv: copy data to sg list failed!\n" );

            fbe_memory_free_request_entry(&memory_request);

            fbe_semaphore_destroy(&semaphore);
            fbe_memory_native_release(flags_array);
            fbe_memory_native_release(packet_array);
            fbe_memory_native_release(sg_list_array);

            return FBE_STATUS_GENERIC_FAILURE;        
        }
        tmp_flags_aray += FBE_BE_BYTES_PER_BLOCK * block_count;
    }

    /*now let's send the packets and wait for them to return  */
    status = fbe_database_send_write_packets_to_pdos(packet_array, sg_list_array, drive_locations,
        number_of_drives_for_operation, 
        block_count,  /*currently the flag occupies 1 blk at most*/
        pls_id,
        NULL); 

    /*free resources*/
    //fbe_memory_request_get_entry_mark_free(&memory_request, &memory_ptr);
    //fbe_memory_free_entry(memory_ptr);
    fbe_memory_free_request_entry(&memory_request);

    fbe_semaphore_destroy(&semaphore);
    fbe_memory_native_release(flags_array);
    fbe_memory_native_release(packet_array);
    fbe_memory_native_release(sg_list_array);

    return status;

}


static fbe_status_t
fbe_database_read_flags_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context)
{
    fbe_semaphore_release((fbe_semaphore_t *)context, 0, 1, FALSE);
    return FBE_STATUS_OK;

}

static fbe_status_t
fbe_database_write_flags_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context)
{
    fbe_semaphore_release((fbe_semaphore_t *)context, 0, 1, FALSE);
    return FBE_STATUS_OK;

}


/*!***************************************************************
 * @fn fbe_database_carve_memory_for_packet_and_sgl_extend
 ****************************************************************
 * @brief
 *  this function carve memory for packets and sg lists from the requested memory
 * @param[out]  packet_aray - array which stores the packets we want to carve memory for
 * @param[out]  sg_list_array - array which stores the sg lists we want to carve memory for
 * @param[in]  number_of_drives_for_operation - this carve is performed for how many drive operations
 * @param[in]  sg_list_length_for_each_packet - how many sg list elements are needed for each packet
 *                    this is the size of the sg list in a packet
 * @param[in]  memory_request - the memory_request
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no errror.
 *
 * @version
 *  11/24/2011 - Created. zphu.
 *
 ****************************************************************/
static fbe_status_t fbe_database_carve_memory_for_packet_and_sgl_extend(fbe_packet_t **packet_aray,
                                                                        fbe_sg_element_t **sg_list_array,
                                                                        fbe_u32_t number_of_drives_for_operation,
                                                                        fbe_u32_t sg_list_length_for_each_packet,
                                                                        fbe_memory_request_t *memory_request)
{
    fbe_u32_t                    count = 0;
    fbe_u32_t                                    count2 = 0;
    fbe_u32_t                     sg_element_size = 0;
    fbe_memory_header_t *       next_memory_header_p = NULL;
    fbe_memory_header_t *        master_memory_header = NULL;
    fbe_u8_t *                    chunk_start_address = NULL;

    master_memory_header = fbe_memory_get_first_memory_header(memory_request);
    sg_element_size = master_memory_header->memory_chunk_size;

    /*let's start with the master packet in the first chunk*/
    *packet_aray = (fbe_packet_t *)fbe_memory_header_to_data(master_memory_header); /*master packet*/
    fbe_zero_memory(*packet_aray, sizeof(fbe_packet_t));

    packet_aray++;

    while (count < number_of_drives_for_operation) {

        fbe_memory_get_next_memory_header(master_memory_header, &next_memory_header_p);
        master_memory_header = next_memory_header_p; 
        if(NULL == master_memory_header)
        {
            database_trace (FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_INFO,
                "db_carve_mem_ex: not enough memory header allocated.\n" );
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /*put a packet here, yes!*/
        chunk_start_address  = (fbe_u8_t *)fbe_memory_header_to_data(next_memory_header_p);
        *packet_aray = (fbe_packet_t *)chunk_start_address;  /*subpacket*/
        fbe_zero_memory(*packet_aray, sizeof(fbe_packet_t));
        chunk_start_address += sizeof(fbe_packet_t); /*roll it forward so we can carve memory for the sg list*/
        packet_aray++;/*go to next packet*/

        /*now let's take care of the sg list*/
        *sg_list_array = (fbe_sg_element_t *)chunk_start_address;
        fbe_zero_memory(*sg_list_array, sizeof(fbe_sg_element_t) * (sg_list_length_for_each_packet + 1)); 

        for(count2 = 0 ; count2 < sg_list_length_for_each_packet; count2++)
        {
            fbe_memory_get_next_memory_header(master_memory_header, &next_memory_header_p);
            master_memory_header = next_memory_header_p;
            if(NULL == master_memory_header)
            {
                database_trace (FBE_TRACE_LEVEL_ERROR,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "db_carve_mem_ex: not enough memory header allocated.\n" );
                return FBE_STATUS_GENERIC_FAILURE;
            }

            chunk_start_address  = (fbe_u8_t *)fbe_memory_header_to_data(next_memory_header_p);
            (*sg_list_array)[count2].address = chunk_start_address;
            (*sg_list_array)[count2].count  = sg_element_size;
        }

        sg_list_array++;/*go to next sg_list*/

        count++;
    }

    return FBE_STATUS_OK;
}


/*!***************************************************************
 * @fn fbe_database_read_data_from_single_raw_drive
 ****************************************************************
 * @brief
 *  this is a generic function which reads data from a raw drive
 *
 * @param[out] data - the buffer where to store the read data
 * @param[in]   data_length - the buffer length, that is how many bytes of data we want to read
 * @param[in]   drive_location_info - indicate which drive do we want to perform 
 *                     read. the "logical_drive_object_id" member is a returned value.
 * @param[in]   pls_id - the private space layout region where we want to read data from
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no errror.
 *
 * @version
 *  11/26/2011 - Created. zphu.
 *
 ****************************************************************/
fbe_status_t fbe_database_read_data_from_single_raw_drive(fbe_u8_t* data,
                                                          fbe_u64_t  data_length,
                                                          database_physical_drive_info_t* drive_location_info,
                                                          fbe_private_space_layout_region_id_t pls_id)
{
    fbe_memory_request_t        memory_request;
    //fbe_memory_ptr_t                memory_ptr;
    fbe_packet_t *            packet_array[2]; /*one master packet plus 1 subpacket*/
    fbe_sg_element_t *    sg_list = NULL;/*subpacket needs an sg list */
    fbe_block_count_t                block_count;
    fbe_memory_chunk_size_t         chunk_size;
    fbe_memory_number_of_objects_t    chunk_number = 1;
    fbe_semaphore_t          semaphore;
    fbe_status_t                    status;
    fbe_u32_t i;

    /* Determine the block count.
     */
    status = fbe_database_determine_block_count_for_io(drive_location_info,
                                                       data_length,
                                                       &block_count);
    if (status != FBE_STATUS_OK) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "db_read_data_from_single_drive: failed to get block count\n" );
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now determine the chunk size.
     */
    if (block_count < FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_PACKET) {
        chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_PACKET;
    } else {
        chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO;
    }

    /*calculate the number of chunks needed for  data*/
    chunk_number = (fbe_memory_number_of_objects_t)(block_count * FBE_BE_BYTES_PER_BLOCK + chunk_size - 1) /chunk_size;
    chunk_number++;  /* add one chunk for subpacket*/

    fbe_semaphore_init(&semaphore, 0, 1);
    fbe_memory_request_init(&memory_request);
    /* Allocate memory for data */
    fbe_memory_build_chunk_request(&memory_request,
        chunk_size,
        chunk_number + 1,  /*one for master packet*/
        2, /* Lowest resource priority 2 to compete with incoming I/O's */
        FBE_MEMORY_REQUEST_IO_STAMP_INVALID,
        (fbe_memory_completion_function_t)fbe_database_single_raw_drive_io_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
        &semaphore);

    status = fbe_memory_request_entry(&memory_request);

    if ((FBE_STATUS_OK != status) && (FBE_STATUS_PENDING != status)) {
        fbe_semaphore_destroy(&semaphore);
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_read_data_from_single_drive: failed to get memory\n" );
        return FBE_STATUS_GENERIC_FAILURE;
    }


    for(i = 0; i < 6; i++){ /* Wait 1 min for memory */
        status = fbe_semaphore_wait_ms(&semaphore, 10000); /* 10 sec timer */
        if (FBE_STATUS_OK != status) {			
            database_trace (FBE_TRACE_LEVEL_WARNING,
                FBE_TRACE_MESSAGE_ID_INFO,
                "db_read_data_from_single_drive: waiting for memory retry %d\n", i );
        } else {
            break;
        }
    }

    if (FBE_STATUS_OK != status) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_read_data_from_single_drive: Can't get memory in 5 min. \n");		
        fbe_memory_abort_request(&memory_request);

        status = fbe_semaphore_wait_ms(&semaphore, 10000); /* 10 sec timer */
        if (FBE_STATUS_OK != status) {			
            database_trace (FBE_TRACE_LEVEL_CRITICAL_ERROR,
                FBE_TRACE_MESSAGE_ID_INFO,
                "db_read_data_from_single_drive: Failed to abort memory request %p\n", &memory_request );
        }
        fbe_semaphore_destroy(&semaphore);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    if (FBE_FALSE == fbe_memory_request_is_allocation_complete(&memory_request)){
        /* Currently this callback doesn't expect any aborted requests */
        fbe_semaphore_destroy(&semaphore);
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_read_data_from_single_drive: memory allocation failed\n" );
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*carve memory and sgl*/
    status = fbe_database_carve_memory_for_packet_and_sgl_extend(packet_array,
        &sg_list, 1, chunk_number - 1, /*sub the one for subpacket*/&memory_request);
    if(FBE_STATUS_OK != status)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_read_data_from_single_drive: carve memory fails!\n" );
        return FBE_STATUS_GENERIC_FAILURE;
    }    

    /*now send the read request*/
    status  = fbe_database_send_read_packets_to_pdos(packet_array, &sg_list, drive_location_info,
        1, block_count, pls_id, NULL);
    if(FBE_STATUS_OK != status)
    {
        //fbe_memory_request_get_entry_mark_free(&memory_request, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
        fbe_memory_free_request_entry(&memory_request);

        fbe_semaphore_destroy(&semaphore);
        database_trace (FBE_TRACE_LEVEL_WARNING, /*just warning because the drive may be missed*/
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_read_data_from_single_drive: send_read_packets_to_pdos failed. The drive may be missed.\n" );

        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_database_copy_data_from_read_sg_list(data, data_length, sg_list);
    //fbe_memory_request_get_entry_mark_free(&memory_request, &memory_ptr);
    //fbe_memory_free_entry(memory_ptr);
    fbe_memory_free_request_entry(&memory_request);

    fbe_semaphore_destroy(&semaphore);

    if(FBE_STATUS_OK != status)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_read_data_from_single_drive: copy data from read sg list fails!\n" );
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_write_data_on_single_raw_drive
 ****************************************************************
 * @brief
 *  this is a generic function which write data to a raw drive
 *
 * @param[in]   data - the buffer which stores the data we want to write
 * @param[in]   data_length - the buffer length, that is how many bytes of data we want to write
 * @param[in]   drive_location_info - indicate which drive do we want to perform 
 *                     write. the "logical_drive_object_id" member is a returned value.
 * @param[in]   pls_id - the private space layout region where we want to write data to
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no errror.
 *
 * @version
 *  11/26/2011 - Created. zphu.
 *
 ****************************************************************/
fbe_status_t fbe_database_write_data_on_single_raw_drive(fbe_u8_t* data,
                                                         fbe_u64_t  data_length,
                                                         database_physical_drive_info_t* drive_location_info,
                                                         fbe_private_space_layout_region_id_t pls_id)
{
    fbe_memory_request_t        memory_request;
    //fbe_memory_ptr_t                memory_ptr;
    fbe_packet_t*              packet_array[2]; /*one master packet plus 1 subpacket*/
    fbe_sg_element_t*       sg_list = NULL;/*subpacket needs an sg list*/
    fbe_block_count_t               block_count;
    fbe_memory_chunk_size_t         chunk_size;
    fbe_memory_number_of_objects_t  chunk_number;
    fbe_semaphore_t          semaphore;
    fbe_status_t                    status;    

    /* Determine the block count.
     */
    status = fbe_database_determine_block_count_for_io(drive_location_info,
                                                       data_length,
                                                       &block_count);
    if (status != FBE_STATUS_OK) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "db_write_data_on_single_drive: failed to get block count\n" );
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Determine the chunk count.
     */
    if (block_count < FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_PACKET) {
        chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_PACKET;
    } else {
        chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO;
    }

    /*calculate the number of chunks needed for data*/
    chunk_number = (fbe_memory_number_of_objects_t)(block_count * FBE_BE_BYTES_PER_BLOCK + chunk_size - 1) /chunk_size;
    chunk_number++;  /* add one chunk for subpacket*/

    fbe_semaphore_init(&semaphore, 0, 1);
    fbe_memory_request_init(&memory_request);
    /* Allocate memroy for data */
    fbe_memory_build_chunk_request(&memory_request,
        chunk_size,
        chunk_number + 1,  /*one for master packet*/
        0, /* Lowest resource priority*/
        FBE_MEMORY_REQUEST_IO_STAMP_INVALID,
        (fbe_memory_completion_function_t)fbe_database_single_raw_drive_io_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
        &semaphore);

    status = fbe_memory_request_entry(&memory_request);

    if ((FBE_STATUS_OK != status) && (FBE_STATUS_PENDING != status)) {
        fbe_semaphore_destroy(&semaphore);
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_write_data_on_single_drive: failed to get memory\n" );
        return FBE_STATUS_GENERIC_FAILURE;
    }


    status = fbe_semaphore_wait_ms(&semaphore, 30000);
    if (FBE_STATUS_OK != status) {
        fbe_semaphore_destroy(&semaphore);
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_write_data_on_single_drive: timed out waiting to get memory\n" );
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (FBE_FALSE == fbe_memory_request_is_allocation_complete(&memory_request)){
        /* Currently this callback doesn't expect any aborted requests */
        fbe_semaphore_destroy(&semaphore);
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_write_data_on_single_drive: memory allocation failed\n" );
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*carve memory and sgl*/
    status = fbe_database_carve_memory_for_packet_and_sgl_extend(packet_array,
        &sg_list, 1, chunk_number - 1, /*sub the one for subpacket*/&memory_request);
    if(FBE_STATUS_OK != status)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_write_data_on_single_drive: carve memory failed!\n" );

        return FBE_STATUS_GENERIC_FAILURE;        
    }

    /*copy data to the sgl*/
    status = fbe_database_copy_data_to_write_sg_list(data, data_length, sg_list);
    if(FBE_STATUS_OK != status)
    {
        //fbe_memory_request_get_entry_mark_free(&memory_request, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
        fbe_memory_free_request_entry(&memory_request);

        fbe_semaphore_destroy(&semaphore);
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_write_data_on_single_drive: copy data to sg list  failed!\n" );

        return FBE_STATUS_GENERIC_FAILURE;        
    }

    /*now send the write request*/
    status  = fbe_database_send_write_packets_to_pdos(packet_array, &sg_list, drive_location_info,
        1, block_count, pls_id, NULL);

    //fbe_memory_request_get_entry_mark_free(&memory_request, &memory_ptr);
    //fbe_memory_free_entry(memory_ptr);
    fbe_memory_free_request_entry(&memory_request);

    fbe_semaphore_destroy(&semaphore);

    if(FBE_STATUS_OK != status)
    {
        database_trace (FBE_TRACE_LEVEL_WARNING, /*just warning as the drive maybe missed*/
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_write_data_on_single_drive: send_write_packets_to_pdos failed\n" );

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}


static fbe_status_t
fbe_database_single_raw_drive_io_allocation_completion(fbe_memory_request_t* mem_request, fbe_memory_completion_context_t context)
{
    fbe_semaphore_release((fbe_semaphore_t *)context, 0, 1, FALSE);
    return FBE_STATUS_OK;
}


/*copy the data from sg_list to out_data_buffer, checksum are skipped
  *
  * created by zphu
  */
static fbe_status_t
fbe_database_copy_data_from_read_sg_list(fbe_u8_t* out_data_buffer, fbe_u64_t data_length,
                                         fbe_sg_element_t* sg_list)
{
    fbe_u8_t*                        buffer;
    fbe_u32_t                       buffer_count;
    fbe_u64_t                       left_count;
    fbe_u32_t                       copied_count = 0;

    if(NULL == out_data_buffer ||NULL == sg_list || 0 ==data_length)
        return FBE_STATUS_GENERIC_FAILURE;


    left_count = data_length;  /*how many bytes we have not copied in*/

    while(sg_list->address != NULL)
    {
        buffer = sg_list->address;
        buffer_count = sg_list->count;
        while ((buffer_count > 0) && (left_count > 0)) {
            /* need to do the checksum here? */
            copied_count = left_count <  FBE_BYTES_PER_BLOCK ? (fbe_u32_t)left_count : DATABASE_SYSTEM_DB_RAW_BLOCK_DATA_SIZE;

            fbe_copy_memory(out_data_buffer, buffer, copied_count);

            out_data_buffer += copied_count;
            left_count  -= copied_count;

            buffer += FBE_BE_BYTES_PER_BLOCK;
            buffer_count -= FBE_BE_BYTES_PER_BLOCK;
        }
        if (0 == left_count) 
            break;

        sg_list++;
    }

    if(left_count)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_copy_data_from_read_sg_list: not all data are read into memory\n" );

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

/* copy the data from in_data_buffer to sg_list, checksum are calculated 
  * and filled into the sg element's address
  *
  * created by zphu
  */
static fbe_status_t
fbe_database_copy_data_to_write_sg_list(fbe_u8_t* in_data_buffer, fbe_u64_t data_length,
                                        fbe_sg_element_t* sg_list)
{
    fbe_u8_t*                        buffer;
    fbe_u32_t                       buffer_count;
    fbe_u64_t                       left_count;
    fbe_u32_t                       checksum;

    if(NULL == in_data_buffer ||NULL == sg_list || 0 ==data_length)
        return FBE_STATUS_GENERIC_FAILURE;

    left_count = data_length;  /*how many bytes we have not copied*/

    while(left_count)
    {
        buffer = sg_list->address;
        buffer_count = sg_list->count;

        if(NULL == buffer)
        {
            database_trace (FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_INFO,
                "db_copy_data_to_write_sg_list: not enough sg elments are given\n" );

            return FBE_STATUS_GENERIC_FAILURE;
        }
        fbe_zero_memory(buffer, buffer_count);

        while ((buffer_count > 0) && (left_count > 0)) {

            if(left_count < FBE_BYTES_PER_BLOCK)
            {
                fbe_copy_memory(buffer, in_data_buffer, (fbe_u32_t)left_count);
                left_count  = 0;
            }
            else
            {
                fbe_copy_memory(buffer, in_data_buffer, FBE_BYTES_PER_BLOCK);
                in_data_buffer += FBE_BYTES_PER_BLOCK;
                left_count  -= FBE_BYTES_PER_BLOCK;
            }

            checksum = fbe_xor_lib_calculate_checksum((fbe_u32_t*)buffer);
            buffer += FBE_BYTES_PER_BLOCK;
            *((fbe_u32_t*)buffer) = checksum;
            buffer += 8;

            buffer_count -= FBE_BE_BYTES_PER_BLOCK;

        }

        sg_list++;
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_send_write_packets_to_pdos
 ****************************************************************
 * @brief
 *  send write packets to the specified pdos
 *  if a drive is not available (missed or is not in READY state), the drive_location's
*   logical_drive_object_id would be set FBE_OBJECT_ID_INVALID
 *
 * @param[in]   packet_array - array which stores packets. The first packet is master 
 *                    packet. All other packets are subpackets, each of which is used for one drive
 * @param[in]   sg_list_array - array which stores sg lists, eahc sg list is for one subpacket
 * @param[in]   drive_location_info - indicate which drive do we want to perform 
 *                     write. the "logical_drive_object_id" member is a returned value if not specified.
 * @param[in]   pls_id - the private space layout region where we want to write data to
 * @param[in]   number_of_drives_for_operation - the size of the drive_locations array
 * @param[in]   write_block_number - how many blocks we want to write
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no errror.
 *
 * @version
 *  11/26/2011 - Created. zphu.
 *
 ****************************************************************/
static fbe_status_t fbe_database_send_write_packets_to_pdos(fbe_packet_t **packet_array,
                                                     fbe_sg_element_t **sg_list_array,
                                                     database_physical_drive_info_t* drive_locations,
                                                     fbe_u32_t  number_of_drives_for_operation,
                                                     fbe_block_count_t block_count,
                                                     fbe_private_space_layout_region_id_t  pls_id,
                                                     fbe_u32_t*   complete_packet_count)
{
    fbe_packet_t *                        master_packet_p = NULL;
    fbe_packet_t *                        sub_packet = NULL;
    fbe_payload_ex_t *                    sep_payload = NULL;
    fbe_payload_block_operation_t *            block_operation = NULL;
    fbe_sg_element_t*                        sg_list;
    fbe_u32_t                                                    sg_list_count;
    fbe_u32_t                             count = 0;
    fbe_private_space_layout_region_t            psl_region_info;
    fbe_status_t                                status;
    fbe_packet_t **                        sub_packet_array = NULL;
    fbe_bool_t*                                                 is_drive_missing = NULL;
    fbe_u32_t                                                    miss_drive_count = 0;
    fbe_cpu_id_t                        cpu_id;
    fbe_topology_control_get_physical_drive_by_location_t    topology_drive_location;
    database_pdo_io_context_t       io_context;
    fbe_lifecycle_state_t lifecycle_state;

    if(NULL == packet_array || NULL == sg_list_array || NULL == drive_locations)
        return FBE_STATUS_GENERIC_FAILURE;

    master_packet_p = *packet_array;/*first packet is the master one*/

    /*get some information about the ICA region*/
    fbe_private_space_layout_get_region_by_region_id(pls_id, &psl_region_info);

    fbe_transport_initialize_sep_packet(master_packet_p);
    cpu_id = fbe_get_cpu_id();
    fbe_transport_set_cpu_id(master_packet_p, cpu_id);

    packet_array++;
    sub_packet_array = packet_array;

    fbe_semaphore_init(&(io_context.semaphore), 0, 1);
    io_context.pdo_io_complete = 0;
    
    is_drive_missing = (fbe_bool_t*)fbe_memory_native_allocate(number_of_drives_for_operation * sizeof(fbe_bool_t));
    if(!is_drive_missing)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_send_write_2_pdo: can not alloc memory for is_drive_missing\n" );

        return FBE_STATUS_GENERIC_FAILURE;
    }
    for(count = 0; count < number_of_drives_for_operation; count++)
        is_drive_missing[count] = FBE_FALSE;

    /*now let's use the other packets*/
    for (count = 0; count < number_of_drives_for_operation; count++) {
        sub_packet = sub_packet_array[count];

        /*copy database physical drive info into topology drive location structure*/
        topology_drive_location.enclosure_number = drive_locations[count].enclosure_number;
        topology_drive_location.slot_number= drive_locations[count].slot_number;
        topology_drive_location.port_number= drive_locations[count].port_number;
        topology_drive_location.physical_drive_object_id= FBE_OBJECT_ID_INVALID;

        /*get the logical drive object state*/
            status = fbe_database_send_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_GET_PHYSICAL_DRIVE_BY_LOCATION,
                &topology_drive_location, 
                sizeof(topology_drive_location),
                FBE_SERVICE_ID_TOPOLOGY,
                NULL,    /* no sg list*/
                0,  /* no sg list*/
                FBE_PACKET_FLAG_EXTERNAL,
                FBE_PACKAGE_ID_PHYSICAL);
            if(FBE_STATUS_OK != status && FBE_STATUS_NO_OBJECT != status){
                fbe_semaphore_destroy(&(io_context.semaphore));
                fbe_database_clean_destroy_packet(master_packet_p);
                fbe_memory_native_release(is_drive_missing);
                drive_locations[count].logical_drive_object_id = FBE_OBJECT_ID_INVALID;
                return status;
            }

            /* Keep track of the missing drives */
            if(FBE_STATUS_NO_OBJECT == status)
            {
                is_drive_missing[count] = FBE_TRUE;
                drive_locations[count].logical_drive_object_id = FBE_OBJECT_ID_INVALID;
                miss_drive_count++;
                sg_list_array++;
                continue;
            }
            status = fbe_database_generic_get_object_state(topology_drive_location.physical_drive_object_id, &lifecycle_state, FBE_PACKAGE_ID_PHYSICAL);
            if (FBE_STATUS_OK != status || FBE_LIFECYCLE_STATE_READY != lifecycle_state)
            {
            
                is_drive_missing[count] = FBE_TRUE;
                drive_locations[count].logical_drive_object_id = FBE_OBJECT_ID_INVALID;
                miss_drive_count++;
                sg_list_array++;
                continue;
            }

            drive_locations[count].logical_drive_object_id = topology_drive_location.physical_drive_object_id;

        
        fbe_transport_initialize_sep_packet(sub_packet);

        /* set block operation same as master packet's prev block operation. */
        sep_payload = fbe_transport_get_payload_ex(sub_packet);
        block_operation = fbe_payload_ex_allocate_block_operation(sep_payload);

        /* set sg list with new packet. */
        sg_list = *sg_list_array;
        sg_list_count = 0;
        while(sg_list->address != NULL)
        {
            sg_list++;
            sg_list_count++;
        }
        fbe_payload_ex_set_sg_list(sep_payload, *sg_list_array, sg_list_count + 1);

        fbe_payload_block_build_operation(block_operation,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
            psl_region_info.starting_block_address,
            block_count,
            FBE_BE_BYTES_PER_BLOCK,
            1,
            NULL);

        /* set completion function */
        fbe_transport_set_completion_function(sub_packet,
            fbe_database_send_io_packets_to_pdos_completion,
            &io_context);

        /* we need to assosiate newly allocated packet with original one */
        fbe_transport_add_subpacket(master_packet_p, sub_packet);

        /*set address correctly*/
        fbe_transport_set_address(sub_packet,
            FBE_PACKAGE_ID_PHYSICAL,
            FBE_SERVICE_ID_TOPOLOGY,
            FBE_CLASS_ID_INVALID,
            topology_drive_location.physical_drive_object_id);

        fbe_payload_ex_increment_block_operation_level(sep_payload);

        sg_list_array++;/*move to new sg list*/
    }

    if(miss_drive_count == number_of_drives_for_operation)
    {
        fbe_semaphore_destroy(&(io_context.semaphore));
        fbe_memory_native_release(is_drive_missing);        
        fbe_database_clean_destroy_packet(master_packet_p);
        database_trace (FBE_TRACE_LEVEL_WARNING,
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_send_write_2_pdo: all drives are missed, so send no write  packets out\n" );

        return FBE_STATUS_GENERIC_FAILURE; 
    }

    for (count = 0; count < number_of_drives_for_operation; count++) {
        if(is_drive_missing[count])
            continue;
    
        sub_packet = sub_packet_array[count];

        status = fbe_topology_send_io_packet(sub_packet);

        /* The drive may not be READY to process io request. In this case, the return status
        * is not OK. We do not need to remove and destroy the subpacket here because
        * this is done in the completion function of the packet*/
        if(FBE_STATUS_OK != status)
        {
            database_trace (FBE_TRACE_LEVEL_WARNING,
                FBE_TRACE_MESSAGE_ID_INFO,
                "db_send_read_2_pdo: send packet to pdo 0x%x failed.\n", 
                drive_locations[count].logical_drive_object_id);
        }
    }

    /*let's wait for the semaphore to complete*/
    status = fbe_semaphore_wait_ms(&(io_context.semaphore), 0);

    /* see if there's problem reported by the lower leverl */
    status = fbe_transport_get_status_code(master_packet_p);

    fbe_semaphore_destroy(&(io_context.semaphore));
    fbe_memory_native_release(is_drive_missing);        
    fbe_database_clean_destroy_packet(master_packet_p);/*we'll free the actucal memory later*/

    if(NULL != complete_packet_count)
        *complete_packet_count = (fbe_u32_t)(io_context.pdo_io_complete);

    if (FBE_STATUS_OK != status) {
        database_trace (FBE_TRACE_LEVEL_WARNING,
            FBE_TRACE_MESSAGE_ID_INFO,
            "db_send_write_2_pdo failed, need to retry\n" );

        return status;/*user will want to retry the write*/
    }

    return FBE_STATUS_OK;
}


static fbe_status_t fbe_database_clean_destroy_packet(fbe_packet_t * packet)
{
    if(packet->subpacket_count) /*If there are subpackets, remove and destroy them first*/
    {
        packet->subpacket_count = 0;
        fbe_transport_destroy_subpackets(packet);
    }

    /*destroy the packet in question*/
    fbe_transport_destroy_packet(packet);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn database_homewrecker_invalidate_pvd_sn
 ****************************************************************
 * @brief
 *  this is the routine to invalidate the specified PVD's Serial Number, including
 *  the SN recorded in the pvd object entry and the one in the PVD object
 *
 * @param[in]   database_service_ptr - the global database service data structure
 * @param[in]   pvd_object_id - the object id of the PVD which needed to be invalidated
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no errror.
 *
 * @version
 *  June 21, 2012 - Created. Zhipeng Hu.
 *
 ****************************************************************/
static fbe_status_t database_homewrecker_invalidate_pvd_sn(fbe_database_service_t* database_service_ptr, fbe_object_id_t  pvd_object_id)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    database_object_entry_t   *pvd_entry = NULL;
    database_object_entry_t   new_pvd_object_entry;

    if(NULL == database_service_ptr)
        return FBE_STATUS_GENERIC_FAILURE;

    fbe_spinlock_lock(&database_service_ptr->object_table.table_lock);
    fbe_database_config_table_get_object_entry_by_id(&database_service_ptr->object_table, pvd_object_id, &pvd_entry);

    if(NULL == pvd_entry)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: fail to get the object entry of PVD 0x%x\n",
            __FUNCTION__, pvd_object_id);
        fbe_spinlock_unlock(&database_service_ptr->object_table.table_lock);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(&new_pvd_object_entry, pvd_entry, sizeof(new_pvd_object_entry));
    fbe_spinlock_unlock(&database_service_ptr->object_table.table_lock);

    /*invalidate the SN in the PVD object first*/
    fbe_zero_memory(&new_pvd_object_entry.set_config_union.pvd_config.serial_num[0], FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1);
    new_pvd_object_entry.set_config_union.pvd_config.update_type = FBE_UPDATE_PVD_SERIAL_NUMBER;
    
    status = database_common_update_config_from_table_entry(&new_pvd_object_entry);
    if(FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: failed to update config of object 0x%x\n",
                       __FUNCTION__, pvd_object_id);
        return status;
    }

    /*At last, we invalidate the actual pvd entry's SN*/
    fbe_spinlock_lock(&database_service_ptr->object_table.table_lock);
    fbe_zero_memory(&pvd_entry->set_config_union.pvd_config.serial_num[0], FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1);
    fbe_spinlock_unlock(&database_service_ptr->object_table.table_lock);

    return FBE_STATUS_OK;

}


fbe_status_t fbe_database_cleanup_and_reinitialize_persist_service(fbe_bool_t clean_up)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t  db_lun_id = 0;

    /* 
      the cleanup and re-initialization done as follows: 
      1) un-set lun for persist service
      2) zero the database lun
      3) set lun and re-initialize persist service on a empty database LUN
    */

     status = fbe_database_persist_unset_lun();
     if (status != FBE_STATUS_OK) {
         /*unset LUN failed*/
         database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "Cleanup and re-intialization: failed to un-set database lun\n");
         return status;
     }

     database_system_objects_get_db_lun_id(&db_lun_id);
     if (clean_up) {
         /*zero database lun*/
         status = fbe_database_zero_lun(db_lun_id, FBE_TRUE);
         if (status != FBE_STATUS_OK) {
             /*unset LUN failed*/
             database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "Cleanup and re-intialization: failed to zero database lun\n");
             return status;
         }
     }

     status = fbe_database_init_persist();
     if (status != FBE_STATUS_OK) {
         /*unset LUN failed*/
         database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "Cleanup and re-intialization: re-init persist service failed\n");
         return status;
     }

     return FBE_STATUS_OK;
}

/*!***************************************************************
@fn fbe_database_check_and_recreate_system_object()
******************************************************************
* @brief
*  This function is used to recreate the system object when system boot up.
*  It will load the system object operation flags from raw mirror.
*  Check whether the system object was marked as recreation or NDB.
*  If it is, recreate the system object by zeroing the non-paged metadata or
*  set the condition.
*  
*
* @return
*  fbe_status_t
*
* @version
* 8/17/2012: created by gaoh1
*
******************************************************************/
fbe_status_t fbe_database_check_and_recreate_system_object(fbe_database_system_object_recreate_flags_t *sys_obj_recreate_flags_p)
{
    fbe_u32_t                               read_drive_mask;
    fbe_object_id_t                         object_id;
    fbe_status_t                            status;
    database_object_entry_t                 pvd_config;


    status = fbe_database_raw_mirror_get_valid_drives(&read_drive_mask);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_INFO, 
            "%s: failed to get valid raw-mirror drives, instead load from all drives \n",
            __FUNCTION__);
        read_drive_mask = DATABASE_SYSTEM_RAW_MIRROR_ALL_DB_DRIVE_MASK;
    }
    status = fbe_database_read_system_object_recreate_flags(sys_obj_recreate_flags_p ,read_drive_mask);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: read system object op flags failed!\n",
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*if NO system object is marked as recreation, do nothing and return FBE_STATUS_OK */
    if (sys_obj_recreate_flags_p->valid_num == 0) {
        database_trace(FBE_TRACE_LEVEL_INFO, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: No system object needs to be recreated\n",
                    __FUNCTION__);
        return FBE_STATUS_OK;
    }
    
    for (object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FIRST; object_id <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LAST; object_id++)
    {
        if ((sys_obj_recreate_flags_p->system_object_op[object_id] & FBE_SYSTEM_OBJECT_OP_RECREATE) == 0)
            continue;

        if (fbe_private_space_layout_object_id_is_system_lun(object_id) || 
            fbe_private_space_layout_object_id_is_system_raid_group(object_id))
        {
            /* If the object needs to be recreate, zero and persist the np metadata of that object*/
            database_trace(FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: zero out non-paged md for object(%d)\n",
                           __FUNCTION__, object_id);
            status = fbe_database_metadata_nonpaged_system_zero_and_persist(object_id, 1);
            if (status != FBE_STATUS_OK) 
            {
                database_trace(FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s: Fail to zero and persist np metadata\n",
                              __FUNCTION__);
                return status;  
            }
            if (fbe_private_space_layout_object_id_is_system_raid_group(object_id))
            {
                continue;
            }
            /* If it is a lun object and NDB flags is set, then reset the configuration with ndb flags */
            if ((sys_obj_recreate_flags_p->system_object_op[object_id] & FBE_SYSTEM_OBJECT_OP_NDB_LUN) != 0)
            {
                status = fbe_database_system_lun_set_ndb_flags(object_id, FBE_TRUE);
                if (status != FBE_STATUS_OK) 
                {
                    database_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s: Fail to set ndb flags of system lun to disk\n",
                               __FUNCTION__);
                    return status;  
                }
            } else {
                status = fbe_database_system_lun_set_ndb_flags(object_id, FBE_FALSE);
                if (status != FBE_STATUS_OK) 
                {
                    database_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s: Fail to set ndb flags of system lun to disk\n",
                               __FUNCTION__);
                    return status;  
                }
            }

        }

        if (fbe_private_space_layout_object_id_is_system_pvd(object_id)) 
        {
            if (sys_obj_recreate_flags_p->system_object_op[object_id] & FBE_SYSTEM_OBJECT_OP_PVD_ZERO)
            {
                status = fbe_database_metadata_nonpaged_system_zero_and_persist(object_id, 1);
                if (status != FBE_STATUS_OK) 
                {
                    database_trace(FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_INFO, 
                                  "%s: Fail to zero and persist np metadata\n",
                                  __FUNCTION__);
                    return status;  
                }
            } else { /* If zero flags is not set, we need to set the condition to re-initialize the non-paged */
                fbe_zero_memory(&pvd_config, sizeof(database_object_entry_t));
                pvd_config.set_config_union.pvd_config.update_type = FBE_UPDATE_PVD_NONPAGED_BY_DEFALT_VALUE;
                status = fbe_database_update_pvd_config(object_id, &pvd_config);
                if (status != FBE_STATUS_OK) 
                {
                    database_trace(FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_INFO, 
                                  "%s: Fail to update the PVD to re-initialize non-paged by paged\n",
                                  __FUNCTION__);
                    return status;  
                }
            }

        }
    }


    return FBE_STATUS_OK;
    
}

/*!***************************************************************
@fn fbe_database_system_lun_set_ndb_flags()
******************************************************************
* @brief
*  This function sets the ndb flags of the system lun.
*  As a result, if this system lun is recreated, the data of this lun
*  won't be zeroed.
*  
* @return
*  fbe_status_t
*
* @version
* 8/21/2012: created by gaoh1
*
******************************************************************/
static fbe_status_t fbe_database_system_lun_set_ndb_flags(fbe_object_id_t object_id, fbe_bool_t ndb)
{
    database_object_entry_t     object_entry;
    fbe_bool_t                  is_system_lun = FBE_TRUE;
    fbe_status_t                status;

    /* Make sure the object is in system object range */
    is_system_lun = fbe_private_space_layout_object_id_is_system_lun(object_id);
    if (is_system_lun == FBE_FALSE) 
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: the input argument is not a system object lun\n",
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(&object_entry, sizeof (database_object_entry_t));
    
    /* load the system object from raw mirror */
    status = fbe_database_system_db_read_object_entry(object_id, &object_entry);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: load object entry(0x%x) failed. status = %d \n",
                       __FUNCTION__, object_id, status);
        return status;
    }

    /* It should be a system LUN object */
    if (object_entry.header.object_id != object_id)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "Get the wrong object entry. (obj_id: 0x%x, object_entry's obj_id :0x%x). \n",
                       object_id, object_entry.header.object_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (ndb == FBE_TRUE)
    {
        /* If the original flags is already with the ndb flags, do nothing and return */
        if ((object_entry.set_config_union.lu_config.config_flags & FBE_LUN_CONFIG_NO_USER_ZERO) == FBE_LUN_CONFIG_NO_USER_ZERO)
        {
            database_trace(FBE_TRACE_LEVEL_INFO, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: The system LUN is already with ndb flag\n",
                           __FUNCTION__);
            return FBE_STATUS_OK;
        } else {
            object_entry.set_config_union.lu_config.config_flags |= FBE_LUN_CONFIG_NO_USER_ZERO;
            status = fbe_database_system_db_raw_persist_object_entry(FBE_DATABASE_SYSTEM_DB_PERSIST_UPDATE, &object_entry);
            if (status != FBE_STATUS_OK) {
                database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: persist object entry failed.\n",
                           __FUNCTION__);
            }
            return status;
        }      

    } else {
        /* If the original flags is not the ndb flags, do nothing and return */
        if ((object_entry.set_config_union.lu_config.config_flags & FBE_LUN_CONFIG_NO_USER_ZERO) != FBE_LUN_CONFIG_NO_USER_ZERO)
        {
            database_trace(FBE_TRACE_LEVEL_INFO, 
                            FBE_TRACE_MESSAGE_ID_INFO, 
                            "%s: The system LUN is already without ndb flag\n",
                            __FUNCTION__);
            return FBE_STATUS_OK;
        } else {
            object_entry.set_config_union.lu_config.config_flags &= ~FBE_LUN_CONFIG_NO_USER_ZERO;
            status = fbe_database_system_db_raw_persist_object_entry(FBE_DATABASE_SYSTEM_DB_PERSIST_UPDATE, &object_entry);
            if (status != FBE_STATUS_OK) {
                database_trace(FBE_TRACE_LEVEL_ERROR, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: persist object entry failed.\n",
                            __FUNCTION__);
            }
            return status;
        } 

    }
}


/***********************************************
* end file fbe_database_boot_path.c
***********************************************/


