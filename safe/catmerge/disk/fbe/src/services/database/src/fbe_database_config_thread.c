/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_database_config_thread.c
***************************************************************************
*
* @brief
*  This file contains database service functions related to the configuration thread.
*  
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_database.h"
#include "fbe/fbe_provision_drive.h"
#include "fbe_database_private.h"
#include "fbe_database_config_tables.h"
#include "fbe_database_cmi_interface.h"
#include "fbe_database_metadata_interface.h"
#include "fbe_database_system_db_interface.h"
#include "fbe_database_homewrecker_db_interface.h"
#include "fbe/fbe_board.h"
#include "fbe_database_persist_interface.h"
#include "fbe_database_drive_connection.h"
#include "fbe/fbe_lun.h"
#include "fbe/fbe_raid_group.h"
#include "fbe/fbe_event_log_api.h"
#include "fbe/fbe_event_log_utils.h"
#include "fbe_metadata.h"
#include "fbe/fbe_base_config.h"
#include "fbe_cmi.h"

/*******************/
/*local definitions*/
/*******************/


/*******************/
/* local arguments */
/*******************/

fbe_semaphore_t     fbe_db_thread_semaphore;
fbe_bool_t          fbe_db_thread_run = FBE_TRUE;

extern fbe_database_service_t fbe_database_service;

/*******************/
/* local function  */
/*******************/
fbe_bool_t fbe_database_is_job_service_ready(void);
static fbe_status_t	fbe_database_config_thread_main_init(fbe_database_service_t *database_service_ptr);
static void fbe_db_thread_process_state(fbe_database_service_t *database_service_ptr);
static fbe_status_t fbe_database_create_objects_active(fbe_database_service_t *database_service_ptr);
static void fbe_db_create_objects_passive(fbe_database_service_t *database_service_ptr);
static fbe_status_t fbe_db_create_objects_passive_completion(fbe_database_cmi_msg_t *returned_msg, fbe_status_t completion_status, void *context);
static void fbe_database_reload_tables_after_peer_died(fbe_database_service_t *database_service_ptr);
static fbe_status_t database_wait_for_all_psl_luns_ready(fbe_database_service_t * database_service_ptr);
static fbe_status_t fbe_database_create_sep_internal_objects_and_exported_objects(fbe_database_service_t *database_service_ptr, fbe_bool_t invalidate_configuration);
static fbe_status_t fbe_database_mark_NR_for_upstream_objects(fbe_object_id_t object_id, fbe_bool_t *is_NR_successful);
static fbe_status_t database_system_objects_set_config_for_system_drives(fbe_database_service_t * database_service_ptr);
static fbe_status_t fbe_database_create_objects_mini_mode(fbe_database_service_t *database_service_ptr);



/*******************************************************************************************************************/
void fbe_database_config_thread(void *context)
{
	fbe_status_t					status;
	fbe_database_service_t *		database_service_ptr = (fbe_database_service_t *)context;

	fbe_semaphore_init(&fbe_db_thread_semaphore, 0, 0x7FFFFFFF);

	fbe_thread_set_affinity(&database_service_ptr->config_thread_handle, 0x1);

	/*before waiting for events from peer and so on we'll init the system on the context of this thread*/
	status  = fbe_database_config_thread_main_init(database_service_ptr);
	if (status != FBE_STATUS_OK && status != FBE_STATUS_SERVICE_MODE) {

		database_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
					   "%s: failed to initialize database\n", __FUNCTION__);

		set_database_service_state(database_service_ptr, FBE_DATABASE_STATE_FAILED);
		fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
		return;
	}

	while (fbe_db_thread_run) {
		/*we we just wait for things to do*/
		fbe_semaphore_wait(&fbe_db_thread_semaphore, NULL);
		if (!fbe_db_thread_run) {
			database_trace(FBE_TRACE_LEVEL_INFO, 
						   FBE_TRACE_MESSAGE_ID_INFO, 
						   "%s: Thread done\n", __FUNCTION__);

			break;
		}

		fbe_db_thread_process_state(database_service_ptr);/*do stuff based on the state we are in*/
	}

	fbe_semaphore_destroy(&fbe_db_thread_semaphore);
	fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
	return;
}

