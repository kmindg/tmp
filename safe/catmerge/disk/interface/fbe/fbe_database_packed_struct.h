#ifndef FBE_DATABASE_PACKED_STRUCT_H
#define FBE_DATABASE_PACKED_STRUCT_H

/***************************************************************************/
/** @file fbe_database_packed_struct.h
***************************************************************************
*
* @brief
* This file contains the data structures used in database service tables 
* (user, object, edge and global info).  They are stored directly on disk 
* and reloaded at boot time.  They can not contain any pointer, as it will 
* not be valid on next boot.
* 
* These structure are also packed to ensure proper alignment.  
* 
* Changes in these structures can cause corrupted configuration data 
* during boot on hardware.  Please let the tech lead know if this file 
* is modified, so the update instruction can be communicated to proper audiences. 
* 
* If any field is modified or deleted, an invalidate disk is required 
* on hardware to avoid loading in a corrupted configuration. 
* 
* If you need to add additional fields to any of the structure, 
* add to the END of the structure only.
* 
***************************************************************************/
#include "fbe/fbe_lun.h"
#include "fbe/fbe_raid_group.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe/fbe_provision_drive.h"

/*base_config*/
/*!*******************************************************************
 * @struct fbe_base_config_configuration_t
 *********************************************************************
 * @brief
 *  This structure is used to set/get encryption related information in the system
 *
 * @ingroup fbe_api_encryption_interface
 *********************************************************************/
#pragma pack(1)
typedef struct fbe_base_config_configuration_s{
	fbe_base_config_encryption_mode_t	encryption_mode;/*!< object encryption mode */
}fbe_base_config_configuration_t;
#pragma pack()

/*PVD*/
/*!*******************************************************************
 * @struct fbe_provision_drive_configuration_t
 *********************************************************************
 * @brief
 *  This structure is describes the configuration of provision drive.
 *
 *********************************************************************/

typedef enum fbe_update_pvd_type_e {
    FBE_UPDATE_PVD_INVALID                  = 0,
	FBE_UPDATE_PVD_TYPE                     = 0x00000001,
	FBE_UPDATE_PVD_SNIFF_VERIFY_STATE       = 0x00000002,    
    FBE_UPDATE_PVD_POOL_ID                  = 0x00000004,
    FBE_UPDATE_PVD_SERIAL_NUMBER            = 0x00000008,
	FBE_UPDATE_PVD_CONFIG                   = 0x00000010,
	FBE_UPDATE_PVD_NONPAGED_BY_DEFALT_VALUE = 0x00000020,
	FBE_UPDATE_BASE_CONFIG_ENCRYPTION_MODE  = 0x00000040,
	FBE_UPDATE_PVD_LAST                     = 0x00000080,
	FBE_UPDATE_PVD_USE_BITMASK              = 0x00001000,
	FBE_UPDATE_PVD_AFTER_ENCRYPTION_ENABLED = 0x00080000,
    FBE_UPDATE_PVD_ENCRYPTION_IN_PROGRESS   = 0x01000000,
    FBE_UPDATE_PVD_REKEY_IN_PROGRESS        = 0x02000000,
    FBE_UPDATE_PVD_UNENCRYPTED              = 0x04000000,
} fbe_update_pvd_type_t;

#pragma pack(1)
typedef struct fbe_provision_drive_configuration_s
{
    /*! @note This allows to configure the provision drive to as hot spare.
       */
    fbe_provision_drive_config_type_t   config_type;

    /*! We can create provisioned drive when LDO does not exist yet.
     */
    fbe_lba_t configured_capacity;

    /*! We can create provisioned drive when LDO does not exist yet. 
     */
    fbe_provision_drive_configured_physical_block_size_t configured_physical_block_size;

    fbe_bool_t sniff_verify_state;

    /*! Generation number. */
    fbe_config_generation_t     generation_number;

    /*! serial number - used for PVD only. */
    fbe_u8_t            serial_num[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1];

    /*! Put any future user configured parameters here.
     */
    /*! Should be initialized to invalid. Used for update pvd config and abort transaction */
    fbe_update_pvd_type_t update_type;
    /*! This is added to support multiple updates at the same time */
    fbe_u32_t             update_type_bitmask;
}fbe_provision_drive_configuration_t;
#pragma pack()

#pragma pack(1)
typedef struct fbe_provision_drive_set_configuration_s
{
    fbe_provision_drive_configuration_t db_config;
    fbe_lba_t user_capacity;
}fbe_provision_drive_set_configuration_t;
#pragma pack()


