/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_database_system_objects_manager.c
***************************************************************************
*
* @brief
*  This file contains system objects related code.
*  
* @version
*  04/06/2011 - Created. 
*
***************************************************************************/
#include "fbe_database_common.h"
#include "fbe_database_private.h"
#include "fbe_database_drive_connection.h"
#include "fbe_private_space_layout.h"
#include "fbe_database_config_tables.h"
#include "fbe/fbe_board.h"
#include "fbe_raid_library.h"
#include "fbe/fbe_notification_lib.h"
#include "fbe_database_system_db_interface.h"
#include "fbe_database_homewrecker_db_interface.h"
#include "fbe_private_space_layout.h"
#include "fbe/fbe_event_log_api.h"
#include "fbe/fbe_event_log_utils.h"


#define DATABASE_SYS_DRIVE_BUS 0
#define DATABASE_SYS_DRIVE_ENCLOSURE 0

#define DATABASE_DEFAULT_PVD_IMPORTED_SIZE  fbe_private_space_get_minimum_system_drive_size()
#define DATABASE_SYSTEM_PVD_METADATA_SIZE   (fbe_private_space_get_minimum_system_drive_size() - fbe_private_space_get_capacity_imported_from_pvd())
#define DATABASE_SYSTEM_VD_METADATA_SIZE 0

typedef struct database_system_object_notification_context_s
{
    fbe_object_id_t system_object_id;
    fbe_lifecycle_state_t expected_state;
    fbe_semaphore_t sem;
}database_system_object_notification_context_t;

typedef struct fbe_database_company_id_s{
    unsigned char bytes[4];
} fbe_database_company_id_t;

typedef struct fbe_database_controller_id_s{
    unsigned char bytes[5];
} fbe_database_controller_id_t;


extern fbe_raw_mirror_t fbe_metadata_nonpaged_raw_mirror;
extern fbe_raw_mirror_t fbe_database_system_db;
extern fbe_raw_mirror_t fbe_database_homewrecker_db;
extern fbe_database_service_t fbe_database_service;

/* Todo: using one notification for DB lun now.  Should be revisited when increasing the number of system objects to monitor */
static fbe_notification_element_t database_system_object_notification_element;
static database_system_object_notification_context_t database_system_object_notification_context;

static fbe_status_t database_system_connect_physical_drive(database_object_entry_t *in_table_ptr,
                                            fbe_u32_t *low_capacity_drives);
static fbe_status_t database_system_objects_notification_callback(fbe_object_id_t object_id, 
        	 								                       fbe_notification_info_t notification_info,
        												           fbe_notification_context_t context);
static fbe_status_t database_system_objects_register_notification(fbe_object_id_t obejct_id, 
                                                                  fbe_lifecycle_state_t expected_state,
																  fbe_topology_object_type_t object_type);

static fbe_status_t database_system_objects_unregister_notification(void);
static fbe_status_t database_system_objects_create_rg_from_psl(fbe_database_service_t *fbe_database_service,
                                                          fbe_object_id_t object_id, 
                                                          fbe_private_space_layout_region_t *region);
static fbe_status_t database_system_objects_generate_rg_config_from_psl(fbe_database_service_t *fbe_database_service,
                                                          fbe_object_id_t object_id, 
                                                          fbe_private_space_layout_region_t *region);
static database_class_id_t convert_psl_raid_type_to_class_id(fbe_u32_t raid_type);


static fbe_status_t psl_lun_find_rg_object_id(fbe_raid_group_number_t raid_group_id, fbe_object_id_t *rg_object_id);

fbe_status_t fbe_database_init_raw_mirror_edges_on_db_drive(void);

fbe_status_t database_system_objects_create_lun_from_psl(fbe_database_service_t *fbe_database_service,
                                                                fbe_object_id_t object_id, 
                                                                fbe_private_space_layout_lun_info_t *lun_info,
                                                                fbe_bool_t invalidate);

/*!***************************************************************
 * @fn fbe_database_init_raw_mirror_edges_on_db_drive
 *****************************************************************
 * @brief
 *   initialize the edges of raw-mirror on db drive.
 *
 * @param N/A.
 *
 * @return failure if can't get edge
 *
 * @version
 *    03/12/2012:   created by Jingcheng Zhang to fix DE505
 *    04/20/2012:   add homewrecker db init edges. by Jian Gao
 *
 ****************************************************************/
fbe_status_t fbe_database_init_raw_mirror_edges_on_db_drive(void)
{
    fbe_status_t status = FBE_STATUS_OK;

    status = fbe_raw_mirror_init_edges(&fbe_metadata_nonpaged_raw_mirror);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                       "Database failed to initialize the edges for metadata raw mirror\n");
        return status;
    }
    status = fbe_raw_mirror_init_edges(&fbe_database_system_db);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                       "Database failed to initialize the edges for system db raw mirror\n");
    }
    status = fbe_raw_mirror_init_edges(&fbe_database_homewrecker_db);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                       "Database failed to initialize the edges for system db raw mirror\n");
    }

    return status;
}

fbe_status_t fbe_database_switch_to_raw_mirror_object(void)
{
    /* Disable the raw mirror library and switch over to raw mirror object.
     */
    fbe_status_t status;

    /* wait for NP lun before setting up persist service */
    status = fbe_database_wait_for_object_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MCR_RAW_NP,  FBE_LIFECYCLE_STATE_READY, DATABASE_WAIT_FOR_OBJECT_STATE_TIME);
    if(status != FBE_STATUS_OK){
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                       "%s: RAW NP Lun not ready in 120 secs, status %d!\n", 
                       __FUNCTION__, status);
        return status;
    }
    status = fbe_metadata_nonpaged_disable_raw_mirror();
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                       "Database failed to disable NP raw mirror\n");
    }
    /* wait for RAW SYS CONFIG lun before setting up persist service */
    status = fbe_database_wait_for_object_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MCR_RAW_SYSTEM_CONFIG,  FBE_LIFECYCLE_STATE_READY, DATABASE_WAIT_FOR_OBJECT_STATE_TIME);
    if(status != FBE_STATUS_OK){
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                       "%s: RAW SYS CONFIG Lun not ready in 120 secs, status %d!\n", 
                       __FUNCTION__, status);
        return status;
    }
    status = fbe_raw_mirror_disable(&fbe_database_system_db);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                       "Database failed to disable sys db raw mirror\n");
    }
    return status;
}


/*!***************************************************************
 * @fn database_system_connect_physical_drive
 *****************************************************************
 * @brief
 *   connect pdo and pvd.  MUST be called after the PVDs are created.
 *
 * @param in_table_ptr - pointer to object table.
 *
 * @return none
 *
 * @version
 *    04/11/2011:   created
 *
 ****************************************************************/
fbe_status_t database_system_connect_physical_drive(database_object_entry_t *in_table_ptr,
                                            fbe_u32_t *low_capacity_drives)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_physical_drive_info_t *physical_drive_entry = NULL; 
    fbe_lba_t pvd_exported_capacity;
    fbe_u32_t i;
    fbe_u32_t low_cap_drv_num;
    database_object_entry_t * object_entry = NULL;
    fbe_u32_t database_system_drives_num = fbe_private_space_layout_get_number_of_system_drives();
    fbe_block_count_t minimum_sys_drive_capacity = fbe_private_space_get_minimum_system_drive_size();

    fbe_bool_t  active_side = database_common_cmi_is_active();
    
    /* system object tables */
    fbe_database_physical_drive_info_t *system_physical_drive_table = NULL;
    
    system_physical_drive_table = 
        (fbe_database_physical_drive_info_t *)fbe_memory_native_allocate(database_system_drives_num*sizeof(fbe_database_physical_drive_info_t));
    if (system_physical_drive_table==NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: database_memory allocate failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    low_cap_drv_num = 0;
    for (i = 0; i < database_system_drives_num; i ++) {

        if(active_side)
        {
            /*active side should connect to pdo by location and update pvd object entry by the pdo information*/
            physical_drive_entry = &system_physical_drive_table[i];
            physical_drive_entry->bus = DATABASE_SYS_DRIVE_BUS;
            physical_drive_entry->enclosure = DATABASE_SYS_DRIVE_ENCLOSURE;
            physical_drive_entry->slot = i;
            status = fbe_database_connect_to_pdo_by_location(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + i, 
                                                         physical_drive_entry->bus,
                                                         physical_drive_entry->enclosure,
                                                         physical_drive_entry->slot,
                                                         physical_drive_entry);
            if (status == FBE_STATUS_OK) {
                /*check whether the system drive capacity big enough to hold the PSL or not*/
                if (physical_drive_entry->exported_capacity < minimum_sys_drive_capacity) {
                    low_cap_drv_num ++;
                    database_trace(FBE_TRACE_LEVEL_ERROR, 
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "The system drive 0_0_%d size %x < PSL required size %x \n",
                                   i, (unsigned int)physical_drive_entry->exported_capacity, 
                                   (unsigned int)minimum_sys_drive_capacity);
                }

                /* Update the PVD set config with the info from LDO */
                object_entry = &in_table_ptr[FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + i];
                status = fbe_database_common_get_provision_drive_exported_size(physical_drive_entry->exported_capacity,
                                                                               physical_drive_entry->block_geometry,
                                                                               &pvd_exported_capacity);
                object_entry->set_config_union.pvd_config.configured_capacity = pvd_exported_capacity;
                if (status != FBE_STATUS_OK) {
                    object_entry->set_config_union.pvd_config.configured_capacity = physical_drive_entry->exported_capacity 
                                                                                    - DATABASE_SYSTEM_PVD_METADATA_SIZE; 
                    database_trace(FBE_TRACE_LEVEL_WARNING, 
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: failed to calculate PVD exported_capacity, set to default value 0x%llx, Status: 0x%x.\n", 
                                   __FUNCTION__, 
                                   object_entry->set_config_union.pvd_config.configured_capacity, 
                                   status);
                }
                /* this convertion does not make sense, shouldn't the block_geometry be just populate up? */
                if ( physical_drive_entry->block_geometry == FBE_BLOCK_EDGE_GEOMETRY_520_520 )
                {
                    object_entry->set_config_union.pvd_config.configured_physical_block_size = FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_520;
                }
                else if ( physical_drive_entry->block_geometry == FBE_BLOCK_EDGE_GEOMETRY_4160_4160 )
                {
                    object_entry->set_config_union.pvd_config.configured_physical_block_size = FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_4160;
                }
                else
                {
                    object_entry->set_config_union.pvd_config.configured_physical_block_size = FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_INVALID;
                    database_trace(FBE_TRACE_LEVEL_WARNING, 
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: Invalid block size for attached to pdo %u\n", __FUNCTION__, physical_drive_entry->drive_object_id);
                }
                
                fbe_copy_memory(object_entry->set_config_union.pvd_config.serial_num, &physical_drive_entry->serial_num, sizeof(object_entry->set_config_union.pvd_config.serial_num));
            
                /* we also need to update VD capacity */
                object_entry = &in_table_ptr[FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VD_FIRST + i];
                status = fbe_database_common_get_virtual_drive_exported_size(pvd_exported_capacity, /* exported_capacity from PVD*/
                                                                             &object_entry->set_config_union.vd_config.capacity);
                if (status == FBE_STATUS_GENERIC_FAILURE) {
                    object_entry->set_config_union.vd_config.capacity = (pvd_exported_capacity 
                                                                                                - DATABASE_SYSTEM_VD_METADATA_SIZE);
                    database_trace(FBE_TRACE_LEVEL_WARNING, 
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: failed to calculate VD exported_capacity, set to default value 0x%llx, Status: 0x%x.\n", 
                                   __FUNCTION__, 
                                   object_entry->set_config_union.vd_config.capacity, 
                                   status);
                }
            
            }            

        }
        else
        {
            /* Passive side just connects pdo by serial number, which is recorded in the pvd object entry
          * There is no need to update the pvd entry because the entry is received from active side
          * so it has already been updated based on downstream pdo's information*/
            database_object_entry_t * object_entry = NULL;
            object_entry = &in_table_ptr[FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + i];

            physical_drive_entry = &system_physical_drive_table[i];
            physical_drive_entry->bus = DATABASE_SYS_DRIVE_BUS;
            physical_drive_entry->enclosure = DATABASE_SYS_DRIVE_ENCLOSURE;
            physical_drive_entry->slot = i;
            
            status = fbe_database_connect_to_pdo_by_serial_number(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + i,
                                                                        object_entry->set_config_union.pvd_config.serial_num,
                                                                        physical_drive_entry);
            if(FBE_STATUS_OK != status)
	    {
		    /*It is OK that this PVD does not connect to LDO for two reasons:
		     * (1) the drive is really not there
		     * (2) the homewrecker logic running on active side decides that it is a new drive, so it invalidates
		     *      the SN of the PVD to block connecting to LDO
		     * (3) the passive sp's system PVD is trying to connect the wrong position drive*/
		    database_trace(FBE_TRACE_LEVEL_WARNING, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: failed to connect PVD 0x%x with downstream LDO, which is accepted by our logic.\n", 
                               __FUNCTION__, 
                               FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + i);
            } else {
                /*check whether the system drive capacity big enough to hold the PSL or not*/
                if (physical_drive_entry->exported_capacity < minimum_sys_drive_capacity) {
                    low_cap_drv_num ++;
                    database_trace(FBE_TRACE_LEVEL_ERROR, 
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "The system drive 0_0_%d size %x < PSL required size %x \n",
                                   i, (unsigned int)physical_drive_entry->exported_capacity, 
                                   (unsigned int)minimum_sys_drive_capacity);
                }
            }

        }

    }

    if (low_capacity_drives != NULL) {
        *low_capacity_drives = low_cap_drv_num;
    }
    
    fbe_memory_native_release(system_physical_drive_table);
    system_physical_drive_table = NULL;
    return FBE_STATUS_OK;
}


void populate_system_drives_user_table(database_table_t *in_table_ptr)
{
    database_user_entry_t * user_entry_ptr = NULL;
    fbe_u32_t i;

    fbe_u32_t   database_system_drives_num = 0;
    database_system_drives_num = fbe_private_space_layout_get_number_of_system_drives();

    /* validate the table */
    if (in_table_ptr->table_size < (FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VD_FIRST + database_system_drives_num)) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: user table is too small. Table size 0x%x < num data entry 0x%x.\n",
                       __FUNCTION__, 
                       in_table_ptr->table_size,
                       (FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VD_FIRST + database_system_drives_num));
        return;
    }

    
    /* populate system pvds' user entry */
    user_entry_ptr = &in_table_ptr->table_content.user_entry_ptr[FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST];
    for (i = 0; i < database_system_drives_num; i++) {
		
		/* Initiate and Initialize the user entries for PVD */
        fbe_zero_memory(user_entry_ptr, sizeof(database_user_entry_t));
		user_entry_ptr->header.entry_id = FBE_PERSIST_SECTOR_START_ENTRY_ID;
		user_entry_ptr->header.object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + i;
		user_entry_ptr->header.state = DATABASE_CONFIG_ENTRY_VALID;
		user_entry_ptr->db_class_id = DATABASE_CLASS_ID_PROVISION_DRIVE;
        user_entry_ptr->header.version_header.size = database_common_user_entry_size(user_entry_ptr->db_class_id);
		user_entry_ptr->user_data_union.pvd_user_data.pool_id = FBE_POOL_ID_INVALID;
        user_entry_ptr++;
	}
    return;
}