/* initialize the system db header in database service*/
fbe_status_t fbe_database_system_db_header_init(database_system_db_header_t *system_db_header)
{
    if (system_db_header == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_INFO, 
                 "%s: function argument is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(system_db_header, sizeof(database_system_db_header_t));
     
    system_db_header->magic_number = DATABASE_SYSTEM_DB_HEADER_MAGIC_NUMBER;
    system_db_header->version_header.size = sizeof(database_system_db_header_t);
    system_db_header->persisted_sep_version = SEP_PACKAGE_VERSION;

    system_db_header->bvd_interface_object_entry_size =
        database_common_object_entry_size(DATABASE_CLASS_ID_BVD_INTERFACE);
    system_db_header->pvd_object_entry_size =
        database_common_object_entry_size(DATABASE_CLASS_ID_PROVISION_DRIVE);
    system_db_header->vd_object_entry_size =
        database_common_object_entry_size(DATABASE_CLASS_ID_VIRTUAL_DRIVE);
    system_db_header->rg_object_entry_size = 
        database_common_object_entry_size(DATABASE_CLASS_ID_STRIPER);
    system_db_header->lun_object_entry_size = 
        database_common_object_entry_size(DATABASE_CLASS_ID_LUN);
    system_db_header->ext_pool_object_entry_size =
        database_common_object_entry_size(DATABASE_CLASS_ID_EXTENT_POOL);
    system_db_header->ext_pool_lun_object_entry_size =
        database_common_object_entry_size(DATABASE_CLASS_ID_EXTENT_POOL_LUN);

    system_db_header->edge_entry_size = database_common_edge_entry_size();

    system_db_header->pvd_user_entry_size = database_common_user_entry_size(DATABASE_CLASS_ID_PROVISION_DRIVE);
    system_db_header->rg_user_entry_size = database_common_user_entry_size(DATABASE_CLASS_ID_STRIPER);
    system_db_header->lun_user_entry_size = database_common_user_entry_size(DATABASE_CLASS_ID_LUN);
    system_db_header->ext_pool_user_entry_size = database_common_user_entry_size(DATABASE_CLASS_ID_EXTENT_POOL);
    system_db_header->ext_pool_lun_user_entry_size = database_common_user_entry_size(DATABASE_CLASS_ID_EXTENT_POOL_LUN);

    system_db_header->power_save_info_global_info_entry_size = 
        database_common_global_info_entry_size(DATABASE_GLOBAL_INFO_TYPE_SYSTEM_POWER_SAVE);
    system_db_header->spare_info_global_info_entry_size = 
        database_common_global_info_entry_size(DATABASE_GLOBAL_INFO_TYPE_SYSTEM_SPARE);
    system_db_header->generation_info_global_info_entry_size = 
        database_common_global_info_entry_size(DATABASE_GLOBAL_INFO_TYPE_SYSTEM_GENERATION);
    system_db_header->time_threshold_info_global_info_entry_size = 
        database_common_global_info_entry_size(DATABASE_GLOBAL_INFO_TYPE_SYSTEM_TIME_THRESHOLD);
    system_db_header->encryption_info_global_info_entry_size = 
        database_common_global_info_entry_size(DATABASE_GLOBAL_INFO_TYPE_SYSTEM_ENCRYPTION);
    system_db_header->pvd_config_global_info_entry_size = 
        database_common_global_info_entry_size(DATABASE_GLOBAL_INFO_TYPE_GLOBAL_PVD_CONFIG);

    system_db_header->pvd_nonpaged_metadata_size = sizeof(fbe_provision_drive_nonpaged_metadata_t); 
    system_db_header->lun_nonpaged_metadata_size = sizeof(fbe_lun_nonpaged_metadata_t); 
    system_db_header->raid_nonpaged_metadata_size = sizeof(fbe_raid_group_nonpaged_metadata_t); 
    system_db_header->ext_pool_nonpaged_metadata_size = sizeof(fbe_extent_pool_nonpaged_metadata_t); 
    system_db_header->ext_pool_lun_nonpaged_metadata_size = sizeof(fbe_lun_nonpaged_metadata_t); 

    return FBE_STATUS_OK;
}

/*do the initial initialization*/
static fbe_status_t	fbe_database_config_thread_main_init(fbe_database_service_t *database_service_ptr)
{
    fbe_status_t 	status;
    fbe_bool_t		active_side = database_common_cmi_is_active();

    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "%s Starting DB main thread\n", __FUNCTION__);

    database_service_ptr->updates_received = 0;
    database_service_ptr->peer_sep_version = INVALID_SEP_PACKAGE_VERSION;

    status = fbe_database_get_logical_objects_limits(&database_service_ptr->logical_objects_limits);
    if (status!=FBE_STATUS_OK){
        return status;
    }else{
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "DB RG Limit:%d\n", database_service_ptr->logical_objects_limits.platform_max_rg);
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "DB User LUN Limit:%d\n", database_service_ptr->logical_objects_limits.platform_max_user_lun);
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "DB LU per RG Limit:%d\n", database_service_ptr->logical_objects_limits.platform_max_lun_per_rg);
    }

    status = fbe_database_allocate_config_tables(database_service_ptr);
    if (status!=FBE_STATUS_OK){
        return status;
    }

    /* Populate the Global Info in in-memory config tables */
    status = fbe_database_populate_default_global_info(&database_service_ptr->global_info);
    if (status != FBE_STATUS_OK){
        return status;
    }

    fbe_spinlock_init(&database_service_ptr->system_db_header_lock);
    fbe_zero_memory(&database_service_ptr->system_db_header, sizeof(database_service_ptr->system_db_header));

    /* Transaction init */
    status = fbe_database_allocate_transaction(database_service_ptr);
    if (status!=FBE_STATUS_OK){
        return status;
    }

    /* init pvd handler related stuff */
    status = fbe_database_drive_connection_init(database_service_ptr);
    if (status!=FBE_STATUS_OK){
        return status;
    }

    /* Init the raw mirror of the system db, should be done on both side */
    status = database_system_db_interface_init();
    if (status != FBE_STATUS_OK){
        database_trace(FBE_TRACE_LEVEL_ERROR, 
            FBE_TRACE_MESSAGE_ID_INFO, 
            "%s: failed to init raw mirror of the system db \n", __FUNCTION__);
        return status;
    }

    /* Init homewrecker db, should be done on both side */
    status = database_homewrecker_db_interface_init(database_service_ptr);
    if (status != FBE_STATUS_OK){
        database_trace(FBE_TRACE_LEVEL_ERROR, 
            FBE_TRACE_MESSAGE_ID_INFO, 
            "%s: failed to init homewrecker db \n", __FUNCTION__);
        return status;
    }

    status = fbe_database_system_db_persist_world_initialize();
    if (status != FBE_STATUS_OK){
        database_trace(FBE_TRACE_LEVEL_ERROR, 
            FBE_TRACE_MESSAGE_ID_INFO, 
            "%s: failed to init system db journal world\n", __FUNCTION__);
        return status;
    }

    /*initialize and load database raw-mirror drive state*/
    fbe_database_raw_mirror_drive_state_initialize();

    /* Finish all the initialization tasks which are common on BOTH SPs before fbe_database_cmi_init().
        * With this, we don't need do the initialization again in fbe_database_process_lost_peer 
        */
    status  = fbe_database_cmi_init();
    if (status != FBE_STATUS_OK) {
        return status;
    }

    database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_INFO, 
                   "%s database CMI has been initialized\n", __FUNCTION__);

    // This will block the passive side until the active side is ready.
    // And it's no-op for active side.
    fbe_cmi_sync_open(FBE_CMI_CLIENT_ID_DATABASE);

    /*in theory this might change during the init but we want to do everything start to end as one mode only*/
    active_side = database_common_cmi_is_active();
    if (active_side) {
        database_service_ptr->prev_cmi_state = FBE_DATABASE_CMI_STATE_ACTIVE;
    } else {
        database_service_ptr->prev_cmi_state = FBE_DATABASE_CMI_STATE_PASSIVE;
    }

    if (fbe_database_mini_mode()) {
        /* Disable all background service operation */
        fbe_base_config_control_system_bg_service_t system_bg_service;

        system_bg_service.enable = FBE_FALSE;
        system_bg_service.bgs_flag = FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_ALL;
        system_bg_service.issued_by_ndu = FBE_FALSE;
        system_bg_service.issued_by_scheduler = FBE_FALSE;

        status = fbe_base_config_control_system_bg_service(&system_bg_service);
        if (status != FBE_STATUS_OK){
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                    FBE_TRACE_MESSAGE_ID_INFO, 
                    "%s: failed to stop background services\n", __FUNCTION__);
        }

        status = fbe_database_create_objects_mini_mode(database_service_ptr);
        if (status != FBE_STATUS_OK){
            database_trace(FBE_TRACE_LEVEL_WARNING,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: fail to create objects in mini mode.\n",
                            __FUNCTION__);
            return status;
        }
    }
    else if (active_side)
    {
        status = fbe_database_create_objects_active(database_service_ptr);
        if (status != FBE_STATUS_OK){
            database_trace(FBE_TRACE_LEVEL_WARNING,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: fail to create objects in active.\n",
                            __FUNCTION__);
            return status;
        }
    }
    else
    {
        /* Wait for the job-service thread to be in RUN mode before sending any request to it. */
        do
        {
            fbe_thread_delay(1000);
        }while (fbe_database_is_job_service_ready() != FBE_TRUE);

        /*ask from the active_side to send us the configuration*/
        set_database_service_state(database_service_ptr, FBE_DATABASE_STATE_WAITING_FOR_CONFIG);
        database_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: JOB SERVICE is READY. Send PEER_DB_UPDATE now.\n",
                       __FUNCTION__);
        status = fbe_database_ask_for_peer_db_update();
        if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING) {
            return status;
        }else{
            return FBE_STATUS_OK;
        }
    }

    return FBE_STATUS_OK;

}


/*create the sep internal objects and export objects in a hard-coded way*/
static fbe_status_t fbe_database_create_sep_internal_objects_and_exported_objects(fbe_database_service_t *database_service_ptr, fbe_bool_t invalidate_configuration)
{
    fbe_status_t    status;
    fbe_bool_t		active_side = database_common_cmi_is_active();

    /* create sep internal objects (beside the drives we already created */
    status = database_system_objects_create_sep_internal_objects(database_service_ptr, invalidate_configuration);
    if (status != FBE_STATUS_OK) {
		database_trace(FBE_TRACE_LEVEL_ERROR, 
					   FBE_TRACE_MESSAGE_ID_INFO, 
					   "%s: failed to create sep_internal_objects\n", __FUNCTION__);
        return status;
    }

    /* set metadata lun and wait for it to finish loading */
    status = fbe_database_wait_for_object_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAID_METADATA,  FBE_LIFECYCLE_STATE_READY, DATABASE_WAIT_FOR_OBJECT_STATE_TIME);
    if (status == FBE_STATUS_TIMEOUT) {
        database_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                       "%s: Metadata Lun not ready in 120 secs! Something went wrong. \n", 
                       __FUNCTION__);
        return status;
    }

    if (active_side) {
        fbe_database_metadata_nonpaged_load();
    }

    /* create export system object */
    status = database_system_objects_create_export_objects(database_service_ptr, invalidate_configuration);

    /* wait for db lun before setting up persist service */
    status = fbe_database_wait_for_object_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MCR_DATABASE,  FBE_LIFECYCLE_STATE_READY, DATABASE_WAIT_FOR_OBJECT_STATE_TIME);
    if (status == FBE_STATUS_TIMEOUT) {
        database_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                       "%s: DB Lun not ready in 240 secs!\n", 
                       __FUNCTION__);
        return status;
    }else if(status != FBE_STATUS_OK){
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                       "%s: Failed to wait for DB Lun ready!\n", 
                       __FUNCTION__);
        return status;
    }

    /* wait for mirror DB ready  before setting up persist service 
     * If RG stays at pending ready for a long time, DB lun may have been ready. But the pending ready RG
     * may impact the I/O from DB lun. So we'd better waiting for RG ready.
     */
    status = fbe_database_wait_for_object_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,  FBE_LIFECYCLE_STATE_READY, DATABASE_WAIT_FOR_OBJECT_STATE_TIME);
    if (status == FBE_STATUS_TIMEOUT) {
        database_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                       "%s: Triple mirror RG not ready in 240 secs!\n", 
                       __FUNCTION__);
        return status;
    }else if(status != FBE_STATUS_OK){
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                       "%s: Failed to wait for Triple mirror RG ready!\n", 
                       __FUNCTION__);
        return status;
    }

    database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_INFO, 
                   "%s: DB Lun ready!\n", 
                   __FUNCTION__);

    return status;
}

