/***************************************************************************
 * Copyright (C) EMC Corporation 2010-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_database_config_tables.c
 ***************************************************************************
 *
 * @brief
 *  This file contains logical configuration service functions which
 *  process control request.
 *  
 * @version
 *  12/15/2010 - Created. 
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_database_config_tables.h"
#include "fbe_system_limits.h"
#include "fbe/fbe_event_log_api.h"
#include "fbe/fbe_event_log_utils.h"


fbe_status_t fbe_database_allocate_config_tables(fbe_database_service_t *database_service_ptr)
{

    fbe_u32_t 		max_topology_objects;
    fbe_status_t	status;

    fbe_u32_t   database_system_drives_num = 0;
    database_system_drives_num = fbe_private_space_layout_get_number_of_system_drives();

    /*first, based on the limits of the system, let's allocate the table size based on this rule:
    We allow for the max clariion drives (1000) to always be set since someone can populate a small system with 1000 drive in theory.
    These drives will show up in physical package and will be converted to PVDs. ESP will kill them eventually since they pass the limit, but still,
    they are PVDs we have to put in the table. this is why we start with 1000 no matter what.

    Then, we just add up all the possible RG and LU and we shold be all set.

    For RG, since they may be R1_0 and generate mirror undeer them which the user is not charged for but they still have 
    to live in the tbale. In this case, if we generate one mirror for each RG it will give us a close number to the other scenario
    in which we generated 62 R10 with 16 mirrors under them which will consume all 1000 drives. Hence we choose to multiply the number of RG

    */

	/*FIXME !!!!
		Until we take off the space from the ESP LUN, remove ESP entries and other junk we can't support 2000 drives
		on the persist service so for now we'll calculate to the max of 1000 drive, after that, enable the line below
		and remopve the one below it we use today
	*/
    /*database_service_ptr->max_configurable_objects = database_service_ptr->logical_objects_limits.platform_fru_count * 2;*/
    
    /* 2/27/2013: When calculating the the configurable objects, we use the mixmum(1000, platform_fru_count) as the base number */
    if (database_service_ptr->logical_objects_limits.platform_fru_count < 1000) {
	    database_service_ptr->max_configurable_objects = 1000 * 2;
    } else {
        database_service_ptr->max_configurable_objects = database_service_ptr->logical_objects_limits.platform_fru_count * 2;  
    }
#if defined(SIMMODE_ENV)
    database_service_ptr->max_configurable_objects = database_service_ptr->logical_objects_limits.platform_fru_count * 2;  /* *2 for PVD and VD objects*/
#endif
    database_service_ptr->max_configurable_objects += ((database_service_ptr->logical_objects_limits.platform_max_rg * 2) +  /* *2 covers RAID10s */
                                                       database_service_ptr->logical_objects_limits.platform_max_user_lun);

    database_service_ptr->max_configurable_objects += FBE_RESERVED_OBJECT_IDS;/*for all the PSL objects*/

    status = fbe_system_limits_get_max_objects(FBE_PACKAGE_ID_SEP_0, &max_topology_objects);

    /* If the final calculated max_configurable_objects exceed the system max topology objects number, it will return failure here */
    if (max_topology_objects < database_service_ptr->max_configurable_objects || status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: system objects verification failed(max=%d)!\n", __FUNCTION__, max_topology_objects);

        set_database_service_state(database_service_ptr, FBE_DATABASE_STATE_FAILED);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Increase the max configurable objects by 2% (1/50). This will allow a new pvd to be inserted in a 
     * max config if the original pvd is stuck in specialize due to a disk removal. 
     */
    database_service_ptr->max_configurable_objects += database_service_ptr->max_configurable_objects/50;

    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,"%s:Object and user size:%d\n", 
                   __FUNCTION__, database_service_ptr->max_configurable_objects);

    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,"%s:Edges size:%d\n", 
                   __FUNCTION__, database_service_ptr->max_configurable_objects * 16);

    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,"%s:System config size:%d\n", 
                   __FUNCTION__, DATABASE_GLOBAL_INFO_TYPE_LAST);



    if(database_service_ptr->user_table.table_content.user_entry_ptr != NULL){
         database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: user table is already allocated! addr:0x%p.\n", 
                           __FUNCTION__, database_service_ptr->user_table.table_content.user_entry_ptr);
     }
     else{
         database_service_ptr->user_table.table_content.user_entry_ptr = fbe_memory_allocate_required(sizeof(database_user_entry_t)* database_service_ptr->max_configurable_objects);
         if (database_service_ptr->user_table.table_content.user_entry_ptr == NULL) {
             database_trace(FBE_TRACE_LEVEL_WARNING, 
                                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                       "%s: failed to allocate memory for user table!\n", 
                                       __FUNCTION__);
             set_database_service_state(database_service_ptr, FBE_DATABASE_STATE_FAILED);
             return FBE_STATUS_GENERIC_FAILURE;
         }
         fbe_zero_memory(database_service_ptr->user_table.table_content.user_entry_ptr, 
                         sizeof(database_user_entry_t)* database_service_ptr->max_configurable_objects);
         database_service_ptr->user_table.table_size = database_service_ptr->max_configurable_objects;
         database_service_ptr->user_table.table_type = DATABASE_CONFIG_TABLE_USER;
         database_service_ptr->user_table.alloc_size = sizeof(database_user_entry_t) * database_service_ptr->user_table.table_size;
         database_service_ptr->user_table.peer_table_start_address = 0;
         fbe_spinlock_init(&database_service_ptr->user_table.table_lock);
     }
    
     if(database_service_ptr->object_table.table_content.object_entry_ptr != NULL){
         database_trace(FBE_TRACE_LEVEL_WARNING, 
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: object table is already allocated! addr:0x%p.\n", 
                                   __FUNCTION__, database_service_ptr->object_table.table_content.object_entry_ptr);
     }
     else{
         database_service_ptr->object_table.table_content.object_entry_ptr = fbe_memory_allocate_required(sizeof(database_object_entry_t)* database_service_ptr->max_configurable_objects);
         if (database_service_ptr->object_table.table_content.object_entry_ptr == NULL) {
             database_trace(FBE_TRACE_LEVEL_WARNING, 
                                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                       "%s: failed to allocate memory for object table!\n", 
                                       __FUNCTION__);
             set_database_service_state(database_service_ptr, FBE_DATABASE_STATE_FAILED);
             return FBE_STATUS_GENERIC_FAILURE;
         }
         fbe_zero_memory(database_service_ptr->object_table.table_content.object_entry_ptr, 
                         sizeof(database_object_entry_t)* database_service_ptr->max_configurable_objects);
         database_service_ptr->object_table.table_size = database_service_ptr->max_configurable_objects;
         database_service_ptr->object_table.table_type = DATABASE_CONFIG_TABLE_OBJECT;
         database_service_ptr->object_table.alloc_size = sizeof(database_object_entry_t) * database_service_ptr->object_table.table_size;
         database_service_ptr->object_table.peer_table_start_address = 0;
         fbe_spinlock_init(&database_service_ptr->object_table.table_lock);
     }
    
     if(database_service_ptr->edge_table.table_content.edge_entry_ptr != NULL){
         database_trace(FBE_TRACE_LEVEL_WARNING, 
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: edge table is already allocated! addr:0x%p.\n", 
                                   __FUNCTION__, database_service_ptr->edge_table.table_content.edge_entry_ptr);
     }
     else{
         database_service_ptr->edge_table.table_size = database_service_ptr->max_configurable_objects * DATABASE_MAX_EDGE_PER_OBJECT;
         database_service_ptr->edge_table.table_content.edge_entry_ptr = fbe_memory_allocate_required(sizeof(database_edge_entry_t)
                                                                           *database_service_ptr->edge_table.table_size);
         if (database_service_ptr->edge_table.table_content.edge_entry_ptr == NULL) {
             database_trace(FBE_TRACE_LEVEL_WARNING, 
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s: failed to allocate memory for edge table!\n", 
                             __FUNCTION__);
             set_database_service_state(database_service_ptr, FBE_DATABASE_STATE_FAILED);
             return FBE_STATUS_GENERIC_FAILURE;
         }
         fbe_zero_memory(database_service_ptr->edge_table.table_content.edge_entry_ptr, 
                         sizeof(database_edge_entry_t)*database_service_ptr->edge_table.table_size);
         database_service_ptr->edge_table.table_type = DATABASE_CONFIG_TABLE_EDGE;
         database_service_ptr->edge_table.alloc_size = sizeof(database_edge_entry_t) * database_service_ptr->edge_table.table_size;
         database_service_ptr->edge_table.peer_table_start_address = 0;
         fbe_spinlock_init(&database_service_ptr->edge_table.table_lock);
     }

     if(database_service_ptr->system_spare_table.table_content.system_spare_entry_ptr != NULL){
         database_trace(FBE_TRACE_LEVEL_WARNING, 
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: system spare table is already allocated! addr:0x%p.\n", 
                                   __FUNCTION__, database_service_ptr->system_spare_table.table_content.system_spare_entry_ptr);
     }
     else{
         database_service_ptr->system_spare_table.table_size = database_system_drives_num;
         database_service_ptr->system_spare_table.table_content.system_spare_entry_ptr 
             = fbe_memory_native_allocate(sizeof(database_system_spare_entry_t)*database_service_ptr->system_spare_table.table_size);
         if (database_service_ptr->system_spare_table.table_content.system_spare_entry_ptr == NULL) {
             database_trace(FBE_TRACE_LEVEL_WARNING, 
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s: failed to allocate memory for system spare table!\n", 
                             __FUNCTION__);
             set_database_service_state(database_service_ptr, FBE_DATABASE_STATE_FAILED);
             return FBE_STATUS_GENERIC_FAILURE;
         }
         fbe_zero_memory(database_service_ptr->system_spare_table.table_content.system_spare_entry_ptr, 
                         sizeof(database_system_spare_entry_t)*database_service_ptr->system_spare_table.table_size);
         database_service_ptr->system_spare_table.table_type = DATABASE_CONFIG_TABLE_SYSTEM_SPARE;
         database_service_ptr->system_spare_table.alloc_size = sizeof(database_system_spare_entry_t) * database_service_ptr->system_spare_table.table_size;
         database_service_ptr->system_spare_table.peer_table_start_address = 0;
         fbe_spinlock_init(&database_service_ptr->system_spare_table.table_lock);
     }

     if(database_service_ptr->global_info.table_content.global_info_ptr != NULL)
     {
         database_trace(FBE_TRACE_LEVEL_WARNING, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s: Global Info table is already allocated! addr:0x%p.\n", 
                        __FUNCTION__, database_service_ptr->global_info.table_content.global_info_ptr);
     }
     else
     {
         database_service_ptr->global_info.table_size = DATABASE_GLOBAL_INFO_TYPE_LAST;
         database_service_ptr->global_info.table_content.global_info_ptr = fbe_memory_native_allocate(sizeof(database_global_info_entry_t)
                                                                                                      *database_service_ptr->global_info.table_size);
         if (database_service_ptr->global_info.table_content.global_info_ptr == NULL)
         {
             database_trace(FBE_TRACE_LEVEL_WARNING, 
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s: failed to allocate memory for global Info table!\n", 
                            __FUNCTION__);

             set_database_service_state(database_service_ptr, FBE_DATABASE_STATE_FAILED);
             return FBE_STATUS_GENERIC_FAILURE;
         }

         fbe_zero_memory(database_service_ptr->global_info.table_content.global_info_ptr, 
                         sizeof(database_global_info_entry_t)*database_service_ptr->global_info.table_size); 
         database_service_ptr->global_info.table_type = DATABASE_CONFIG_TABLE_GLOBAL_INFO;
         database_service_ptr->global_info.alloc_size = sizeof(database_global_info_entry_t) * database_service_ptr->global_info.table_size;
         database_service_ptr->global_info.peer_table_start_address = 0;
         fbe_spinlock_init(&database_service_ptr->global_info.table_lock);
     }

     return FBE_STATUS_OK;
}