void populate_system_drives_object_table(database_table_t *in_table_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    database_object_entry_t * object_entry = NULL;
    fbe_u32_t i;
    
    fbe_u32_t   database_system_drives_num = 0;
    database_system_drives_num = fbe_private_space_layout_get_number_of_system_drives();

    /* validate the table */
    if (in_table_ptr->table_size < (FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VD_FIRST + database_system_drives_num)) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: object table is too small. Table size 0x%x < num data entry 0x%x.\n",
                       __FUNCTION__, 
                       in_table_ptr->table_size,
                       (FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VD_FIRST + database_system_drives_num));
        return;
    }

    /* bvd interface */
    object_entry = &in_table_ptr->table_content.object_entry_ptr[FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_BVD_INTERFACE];
    fbe_zero_memory(object_entry, sizeof(database_object_entry_t));
    object_entry->header.state = DATABASE_CONFIG_ENTRY_VALID;
    object_entry->header.entry_id = FBE_PERSIST_SECTOR_START_ENTRY_ID;
    object_entry->header.object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_BVD_INTERFACE;
    object_entry->db_class_id = DATABASE_CLASS_ID_BVD_INTERFACE;
    object_entry->header.version_header.size =  database_common_object_entry_size(DATABASE_CLASS_ID_BVD_INTERFACE);
    
    /* pvds */
    object_entry = &in_table_ptr->table_content.object_entry_ptr[FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST];
    for (i = 0; i < database_system_drives_num; i++) {
        fbe_zero_memory(object_entry, sizeof(database_object_entry_t));
        object_entry->header.state = DATABASE_CONFIG_ENTRY_VALID;
        object_entry->header.entry_id = FBE_PERSIST_SECTOR_START_ENTRY_ID;
        object_entry->header.object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + i;
        object_entry->db_class_id = DATABASE_CLASS_ID_PROVISION_DRIVE;
        object_entry->header.version_header.size = database_common_object_entry_size(DATABASE_CLASS_ID_PROVISION_DRIVE);
        if (object_entry->header.object_id <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_LAST) {
            object_entry->set_config_union.pvd_config.config_type = FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED;
        }else{
            object_entry->set_config_union.pvd_config.config_type = FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED; /* init the system spare object as unused PVD */
        }
        /* By default sniffing is enabled on all drives.
         */
        object_entry->set_config_union.pvd_config.sniff_verify_state = FBE_TRUE;
        status = fbe_database_common_get_provision_drive_exported_size(DATABASE_DEFAULT_PVD_IMPORTED_SIZE,
                                                                       FBE_BLOCK_EDGE_GEOMETRY_520_520,
                                                                       &object_entry->set_config_union.pvd_config.configured_capacity);
        if (status == FBE_STATUS_GENERIC_FAILURE) {
            object_entry->set_config_union.pvd_config.configured_capacity = (DATABASE_DEFAULT_PVD_IMPORTED_SIZE
                                                                             - DATABASE_SYSTEM_PVD_METADATA_SIZE);
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: failed to calculate VD exported_capacity, set to default value 0x%llx, Status: 0x%x.\n", 
                           __FUNCTION__, 
                           (unsigned long long)object_entry->set_config_union.pvd_config.configured_capacity, 
                           status);
        }
        object_entry->set_config_union.pvd_config.configured_physical_block_size = FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_INVALID; /*! @todo  get from LDO */
        object_entry->set_config_union.pvd_config.generation_number = object_entry->header.object_id; /*! @todo  What should be here? */
        fbe_zero_memory(object_entry->set_config_union.pvd_config.serial_num, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);

        object_entry++;
    }

}

/* When we reload all the system object entries, the edges from pvd to vd need to be updated again */
void update_system_pvd_to_vd_edge_table(database_table_t *in_edge_table_ptr, database_table_t * in_object_table_ptr)
{
    database_edge_entry_t * edge_entry = NULL;
    database_object_entry_t * pvd_object_entry = NULL;
    fbe_u32_t i, client_index;
    fbe_u32_t   database_system_drives_num = 0;

    if ((in_edge_table_ptr == NULL) || in_object_table_ptr == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: invalid arguments \n",
                       __FUNCTION__);
        return;
    }
   
    database_system_drives_num = fbe_private_space_layout_get_number_of_system_drives();
    /* update edges that connect pvd to vd */
    for (i = 0; i < database_system_drives_num; i++) {
        edge_entry = &in_edge_table_ptr->table_content.edge_entry_ptr[(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VD_FIRST + i) * DATABASE_MAX_EDGE_PER_OBJECT];
        /* get the pvd capacity for the object table, by now, PVD is connected with pdo and has the proper capacity set */
        for (client_index = 0; client_index < DATABASE_MAX_EDGE_PER_OBJECT; client_index++, edge_entry++ ) {
            /* don't update the invalid edges */
            if (edge_entry->header.state != DATABASE_CONFIG_ENTRY_VALID) {
                continue;
            }
            /* make sure the server_id belong to PVDs or SPARE_PVDs */
            if (fbe_database_is_object_system_pvd(edge_entry->server_id)==FBE_FALSE) {
                database_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: The server_id should be in [PVD_0, SPARE_PVD3], but server_id = 0x%x\n",
                               __FUNCTION__, 
                               edge_entry->server_id);
                continue;
            }
            /* update the values with attributes of its server */
            fbe_database_config_table_get_object_entry_by_id(in_object_table_ptr, edge_entry->server_id, &pvd_object_entry);
            if ((pvd_object_entry->header.state != DATABASE_CONFIG_ENTRY_VALID)
                &&(pvd_object_entry->db_class_id != DATABASE_CLASS_ID_PROVISION_DRIVE))
            {
                database_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: PVD 0x%x does not have a valid object entry in database.\n",
                               __FUNCTION__, 
                               edge_entry->server_id);
                edge_entry->capacity = DATABASE_DEFAULT_PVD_IMPORTED_SIZE - DATABASE_SYSTEM_PVD_METADATA_SIZE;
                database_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: Force the edge capacity between pvd %d - vd %d to 0x%llx.\n",
                               __FUNCTION__, 
                               edge_entry->server_id,
                               edge_entry->header.object_id,
                               (unsigned long long)edge_entry->capacity);
            } else {  /* we got a valid pvd object entry, let's set the edge capacity to it's exported capacity*/
                edge_entry->capacity = pvd_object_entry->set_config_union.pvd_config.configured_capacity;
            }
        }
    }
    return ;
}

void populate_system_pvd_to_vd_edge_table(database_table_t *in_edge_table_ptr, database_table_t *in_object_table_ptr)
{
    database_edge_entry_t * edge_entry = NULL;
    database_object_entry_t * pvd_object_entry = NULL;
    fbe_u32_t i;
    
    fbe_u32_t   database_system_drives_num = 0;
    database_system_drives_num = fbe_private_space_layout_get_number_of_system_drives();
  
    /* validate the table */
    if (in_edge_table_ptr->table_size < (FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VD_FIRST + database_system_drives_num)*DATABASE_MAX_EDGE_PER_OBJECT) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: object table is too small. Table size 0x%x < num data entry 0x%x.\n",
                       __FUNCTION__, 
                       in_edge_table_ptr->table_size,
                       (FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VD_FIRST + database_system_drives_num));
        return;
    }

    /* edges between pvd and pdo are created by now */

    /* pvd to vd */
    edge_entry = &in_edge_table_ptr->table_content.edge_entry_ptr[FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VD_FIRST*DATABASE_MAX_EDGE_PER_OBJECT];
    for (i = 0; i < database_system_drives_num; i++) {
        fbe_zero_memory(edge_entry, sizeof(database_edge_entry_t));
        edge_entry->header.state = DATABASE_CONFIG_ENTRY_VALID;
        edge_entry->header.entry_id = FBE_PERSIST_SECTOR_START_ENTRY_ID;
        edge_entry->header.object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VD_FIRST + i;
        edge_entry->header.version_header.size = database_common_edge_entry_size();
        edge_entry->client_index = 0;
        edge_entry->offset = 0;
        edge_entry->server_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + i; 

        /* get the pvd capacity for the object table, by now, PVD is connected with pdo and has the proper capacity set */
        fbe_database_config_table_get_object_entry_by_id(in_object_table_ptr, edge_entry->server_id, &pvd_object_entry);
        if ((pvd_object_entry->header.state != DATABASE_CONFIG_ENTRY_VALID)
            &&(pvd_object_entry->db_class_id != DATABASE_CLASS_ID_PROVISION_DRIVE))
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: PVD 0x%x does not have a valid object entry in database.\n",
                           __FUNCTION__, 
                           edge_entry->server_id);
            edge_entry->capacity = DATABASE_DEFAULT_PVD_IMPORTED_SIZE - DATABASE_SYSTEM_PVD_METADATA_SIZE;
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: Force the edge capacity between pvd %d - vd %d to 0x%llx.\n",
                           __FUNCTION__, 
                           edge_entry->server_id,
                           edge_entry->header.object_id,
                           (unsigned long long)edge_entry->capacity);
        }
        else{  /* we got a valid pvd object entry, let's set the edge capacity to it's exported capacity*/
            edge_entry->capacity = pvd_object_entry->set_config_union.pvd_config.configured_capacity;
        }
        /* move on to the next edge entry*/
        edge_entry = edge_entry + DATABASE_MAX_EDGE_PER_OBJECT;
    }
}

/*!***************************************************************
 * @fn database_system_objects_create_and_commit_system_spare_drives
 *****************************************************************
 * @brief
 *   create system spare drives
 *
 * @param  
 *
 * @return fbe_status_t - status
 *
 * @version
 *   10/14/2011:   created
 *
 ****************************************************************/
fbe_status_t database_system_objects_create_and_commit_system_spare_drives()
{
    database_table_t *table_ptr = NULL;
    database_object_entry_t *current_entry = NULL;
    fbe_database_physical_drive_info_t pdo_info;
    fbe_status_t status;
    fbe_u32_t i;
    
    fbe_u32_t   database_system_drives_num = 0;
    database_system_drives_num = fbe_private_space_layout_get_number_of_system_drives();

    status = fbe_database_get_service_table_ptr(&table_ptr, DATABASE_CONFIG_TABLE_OBJECT);
    /* create spare pvds */
    status = database_common_create_object_from_table(table_ptr, 
                                                      FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_SPARE_FIRST, 
                                                      (FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_SPARE_LAST - FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_SPARE_FIRST + 1));
    /* connect physical drive */
    for (i = 0; i < database_system_drives_num; i++) {
        current_entry = &table_ptr->table_content.object_entry_ptr[FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_SPARE_FIRST + i];
        if (current_entry->header.state != DATABASE_CONFIG_ENTRY_VALID) { 
            /* not a valid entry, skip it*/
            /* move down to the next entry*/
            current_entry++;
            continue;
        }
        status = fbe_database_connect_to_pdo_by_serial_number(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_SPARE_FIRST+i,
                                                              current_entry->set_config_union.pvd_config.serial_num,
                                                              &pdo_info
                                                              );
    }
    status = database_common_set_config_from_table(table_ptr, 
                                                   FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_SPARE_FIRST, 
                                                   (FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_SPARE_LAST - FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_SPARE_FIRST + 1));

    /* commit spare pvds */
    status = database_common_commit_object_from_table(table_ptr, 
                                                      FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_SPARE_FIRST, 
                                                      (FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_SPARE_LAST - FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_SPARE_FIRST + 1),
                                                      FBE_FALSE);

    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_CREATE_OBJECT, "%s: spare pvd connected drives\n", __FUNCTION__);
    return status;
}


/*!***************************************************************
 * @fn database_system_objects_create_system_drives
 *****************************************************************
 * @brief
 *   create system drives
 *
 * @param  database_service - pointer to the database service
 *
 * @return fbe_status_t - status
 *
 * @version
 *    04/06/2011:   created
 *
 ****************************************************************/