/*!***************************************************************
@fn fbe_database_mark_NR_for_upstream_objects()
******************************************************************
* @brief
*  This function is used to mark all the raids above the PVD Need-Rebuild.
*  It will get all the raids which are on the top of the PVD.
*  Then all the raid will be marked as NR.
*  
*
* @param  is_NR_successful: if mark NR successful, it will be FBE_TRUE.
*                                        else FBE_FALSE.
* @return
*  fbe_status_t
*
* @version
* 8/24/2012: created by gaoh1
*
******************************************************************/
static fbe_status_t fbe_database_mark_NR_for_upstream_objects(fbe_object_id_t object_id, fbe_bool_t *is_NR_successful)
{
    fbe_object_id_t rg_list[MAX_RG_COUNT_ON_TOP_OF_PVD];
    fbe_raid_group_number_t rg_number_list[MAX_RG_COUNT_ON_TOP_OF_PVD];
    fbe_u32_t   rg_number;
    fbe_status_t    status;
    fbe_base_config_drive_to_mark_nr_t      drive_to_mark_nr;
    fbe_u32_t   index;

    fbe_zero_memory(rg_list, sizeof(rg_list));
    fbe_zero_memory(rg_number_list, sizeof(rg_number_list));
    status = fbe_database_get_rgs_on_top_of_pvd(object_id,
                                        rg_list,
                                        rg_number_list,
                                        MAX_RG_COUNT_ON_TOP_OF_PVD,
                                        &rg_number,
										NULL);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get PVD list for object 0x%x\n",
                       __FUNCTION__, object_id);
        return status;
    }

    drive_to_mark_nr.pvd_object_id = object_id;
    drive_to_mark_nr.nr_status = FBE_FALSE;
    drive_to_mark_nr.force_nr = FBE_FALSE;

    for(index=0; index < rg_number; index++)
    {

        status = fbe_database_wait_for_object_state(rg_list[index], FBE_LIFECYCLE_STATE_READY, 20000);
        if (status == FBE_STATUS_TIMEOUT) 
        {
            *is_NR_successful = FBE_FALSE;
            return status;
        }

        status = fbe_database_send_packet_to_object(FBE_RAID_GROUP_CONTROL_CODE_MARK_NR,
                                                 &drive_to_mark_nr,
                                                 sizeof(fbe_base_config_drive_to_mark_nr_t),
                                                 rg_list[index],
                                                 NULL,
                                                 0,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 FBE_PACKAGE_ID_SEP_0);        
        if(status == FBE_STATUS_OK)
        {
            *is_NR_successful = drive_to_mark_nr.nr_status;
            if(drive_to_mark_nr.nr_status == FBE_FALSE)
            {
                return status;
            }
        }
        else if(status != FBE_STATUS_OK)
        {
            *is_NR_successful = FBE_FALSE;
            return status;
        }
    }                  
    return status;
    
}

/* Don't commit the drives which are marked as recreated with NR flags .
  * We will commit these drives after notifying the upper RG with NR in another function.
  */
static fbe_status_t database_system_objects_commit_system_drives_by_condition(fbe_database_service_t *database_service_ptr,
                                    fbe_database_system_object_recreate_flags_t *recreate_flags)
{
    fbe_object_id_t object_id;
    fbe_status_t    status = FBE_STATUS_OK;

    for (object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST; object_id <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_LAST; object_id++) {
        /* If this PVD needs notify raid to be rebuilt, don't commit it now */
        if ((recreate_flags->system_object_op[object_id] & FBE_SYSTEM_OBJECT_OP_PVD_NR) != 0) {
            continue;
        }

        if(recreate_flags->system_object_op[object_id] & FBE_SYSTEM_OBJECT_OP_RECREATE)
        {
            status = fbe_database_commit_object(object_id, DATABASE_CLASS_ID_PROVISION_DRIVE, FBE_TRUE);
        }
        else
        {
            status = fbe_database_commit_object(object_id, DATABASE_CLASS_ID_PROVISION_DRIVE, FBE_FALSE);
        }
        
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: failed to commit system drive. obj_id = 0x%x \n", 
                           __FUNCTION__, object_id);
            break;
        }
    }

    return status;
}