/* VD */
typedef enum fbe_update_vd_type_e {
    FBE_UPDATE_VD_INVALID,
    FBE_UPDATE_VD_MODE    
} fbe_update_vd_type_t;


/*!*******************************************************************
 * @struct fbe_virtual_drive_configuration_t
 *********************************************************************
 * @brief
 *  This structure is used with the FBE_DATABASE_CONTROL_CODE_CREATE_VIRTUAL_DRIVE
 *
 *********************************************************************/
/* NOTE: when this data structure grows, the following codes need to be updated:
 * 1. update fbe_database_control_vd_set_configuration_t.
 *    Append new element at the end of that data strcuture.
 * 2. update function: fbe_database_set_vd_config and fbe_database_update_vd_config
 */
#pragma pack(1)
typedef struct fbe_virtual_drive_configuration_s
{
    /*! Virtual drive object gets created with configuration mode as 
     *  PASS_THRU_EDGE0 but it can change its configuration mode
     *  dynamically from the system itself.
     *  
     *  User does not allow to set the configuration mode other than
     *  PASS_THRU_EDGE0.
     */
    fbe_virtual_drive_configuration_mode_t   configuration_mode;

     /*! Should be initialized to invalid. Used for update vd config and abort transaction */
    fbe_update_vd_type_t                    update_type;

    /*! Put any future user configured parameters here.
     */
    fbe_lba_t                               vd_default_offset;

    /*! A job is in progress. 
     */
    fbe_bool_t b_job_in_progress;

}fbe_virtual_drive_configuration_t;
#pragma pack()

/* FBE_DATABASE_CONTROL_CODE_UPDATE_RAID */
typedef enum fbe_raid_update_type_e{
    FBE_UPDATE_RAID_TYPE_INVALID,
    FBE_UPDATE_RAID_TYPE_ENABLE_POWER_SAVE,
    FBE_UPDATE_RAID_TYPE_DISABLE_POWER_SAVE,
    FBE_UPDATE_RAID_TYPE_CHANGE_IDLE_TIME,
	FBE_UPDATE_RAID_TYPE_EXPAND_RG,
}fbe_update_raid_type_t;


/*!*******************************************************************
 * @struct fbe_raid_group_configuration_t
 *********************************************************************
 * @brief
 *  This structure is used with the
 *  FBE_RAID_GROUP_CONTROL_CODE_SET_CONFIGURATION control code.
 *
 *********************************************************************/

/* NOTE: when this data structure grows, the following codes need to be updated:
 * 1. update fbe_database_control_vd_set_configuration_t.
 *    Append new element at the end of that data strcuture.
 * 2. update function: fbe_database_set_vd_config and fbe_database_update_vd_config
 */
#pragma pack(1)
typedef struct fbe_raid_group_configuration_s
{
    /*! Number of drives in the raid group.
     */
    fbe_u32_t width;

    /*! The overall capacity of the raid group.
     */
    fbe_lba_t capacity;

    /*! The number of blocks in a raid group bitmap chunk.
     */
    fbe_chunk_size_t chunk_size;

    /*! This is the type of raid group (user configurable).
     */
    fbe_raid_group_type_t raid_type;

    /*! Size of each data element.
     */
    fbe_element_size_t           element_size;

    /*! Number of elements before parity rotates.
     */
    fbe_elements_per_parity_t   elements_per_parity;

    /*! raid group debug flags to force different options.
     */
    fbe_raid_group_debug_flags_t debug_flags;

    /*! how long do we wait w/o IO before we go to hibernate*/
    fbe_u64_t	power_saving_idle_time_in_seconds;

    /*! enable or disbale power saving of this objects*/
    fbe_bool_t	power_saving_enabled;

    /*! how many seconds the RAID user is willing to wait for the object to become
      ready after saving power*/
    fbe_u64_t max_raid_latency_time_in_sec;

    /*! Generation Number
     */
    fbe_config_generation_t generation_number;

    /*! Should be initialized to invalid. Used for update raid config and abort transaction */
    fbe_update_raid_type_t update_type;

    fbe_drive_type_t raid_group_drives_type;

}fbe_raid_group_configuration_t;
#pragma pack()


/*!*******************************************************************
 * @enum fbe_lun_config_flags_e
 * *********************************************************************
 * @brief
 *  This defines the individual bit flags that may be bitwise
 *  or'ed in the lun config.
 *
 *********************************************************************/