fbe_status_t fbe_database_config_tables_destroy(fbe_database_service_t *database_service_ptr)
{
    if(database_service_ptr->user_table.table_content.user_entry_ptr != NULL){
        database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: free user table addr:0x%p.\n",
                       __FUNCTION__, database_service_ptr->user_table.table_content.user_entry_ptr);
        fbe_memory_release_required(database_service_ptr->user_table.table_content.user_entry_ptr);
        database_service_ptr->user_table.table_content.user_entry_ptr = NULL;
        database_service_ptr->user_table.table_type = DATABASE_CONFIG_TABLE_INVALID;
        fbe_spinlock_destroy(&database_service_ptr->user_table.table_lock);
    }
    else{
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: user table is already free!\n", 
                       __FUNCTION__);
    }

    if(database_service_ptr->object_table.table_content.object_entry_ptr != NULL){
        database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: free object table addr:0x%p.\n",
                       __FUNCTION__, database_service_ptr->object_table.table_content.object_entry_ptr);
        fbe_memory_release_required(database_service_ptr->object_table.table_content.object_entry_ptr);
        database_service_ptr->object_table.table_content.object_entry_ptr = NULL;
        database_service_ptr->object_table.table_type = DATABASE_CONFIG_TABLE_INVALID;
        fbe_spinlock_destroy(&database_service_ptr->object_table.table_lock);
    }
    else{
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: object table is already free!\n", 
                       __FUNCTION__);
    }

    if(database_service_ptr->edge_table.table_content.edge_entry_ptr != NULL){
        database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: free edge table addr:0x%p.\n",
                       __FUNCTION__, database_service_ptr->edge_table.table_content.edge_entry_ptr);
        fbe_memory_release_required(database_service_ptr->edge_table.table_content.edge_entry_ptr);
        database_service_ptr->edge_table.table_type = DATABASE_CONFIG_TABLE_INVALID;
        database_service_ptr->edge_table.table_content.edge_entry_ptr = NULL;
        fbe_spinlock_destroy(&database_service_ptr->edge_table.table_lock);
    }
    else{
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: edge table is already free!\n", 
                       __FUNCTION__);
    }

    if(database_service_ptr->system_spare_table.table_content.system_spare_entry_ptr != NULL){
        database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: free system spare table addr:0x%p.\n",
                       __FUNCTION__, database_service_ptr->edge_table.table_content.edge_entry_ptr);
        fbe_memory_native_release(database_service_ptr->system_spare_table.table_content.system_spare_entry_ptr);
        database_service_ptr->system_spare_table.table_type = DATABASE_CONFIG_TABLE_INVALID;
        database_service_ptr->system_spare_table.table_content.system_spare_entry_ptr = NULL;
        fbe_spinlock_destroy(&database_service_ptr->system_spare_table.table_lock);
    }
    else{
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: system spare table is already free!\n", 
                       __FUNCTION__);
    }


    if(database_service_ptr->global_info.table_content.global_info_ptr != NULL)
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: free global_info table addr:0x%p.\n",
                       __FUNCTION__, database_service_ptr->global_info.table_content.global_info_ptr);

        fbe_memory_native_release(database_service_ptr->global_info.table_content.global_info_ptr);
        database_service_ptr->global_info.table_content.global_info_ptr = NULL;
        database_service_ptr->global_info.table_type = DATABASE_CONFIG_TABLE_INVALID;
        fbe_spinlock_destroy(&database_service_ptr->global_info.table_lock);
    }
    else
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: global_info table is already free!\n", 
                       __FUNCTION__);
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_populate_system_power_save_info
 *****************************************************************
 * @brief
 *   This function populates the system power save info with default entries.
 *
 * @param in_entry_ptr - pointer to the system power save info in the DB table.
 *
 * @return fbe_status_t - Always OK. 
 *
 * @version
 *    5/31/2011: Arun S - created
 *
 ****************************************************************/