/* For the system drive which is marked as recreated with NR flags,
* we need to notify the upper RG to be rebuilt. After that, we commit the system drives
*/
static fbe_status_t database_system_objects_notify_RG_NR_and_commit_system_drives(fbe_database_service_t *database_service_ptr,
                                    fbe_database_system_object_recreate_flags_t *recreate_flags)
{
    fbe_object_id_t object_id;
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_bool_t  is_NR_succesfull;
    
    for (object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST; object_id <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_LAST; object_id++) {
        /* If this PVD needs recreation, don't commit it now */
        if ((recreate_flags->system_object_op[object_id] & FBE_SYSTEM_OBJECT_OP_PVD_NR) != 0) {
            /* Mark all the Raid above this PVD as NR */
            status = fbe_database_mark_NR_for_upstream_objects(object_id, &is_NR_succesfull);
            if (status != FBE_STATUS_OK || is_NR_succesfull == FBE_FALSE) {
                database_trace(FBE_TRACE_LEVEL_ERROR, 
                            FBE_TRACE_MESSAGE_ID_INFO, 
                            "%s: fail to mark NR for upstream objects of PVD 0x%x. status=%d\n",
                            __FUNCTION__, object_id, status);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            /* If Mark RAID as NR successfully, then commit this PVD object */
            status = fbe_database_commit_object(object_id, DATABASE_CLASS_ID_PROVISION_DRIVE, FBE_TRUE);
            if (status != FBE_STATUS_OK) {
                database_trace(FBE_TRACE_LEVEL_ERROR, 
                            FBE_TRACE_MESSAGE_ID_INFO, 
                            "%s: fail to commit PVD object (0x%x)\n",
                            __FUNCTION__, object_id);
                return status;
            }
        }
    }

    return status;
}

static fbe_status_t fbe_database_create_objects_active(fbe_database_service_t *database_service_ptr)
{
    fbe_status_t 	status;
    fbe_bool_t 		invalidate_configuration = FBE_FALSE; 
    fbe_u32_t       valid_db_drives = 0;
    fbe_u32_t       retry_count = 0;
    fbe_database_system_object_recreate_flags_t   sys_obj_recreate_flags;
    fbe_bool_t      some_system_lun_or_rg_missing = FBE_FALSE;
    fbe_board_mgmt_get_resume_t board_info;

    /*create drives only and do not connect anything to them yet,
    we need to read ICA stamp from the drives and possibly zero out the PVD before we do anything with them*/
    status = database_system_objects_create_system_drives(database_service_ptr);
    if (status != FBE_STATUS_OK){
        database_trace(FBE_TRACE_LEVEL_ERROR, 
            FBE_TRACE_MESSAGE_ID_INFO, 
            "%s: failed to create system drives \n", __FUNCTION__);
        return status;
    }
    /*Read signatures off the drives and see if we need some specail treatment*/
    status = fbe_database_get_ica_flags(&invalidate_configuration);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_INFO, 
            "%s: failed to read and process ica flags\n", __FUNCTION__);
        return status;
    }
    
    if (invalidate_configuration == FBE_TRUE) {
        database_trace(FBE_TRACE_LEVEL_INFO, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s invalidate flag is set\n", __FUNCTION__);
        
        /* initialize the system db header when persist it first time*/
        fbe_database_system_db_header_init(&database_service_ptr->system_db_header);
        status = database_initialize_shared_expected_memory_info(database_service_ptr);
        if(FBE_STATUS_OK != status)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_INFO, 
                "%s: failed to initialize shared EMV info in ICA case\n", __FUNCTION__);
            return status;
        }

        status = fbe_database_metadata_nonpaged_system_clear();
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: fail to initialize raw mirror for system nonpaged metadata\n",
                            __FUNCTION__);
            return status;
        }

        status = database_system_db_interface_clear();
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: fail to initialize raw mirror for system DB\n",
                            __FUNCTION__);
            return status;
        }
        
        status = database_homewrecker_db_interface_clear();
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: fail to initialize raw mirror for homewrecker DB\n",
                            __FUNCTION__);
            return status;
        }
        
        retry_count = 0;
        status = fbe_database_system_disk_fru_descriptor_init(database_service_ptr);
        while((status != FBE_STATUS_OK) && (retry_count < FBE_DB_FAILED_RAW_IO_RETRIES)){
            /* Wait 1 second */
            fbe_thread_delay(FBE_DB_FAILED_RAW_IO_RETRY_INTERVAL);
            retry_count++;
            status = fbe_database_system_disk_fru_descriptor_init(database_service_ptr);
        }
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: fail to initialize system disk fru descriptor\n",
                            __FUNCTION__);
            return status;
        }
        
        retry_count = 0;
        status = fbe_database_stamp_fru_signature_to_system_drives();
        while((status != FBE_STATUS_OK) && (retry_count < FBE_DB_FAILED_RAW_IO_RETRIES)){
            /* Wait 1 second */
            fbe_thread_delay(FBE_DB_FAILED_RAW_IO_RETRY_INTERVAL);
            retry_count++;
            status = fbe_database_stamp_fru_signature_to_system_drives();
        }
         if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: fail to initialize system disk fru signature\n",
                            __FUNCTION__);
            return status;
        }       
    }
    else
    {
        database_trace(FBE_TRACE_LEVEL_INFO, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s invalidate flag is not set\n", __FUNCTION__);

        database_trace(FBE_TRACE_LEVEL_INFO, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: Begin to start the homewrecker checking.\n", __FUNCTION__);

        database_homewrecker_db_interface_init_raw_mirror_block_seq();

        status = fbe_database_homewrecker_process(database_service_ptr);
        if( FBE_STATUS_SERVICE_MODE == status)
        {
            database_trace(FBE_TRACE_LEVEL_INFO, 
                        FBE_TRACE_MESSAGE_ID_INFO, 
                        "Database service entered service mode!\n");
            return status;
        }
        else if (FBE_STATUS_OK != status)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                            FBE_TRACE_MESSAGE_ID_INFO, 
                            "%s: Failed to do Homewrecker check.\n", __FUNCTION__);
            return status;
        }
        else
        {
            database_trace(FBE_TRACE_LEVEL_INFO, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: Homewrecker check end.\n", __FUNCTION__);
        }
    }        

    
    retry_count = 0;
    do{
        status = fbe_database_get_board_resume_info(&board_info);
        if (status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: retry #%d to get board information, status %d\n",
                           __FUNCTION__,retry_count, status);
                           
            /* Wait 1 second */
            fbe_thread_delay(FBE_DB_FAILED_RAW_IO_RETRY_INTERVAL);
            retry_count++;
        }
    }while((status != FBE_STATUS_OK) && (retry_count < FBE_DB_FAILED_RAW_IO_RETRIES));
    
    if (FBE_STATUS_OK != status)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Failed to get board information\n",
                        __FUNCTION__);
        return status;
    }

    status = fbe_database_persist_prom_info(&board_info, &database_service_ptr->prom_info);
    if (FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                        FBE_TRACE_MESSAGE_ID_INFO, 
                        "%s: Failed assign board info to database.\n", __FUNCTION__);
        return status;
    }

    
    status = fbe_database_raw_mirror_get_valid_drives(&valid_db_drives);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_INFO, 
            "Database service failed to get valid raw-mirror drives, instead load from all drives \n");
        valid_db_drives = DATABASE_SYSTEM_RAW_MIRROR_ALL_DB_DRIVE_MASK;
    }

    /* int the system db seq number */  
    fbe_database_raw_mirror_init_block_seq_number(valid_db_drives);
 
    /* init the system db journal */
    status = database_system_db_persist_init(valid_db_drives);
    if (status != FBE_STATUS_OK){
        database_trace(FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_INFO, 
            "%s: failed to init system db journal\n", __FUNCTION__);
        fbe_event_log_write(SEP_ERROR_CODE_SYSTEM_DB_CONTENT_RW_ERROR,
                            NULL, 0, NULL);
        fbe_database_enter_service_mode(FBE_DATABASE_SERVICE_MODE_REASON_INTERNAL_OBJS);
        status = FBE_STATUS_SERVICE_MODE;
        return status;
    }
    
    /* If Not Set ICA Flag, load system db header from system db before commit any objects.
           * After that, the system db header is valid for usage
           * Then we will do Memory Configuration Check using EMV.
           */
    if (invalidate_configuration != FBE_TRUE) {
        status = fbe_database_system_db_interface_read_and_check_system_db_header(database_service_ptr, valid_db_drives);
        /* If the system db header is not valid, report Service mode */
        if (status != FBE_STATUS_OK){
            return status;
        }

        /*Read shared emv info from disks*/
        status = fbe_database_system_db_read_semv_info(&database_service_ptr->shared_expected_memory_info, valid_db_drives);
        if (FBE_STATUS_OK != status) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_INFO, 
                "%s: failed to read shared emv info from db raw-mirror\n", __FUNCTION__);
            return status;
        }
        
        /*Do memory configuration validation right after system db header is loaded
                  *If any thing is wrong, make system enter maintanence mode to block system going further*/
        status = database_check_system_memory_configuration(database_service_ptr);
        if(FBE_STATUS_OK != status)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_INFO, 
                "%s: failed to check system memory configuration\n", __FUNCTION__);
            /*in the check routine we have entered service mode if needed*/
            return status;
        }            
        
    }
    
    /*load system non-paged metadata from drives Homerecker consider it not new drive*/
    status = fbe_database_metadata_nonpaged_system_load_with_drivemask(valid_db_drives);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
            FBE_TRACE_MESSAGE_ID_INFO, 
            "%s: failed to load system nopaged metadata with drives 0x%x\n", 
            __FUNCTION__,
            valid_db_drives);
        return status;
    }
    database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_INFO, 
                   "%s: load system nonpaged metadata sucessfully\n", __FUNCTION__);    

    /* We check whether user mark system objects for recreation.
        * This function should be invoked after loading the system nonpaged metadata and 
        * before committing system PVDs.
        */
    if (invalidate_configuration != FBE_TRUE) {
        fbe_zero_memory(&sys_obj_recreate_flags, sizeof(fbe_database_system_object_recreate_flags_t));
        status = fbe_database_check_and_recreate_system_object(&sys_obj_recreate_flags);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s: failed to check and recreate system objects\n", __FUNCTION__);
            return status;
        }
    }

    /* If ICA flag is set, boot system by hardcoded configuration*/
    if (invalidate_configuration == FBE_TRUE) {
        status = database_system_objects_commit_system_drives(database_service_ptr, FBE_TRUE);
        if (status != FBE_STATUS_OK){
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to commit system drives in ICA case\n", __FUNCTION__);
            return status;
        }
        
        status = fbe_database_create_sep_internal_objects_and_exported_objects(database_service_ptr, invalidate_configuration);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_INFO, 
                "%s: failed to create system sep objects \n", __FUNCTION__);
            return status;
        }
        /* Persist all system entries to raw mirror of system db if ICA flag is set */
        status = fbe_database_system_db_persist_entries(database_service_ptr);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: failed to persist system objects entries\n", __FUNCTION__);
            return status;
        }

        status = fbe_database_reset_ica_flags();
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: failed to reset ICA flags\n", __FUNCTION__);
            return status;
        }
    } else { /* else Boot system by configuration loading from system db*/
        /*(1) Load system configurations from disks first*/    
        status = fbe_database_system_entries_load_from_persist();
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: failed to load system entries. status = 0x%x\n",
                __FUNCTION__, status);
            return status;
        }
        /* Decide what the encryption mode of the system is.
         * This is done since system objects need to know this before the
         * system db tables are loaded.
         */
        database_system_objects_check_for_encryption();
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to check for encryption status = 0x%x\n",
                           __FUNCTION__, status);
        }
        /*(2) Check whether there is missing system object, if there is, DB would make up its configuration
          *       then, persist that configuration to raw_mirror, it would be created when create system topology.
          */
        status = database_check_and_make_up_system_object_configuration(database_service_ptr, &some_system_lun_or_rg_missing);
        if (status != FBE_STATUS_OK){
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_INFO, 
                "%s: failed to check and make up system object configuration.\n", __FUNCTION__);
            return status;
        }

        if (some_system_lun_or_rg_missing)
        {
            /* Persist all system entries to raw mirror of system db if some system object 
              * missing and DB made up their configuration from PSL
              */
            database_trace(FBE_TRACE_LEVEL_INFO, 
                               FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s: Some system objects' configuration was missing, we made up it.\n", __FUNCTION__);
            database_trace(FBE_TRACE_LEVEL_INFO, 
                               FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s: Persist system objects' configuration.\n", __FUNCTION__);
            
            status = fbe_database_system_db_persist_entries(database_service_ptr);
            if (status != FBE_STATUS_OK) {
                database_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s: failed to persist system objects entries\n", __FUNCTION__);
                return status;
            }
        }
        else
        {
            database_trace(FBE_TRACE_LEVEL_INFO, 
                               FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s: all system objects' configuration are OK.\n", __FUNCTION__);
        }
        
        /*(3) Set the config again for system PVDs using the info we read from persisted tables to overwrite
         *    the default values we set before. 
         */
        status = database_system_objects_set_config_for_system_drives(database_service_ptr);
        if (status != FBE_STATUS_OK){
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_INFO, 
                "%s: failed to set config for system drives in non ICA case\n", __FUNCTION__);
            return status;
        }

        /*(4) Commit system drives only after loading the system configuration from raw-mirror so
         *    the geometry of the system PVDs would be calculated by the correct configuration.
         */        
        if(database_has_system_object_to_be_recreated(&sys_obj_recreate_flags) == FBE_FALSE)
        {
            status = database_system_objects_commit_system_drives(database_service_ptr, FBE_FALSE);        
        } 
        else
        {
            status = database_system_objects_commit_system_drives_by_condition(database_service_ptr, &sys_obj_recreate_flags);
        }

        if (status != FBE_STATUS_OK){
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to commit system drives in non ICA case\n", __FUNCTION__);
            return status;
        }

        /*(5) Create system objects according to the entries read from raw-mirror*/
        status = fbe_database_system_create_topology(database_service_ptr);
        if (status != FBE_STATUS_OK) {
            database_trace((status == FBE_STATUS_SERVICE_MODE)?FBE_TRACE_LEVEL_WARNING:FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: fail to load system object topology\n", __FUNCTION__);
            return status;
        }

        /*(6) Now all system objects are created, if there is system PVD recreated, we need to notify all the raid group
        * above this PVD need-rebuild. If mark raid group rebuild succesfully, commit the PVD object */
        if (database_has_system_object_to_be_recreated(&sys_obj_recreate_flags) == FBE_TRUE) {
            status = database_system_objects_notify_RG_NR_and_commit_system_drives(database_service_ptr, &sys_obj_recreate_flags);
            if (status != FBE_STATUS_OK) 
            {
                database_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: fail to notify RG NR and commit the system drives\n",
                            __FUNCTION__);
                return status;        
            }

            /* After recreating the system object according to the system object recreation flags, reset and persist the flags */
            fbe_zero_memory(&sys_obj_recreate_flags, sizeof(fbe_database_system_object_recreate_flags_t));
            status = fbe_database_persist_system_object_recreate_flags(&sys_obj_recreate_flags);
            if (status != FBE_STATUS_OK) 
            {
                database_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: Reset and persist the system object operation falgs failed\n",
                            __FUNCTION__);
                return status;        
            }
                    
        }
    }

    /* Now that we have read in the system config and system np using the raw mirror 
     * we need to use the raw mirror object.
     */
    status = fbe_database_switch_to_raw_mirror_object();
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to switch over to raw mirror raid group object\n", __FUNCTION__);
        return status;
    }

    /* Enable bg ops in RG now that our system ops are going to the raw mirror rg. 
         * We must wait until this point to enable the ops to avoid operations from running in the rg at the same time 
         * as we are using the raw mirror. 
         * bg ops in PVD will enable later from sep_init once database state is READY
         */
    status = fbe_database_enable_bg_operations(FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_INITIAL);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to enable bg operations\n", __FUNCTION__);
        return status;
    }

    /* setup persist service (should be done on active side, when switching from passive to active persis will be initialized*/
    status = fbe_database_init_persist();
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
            FBE_TRACE_MESSAGE_ID_INFO, 
            "%s: failed to init persist service\n", __FUNCTION__);
        return status;
    } 
 
    /* If active side, load and create the persisted objects/edges */
    status = fbe_database_load_topology(database_service_ptr, FBE_TRUE);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
            FBE_TRACE_MESSAGE_ID_INFO, 
            "%s: failed to load topology\n", __FUNCTION__);
        return status;
    }

    /* starts initial discovery.*/
    fbe_database_drive_discover(&database_service_ptr->pvd_operation);

    /*let's make sure All PSL luns are ok before we declare to be ready*/
    status = database_wait_for_all_psl_luns_ready(database_service_ptr);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
            FBE_TRACE_MESSAGE_ID_INFO, 
            "%s: failed to wait for PSL luns to be ready\n", __FUNCTION__);
        return status;
    }

    /* open to transaction request */
    if (database_service_ptr->state != FBE_DATABASE_STATE_CORRUPT) {
        set_database_service_state(database_service_ptr, FBE_DATABASE_STATE_READY);
    }

    /*start the drive process thread right after database becomes READY*/
    fbe_database_drive_connection_start_drive_process(database_service_ptr);

    database_trace(FBE_TRACE_LEVEL_INFO,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: end\n", __FUNCTION__);
    return FBE_STATUS_OK;
}