fbe_status_t database_system_objects_create_system_drives(fbe_database_service_t *fbe_database_service)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t   low_capacity_drives = 0;

    fbe_bool_t	active_side = database_common_cmi_is_active();

    if (active_side) {
        populate_system_drives_object_table(&fbe_database_service->object_table);
        populate_system_drives_user_table(&fbe_database_service->user_table);
	}
    
    status = database_common_create_object_from_table(&fbe_database_service->object_table, 
                                                     FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST, 
                                                     (FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_LAST-FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST+1));

    status = database_system_connect_physical_drive(fbe_database_service->object_table.table_content.object_entry_ptr,
                                                    &low_capacity_drives);

    if (status == FBE_STATUS_OK && low_capacity_drives > 0) {
        /* 
          If system drive capacity less than PSL requiring, system raid group can't be 
          created on it. so enter service mode.
        */
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "Fail to create system drives, %d drives smaller than PSL required, enter degraded mode \n",
                       low_capacity_drives);
		fbe_event_log_write(SEP_ERROR_CODE_TOO_SMALL_DB_DRIVE,
							NULL, 0,
							"%d",
							low_capacity_drives);
		fbe_database_enter_service_mode(FBE_DATABASE_SERVICE_MODE_REASON_SMALL_SYSTEM_DRIVE);
        return FBE_STATUS_SERVICE_MODE;
    }


    status = database_common_set_config_from_table(&fbe_database_service->object_table, 
                                                      FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST, 
                                                      (FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_LAST-FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST+1));

    status = fbe_database_init_raw_mirror_edges_on_db_drive();

    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_CREATE_OBJECT, "%s: connected drives\n", __FUNCTION__);

    return status;
}

/*!***************************************************************
 * @fn database_system_objects_commit_system_drives
 *****************************************************************
 * @brief
 *   create system drives
 *
 * @param  database_service - pointer to the database service
 * @param  is_initial_creation - whether the objects are created for the first time.
 *
 * @return fbe_status_t - status
 *
 * @version
 *    04/06/2011:   created
 *
 ****************************************************************/
fbe_status_t database_system_objects_commit_system_drives(fbe_database_service_t *fbe_database_service, fbe_bool_t is_initial_creation)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
 
    status = database_common_commit_object_from_table(&fbe_database_service->object_table, 
                                                      FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST, 
                                                     (FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_LAST-FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST+1),
                                                     is_initial_creation);

    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "%s: Status 0x%x\n", __FUNCTION__, status);
    return status;
}


/*!***************************************************************
 * @fn database_system_objects_register_notification
 *****************************************************************
 * @brief
 *   register with the notification service for the system objects that we are interested in.
 *
 * @param  
 *
 * @return fbe_status_t - status
 *
 * @version
 *    04/11/2011:   created
 *
 ****************************************************************/
static fbe_status_t database_system_objects_register_notification(fbe_object_id_t obejct_id,
																  fbe_lifecycle_state_t expected_state,
																  fbe_topology_object_type_t object_type)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    /* Todo: using one notification for DB lun now.  Should be revisited when increasing the number of system objects to monitor */
    /* settin up the context*/
    database_system_object_notification_context.system_object_id = obejct_id;
    database_system_object_notification_context.expected_state = expected_state;
    fbe_semaphore_init( &database_system_object_notification_context.sem, 0, 1);
    /* settin up the notification_element*/
    database_system_object_notification_element.notification_function = database_system_objects_notification_callback;
    database_system_object_notification_element.notification_context = &database_system_object_notification_context;
    database_system_object_notification_element.targe_package = FBE_PACKAGE_ID_SEP_0;
    database_system_object_notification_element.object_type = object_type;

	fbe_notification_convert_state_to_notification_type(expected_state ,
														&database_system_object_notification_element.notification_type);

    status = database_common_register_notification ( &database_system_object_notification_element,
                                                     FBE_PACKAGE_ID_SEP_0);

    return status;
}

/*!***************************************************************
 * @fn database_system_objects_notification_callback
 *****************************************************************
 * @brief
 *   callback function for the system objects.
 *
 * @param  object_id - the id of the object that generates the notification
 * @param  notification_info - info of the notification
 * @param  context - context passed in at the timeof registering the notification.
 *
 * @return fbe_status_t - status
 *
 * @version
 *    04/11/2011:   created
 *
 ****************************************************************/
static fbe_status_t database_system_objects_notification_callback(fbe_object_id_t object_id, 
        	 								                       fbe_notification_info_t notification_info,
        												           fbe_notification_context_t context)
{
    database_system_object_notification_context_t * ns_context = NULL;
	fbe_lifecycle_state_t state;
	fbe_status_t status;

    ns_context = (database_system_object_notification_context_t *)context;

    if ((ns_context->system_object_id == object_id)
        &&(notification_info.notification_type & FBE_NOTIFICATION_TYPE_LIFECYCLE_ANY_STATE_CHANGE)){

		status = fbe_notification_convert_notification_type_to_state(notification_info.notification_type, &state);
		if (status != FBE_STATUS_OK) {
			database_trace(FBE_TRACE_LEVEL_ERROR, 
						   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
						   "%s: can't map 0x%llX, lost notification\n", __FUNCTION__, (unsigned long long)notification_info.notification_type);
			return FBE_STATUS_GENERIC_FAILURE;
		}
	
        if (ns_context->expected_state == state){
			fbe_semaphore_release(&ns_context->sem, 0, 1, FBE_FALSE);
		}
    }
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn database_system_objects_unregister_notification
 *****************************************************************
 * @brief
 *   unregister with the notification service for the system objects that we are interested in.
 *
 * @param  
 *
 * @return fbe_status_t - status
 *
 * @version
 *    04/11/2011:   created
 *
 ****************************************************************/
static fbe_status_t database_system_objects_unregister_notification(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    status = database_common_unregister_notification ( &database_system_object_notification_element,
                                                       FBE_PACKAGE_ID_SEP_0);

    database_system_object_notification_element.notification_function = NULL;
    database_system_object_notification_element.notification_context = NULL;
    database_system_object_notification_element.registration_id = FBE_PACKAGE_ID_INVALID;
    fbe_semaphore_destroy( &database_system_object_notification_context.sem);

    return status;
}

/*!***************************************************************
 * @fn database_system_objects_get_db_lun_id()
 *****************************************************************
 * @brief
 *   get the object id of database lun.
 *
 * @param  
 *
 * @return fbe_status_t - status
 *
 * @version
 *    04/11/2011:   created
 *
 ****************************************************************/
fbe_status_t database_system_objects_get_db_lun_id(fbe_object_id_t *object_id)
{
    *object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MCR_DATABASE;

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn database_system_objects_get_db_lun_capacity()
 *****************************************************************
 * @brief
 *   get the capacity of database lun.
 *
 * @param  
 *
 * @return fbe_status_t - status
 *
 * @version
 *    04/11/2011:   created
 *
 ****************************************************************/
fbe_lba_t database_system_objects_get_db_lun_capacity()
{
    fbe_private_space_layout_lun_info_t lun_info;
	fbe_status_t	status;

    status = fbe_private_space_layout_get_lun_by_object_id(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MCR_DATABASE, &lun_info);
	if (status == FBE_STATUS_OK) {
		return lun_info.external_capacity;
	} else {
		return 0;
	}
}

/*!***************************************************************
 * @fn database_system_objects_enumerate_objects()
 *****************************************************************
 * @brief
 *   enumerate the system object ids.
 *
 * @param  object_ids_p - the buffer to fill with the object_id
 * @param  result_list_size - size of the buffer to fill
 * @param  num_objects_p - number of object copied
 *
 * @return fbe_status_t - status
 *
 * @version
 *    04/27/2011:   created
 *
 ****************************************************************/
fbe_status_t database_system_objects_enumerate_objects(fbe_object_id_t *object_ids_p, 
                                                       fbe_u32_t result_list_size,
                                                       fbe_u32_t *num_objects_p)

{
    fbe_status_t     status = FBE_STATUS_OK;
    fbe_object_id_t *object_id_p = NULL;
    fbe_u32_t        i;
    fbe_u32_t        count = 0;
    fbe_object_id_t  last_system_object_id;

    fbe_database_get_last_system_object_id(&last_system_object_id);

    /* Populate the list passed.
     */
    object_id_p = object_ids_p;
    for (i = 0; i <= last_system_object_id; i++)
    {
        if (count > result_list_size)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (fbe_private_space_layout_object_id_is_system_object(i))
        {

            *object_id_p++ = i;
            count++;
        }
    }

    *num_objects_p = count;

    return status;
}

/*!***************************************************************
 * @fn database_system_objects_get_count()
 *****************************************************************
 * @brief
 *   the the number of system objects.
 *
 * @param  system_object_count - (OUT) the number of system objects
 *
 * @return fbe_status_t - status
 *
 * @version
 *    04/27/2011:   created
 *
 ****************************************************************/
fbe_status_t database_system_objects_get_expected_count(fbe_u32_t *system_object_count)
{
    fbe_status_t     status = FBE_STATUS_OK;
    fbe_u64_t        current_commit_level;

    fbe_spinlock_lock(&fbe_database_service.system_db_header_lock);
    if(fbe_database_service.use_transient_commit_level)
    {
        current_commit_level = fbe_database_service.transient_commit_level;
    }
    else 
    {
        current_commit_level = fbe_database_service.system_db_header.persisted_sep_version;
    }
    fbe_spinlock_unlock(&fbe_database_service.system_db_header_lock);

    status = fbe_private_space_layout_get_number_of_system_objects(system_object_count, current_commit_level);

    return status;
}

fbe_status_t database_system_objects_get_reserved_count(fbe_u32_t *system_object_count)
{
    fbe_status_t     status = FBE_STATUS_OK;
    *system_object_count = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LAST + 1;
    return status;
}

fbe_status_t fbe_database_get_system_pvd(fbe_u32_t pvd_index, fbe_object_id_t *object_id_p)
{
    fbe_u32_t   database_system_drives_num = 0;
    database_system_drives_num = fbe_private_space_layout_get_number_of_system_drives();

    if (pvd_index >= database_system_drives_num)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: failed to get object id for pvd index %d\n", __FUNCTION__, pvd_index);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *object_id_p = (FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + pvd_index);
    return FBE_STATUS_OK;
}


fbe_status_t fbe_database_get_metadata_lun_id(fbe_object_id_t * object_id)
{
    * object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAID_METADATA;
    return FBE_STATUS_OK;
}
fbe_status_t fbe_database_get_raw_mirror_rg_id(fbe_object_id_t * object_id)
{
    * object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAW_MIRROR_RG;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_database_get_raw_mirror_metadata_lun_id(fbe_object_id_t * object_id)
{
    fbe_database_state_t db_state;

    get_database_service_state(&fbe_database_service, &db_state);

    /* accessing state directly to avoid function call penalty */
    if (db_state != FBE_DATABASE_STATE_DESTROYING_SYSTEM)
    {
        * object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MCR_RAW_NP;
    }
    else
    {
        database_trace(FBE_TRACE_LEVEL_INFO, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: database is destroying metadata lun, returning invalid object id.\n", __FUNCTION__);
        * object_id = FBE_OBJECT_ID_INVALID;
    }
    return FBE_STATUS_OK;
}
fbe_status_t fbe_database_get_raw_mirror_config_lun_id(fbe_object_id_t * object_id)
{
    * object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MCR_RAW_SYSTEM_CONFIG;
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn database_check_and_make_up_system_object_configuration
 *****************************************************************
 * @brief
 *   check whether there is missing system object, which are defined in PSL
 *   If there is, generate its configuration from PSL.
 *   
 * @param  
 *
 * @return fbe_status_t - status
 *
 * Created by Jian @ March 12, 2013
 ****************************************************************/
fbe_status_t database_check_and_make_up_system_object_configuration(fbe_database_service_t *fbe_database_service, fbe_bool_t *some_object_missing)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t index;
    fbe_private_space_layout_region_t region = {0};
    fbe_private_space_layout_lun_info_t lun_info = {0};
    database_object_entry_t *current_entry = NULL;
    fbe_object_id_t temp_object_id;
    fbe_bool_t some_system_lun_missing = FBE_FALSE;
    fbe_bool_t some_system_rg_missing = FBE_FALSE;
    fbe_u64_t                           current_commit_level;

    fbe_spinlock_lock(&fbe_database_service->system_db_header_lock);
    current_commit_level = fbe_database_service->system_db_header.persisted_sep_version;
    fbe_spinlock_unlock(&fbe_database_service->system_db_header_lock);

    database_trace(FBE_TRACE_LEVEL_INFO, 
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    "%s entry.\n",
                                    __FUNCTION__);

    /* check PSL rgs */
    for(index = 0; index < FBE_PRIVATE_SPACE_LAYOUT_MAX_REGIONS; index++)
    {
        status = fbe_private_space_layout_get_region_by_index(index, &region);
        if (status != FBE_STATUS_OK) 
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "db_check&mkup_sys_objs_conf:fail to get pri space layout for region index %d,stat:0x%x\n",
                           index,
                           status);
        }
        else
        {
            temp_object_id = region.raid_info.object_id;
            /* Hit the Terminator */
            if(region.region_id == FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_INVALID) 
            {
                database_trace(FBE_TRACE_LEVEL_INFO,                                                     
                                                FBE_TRACE_MESSAGE_ID_INFO,
                                                "db_check&mkup_sys_objs_conf: system RG config check done.\n");
                if(some_system_rg_missing)
                {
                    database_trace(FBE_TRACE_LEVEL_INFO, 
                                                    FBE_TRACE_MESSAGE_ID_INFO,
                                                    "db_check&mkup_sys_objs_conf: there is/are missing system RG/RGs.\n");
                }
                else
                {
                    database_trace(FBE_TRACE_LEVEL_INFO, 
                                                   FBE_TRACE_MESSAGE_ID_INFO,
                                                   "db_check&mkup_sys_objs_conf: there is NO missing system RG.\n");
                }
                
                break;
            }
            else if(region.region_type == FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_RAID_GROUP) 
            {
                /* get the object entry with this object id */
                current_entry = &(fbe_database_service->object_table.table_content.object_entry_ptr[temp_object_id]);
                if(current_entry->header.state == DATABASE_CONFIG_ENTRY_VALID)
                    /* There is valid object entry in Database, check next one in PSL*/
                    continue;
                else
                {
                    /* The configuration for this system RG is missing from DB */
                    database_trace(FBE_TRACE_LEVEL_INFO, 
                                                    FBE_TRACE_MESSAGE_ID_INFO,
                                                    "db_check&mkup_sys_objs_conf: system RG 0x%x conf missing.\n", temp_object_id);
                    database_trace(FBE_TRACE_LEVEL_INFO, 
                                                    FBE_TRACE_MESSAGE_ID_INFO,
                                                    "db_check&mkup_sys_objs_conf: generating system RG 0x%x conf from PSL.\n", temp_object_id);
                    
                    /* Generate rg configuration from PSL */
                    status = database_system_objects_generate_rg_config_from_psl(fbe_database_service,
                                                                                                                                                       temp_object_id,
                                                                                                                                                       &region);
                    if (status != FBE_STATUS_OK) 
                    {
                        database_trace(FBE_TRACE_LEVEL_ERROR, 
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s: fail to generate configuration from PSL for obj: 0x%x\n",
                            __FUNCTION__, temp_object_id);
                    }
                    some_system_rg_missing= FBE_TRUE;
                }
            }
        }
    }

    /* check PSL luns  */
    for(index = 0; index < FBE_PRIVATE_SPACE_LAYOUT_MAX_PRIVATE_LUNS; index++)
    {
        status = fbe_private_space_layout_get_lun_by_index(index, &lun_info);
        if (status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "db_check&mkup_sys_objs_conf:fail to get pri space layout for LUN index %d,stat: 0x%x\n",
                           index,
                           status);
        }
        else if(lun_info.lun_number == FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_INVALID) 
        {
            // hit the terminator
            database_trace(FBE_TRACE_LEVEL_INFO,                                                     
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            "db_check&mkup_sys_objs_conf: system lun config check done.\n");
            if(some_system_lun_missing)
            {
                database_trace(FBE_TRACE_LEVEL_INFO, 
                                                FBE_TRACE_MESSAGE_ID_INFO,
                                                "db_check&mkup_sys_objs_conf: there is/are missing system lun/luns.\n");
            }
            else
            {
                database_trace(FBE_TRACE_LEVEL_INFO, 
                                                FBE_TRACE_MESSAGE_ID_INFO,
                                                "db_check&mkup_sys_objs_conf: there is NO missing system lun.\n");
            }
            
            break;
        }
        else if(lun_info.commit_level > current_commit_level)
        {
            database_trace(FBE_TRACE_LEVEL_INFO, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "db_check&mkup_sys_objs_conf: Skipping LUN above our commit level\n");
        }
        else
        {
            temp_object_id = lun_info.object_id;
            current_entry = &(fbe_database_service->object_table.table_content.object_entry_ptr[temp_object_id]);
            if(current_entry->header.state == DATABASE_CONFIG_ENTRY_VALID)
                /* There is valid object entry in Database, check next one in PSL*/
                continue;
            else
            {
                /* The configuration for this system lun is missing from DB */
                database_trace(FBE_TRACE_LEVEL_INFO, 
                                                FBE_TRACE_MESSAGE_ID_INFO,
                                                "db_check&mkup_sys_objs_conf: system lun 0x%x conf missing.\n", temp_object_id);
                database_trace(FBE_TRACE_LEVEL_INFO, 
                                                FBE_TRACE_MESSAGE_ID_INFO,
                                                "db_check&mkup_sys_objs_conf: generating system lun 0x%x conf from PSL.\n", temp_object_id);
                
                /* Generate lun configuration WITHOUT "invalidate" option from PSL */
                status = database_system_objects_generate_lun_from_psl(fbe_database_service,
                                                                                                                                       temp_object_id,
                                                                                                                                       &lun_info,
                                                                                                                                       FBE_FALSE,
                                                                                                                                       FBE_FALSE);
                if (status != FBE_STATUS_OK) 
                {
                    database_trace(FBE_TRACE_LEVEL_ERROR, 
                                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                    "%s: fail to generate configuration from PSL for obj: 0x%x\n",
                                                    __FUNCTION__, temp_object_id);
                }
                some_system_lun_missing= FBE_TRUE;
            }
        }
    }

    *some_object_missing = (some_system_lun_missing || some_system_rg_missing);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn database_system_objects_create_sep_internal_objects_from_persist
 *****************************************************************
 * @brief
 *   create system objects that are internal to sep and edges.
 *   the database table info is loaded from disk
 *   These objects include two rg and 3 luns (db, metadata, and metastatistic)
 *
 * @param  
 *
 * @return fbe_status_t - status
 *
 ****************************************************************/
fbe_status_t database_system_objects_create_sep_internal_objects_from_persist(fbe_database_service_t *fbe_database_service)
{
    fbe_u32_t index;
    fbe_status_t status;
    fbe_private_space_layout_lun_info_t lun_info;
    fbe_private_space_layout_region_t region;

    /* 1: create the bvd interface*/
    status = database_system_objects_create_object_and_its_edges(fbe_database_service, FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_BVD_INTERFACE, FBE_FALSE);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "db_sys_objs_create_sep_internal_objects:fail to create BVD interface objid %d,stat:0x%x\n",
                       FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_BVD_INTERFACE,
                       status);
        return status;
    }

    /* create VDs */
    /*status = database_common_create_object_from_table(&fbe_database_service->object_table, 
                                             FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VD_0, 
                                             (FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_FIRST-FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VD_0));
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: fail to create vds,status:0x%x\n",
                       __FUNCTION__,
                       status);
        return status;
    }

    status = database_common_set_config_from_table(&fbe_database_service->object_table, 
                                          FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VD_0, 
                                          (FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_FIRST-FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VD_0));
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: fail to set config for vds,status:0x%x\n",
                       __FUNCTION__,
                       status);
        return status;
    }

    status = database_common_commit_object_from_table(&fbe_database_service->object_table, 
                                             FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VD_0, 
                                             (FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_FIRST-FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VD_0));
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: fail to commit object for vds,status:0x%x\n",
                       __FUNCTION__,
                       status);
        return status;
    }

    status = database_common_create_edge_from_table(&fbe_database_service->edge_table,
                                                    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VD_0*DATABASE_MAX_EDGE_PER_OBJECT,
                                                    (FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_FIRST-FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VD_0)*DATABASE_MAX_EDGE_PER_OBJECT);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: create edge failed\n",
                       __FUNCTION__);
        return status;
    }*/

    /* 2: Create RGs */
    for(index = 0; index < FBE_PRIVATE_SPACE_LAYOUT_MAX_REGIONS; index++){
        status = fbe_private_space_layout_get_region_by_index(index, &region);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "db_sys_objs_create_sep_internal_objects:fail to get pri space layout for region index %d,stat:0x%x\n",
                           index,
                           status);
        }
        else
        {
            // Hit the Terminator
            if(region.region_id == FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_INVALID) {
                break;
            }
            else if(region.region_type == FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_RAID_GROUP) {
                status = database_system_objects_create_object_and_its_edges(fbe_database_service, region.raid_info.object_id, FBE_FALSE);
            }
#ifdef C4_INTEGRATED
            else if(region.region_id == FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_UTILITY_BOOT_VOLUME_SPA)
            {
                /* Create export luns for mirror */
                status = database_system_objects_create_export_lun_objects(fbe_database_service, NULL);
                if (status != FBE_STATUS_OK) {
                    database_trace(FBE_TRACE_LEVEL_ERROR, 
                                   FBE_TRACE_MESSAGE_ID_INFO, 
                                   "DB_CREATE_OBJ: Failed to create export LUN objects. Sts: 0x%x\n",
                                   status);
                    return status;
                }
            }
#endif
        }
    }
    /* 3: Create LUNs */
    for(index = 0; index < FBE_PRIVATE_SPACE_LAYOUT_MAX_PRIVATE_LUNS; index++){
        status = fbe_private_space_layout_get_lun_by_index(index, &lun_info);
        if (status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "db_sys_objs_create_sep_internal_objects:fail to get pri space layout for LUN index %d,stat: 0x%x\n",
                           index,
                           status);
        }
        else if(lun_info.lun_number == FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_INVALID) {
            // hit the terminator
            break;
        }
        else if(lun_info.flags & FBE_PRIVATE_SPACE_LAYOUT_FLAG_SEP_INTERNAL)
        {
            status = database_system_objects_create_object_and_its_edges(fbe_database_service, lun_info.object_id, FBE_FALSE);
        }
    }

    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_CREATE_OBJECT, "db_sys_objs_create_sep_internal_objects: END.\n");
    return status;
}