fbe_status_t fbe_database_populate_system_power_save_info(fbe_system_power_saving_info_t *in_entry_ptr)
{
    in_entry_ptr->enabled = FBE_FALSE;
    in_entry_ptr->hibernation_wake_up_time_in_minutes = FBE_DATABASE_DEFAULT_WAKE_UP_TIMER;
    in_entry_ptr->stats_enabled = FBE_FALSE;

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_populate_system_encryption_info
 *****************************************************************
 * @brief
 *   This function populates the system encryption info with default entries.
 *
 * @param in_entry_ptr - pointer to the system encryption info in the DB table.
 *
 * @return fbe_status_t - Always OK. 
 *
 * @version
 *
 ****************************************************************/
fbe_status_t fbe_database_populate_system_encryption_info(fbe_system_encryption_info_t *in_entry_ptr)
{
    in_entry_ptr->encryption_mode   = FBE_SYSTEM_ENCRYPTION_MODE_INVALID;
    in_entry_ptr->encryption_paused = FBE_FALSE;
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_populate_system_spare
 *****************************************************************
 * @brief
 *   This function populates the system spare info with default entries.
 *
 * @param in_entry_ptr - pointer to the system spare info in the DB table.
 *
 * @return fbe_status_t - Always OK. 
 *
 * @version
 *    5/31/2011: Arun S - created
 *
 ****************************************************************/
fbe_status_t fbe_database_populate_system_spare(fbe_system_spare_config_info_t *in_entry_ptr)
{
    in_entry_ptr->permanent_spare_trigger_time = FBE_SPARE_DEFAULT_SWAP_IN_TRIGGER_TIME;
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_populate_system_generation_info
 *****************************************************************
 * @brief
 *   This function populates the system spare info with default entries.
 *
 * @param in_entry_ptr - pointer to the system spare info in the DB table.
 *
 * @return fbe_status_t - Always OK. 
 *
 * @version
 *    7/19/2011: created
 *
 ****************************************************************/
fbe_status_t fbe_database_populate_system_generation_info(fbe_system_generation_info_t *in_entry_ptr)
{
    in_entry_ptr->current_generation_number = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_LAST;
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_populate_system_time_threshold_info
 *****************************************************************
 * @brief
 *   This function populates the system time threshold info with default value.
 *
 * @param in_entry_ptr - pointer to the system time threshold entry in the DB table.
 *
 * @return fbe_status_t - Always OK. 
 *
 * @version
 *    1/09/2012: created
 *
 ****************************************************************/
fbe_status_t fbe_database_populate_system_time_threshold_info(fbe_system_time_threshold_info_t *in_entry_ptr)
{
    in_entry_ptr->time_threshold_in_minutes = 10080;// one week 
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_populate_global_pvd_config
 *****************************************************************
 * @brief
 *   This function populates the system pvd info with default entries.
 *
 * @param in_entry_ptr - pointer to the system pvd info in the DB table.
 *
 * @return fbe_status_t - Always OK. 
 *
 * @version
 *
 ****************************************************************/
fbe_status_t fbe_database_populate_global_pvd_config(fbe_global_pvd_config_t *in_entry_ptr)
{
    in_entry_ptr->user_capacity_limit = 0;

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_populate_default_global_info
 *****************************************************************
 * @brief
 *   This function populates the global info with default entries.
 *
 * @param in_table_ptr - pointer to the global info in the DB table.
 *
 * @return fbe_status_t - Always OK. 
 *
 * @version
 *    5/20/2011: Arun S - created
 *             : zhangy - add time_threshold
 ****************************************************************/
fbe_status_t fbe_database_populate_default_global_info(database_table_t *in_table_ptr)
{
    fbe_u32_t index = 0;
    database_global_info_entry_t *global_info_ptr = NULL;

    if(in_table_ptr->table_type != DATABASE_CONFIG_TABLE_GLOBAL_INFO) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: Wrong - Table Type 0x%x! Expect 0x%x\n", 
                       __FUNCTION__, in_table_ptr->table_type, DATABASE_CONFIG_TABLE_GLOBAL_INFO);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Initialize the global info entries (including INVALID) with default values. */
    global_info_ptr = in_table_ptr->table_content.global_info_ptr;
    for (index = DATABASE_GLOBAL_INFO_TYPE_INVALID + 1; index < in_table_ptr->table_size; index++)
    {
        fbe_zero_memory(global_info_ptr, sizeof(database_global_info_entry_t));
        global_info_ptr->header.state = DATABASE_CONFIG_ENTRY_UNCOMMITTED;
        global_info_ptr->header.entry_id = 0;
        global_info_ptr->header.object_id = FBE_OBJECT_ID_INVALID;
        global_info_ptr->type = index;
        global_info_ptr->header.version_header.size = database_common_global_info_entry_size(global_info_ptr->type);
        switch(global_info_ptr->type) {
        case DATABASE_GLOBAL_INFO_TYPE_SYSTEM_POWER_SAVE:
            /* Initialize the system power save structure */
            fbe_database_populate_system_power_save_info(&global_info_ptr->info_union.power_saving_info);
            break;
        case DATABASE_GLOBAL_INFO_TYPE_SYSTEM_SPARE:
            /* Initialize the system spare structure */
            fbe_database_populate_system_spare(&global_info_ptr->info_union.spare_info);
            break;
        case DATABASE_GLOBAL_INFO_TYPE_SYSTEM_GENERATION:
            /* Initialize the system generation_info structure */
            fbe_database_populate_system_generation_info(&global_info_ptr->info_union.generation_info);
            break;
        case DATABASE_GLOBAL_INFO_TYPE_SYSTEM_TIME_THRESHOLD:
            /* Initialize the system time_threshold_info structure */
            fbe_database_populate_system_time_threshold_info(&global_info_ptr->info_union.time_threshold_info);
            break;
        case DATABASE_GLOBAL_INFO_TYPE_SYSTEM_ENCRYPTION:
            /* Initialize the system power save structure */
            fbe_database_populate_system_encryption_info(&global_info_ptr->info_union.encryption_info);
            break;
        case DATABASE_GLOBAL_INFO_TYPE_GLOBAL_PVD_CONFIG:
            /* Initialize the system power save structure */
            fbe_database_populate_global_pvd_config(&global_info_ptr->info_union.pvd_config);
            break;
        default:
            break;
        }
        global_info_ptr++;
    }
    return FBE_STATUS_OK;
}

/*callers of this function should lock the object table of database service in their context*/
fbe_status_t fbe_database_config_table_get_non_reserved_free_object_id(database_object_entry_t *in_object_table_ptr, 
                                                             database_table_size_t size, 
                                                             fbe_object_id_t *out_object_id_p)
{
    database_table_header_t * header_ptr = NULL;
    database_object_entry_t * current_entry_ptr = NULL;
    fbe_u64_t               index = 0;
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;

    if(NULL == out_object_id_p)
        return FBE_STATUS_GENERIC_FAILURE;

    current_entry_ptr = &in_object_table_ptr[FBE_RESERVED_OBJECT_IDS];
    for(index = FBE_RESERVED_OBJECT_IDS; index < size; index++) {
        if(current_entry_ptr != NULL) {
            header_ptr = (database_table_header_t *)current_entry_ptr;
            if(header_ptr->state == DATABASE_CONFIG_ENTRY_INVALID) {
                /* this is a free entry, let's return it to the caller */
                *out_object_id_p = (fbe_object_id_t)index;
                status = FBE_STATUS_OK;
                break;
            }
        }
        else{
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: Table ptr is NULL!\n", 
                           __FUNCTION__);
            break;
        }
        current_entry_ptr++;
    }
    return status;
}

/* callers of these get_free_xxxx_entry needs to get proper locks.
 * Note that currently they are only called from transaction with its private table entries.
 */
fbe_status_t fbe_database_config_table_get_free_object_entry(database_object_entry_t *in_object_table_ptr, 
                                                             database_table_size_t size, 
                                                             database_object_entry_t **out_object_entry_ptr)
{
    database_table_header_t * header_ptr = NULL;
    database_object_entry_t * current_entry_ptr = NULL;
    fbe_u64_t               index = 0;
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;

    *out_object_entry_ptr = NULL; /* start with NULL */
    current_entry_ptr = in_object_table_ptr;
    for(index = 0; index < size; index++) {
        if(current_entry_ptr != NULL) {
            header_ptr = (database_table_header_t *)current_entry_ptr;
            if(header_ptr->state == DATABASE_CONFIG_ENTRY_INVALID) {
                /* this is a free entry, let's return it to the caller */
                *out_object_entry_ptr = current_entry_ptr;
                status = FBE_STATUS_OK;
                break;
            }
        }
        else{
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: Table ptr is NULL!\n", 
                           __FUNCTION__);
            break;
        }
        current_entry_ptr++;
    }
    return status;
}

fbe_status_t fbe_database_config_table_get_free_user_entry(database_user_entry_t *in_user_table_ptr, 
                                                             database_table_size_t size, 
                                                             database_user_entry_t **out_user_entry_ptr)
{
    database_table_header_t * header_ptr = NULL;
    database_user_entry_t * current_entry_ptr = NULL;
    fbe_u64_t               index = 0;

    *out_user_entry_ptr = NULL; /* start with NULL */
    current_entry_ptr = in_user_table_ptr;
    for(index = 0; index < size; index++) {
        if(current_entry_ptr != NULL) {
            header_ptr = (database_table_header_t *)current_entry_ptr;
            if(header_ptr->state == DATABASE_CONFIG_ENTRY_INVALID) {
                /* this is a free entry, let's return it to the caller */
                *out_user_entry_ptr = current_entry_ptr;
                return FBE_STATUS_OK;
            }
        }
        else{
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: Table ptr is NULL!\n", 
                           __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        current_entry_ptr++;
    }
    return FBE_STATUS_GENERIC_FAILURE;
}

fbe_status_t fbe_database_config_table_get_free_edge_entry(database_edge_entry_t *in_edge_table_ptr, 
                                                             database_table_size_t size, 
                                                             database_edge_entry_t **out_edge_entry_ptr)
{
    database_table_header_t * header_ptr = NULL;
    database_edge_entry_t * current_entry_ptr = NULL;
    fbe_u64_t               index = 0;

    *out_edge_entry_ptr = NULL; /* start with NULL */
    current_entry_ptr = in_edge_table_ptr;
    for(index = 0; index < size; index++) {
        if(current_entry_ptr != NULL) {
            header_ptr = (database_table_header_t *)current_entry_ptr;
            if(header_ptr->state == DATABASE_CONFIG_ENTRY_INVALID) {
                /* this is a free entry, let's return it to the caller */
                *out_edge_entry_ptr = current_entry_ptr;
                return FBE_STATUS_OK;
            }
        }
        else{
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: Table ptr is NULL!\n", 
                           __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        current_entry_ptr++;
    }
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!***************************************************************
 * @fn fbe_database_config_table_get_free_global_info_entry
 *****************************************************************
 * @brief
 *   This function gets the free global info entry in the
 *   database table before adding the transaction to the table. 
 *
 * @param in_global_info_table_ptr - pointer to the db system setting interface.
 * @param out_global_info_entry_ptr - pointer to the db system setting interface.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    5/11/2011: Arun S - created
 *
 ****************************************************************/
fbe_status_t fbe_database_config_table_get_free_global_info_entry(database_global_info_entry_t *in_global_info_table_ptr, 
                                                                  database_table_size_t size, 
                                                                  database_global_info_entry_t **out_global_info_entry_ptr)
{
    database_table_header_t         * header_ptr = NULL;
    database_global_info_entry_t    * current_entry_ptr = NULL;
    fbe_u64_t                         index = 0;

    *out_global_info_entry_ptr = NULL; /* start with NULL */
    current_entry_ptr = in_global_info_table_ptr;

    for(index = 0; index < size; index++)
    {
        if(current_entry_ptr != NULL)
        {
            header_ptr = (database_table_header_t *)current_entry_ptr;
            if(header_ptr->state == DATABASE_CONFIG_ENTRY_INVALID)
            {
                /* this is a free entry, let's return it to the caller */
                *out_global_info_entry_ptr = current_entry_ptr;
                return FBE_STATUS_OK;
            }
        }
        else
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: Table ptr for Global Info is NULL!\n", 
                           __FUNCTION__);

            return FBE_STATUS_GENERIC_FAILURE;
        }
        current_entry_ptr++;
    }

    return FBE_STATUS_GENERIC_FAILURE;
}

fbe_status_t fbe_database_config_table_get_object_entry_by_id(database_table_t *in_table_ptr,
                                                              fbe_object_id_t object_id,
                                                              database_object_entry_t **out_entry_ptr)
{
    if(in_table_ptr->table_type == DATABASE_CONFIG_TABLE_INVALID) {
        *out_entry_ptr = NULL;
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: Table ptr is invalid, table is either not init'd or destroyed!\n", 
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (object_id > in_table_ptr->table_size) {
        *out_entry_ptr = NULL;
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: object id:%d bigger than table size:%d\n", 
                       __FUNCTION__, object_id,in_table_ptr->table_size );

        return FBE_STATUS_GENERIC_FAILURE;
    }

    *out_entry_ptr = &in_table_ptr->table_content.object_entry_ptr[object_id];
    return FBE_STATUS_OK;
}

fbe_status_t fbe_database_config_table_get_user_entry_by_object_id(database_table_t *in_table_ptr,
                                                              fbe_object_id_t object_id,
                                                              database_user_entry_t **out_entry_ptr)
{
    if(in_table_ptr->table_type == DATABASE_CONFIG_TABLE_INVALID || object_id >= in_table_ptr->table_size ) {
        *out_entry_ptr = NULL;
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: Table ptr (%d) or object id (0x%X) invalid\n", 
                       __FUNCTION__, in_table_ptr->table_type, object_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *out_entry_ptr = &in_table_ptr->table_content.user_entry_ptr[object_id];
    return FBE_STATUS_OK;
}

fbe_status_t fbe_database_config_table_get_edge_entry(database_table_t *in_table_ptr,
                                                      fbe_object_id_t object_id,
                                                      fbe_edge_index_t client_index,
                                                      database_edge_entry_t **out_entry_ptr)
{
    fbe_status_t status;
    database_edge_entry_t *in_entry_ptr = in_table_ptr->table_content.edge_entry_ptr;
    fbe_u32_t entry_index = object_id*DATABASE_MAX_EDGE_PER_OBJECT + client_index;

    if(in_table_ptr->table_type == DATABASE_CONFIG_TABLE_INVALID) {
        *out_entry_ptr = NULL;
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: Table ptr is invalid, table is either not init'd or destroyed!\n", 
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (entry_index < in_table_ptr->table_size)
    {
        *out_entry_ptr = &in_entry_ptr[object_id*DATABASE_MAX_EDGE_PER_OBJECT + client_index];
        status = FBE_STATUS_OK;
    }
    else
    {
        *out_entry_ptr = NULL;
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    return (status);
}

/*!***************************************************************
 * @fn fbe_database_config_table_get_global_info_entry
 *****************************************************************
 * @brief
 *   This function gets the system setting entry from the
 *   database table.
 *
 * @param in_table_ptr - pointer to the database table
 * @param type - Type of the Global Info.
 * @param out_entry_ptr - pointer to the database system setting interface.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    5/11/2011: Arun S - created
 *
 ****************************************************************/
fbe_status_t fbe_database_config_table_get_global_info_entry(database_table_t *in_table_ptr, 
                                                             database_global_info_type_t type, 
                                                             database_global_info_entry_t **out_entry_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t index = 0;
    database_table_header_t * header_ptr = NULL;
    database_global_info_entry_t *global_info_ptr = NULL;

    if(in_table_ptr->table_type == DATABASE_CONFIG_TABLE_INVALID) {
        *out_entry_ptr = NULL;
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: Table ptr is invalid, table is either not init'd or destroyed!\n", 
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    global_info_ptr = in_table_ptr->table_content.global_info_ptr;

    *out_entry_ptr = NULL; /* start with NULL */
    for(index = 0; index < in_table_ptr->table_size; index++)
    {
        if(global_info_ptr != NULL)
        {
            header_ptr = (database_table_header_t *)global_info_ptr;
            if(((header_ptr->state == DATABASE_CONFIG_ENTRY_VALID)||
                (header_ptr->state == DATABASE_CONFIG_ENTRY_UNCOMMITTED))
               &&(global_info_ptr->type == type))
            {
                *out_entry_ptr = global_info_ptr;	
                return FBE_STATUS_OK;
            }
        }
        else
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: Global Info ptr is NULL!\n", 
                           __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        global_info_ptr++;
    }

    if((status == FBE_STATUS_OK)&&(out_entry_ptr == NULL))
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: get global info entry failed!\n", 
                       __FUNCTION__);
    }

    return (status);
}

/*!***************************************************************
 * @fn fbe_database_config_table_get_system_spare_entry
 *****************************************************************
 * @brief
 *   This function gets the system_spare entry from the
 *   database table.
 *
 * @param in_table_ptr - pointer to the database table
 * @param object_id - for now we use object-id to find the entry in this table
 * @param out_entry_ptr - pointer to the database system setting interface.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    9/15/2011: created
 *
 ****************************************************************/
fbe_status_t fbe_database_config_table_get_system_spare_entry(database_table_t *in_table_ptr, 
                                                              fbe_object_id_t object_id, 
                                                              database_system_spare_entry_t **out_entry_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t index = 0;
    database_table_header_t * header_ptr = NULL;
    database_system_spare_entry_t *system_spare_ptr = NULL;

    if(in_table_ptr->table_type != DATABASE_CONFIG_TABLE_SYSTEM_SPARE) {
        *out_entry_ptr = NULL;
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: table type 0x%x is not system spare!\n", 
                       __FUNCTION__, in_table_ptr->table_type);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    system_spare_ptr = in_table_ptr->table_content.system_spare_entry_ptr;

    *out_entry_ptr = NULL; /* start with NULL */
    for(index = 0; index < in_table_ptr->table_size; index++)
    {
        if(system_spare_ptr != NULL)
        {
            header_ptr = (database_table_header_t *)system_spare_ptr;
            if((header_ptr->state == DATABASE_CONFIG_ENTRY_VALID)
               &&(system_spare_ptr->header.object_id == object_id))
            {
                *out_entry_ptr = system_spare_ptr;	
                return FBE_STATUS_OK;
            }
        }
        else
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: system_spare_ptr is NULL!\n", 
                           __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        system_spare_ptr++;
    }

    return (status);
}


fbe_status_t fbe_database_config_table_get_object_entry_by_pvd_SN(database_object_entry_t *in_table_ptr,
                                                                  database_table_size_t size, 
                                                                  fbe_u8_t *SN,
                                                                  database_object_entry_t **out_entry_ptr)
{
    database_table_header_t * header_ptr = NULL;
    database_object_entry_t * current_entry_ptr = NULL;
    fbe_u64_t               index = 0;
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;

    *out_entry_ptr = NULL; /* start with NULL */
    current_entry_ptr = in_table_ptr;
    for(index = 0; index < size; index++) {
        if(current_entry_ptr != NULL) {
            header_ptr = (database_table_header_t *)current_entry_ptr;
            if((header_ptr->state == DATABASE_CONFIG_ENTRY_VALID) &&
               (current_entry_ptr->db_class_id == DATABASE_CLASS_ID_PROVISION_DRIVE)){
                /* this is valid PVD entry */
                if(fbe_compare_string(current_entry_ptr->set_config_union.pvd_config.serial_num, 
                                      sizeof(fbe_u8_t)*FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE,
                                      SN, 
                                      sizeof(fbe_u8_t)*FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE,
                                      FBE_FALSE) == 0) {
                    *out_entry_ptr = current_entry_ptr;
                    status = FBE_STATUS_OK;
                    break;
                }
            }
        }
        else{
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: Table ptr is NULL!\n", 
                           __FUNCTION__);
            break;
        }
        current_entry_ptr++;
    }
    return status;
}

/* This function is for Homewrecker,
  * After a system drive and a user drive are swapped when array offline, array boot up.
  * System PVD would be created hard coded and connect to LDO by slot, then in-memory 
  * database table entries get updated, all of those operation are applied to a invalid system
  * drive, that drive was a user drive of this array, Homewrecker does not accept this drive before 
  * database do these.
  * And when Homewrecker try to find the PVD info in database, it will find database says
  * it is a system drive, obviously, it is wrong. It should be a user drive.
  *
  * I have not so much choice, so I write another function to find the last PVD has the same serial
  * number in database entry, because, under any situation it return the correct value. But a condition
  * must meet, that is this drive was really in this array before.
  * If a new drive inserted into system slot when array offline, database also return wrong result. Database
  * can find a entry relates to this LDO, however, this drive should not be in the database, it is a new drive for
  * Homewrecker, for array. Until Homewrecker accept this drive entering array, promote this drive to a legal
  * drive for this array, then it should be in database.
  * Homewrecker can make up this database by-design bug, via persist hw_fru_descriptor to system drive and 
  * check drive type by this on-disk data by itself.
  */


fbe_status_t fbe_database_config_table_get_last_object_entry_by_pvd_SN(database_object_entry_t *in_table_ptr,
                                                                  database_table_size_t size, 
                                                                  fbe_u8_t *SN,
                                                                  database_object_entry_t **out_entry_ptr)
{
    fbe_u32_t out_num_drive_entries;
    return fbe_database_config_table_get_last_object_entry_by_pvd_SN_or_max_entries(in_table_ptr, size, SN, out_entry_ptr, &out_num_drive_entries);
}


fbe_status_t fbe_database_config_table_get_last_object_entry_by_pvd_SN_or_max_entries(database_object_entry_t *in_table_ptr,
                                                                  database_table_size_t size, 
                                                                  fbe_u8_t *SN,
                                                                  database_object_entry_t **out_entry_ptr,
                                                                  fbe_u32_t *out_num_drive_entries)
{
    database_table_header_t * header_ptr = NULL;
    database_object_entry_t * current_entry_ptr = NULL;
    fbe_u64_t               index = 0;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_u32_t               num_drive_entries = 0;

    if(NULL == out_entry_ptr || NULL == in_table_ptr || NULL == out_num_drive_entries)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
               "%s: input parmeter is NULL.\n", 
               __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *out_entry_ptr = NULL; /* start with NULL */
    *out_num_drive_entries = 0;

    current_entry_ptr = in_table_ptr;
    for(index = 0; index < size; index++) {
        
        if(current_entry_ptr != NULL) 
        {
            header_ptr = (database_table_header_t *)current_entry_ptr;

            /* count num PVDs until we find a match. */
            if (header_ptr->state != DATABASE_CONFIG_ENTRY_INVALID &&
                current_entry_ptr->db_class_id == DATABASE_CLASS_ID_PROVISION_DRIVE)
            {
                num_drive_entries++;
            }
            
            if((header_ptr->state == DATABASE_CONFIG_ENTRY_VALID) &&
               (current_entry_ptr->db_class_id == DATABASE_CLASS_ID_PROVISION_DRIVE))
            {
                /* this is valid PVD entry */
                if(fbe_compare_string(current_entry_ptr->set_config_union.pvd_config.serial_num, 
                                      sizeof(fbe_u8_t)*FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE,
                                      SN, 
                                      sizeof(fbe_u8_t)*FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE,
                                      FBE_FALSE) == 0) {
                    *out_entry_ptr = current_entry_ptr;
                    status = FBE_STATUS_OK;
                }
            }
            
        }
        else
        {
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
        }
        current_entry_ptr++;
    }

    *out_num_drive_entries = num_drive_entries;   /* only set when entry not found */
    return status;
}


fbe_status_t fbe_database_config_table_get_num_pvds(database_object_entry_t *in_table_ptr,
                                                    database_table_size_t size, 
                                                    fbe_u32_t *out_num_pvds)
{
    database_table_header_t * header_ptr = NULL;
    database_object_entry_t * current_entry_ptr = NULL;
    fbe_u64_t               index = 0;
    fbe_status_t            status = FBE_STATUS_OK;

    if(NULL == in_table_ptr || NULL == out_num_pvds)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
               "%s: input parmeter is NULL.\n", 
               __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *out_num_pvds = 0;

    current_entry_ptr = in_table_ptr;
    for(index = 0; index < size; index++) {

        if(current_entry_ptr != NULL) 
        {
            header_ptr = (database_table_header_t *)current_entry_ptr;

            /* count num PVDs until we find a match. */
            if (header_ptr->state != DATABASE_CONFIG_ENTRY_INVALID &&
                current_entry_ptr->db_class_id == DATABASE_CLASS_ID_PROVISION_DRIVE)
            {
                (*out_num_pvds)++;
            }
        }
        else
        {
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
        }
        current_entry_ptr++;
    }
    return status;
}

fbe_status_t fbe_database_config_table_get_user_entry_by_rg_id(database_table_t *in_table_ptr, 
                                                               fbe_raid_group_number_t raid_group_number,
                                                               database_user_entry_t **out_entry_ptr)
{
    database_table_header_t * header_ptr = NULL;
    database_user_entry_t * current_entry_ptr = NULL;
    fbe_u64_t               index = 0;

    if(in_table_ptr->table_type == DATABASE_CONFIG_TABLE_INVALID) {
        *out_entry_ptr = NULL;
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: Table ptr is invalid, table is either not init'd or destroyed!\n", 
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *out_entry_ptr = NULL; /* start with NULL */
    current_entry_ptr = in_table_ptr->table_content.user_entry_ptr;
    for(index = 0; index < in_table_ptr->table_size; index++) {
        if(current_entry_ptr != NULL) {
            header_ptr = (database_table_header_t *)current_entry_ptr;
            if((header_ptr->state == DATABASE_CONFIG_ENTRY_VALID)
               &&(current_entry_ptr->db_class_id > DATABASE_CLASS_ID_RAID_START)
               &&(current_entry_ptr->db_class_id < DATABASE_CLASS_ID_RAID_END)){
                /* this is valid RG entry */
                if(current_entry_ptr->user_data_union.rg_user_data.raid_group_number == raid_group_number) {
                    *out_entry_ptr = current_entry_ptr;
                    return FBE_STATUS_OK;
                }
            }
        }
        else{
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: Table ptr is NULL!\n", 
                           __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        current_entry_ptr++;
    }
    return FBE_STATUS_GENERIC_FAILURE;
}

fbe_status_t fbe_database_config_table_get_user_entry_by_lun_id(database_table_t *in_table_ptr, 
                                                               fbe_lun_number_t lun_number,
                                                               database_user_entry_t **out_entry_ptr)
{
    database_table_header_t * header_ptr = NULL;
    database_user_entry_t * current_entry_ptr = NULL;
    fbe_u64_t               index = 0;

    *out_entry_ptr = NULL; /* start with NULL */
    current_entry_ptr = in_table_ptr->table_content.user_entry_ptr;
    for(index = 0; index < in_table_ptr->table_size; index++) {
        if(current_entry_ptr != NULL) {				
            header_ptr = (database_table_header_t *)current_entry_ptr;
            if((header_ptr->state == DATABASE_CONFIG_ENTRY_VALID)
               &&(current_entry_ptr->db_class_id == DATABASE_CLASS_ID_LUN)){
                /* this is valid lun entry */					               
                if(current_entry_ptr->user_data_union.lu_user_data.lun_number == lun_number) {
                    *out_entry_ptr = current_entry_ptr;					
                    return FBE_STATUS_OK;
                }
            }
        }
        else{
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: Table ptr is NULL!\n", 
                           __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        current_entry_ptr++;
    }
    return FBE_STATUS_GENERIC_FAILURE;
}

fbe_status_t fbe_database_config_table_get_user_entry_by_ext_pool_id(database_table_t *in_table_ptr, 
                                                                     fbe_u32_t pool_id,
                                                                     database_user_entry_t **out_entry_ptr)
{
    database_table_header_t * header_ptr = NULL;
    database_user_entry_t * current_entry_ptr = NULL;
    fbe_u64_t               index = 0;

    if(in_table_ptr->table_type == DATABASE_CONFIG_TABLE_INVALID) {
        *out_entry_ptr = NULL;
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: Table ptr is invalid, table is either not init'd or destroyed!\n", 
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *out_entry_ptr = NULL; /* start with NULL */
    current_entry_ptr = in_table_ptr->table_content.user_entry_ptr;
    for(index = 0; index < in_table_ptr->table_size; index++) {
        if(current_entry_ptr != NULL) {
            header_ptr = (database_table_header_t *)current_entry_ptr;
            if((header_ptr->state == DATABASE_CONFIG_ENTRY_VALID)
               &&(current_entry_ptr->db_class_id == DATABASE_CLASS_ID_EXTENT_POOL)){
                /* this is valid RG entry */
                if(current_entry_ptr->user_data_union.ext_pool_user_data.pool_id == pool_id) {
                    *out_entry_ptr = current_entry_ptr;
                    return FBE_STATUS_OK;
                }
            }
        }
        else{
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: Table ptr is NULL!\n", 
                           __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        current_entry_ptr++;
    }
    return FBE_STATUS_GENERIC_FAILURE;
}

fbe_status_t fbe_database_config_table_get_user_entry_by_ext_pool_lun_id(database_table_t *in_table_ptr, 
                                                                         fbe_u32_t pool_number,
                                                                         fbe_lun_number_t lun_number,
                                                                         database_user_entry_t **out_entry_ptr)
{
    database_table_header_t * header_ptr = NULL;
    database_user_entry_t * current_entry_ptr = NULL;
    fbe_u64_t               index = 0;

    *out_entry_ptr = NULL; /* start with NULL */
    current_entry_ptr = in_table_ptr->table_content.user_entry_ptr;
    for(index = 0; index < in_table_ptr->table_size; index++) {
        if(current_entry_ptr != NULL) {				
            header_ptr = (database_table_header_t *)current_entry_ptr;
            if((header_ptr->state == DATABASE_CONFIG_ENTRY_VALID)
               &&((current_entry_ptr->db_class_id == DATABASE_CLASS_ID_EXTENT_POOL_LUN) || 
                  (current_entry_ptr->db_class_id == DATABASE_CLASS_ID_EXTENT_POOL_METADATA_LUN))){
                /* this is valid lun entry */					               
                if ((pool_number == FBE_POOL_ID_INVALID || current_entry_ptr->user_data_union.ext_pool_lun_user_data.pool_id == pool_number) &&
                    (current_entry_ptr->user_data_union.ext_pool_lun_user_data.lun_id == lun_number)) {
                    *out_entry_ptr = current_entry_ptr;					
                    return FBE_STATUS_OK;
                }
            }
        }
        else{
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: Table ptr is NULL!\n", 
                           __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        current_entry_ptr++;
    }
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!***************************************************************
 * @fn fbe_database_config_table_update_global_info_entry
 *****************************************************************
 * @brief
 *   This function updates the system setting entry in the
 *   database table. 
 *
 * @param in_table_ptr - pointer to the database table
 * @param in_entry_ptr - pointer to the db system setting interface.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    5/11/2011: Arun S - created
 *    4/25/2012: Gaoh1 - modified
 *
 ****************************************************************/
fbe_status_t fbe_database_config_table_update_global_info_entry(database_table_t *in_table_ptr, 
                                                                database_global_info_entry_t *in_entry_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    database_global_info_entry_t *out_entry_ptr = NULL;
    fbe_u32_t entry_size = 0, local_entry_size = 0;

    /* Check the size(loaded from disk), if the size is bigger than the size in software, 
     * report error and enter into service mode. The reson for that is: 
     * if the size in disk is bigger than size in software, 
     * the software writes a smaller size of data to disk, 
     * which may erase the data bigger than the software size
    */
    entry_size = in_entry_ptr->header.version_header.size;
    /* can't check zero, because the first global info entry is a NULL entry */
    /*
    if (entry_size == 0) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: entry's size is zero\n", 
                       __FUNCTION__);
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }
    */
    
    local_entry_size = database_common_global_info_entry_size(in_entry_ptr->type);
    
    if (entry_size > local_entry_size) {
		database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: the entry in software is smaller than the entry in disk or peer, entry degraded mode\n", 
                       __FUNCTION__);
		fbe_event_log_write(SEP_ERROR_CODE_NEWER_CONFIG_FROM_PEER,
												NULL, 0, "%x %x",
												entry_size, local_entry_size);
		fbe_database_enter_service_mode(FBE_DATABASE_SERVICE_MODE_REASON_PROBLEMATIC_DATABASE_VERSION);
		return FBE_STATUS_SERVICE_MODE;  
    }

    status = fbe_database_config_table_get_global_info_entry(in_table_ptr, 
                                                             in_entry_ptr->type, 
                                                             &out_entry_ptr);

    if((status != FBE_STATUS_OK)||(out_entry_ptr == NULL)) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: get global info entry failed!\n", 
                       __FUNCTION__);
        status = FBE_STATUS_GENERIC_FAILURE; 
    }
    else
    {
        database_common_init_global_info_entry(out_entry_ptr);
        fbe_copy_memory(out_entry_ptr, in_entry_ptr, FBE_MIN(entry_size, local_entry_size));
        out_entry_ptr->header.state = DATABASE_CONFIG_ENTRY_VALID;
    }

    return status;
}

fbe_status_t fbe_database_config_table_update_system_spare_entry(database_table_t *in_table_ptr, 
                                                                database_system_spare_entry_t *in_entry_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    database_system_spare_entry_t *out_entry_ptr = NULL;
    fbe_u32_t entry_size = 0;

    /* Check the size(loaded from disk), if the size is bigger than the size in software, 
     * report error and enter into service mode. The reson for that is: 
     * if the size in disk is bigger than size in software, 
     * the software writes a smaller size of data to disk, 
     * which may erase the data bigger than the software size
    */
    entry_size = in_entry_ptr->header.version_header.size;
    if (entry_size == 0) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: entry's size is zero\n", 
                       __FUNCTION__);
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }

    status = fbe_database_config_table_get_system_spare_entry(in_table_ptr, 
                                                             in_entry_ptr->header.object_id, 
                                                             &out_entry_ptr);

    if((status != FBE_STATUS_OK)||(out_entry_ptr == NULL)) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: get system_spare entry failed!\n", 
                       __FUNCTION__);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        database_common_init_system_spare_entry(out_entry_ptr);
        fbe_copy_memory(out_entry_ptr, in_entry_ptr, entry_size);
        out_entry_ptr->header.state = DATABASE_CONFIG_ENTRY_VALID;
    }

    return status;
}


fbe_status_t fbe_database_config_table_update_user_entry(database_table_t *in_table_ptr,
                                                         database_user_entry_t *in_entry_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    database_user_entry_t *out_entry_ptr = NULL;
    fbe_u32_t entry_size = 0, local_entry_size = 0;

    /* Check the size(loaded from disk), if the size is bigger than the size in software, 
     * report error and enter into service mode. The reson for that is: 
     * if the size in disk is bigger than size in software, 
     * the software writes a smaller size of data to disk, 
     * which may erase the data bigger than the software size
    */
    entry_size = in_entry_ptr->header.version_header.size;
    if (entry_size == 0) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: user entry's size is zero\n", 
                       __FUNCTION__);
        fbe_event_log_write(SEP_ERROR_CODE_SYSTEM_DB_CONTENT_CORRUPTED,
									NULL, 0, NULL);
        /* We go to degraded mode if the version_header size of user entry is 0.*/
        fbe_database_enter_service_mode(FBE_DATABASE_SERVICE_MODE_REASON_INTERNAL_OBJS);
        
        status = FBE_STATUS_SERVICE_MODE;
        return status;
    }

    if(in_entry_ptr->db_class_id == DATABASE_CLASS_ID_INVALID)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: the class_id of the user_entry is invalid.\n", 
                       __FUNCTION__);
        fbe_event_log_write(SEP_ERROR_CODE_SYSTEM_DB_CONTENT_CORRUPTED,
						    NULL, 0, NULL);
        /* We go to degraded mode if the class id of user entry is invalid.*/
        fbe_database_enter_service_mode(FBE_DATABASE_SERVICE_MODE_REASON_INTERNAL_OBJS);
        
        status = FBE_STATUS_SERVICE_MODE;
        return status;
    }

    local_entry_size = database_common_user_entry_size(in_entry_ptr->db_class_id);

    if (entry_size > local_entry_size) {
		database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: the entry in software is smaller than the entry in disk or peer, entry degraded mode\n", 
                       __FUNCTION__);
		fbe_event_log_write(SEP_ERROR_CODE_NEWER_CONFIG_FROM_PEER,
												NULL, 0, "%x %x",
												entry_size, local_entry_size);
		fbe_database_enter_service_mode(FBE_DATABASE_SERVICE_MODE_REASON_PROBLEMATIC_DATABASE_VERSION);
		return FBE_STATUS_SERVICE_MODE;        
    }

    fbe_spinlock_lock(&in_table_ptr->table_lock);
    status = fbe_database_config_table_get_user_entry_by_object_id(in_table_ptr,
                                                          in_entry_ptr->header.object_id,
                                                          &out_entry_ptr);
    if((status != FBE_STATUS_OK)||(out_entry_ptr == NULL)) {
        fbe_spinlock_unlock(&in_table_ptr->table_lock);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: get user entry failed!\n", 
                       __FUNCTION__);
        return status;
    }

    database_common_init_user_entry(out_entry_ptr);
    fbe_copy_memory(out_entry_ptr, in_entry_ptr, entry_size);
    out_entry_ptr->header.state = DATABASE_CONFIG_ENTRY_VALID;
    fbe_spinlock_unlock(&in_table_ptr->table_lock);
    return status;
}