/*called on the passive side to ask the thread to go over the tables and create objects based on that*/
void fbe_database_config_thread_init_passive(void)
{
	fbe_semaphore_release(&fbe_db_thread_semaphore, 0, 1, FALSE);

}

void fbe_db_thread_stop(fbe_database_service_t *database_Service_ptr)
{
	fbe_db_thread_run = FBE_FALSE;
	fbe_semaphore_release(&fbe_db_thread_semaphore, 0, 1, FALSE);
	fbe_thread_wait(&database_Service_ptr->config_thread_handle);
}

void fbe_db_thread_wakeup(void)
{
        fbe_semaphore_release(&fbe_db_thread_semaphore, 0, 1, FALSE);
}

/*do things based on the state we are at*/
static void fbe_db_thread_process_state(fbe_database_service_t *database_service_ptr)
{
    fbe_bool_t				active_side = database_common_cmi_is_active();
	fbe_database_state_t 	db_state;

	get_database_service_state(database_service_ptr, &db_state);

	/*if we are in a state of initializing and we are the passice SP it means the tables were updated
	by the peer and now we need to create objects based on that. We let the state as FBE_DATABASE_STATE_WAITING_FOR_CONFIG on purpose
	to know that we are still in the process of restarting ourselvs*/
	if (!active_side && (db_state == FBE_DATABASE_STATE_WAITING_FOR_CONFIG)) {

		/*before we do anything we have to make sure we got all messages*/
		while ((database_service_ptr->updates_sent_by_active_side != database_service_ptr->updates_received) &&
               !active_side && (db_state == FBE_DATABASE_STATE_WAITING_FOR_CONFIG)) {
			database_trace(FBE_TRACE_LEVEL_INFO, 
						   FBE_TRACE_MESSAGE_ID_INFO, 
						   "%s: updates_sent_by_active_side = 0x%llx, updates_received = 0x%llx \n", 
						   __FUNCTION__,
                           (unsigned long long)database_service_ptr->updates_sent_by_active_side,
                           (unsigned long long)database_service_ptr->updates_received);

			fbe_thread_delay(100);
            active_side = database_common_cmi_is_active();
            get_database_service_state(database_service_ptr, &db_state);
		}

        if (!active_side && (db_state == FBE_DATABASE_STATE_WAITING_FOR_CONFIG) &&
            (database_service_ptr->updates_sent_by_active_side == database_service_ptr->updates_received)) {
            database_trace(FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: PASSIVE side got 0x%llx updates, notify CMI thread to create objects.\n",
                           __FUNCTION__, (unsigned long long)database_service_ptr->updates_received);

            fbe_database_cmi_thread_set_event(FBE_DB_CMI_EVENT_CONFIG_DONE);
        } else {
            /* We can go here only when:
             * 1. Peer dead. Now wa are active and active_side == TRUE
             * 2. db_state != FBE_DATABASE_STATE_WAITING_FOR_CONFIG. There should be some bug in database cmi code.
             */
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s:update db failed. is_active: %u, db_state: %u, sent: 0x%llx, recv: 0x%llx\n",
                           __FUNCTION__, active_side, db_state,
                           (unsigned long long)database_service_ptr->updates_sent_by_active_side,
                           (unsigned long long)database_service_ptr->updates_received);
        }
	}

	/*
     * peer is dead in mid update, or database is reloading
     * let's erase everything and start from scratch.
     *
     * We have better do this in cmi thread too.
     * Since we can go here only when peer lost, so there is no cmi message any more,
     * It is safe to put it here.
     */
    if (active_side &&
        (db_state == FBE_DATABASE_STATE_WAITING_FOR_CONFIG
         || db_state == FBE_DATABASE_STATE_INITIALIZING)) {
        database_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: reload tables due to peer dead\n",
                       __FUNCTION__);

		fbe_database_reload_tables_after_peer_died(database_service_ptr);
	}


}