/*!***************************************************************
 * @fn database_system_objects_create_sep_internal_objects
 *****************************************************************
 * @brief
 *   create system objects that are internal to sep and edges.
 *   These objects include two rg and 3 luns (db, metadata, and metastatistic)
 *
 * @param  
 *
 * @return fbe_status_t - status
 *
 ****************************************************************/
fbe_status_t database_system_objects_create_sep_internal_objects(fbe_database_service_t *fbe_database_service, fbe_bool_t invalidate)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t index;
    fbe_private_space_layout_region_t region = {0};
    fbe_private_space_layout_lun_info_t lun_info = {0};
    fbe_bool_t  is_first_create = invalidate;

    database_system_objects_register_notification(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAID_METADATA,
												  FBE_LIFECYCLE_STATE_READY,
												  FBE_TOPOLOGY_OBJECT_TYPE_LUN);

    /*create the bvd interface*/
    status = database_system_objects_create_object_and_its_edges(fbe_database_service, FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_BVD_INTERFACE, is_first_create);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "db_sys_objs_create_sep_internal_objects:fail to create BVD interface objid %d,stat:0x%x\n",
                       FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_BVD_INTERFACE,
                       status);
        return status;
    }


    /* create the rgs */
    for(index = 0; index < FBE_PRIVATE_SPACE_LAYOUT_MAX_REGIONS; index++){
        status = fbe_private_space_layout_get_region_by_index(index, &region);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "db_sys_objs_create_sep_internal_objects:fail to get pri space layout for region index %d,stat:0x%x\n",
                           index,
                           status);
        }
        else
        {
            // Hit the Terminator
            if(region.region_id == FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_INVALID) {
                break;
            }
            else if(region.region_type == FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_RAID_GROUP) {
                status = database_system_objects_create_rg_from_psl(fbe_database_service,
                                                                    region.raid_info.object_id, 
                                                                    &region);
            }
#ifdef C4_INTEGRATED
            else if(region.region_id == FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_UTILITY_BOOT_VOLUME_SPA)
            {
                /* Create export luns for mirror */
                status = database_system_objects_create_export_lun_objects(fbe_database_service, NULL);
                if (status != FBE_STATUS_OK) {
                    database_trace(FBE_TRACE_LEVEL_ERROR, 
                                   FBE_TRACE_MESSAGE_ID_INFO, 
                                   "DB_CREATE_OBJ: Failed to create export LUN objects. Sts: 0x%x\n",
                                   status);

                    database_system_objects_unregister_notification();
                    return status;
                }
            }
#endif
        }
    }

    /* create the sep internal luns  */
    for(index = 0; index < FBE_PRIVATE_SPACE_LAYOUT_MAX_PRIVATE_LUNS; index++){
        status = fbe_private_space_layout_get_lun_by_index(index, &lun_info);
        if (status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "db_sys_objs_create_sep_internal_objects:fail to get pri space layout for LUN index %d,stat: 0x%x\n",
                           index,
                           status);
        }
        else if(lun_info.lun_number == FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_INVALID) {
            // hit the terminator
            break;
        }
        else if(lun_info.flags & FBE_PRIVATE_SPACE_LAYOUT_FLAG_SEP_INTERNAL)
        {
            status = database_system_objects_create_lun_from_psl(fbe_database_service,
                                                                 lun_info.object_id, 
                                                                 &lun_info,
                                                                 invalidate);
        }
    }

    status = fbe_semaphore_wait_ms(&database_system_object_notification_context.sem, 120000); 
    if (status != FBE_STATUS_OK) {
		database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
			 "db_sys_objs_create_sep_internal_objects time out wait for internal DB LUNs to be ready\n");
		return status;
	}
	
    database_system_objects_unregister_notification();
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_CREATE_OBJECT, "db_sys_objs_create_sep_internal_objects: DB is ready.\n");
    return status;
}

/*!***************************************************************
 * @fn database_system_objects_create_export_objects_from_persist
 *****************************************************************
 * @brief
 *   create system objects that are internal to sep and edges,
 *   using the database table from disk.
 *   These objects include two rg and 3 luns (db, metadata, and metastatistic)
 *
 * @param  
 *
 * @return fbe_status_t - status
 *
 ****************************************************************/
fbe_status_t database_system_objects_create_export_objects_from_persist(fbe_database_service_t *fbe_database_service)
{
    fbe_u32_t index;
    fbe_status_t status;
    fbe_private_space_layout_lun_info_t lun_info;

    for(index = 0; index < FBE_PRIVATE_SPACE_LAYOUT_MAX_PRIVATE_LUNS; index++){
        status = fbe_private_space_layout_get_lun_by_index(index, &lun_info);

        if (status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "db_sys_objs_create_sep_internal_objects:fail to get pri space layout for LUN index %d,stat: 0x%x\n",
                           index,
                           status);
        }
        else if(lun_info.lun_number == FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_INVALID) {
            // hit the terminator
            break;
        }
        else if(! (lun_info.flags & FBE_PRIVATE_SPACE_LAYOUT_FLAG_SEP_INTERNAL))
        {
            status = database_system_objects_create_object_and_its_edges(fbe_database_service, lun_info.object_id, FBE_FALSE);
        }
    }

    return status;
}
/*!***************************************************************
 * @fn database_system_objects_create_export_objects
 *****************************************************************
 * @brief
 *   create system objects that are internal to sep and edges.
 *   These objects include two rg and 3 luns (db, metadata, and metastatistic)
 *
 * @param  
 *
 * @return fbe_status_t - status
 *
 ****************************************************************/