enum fbe_lun_config_flags_e
{
    FBE_LUN_CONFIG_NONE                 = 0x00,
    FBE_LUN_CONFIG_NO_INITIAL_VERIFY    = 0x01, /* maps to noinitialverify in navi bind. For system luns, it is always false */
    FBE_LUN_CONFIG_NO_USER_ZERO         = 0x02, /* maps to NDB, lun will not send "user zero" request */
    FBE_LUN_CONFIG_CLEAR_NEED_ZERO      = 0x04, /* only used with NDB on system luns that we don't want to zero at all. Even when invalidate disk is set */
    FBE_LUN_CONFIG_EXPROT_LUN           = 0x08, /* the lun should be exported by blkshim */
};
typedef fbe_u32_t  fbe_lun_config_flags_t;

/*!*******************************************************************
* @struct fbe_database_lun_configuration_t
*********************************************************************
* @brief
*  This structure is used with the
*  FBE_LUN_CONTROL_CODE_SET_CONFIGURATION control code.
*
*********************************************************************/
#pragma pack(1)
typedef struct fbe_database_lun_configuration_s{
	/*! The overall capacity of the LUN.
	*/
	fbe_lba_t capacity;

	/*! Generation Number
	*/
	fbe_config_generation_t generation_number;

	/*! power saving related information
	*/
	fbe_u64_t power_save_io_drain_delay_in_sec;
	fbe_u64_t max_lun_latency_time_is_sec;
	fbe_u64_t   power_saving_idle_time_in_seconds;
	fbe_bool_t  power_saving_enabled;
    /*! lun config flags, including noinitialverify, 
     * ndb and other boolean settings for lun 
     */
    fbe_lun_config_flags_t  config_flags;
}fbe_database_lun_configuration_t;
#pragma pack()


/*!*******************************************************************
 * @struct database_pvd_user_data_t
 *********************************************************************
 * @brief
 *  This is the definition of the user_entry, which stores all
 *  the data user asks us to store.  This data is persisted in
 *  FBE_PERSIST_SECTOR_TYPE_SEP_ADMIN_CONVERSION
 *
 *********************************************************************/
#pragma pack(1)
typedef struct database_pvd_user_data_s {
    fbe_u8_t        opaque_data[FBE_DATABASE_PROVISION_DRIVE_OPAQUE_DATA_MAX];
    fbe_u32_t       pool_id;
}database_pvd_user_data_t;
#pragma pack()

/*!*******************************************************************
 * @struct database_rg_user_data_t
 *********************************************************************
 * @brief
 *  This is the definition of the user_entry, which stores all
 *  the data user asks us to store.  This data is persisted in
 *  FBE_PERSIST_SECTOR_TYPE_SEP_ADMIN_CONVERSION
 *
 *********************************************************************/
#pragma pack(1)
typedef struct database_rg_user_data_s {
    fbe_bool_t              is_system;
    fbe_raid_group_number_t raid_group_number;
    fbe_bool_t              user_private;
}database_rg_user_data_t;
#pragma pack()

/*!*******************************************************************
 * @struct database_lu_user_data_t
 *********************************************************************
 * @brief
 *  This is the definition of the lu user data, which stores all
 *  the data user, such as unisphere asks us to store.  
 *
 *********************************************************************/
#pragma pack(1)
typedef struct database_lu_user_data_s {
    fbe_bool_t                  export_device_b;
    fbe_lun_number_t            lun_number;
    fbe_assigned_wwid_t         world_wide_name;
    fbe_user_defined_lun_name_t user_defined_name;
    fbe_time_t                  bind_time;
    fbe_bool_t                  user_private;
    fbe_u32_t                   attributes;
}database_lu_user_data_t;
#pragma pack()

/*!*******************************************************************
 * @struct fbe_system_power_saving_info_t
 *********************************************************************
 * @brief
 *  This structure is used to set power save related information in the system
 *
 * @ingroup fbe_api_power_save_interface
 *********************************************************************/
#pragma pack(1)
typedef struct fbe_system_power_saving_info_s{
	fbe_bool_t	enabled;/*!< enable or disable the system wide power saving */
	fbe_u64_t	hibernation_wake_up_time_in_minutes;/*!< how often should we wake up and sniff for a while when we are sleeping */
    fbe_bool_t  stats_enabled; /*!< Used to enable/disable Power Saving statistics logging */
}fbe_system_power_saving_info_t;
#pragma pack()

/*!*******************************************************************
 * @struct fbe_system_encryption_info_t
 *********************************************************************
 * @brief
 *  This structure is used to set/get encryption related information in the system
 *
 * @ingroup fbe_api_encryption_interface
 *********************************************************************/