/*!**************************************************************
 * fbe_db_create_objects_passive_in_cmi_thread
 ****************************************************************
 * @brief
 *  Sync the create of passive objects in CMI thread.
 *  Fix the unsync accessing of db_state issue
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  2014-05-19 - Created. Jamin Kang
 *
 ****************************************************************/
void fbe_db_create_objects_passive_in_cmi_thread(void)
{
    fbe_bool_t				active_side = database_common_cmi_is_active();
	fbe_database_state_t 	db_state;
    fbe_database_service_t *database_service_ptr = &fbe_database_service;

	get_database_service_state(database_service_ptr, &db_state);
    database_trace(FBE_TRACE_LEVEL_INFO,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: is active: %u, state: %u\n",
                   __FUNCTION__, active_side, db_state);
    if (active_side || db_state != FBE_DATABASE_STATE_WAITING_FOR_CONFIG) {
        /* We got peer lost after setting the event.
         * Just ignore here, config thread will reload the tables
         */
        database_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: state changed after setting the event. Ignore it!\n",
                       __FUNCTION__);
        return;
    }
    if (database_service_ptr->updates_sent_by_active_side != database_service_ptr->updates_received) {
        database_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Update unmatch: sent: %llu, received: %llu\n",
                       __FUNCTION__,
                       (unsigned long long)database_service_ptr->updates_sent_by_active_side,
                       (unsigned long long)database_service_ptr->updates_received);
        return;
    }
    database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_INFO, 
                   "%s: PASSIVE side got %llu updates and will activate tables\n",
                   __FUNCTION__, (unsigned long long)database_service_ptr->updates_received);
    fbe_db_create_objects_passive(database_service_ptr);
}

static void fbe_db_create_objects_passive(fbe_database_service_t *database_service_ptr)
{
	fbe_status_t 				status;
	fbe_database_cmi_msg_t * 	msg_memory = NULL;
        fbe_u32_t   valid_db_drives;

    /*create drives only*/
	status = database_system_objects_create_system_drives(database_service_ptr);
	if (status!=FBE_STATUS_OK){
		database_trace(FBE_TRACE_LEVEL_ERROR, 
					   FBE_TRACE_MESSAGE_ID_INFO, 
					   "%s: failed to create drives\n", __FUNCTION__);
		return;
	}
	/*todo:  assuming we are not doing a invalidate configuration here.  Is it always true???*/

    /*get valid db drives mask*/
    status = fbe_database_raw_mirror_get_valid_drives(&valid_db_drives);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_INFO, 
            "Database service failed to get valid raw-mirror drives, instead load from all drives \n");
        valid_db_drives = DATABASE_SYSTEM_RAW_MIRROR_ALL_DB_DRIVE_MASK;
    }

    /*Right after create system drives we should check system memory configuration*/
    /*first read shared emv info from disks*/
    status = fbe_database_system_db_read_semv_info(&database_service_ptr->shared_expected_memory_info, valid_db_drives);
    if (FBE_STATUS_OK != status) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
            FBE_TRACE_MESSAGE_ID_INFO, 
            "%s: failed to read shared emv info from db raw-mirror\n", __FUNCTION__);
        return;
    }
    
    status = database_check_system_memory_configuration(database_service_ptr);
    if(FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_INFO, 
                 "%s: failed to check system memory configuration\n", __FUNCTION__);
        return;
    }

    /* Do not commit before the metadata service load the nonpaged system */
	status = database_system_objects_commit_system_drives(database_service_ptr, FBE_FALSE);
	if (status!=FBE_STATUS_OK){
		return;
	}

    /* In passive side, it won't load entries from raw-3way-mirror */
    status = fbe_database_system_create_topology(database_service_ptr);
    if (status != FBE_STATUS_OK) {
        if (status == FBE_STATUS_SERVICE_MODE) {
            database_trace(FBE_TRACE_LEVEL_WARNING,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: load system topoloby failed, enter into degraded mode\n",
                           __FUNCTION__);
            return;
        }
        database_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
					   FBE_TRACE_MESSAGE_ID_INFO, 
					   "%s: failed to load system object in passive side\n", __FUNCTION__);
        return ;
    }
 
    /* Passive side switches right away to use raw mirror object for writes.
     */
    status = fbe_database_switch_to_raw_mirror_object();
    
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to switch over to raw mirror raid group object\n", __FUNCTION__);
        return;
    }

    /* Enable bg ops in RG now that our system ops are going to the raw mirror rg. 
         * We must wait until this point to enable the ops to avoid operations from running in the rg at the same time 
         * as we are using the raw mirror. 
         * bg ops in PVD will enable later from sep_init once database state is READY
         */
    status = fbe_database_enable_bg_operations(FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_INITIAL);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to enable bg operations\n", __FUNCTION__);
        return;
    }

	/* setup persist service (should be done on active side, when switching from passive to active persis will be initialized*/
	status = fbe_database_init_persist();
	if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_INFO, 
                "%s: failed to init persist service\n", __FUNCTION__);
        return;
	}

    /*create the rest of the topology in the system*/
	status = fbe_database_load_topology(database_service_ptr,FBE_FALSE);
	if (status != FBE_STATUS_OK) {
		database_trace(FBE_TRACE_LEVEL_ERROR, 
					   FBE_TRACE_MESSAGE_ID_INFO, 
					   "%s: failed to create non system objects\n", __FUNCTION__);

		return;
	}

    /*let's make sure All PSL luns are ok before we declare to be ready*/
    status = database_wait_for_all_psl_luns_ready(database_service_ptr);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
            FBE_TRACE_MESSAGE_ID_INFO, 
            "%s: failed to wait for PSL luns to be ready\n", __FUNCTION__);
        return;
    }

    if (database_service_ptr->state != FBE_DATABASE_STATE_CORRUPT) {
        set_database_service_state(database_service_ptr, FBE_DATABASE_STATE_READY);
    }
    
    /*start the drive process thread right after database becomes READY*/
    fbe_database_drive_connection_start_drive_process(database_service_ptr);

	/*now send a CMI to the peer letting it*/
	msg_memory = fbe_database_cmi_get_msg_memory();
	if (msg_memory == NULL) {
		database_trace(FBE_TRACE_LEVEL_ERROR, 
					   FBE_TRACE_MESSAGE_ID_INFO, 
					   "%s: failed to get CMI message to send to the peer\n", __FUNCTION__);

		return;
	}

	msg_memory->completion_callback = fbe_db_create_objects_passive_completion;
	msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_UPDATE_CONFIG_PASSIVE_INIT_DONE;

	fbe_database_cmi_send_message(msg_memory, NULL);
}

static fbe_status_t fbe_db_create_objects_passive_completion(fbe_database_cmi_msg_t *returned_msg, fbe_status_t completion_status, void *context)
{
	/*let's return the message memory first*/
	fbe_database_cmi_return_msg_memory(returned_msg);

	/*and check the status*/
    if (completion_status != FBE_STATUS_OK) {
		/*we do nothing here, the CMI interrupt for a dead SP will trigger us doing the config read on our own*/
		database_trace (FBE_TRACE_LEVEL_WARNING,
						FBE_TRACE_MESSAGE_ID_INFO,
						"%s: failed to send FBE_DATABE_CMI_MSG_TYPE_UPDATE_CONFIG_PASSIVE_INIT_DONE to peer\n", __FUNCTION__ );
	}

	return FBE_STATUS_OK;
}