fbe_status_t fbe_database_config_table_update_edge_entry(database_table_t *in_table_ptr,
                                                         database_edge_entry_t *in_entry_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    database_edge_entry_t *out_entry_ptr = NULL;
    fbe_u32_t entry_size = 0, local_entry_size = 0;

    /* Check the size(loaded from disk), if the size is bigger than the size in software, 
     * report error and enter into service mode. The reson for that is: 
     * if the size in disk is bigger than size in software, 
     * the software writes a smaller size of data to disk, 
     * which may erase the data bigger than the software size
    */
    entry_size = in_entry_ptr->header.version_header.size;
    if (entry_size == 0) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: edge entry's size is zero\n", 
                       __FUNCTION__);
        fbe_event_log_write(SEP_ERROR_CODE_SYSTEM_DB_CONTENT_CORRUPTED,
									NULL, 0, NULL);
        /* We go to degraded mode if the version_header size of edge entry is 0.*/
        fbe_database_enter_service_mode(FBE_DATABASE_SERVICE_MODE_REASON_INTERNAL_OBJS);
        
        status = FBE_STATUS_SERVICE_MODE;
        return status;
    }
    local_entry_size = database_common_edge_entry_size();
    
	/*If the size value in the entry is bigger than the size of the data structure, enter service mode */
    if (entry_size > local_entry_size) {
		database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: the entry in software is smaller than the entry in disk or peer, entry degraded mode\n", 
                       __FUNCTION__);
		fbe_event_log_write(SEP_ERROR_CODE_NEWER_CONFIG_FROM_PEER,
												NULL, 0, "%x %x",
												entry_size, local_entry_size);
		fbe_database_enter_service_mode(FBE_DATABASE_SERVICE_MODE_REASON_PROBLEMATIC_DATABASE_VERSION);
		return FBE_STATUS_SERVICE_MODE;
	}

    fbe_spinlock_lock(&in_table_ptr->table_lock);

    status = fbe_database_config_table_get_edge_entry(in_table_ptr,
                                                      in_entry_ptr->header.object_id,
                                                      in_entry_ptr->client_index,
                                                      &out_entry_ptr);

    if((status != FBE_STATUS_OK)||(out_entry_ptr == NULL)) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: get edge entry failed!\n", 
                       __FUNCTION__);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        database_common_init_edge_entry(out_entry_ptr);
        fbe_copy_memory(out_entry_ptr, in_entry_ptr, entry_size);
        out_entry_ptr->header.state = DATABASE_CONFIG_ENTRY_VALID;
    }
    fbe_spinlock_unlock(&in_table_ptr->table_lock);
    return status;
}