#pragma pack(1)
typedef struct fbe_system_encryption_info_s{
    fbe_system_encryption_mode_t	encryption_mode;/*!< system wide encryption mode */
    fbe_u64_t                       encryption_stamp1; 
    fbe_u64_t                       encryption_stamp2;
    fbe_bool_t                      encryption_paused;
}fbe_system_encryption_info_t;
#pragma pack()

/*!*******************************************************************
 * @struct fbe_system_generation_info_t
 *********************************************************************
 * @brief
 *  This structure is used to track the generation number in the system
 *
 *********************************************************************/
#pragma pack(1)
typedef struct fbe_system_generation_info_s
{
    fbe_config_generation_t current_generation_number;
}fbe_system_generation_info_t;
#pragma pack()

/*!*******************************************************************
 * @struct fbe_system_time_threshold_info_t
 *********************************************************************
 * @brief
 *  This structure is used to set time_threshold related information in the system
 *  time_threshold is uesed for pvd garbage collection. If the pvd unused_time
 *  reaches the threshold, it should be destroyed.
 *
 *********************************************************************/
#pragma pack(1)
typedef struct fbe_system_time_threshold_info_s{
    fbe_u64_t    time_threshold_in_minutes;/*! how long before we clean up the pemanently removed pvd */
}fbe_system_time_threshold_info_t;
#pragma pack()

/*!*******************************************************************
 * @struct fbe_global_pvd_config_t
 *********************************************************************
 * @brief
 *  This structure is used to set/get pvd related information in the system
 *
 *********************************************************************/
#pragma pack(1)
typedef struct fbe_global_pvd_config_s{
	fbe_u32_t	user_capacity_limit; /*!< capacity limit for user drives */
}fbe_global_pvd_config_t;
#pragma pack()

/* */
typedef enum fbe_lun_update_type_e {
    FBE_LUN_UPDATE_INVALID,
	FBE_LUN_UPDATE_WWN,
	FBE_LUN_UPDATE_UDN,    
    FBE_LUN_UPDATE_ATTRIBUTES,
} fbe_lun_update_type_t;

/*!*******************************************************************
 * @struct fbe_extent_pool_configuration_t
 *********************************************************************
 * @brief
 *  This is the definition of the object_entry.
 *
 *********************************************************************/
#pragma pack(1)
typedef struct fbe_extent_pool_configuration_s
{
    /*! Number of drives in the pool.
     */
    fbe_u32_t width;

    /*! The overall capacity of pool.
     */
    fbe_lba_t capacity;

    /*! Generation Number
     */
    fbe_config_generation_t generation_number;

    fbe_drive_type_t drives_type;

} fbe_extent_pool_configuration_t;
#pragma pack()

/*!*******************************************************************
 * @struct database_ext_pool_user_data_t
 *********************************************************************
 * @brief
 *  This is the definition of the user_entry, which stores all
 *  the data user asks us to store.  This data is persisted in
 *  FBE_PERSIST_SECTOR_TYPE_SEP_ADMIN_CONVERSION
 *
 *********************************************************************/
#pragma pack(1)
typedef struct database_ext_pool_user_data_s {
    fbe_u32_t       pool_id;
}database_ext_pool_user_data_t;
#pragma pack()

/*!*******************************************************************
 * @struct fbe_ext_pool_lun_configuration_t
 *********************************************************************
 * @brief
 *  This is the definition of the object_entry.
 *
 *********************************************************************/
#pragma pack(1)
typedef struct fbe_ext_pool_lun_configuration_s
{
    /*! The overall capacity of lun.
     */
    fbe_lba_t capacity;

    /*! Generation Number
     */
    fbe_config_generation_t generation_number;

    /*! edge offset
     */
    fbe_lba_t offset;

    /*! Server_index
     */
    fbe_edge_index_t server_index;
} fbe_ext_pool_lun_configuration_t;
#pragma pack()

/*!*******************************************************************
 * @struct database_ext_pool_lun_user_data_t
 *********************************************************************
 * @brief
 *  This is the definition of the user_entry, which stores all
 *  the data user asks us to store.  This data is persisted in
 *  FBE_PERSIST_SECTOR_TYPE_SEP_ADMIN_CONVERSION
 *
 *********************************************************************/
#pragma pack(1)
typedef struct database_ext_pool_lun_user_data_s {
    fbe_u32_t            pool_id;
    fbe_u32_t            lun_id;
    fbe_assigned_wwid_t  world_wide_name;
    fbe_user_defined_lun_name_t user_defined_name;
}database_ext_pool_lun_user_data_t;
#pragma pack()

#endif /* FBE_DATABASE_PACKED_STRUCT_H */