/*this function is called when the active sp started sending us the tables but died in the middle
we'll clean the tables and do our own discovery*/
void fbe_database_reload_tables_in_mid_update()
{
	/*we will be in a state so the thread will know what to do*/
	fbe_semaphore_release(&fbe_db_thread_semaphore, 0, 1, FALSE);
}

static void fbe_database_reload_tables_after_peer_died(fbe_database_service_t *database_service_ptr)
{
	fbe_status_t		status;

	database_trace (FBE_TRACE_LEVEL_INFO,
					FBE_TRACE_MESSAGE_ID_INFO,
					"%s: resetting DB after peer died\n", __FUNCTION__ );

	set_database_service_state(database_service_ptr, FBE_DATABASE_STATE_INITIALIZING);

	fbe_database_config_tables_destroy(database_service_ptr);

	status = fbe_database_allocate_config_tables(database_service_ptr);
	if (status != FBE_STATUS_OK){
		database_trace(FBE_TRACE_LEVEL_ERROR, 
					   FBE_TRACE_MESSAGE_ID_INFO, 
					   "%s: failed to allocate config tables\n", __FUNCTION__);
		
		return;
	}

	/* Populate the Global Info in in-memory config tables */
	status = fbe_database_populate_default_global_info(&database_service_ptr->global_info);
	if (status != FBE_STATUS_OK){
		database_trace(FBE_TRACE_LEVEL_ERROR, 
					   FBE_TRACE_MESSAGE_ID_INFO, 
					   "%s: failed to fill default sys info\n", __FUNCTION__);
		return ;
	}

    status = fbe_database_create_objects_active(database_service_ptr);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
					   FBE_TRACE_MESSAGE_ID_INFO, 
					   "%s: failed to create object in active\n", __FUNCTION__);
		
		return;
	}
    
	return;
}

static fbe_status_t database_wait_for_all_psl_luns_ready(fbe_database_service_t * database_service_ptr)
{
	fbe_status_t 						status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t 					object_id; 
    fbe_private_space_layout_lun_info_t lun_info;
    fbe_u64_t current_commit_level;

    fbe_spinlock_lock(&fbe_database_service.system_db_header_lock);
    current_commit_level = fbe_database_service.system_db_header.persisted_sep_version;
    fbe_spinlock_unlock(&fbe_database_service.system_db_header_lock);

    /* We dont need to wait for all the PSL LUN's to be in READY state to bring SEP.
     * It is enough to wait for all the DB LUN's and PSM and not all Vault LU's 
     * (due to a possible Degraded or Broken Vault LUN).
     */
    for( object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_FIRST; 
         object_id <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_LAST;
         object_id ++){

        status = fbe_private_space_layout_get_lun_by_object_id(object_id, &lun_info);
        if( (status == FBE_STATUS_OK)
            && (lun_info.flags & FBE_PRIVATE_SPACE_LAYOUT_FLAG_SYSTEM_CRITICAL)
            && (lun_info.commit_level <= current_commit_level))
        {
            status = fbe_database_wait_for_object_state(object_id,  FBE_LIFECYCLE_STATE_READY, DATABASE_WAIT_FOR_OBJECT_STATE_TIME);
            if (status == FBE_STATUS_TIMEOUT) {
               database_trace(FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s: PSL Lun obj 0x%x not ready in 240 sec!\n", 
                              __FUNCTION__, object_id);
               fbe_event_log_write(SEP_ERROR_CODE_SYSTEM_DB_CONTENT_CORRUPTED,
									NULL, 0, NULL);
               fbe_database_enter_service_mode(FBE_DATABASE_SERVICE_MODE_REASON_INTERNAL_OBJS);

               return FBE_STATUS_SERVICE_MODE; /* We go to degraded mode if we have a probleme with a system lun.*/ 
                
            }else if(status != FBE_STATUS_OK){
               database_trace(FBE_TRACE_LEVEL_ERROR, 
               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
               "%s: PSL Lun obj 0x%x not ready in 240 sec!\n", 
               __FUNCTION__, object_id);
               fbe_event_log_write(SEP_ERROR_CODE_SYSTEM_DB_CONTENT_CORRUPTED,
									NULL, 0, NULL);
               fbe_database_enter_service_mode(FBE_DATABASE_SERVICE_MODE_REASON_INTERNAL_OBJS);

               return FBE_STATUS_SERVICE_MODE; /* We go to degraded mode if we have a probleme with a system lun.*/
            }
            database_trace(FBE_TRACE_LEVEL_INFO, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "PSL Lun 0x%x ready!\n", 
                           object_id);  

        }
        
    }
    return FBE_STATUS_OK;

}
fbe_status_t fbe_database_enable_bg_operations(fbe_base_config_control_system_bg_service_flags_t bgs_flags)
{
    fbe_status_t status;
    fbe_base_config_control_system_bg_service_t system_bg_service;

    /* start all background services */
    system_bg_service.enable = FBE_TRUE;
    system_bg_service.bgs_flag = bgs_flags;
    system_bg_service.issued_by_ndu = FBE_FALSE;
    system_bg_service.issued_by_scheduler = FBE_FALSE;

    status = fbe_base_config_control_system_bg_service(&system_bg_service);
    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to stop background ops\n", __FUNCTION__);
    }
    return status;
}

fbe_bool_t fbe_database_is_job_service_ready(void)
{
    fbe_bool_t is_job_service_ready = FBE_FALSE;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_database_send_packet_to_service(FBE_JOB_CONTROL_CODE_GET_STATE, 
                                                 &is_job_service_ready, 
                                                 sizeof(fbe_bool_t),
                                                 FBE_SERVICE_ID_JOB_SERVICE, 
                                                 NULL,  /* no sg list*/ 
                                                 0,  /* no sg list*/ 
                                                 FBE_PACKET_FLAG_NO_ATTRIB, 
                                                 FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) 
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                       "%s: send Get state request failed.  Error status: 0x%x\n", 
                       __FUNCTION__, 
                       status);
    }

    return is_job_service_ready;
}

static fbe_status_t database_system_objects_set_config_for_system_drives(fbe_database_service_t * database_service_ptr)
{
    fbe_status_t    status;
    
    status = database_common_set_config_from_table(&database_service_ptr->object_table, 
                                                      FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST, 
                                                      (FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_LAST-FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST+1));

    if(FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                       "%s: FAILED.  Error status: 0x%x\n", 
                       __FUNCTION__, 
                       status);
    }

    return status;    
}