fbe_status_t fbe_database_config_table_update_object_entry(database_table_t *in_table_ptr,
                                                           database_object_entry_t *in_entry_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    database_object_entry_t *out_entry_ptr = NULL;
    fbe_u32_t entry_size = 0, local_entry_size = 0;

    /* Check the size(loaded from disk), if the size is bigger than the size in software, 
     * report error and enter into service mode. The reson for that is: 
     * if the size in disk is bigger than size in software, 
     * the software writes a smaller size of data to disk, 
     * which may erase the data bigger than the software size
    */
    entry_size = in_entry_ptr->header.version_header.size;
    if (entry_size == 0) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: object entry's size is zero\n", 
                       __FUNCTION__);
        fbe_event_log_write(SEP_ERROR_CODE_SYSTEM_DB_CONTENT_CORRUPTED,
									NULL, 0, NULL);
        /* We go to degraded mode if the version_header size of object entry is 0.*/
        fbe_database_enter_service_mode(FBE_DATABASE_SERVICE_MODE_REASON_INTERNAL_OBJS);
        
        status = FBE_STATUS_SERVICE_MODE;
        return status;
    }
    
    if(in_entry_ptr->db_class_id == DATABASE_CLASS_ID_INVALID)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: the class_id of the object_entry is invalid.\n", 
                       __FUNCTION__);
        fbe_event_log_write(SEP_ERROR_CODE_SYSTEM_DB_CONTENT_CORRUPTED,
									NULL, 0, NULL);
        /* We go to degraded mode if the class id of object entry is invalid.*/
        fbe_database_enter_service_mode(FBE_DATABASE_SERVICE_MODE_REASON_INTERNAL_OBJS);
        
        status = FBE_STATUS_SERVICE_MODE;
        return status;
    }

    local_entry_size = database_common_object_entry_size(in_entry_ptr->db_class_id);
    
    if (entry_size > local_entry_size) {
		database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: the entry in software is smaller than the entry in disk or peer, entry degraded mode\n", 
                       __FUNCTION__);
		fbe_event_log_write(SEP_ERROR_CODE_NEWER_CONFIG_FROM_PEER,
												NULL, 0, "%x %x",
												entry_size, local_entry_size);
		fbe_database_enter_service_mode(FBE_DATABASE_SERVICE_MODE_REASON_PROBLEMATIC_DATABASE_VERSION);
		return FBE_STATUS_SERVICE_MODE;

    }

    fbe_spinlock_lock(&in_table_ptr->table_lock);
    status = fbe_database_config_table_get_object_entry_by_id(in_table_ptr,
                                                              in_entry_ptr->header.object_id,
                                                              &out_entry_ptr);
    if((status != FBE_STATUS_OK)||(out_entry_ptr == NULL)) 
    {
        fbe_spinlock_unlock(&in_table_ptr->table_lock);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: get object entry failed!\n", 
                       __FUNCTION__);
        return status;
    }

    database_common_init_object_entry(out_entry_ptr);     
    fbe_copy_memory(out_entry_ptr, in_entry_ptr, FBE_MIN(entry_size, local_entry_size));
    out_entry_ptr->header.state = DATABASE_CONFIG_ENTRY_VALID;
    fbe_spinlock_unlock(&in_table_ptr->table_lock);
    return status;
}