fbe_status_t database_system_objects_create_export_objects(fbe_database_service_t *fbe_database_service, fbe_bool_t invalidate)
{
    fbe_u32_t index;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_private_space_layout_lun_info_t lun_info;
    fbe_u64_t                           current_commit_level;

    fbe_spinlock_lock(&fbe_database_service->system_db_header_lock);
    current_commit_level = fbe_database_service->system_db_header.persisted_sep_version;
    fbe_spinlock_unlock(&fbe_database_service->system_db_header_lock);

    for(index = 0; index < FBE_PRIVATE_SPACE_LAYOUT_MAX_PRIVATE_LUNS; index++){
        status = fbe_private_space_layout_get_lun_by_index(index, &lun_info);

        if (status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "db_sys_objs_create_sep_internal_objects:fail to get pri space layout for LUN index %d,stat: 0x%x\n",
                           index,
                           status);
        }
        else if(lun_info.lun_number == FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_INVALID) {
            // hit the terminator
            break;
        }
        else if(lun_info.commit_level > current_commit_level)
        {
            // Nothing to do here
        }
        else if(! (lun_info.flags & FBE_PRIVATE_SPACE_LAYOUT_FLAG_SEP_INTERNAL))
        {
            status = database_system_objects_create_lun_from_psl(fbe_database_service,
                                                                 lun_info.object_id, 
                                                                 &lun_info,
                                                                 invalidate);
        }
    }

    return status;
}

/*!***************************************************************
 * @fn database_system_objects_generate_rg_config_from_psl
 *****************************************************************
 * @brief
 *   generate the configuration for raid group from the PSL.
 *
 * @param  
 *
 * @return fbe_status_t - status
 *
 * @version
 *    11/02/2012:   created
 *
 ****************************************************************/
static fbe_status_t database_system_objects_generate_rg_config_from_psl(fbe_database_service_t *fbe_database_service,
                                                          fbe_object_id_t object_id, 
                                                          fbe_private_space_layout_region_t *region)
{
    database_object_entry_t     * object_entry_ptr = NULL;
    database_user_entry_t       * user_entry_ptr = NULL;
    database_edge_entry_t       * edge_entry_ptr = NULL;
    fbe_u32_t                   edge_index = 0;
    fbe_status_t                status = FBE_STATUS_OK;

    if (region == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: psl region pointer is NULL\n",
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service->object_table, 
                                                     object_id, 
                                                     &object_entry_ptr);
	if (status != FBE_STATUS_OK) {
		database_trace(FBE_TRACE_LEVEL_ERROR, 
							   FBE_TRACE_MESSAGE_ID_INFO,
							   "%s: fail to get object entry by object id\n",
							   __FUNCTION__);
		return status;
	}
    object_entry_ptr->header.entry_id = FBE_PERSIST_SECTOR_START_ENTRY_ID;
    object_entry_ptr->header.object_id = object_id;
    object_entry_ptr->header.state = DATABASE_CONFIG_ENTRY_VALID;
    object_entry_ptr->db_class_id = convert_psl_raid_type_to_class_id(region->raid_info.raid_type);
    object_entry_ptr->header.version_header.size = database_common_object_entry_size(object_entry_ptr->db_class_id);
    object_entry_ptr->set_config_union.rg_config.capacity = region->raid_info.exported_capacity;
    object_entry_ptr->set_config_union.rg_config.raid_type = region->raid_info.raid_type;
    object_entry_ptr->set_config_union.rg_config.width = region->number_of_frus;
    object_entry_ptr->set_config_union.rg_config.chunk_size = FBE_RAID_DEFAULT_CHUNK_SIZE;
    object_entry_ptr->set_config_union.rg_config.debug_flags = FBE_RAID_GROUP_DEBUG_FLAG_NONE;
    object_entry_ptr->set_config_union.rg_config.element_size = FBE_RAID_SECTORS_PER_ELEMENT;

    if ((object_entry_ptr->set_config_union.rg_config.raid_type == FBE_RAID_GROUP_TYPE_RAID1) ||
        (object_entry_ptr->set_config_union.rg_config.raid_type == FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER)  ||
        (object_entry_ptr->set_config_union.rg_config.raid_type == FBE_RAID_GROUP_TYPE_RAW_MIRROR))
    {
        /* Mirrors do not have parity.
         */
        object_entry_ptr->set_config_union.rg_config.elements_per_parity = 0;
    }
    else
    {
        object_entry_ptr->set_config_union.rg_config.elements_per_parity = FBE_RAID_ELEMENTS_PER_PARITY;
    }

    object_entry_ptr->set_config_union.rg_config.power_saving_enabled = FBE_FALSE;  /* no power saving on system drives */
    object_entry_ptr->set_config_union.rg_config.generation_number = object_entry_ptr->header.object_id; /* todo:  What should be here? */

    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service->user_table, 
                                                     object_id, 
                                                     &user_entry_ptr);
	if (status != FBE_STATUS_OK) {
		database_trace(FBE_TRACE_LEVEL_ERROR, 
							   FBE_TRACE_MESSAGE_ID_INFO,
							   "%s: fail to get user entry by object id\n",
							   __FUNCTION__);
		return status;
	}
    user_entry_ptr->header.entry_id = FBE_PERSIST_SECTOR_START_ENTRY_ID;
    user_entry_ptr->header.object_id = object_id;
    user_entry_ptr->header.state = DATABASE_CONFIG_ENTRY_VALID;
    user_entry_ptr->db_class_id = convert_psl_raid_type_to_class_id(region->raid_info.raid_type);
    user_entry_ptr->header.version_header.size = database_common_user_entry_size(user_entry_ptr->db_class_id);
    user_entry_ptr->user_data_union.rg_user_data.is_system = FBE_TRUE;
    user_entry_ptr->user_data_union.rg_user_data.raid_group_number = region->raid_info.raid_group_id;

	database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "DB:PSL_RG:obj:0x%X Cap:0x%llX\n",object_id, (unsigned long long)region->raid_info.exported_capacity);

    for (edge_index = 0; edge_index < region->number_of_frus; edge_index++ )
    {
        status = fbe_database_config_table_get_edge_entry(&fbe_database_service->edge_table,
                                                 object_id,
                                                 edge_index,
                                                 &edge_entry_ptr);
		if (status != FBE_STATUS_OK) {
			database_trace(FBE_TRACE_LEVEL_ERROR, 
							   FBE_TRACE_MESSAGE_ID_INFO,
							   "%s: fail to get edge entry by objid and edge index\n",
							   __FUNCTION__);
			return status;
		}
        edge_entry_ptr->header.entry_id = FBE_PERSIST_SECTOR_START_ENTRY_ID;
        edge_entry_ptr->header.object_id = object_id;
        edge_entry_ptr->header.state = DATABASE_CONFIG_ENTRY_VALID;
        edge_entry_ptr->header.version_header.size = database_common_edge_entry_size();
        edge_entry_ptr->client_index = edge_index;
        edge_entry_ptr->capacity = region->size_in_blocks;  /* this is the imported capacity from VD*/
        edge_entry_ptr->offset = region->starting_block_address;
        //edge_entry_ptr->server_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + edge_index; /* PVD object id */
        edge_entry_ptr->server_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + region->fru_id[edge_index]; /* PVD object id */
        if (!fbe_private_space_layout_object_id_is_system_pvd(edge_entry_ptr->server_id)) {
            edge_entry_ptr->server_id = region->fru_id[edge_index]; /* non-system PVD object id */
        }

		database_trace(FBE_TRACE_LEVEL_INFO, 
					   FBE_TRACE_MESSAGE_ID_INFO,
					   "DB:PSL_RG:index:%d, cap:0x%llX, offset:0x%llX\n", edge_index, (unsigned long long)region->size_in_blocks, (unsigned long long)region->starting_block_address );
    }

    return status;

}


static fbe_status_t database_system_objects_create_rg_from_psl(fbe_database_service_t *fbe_database_service,
                                                          fbe_object_id_t object_id, 
                                                          fbe_private_space_layout_region_t *region)
{
	fbe_status_t				status = FBE_STATUS_OK;

    if (region == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: psl region pointer is NULL\n",
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = database_system_objects_generate_rg_config_from_psl(fbe_database_service,
                                                        object_id,
                                                        region);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: fail to generate configuration from PSL for obj: %d\n",
                       __FUNCTION__, object_id);
        return status;
    }

    /* now create them in toplogy*/
    return database_system_objects_create_object_and_its_edges(fbe_database_service, object_id, FBE_TRUE);
}

static database_class_id_t convert_psl_raid_type_to_class_id(fbe_u32_t raid_type)
{
    switch(raid_type) {
    case FBE_RAID_GROUP_TYPE_RAID3:
    case FBE_RAID_GROUP_TYPE_RAID5:
    case FBE_RAID_GROUP_TYPE_RAID6:
        return DATABASE_CLASS_ID_PARITY;
    case FBE_RAID_GROUP_TYPE_RAID1:
    case FBE_RAID_GROUP_TYPE_RAW_MIRROR:
        return DATABASE_CLASS_ID_MIRROR;
    default:
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: unsupported RAID type\n",
                       __FUNCTION__);
        return DATABASE_CLASS_ID_INVALID;
    }
}

/*!***************************************************************
 * @fn database_system_objects_generate_lun_from_psl
 *****************************************************************
 * @brief
 *   generate the configuration for lun from the PSL.
 *
 * @param  
 *
 * @return fbe_status_t - status
 *
 * @version
 *    11/02/2012:   created
 *
 ****************************************************************/
fbe_status_t database_system_objects_generate_lun_from_psl(fbe_database_service_t *fbe_database_service,
                                                                fbe_object_id_t object_id, 
                                                                fbe_private_space_layout_lun_info_t *lun_info,
                                                                fbe_bool_t invalidate,
                                                                fbe_bool_t recreate_sys_obj)
{
    database_object_entry_t     * object_entry_ptr = NULL;
    database_user_entry_t       * user_entry_ptr = NULL;
    database_edge_entry_t       * edge_entry_ptr = NULL;
    fbe_u32_t                   edge_index = 0;
    fbe_bool_t                  ndb_b       = FBE_FALSE;
    fbe_bool_t                  clear_nz_b  = FBE_FALSE;
	fbe_imaging_flags_t			imaging_flags;
	fbe_assigned_wwid_t			wwid;
	fbe_status_t				status;

    /* ???? */
	fbe_database_get_imaging_flags(&imaging_flags);

    if (lun_info == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: psl lun_info pointer is NULL\n",
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* determine if the lun should be bind with ndb or not. */
    
    if(invalidate)
    {
        if (lun_info->flags & FBE_PRIVATE_SPACE_LAYOUT_FLAG_EXTERNALLY_INITIALIZED)
        {
            /* Some rgs, like the raw mirror rg is initialized via raid library at boot time, not at lun create time.
             */
            ndb_b       = FBE_TRUE;
            clear_nz_b  = FBE_TRUE;
        }
        else if(lun_info->ica_image_type == FBE_IMAGING_IMAGE_TYPE_INVALID)
        {
            ndb_b       = FBE_FALSE;
        }
        else if(imaging_flags.images_installed[lun_info->ica_image_type] == FBE_IMAGING_FLAGS_TRUE)
        {
            ndb_b       = FBE_TRUE;
            clear_nz_b  = FBE_TRUE;
        }
        else
        {
            ndb_b       = FBE_FALSE;
        }
    }
    else
    {
        ndb_b = FBE_TRUE;
    }

    /*If the caller is trying to recreate the broken system object LUN, do the non-ndb*/
    if (recreate_sys_obj) 
    {
        ndb_b = FBE_FALSE;
    }

    status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service->object_table, 
                                                     object_id, 
                                                     &object_entry_ptr);
	if (status != FBE_STATUS_OK) {
		database_trace(FBE_TRACE_LEVEL_ERROR, 
							   FBE_TRACE_MESSAGE_ID_INFO,
							   "%s: fail to get object entry by object id\n",
							   __FUNCTION__);
		return status;
	}
    fbe_zero_memory(object_entry_ptr, sizeof(database_object_entry_t));
    object_entry_ptr->header.entry_id = FBE_PERSIST_SECTOR_START_ENTRY_ID;
    object_entry_ptr->header.object_id = object_id;
    object_entry_ptr->header.state = DATABASE_CONFIG_ENTRY_VALID;
    object_entry_ptr->db_class_id = DATABASE_CLASS_ID_LUN;
    object_entry_ptr->header.version_header.size = database_common_object_entry_size(object_entry_ptr->db_class_id);
    object_entry_ptr->set_config_union.lu_config.capacity = lun_info->external_capacity;
    object_entry_ptr->set_config_union.lu_config.generation_number = object_entry_ptr->header.object_id; /* todo:  What should be here? */
    object_entry_ptr->set_config_union.lu_config.power_saving_enabled = FBE_FALSE; /* no power saving on system objects */
    object_entry_ptr->set_config_union.lu_config.power_save_io_drain_delay_in_sec = FBE_LUN_POWER_SAVE_DELAY_DEFAULT;
    object_entry_ptr->set_config_union.lu_config.power_saving_idle_time_in_seconds = FBE_LUN_DEFAULT_POWER_SAVE_IDLE_TIME;
    object_entry_ptr->set_config_union.lu_config.max_lun_latency_time_is_sec = FBE_LUN_MAX_LATENCY_TIME_DEFAULT;
    object_entry_ptr->set_config_union.lu_config.config_flags = FBE_LUN_CONFIG_NONE;

    if(ndb_b) {
        object_entry_ptr->set_config_union.lu_config.config_flags |= FBE_LUN_CONFIG_NO_USER_ZERO;
    }

    if(clear_nz_b) {
        object_entry_ptr->set_config_union.lu_config.config_flags |= FBE_LUN_CONFIG_CLEAR_NEED_ZERO;
    }

    if (lun_info->flags & FBE_PRIVATE_SPACE_LAYOUT_FLAG_EXPORT_LUN) {
        object_entry_ptr->set_config_union.lu_config.config_flags |= FBE_LUN_CONFIG_EXPROT_LUN;
#if defined(SIMMODE_ENV)
        /* We don't issue initial verify in simulation, since simulated drive can't be read before writing*/
        if (ndb_b) {
            object_entry_ptr->set_config_union.lu_config.config_flags |= FBE_LUN_CONFIG_NO_INITIAL_VERIFY;
        }
#endif
    }
    
    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service->user_table, 
                                                     object_id, 
                                                     &user_entry_ptr);
	if (status != FBE_STATUS_OK) {
		database_trace(FBE_TRACE_LEVEL_ERROR, 
							   FBE_TRACE_MESSAGE_ID_INFO,
							   "%s: fail to get user entry by object id\n",
							   __FUNCTION__);
		return status;
	}
    user_entry_ptr->header.entry_id = FBE_PERSIST_SECTOR_START_ENTRY_ID;
    user_entry_ptr->header.object_id = object_id;
    user_entry_ptr->header.state = DATABASE_CONFIG_ENTRY_VALID;
    user_entry_ptr->db_class_id = DATABASE_CLASS_ID_LUN;
    user_entry_ptr->header.version_header.size = database_common_user_entry_size(user_entry_ptr->db_class_id);
    user_entry_ptr->user_data_union.lu_user_data.export_device_b = lun_info->export_device_b;
    user_entry_ptr->user_data_union.lu_user_data.lun_number = lun_info->lun_number;
    fbe_copy_memory(&user_entry_ptr->user_data_union.lu_user_data.user_defined_name, 
                    lun_info->exported_device_name,
                    sizeof(user_entry_ptr->user_data_union.lu_user_data.user_defined_name));

    user_entry_ptr->user_data_union.lu_user_data.attributes = 0;


    /*and generate a new WWN*/
    fbe_database_generate_wwn(&wwid, object_id, &fbe_database_service->prom_info);

    /*zero wwn first*/
    fbe_copy_memory(&user_entry_ptr->user_data_union.lu_user_data.world_wide_name,
                    &wwid,
                    sizeof(user_entry_ptr->user_data_union.lu_user_data.world_wide_name));

    status = fbe_database_config_table_get_edge_entry(&fbe_database_service->edge_table,
                                             object_id,
                                             edge_index,
                                             &edge_entry_ptr);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: fail to get edge entry by obj_id and edge_inx\n",
                                __FUNCTION__);
        return status;
    }
    edge_entry_ptr->header.entry_id = FBE_PERSIST_SECTOR_START_ENTRY_ID;
    edge_entry_ptr->header.object_id = object_id;
    edge_entry_ptr->header.state = DATABASE_CONFIG_ENTRY_VALID;
    edge_entry_ptr->header.version_header.size = database_common_edge_entry_size();
    edge_entry_ptr->client_index = edge_index;
    edge_entry_ptr->capacity = lun_info->internal_capacity;  /* this is the imported capacity from RG*/
    edge_entry_ptr->offset = lun_info->raid_group_address_offset;
    psl_lun_find_rg_object_id(lun_info->raid_group_id, &edge_entry_ptr->server_id);/* RG object id */

    database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                       "DB:PSL_LU:obj:%X, LUN#:%d, cap:0x%llX,offset:0x%llX, NDB:%s\n",
                        object_id, lun_info->lun_number, (unsigned long long)lun_info->internal_capacity, (unsigned long long)lun_info->raid_group_address_offset,  
                       ndb_b == FBE_TRUE ? "Y" : "N");

    /* now create them in toplogy*/
    return FBE_STATUS_OK;
}