static fbe_status_t fbe_database_create_objects_mini_mode(fbe_database_service_t *database_service_ptr)
{
    fbe_status_t 	status;
    fbe_bool_t 		invalidate_configuration = FBE_FALSE; 
    fbe_u32_t       valid_db_drives = 0;
    fbe_u32_t       retry_count = 0;
    fbe_board_mgmt_get_resume_t board_info;
    homewrecker_system_disk_table_t system_disk_table[FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER] = {0};

    /*create drives only and do not connect anything to them yet,
    we need to read ICA stamp from the drives and possibly zero out the PVD before we do anything with them*/
    status = database_system_objects_create_system_drives(database_service_ptr);
    if (status != FBE_STATUS_OK){
        database_trace(FBE_TRACE_LEVEL_ERROR, 
            FBE_TRACE_MESSAGE_ID_INFO, 
            "%s: failed to create system drives \n", __FUNCTION__);
        return status;
    }
    /*Read signatures of the drives and see if we need some specail treatment*/
    status = fbe_database_get_ica_flags(&invalidate_configuration);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_INFO, 
            "%s: failed to read and process ica flags\n", __FUNCTION__);
        return status;
    }
    
    if (invalidate_configuration == FBE_TRUE) {
        database_trace(FBE_TRACE_LEVEL_INFO, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s invalidate flag is set\n", __FUNCTION__);
        
        /* initialize the system db header when persist it first time*/
        fbe_database_system_db_header_init(&database_service_ptr->system_db_header);

        status = fbe_database_metadata_nonpaged_system_zero_and_persist(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FIRST,
                                                                (FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_LAST-FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FIRST+1));
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: fail to initialize raw mirror for system nonpaged metadata\n",
                            __FUNCTION__);
            return status;
        }

        status = database_system_db_interface_clear();
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: fail to initialize raw mirror for system DB\n",
                            __FUNCTION__);
            return status;
        }

        status = database_homewrecker_db_interface_clear();
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: fail to initialize raw mirror for homewrecker DB\n",
                            __FUNCTION__);
            return status;
        }
        
        retry_count = 0;
        status = fbe_database_system_disk_fru_descriptor_init(database_service_ptr);
        while((status != FBE_STATUS_OK) && (retry_count < FBE_DB_FAILED_RAW_IO_RETRIES)){
            /* Wait 1 second */
            fbe_thread_delay(FBE_DB_FAILED_RAW_IO_RETRY_INTERVAL);
            retry_count++;
            status = fbe_database_system_disk_fru_descriptor_init(database_service_ptr);
        }
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: fail to initialize system disk fru descriptor\n",
                            __FUNCTION__);
            return status;
        }
        
        retry_count = 0;
        status = fbe_database_stamp_fru_signature_to_system_drives();
        while((status != FBE_STATUS_OK) && (retry_count < FBE_DB_FAILED_RAW_IO_RETRIES)){
            /* Wait 1 second */
            fbe_thread_delay(FBE_DB_FAILED_RAW_IO_RETRY_INTERVAL);
            retry_count++;
            status = fbe_database_stamp_fru_signature_to_system_drives();
        }
         if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: fail to initialize system disk fru signature\n",
                            __FUNCTION__);
            return status;
        }       
    }
    else
    {
        database_trace(FBE_TRACE_LEVEL_INFO, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s invalidate flag is not set\n", __FUNCTION__);

        database_trace(FBE_TRACE_LEVEL_INFO, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: Begin to start the mini homewrecker checking.\n", __FUNCTION__);

        database_homewrecker_db_interface_init_raw_mirror_block_seq();

        /*fbe_database_mini_homewrecker_process*/
        status = fbe_database_mini_homewrecker_process(database_service_ptr, system_disk_table);
        if( FBE_STATUS_SERVICE_MODE == status)
        {
            database_trace(FBE_TRACE_LEVEL_INFO, 
                        FBE_TRACE_MESSAGE_ID_INFO, 
                        "Database service entered service mode!\n");
            return status;
        }
        else if (FBE_STATUS_OK != status)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                            FBE_TRACE_MESSAGE_ID_INFO, 
                            "%s: Failed to do Homewrecker check.\n", __FUNCTION__);
            return status;
        }
        else
        {
            database_trace(FBE_TRACE_LEVEL_INFO, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: Homewrecker check end.\n", __FUNCTION__);
        }
    }        

    
    retry_count = 0;
    do{
        status = fbe_database_get_board_resume_info(&board_info);
        if (status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: retry #%d to get board information, status %d\n",
                           __FUNCTION__,retry_count, status);
                           
            /* Wait 1 second */
            fbe_thread_delay(FBE_DB_FAILED_RAW_IO_RETRY_INTERVAL);
            retry_count++;
        }
    }while((status != FBE_STATUS_OK) && (retry_count < FBE_DB_FAILED_RAW_IO_RETRIES));
    
    if (FBE_STATUS_OK != status)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Failed to get board information\n",
                        __FUNCTION__);
        return status;
    }

    status = fbe_database_persist_prom_info(&board_info, &database_service_ptr->prom_info);
    if (FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                        FBE_TRACE_MESSAGE_ID_INFO, 
                        "%s: Failed assign board info to database.\n", __FUNCTION__);
        return status;
    }

    
    status = fbe_database_raw_mirror_get_valid_drives(&valid_db_drives);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_INFO, 
            "Database service failed to get valid raw-mirror drives, instead load from all drives \n");
        valid_db_drives = DATABASE_SYSTEM_RAW_MIRROR_ALL_DB_DRIVE_MASK;
    }

    
    /* If Not Set ICA Flag, load system db header from system db before commit any objects.
           * After that, the system db header is valid for usage
           * Then we will do Memory Configuration Check using EMV.
           */
    if (invalidate_configuration != FBE_TRUE) {
        /* int the system db seq number */  
        fbe_database_raw_mirror_init_block_seq_number(valid_db_drives);

        status = fbe_database_system_db_interface_read_and_check_system_db_header(database_service_ptr, valid_db_drives);
        /* If the system db header is not valid, report Service mode */
        if (status != FBE_STATUS_OK){
            return status;
        }
    }
    
    /*load system non-paged metadata from drives Homerecker consider it not new drive*/
    status = fbe_database_metadata_nonpaged_system_read_persist_with_drivemask(valid_db_drives, 
                                                                     FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FIRST,
                                                                     (FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_LAST-FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FIRST+1));
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
            FBE_TRACE_MESSAGE_ID_INFO, 
            "%s: failed to load system nopaged metadata with drives 0x%x\n", 
            __FUNCTION__,
            valid_db_drives);
        return status;
    }

    database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_INFO, 
                   "%s: load system nonpaged metadata sucessfully\n", __FUNCTION__);
 
    /* If ICA flag is set, boot system by hardcoded configuration*/
    if (invalidate_configuration == FBE_TRUE) {
        status = database_system_objects_commit_system_drives(database_service_ptr, FBE_TRUE);
        if (status != FBE_STATUS_OK){
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to commit system drives in ICA case\n", __FUNCTION__);
            return status;
        }
        /*create the bvd interface*/
        status = database_system_objects_create_object_and_its_edges(database_service_ptr, FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_BVD_INTERFACE, FBE_TRUE);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s:fail to create BVD interface in ICA case\n", __FUNCTION__);
            return status;
        }

    } else { /* else Boot system by configuration loading from system db*/
        /*(1) Load system configurations from disks first*/    
        status = fbe_database_system_entries_load_from_persist();
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: failed to load system entries. status = 0x%x\n",
                __FUNCTION__, status);
            return status;
        }
        /* Decide what the encryption mode of the system is.
         * This is done since system objects need to know this before the
         * system db tables are loaded.
         */
        database_system_objects_check_for_encryption();
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to check for encryption status = 0x%x\n",
                           __FUNCTION__, status);
        }
        /*(3) Set the config again for system PVDs using the info we read from persisted tables to overwrite
         *    the default values we set before. 
         */
        status = database_system_objects_set_config_for_system_drives(database_service_ptr);
        if (status != FBE_STATUS_OK){
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_INFO, 
                "%s: failed to set config for system drives in non ICA case\n", __FUNCTION__);
            return status;
        }

        /*(4) Commit system drives only after loading the system configuration from raw-mirror so
         *    the geometry of the system PVDs would be calculated by the correct configuration.
         */        
        status = database_system_objects_commit_system_drives(database_service_ptr, FBE_FALSE);        
        if (status != FBE_STATUS_OK){
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to commit system drives in non ICA case\n", __FUNCTION__);
            return status;
        }

        /* create bvd interface object */
        status = database_system_objects_create_object_and_its_edges(database_service_ptr, FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_BVD_INTERFACE, FBE_FALSE);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: fail to create BVD interface in non ICA case\n", __FUNCTION__);
            return status;
        }
    }

    /* Create export luns for mirror */
    status = database_system_objects_create_export_lun_objects(&fbe_database_service, system_disk_table);
    if (status != FBE_STATUS_OK) {
	    database_trace(FBE_TRACE_LEVEL_ERROR, 
			    FBE_TRACE_MESSAGE_ID_INFO, 
			    "%s: failed to create export lun objects object\n", __FUNCTION__);
	    return status;
    }

    /* starts initial discovery.*/
    fbe_database_drive_discover(&database_service_ptr->pvd_operation);

    /* open to transaction request */
    if (database_service_ptr->state != FBE_DATABASE_STATE_CORRUPT) {
        set_database_service_state(database_service_ptr, FBE_DATABASE_STATE_READY);
    }

    /*start the drive process thread right after database becomes READY*/
    fbe_database_drive_connection_start_drive_process(database_service_ptr);

    database_trace(FBE_TRACE_LEVEL_INFO,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: end\n", __FUNCTION__);
    return FBE_STATUS_OK;
}