/*!***************************************************************
 * @fn fbe_database_config_table_remove_global_info_entry
 *****************************************************************
 * @brief
 *   This function removes the system setting entry from the
 *   database table. 
 *
 * @param in_table_ptr - pointer to the database table
 * @param database_global_info_entry_t - pointer to the
 * database system setting entry interface.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    5/11/2011: Arun S - created
 *
 ****************************************************************/
fbe_status_t fbe_database_config_table_remove_global_info_entry(database_table_t *in_table_ptr, 
                                                                database_global_info_entry_t *in_entry_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    database_global_info_entry_t *out_entry_ptr = NULL;

    fbe_spinlock_lock(&in_table_ptr->table_lock);

    status = fbe_database_config_table_get_global_info_entry(in_table_ptr, 
                                                             in_entry_ptr->type, 
                                                             &out_entry_ptr);

    if((status != FBE_STATUS_OK)||(out_entry_ptr == NULL)) 
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: remove system setting entry failed!\n", 
                       __FUNCTION__);

        status = FBE_STATUS_GENERIC_FAILURE;
    }  
    else
    {
        fbe_zero_memory(out_entry_ptr, sizeof(database_global_info_entry_t));
    }

    fbe_spinlock_unlock(&in_table_ptr->table_lock);

    return status;
}

fbe_status_t fbe_database_config_table_remove_object_entry(database_table_t *in_table_ptr, 
                                                           database_object_entry_t *in_entry_ptr)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    database_object_entry_t *out_entry_ptr = NULL;

    fbe_spinlock_lock(&in_table_ptr->table_lock);
    status = fbe_database_config_table_get_object_entry_by_id(in_table_ptr,
                                                            in_entry_ptr->header.object_id,
                                                            &out_entry_ptr);
    if((status != FBE_STATUS_OK)||(out_entry_ptr == NULL)) {
        fbe_spinlock_unlock(&in_table_ptr->table_lock);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: get object entry failed!\n", 
            __FUNCTION__);
        return status;
    }  
    fbe_zero_memory(out_entry_ptr, sizeof(database_object_entry_t));
    fbe_spinlock_unlock(&in_table_ptr->table_lock);
    return status;
}