fbe_status_t database_system_objects_create_lun_from_psl(fbe_database_service_t *fbe_database_service,
                                                                fbe_object_id_t object_id, 
                                                                fbe_private_space_layout_lun_info_t *lun_info,
                                                                fbe_bool_t invalidate)
{
    fbe_status_t            status;

    status = database_system_objects_generate_lun_from_psl(fbe_database_service, 
                                                  object_id,
                                                  lun_info,
                                                  invalidate,
                                                  FBE_FALSE);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: fail to generate configuration from PSL for obj: %d\n",
                       __FUNCTION__, object_id);
        return status;
    }

    /* now create them in toplogy*/
    return database_system_objects_create_object_and_its_edges(fbe_database_service, object_id, FBE_TRUE);
}

static fbe_status_t psl_lun_find_rg_object_id(fbe_raid_group_number_t raid_group_id, fbe_object_id_t *rg_object_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_private_space_layout_region_t region;

    *rg_object_id = FBE_OBJECT_ID_INVALID;
    status = fbe_private_space_layout_get_region_by_raid_group_id(raid_group_id, &region);
    if (status != FBE_STATUS_OK) {
        /* The c4mirror raid groups and luns are not added to PSL, find the c4mirror regions from locals.
         */
        status = fbe_database_layout_get_export_lun_region_by_raid_group_id(raid_group_id, &region);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: Did not find RG with id %d in psl.\n",
                       __FUNCTION__,
                       raid_group_id);
            return status;
       }
    }
    *rg_object_id = region.raid_info.object_id;
    return status;
}

fbe_status_t database_system_objects_create_object_and_its_edges (fbe_database_service_t *fbe_database_service, 
                                                                  fbe_object_id_t object_id,
                                                                  fbe_bool_t    is_initial_created)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    /* now create them in toplogy*/
	status = database_common_create_object_from_table(&fbe_database_service->object_table, 
                                                      object_id, 1);
    if(status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: create_object 0x%x failed. Status 0x%x \n",
                       __FUNCTION__,
                       object_id,
                       status);
        return status;
    }

    status = database_common_set_config_from_table(&fbe_database_service->object_table, 
                                                   object_id, 1);
    if(status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: set config to object 0x%x failed. Status 0x%x \n",
                       __FUNCTION__,
                       object_id,
                       status);
        return status;
    }
    status = database_common_create_edge_from_table(&fbe_database_service->edge_table,
                                                    object_id*DATABASE_MAX_EDGE_PER_OBJECT,
                                                    DATABASE_MAX_EDGE_PER_OBJECT);
    if(status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: create edge of object 0x%x failed. Status 0x%x \n",
                       __FUNCTION__,
                       object_id,
                       status);
        return status;
    }
    status = database_common_commit_object_from_table(&fbe_database_service->object_table, 
                                                      object_id, 1, is_initial_created);
    if(status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: commit object 0x%x failed. Status 0x%x \n",
                       __FUNCTION__,
                       object_id,
                       status);
        return status;
    }
    return status;
}
fbe_status_t database_system_objects_check_for_encryption(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t           system_object_count = 0;
    database_object_entry_t *entry_p = NULL;
    fbe_bool_t b_found = FBE_FALSE;
    fbe_u32_t index;
    fbe_database_service_t *fbe_database_service_p = &fbe_database_service;

    database_system_objects_get_reserved_count(&system_object_count);

    for (index = 0; index < system_object_count; index++) {

        status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service_p->object_table,
                                                                  index,
                                                                  &entry_p);
        if (status != FBE_STATUS_OK || entry_p == NULL) {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: get destination object entry failed!\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if ( !database_is_entry_valid(&entry_p->header) ) {
            continue;
        }
        if (entry_p->base_config.encryption_mode > FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED){
            b_found = FBE_TRUE;
            break;
        }
    }
    if (b_found) {
        database_global_info_entry_t *global_info_entry = NULL;
        status = fbe_database_config_table_get_global_info_entry(&fbe_database_service_p->global_info, 
                                                                 DATABASE_GLOBAL_INFO_TYPE_SYSTEM_ENCRYPTION, 
                                                                 &global_info_entry);

        if ( (status != FBE_STATUS_OK) || (global_info_entry == NULL) ) {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to get Global Info entry from the DB service.\n", 
                           __FUNCTION__);
            if(global_info_entry != NULL)
            {
                global_info_entry->info_union.encryption_info.encryption_mode = FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED;
            }
            else
            {
                status = FBE_STATUS_GENERIC_FAILURE;
            }
            return status;
        }
        global_info_entry->info_union.encryption_info.encryption_mode = FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED;
    }
    return status;
}

fbe_status_t fbe_database_get_last_system_object_id(fbe_object_id_t * object_id)
{
    /* The c4mirror raid groups/luns are not added to PSL,
     * use FBE_RESERVED_OBJECT_IDS - 1 instead of FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LAST
     */
    * object_id = FBE_RESERVED_OBJECT_IDS - 1;
    return FBE_STATUS_OK;
}

fbe_bool_t fbe_database_is_object_system_pvd(fbe_object_id_t pvd_id)
{
    if(fbe_private_space_layout_object_id_is_system_pvd(pvd_id) || fbe_private_space_layout_object_id_is_system_spare_pvd(pvd_id)) 
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}

fbe_bool_t fbe_database_is_object_system_rg(fbe_object_id_t rg_id)
{
    return fbe_private_space_layout_object_id_is_system_raid_group(rg_id);
}
fbe_bool_t fbe_database_can_encryption_start(fbe_object_id_t rg_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    database_object_entry_t *entry_p = NULL;
    fbe_database_service_t *fbe_database_service_p = &fbe_database_service;

    if (rg_id == FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG) {
        /* We always let the vault start.
         */
        return FBE_TRUE;
    }
    /* For non-vault raid groups, we just check if the vault is done encrypting. 
     * We require the vault to be done encrypting before we allow anyother RGs to start encrypting. 
     * This is needed since we do not want the system PVD encryption mode to get transitioned by 
     * more than one raid group (user and vault). 
     * Also if the vault is still in use it is not the best time to start encrypting since encryption requires cache. 
     */
    status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service_p->object_table,
                                                              FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG,
                                                              &entry_p);
    if (database_is_entry_valid(&entry_p->header) &&
        (entry_p->base_config.encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED)) {
        return FBE_TRUE;
    }
    return FBE_FALSE;
}
fbe_status_t fbe_database_get_non_paged_metadata_offset_capacity(fbe_lba_t *offset_p, 
                                                                 fbe_lba_t *rg_offset_p,
                                                                 fbe_block_count_t *capacity_p)
{
	fbe_private_space_layout_region_t region;
    fbe_private_space_layout_lun_info_t lun_info;
	fbe_private_space_layout_get_region_by_region_id(FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_SEP_RAW_MIRROR_RG, &region);
    fbe_private_space_layout_get_lun_by_lun_number(FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MCR_RAW_NP, &lun_info);

    /* Internal capacity is needed since we initialize the entire LUN's capacity including metadata.
     * This is because these LUNs are entirely initted at first boot and the LUN is NDB'd. 
     */
    if (offset_p != NULL)
    {
        *offset_p = region.starting_block_address + lun_info.raid_group_address_offset;
    }
    if (rg_offset_p != NULL)
    {
        *rg_offset_p = lun_info.raid_group_address_offset;
    }
    *capacity_p = lun_info.internal_capacity;
    return FBE_STATUS_OK;
}

fbe_status_t database_system_objects_get_system_db_offset_capacity(fbe_lba_t *offset_p, 
                                                                   fbe_lba_t *rg_offset_p,
                                                                   fbe_block_count_t *capacity_p)
{
	fbe_private_space_layout_region_t region;
    fbe_private_space_layout_lun_info_t lun_info;
	fbe_private_space_layout_get_region_by_region_id(FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_SEP_RAW_MIRROR_RG, &region);
    fbe_private_space_layout_get_lun_by_lun_number(FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MCR_RAW_SYSTEM_CONFIG, &lun_info);

    /* Internal capacity is needed since we initialize the entire LUN's capacity including metadata.
     * This is because these LUNs are entirely initted at first boot and the LUN is NDB'd. 
     */

    if (offset_p != NULL)
    {
        *offset_p = region.starting_block_address + lun_info.raid_group_address_offset;
    }
    if (rg_offset_p != NULL)
    {
        *rg_offset_p = lun_info.raid_group_address_offset;
    }
    *capacity_p = lun_info.internal_capacity;
    return FBE_STATUS_OK;
}

fbe_status_t database_system_objects_get_homewrecker_db_offset_capacity(fbe_lba_t *offset_p, fbe_block_count_t *capacity_p)
{
	fbe_private_space_layout_region_t region;
	fbe_private_space_layout_get_region_by_region_id(FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_HOMEWRECKER_DB, &region);

    *offset_p = region.starting_block_address;
    *capacity_p = region.size_in_blocks;
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_get_board_resume_info
 *****************************************************************
 * @brief
 *   This functions gets the Midplane resume information. We need the
 *   serial no, part no, wwn and rev of the board to persist in the DB. 
 *
 * @param  
 *
 * @return fbe_status_t - status
 *
 * @author 08/15/11 - Arun S - Created
 * 
 ****************************************************************/
fbe_status_t fbe_database_get_board_resume_info(fbe_board_mgmt_get_resume_t *board_info)
{
    fbe_topology_control_get_board_t         board;
    fbe_status_t                             status;

    fbe_zero_memory(board_info, sizeof(fbe_board_mgmt_get_resume_t));

    board_info->device_type = FBE_DEVICE_TYPE_ENCLOSURE;

    /* First get the object id of the board */
    status = fbe_database_send_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_GET_BOARD,
                                                 &board,
                                                 sizeof(board),
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 NULL,
                                                 0,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if ( status != FBE_STATUS_OK ){
        return status;
    }

    /* Get the resume info of the board */
    status = fbe_database_send_packet_to_object(FBE_BASE_BOARD_CONTROL_CODE_GET_RESUME,
                                                board_info,
                                                sizeof(fbe_board_mgmt_get_resume_t),
                                                board.board_object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_PHYSICAL);

    return status;
}


void fbe_database_generate_wwn(fbe_assigned_wwid_t *wwn, fbe_object_id_t object_id,
                                      database_midplane_info_t *board_info)
{
    static fbe_database_company_id_t fbe_config_system_objects_wwn_company_id = { 0x60, 0x06, 0x01, 0x60 };
    static fbe_database_controller_id_t fbe_config_system_objects_wwn_controller_id = {0x01, 0x23, 0x45, 0x67, 0x89};

    fbe_u8_t message[3 * sizeof(*wwn) + 16];
    fbe_u32_t i;
    fbe_u32_t serial_no_size;
    
    /* start with the first 3 1/2 bytes, from company id */
    *(fbe_database_company_id_t*)(&wwn->bytes[0]) = fbe_config_system_objects_wwn_company_id;

    /* start with the first 3 1/2 bytes, from company id */
    *(fbe_database_controller_id_t*)(&wwn->bytes[3]) = fbe_config_system_objects_wwn_controller_id;
    
    /* high nibble of bytes 3 is the company id, low part is the controller id
       the last assignment of byte 3 is the first byte of controller_id (byte[3] = 0x01),
       let's put the company id to the high nibble of byte 3 to make it (0x01|0x60 = 0x61)*/
    wwn->bytes[3] |= fbe_config_system_objects_wwn_company_id.bytes[3];

    /* now we have 0x60060161-23456789 */
    serial_no_size = sizeof(board_info->emc_serial_num);

    /* Lower 64 bits is the last 64 bits of the board serial no */
    for(i = 0; i < 8; ++i ){
        fbe_u32_t serial_no_index = serial_no_size - 8 + i;
        fbe_u8_t c;

        c = serial_no_index < 0 ? 0 : board_info->emc_serial_num[serial_no_index];

        wwn->bytes[i + 8] = c;
    }

    /* make the high byte of the last 8 the object id */
    wwn->bytes[8] = object_id;

    /* Build a message so we can log it */
    fbe_zero_memory (&message, sizeof(fbe_u8_t)*(3 * sizeof(*wwn) + 16));
    for ( i = 0; i < sizeof(*wwn); ++i ){
        fbe_sprintf(message + 3 * i, 2, "%02x", wwn->bytes[i]);

        /* Print a separator */
        fbe_sprintf(message + 3 * i + 2, 1, "%c", ((i+1)%4) ? ':' : ' ');
    }

    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                 "%s:LunId:0x%X, WWN:%s\n", __FUNCTION__,
                 object_id, message);

}


/*!***************************************************************
 * @fn fbe_database_persist_prom_info
 *****************************************************************
 * @brief
 *   This functions saves the serial no, part no, wwn and rev
 *   of the board in a static memory location in the DB service. 
 *
 * @param  
 *
 * @return fbe_status_t - status
 *
 * @author 08/19/11 - Arun S - Created
 * 
 ****************************************************************/
fbe_status_t fbe_database_persist_prom_info(fbe_board_mgmt_get_resume_t *board_info, database_midplane_info_t *prom_info)
{
    /***********************************************************
     * This doesn't actually do a persist as of today. But,    *
     * stores the info in static memory as long as the         *
     * DB service is up. The data won't persist across reboots.*
     * This should be removed/changed to do a actual perist    *
     * in DB so that the data persist across reboots.          *
     ***********************************************************/

    /* Fill up the prom info in database */
    fbe_copy_memory(prom_info->emc_serial_num, 
                    board_info->resume_data.resumePromData.emc_sub_assy_serial_num,
                    FBE_EMC_SERIAL_NUM_SIZE);
    fbe_copy_memory(prom_info->emc_revision, 
                    board_info->resume_data.resumePromData.emc_sub_assy_rev,
                    FBE_EMC_ASSEMBLY_REV_SIZE);
    fbe_copy_memory(prom_info->emc_part_number, 
                    board_info->resume_data.resumePromData.emc_sub_assy_part_num,
                    FBE_EMC_PART_NUM_SIZE);
    prom_info->world_wide_name = board_info->resume_data.resumePromData.wwn_seed;

    return FBE_STATUS_OK;
}


/* map system pvd id to spare pvd id*/
fbe_object_id_t fbe_database_get_system_drive_spare_id (fbe_object_id_t system_pvd_id)
{
    fbe_u32_t offset = system_pvd_id - FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST;

    return(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_SPARE_FIRST + offset);
}


/*!******************************************************************************
 * @fn fbe_database_system_objects_generate_config_for_system_object_and_persist
 ********************************************************************************
 * @brief
 *   generate the configuration for specified raid group or LUN from the PSL
 *   and persist the configuration to disk.
 *
 * @param  
 *
 * @return fbe_status_t - status
 *
 * @version
 *    May 24, 2013   Created: Yingying Song
 *
 ****************************************************************/
fbe_status_t fbe_database_system_objects_generate_config_for_system_object_and_persist(fbe_object_id_t object_id, fbe_bool_t invalidate, fbe_bool_t recreate)
{
    fbe_object_id_t system_object_id = object_id;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_private_space_layout_region_t region = {0};
    fbe_private_space_layout_lun_info_t lun_info = {0};
    database_object_entry_t     * object_entry_ptr = NULL;
    database_user_entry_t       * user_entry_ptr = NULL;
    database_edge_entry_t       * edge_entry_ptr = NULL;
    fbe_u32_t                   edge_index = 0;


    if((system_object_id <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_LAST) 
       && (system_object_id >= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_FIRST))
    {
        fbe_private_space_layout_get_region_by_object_id(system_object_id, &region);

        /* Generate rg configuration from PSL in memory */
        status = database_system_objects_generate_rg_config_from_psl(&fbe_database_service,
                                                                     system_object_id,
                                                                     &region);
        if (status != FBE_STATUS_OK) 
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: failed to generate config from PSL for rg obj: 0x%x\n",
                           __FUNCTION__, system_object_id);
            return status;
        }

        /* Get the generated rg object entry from memory */
        status = fbe_database_config_table_get_object_entry_by_id(&(fbe_database_service.object_table), 
                                                                  system_object_id, 
                                                                  &object_entry_ptr);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: fail to get object entry by object id 0x%x\n",
                           __FUNCTION__,system_object_id);
            return status;
        }

        /* Persist the genereted rg object entry to disk */
        status = fbe_database_system_db_raw_persist_object_entry(FBE_DATABASE_SYSTEM_DB_PERSIST_UPDATE , object_entry_ptr);

        if(status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to persist object entry for object id 0x%x\n",
                           __FUNCTION__, system_object_id);
            return status;
        }

        /* Get the generated rg user entry from memory */
        status = fbe_database_config_table_get_user_entry_by_object_id(&(fbe_database_service.user_table), 
                                                                       system_object_id, 
                                                                       &user_entry_ptr);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: fail to get user entry by object id 0x%x\n",
                           __FUNCTION__, system_object_id);
            return status;
        }

        /*Persist the generated rg user entry to disk */
        status = fbe_database_system_db_raw_persist_user_entry(FBE_DATABASE_SYSTEM_DB_PERSIST_UPDATE , user_entry_ptr);

        if(status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to persist user entry for object id 0x%x\n",
                           __FUNCTION__, system_object_id);
            return status;
        }


        for (edge_index = 0; edge_index < region.number_of_frus; edge_index++ )
        {
            /* Get the generated rg edge entry from memory */
            status = fbe_database_config_table_get_edge_entry(&(fbe_database_service.edge_table),
                                                              system_object_id,
                                                              edge_index,
                                                              &edge_entry_ptr);
            if (status != FBE_STATUS_OK) {
                database_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: fail to get edge entry by objid 0x%x and edge index 0x%x\n",
                               __FUNCTION__, system_object_id, edge_index);
                return status;
            }

            /* Persist the generated rg edge entry to disk */
            status = fbe_database_system_db_raw_persist_edge_entry(FBE_DATABASE_SYSTEM_DB_PERSIST_UPDATE , edge_entry_ptr);

            if(status != FBE_STATUS_OK)
            {
                database_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: failed to persist edge entry(index: 0x%x) for object id 0x%x\n",
                               __FUNCTION__,edge_index, system_object_id);
                return status;
            }

        }

        return FBE_STATUS_OK;
    }
    else if((system_object_id >= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_FIRST)
            && (system_object_id <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_LAST))
    {
        fbe_private_space_layout_get_lun_by_object_id(system_object_id, &lun_info);

        /* Generate lun configuration WITHOUT "invalidate" option from PSL in memory*/
        status = database_system_objects_generate_lun_from_psl(&fbe_database_service,
                                                               system_object_id,
                                                               &lun_info,
                                                               invalidate,
                                                               recreate);
        if (status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: failed to generate config from PSL for lun obj: 0x%x\n",
                           __FUNCTION__, system_object_id);
            return status;
        }

        /* Get the generated lun object entry from memory */
        status = fbe_database_config_table_get_object_entry_by_id(&(fbe_database_service.object_table), 
                                                                  system_object_id, 
                                                                  &object_entry_ptr);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: fail to get object entry by object id 0x%x\n",
                           __FUNCTION__,system_object_id);
            return status;
        }

        /* Persist the genereted lun object entry to disk */
        status = fbe_database_system_db_raw_persist_object_entry(FBE_DATABASE_SYSTEM_DB_PERSIST_UPDATE , object_entry_ptr);

        if(status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to persist object entry for object id 0x%x\n",
                           __FUNCTION__, system_object_id);
            return status;
        }

        /* Get the generated lun user entry from memory */
        status = fbe_database_config_table_get_user_entry_by_object_id(&(fbe_database_service.user_table), 
                                                                       system_object_id, 
                                                                       &user_entry_ptr);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: fail to get user entry by object id 0x%x\n",
                           __FUNCTION__, system_object_id);
            return status;
        }

        /*Persist the generated lun user entry to disk */
        status = fbe_database_system_db_raw_persist_user_entry(FBE_DATABASE_SYSTEM_DB_PERSIST_UPDATE , user_entry_ptr);

        if(status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to persist user entry for object id 0x%x\n",
                           __FUNCTION__, system_object_id);
            return status;
        }

        edge_index = 0;
        /* Get the generated lun edge entry from memory */
        status = fbe_database_config_table_get_edge_entry(&(fbe_database_service.edge_table),
                                                          system_object_id,
                                                          edge_index,
                                                          &edge_entry_ptr);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: fail to get edge entry by objid 0x%x and edge index 0x%x\n",
                           __FUNCTION__, system_object_id, edge_index);
            return status;
        }

        /* Persist the generated rg edge entry to disk */
        status = fbe_database_system_db_raw_persist_edge_entry(FBE_DATABASE_SYSTEM_DB_PERSIST_UPDATE , edge_entry_ptr);

        if(status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to persist edge entry(index: 0x%x) for object id 0x%x\n",
                           __FUNCTION__,edge_index, system_object_id);
            return status;
        }


        return FBE_STATUS_OK;
    }
    else
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: obj 0x%x is not a sytem rg or lun, config cannot be generated\n",
                       __FUNCTION__,system_object_id);
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }
}/**********************************************************************************
  * END of fbe_database_system_objects_generate_config_for_system_object_and_persist
  **********************************************************************************/

/*!***************************************************************
 * @fn fbe_database_system_objects_generate_config_for_all_system_rg_and_lun
 *****************************************************************
 * @brief
 *   generate the configuration for all system rg and lun
 *
 * @param  
 *
 * @return fbe_status_t - status
 *
 * @version
 *    May 24, 2013   Created: Yingying Song
 ****************************************************************/
fbe_status_t fbe_database_system_objects_generate_config_for_all_system_rg_and_lun(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t region_index, lun_info_index;
    fbe_private_space_layout_region_t region = {0};
    fbe_private_space_layout_lun_info_t lun_info = {0};
    fbe_u64_t                           current_commit_level;

    fbe_spinlock_lock(&fbe_database_service.system_db_header_lock);
    current_commit_level = fbe_database_service.system_db_header.persisted_sep_version;
    fbe_spinlock_unlock(&fbe_database_service.system_db_header_lock);

    /* generate config for all system RG */
    for(region_index = 0; region_index < FBE_PRIVATE_SPACE_LAYOUT_MAX_REGIONS; region_index++)
    {
        status = fbe_private_space_layout_get_region_by_index(region_index, &region);
        if (status != FBE_STATUS_OK) 
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "failed to get pri space layout for region index %d,stat:0x%x\n",
                           region_index,
                           status);
            return status;
        }
        else
        {
            if(region.region_id == FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_INVALID) 
            {
                // hit the terminator
                database_trace(FBE_TRACE_LEVEL_INFO, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "generate config for all system rgs done\n");
                break;
            }
            else if(region.region_type == FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_RAID_GROUP) 
            {

                /*First, generate raid group configuration from PSL*/
                status = database_system_objects_generate_rg_config_from_psl(&fbe_database_service,
                                                                             region.raid_info.object_id, 
                                                                             &region);
                if (status != FBE_STATUS_OK)
                {
                    database_trace(FBE_TRACE_LEVEL_ERROR, 
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: fail to generate config for rg: 0x%x\n",
                                   __FUNCTION__, region.raid_info.object_id);
                    return status;
                }
            }
        }

    }

    /* generate config for all psl luns  */
    for(lun_info_index = 0; lun_info_index < FBE_PRIVATE_SPACE_LAYOUT_MAX_PRIVATE_LUNS; lun_info_index++)
    {
        status = fbe_private_space_layout_get_lun_by_index(lun_info_index, &lun_info);
        if (status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "failed to get pri space layout for LUN index 0x%x,stat: 0x%x\n",
                           lun_info_index,
                           status);
        }
        else if(lun_info.lun_number == FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_INVALID) 
        {
            // hit the terminator
            database_trace(FBE_TRACE_LEVEL_INFO, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "generate config for all system luns done\n");
            break;
        }
        else if(lun_info.commit_level > current_commit_level)
        {
            // Nothing to do
        }
        else
        {

            /* Generate lun configuration WITHOUT "invalidate" option from PSL */
            status = database_system_objects_generate_lun_from_psl(&fbe_database_service,
                                                                   lun_info.object_id,
                                                                   &lun_info,
                                                                   FBE_FALSE,
                                                                   FBE_FALSE);
            if (status != FBE_STATUS_OK) 
            {
                database_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: fail to generate config for lun obj: 0x%x\n",
                               __FUNCTION__, lun_info.object_id);
            }
        }
    }

    database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                   "%s: generate config for all system rgs and luns done\n",
                   __FUNCTION__);

    return status;


}