fbe_status_t fbe_database_config_table_remove_edge_entry(database_table_t *in_table_ptr, 
                                                         database_edge_entry_t *in_entry_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    database_edge_entry_t *out_entry_ptr = NULL;

    fbe_spinlock_lock(&in_table_ptr->table_lock);

    status = fbe_database_config_table_get_edge_entry(in_table_ptr,
                                                      in_entry_ptr->header.object_id,
                                                      in_entry_ptr->client_index,
                                                      &out_entry_ptr);
    if((status != FBE_STATUS_OK)||(out_entry_ptr == NULL)) 
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: remove edge entry failed!\n", 
                       __FUNCTION__);
        status = FBE_STATUS_GENERIC_FAILURE;
    }  
    else
    {
        fbe_zero_memory(out_entry_ptr, sizeof(database_edge_entry_t));
    }

    fbe_spinlock_unlock(&in_table_ptr->table_lock);

    return status;
}


fbe_status_t fbe_database_config_table_remove_user_entry(database_table_t *in_table_ptr,
                                                         database_user_entry_t *in_entry_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    database_user_entry_t *out_entry_ptr = NULL;

    fbe_spinlock_lock(&in_table_ptr->table_lock);
    status = fbe_database_config_table_get_user_entry_by_object_id(in_table_ptr,
                                                                   in_entry_ptr->header.object_id,
                                                                   &out_entry_ptr);
    if((status != FBE_STATUS_OK)||(out_entry_ptr == NULL)) 
    {
        fbe_spinlock_unlock(&in_table_ptr->table_lock);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: remove user entry failed!\n", 
                       __FUNCTION__);
        return status;
    }  
    fbe_zero_memory(out_entry_ptr, sizeof(database_user_entry_t));
    fbe_spinlock_unlock(&in_table_ptr->table_lock);
    
    return status;
}
/*!**************************************************************************
 * @file fbe_database_config_table_copy_edge_entries
 ***************************************************************************
 *
 * @brief
 *  Copies all the edges from Service table back to transaction table for 
 *  the given object id
 * @parameter - object id
 *
 ***************************************************************************/
fbe_status_t fbe_database_config_table_copy_edge_entries(database_table_t *in_table_ptr,
                                                         fbe_object_id_t object_id,
                                                         database_transaction_t *transaction)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    database_edge_entry_t    *out_entry_ptr = NULL;
    database_edge_entry_t    *free_entry = NULL;
    fbe_edge_index_t          index = 0;

    fbe_spinlock_lock(&in_table_ptr->table_lock);

    status = fbe_database_config_table_get_edge_entry(in_table_ptr, object_id, 0, &out_entry_ptr);
    if ((status != FBE_STATUS_OK)||(out_entry_ptr == NULL)) {
        fbe_spinlock_unlock(&in_table_ptr->table_lock);
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: failed to get edge entry of obj 0x%x!\n",
            __FUNCTION__, object_id);
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }

    for(index = 0; index < DATABASE_MAX_EDGE_PER_OBJECT; index++)
    {		
        /* get the entry ok */
        if (out_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID) {
            status = fbe_database_transaction_get_free_edge_entry(transaction, &free_entry);
            if ((status != FBE_STATUS_OK)||(free_entry == NULL))
            {
                database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: Can't get free edge entry!\n",
                    __FUNCTION__);
                status = FBE_STATUS_GENERIC_FAILURE;
                break;  // we need to unlock the table before return
            }
            /* we got a free entry from transaction edge table, fill it with Service table edge entry */				
            fbe_copy_memory(free_entry, out_entry_ptr, sizeof(database_edge_entry_t));
            free_entry->header.state = DATABASE_CONFIG_ENTRY_DESTROY;
        }
        /* move on to the next entry */
        out_entry_ptr++;
    }

    fbe_spinlock_unlock(&in_table_ptr->table_lock);

    return status;
}

fbe_status_t fbe_database_config_table_get_count_by_db_class(database_table_t *in_table_ptr,
                                                             database_class_id_t class_id,
                                                             fbe_u32_t *count)
{
    database_table_header_t * header_ptr = NULL;
    database_user_entry_t * current_entry_ptr = NULL;
    fbe_u64_t               index = 0;
    fbe_u32_t				current_count = 0;

    current_entry_ptr = in_table_ptr->table_content.user_entry_ptr;
    for(index = 0; index < in_table_ptr->table_size; index++) {
        if(current_entry_ptr != NULL) {				
            header_ptr = (database_table_header_t *)current_entry_ptr;
            if(((header_ptr->state == DATABASE_CONFIG_ENTRY_VALID) &&(current_entry_ptr->db_class_id == class_id)) ||
               ((class_id == DATABASE_CLASS_ID_RAID_START) && (header_ptr->state == DATABASE_CONFIG_ENTRY_VALID) && 
                (current_entry_ptr->db_class_id > DATABASE_CLASS_ID_RAID_START && current_entry_ptr->db_class_id < DATABASE_CLASS_ID_RAID_END) &&
                (FBE_RAID_ID_INVALID != current_entry_ptr->user_data_union.rg_user_data.raid_group_number) )){
                /* this is valid entry ( we don't count RG which are PSL or the mirror part of a RAID 10)*/					               
                current_count++;
            }
        }
        else{
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: Table ptr is NULL!\n", 
                           __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        current_entry_ptr++;
    }

    *count = current_count;
    return FBE_STATUS_OK;

}

fbe_status_t fbe_database_config_destroy_all_objects_of_class(database_class_id_t class_id,
                                                              fbe_database_service_t *database_sefvice_ptr,
                                                              fbe_bool_t internal_obj_only)
{
    database_table_t *			               table_ptr = &database_sefvice_ptr->object_table;
    database_object_entry_t * 	               current_entry_ptr = NULL;
    database_table_header_t * 	               header_ptr = NULL;
    fbe_u32_t               	               index = 0;
    fbe_status_t				               status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t	                               wait_count;
    fbe_topology_mgmt_check_object_existent_t  check_object_existent;
    fbe_u32_t               	               start, end;
    //fbe_private_space_layout_lun_info_t        lun_info;
    fbe_object_id_t                            last_system_object_id;

    current_entry_ptr = table_ptr->table_content.object_entry_ptr;/*point to start*/
    if(current_entry_ptr == NULL) 
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "db_cfg_destroy_all_objects: Table ptr is NULL!\n");
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    fbe_database_get_last_system_object_id(&last_system_object_id);
    if(internal_obj_only) {
        start   = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FIRST;
        end     = last_system_object_id + 1;
    } else {
        start   = last_system_object_id + 1;
        end     = table_ptr->table_size;
    }

    current_entry_ptr += start; /* adjust the start */

	/*first of all, let ask all the objects for this class to destroy themselvs*/
    for(index = start; index < end; index++){
        header_ptr = (database_table_header_t *)current_entry_ptr;
        if((header_ptr->state == DATABASE_CONFIG_ENTRY_VALID) && (current_entry_ptr->db_class_id == class_id)){
            /* this is valid entry and has an object there, let's ask it to set destroy condition*/					               
            status = fbe_database_send_packet_to_object(FBE_BASE_OBJECT_CONTROL_CODE_SET_DESTROY_COND,
                                                         NULL,
                                                         0,
                                                         (fbe_object_id_t)index,
                                                         NULL,  /* no sg list*/
                                                         0,  /* no sg list*/
                                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                                         FBE_PACKAGE_ID_SEP_0);

            if (status != FBE_STATUS_OK && status != FBE_STATUS_NO_OBJECT) {
                database_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "db_cfg_destroy_all_objects: Failed to set destroy cond for Obj: 0x%X, sts=0x%x\n", 
                               (fbe_object_id_t)index, status);
            }
        }  /* end if header_ptr->state case */
        current_entry_ptr++;/*go to next one*/
    } /* end for-loop */


	/*it will be a while for all of them to destroy so let's wait just a bit before we start bugging them*/
	//fbe_thread_delay(2000); 

	current_entry_ptr = table_ptr->table_content.object_entry_ptr;/*point to start*/
    current_entry_ptr += start; /* adjust the start */

	/*now let's goa gain and see if they are still there*/
	for(index = start; index < end; index++) {
        /* initialize wait count */
        wait_count = 0;

        header_ptr = (database_table_header_t *)current_entry_ptr;        
        if((header_ptr->state == DATABASE_CONFIG_ENTRY_VALID) && (current_entry_ptr->db_class_id == class_id)) {
            /* Populate the Object ID and set default exists to TRUE */
            check_object_existent.object_id = (fbe_object_id_t)index;

            /* Wait up to one minute for the object to completely destroy */
            do {
                /* Ask topology to check for object existent */
                status = fbe_database_send_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_CHECK_OBJECT_EXISTENT,
                                             &check_object_existent,
                                             sizeof(fbe_topology_mgmt_check_object_existent_t),
                                             FBE_SERVICE_ID_TOPOLOGY,
                                             NULL,  /* no sg list*/
                                             0,  /* no sg list*/
                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                             FBE_PACKAGE_ID_SEP_0);

                if (check_object_existent.exists_b == FBE_TRUE) {
                    /* Wait 100ms before retry again */
                    fbe_thread_delay(200); 

                    /* Increment wait count */
                    wait_count++;

                    /* Object has not completely destroyed.  Check the object again. */
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                        "db_cfg_destroy_all_objects: Retry-Destroy Obj:=0x%X, exists_b=%d, Sts=0x%x, wait_count=%d\n",
                        (fbe_object_id_t)index, check_object_existent.exists_b, status, wait_count);

                }

            } while ((check_object_existent.exists_b == FBE_TRUE) && (wait_count < 100));

            /* If it still exist log a critical error. */
            if (check_object_existent.exists_b == FBE_TRUE) {
                database_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "db_cfg_destroy_all_objects: Obj:=0x%X still existed. Sts=0x%x, wait_count=%d\n",
                    (fbe_object_id_t)index, status, wait_count);
            }
        }  /* end if header_ptr->state case */

        current_entry_ptr++;/*go to next one*/
    } /* end for-loop */

    return FBE_STATUS_OK;
}