/*!***************************************************************
 * @fn fbe_database_system_objects_generate_config_for_parity_rg_and_lun_from_psl
 *****************************************************************
 * @brief
 *   generate the configuration for raid group and related LUN from the PSL.
 *   Only the Raid with parity type will be generated again. And all the related
 *   LUN configuration will be generated too. 
 *
 *
 * @param  
 *
 * @return fbe_status_t - status
 *
 * @version
 *    11/02/2012:   created
 *
 ****************************************************************/
fbe_status_t fbe_database_system_objects_generate_config_for_parity_rg_and_lun_from_psl(fbe_bool_t ndb)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t region_index, lun_info_index;
    fbe_private_space_layout_region_t region = {0};
    fbe_private_space_layout_lun_info_t lun_info = {0};
    fbe_raid_group_number_t rg_number;
    fbe_u64_t current_commit_level;

    fbe_spinlock_lock(&fbe_database_service.system_db_header_lock);
    current_commit_level = fbe_database_service.system_db_header.persisted_sep_version;
    fbe_spinlock_unlock(&fbe_database_service.system_db_header_lock);

    /* generate config for parity RG */
    for(region_index = 0; region_index < FBE_PRIVATE_SPACE_LAYOUT_MAX_REGIONS; region_index++){
        status = fbe_private_space_layout_get_region_by_index(region_index, &region);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "db_sys_objs_create_sep_internal_objects:fail to get pri space layout for region index %d,stat:0x%x\n",
                           region_index,
                           status);
            return status;
        }
        else
        {
            // Hit the Terminator
            if(region.region_id == FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_INVALID) {
                break;
            }
            else if(region.region_type == FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_RAID_GROUP) {
                if (region.raid_info.raid_type == FBE_RAID_GROUP_TYPE_RAID3 ||
                    region.raid_info.raid_type == FBE_RAID_GROUP_TYPE_RAID5 ||
                    region.raid_info.raid_type == FBE_RAID_GROUP_TYPE_RAID6)
                {
                    status = fbe_database_lookup_raid_by_object_id(region.raid_info.object_id, &rg_number);
                    if (status != FBE_STATUS_NO_OBJECT)
                    {
                        if (rg_number != region.raid_info.raid_group_id) 
                        {
                            database_trace(FBE_TRACE_LEVEL_WARNING, 
                                           FBE_TRACE_MESSAGE_ID_INFO,
                                           "%s: the raid group number is not matched. rg_num: %d\n",
                                           __FUNCTION__, rg_number);
                        }
                        /*If the database configuration is already there, go to next one. */
                        continue;
                    }

                    rg_number = region.raid_info.raid_group_id;
                    /*First, generate raid group configuration from PSL*/
                    status = database_system_objects_generate_rg_config_from_psl(&fbe_database_service,
                                                                    region.raid_info.object_id, 
                                                                    &region);
                    if (status != FBE_STATUS_OK)
                    {
                        database_trace(FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s: fail to generate config for rg: %d\n",
                                    __FUNCTION__, region.raid_info.object_id);
                        return status;
                    }
                    
                    /*Second, generate the LUN configuration from the PSL*/
                    for(lun_info_index = 0; lun_info_index < FBE_PRIVATE_SPACE_LAYOUT_MAX_PRIVATE_LUNS; lun_info_index++)
                    {
                        status = fbe_private_space_layout_get_lun_by_index(lun_info_index, &lun_info);
                        if (status != FBE_STATUS_OK)
                        {
                            database_trace(FBE_TRACE_LEVEL_ERROR, 
                                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                           "db_sys_objs_create_sep_internal_objects:fail to get pri space layout for LUN index %d,stat: 0x%x\n",
                                           lun_info_index,
                                           status);
                            return status;
                        }
                        else if(lun_info.lun_number == FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_INVALID)
                        {
                            // hit the terminator
                            break;
                        }
                        else if(lun_info.commit_level > current_commit_level) {
                            // Do nothing
                        }
                        else if(rg_number == lun_info.raid_group_id)
                        {   
                            if(ndb == FBE_FALSE)
                            {
                                status = database_system_objects_generate_lun_from_psl(&fbe_database_service,
                                                                                       lun_info.object_id,
                                                                                       &lun_info,
                                                                                       FBE_FALSE,
                                                                                       FBE_TRUE);
                            }
                            else
                            {
                                status = database_system_objects_generate_lun_from_psl(&fbe_database_service,
                                                                                       lun_info.object_id,
                                                                                       &lun_info,
                                                                                       FBE_FALSE,
                                                                                       FBE_FALSE);
                            }

                            if (status != FBE_STATUS_OK)
                            {
                                database_trace(FBE_TRACE_LEVEL_ERROR, 
                                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                            "%s: fail to generate config for lun: %d\n",
                                            __FUNCTION__, lun_info.object_id);
                                return status;
                            }
                        }

                    }

                }
            }
        }

    }

    database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                   "%s: generate config done\n",
                   __FUNCTION__);

    return status;

}

/*!***************************************************************
 * @fn fbe_database_disable_peer_object
 *****************************************************************
 * @brief
 *   disable peer object .
 *
 * @param object_id - the object to set 
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    03/06/2015:   created
 *
 ****************************************************************/
fbe_status_t fbe_database_disable_peer_object(fbe_object_id_t  object_id)
{
    fbe_status_t status;

    /* Configure the base_config peer object is not alive. */
    status = fbe_database_send_packet_to_object(FBE_BASE_CONFIG_CONTROL_CODE_DISABLE_PEER_OBJECT,
                                                NULL,
                                                0,
                                                object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0);
    if(status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s Failed to send config to base_config object 0x%x\n",
                   __FUNCTION__, object_id);       
    }
    return status;
}

fbe_status_t database_system_objects_create_c4_mirror_object_and_its_edges (fbe_database_service_t *fbe_database_service, 
                                                                           fbe_object_id_t object_id,
                                                                           fbe_bool_t    is_initial_created)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    /* now create them in toplogy*/
	status = database_common_create_object_from_table(&fbe_database_service->object_table, 
                                                      object_id, 1);
    if(status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: create_object 0x%x failed. Status 0x%x \n",
                       __FUNCTION__,
                       object_id,
                       status);
        return status;
    }

    status = database_common_set_config_from_table(&fbe_database_service->object_table, 
                                                   object_id, 1);
    if(status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: set config to object 0x%x failed. Status 0x%x \n",
                       __FUNCTION__,
                       object_id,
                       status);
        return status;
    }
    status = database_common_create_edge_from_table(&fbe_database_service->edge_table,
                                                    object_id*DATABASE_MAX_EDGE_PER_OBJECT,
                                                    DATABASE_MAX_EDGE_PER_OBJECT);
    if(status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: create edge of object 0x%x failed. Status 0x%x \n",
                       __FUNCTION__,
                       object_id,
                       status);
        return status;
    }

    /* peer object is not alive, disable cmi */
    status = fbe_database_disable_peer_object(object_id); 

    if(status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: disable peer object 0x%x failed. Status 0x%x \n",
                       __FUNCTION__,
                       object_id,
                       status);
        return status;
    }

    status = database_common_commit_object_from_table(&fbe_database_service->object_table, 
                                                      object_id, 1, is_initial_created);
    if(status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: commit object 0x%x failed. Status 0x%x \n",
                       __FUNCTION__,
                       object_id,
                       status);
        return status;
    }

    return status;
}

fbe_status_t database_system_objects_create_c4_mirror_rg_from_psl(fbe_database_service_t *fbe_database_service,
                                                          fbe_object_id_t object_id, 
                                                          fbe_private_space_layout_region_t *region)
{
	fbe_status_t				status = FBE_STATUS_OK;

    if (region == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: psl region pointer is NULL\n",
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = database_system_objects_generate_rg_config_from_psl(fbe_database_service,
                                                        object_id,
                                                        region);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: fail to generate configuration from PSL for obj: %d\n",
                       __FUNCTION__, object_id);
        return status;
    }

    /* now create them in toplogy*/
    return database_system_objects_create_c4_mirror_object_and_its_edges(fbe_database_service, object_id, FBE_TRUE);
}

fbe_status_t database_system_objects_create_c4_mirror_lun_from_psl(fbe_database_service_t *fbe_database_service,
                                                                  fbe_object_id_t object_id, 
                                                                  fbe_private_space_layout_lun_info_t *lun_info,
                                                                  fbe_bool_t invalidate)
{
    fbe_status_t            status;

    status = database_system_objects_generate_lun_from_psl(fbe_database_service, 
                                                  object_id,
                                                  lun_info,
                                                  invalidate,
                                                  FBE_FALSE);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: fail to generate configuration from PSL for obj: %d\n",
                       __FUNCTION__, object_id);
        return status;
    }

    /* now create them in toplogy*/
    return database_system_objects_create_c4_mirror_object_and_its_edges(fbe_database_service, object_id, FBE_TRUE);
}

/*!***************************************************************
 * @fn fbe_database_reconstruct_export_lun_tables()
 *****************************************************************
 * @brief
 *   This function reconstruct the export lun table entries.
 *
 * @param void
 *
 * @return fbe_status_t - FBE_STATUS_OK
 *
 * @version
 *
 ****************************************************************/
fbe_status_t fbe_database_reconstruct_export_lun_tables(fbe_database_service_t *fbe_database_service)
{
    fbe_status_t                status;
    fbe_object_id_t             object_id;
    fbe_u32_t                   edge_index;
    database_object_entry_t    * object_entry_ptr = NULL;
    database_user_entry_t      * user_entry_ptr = NULL;
    database_edge_entry_t      * edge_entry_ptr = NULL;

    for (object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_UTILITY_BOOT_VOLUME_SPA;
         object_id <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_PRIMARY_BOOT_VOLUME_SPB;
         object_id++)
    {
        /* remove object entry */
        status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service->object_table,
                                                         object_id,
                                                         &object_entry_ptr);
        if (status == FBE_STATUS_OK) 
        {
            fbe_zero_memory(object_entry_ptr, sizeof(database_object_entry_t));
        }


        /* remove user entry */
        status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service->user_table,
                                                         object_id,
                                                         &user_entry_ptr);
        if (status == FBE_STATUS_OK) 
        {
            fbe_zero_memory(user_entry_ptr, sizeof(database_user_entry_t));
        }

        /* remove edge entry */
        for (edge_index = 0; edge_index < DATABASE_MAX_EDGE_PER_OBJECT; edge_index++ )
        {
            status = fbe_database_config_table_get_edge_entry(&fbe_database_service->edge_table,
                                                     object_id,
                                                     edge_index,
                                                     &edge_entry_ptr);
            if (status == FBE_STATUS_OK) 
            {
                fbe_zero_memory(edge_entry_ptr, sizeof(database_edge_entry_t));
            }
        }
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_is_object_c4_mirror()
 *****************************************************************
 * @brief
 *   This function determine whether the object is c4mirror rg or lun.
 *
 * @param object_id
 *
 * @return fbe_status_t - FBE_STATUS_OK
 *
 * @version
 *
 ****************************************************************/
fbe_bool_t fbe_database_is_object_c4_mirror(fbe_object_id_t object_id)
{
    if (object_id >= FBE_RPIVATE_SPACE_LAYOUT_OBJECT_ID_BOOT_VOLUME_FIRST &&
        object_id <= FBE_RPIVATE_SPACE_LAYOUT_OBJECT_ID_BOOT_VOLUME_LAST)
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}

/*!***************************************************************
 * @fn fbe_database_get_platform_info
 *****************************************************************
 * @brief
 *   This functions gets the platform information.
 *
 * @param platform_info
 *
 * @return fbe_status_t - status
 *
 * @version
 *
 ****************************************************************/
fbe_status_t fbe_database_get_platform_info(fbe_board_mgmt_platform_info_t *platform_info)
{
    fbe_topology_control_get_board_t         board;
    fbe_status_t                             status;

    fbe_zero_memory(platform_info, sizeof(fbe_board_mgmt_platform_info_t));

    /* First get the object id of the board */
    status = fbe_database_send_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_GET_BOARD,
                                                 &board,
                                                 sizeof(board),
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 NULL,
                                                 0,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if ( status != FBE_STATUS_OK ){
        return status;
    }

    /* Get the platform info of the board */
    status = fbe_database_send_packet_to_object(FBE_BASE_BOARD_CONTROL_CODE_GET_PLATFORM_INFO,
                                                platform_info,
                                                sizeof(fbe_board_mgmt_platform_info_t),
                                                board.board_object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_PHYSICAL);

    return status;
}

/*!***************************************************************
 * @fn fbe_database_set_sp_fault_led
 *****************************************************************
 * @brief
 *   This functions gets the platform information.
 *
 * @param blink_rate - LED_BLINK_RATE
 * @param status_condition - fbe_u32_t 
 *
 * @return fbe_status_t - status
 *
 * @version
 *
 ****************************************************************/
fbe_status_t fbe_database_set_sp_fault_led(LED_BLINK_RATE blink_rate, fbe_u32_t status_condition)
{
    fbe_topology_control_get_board_t         board;
    fbe_status_t                             status;
    fbe_board_mgmt_set_sp_fault_LED_t        sp_fault_LED;

    fbe_zero_memory(&sp_fault_LED, sizeof(fbe_board_mgmt_set_sp_fault_LED_t));

    /* First get the object id of the board */
    status = fbe_database_send_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_GET_BOARD,
                                                 &board,
                                                 sizeof(board),
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 NULL,
                                                 0,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if ( status != FBE_STATUS_OK ){
        return status;
    }
    
    sp_fault_LED.blink_rate = blink_rate;
    sp_fault_LED.status_condition = status_condition;

    /* Set the sp fault LED */
    status = fbe_database_send_packet_to_object(FBE_BASE_BOARD_CONTROL_CODE_SET_SP_FAULT_LED,
                                                &sp_fault_LED,
                                                sizeof(fbe_board_mgmt_set_sp_fault_LED_t),
                                                board.board_object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_PHYSICAL);

    return status;
}