fbe_status_t fbe_database_config_table_get_user_entry_by_lun_wwn(database_table_t *in_table_ptr, 
																 fbe_assigned_wwid_t lun_wwid,
																 database_user_entry_t **out_entry_ptr)
{
	database_table_header_t * header_ptr = NULL;
    database_user_entry_t * current_entry_ptr = NULL;
    fbe_u64_t               index = 0;
	fbe_u32_t				byte_count = 0;
	fbe_u32_t 				match = FBE_TRUE;

    *out_entry_ptr = NULL; /* start with NULL */
    current_entry_ptr = in_table_ptr->table_content.user_entry_ptr;
    for(index = 0; index < in_table_ptr->table_size; index++) {
        if(current_entry_ptr != NULL) {				
            header_ptr = (database_table_header_t *)current_entry_ptr;
            if((header_ptr->state == DATABASE_CONFIG_ENTRY_VALID)
               && (current_entry_ptr->db_class_id == DATABASE_CLASS_ID_LUN || current_entry_ptr->db_class_id == DATABASE_CLASS_ID_EXTENT_POOL_LUN)){
                byte_count = 0;
				match = FBE_TRUE;
				while (byte_count < FBE_WWN_BYTES) {
					if (current_entry_ptr->db_class_id == DATABASE_CLASS_ID_LUN && 
                        current_entry_ptr->user_data_union.lu_user_data.world_wide_name.bytes[byte_count] != lun_wwid.bytes[byte_count]) {
						match = FBE_FALSE;
						break;
					} else if (current_entry_ptr->db_class_id == DATABASE_CLASS_ID_EXTENT_POOL_LUN && 
                               current_entry_ptr->user_data_union.ext_pool_lun_user_data.world_wide_name.bytes[byte_count] != 
                               lun_wwid.bytes[byte_count]) {
						match = FBE_FALSE;
						break;
					}
					byte_count++;
				}

				/* is this a valid WWN entry ?*/	
                if(FBE_IS_TRUE(match)) {
                    *out_entry_ptr = current_entry_ptr;					
                    return FBE_STATUS_OK;
                }
            }
        }
        else{
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: Table ptr is NULL!\n", 
                           __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        current_entry_ptr++;
    }
    return FBE_STATUS_GENERIC_FAILURE;

}

/*!***************************************************************
 * @fn fbe_database_config_table_get_global_info_entry
 *****************************************************************
 * @brief
 *   This function checks to see if global_info table needs commit
 *
 * @param in_table_ptr - pointer to the database table
 *
 * @return fbe_bool_t 
 *
 * @version
 *
 ****************************************************************/
fbe_bool_t fbe_database_config_is_global_info_commit_required(database_table_t *in_table_ptr)
{
    fbe_bool_t commit_required = FBE_FALSE;
    fbe_u32_t index = 0;
    database_table_header_t * header_ptr = NULL;
    database_global_info_entry_t *global_info_ptr = NULL;

    global_info_ptr = in_table_ptr->table_content.global_info_ptr;

    if ((in_table_ptr->table_type != DATABASE_CONFIG_TABLE_GLOBAL_INFO) ||
        (global_info_ptr == NULL)){
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: Table ptr is invalid!\n", 
                       __FUNCTION__);
        return commit_required;
    }

    /* don't have to check the last entry */
    for(index = 0; index < in_table_ptr->table_size - 1; index++)
    {
        header_ptr = (database_table_header_t *)global_info_ptr;
        if(header_ptr->state == DATABASE_CONFIG_ENTRY_UNCOMMITTED)
        {
            commit_required = FBE_TRUE;	
            break;
        }

        global_info_ptr++;
    }

    return (commit_required);
}

/*!***************************************************************
 * @fn fbe_database_config_is_object_table_commit_required
 *****************************************************************
 * @brief
 *   This function checks to see if object table needs commit
 *
 * @param in_table_ptr - pointer to the database table
 *
 * @return fbe_bool_t 
 *
 * @version
 *
 ****************************************************************/
fbe_bool_t fbe_database_config_is_object_table_commit_required(database_table_t *in_table_ptr)
{
    fbe_bool_t commit_required = FBE_FALSE;
    database_object_entry_t *   object_entry_ptr = NULL;

    object_entry_ptr = in_table_ptr->table_content.object_entry_ptr;

    if ((in_table_ptr->table_type != DATABASE_CONFIG_TABLE_OBJECT) ||
        (object_entry_ptr == NULL)){
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: Table ptr is invalid!\n", 
                       __FUNCTION__);
        return commit_required;
    }

    if (object_entry_ptr->header.version_header.size != database_common_object_entry_size(object_entry_ptr->db_class_id))
    {
        commit_required = FBE_TRUE;
    }

    return (commit_required);
}


/*!***************************************************************
 * @fn fbe_database_config_update_global_info_prior_commit
 *****************************************************************
 * @brief
 *   This function updates global info table before commit
 *
 * @param in_table_ptr - pointer to the database table
 *
 * @return fbe_status_t 
 *
 * @version
 *
 ****************************************************************/
fbe_status_t fbe_database_config_update_global_info_prior_commit(database_table_t *in_table_ptr)
{
    fbe_u32_t index = 0;
    database_table_header_t * header_ptr = NULL;
    database_global_info_entry_t *global_info_ptr = NULL;

    global_info_ptr = in_table_ptr->table_content.global_info_ptr;

    if ((in_table_ptr->table_type != DATABASE_CONFIG_TABLE_GLOBAL_INFO) ||
        (global_info_ptr == NULL)){
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: Table ptr is invalid!\n", 
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_lock(&in_table_ptr->table_lock);
    /* don't have to check the last entry */
    for(index = 0; index < in_table_ptr->table_size - 1; index++)
    {
        header_ptr = (database_table_header_t *)global_info_ptr;
        if(header_ptr->state == DATABASE_CONFIG_ENTRY_UNCOMMITTED)
        {
            header_ptr->state = DATABASE_CONFIG_ENTRY_VALID;
        }
        global_info_ptr++;
    }
    fbe_spinlock_unlock(&in_table_ptr->table_lock);

    return FBE_STATUS_OK;
}


/*!***************************************************************
 * @fn fbe_database_config_update_object_table_prior_commit
 *****************************************************************
 * @brief
 *   This function updates object table before commit
 *
 * @param in_table_ptr - pointer to the database table
 *
 * @return fbe_bool_t 
 *
 * @version
 *
 ****************************************************************/
fbe_status_t fbe_database_config_update_object_table_prior_commit(database_table_t *in_table_ptr)
{
    database_object_entry_t *   object_entry_ptr = NULL;
    fbe_u32_t index = 0;

    object_entry_ptr = in_table_ptr->table_content.object_entry_ptr;

    if ((in_table_ptr->table_type != DATABASE_CONFIG_TABLE_OBJECT) ||
        (object_entry_ptr == NULL)){
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: Table ptr is invalid!\n", 
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_lock(&in_table_ptr->table_lock);
    for(index = 0; index < in_table_ptr->table_size; index++)
    {
        /* update object size */
        if (object_entry_ptr->header.state != DATABASE_CONFIG_ENTRY_INVALID)
        {
            object_entry_ptr->header.version_header.size = database_common_object_entry_size(object_entry_ptr->db_class_id);
        }
        object_entry_ptr++;
    }
    fbe_spinlock_unlock(&in_table_ptr->table_lock);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_config_table_get_striper
 *****************************************************************
 * @brief
 *   This function gets the striper object id from internal RG.
 *
 * @param object_id - internal RG object id
 * @param striper_id - striper RG object id pointer
 *
 * @return fbe_status_t 
 *
 * @version
 *
 ****************************************************************/
fbe_status_t fbe_database_config_table_get_striper(fbe_object_id_t object_id,
                                                   fbe_object_id_t * striper_id)
{
    fbe_status_t status;
    database_table_t              * edge_table_ptr = NULL;
    fbe_u32_t                       edge_index; 

    *striper_id = FBE_OBJECT_ID_INVALID;
    status = fbe_database_get_service_table_ptr(&edge_table_ptr, DATABASE_CONFIG_TABLE_EDGE);
    if ((status != FBE_STATUS_OK)) 
    {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                  FBE_TRACE_MESSAGE_ID_INFO,
                  "%s: get_service_table_ptr failed, status = 0x%x\n",
                  __FUNCTION__, status);
        return status;
    }

    fbe_spinlock_lock(&edge_table_ptr->table_lock);
    for (edge_index = 0; edge_index < edge_table_ptr->table_size; edge_index++)
    {
        if (edge_table_ptr->table_content.edge_entry_ptr[edge_index].server_id == object_id)
        {
            fbe_spinlock_unlock(&edge_table_ptr->table_lock);
            *striper_id = edge_index/DATABASE_MAX_EDGE_PER_OBJECT;
            return FBE_STATUS_OK;
        }
    }
    fbe_spinlock_unlock(&edge_table_ptr->table_lock);

    return FBE_STATUS_GENERIC_FAILURE;
}


/*!***************************************************************
 * @fn fbe_database_config_is_global_info_out_of_sync
 *****************************************************************
 * @brief
 *   This function checks to see if global_info table is out of sync
 * with media (and active side memory)
 *
 * @param in_table_ptr - pointer to the database table
 *
 * @return fbe_bool_t 
 *
 * @version
 *
 ****************************************************************/
fbe_bool_t fbe_database_config_is_global_info_out_of_sync(database_table_t *in_table_ptr)
{
    fbe_bool_t out_of_sync = FBE_FALSE;
    fbe_u32_t index = 0;
    database_table_header_t * header_ptr = NULL;
    database_global_info_entry_t *global_info_ptr = NULL;

    global_info_ptr = in_table_ptr->table_content.global_info_ptr;

    if ((in_table_ptr->table_type != DATABASE_CONFIG_TABLE_GLOBAL_INFO) ||
        (global_info_ptr == NULL)){
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: Table ptr is invalid!\n", 
                       __FUNCTION__);
        return out_of_sync;
    }

    /* don't have to check the first entry and the last entry */
    for(index = 1; index < in_table_ptr->table_size-1; index++)
    {
        header_ptr = (database_table_header_t *)global_info_ptr;
        if ((header_ptr->entry_id == 0) && (header_ptr->state == DATABASE_CONFIG_ENTRY_VALID))
        {
            out_of_sync = FBE_TRUE;	
            break;
        }

        global_info_ptr++;
    }

    return (out_of_sync);
}
