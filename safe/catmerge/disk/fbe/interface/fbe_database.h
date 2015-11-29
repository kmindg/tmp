#ifndef FBE_DATABASE_H
#define FBE_DATABASE_H

/***************************************************************************/
/** @file fbe_database.h
***************************************************************************
*
* @brief
*  This file contains definitions of functions that are exported by the 
*  database service
* 
***************************************************************************/
#include "fbe/fbe_service.h"
#include "fbe/fbe_raid_group.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe/fbe_lun.h"
#include "fbe_block_transport.h"
#include "fbe/fbe_database_packed_struct.h"
#include "fbe/fbe_persist_interface.h"
#include "fbe_fru_descriptor_structure.h"
#include "fbe_fru_signature.h"
#include "fbe/fbe_encryption.h"
#include "fbe/fbe_cmi_interface.h"

#include "ndu_toc_common.h"
#include "k10diskdriveradmin.h"

#define FBE_DATABASE_TRANSACTION_MAX_CREATE_OBJECTS_PER_JOB  25     /*moved from fbe_job_service.h here to solve recursive include*/    
#define FBE_DATABASE_TRANSACTION_DESTROY_UNCONSUMED_PVDS_ID (FBE_OBJECT_ID_INVALID+1)
#define FBE_DATABASE_MAX_GARBAGE_COLLECTION_PVDS  10
#define FBE_DATABASE_FORCED_GARBAGE_COLLECTION_DEBOUNCE_TIME_DEFAULT_MS 60000   /*60 sec */

#define SEP_K10ATTACH_CURRENT_VERSION  K10ATTACH_CURRENT_VERSION   /* The version must be the same as K10ATTACH_CURRENT_VERSION, */

#define INVALID_SEP_PACKAGE_VERSION 0xFFFFFFFFFFFFFFFE

#define FBE_START_TRANSACTION_RETRY_INTERVAL    3000
#define FBE_START_TRANSACTION_RETRIES           10

// max number of full polls before complaining
#define FBE_DATABASE_MAX_POLL_RECORDS                45 /* max number of polls to keep track of */

#define FBE_PORT_SERIAL_NUM_SIZE 16
#define FBE_MAX_BE_PORTS_PER_SP 32

/*!*******************************************************************
 * @def SEP_DB_DATA_MEM_MB
 *********************************************************************
 * @brief
 *   This is the total memory in MB assigned from cmm for Database for
 *   non dumpable data memory.
 *
 *********************************************************************/
#define SEP_DB_DATA_MEM_MB 10

/*!********************************************************************* 
* @typedef fbe_database_object_visibility_t 
*
* @brief 
*      Definition of object visibility
**********************************************************************/
typedef enum fbe_database_object_group_e{
    FBE_DATABASE_OBJECT_GROUP_INVALID,
    FBE_DATABASE_OBJECT_GROUP_SYSTEM,
    FBE_DATABASE_OBJECT_GROUP_LAST
}fbe_database_object_group_t;

typedef struct fbe_database_lun_info_s
{
    fbe_object_id_t             lun_object_id;
    fbe_raid_group_get_info_t   raid_info;
    fbe_u8_t                    lun_characteristics; // e.x. Thin, Replica etc
    fbe_u16_t                   rotational_rate; //0x01 for SSD bound lun
    fbe_lba_t                   capacity;           // capacity of LUN as seen by the user(user capacity)
    fbe_lba_t                   overall_capacity;   // capacity of LUN in SEP(user capacity + extra space for cache) 
    fbe_lba_t                   offset;             // offset of LUN in RG
    fbe_assigned_wwid_t         world_wide_name;   
    fbe_user_defined_lun_name_t user_defined_name;
    fbe_object_id_t             raid_group_obj_id;  
    fbe_u32_t					attributes;
    fbe_time_t                  bind_time;
    fbe_bool_t                  user_private;
    /*these are extended fileds used mostly by Admin*/
    fbe_lun_number_t 			lun_number;
    fbe_raid_group_number_t 	rg_number;
    fbe_lun_get_zero_status_t 	lun_zero_status;
    fbe_lifecycle_state_t 		lifecycle_state;
    fbe_lun_rebuild_status_t    rebuild_status;
    fbe_bool_t                  is_degraded; /*whether the LUN is degraded now*/
} fbe_database_lun_info_t;


/*!*******************************************************************
 * @enum fbe_database_state_e
 *********************************************************************
 * @brief
 *  This is the set of state of the database service.
 *
 *********************************************************************/
typedef enum fbe_database_state_e {
    FBE_DATABASE_STATE_INVALID,
    FBE_DATABASE_STATE_INITIALIZING,
    FBE_DATABASE_STATE_INITIALIZED,
    FBE_DATABASE_STATE_READY,
    FBE_DATABASE_STATE_FAILED,	
    FBE_DATABASE_STATE_UPDATING_PEER,
    FBE_DATABASE_STATE_WAITING_FOR_CONFIG,
    FBE_DATABASE_STATE_DEGRADED,
    FBE_DATABASE_STATE_DESTROYING,
    FBE_DATABASE_STATE_SERVICE_MODE,
    FBE_DATABASE_STATE_CORRUPT,
    FBE_DATABASE_STATE_DESTROYING_SYSTEM,
    FBE_DATABASE_STATE_WAITING_ON_KMS,
    FBE_DATABASE_STATE_KMS_APPROVED,
    FBE_DATABASE_STATE_BOOTFLASH_SERVICE_MODE,

    FBE_DATABASE_STATE_LAST
} fbe_database_state_t;

/*********************************************************************/
/** @enum fbe_database_control_code_t
*********************************************************************
* @brief enums for database service control codes
*********************************************************************/
typedef enum fbe_database_control_code_e {
    FBE_DATABASE_CONTROL_CODE_INVALID = FBE_SERVICE_CONTROL_CODE_INVALID_DEF(FBE_SERVICE_ID_DATABASE),

    FBE_DATABASE_CONTROL_CODE_TRANSACTION_START,           /**< Start a Transaction   */
    FBE_DATABASE_CONTROL_CODE_TRANSACTION_COMMIT,          /**< Commit a Transaction  */
    FBE_DATABASE_CONTROL_CODE_TRANSACTION_ABORT,           /**< Abort a Transaction   */

    FBE_DATABASE_CONTROL_CODE_CREATE_VD,					/**< Create a VD */
    FBE_DATABASE_CONTROL_CODE_DESTROY_VD,					/**< Destroy a VD */
    FBE_DATABASE_CONTROL_CODE_GET_VD,					    /**< Retrieve configuration for a VD */
    FBE_DATABASE_CONTROL_CODE_UPDATE_VD,				    /**< Change configuration for a VD */

    FBE_DATABASE_CONTROL_CODE_CREATE_PVD,				    /**< Create a PVD */
    FBE_DATABASE_CONTROL_CODE_DESTROY_PVD,				    /**< Destroy a PVD */
    FBE_DATABASE_CONTROL_CODE_UPDATE_PVD,					/**< Change the configudation for PVD */

    FBE_DATABASE_CONTROL_CODE_CREATE_RAID,                 /**< Create a RAID */
    FBE_DATABASE_CONTROL_CODE_DESTROY_RAID,                /**< Destroy a RAID */
    FBE_DATABASE_CONTROL_CODE_GET_RAID,                    /**< Get raid configuration */
    FBE_DATABASE_CONTROL_CODE_UPDATE_RAID,                 /**< change info in the existing RG object */
    FBE_DATABASE_CONTROL_CODE_LOOKUP_RAID_BY_NUMBER,       /**< Find a RAID by RAID number */
    FBE_DATABASE_CONTROL_CODE_LOOKUP_RAID_BY_OBJECT_ID,    /**< Find a RAID by object id */
    FBE_DATABASE_CONTROL_CODE_GET_RAID_USER_PRIVATE,       /**< Get RAID is a private one or not */

    FBE_DATABASE_CONTROL_CODE_CREATE_LUN,                  /**< Create a LUN */
    FBE_DATABASE_CONTROL_CODE_DESTROY_LUN,                 /**< Destroy a LUN */
    FBE_DATABASE_CONTROL_CODE_GET_LUN,
    FBE_DATABASE_CONTROL_CODE_UPDATE_LUN,                  /**< change info in the existing LUN object */
    FBE_DATABASE_CONTROL_CODE_LOOKUP_LUN_BY_NUMBER,        /**< Find a LUN by LUN number*/
    FBE_DATABASE_CONTROL_CODE_LOOKUP_LUN_BY_OBJECT_ID,     /**< Find a LUN by object id */
    FBE_DATABASE_CONTROL_CODE_GET_LUN_INFO,

    FBE_DATABASE_CONTROL_CODE_CLONE_OBJECT,                /**< Clone source object to destination one */

    FBE_DATABASE_CONTROL_CODE_PERSIST_PROM_WWNSEED,                /**< Persist wwnseed to resume prom on board>*/
    FBE_DATABASE_CONTROL_CODE_OBTAIN_PROM_WWNSEED,                 /**< Get wwnseed from resume prom on board>*/
    
    FBE_DATABASE_CONTROL_CODE_CREATE_EDGE,                 /**< Create an edge */
    FBE_DATABASE_CONTROL_CODE_DESTROY_EDGE,                /**< Destroy an edge */
    FBE_DATABASE_CONTROL_CODE_SWAP_EDGE,                   /**< Change the configudation for PVD */

    FBE_DATABASE_CONTROL_SET_POWER_SAVE,                   /**< Set power saving configuration */
    FBE_DATABASE_CONTROL_GET_POWER_SAVE,                   /**< Retrieve power saving configuration */

    FBE_DATABASE_CONTROL_CODE_UPDATE_SYSTEM_SPARE_CONFIG,         /**< Update the spare configuration info. */

    FBE_DATABASE_CONTROL_CODE_ENUMERATE_SYSTEM_OBJECTS,    /**< Enumerate system objects */
    FBE_DATABASE_CONTROL_CODE_LOOKUP_SYSTEM_OBJECT_ID,

    FBE_DATABASE_CONTROL_CODE_GET_EMV,						/**< Get the EMV value stored in DB*/
    FBE_DATABASE_CONTROL_CODE_SET_EMV,						/**< Persist the EMV value in DB*/

    FBE_DATABASE_CONTROL_CODE_GET_STATS,                   /**< Get object stats */
    FBE_DATABASE_CONTROL_CODE_GET_STATE,				   /**< Get service state */
    FBE_DATABASE_CONTROL_CODE_GET_TABLES,				   /**< Get database tables */

    FBE_DATABASE_CONTROL_CODE_GET_TRANSFER_LIMITS,         /**< Get any transfer limits (max bytes per drive request etc) */
    FBE_DATABASE_CONTROL_CODE_GET_INTERNALS,               /**< Access internal values - for debugging */
    FBE_DATABASE_CONTROL_CODE_SET_MASTER_RECORD,           /**< Set the master record.*/
    FBE_DATABASE_CONTROL_CODE_UPDATE_PEER_CONFIG,			/**< give the peer a copy of our configuration*/			
    FBE_DATABASE_CONTROL_CODE_GET_TRANSACTION_INFO,        /*get in-progress transaction information*/
    FBE_DATABASE_CONTROL_CODE_SYSTEM_DB_OP_CMD,		       /*database system db operation*/
    FBE_DATABASE_CONTROL_CODE_GET_SERVICE_MODE_REASON, /* get the reason enter service mode */
    FBE_DATABASE_CONTROL_CODE_GET_FRU_DESCRIPTOR, /*get fru descriptor*/
    FBE_DATABASE_CONTROL_CODE_GET_DISK_SIGNATURE,   /*get disk fru signature*/

    FBE_DATABASE_CONTROL_CODE_SET_DISK_SIGNATURE,   /*set disk fru signature*/
    FBE_DATABASE_CONTROL_CODE_SET_FRU_DESCRIPTOR,
    FBE_DATABASE_CONTROL_CODE_CLEAR_DISK_SIGNATURE,
        
    FBE_DATABASE_CONTROL_SET_PVD_DESTROY_TIMETHRESHOLD,                   /**< Set time threshold configuration. 
                                                               If pvd unused time reaches the threshold, it should be destroyed */
    FBE_DATABASE_CONTROL_GET_PVD_DESTROY_TIMETHRESHOLD,                   /**< Retrieve time threshold configuration */
    FBE_DATABASE_CONTROL_CODE_UPDATE_PEER_SYSTEM_BG_SERVICE_FLAG,            /**< update system background services flag on Peer */
    
    FBE_DATABASE_CONTROL_CODE_SET_OBJECT_STATE_COND,     /*not used anymore. Leave it here just to avoid disruptive upgrade*/
    FBE_DATABASE_CONTROL_CODE_LOOKUP_LUN_BY_WWID,        /**< Find a LUN by LUN WWN*/
    FBE_DATABASE_CONTROL_CODE_CLEAR_PENDING_TRANSACTION,
    FBE_DATABASE_CONTROL_CODE_GET_COMPAT_MODE,          /**< Get the Sep package version(compiled version and commmitted version */
    FBE_DATABASE_CONTROL_CODE_COMMIT,                   /**< Deliver "NDU commit" to database service*/
    FBE_DATABASE_CONTROL_CODE_SET_NDU_STATE,            /**< Send NDU state to database service*/
    FBE_DATABASE_CONTROL_CODE_GET_ALL_LUNS, 			/**< Returns all LUNS in one shot*/
    FBE_DATABASE_CONTROL_CODE_GET_ALL_RAID_GROUPS,      /**< Returns all RAID GroupS in one shot*/
    FBE_DATABASE_CONTROL_CODE_GET_ALL_PVDS,             /**< Returns all PVDS in one shot*/
    FBE_DATABASE_CONTROL_CODE_GET_RAID_GROUP,      		/**< Returns info about a single RG*/
    FBE_DATABASE_CONTROL_CODE_GET_PVD,             		/**< Returns info about a single PVD*/
    FBE_DATABASE_CONTROL_CODE_COMMIT_DATABASE_TABLES,   /*update database tables in the memory and on the disk*/
    FBE_DATABASE_CONTROL_CODE_VERSIONING,               /* check database versioning related information */
    FBE_DATABASE_CONTROL_CODE_STOP_ALL_BACKGROUND_SERVICES,   /* to be used before vault dump*/
    FBE_DATABASE_CONTROL_CODE_RESTART_ALL_BACKGROUND_SERVICES,   /* to be used to restart after FBE_DATABASE_CONTROL_CODE_STOP_ALL_BACKGROUND_SERVICES was called*/
    FBE_DATABASE_CONTROL_CODE_CLEANUP_RE_INIT_PERSIST_SERVICE,   /* Clear the database LUN and re-initalize the persist service, for config restore*/
    FBE_DATABASE_CONTROL_CODE_RESTORE_USER_CONFIGURATION,        /* restore user configuration to database LUN*/

    FBE_DATABASE_CONTROL_ENABLE_LOAD_BALANCE,
    FBE_DATABASE_CONTROL_DISABLE_LOAD_BALANCE,

    FBE_DATABASE_CONTROL_CODE_GET_SYSTEM_DB_HEADER, /*Get the system db header in database service*/
    FBE_DATABASE_CONTROL_CODE_SET_SYSTEM_DB_HEADER, /*Set the system db header in database service*/
    FBE_DATABASE_CONTROL_CODE_INIT_SYSTEM_DB_HEADER, /*Init the system db header in database service*/
    FBE_DATABASE_CONTROL_CODE_PERSIST_SYSTEM_DB_HEADER,/*Persist the system db header in dump file to disk*/

    FBE_DATABASE_CONTROL_CODE_GET_DISK_DDMI_DATA,      /*  Get disk Data Directory Management Index data */

    FBE_DATABASE_CONTROL_CODE_GET_SYSTEM_OBJECT_RECREATE_FLAGS, /*Get the system object recreation operation flags */
    FBE_DATABASE_CONTROL_CODE_PERSIST_SYSTEM_OBJECT_OP_FLAGS, /*Persist the system object operation flags*/

    FBE_DATABASE_CONTROL_CODE_GENERATE_CONFIGURATION_FOR_SYSTEM_PARITY_RG_AND_LUN, /* generate config for system parity RGs and LUNs */
    FBE_DATABASE_CONTROL_CODE_GENERATE_SYSTEM_OBJECT_CONFIG_AND_PERSIST, /* generate config for specified object from PSL and persist to disk */
    FBE_DATABASE_CONTROL_CODE_GENERATE_ALL_SYSTEM_OBJECTS_CONFIG_AND_PERSIST, /*generate config for all system rgs and luns and persit to disk */

    FBE_DATABASE_CONTROL_MAKE_PLANNED_DRIVE_ONLINE,

    FBE_DATABASE_CONTROL_CODE_GET_TABLES_IN_RANGE_WITH_TYPE,/*Get the object entries,user entries and edge entries in a range*/
    FBE_DATABASE_CONTROL_CODE_PERSIST_SYSTEM_DB,/*persist all the entries in system db to db raw morrir*/
    FBE_DATABASE_CONTROL_CODE_GET_MAX_CONFIGURABLE_OBJECTS,/*get the max objects number in database*/
    FBE_DATABASE_CONTROL_CODE_CONNECT_DRIVES,
    FBE_DATABASE_CONTROL_CODE_SET_INVALIDATE_CONFIG_FLAG,/*Set the invalidate configuration flag.*/
    FBE_DATABASE_CONTROL_CODE_HOOK, /*set/unset hook in database service*/

    
    FBE_DATABASE_CONTROL_CODE_MARK_LU_OPERATION_FROM_CLI, /*send a ktrace warning message that lun operation was done from cli*/
    FBE_DATABASE_CONTROL_CODE_GET_LU_RAID_INFO, /* Return the raid information on the LUN. */
    FBE_DATABASE_CONTROL_CODE_SETUP_ENCRYPTION_KEY, /* Sets up the object with Encryption Keys */

    FBE_DATABASE_CONTROL_SET_ENCRYPTION,                   /**< Set system encryption mode */
    FBE_DATABASE_CONTROL_GET_ENCRYPTION,                   /**< Retrieve system encryption mode */
    FBE_DATABASE_CONTROL_CODE_GET_PEER_STATE,              /* return peer state */

    FBE_DATABASE_CONTROL_CODE_UPDATE_ENCRYPTION_MODE,  /* Updated the encryption mode */

    FBE_DATABASE_CONTROL_CODE_SETUP_KEK, 
    FBE_DATABASE_CONTROL_CODE_DESTROY_KEK,

    FBE_DATABASE_CONTROL_CODE_SETUP_KEK_KEK, 
    FBE_DATABASE_CONTROL_CODE_DESTROY_KEK_KEK,
    FBE_DATABASE_CONTROL_CODE_REESTABLISH_KEK_KEK, 
    
    FBE_DATABASE_CONTROL_CODE_GET_TOTAL_LOCKED_OBJECTS_OF_CLASS, /* Returns total number of Objects in locked state */
    FBE_DATABASE_CONTROL_CODE_GET_LOCKED_OBJECT_INFO_OF_CLASS, /* Returns information about the Objects in the locked state */

    FBE_DATABASE_CONTROL_CODE_SETUP_ENCRYPTION_REKEY, /* Sets up the object with additional Encryption Keys */

    FBE_DATABASE_CONTROL_CODE_GET_CAPACITY_LIMIT, /* Gets the limit for PVD exported capacity */
    FBE_DATABASE_CONTROL_CODE_SET_CAPACITY_LIMIT, /* Sets the limit for PVD exported capacity */

    FBE_DATABASE_CONTROL_CODE_REMOVE_ENCRYPTION_KEYS, /* Remove the object with Encryption Keys */
    FBE_DATABASE_CONTROL_CODE_GET_SYSTEM_ENCRYPTION_INFO,
    FBE_DATABASE_CONTROL_CODE_GET_SYSTEM_ENCRYPTION_PROGRESS,
    FBE_DATABASE_CONTROL_CODE_UPDATE_DRIVE_KEYS, /* Update the object with Encryption Keys */

    FBE_DATABASE_CONTROL_CODE_SCRUB_OLD_USER_DATA,

    FBE_DATABASE_CONTROL_CODE_GET_BACKUP_INFO, /* Gets backup information */

    FBE_DATABASE_CONTROL_CODE_GET_BE_PORT_INFO,

    FBE_DATABASE_CONTROL_CODE_SET_BACKUP_INFO, /* Sets backup information */

    FBE_DATABASE_CONTROL_CODE_SET_UPDATE_DB_ON_PEER, 

    FBE_DATABASE_CONTROL_CODE_GET_DRIVE_SN_FOR_RAID, /* Get drive serial numbers for a RG */
    FBE_DATABASE_CONTROL_CODE_SET_PORT_ENCRYPTION_MODE, /* Sets the port encryption mode */
    FBE_DATABASE_CONTROL_CODE_GET_PORT_ENCRYPTION_MODE, /* Gets the port encryption mode */

    FBE_DATABASE_CONTROL_CODE_GET_SYSTEM_SCRUB_PROGRESS,

    FBE_DATABASE_CONTROL_CODE_SET_POLL_INTERVAL,  /* controls the frequency to update */
    FBE_DATABASE_CONTROL_CODE_SET_EXTENDED_CACHE_ENABLED, /* set whether to shoot vault drives base on drive tier criteria */

    FBE_DATABASE_CONTROL_SET_ENCRYPTION_PAUSED,
    FBE_DATABASE_CONTROL_GET_ENCRYPTION_PAUSED,

    FBE_DATABASE_CONTROL_SET_GARBAGE_COLLECTION_DEBOUNCER,

    FBE_DATABASE_CONTROL_CODE_CREATE_EXT_POOL,
    FBE_DATABASE_CONTROL_CODE_DESTROY_EXT_POOL,
    FBE_DATABASE_CONTROL_CODE_CREATE_EXT_POOL_LUN,
    FBE_DATABASE_CONTROL_CODE_DESTROY_EXT_POOL_LUN,
    FBE_DATABASE_CONTROL_CODE_CREATE_EXT_POOL_METADATA_LUN,
    FBE_DATABASE_CONTROL_CODE_DESTROY_EXT_POOL_METADATA_LUN,
    FBE_DATABASE_CONTROL_GET_PVD_LIST_FOR_EXT_POOL,
    FBE_DATABASE_CONTROL_CODE_LOOKUP_EXT_POOL_BY_NUMBER,
    FBE_DATABASE_CONTROL_CODE_LOOKUP_EXT_POOL_LUN_BY_NUMBER,

    FBE_DATABASE_CONTROL_MARK_PVD_SWAP_PENDING,
    FBE_DATABASE_CONTROL_CLEAR_PVD_SWAP_PENDING,

    FBE_DATABASE_CONTROL_CODE_SET_DEBUG_FLAGS,
    FBE_DATABASE_CONTROL_CODE_GET_DEBUG_FLAGS,

    FBE_DATABASE_CONTROL_CODE_IS_VALIDATION_ALLOWED,
    FBE_DATABASE_CONTROL_CODE_VALIDATE_DATABASE,

    FBE_DATABASE_CONTROL_CODE_ENTER_DEGRADED_MODE,
    FBE_DATABASE_CONTROL_CODE_GET_PEER_SEP_VERSION,
    FBE_DATABASE_CONTROL_CODE_SET_PEER_SEP_VERSION,

    FBE_DATABASE_CONTROL_CODE_UPDATE_PVD_BLOCK_SIZE,					/**< ENGINEERING ONLY - Change PVD's physical block size */
    FBE_DATABASE_CONTROL_GET_ADDL_SUPPORTED_DRIVE_TYPES,
    FBE_DATABASE_CONTROL_SET_ADDL_SUPPORTED_DRIVE_TYPES,
    FBE_DATABASE_CONTROL_CODE_CHECK_BOOTFLASH_MODE,		
    FBE_DATABASE_CONTROL_CODE_UPDATE_MIRROR_PVD_MAP,
    FBE_DATABASE_CONTROL_CODE_GET_MIRROR_PVD_MAP,
    

    FBE_DATABASE_CONTROL_CODE_LAST
}fbe_database_control_code_t;


/* transaction stuff */
typedef fbe_u64_t fbe_database_transaction_id_t;
#define FBE_DATABASE_TRANSACTION_ID_INVALID 0

typedef enum fbe_database_transaction_type_e {
    FBE_DATABASE_TRANSACTION_CREATE = 0,
    FBE_DATABASE_TRANSACTION_RECOVERY,
    FBE_DATABASE_TRANSACTION_LAST
} fbe_database_transaction_type_t;

typedef struct fbe_database_transaction_info_s{
    fbe_database_transaction_id_t   transaction_id;
    fbe_database_transaction_type_t transaction_type;
    fbe_u64_t   job_number;  /*job number of the job that starts this transaction*/ 
} fbe_database_transaction_info_t;

/*PVD*/
typedef struct fbe_database_control_pvd_s
{
    fbe_object_id_t                           object_id;
    fbe_database_transaction_id_t             transaction_id;	
    fbe_provision_drive_configuration_t   pvd_configurations;	
} fbe_database_control_pvd_t;

typedef struct fbe_database_control_update_pvd_s
{    
    fbe_object_id_t                        object_id;
    fbe_database_transaction_id_t          transaction_id;
    fbe_provision_drive_config_type_t      config_type;
    fbe_update_pvd_type_t                  update_type;
    fbe_u32_t                              update_type_bitmask;
    fbe_bool_t                             sniff_verify_state;
    fbe_bool_t                             update_opaque_data;
    fbe_u8_t                               opaque_data[FBE_DATABASE_PROVISION_DRIVE_OPAQUE_DATA_MAX]; /* opaque_data for the spare stuff*/
    fbe_u32_t                              pool_id;
    fbe_lba_t                               configured_capacity;
    fbe_provision_drive_configured_physical_block_size_t configured_physical_block_size;
    fbe_base_config_encryption_mode_t      base_config_encryption_mode;
    fbe_u8_t            serial_num[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1];
} fbe_database_control_update_pvd_t;

typedef struct fbe_database_control_update_pvd_block_size_s
{    
    fbe_object_id_t                        object_id;
    fbe_database_transaction_id_t          transaction_id;
    fbe_provision_drive_configured_physical_block_size_t configured_physical_block_size;
} fbe_database_control_update_pvd_block_size_t;


typedef struct fbe_database_control_update_encryption_mode_s
{    
    fbe_object_id_t                        object_id;
    fbe_database_transaction_id_t          transaction_id;
    fbe_base_config_encryption_mode_t         base_config_encryption_mode;
} fbe_database_control_update_encryption_mode_t;

typedef struct fbe_database_control_user_private_raid_s
{
    fbe_object_id_t             object_id;
    fbe_bool_t                  user_private;
} fbe_database_control_user_private_raid_t;


typedef struct fbe_database_control_lookup_system_object_s {
    //fbe_database_system_object_t     system_object;
    fbe_object_id_t                   object_id;
} fbe_database_control_lookup_system_object_t;

/* VD */
typedef struct fbe_database_control_vd_set_configuration_s
{
    /* The first part is the fbe_raid_group_configuration. 
    * Includes all the fields of fbe_raid_group_configuration_t
    * When fbe_raid_group_configuration_t grows, 
    * append the new element at the end of this data structure
    */
    fbe_u32_t width;
    fbe_lba_t capacity;
    fbe_chunk_size_t chunk_size;
    fbe_raid_group_type_t raid_type;
    fbe_element_size_t           element_size;
    fbe_elements_per_parity_t   elements_per_parity;
    fbe_raid_group_debug_flags_t debug_flags;
    fbe_u64_t	power_saving_idle_time_in_seconds;
    fbe_bool_t	power_saving_enabled;
    fbe_u64_t max_raid_latency_time_in_sec;
    fbe_config_generation_t generation_number;
    fbe_update_raid_type_t update_rg_type;

    /*! The following are the other virtual drive configuration
    *  parameters in addition to the raid group parameters.
    *  includes all the fields of fbe_virtual_drive_configuration_t
    *  When fbe_virtual_drive_configuration_t grows,
    *  append the new element at the end of this data structure
    */
    fbe_virtual_drive_configuration_mode_t configuration_mode;
    fbe_update_vd_type_t                   update_vd_type;

    /*! This is the configured default offset for the virtual drive*/
    fbe_lba_t                               vd_default_offset;

    /* When new version releases, new elements append here */
} fbe_database_control_vd_set_configuration_t;

typedef struct fbe_database_control_vd_s {
    /* object id will be set the id of the created Virtual drive if we are creating one */
    /* object id must be the id of an existing Virtual drive if we are updating */
    fbe_object_id_t object_id; 

    fbe_database_transaction_id_t transaction_id;

    fbe_database_control_vd_set_configuration_t configurations;

}fbe_database_control_vd_t;

typedef struct fbe_database_control_update_vd_s
{
    /* object id must be the id of an existing vd since we are updating. */
    fbe_object_id_t                                 object_id;
    fbe_database_transaction_id_t                   transaction_id;
    fbe_virtual_drive_configuration_mode_t          configuration_mode;
    fbe_update_vd_type_t                   update_type;
} fbe_database_control_update_vd_t;

/* RG */
typedef struct fbe_database_control_raid_s {
    fbe_object_id_t                 object_id; /* Raid will be created with this object id */

    fbe_bool_t                      private_raid;
    fbe_raid_group_number_t         raid_id;

    fbe_database_transaction_id_t   transaction_id;
    fbe_class_id_t                  class_id;

    fbe_raid_group_configuration_t  raid_configuration;
    fbe_bool_t                      user_private;
}fbe_database_control_raid_t;

typedef struct fbe_database_control_lookup_raid_s
{
    fbe_raid_group_number_t     raid_id;
    fbe_object_id_t             object_id;
} fbe_database_control_lookup_raid_t;

typedef struct fbe_database_control_update_raid_s {
    /* object id must be the id of an existing RAID since we are updating. */
    fbe_object_id_t                      object_id; 
    fbe_database_transaction_id_t          transaction_id;
    fbe_u64_t                            power_save_idle_time_in_sec;
    fbe_update_raid_type_t update_type;
	fbe_lba_t							new_rg_size;/*used for expansion*/
	fbe_lba_t							new_edge_capacity;/*used for expansion*/
}fbe_database_control_update_raid_t;

/*LUN*/
typedef struct fbe_database_control_lun_s {
    fbe_object_id_t object_id; /* Lun will be created with this object id */

    fbe_lun_number_t   lun_number;
    fbe_assigned_wwid_t         world_wide_name;
    fbe_user_defined_lun_name_t user_defined_name;
    fbe_time_t         bind_time;

    fbe_database_transaction_id_t transaction_id;

    fbe_database_lun_configuration_t lun_set_configuration;
    fbe_bool_t         user_private;
}fbe_database_control_lun_t;

typedef struct fbe_database_control_lookup_lun_s
{
    fbe_lun_number_t   lun_number;
    fbe_object_id_t    object_id;
} fbe_database_control_lookup_lun_t;

typedef struct fbe_database_control_update_lun_s
{
    fbe_database_transaction_id_t transaction_id;
    fbe_object_id_t             object_id;
    fbe_assigned_wwid_t         world_wide_name;
    fbe_u32_t                   attributes;
    fbe_user_defined_lun_name_t user_defined_name;
    fbe_lun_update_type_t       update_type;
} fbe_database_control_update_lun_t;

/* Clone an Object*/
typedef struct fbe_database_control_clone_object_s
{
    fbe_database_transaction_id_t transaction_id;
    fbe_object_id_t     src_object_id;
    fbe_object_id_t     des_object_id;
} fbe_database_control_clone_object_t;

/* EDGE*/
typedef struct fbe_database_control_create_edge_s {
    fbe_object_id_t object_id; /* object-id of the client */

    fbe_database_transaction_id_t transaction_id;

    /*! The following are the other parameters required to create an edge.
    */
    //fbe_block_transport_control_create_edge_t   block_transport_create_edge;
    fbe_object_id_t   server_id;        
    fbe_edge_index_t client_index;     
    fbe_lba_t capacity;
    fbe_lba_t offset;
    fbe_edge_flags_t  ignore_offset;
    fbe_u8_t    serial_num[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1]; /*only used for creating edge between PVD and LDO*/
}fbe_database_control_create_edge_t;

typedef struct fbe_database_control_destroy_edge_s {
    fbe_object_id_t object_id; /* object-id of the client */

    fbe_database_transaction_id_t transaction_id;

    fbe_block_transport_control_destroy_edge_t  block_transport_destroy_edge;
}fbe_database_control_destroy_edge_t;

typedef struct fbe_database_control_online_planned_drive_s
{
    fbe_u32_t port_number; /* IN */
    fbe_u32_t enclosure_number; /* IN */
    fbe_u32_t slot_number; /* IN */
    fbe_object_id_t pdo_object_id;
}fbe_database_control_online_planned_drive_t;

/* payload for FBE_DATABASE_CONTROL_CODE_GET_STATE and FBE_DATABASE_CONTROL_CODE_GET_PEER_STATE */
typedef struct fbe_database_control_get_state_s {
    fbe_database_state_t state;
}fbe_database_control_get_state_t;


/* FBE_DATABASE_CONTROL_CODE_ENUMERATE_SYSTEM_OBJECTS */
typedef struct fbe_database_control_enumerate_system_objects_s {
    fbe_class_id_t class_id; /* IN class id to enumerate */
    fbe_u32_t      number_of_objects;/* INPUT */
    fbe_u32_t      number_of_objects_copied;/* OUTPUT */
}fbe_database_control_enumerate_system_objects_t;


//FBE_DATABASE_CONTROL_CODE_GET_STATS
typedef struct fbe_database_control_get_stats {
    fbe_u32_t num_user_raids;
    fbe_u32_t num_user_luns;
    fbe_u32_t num_system_raids;
    fbe_u32_t num_system_luns;
    fbe_u32_t max_allowed_user_rgs;
    fbe_u32_t max_allowed_user_luns;
    fbe_u32_t max_allowed_luns_per_rg;
    fbe_u32_t   threshold_count;
    fbe_time_t  last_poll_time;
    fbe_u8_t    last_poll_bitmap;
}fbe_database_control_get_stats_t;

/* FBE_DATABASE_CONTROL_CODE_..._POWER_SAVE */
typedef struct fbe_database_power_save_s
{
    fbe_database_transaction_id_t   transaction_id;
    fbe_system_power_saving_info_t  system_power_save_info;
} fbe_database_power_save_t;

/* FBE_DATABASE_CONTROL_CODE_..._ENCRYPTION */
typedef struct fbe_database_encryption_s
{
    fbe_database_transaction_id_t   transaction_id;
    fbe_system_encryption_info_t    system_encryption_info;
} fbe_database_encryption_t;

/* FBE_DATABASE_CONTROL_CODE_UPDATE_SPARE_CONFIG */
typedef enum fbe_database_update_spare_config_type_e {
    FBE_DATABASE_UPDATE_INVALID                 = 0,
    FBE_DATABASE_UPDATE_PERMANENT_SPARE_TIMER,
} fbe_database_update_spare_config_type_t;

typedef struct fbe_database_control_update_system_spare_config_s
{
    fbe_database_transaction_id_t              transaction_id;
    fbe_system_spare_config_info_t             system_spare_info;
    fbe_database_update_spare_config_type_t    update_type;
} fbe_database_control_update_system_spare_config_t;

/* FBE_DATABASE_CONTROL_CODE_..._CAPACITY_LIMIT */
typedef struct fbe_database_capacity_limit_s
{
    fbe_u32_t    cap_limit;
} fbe_database_capacity_limit_t;

typedef enum fbe_homewrecker_get_raw_mirror_status_e{
    FBE_GET_HW_STATUS_INVALID,
    FBE_GET_HW_STATUS_OK ,
    FBE_GET_HW_STATUS_WARNING ,
    FBE_GET_HW_STATUS_ERROR 
}fbe_homewrecker_get_raw_mirror_status_t;


/*Disk operation mask for homewrecker*/
typedef enum fbe_homewrecker_get_fru_disk_mask_e{
    FBE_FRU_DISK_INVALID    = 0x0,
    FBE_FRU_DISK_0  = 0x1,
    FBE_FRU_DISK_1  = 0x2,
    FBE_FRU_DISK_0_1        = 0x3,
    FBE_FRU_DISK_2          = 0x4,
    FBE_FRU_DISK_0_2        = 0x5,
    FBE_FRU_DISK_1_2        = 0x6,
    FBE_FRU_DISK_ALL        = 0x7,
}fbe_homewrecker_get_fru_disk_mask_t;

typedef struct fbe_database_control_get_fru_descriptor_s {
    fbe_homewrecker_fru_descriptor_t            descriptor; /*out*/
    fbe_homewrecker_get_fru_disk_mask_t         disk_mask;  /*in*/
    fbe_homewrecker_get_raw_mirror_status_t     access_status;
}fbe_database_control_get_fru_descriptor_t;


typedef struct fbe_database_control_signature_t_s {
    fbe_u32_t bus; 
    fbe_u32_t enclosure; 
    fbe_u32_t slot;
    fbe_fru_signature_t signature;
}fbe_database_control_signature_t;

typedef struct fbe_database_control_ddmi_data_s {
    fbe_u32_t bus;                          /* Input */
    fbe_u32_t enclosure;                    /* Input */
    fbe_u32_t slot;                         /* Input */
    fbe_u32_t number_of_bytes_requested;    /* Input */
    fbe_u32_t number_of_bytes_returned;     /* Output */
}fbe_database_control_ddmi_data_t;


/*For specifying items to be modified in fru_descriptor*/
typedef enum fbe_database_control_set_fru_descriptor_mask_e{
    FBE_FRU_WWN_SEED            = 0x01,
    FBE_FRU_CHASSIS_REPLACEMENT = 0x02,
    FBE_FRU_SERIAL_NUMBER       = 0x04,
    FBE_FRU_STRUCTURE_VERSION   = 0x08,
}fbe_database_control_set_fru_descriptor_mask_t;

/* set fru descriptor */
typedef struct fbe_database_control_set_fru_descriptor_s {
    fbe_u32_t       wwn_seed;                       /*in*/
    fbe_u32_t       chassis_replacement_movement;   /*in*/
    serial_num_t    system_drive_serial_number[FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER]; /*in*/
    fbe_u32_t       structure_version;              /*in*/
}fbe_database_control_set_fru_descriptor_t;

typedef struct fbe_database_control_update_peer_system_bg_service_s{
    fbe_base_config_control_system_bg_service_t system_bg_service;
}fbe_database_control_update_peer_system_bg_service_t;

typedef struct fbe_database_control_update_ps_stats_s{
    fbe_object_id_t                         object_id;
    fbe_physical_drive_power_saving_stats_t ps_stats;
}fbe_database_control_update_ps_stats_t;

/* job service assume this structure is a simple wrapper around the 
 * encryption key table, be careful don't add anything in here
 * without updating job service handler
 */
typedef struct fbe_database_control_setup_encryption_key_s{
    fbe_encryption_key_table_t key_setup;  /* This has to be the first to avoid conflict with job */
}fbe_database_control_setup_encryption_key_t;

typedef struct fbe_database_control_remove_encryption_keys_s{
    fbe_object_id_t object_id;
}fbe_database_control_remove_encryption_keys_t;

typedef struct fbe_database_control_update_drive_key_s{
    fbe_encryption_key_table_t key_setup;  /* This has to be the first to avoid conflict with job */
    fbe_u32_t mask;
}fbe_database_control_update_drive_key_t;

typedef struct fbe_database_control_setup_kek_s{
    fbe_cmi_sp_id_t sp_id; /* SP ID this command is destined to */
    fbe_object_id_t port_object_id; /* Destination port ID for this setup*/
    fbe_u32_t key_size;
    fbe_u8_t kek[FBE_ENCRYPTION_KEY_SIZE];
    fbe_key_handle_t kek_kek_handle; /* We need the kek kek handle to unwrap this KEK */
    fbe_key_handle_t kek_handle; /* OUTPUT */
}fbe_database_control_setup_kek_t;

typedef struct fbe_database_control_destroy_kek_s{
    fbe_cmi_sp_id_t sp_id;
    fbe_object_id_t port_object_id;
    fbe_key_handle_t kek_handle;
}fbe_database_control_destroy_kek_t;

typedef struct fbe_database_kek_kek_handle_info_s{
    fbe_u32_t id;
    fbe_key_handle_t kek_of_kek_handle;
}fbe_database_kek_kek_handle_info_t;

typedef struct fbe_database_control_setup_kek_kek_s{
    fbe_cmi_sp_id_t sp_id;
    fbe_object_id_t port_object_id;
    fbe_u32_t   key_size;
    fbe_u8_t kek_kek[FBE_ENCRYPTION_KEY_SIZE];
    fbe_key_handle_t kek_kek_handle; /* OUTPUT */
}fbe_database_control_setup_kek_kek_t;

typedef struct fbe_database_control_destroy_kek_kek_s{
    fbe_cmi_sp_id_t sp_id;
    fbe_object_id_t port_object_id;
    fbe_key_handle_t kek_kek_handle;
}fbe_database_control_destroy_kek_kek_t;

typedef struct fbe_database_control_reestablish_kek_kek_s{
    fbe_cmi_sp_id_t sp_id;
    fbe_object_id_t port_object_id;
    fbe_key_handle_t kek_kek_handle;
}fbe_database_control_reestablish_kek_kek_t;

typedef struct fbe_database_control_set_port_encryption_mode_s{ 
    fbe_cmi_sp_id_t sp_id;
    fbe_object_id_t port_object_id;
    fbe_port_encryption_mode_t mode;
} fbe_database_control_port_encryption_mode_t;

typedef struct fbe_database_control_get_pvd_list_for_ext_pool_s
{    
    fbe_u32_t                              pool_id;
    fbe_u32_t                              drive_count;
    fbe_drive_type_t                       drive_type;
    fbe_object_id_t                       *pvd_list;
} fbe_database_control_get_pvd_list_for_ext_pool_t;

typedef struct fbe_database_control_create_ext_pool_s {
    fbe_object_id_t                 object_id;

    fbe_u32_t                       pool_id;
    fbe_u32_t                       drive_count;
    fbe_config_generation_t	        generation_number;

    fbe_database_transaction_id_t   transaction_id;
    fbe_class_id_t                  class_id;
}fbe_database_control_create_ext_pool_t;

typedef struct fbe_database_control_create_ext_pool_lun_s {
    fbe_object_id_t                 object_id;

    fbe_u32_t                       pool_id;
    fbe_u32_t                       lun_id;
    fbe_assigned_wwid_t             world_wide_name;
    fbe_user_defined_lun_name_t     user_defined_name;
    fbe_lba_t                       capacity;
    fbe_config_generation_t	        generation_number;

    fbe_database_transaction_id_t   transaction_id;
    fbe_class_id_t                  class_id;
}fbe_database_control_create_ext_pool_lun_t;

typedef struct fbe_database_control_mark_pvd_swap_pending_s {
    fbe_object_id_t object_id; /* object-id of server (pvd) */
    fbe_spare_swap_command_t swap_command;
    fbe_database_transaction_id_t transaction_id;
}fbe_database_control_mark_pvd_swap_pending_t;

typedef struct fbe_database_control_clear_pvd_swap_pending_s {
    fbe_object_id_t object_id; /* object-id of server (pvd) */
    fbe_spare_swap_command_t swap_command;
    fbe_database_transaction_id_t transaction_id;
}fbe_database_control_clear_pvd_swap_pending_t;


/* TODO: this function should be moved to the limits lib. */
fbe_status_t fbe_database_get_drive_transfer_limits(fbe_u32_t *max_bytes_per_drive_request, fbe_u32_t *max_sg_entries);
fbe_status_t fbe_database_get_lun_export_type(fbe_object_id_t object_id, fbe_bool_t * export_type);
fbe_status_t fbe_database_get_lun_number(fbe_object_id_t object_id, fbe_lun_number_t *lun_number);
fbe_status_t fbe_database_get_rg_number(fbe_object_id_t object_id, fbe_raid_group_number_t *rg_number);

/* FBE_DATABASE_CONTROL_CODE_DESTROY_LUN/RAID */
typedef struct fbe_database_control_destroy_object_s {
    fbe_object_id_t object_id;

    fbe_database_transaction_id_t transaction_id;
}fbe_database_control_destroy_object_t;

typedef enum fbe_object_type_e
{
    FEE_OBJECT_TYPE_INVALID,

    FEE_OBJECT_TYPE_PVD,
    FEE_OBJECT_TYPE_LUN,
    FEE_OBJECT_TYPE_RAID,

    FEE_OBJECT_TYPE_LAST
}fbe_object_type_t;

fbe_status_t fbe_db_init_data_memory(fbe_u32_t total_mem_mb, void * virtual_addr);
fbe_bool_t fbe_database_is_pvd_exists(fbe_u8_t *SN);
fbe_bool_t fbe_database_is_user_pvd_exists(fbe_u8_t *SN, fbe_object_id_t *object_id);
fbe_bool_t fbe_database_is_pvd_exists_by_id(fbe_object_id_t object_id);
fbe_status_t fbe_database_get_default_offset(fbe_class_id_t obj_class_id, fbe_object_id_t object_id, fbe_lba_t *default_lba);
fbe_status_t fbe_database_get_system_pvd(fbe_u32_t pvd_index, fbe_object_id_t *object_id_p);
fbe_status_t fbe_database_get_non_paged_metadata_offset_capacity(fbe_lba_t *offset_p,
                                                                 fbe_lba_t *rg_offset_p,
                                                               fbe_block_count_t *capacity_p);
fbe_bool_t fbe_database_get_metadata_nonpaged_loaded(void);
void fbe_database_set_metadata_nonpaged_loaded(void);
fbe_status_t fbe_database_get_system_encryption_mode(fbe_system_encryption_mode_t * system_encryption_mode);
fbe_status_t fbe_database_get_wwn_seed(fbe_u32_t * wwn_seed);
fbe_status_t fbe_database_get_metadata_lun_id(fbe_object_id_t * object_id);
fbe_status_t fbe_database_get_raw_mirror_config_lun_id(fbe_object_id_t * object_id);
fbe_status_t fbe_database_get_raw_mirror_metadata_lun_id(fbe_object_id_t * object_id);
fbe_status_t fbe_database_get_raw_mirror_rg_id(fbe_object_id_t * object_id);
fbe_status_t fbe_database_get_pvd_opaque_data(fbe_object_id_t object_id, fbe_u8_t *opaque_data_p, fbe_u32_t size);
fbe_status_t fbe_database_get_pvd_pool_id(fbe_object_id_t object_id, fbe_u32_t *pool_id);
fbe_status_t fbe_database_set_pvd_pool_id(fbe_object_id_t object_id, fbe_u32_t pool_id);
fbe_status_t fbe_database_get_last_system_object_id(fbe_object_id_t * object_id);
fbe_status_t fbe_database_get_raw_mirror_lba(fbe_lba_t * lba);
fbe_object_id_t fbe_database_get_system_drive_spare_id (fbe_object_id_t system_pvd_id);
fbe_bool_t fbe_database_is_object_system_pvd(fbe_object_id_t pvd_id);
fbe_bool_t fbe_database_is_object_system_rg(fbe_object_id_t rg_id);
fbe_bool_t fbe_database_is_rg_with_system_drive(fbe_object_id_t rg_object_id);
fbe_bool_t fbe_database_can_encryption_start(fbe_object_id_t rg_id);
fbe_bool_t fbe_database_is_4k_drive_committed(void);
fbe_bool_t fbe_database_is_unmap_committed(void);
fbe_status_t fbe_database_get_ext_pool_configuration_info(fbe_object_id_t pool_object_id, 
                                             fbe_u32_t * pool_id, 
                                             fbe_u32_t * drive_count, 
                                             fbe_object_id_t * pvd_list);
fbe_status_t fbe_database_get_ext_pool_lun_config_info(fbe_object_id_t lun_object_id, 
                                             fbe_u32_t * index, 
                                             fbe_lba_t * capacity, 
                                             fbe_lba_t * offset, 
                                             fbe_object_id_t * pool_object_id);
fbe_status_t fbe_database_get_pvd_capacity(fbe_object_id_t object_id,
                                           fbe_block_count_t *capacity_p);

/*!*******************************************************************
 * @struct fbe_database_ds_object_list_s
 *********************************************************************
 * @brief
 *  This structure is used to get the downstream object list.
 *
 *********************************************************************/
typedef struct fbe_database_ds_object_list_s
{
    /*! This field is used to return the number of downstream object
     *  information.
     */
    fbe_u32_t number_of_downstream_objects;
    fbe_object_id_t downstream_object_list[FBE_MAX_DOWNSTREAM_OBJECTS * 3]; /* mirror, vd, pvd*/
}
fbe_database_ds_object_list_t;
fbe_status_t fbe_database_rg_get_vd_pvd_list(fbe_object_id_t rg_object_id,
                                          fbe_database_ds_object_list_t *ds_list_p);
void fbe_raw_mirror_util_notify_db_drive_up(fbe_u32_t pos);
void fbe_raw_mirror_util_notify_db_drive_failure(fbe_object_id_t id);

/* global info time_treshold related functions*/
fbe_status_t database_get_time_threshold(fbe_system_time_threshold_info_t * time_threshold);

/* If upgrade commmit is done, return true, else return false*/
fbe_bool_t fbe_database_ndu_is_committed(void);

/*** HACK to support revision incompatibility ****/
fbe_bool_t fbe_database_is_peer_running_pre_snake_river_release(void);
fbe_status_t database_notify_metadata_raw_mirror_rebuild(fbe_lba_t start_lba, fbe_block_count_t block_counts);
fbe_status_t database_perform_metadata_raw_mirror_rebuild(fbe_bool_t* out_is_finished);
fbe_status_t database_wait_system_object_ready(fbe_object_id_t system_object_id,fbe_u16_t time_count, fbe_bool_t *is_ready_p);

fbe_status_t fbe_database_get_committed_nonpaged_metadata_size(fbe_class_id_t classid, fbe_u32_t *size);
fbe_status_t fbe_database_get_rg_user_private(fbe_object_id_t object_id, fbe_bool_t *user_private);


fbe_status_t fbe_database_lookup_raid_by_object_id(fbe_object_id_t object_id, fbe_u32_t *raid_group_number);
fbe_status_t fbe_database_lookup_raid_for_internal_rg(fbe_object_id_t object_id, fbe_u32_t *raid_group_number);

fbe_status_t fbe_database_config_table_get_striper(fbe_object_id_t object_id, fbe_object_id_t *striper_id);
fbe_status_t fbe_database_get_object_encryption_mode(fbe_object_id_t object_id,
                                                     fbe_base_config_encryption_mode_t *encryption_mode_p);
/*!*******************************************************************
 * @enum database_class_id_e
 *********************************************************************
 * @brief
 *  This is the set of object class supported by the database service.
 *  fbe_class_id can get shifted when a new class is added.  Database
 *  has it's own class that translated on the fly.
 *
 *********************************************************************/
typedef enum database_class_id_e {
    DATABASE_CLASS_ID_INVALID,
    DATABASE_CLASS_ID_BVD_INTERFACE,
    DATABASE_CLASS_ID_LUN,

    DATABASE_CLASS_ID_RAID_START,
    DATABASE_CLASS_ID_MIRROR,
    DATABASE_CLASS_ID_STRIPER,
    DATABASE_CLASS_ID_PARITY,
    DATABASE_CLASS_ID_RAID_END,

    DATABASE_CLASS_ID_VIRTUAL_DRIVE,
    DATABASE_CLASS_ID_PROVISION_DRIVE,

    DATABASE_CLASS_ID_RAID_GROUP,

    DATABASE_CLASS_ID_EXTENT_POOL,
    DATABASE_CLASS_ID_EXTENT_POOL_LUN,
    DATABASE_CLASS_ID_EXTENT_POOL_METADATA_LUN,

    /*WHEN ADDING new classes, please update function database_control_destroy_all_objects
    so it destroys this class too. Also update database_common_commit_object_from_table to 
    commit the new class to the database*/

    DATABASE_CLASS_ID_LAST
} database_class_id_t;

/*!*******************************************************************
 * @enum database_config_table_type_e
 *********************************************************************
 * @brief
 *  This is the set of tables in the database service.
 *
 *********************************************************************/
typedef enum database_config_table_type_e {
    DATABASE_CONFIG_TABLE_INVALID,
    DATABASE_CONFIG_TABLE_USER,
    DATABASE_CONFIG_TABLE_OBJECT,
    DATABASE_CONFIG_TABLE_EDGE,
    DATABASE_CONFIG_TABLE_GLOBAL_INFO,
    DATABASE_CONFIG_TABLE_SYSTEM_SPARE,
    DATABASE_CONFIG_TABLE_LAST,

    /* key memory was put behind _LAST because it's processed different,
     * we don't want to iterate this table type as others.
     */
    DATABASE_CONFIG_TABLE_KEY_MEMORY        = 16
}database_config_table_type_t;

typedef fbe_u32_t database_table_size_t;

#define FBE_DATABASE_DEFAULT_WAKE_UP_TIMER 1440 /* Default wakeup timer is 24hrs */

/*!*******************************************************************
 * @enum database_config_entry_state_t
 *********************************************************************
 * @brief
 *  This is the set of state of the database service.
 *
 *********************************************************************/
typedef enum database_config_entry_state_e {
    DATABASE_CONFIG_ENTRY_INVALID,
    /* for service tables only*/
    DATABASE_CONFIG_ENTRY_VALID,
    /* for transaction table only */
    DATABASE_CONFIG_ENTRY_CREATE,
    DATABASE_CONFIG_ENTRY_DESTROY,
    DATABASE_CONFIG_ENTRY_MODIFY,
    /* for persist only*/
    DATABASE_CONFIG_ENTRY_CORRUPT,
    DATABASE_CONFIG_ENTRY_UNCOMMITTED /* for global info table */
}database_config_entry_state_t;

/*!*******************************************************************
 * @enum database_global_info_type_t
 *********************************************************************
 * @brief
 *  This lists the type of the database transaction for system global info.
 *
 *********************************************************************/
typedef enum database_global_info_type_e {
    DATABASE_GLOBAL_INFO_TYPE_INVALID,
    DATABASE_GLOBAL_INFO_TYPE_SYSTEM_POWER_SAVE,
    DATABASE_GLOBAL_INFO_TYPE_SYSTEM_SPARE,
    DATABASE_GLOBAL_INFO_TYPE_SYSTEM_GENERATION,
    DATABASE_GLOBAL_INFO_TYPE_SYSTEM_TIME_THRESHOLD,
    DATABASE_GLOBAL_INFO_TYPE_SYSTEM_ENCRYPTION,
    DATABASE_GLOBAL_INFO_TYPE_GLOBAL_PVD_CONFIG,
    DATABASE_GLOBAL_INFO_TYPE_LAST
}database_global_info_type_t;



/*!*******************************************************************
 * @struct database_table_header_t
 *********************************************************************
 * @brief
 *  This is the definition of the common header 
 *
 *********************************************************************/
#pragma pack(1)
typedef struct database_table_header_s {
    union{
        fbe_persist_entry_id_t entry_id; /* must be the first field, so that persist service can update it, when we do write entry */
        fbe_lba_t   lba;  /* this is used by the system_object entries stored in the raw_3way_mirror */
    };
    database_config_entry_state_t state;
    fbe_object_id_t object_id;
    fbe_sep_version_header_t version_header;
}database_table_header_t;
#pragma pack()
/*!*******************************************************************
 * @struct database_object_entry_t
 *********************************************************************
 * @brief
 *  This is the definition of the object_entry
 * 
 * @note
 *  After supporting database versioning, there are some limitations:
 *  1> Only allow appending at the end of the configuration part
 *  2> Don't allow the nested configuration data structures 
 *  3> when a new configuration is introduced, update the funcion:
 *     fbe_database_system_db_header_init 
 *  4> If this data structure was modifed, sep version should be increased
 *  
 * @version
 *  08/05/2012  modified by Gaohp
 *********************************************************************/
#define FBE_DATABASE_OBJECT_ENTRY_OBJECT_SPECIAL_CONFIG_SIZE 256
#pragma pack(1)
typedef struct database_object_entry_s {
    database_table_header_t header;
    database_class_id_t     db_class_id;
    union{                                          
        fbe_provision_drive_configuration_t     pvd_config;
        fbe_database_control_vd_set_configuration_t vd_config;
        fbe_raid_group_configuration_t          rg_config;
        fbe_database_lun_configuration_t        lu_config;
        fbe_extent_pool_configuration_t         ext_pool_config;
        fbe_ext_pool_lun_configuration_t        ext_pool_lun_config;
        fbe_u8_t                                pad[FBE_DATABASE_OBJECT_ENTRY_OBJECT_SPECIAL_CONFIG_SIZE];
    }set_config_union;
    fbe_base_config_configuration_t             base_config; 
}database_object_entry_t;
#pragma pack()

/*!*******************************************************************
 * @struct database_edge_entry_t
 *********************************************************************
 * @brief
 *  This is the definition of the edge_entry
 *
 * @note
 *  After supporting database versioning, there are some limitations:
 *  1> Only allow appending at the end of the configuration part
 *  2> Don't allow the nested data structures 
 *  3> when this data structure changed, may need to update the funcion:
 *     fbe_database_system_db_header_init 
 *  4> If this data structure was modifed, sep version should be increased
 *  
 * @version
 *  08/05/2012  modified by Gaohp
 *********************************************************************/
#pragma pack(1)
typedef struct database_edge_entry_s {
    database_table_header_t header;
    /* used to create the edge between client and server*/
    fbe_object_id_t   server_id;       /* object id of the functional transport server */
    fbe_edge_index_t client_index;     /* index of the client edge (second edge of mirror object for example) */
    fbe_lba_t capacity;                /* This is the capacity in blocks of the extent exported by the server on this edge. */
    fbe_lba_t offset;                  /* This is the block offset of this extent from the start of the device. */
    fbe_edge_flags_t   ignore_offset;        /* Used for user vd's in first 4 drives */
}database_edge_entry_t;
#pragma pack()

/*!*******************************************************************
 * @struct database_user_entry_t
 *********************************************************************
 * @brief
 *  This is the definition of the user_entry, which stores all
 *  the data user ask us to store.  This data is persisted in
 *  FBE_PERSIST_SECTOR_TYPE_SEP_ADMIN_CONVERSION
 *
 * @note
 *  After supporting database versioning, there are some limitations:
 *  1> Only allow appending at the end of the configuration part
 *  2> Don't allow the nested configuration data structures 
 *  3> when a new configuration is introduced, update the funcion:
 *     fbe_database_system_db_header_init 
 *  4> If this data structure was modifed, sep version should be increased
 *  
 * @version
 *  08/05/2012  modified by Gaohp
 *********************************************************************/
#pragma pack(1)
typedef struct database_user_entry_s {
    database_table_header_t header;
    database_class_id_t     db_class_id;
    union{
        database_pvd_user_data_t pvd_user_data;
        database_rg_user_data_t  rg_user_data;
        database_lu_user_data_t  lu_user_data;
        database_ext_pool_user_data_t ext_pool_user_data;
        database_ext_pool_lun_user_data_t ext_pool_lun_user_data;
    }user_data_union;
}database_user_entry_t;
#pragma pack()

/*!*******************************************************************
 * @struct database_global_info_entry_t
 *********************************************************************
 * @brief
 *  This is the definition of the _system_setting entry, which stores
 *  system stuff, e.g power saving.  This data is persisted in
 *  FBE_PERSIST_SECTOR_TYPE_SYSTEM_GLOBAL_DATA
 *
 * @note
 *  After supporting database versioning, there are some limitations:
 *  1> Only allow appending at the end of the configuration part
 *  2> Don't allow the nested configuration data structures 
 *  3> when a new configuration is introduced, update the funcion:
 *     fbe_database_system_db_header_init 
 *  4> If this data structure was modifed, sep version should be increased
 *  
 * @version
 *  08/05/2012  modified by Gaohp
 *********************************************************************/
#pragma pack(1)
typedef struct database_global_info_entry_s {
    database_table_header_t header;
    database_global_info_type_t type;
    union{
        fbe_system_power_saving_info_t power_saving_info;
        fbe_system_spare_config_info_t spare_info;
        fbe_system_generation_info_t   generation_info;
        fbe_system_time_threshold_info_t time_threshold_info;
        fbe_system_encryption_info_t   encryption_info;
        fbe_global_pvd_config_t          pvd_config;
    }info_union;
}database_global_info_entry_t;
#pragma pack()

/*!*******************************************************************
 * @struct database_system_spare_entry_t
 *********************************************************************
 * @brief
 *  This is the definition of the _system_spare entry, which stores
 *  spare look up, This data is persisted in raw_3way_mirror
 *
 *********************************************************************/
#pragma pack(1)
/* TODO: This is a place holder.  the content of this table is to be changed to meet the spare need. */
typedef struct database_system_spare_entry_s {
    database_table_header_t header;
    fbe_object_id_t spare_drive_id;
}database_system_spare_entry_t;
#pragma pack()


typedef struct database_table_s {
    database_config_table_type_t table_type;
    database_table_size_t        table_size;
    fbe_u32_t                    alloc_size;
    fbe_u64_t                    peer_table_start_address;
    union{
        database_object_entry_t *object_entry_ptr;
        database_edge_entry_t   *edge_entry_ptr;
        database_user_entry_t   *user_entry_ptr;
        database_global_info_entry_t *global_info_ptr;
        database_system_spare_entry_t *system_spare_entry_ptr;
    }table_content;
    fbe_spinlock_t              table_lock;
}database_table_t;


/*!*******************************************************************
 * @struct database_transaction_state_t
 *********************************************************************
 * @brief
 *  Defines the transaction states.
 *
 *********************************************************************/
typedef enum database_transaction_state_e {
    DATABASE_TRANSACTION_STATE_INACTIVE,
    DATABASE_TRANSACTION_STATE_ACTIVE,
    DATABASE_TRANSACTION_STATE_COMMIT,
    DATABASE_TRANSACTION_STATE_ROLLBACK,
}database_transaction_state_t;

typedef enum {
    /* R10 16 Drive RG creation uses max user entries, it does not have any edge entry, so total number of entries is less than persist limits. */
    DATABASE_MAX_RG_CREATE_USER_ENTRY = 9,/* can have up to 9 RG in one transaction, when create R10, 1 striper, 8 mirror_under_striper*/
    DATABASE_MAX_RG_CREATE_OBJECT_ENTRY = FBE_DATABASE_TRANSACTION_MAX_CREATE_OBJECTS_PER_JOB + 16 + 16, /* 16*2 pvd to modifty */
    DATABASE_MAX_RG_CREATE_EDGE_ENTRY = 40, /* Max edges in a transaction 8 rg-rg, 16 rg-vd, 16 vd-pvd*/

    /* To support mapped raid there can be this many pvds in a pool.
     */
#if defined(SIMMODE_ENV)
    DATABASE_MAPPED_RAID_MAX_POOL_ENTRY = 50,
#else
    /*! @note Mapped raid is not fulling supported on hardware. */
    DATABASE_MAPPED_RAID_MAX_POOL_ENTRY = FBE_DATABASE_TRANSACTION_MAX_CREATE_OBJECTS_PER_JOB,
#endif

    /* PVD creation uses max user entries, it does not have any edge entry, so total number of entries is less than persist limits. */
                                                    /* same as what is allowed per job                  maximum pvds in extent pool */
    DATABASE_MAX_PVD_CREATE_USER_ENTRY = FBE_MAX(FBE_DATABASE_TRANSACTION_MAX_CREATE_OBJECTS_PER_JOB, DATABASE_MAPPED_RAID_MAX_POOL_ENTRY),
                                                    /* same as what is allowed per job                  maximum pvds in extent pool*/
    DATABASE_MAX_PVD_CREATE_OBJECT_ENTRY = FBE_MAX(FBE_DATABASE_TRANSACTION_MAX_CREATE_OBJECTS_PER_JOB, DATABASE_MAPPED_RAID_MAX_POOL_ENTRY),
    DATABASE_MAX_PVD_CREATE_EDGE_ENTRY = 0, /* Max edges in a PVD create transaction is 0 */

    /* NOTE: the total of entries of a transaction can not exceed max persist service transaction limits. If so, you have a problem dude..
     * Use the max values based on the object (PVD, RG..) listed above.
     * 
     * Two scenarios uses the max entries, but does not use max entries of all 4 tables(user, object, edge, and global info).
     * 1.	PVD creation uses max user entries, it does not have any edge entry, so total number of entries is less than persist limits.
     * 2.	R10 16 drive RG creation uses max edge entries and object entries, but not all the user entries. So its total is less than persist limits.
     */
    DATABASE_TRANSACTION_MAX_USER_ENTRY = FBE_MAX(DATABASE_MAX_RG_CREATE_USER_ENTRY, DATABASE_MAX_PVD_CREATE_USER_ENTRY),
    DATABASE_TRANSACTION_MAX_OBJECTS = FBE_MAX(DATABASE_MAX_RG_CREATE_OBJECT_ENTRY, DATABASE_MAX_PVD_CREATE_OBJECT_ENTRY),
    DATABASE_TRANSACTION_MAX_EDGES = FBE_MAX(DATABASE_MAX_RG_CREATE_EDGE_ENTRY, DATABASE_MAX_PVD_CREATE_EDGE_ENTRY),
    DATABASE_TRANSACTION_MAX_GLOBAL_INFO = 1, /* There should be only one system transaction can happen at a time */


    DATABASE_MAX_SUPPORTED_PHYSICAL_DRIVES = 1024, /* this should be derived from the hardware type */
    DATABASE_MAX_EDGE_PER_OBJECT = 16,
    DATABASE_SYSTEM_DB_BLOCK_SIZE = 520,
    DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE = FBE_RAW_MIRROR_DATA_SIZE,
    DATABASE_SYSTEM_DB_RAW_BLOCK_DATA_SIZE = 512,
    DATABASE_HOMEWRECKER_DB_BLOCK_SIZE = 520,
    DATABASE_HOMEWRECKER_DB_BLOCK_DATA_SIZE = FBE_RAW_MIRROR_DATA_SIZE,
    DATABASE_HOMEWRECKER_DB_RAW_BLOCK_DATA_SIZE = 512
}database_limits_t;

/*!*******************************************************************
 * @struct database_transaction_t
 *********************************************************************
 * @brief
 *  This is describes the transaction.
 *
 *********************************************************************/
typedef struct database_transaction_s {
    /* the following are used in a transaction */
    fbe_database_transaction_id_t        transaction_id;
    fbe_database_transaction_type_t      transaction_type;
    fbe_u64_t   job_number;  /*job number of the job that starts this database transaction*/

    database_transaction_state_t state;
    /* Holds work items in one transaction */
    database_user_entry_t user_entries[DATABASE_TRANSACTION_MAX_USER_ENTRY];
    database_object_entry_t object_entries[DATABASE_TRANSACTION_MAX_OBJECTS];
    database_edge_entry_t edge_entries[DATABASE_TRANSACTION_MAX_EDGES];
    /* Holds system global info entries */
    database_global_info_entry_t global_info_entries[DATABASE_TRANSACTION_MAX_GLOBAL_INFO];

}database_transaction_t;


typedef struct fbe_database_get_tables_s { 
        database_object_entry_t object_entry;
        database_edge_entry_t   edge_entry[DATABASE_MAX_EDGE_PER_OBJECT];
        database_user_entry_t   user_entry;
}fbe_database_get_tables_t;

/* payload for FBE_DATABASE_CONTROL_CODE_GET_TABLES */
typedef struct fbe_database_control_get_tables_s {
    fbe_database_get_tables_t tables;
    fbe_object_id_t object_id;
}fbe_database_control_get_tables_t;


/*The control command operation is used to read/write the 
  system db entries in the raw-3way-mirror. It should only
  be used in test*/
/*payload for FBE_DATABASE_CONTROL_CODE_SYSTEM_DB_OP_CMD*/
typedef enum{
    FBE_DATABASE_CONTROL_SYSTEM_DB_OP_INVALID = 0,
    FBE_DATABASE_CONTROL_SYSTEM_DB_READ_OBJECT,
    FBE_DATABASE_CONTROL_SYSTEM_DB_READ_USER,
    FBE_DATABASE_CONTROL_SYSTEM_DB_READ_EDGE,
    FBE_DATABASE_CONTROL_SYSTEM_DB_WRITE_OBJECT,
    FBE_DATABASE_CONTROL_SYSTEM_DB_WRITE_USER,
    FBE_DATABASE_CONTROL_SYSTEM_DB_WRITE_EDGE,
    FBE_DATABASE_CONTROL_SYSTEM_DB_VERIFY_BOOT,
    FBE_DATABASE_CONTROL_SYSTEM_DB_GET_RAW_MIRROR_ERR_REPORT
} fbe_database_control_system_db_cmd_t;

#define MAX_SYSTEM_DB_OP_CMD_BUFFER_SIZE      512

typedef enum{
    FBE_DATABASE_SYSTEM_DB_PERSIST_CREATE,
    FBE_DATABASE_SYSTEM_DB_PERSIST_UPDATE,
    FBE_DATABASE_SYSTEM_DB_PERSIST_DELETE
} fbe_database_system_db_persist_type_t;

/*it is used to return database raw-mirror error report to caller or 
  test to check the error and rebuild state*/
typedef struct fbe_database_control_raw_mirror_err_report{

    /*detected and flushed error count*/
    fbe_u64_t             error_count;
    fbe_u64_t             flush_error_count;
    fbe_u64_t             failed_flush_count;

    /*bitmap identify which disk is problematic (flush failed)*/
    fbe_u32_t             degraded_disk_bitmap;

    /*bitmap identify which disk is down (failed) now*/
    fbe_u32_t             down_disk_bitmap;

    /*bitmap identify which disk need rebuild*/
    fbe_u32_t             nr_disk_bitmap;
    fbe_u32_t             raw_mirror_recoverable;

    /*background rebuild checkpoint*/
    fbe_u64_t             rebuild_checkpoint;
} fbe_database_control_raw_mirror_err_report_t;

typedef struct fbe_database_control_system_db_op_s{
    fbe_database_control_system_db_cmd_t cmd;
    fbe_database_system_db_persist_type_t persist_type;
    fbe_object_id_t object_id;
    fbe_edge_index_t edge_index;
    fbe_u8_t cmd_data[MAX_SYSTEM_DB_OP_CMD_BUFFER_SIZE];
} fbe_database_control_system_db_op_t;

/*payload for FFBE_DATABASE_CONTROL_CODE_VERSIONING */
typedef enum {
    FBE_DATABASE_CONTROL_CHECK_VERSION_HEADER_IN_ENTRY, /* Used to check the version header of entry*/
    FBE_DATABASE_CONTROL_CHECK_UNKNOWN_CMI_MSG_HANDLING, /* Used to check the unknown cmi msg handling*/
    /******add more codes here*******/

    FBE_DATABASE_CONTROL_VERSIONING_OP_MAX, /* Number of versioning codes */
} fbe_database_control_versioning_op_code_t;

typedef enum {
    FBE_DATABASE_SERVICE_MODE_REASON_INVALID,
    FBE_DATABASE_SERVICE_MODE_REASON_INTERNAL_OBJS,
    FBE_DATABASE_SERVICE_MODE_REASON_DB_DRIVES_OR_CHASSIS_MISMATCH, /*leave it here for later delete*/
    FBE_DATABASE_SERVICE_MODE_REASON_HOMEWRECKER_CHASSIS_MISMATCHED,
    FBE_DATABASE_SERVICE_MODE_REASON_HOMEWRECKER_SYSTEM_DISK_DISORDER,
    FBE_DATABASE_SERVICE_MODE_REASON_HOMEWRECKER_INTEGRITY_BROKEN,  /*Three or more system drives are invalid*/
    FBE_DATABASE_SERVICE_MODE_REASON_HOMEWRECKER_DOUBLE_INVALID_DRIVE_WITH_DRIVE_IN_USER_SLOT,
    FBE_DATABASE_SERVICE_MODE_REASON_HOMEWRECKER_OPERATION_ON_WWNSEED_CHAOS,
    FBE_DATABASE_SERVICE_MODE_REASON_SYSTEM_DB_HEADER_IO_ERROR,
    FBE_DATABASE_SERVICE_MODE_REASON_SYSTEM_DB_HEADER_TOO_LARGE,
    FBE_DATABASE_SERVICE_MODE_REASON_SYSTEM_DB_HEADER_DATA_CORRUPT,
    FBE_DATABASE_SERVICE_MODE_REASON_INVALID_MEMORY_CONF_CHECK,
    FBE_DATABASE_SERVICE_MODE_REASON_PROBLEMATIC_DATABASE_VERSION,
    FBE_DATABASE_SERVICE_MODE_REASON_SMALL_SYSTEM_DRIVE,
    FBE_DATABASE_SERVICE_MODE_REASON_NOT_ALL_DRIVE_SET_ICA_FLAGS,
    FBE_DATABASE_SERVICE_MODE_REASON_DB_VALIDATION_FAILED,
    /*********************************************************** 
        *NOTE: If you add new service mode code here, remember to update:
        * 1. funcion: DisplayDegradeModeReason,fbe_database_set_degraded_mode_reason_in_SPID
        * 2. SPID_DEGRADED_REASON 
        **********************************************************/
    FBE_DATABASE_SERVICE_MODE_REASON_LAST,

} fbe_database_service_mode_reason_t;

#define MAX_DB_SERVICE_MODE_REASON_STR 256
typedef struct {
    fbe_database_service_mode_reason_t reason;
    fbe_char_t reason_str[MAX_DB_SERVICE_MODE_REASON_STR];
} fbe_database_control_get_service_mode_reason_t;

void fbe_database_get_state(fbe_database_state_t *state);
fbe_status_t fbe_database_enable_bg_operations(fbe_base_config_control_system_bg_service_flags_t bgs_flags);

/* FBE_DATABASE_CONTROL_CODE_..._TIME_THRESHOLD */
// it is used for pvd garbage collection
typedef struct fbe_database_time_threshold_s
{
    fbe_database_transaction_id_t   transaction_id;
    fbe_system_time_threshold_info_t  system_time_threshold_info;
} fbe_database_time_threshold_t;

typedef struct fbe_database_control_lookup_lun_by_wwn_s
{
    fbe_assigned_wwid_t   lun_wwid;
    fbe_object_id_t    object_id;/*OUT*/
} fbe_database_control_lookup_lun_by_wwn_t;

/* It is used for returning the commit state.
 * If (version == commit_level)
 *      has committed;
 */
typedef struct fbe_compatibility_mode_s
{
    fbe_u32_t   datasize;   /* The size of this data structure */
    fbe_u16_t   version;    /* The statically compiled version of Sep package */
    fbe_u16_t   commit_level;   /* The committed version of Sep package */
    K10_DRIVER_COMPATIBILITY_MODE mode;   /* The compatibility mode */
}fbe_compatibility_mode_t;


/* This enum is compatible with K10_NDU_ADMIN_OPC_MANAGE
   When K10_NDU_ADMIN_OPC_MANAGE changed, update here.
 */
typedef enum fbe_ndu_admin_opc_manage_e
{
    FBE_NDU_ADMIN_OPC_MANAGE_MIN = 1,
    FBE_NDU_ADMIN_OPC_NOOP = FBE_NDU_ADMIN_OPC_MANAGE_MIN,
    FBE_NDU_ADMIN_OPC_INSTALL,
    FBE_NDU_ADMIN_OPC_COMMIT,
    FBE_NDU_ADMIN_OPC_REVERT,
    FBE_NDU_ADMIN_OPC_UNINSTALL,
    FBE_NDU_ADMIN_OPC_CLEAN,
    FBE_NDU_ADMIN_OPC_POLL,
    FBE_NDU_ADMIN_OPC_SYNCHRONIZE_TOC,
    FBE_NDU_ADMIN_OPC_QUIESCE_PEER,
    FBE_NDU_ADMIN_OPC_CLEAR_STATUS,
    FBE_NDU_ADMIN_OPC_UNINSTALL_INACTIVE,
    FBE_NDU_ADMIN_OPC_SET_PROPERTIES,
    FBE_NDU_ADMIN_OPC_COMPLETE_ASYNC_OP,
    FBE_NDU_ADMIN_OPC_MANAGE_MAX = FBE_NDU_ADMIN_OPC_COMPLETE_ASYNC_OP
}fbe_ndu_admin_opc_manage_t;

/* This enum is compatible with K10_NDU_PROGRESS
   When K10_NDU_NDU_PROGRESS changed, update here.
 */
typedef enum fbe_ndu_progress_e
{
    FBE_NDU_PROGRESS_MIN = 1,
    FBE_NDU_PROGRESS_SETTING_UP = FBE_NDU_PROGRESS_MIN,
    FBE_NDU_PROGRESS_STORING_PACKAGES,
    FBE_NDU_PROGRESS_INSTALLING_SLAVE,
    FBE_NDU_PROGRESS_REBOOTING_SLAVE,
    FBE_NDU_PROGRESS_INSTALLING_MASTER,
    FBE_NDU_PROGRESS_REBOOTING_MASTER,
    FBE_NDU_PROGRESS_POST_CHECK_COMPLETE,
    FBE_NDU_PROGRESS_RECOVERING_SLAVE,
    FBE_NDU_PROGRESS_RECOVERING_MASTER,
    FBE_NDU_PROGRESS_DISABLING_CACHE,
    FBE_NDU_PROGRESS_RESTARTING_SLAVE,
    FBE_NDU_PROGRESS_RESTARTING_MASTER,
    FBE_NDU_PROGRESS_RUNNING_CHECK_SCRIPTS,
    FBE_NDU_PROGRESS_QUIESCING_SLAVE,
    FBE_NDU_PROGRESS_DEACTIVATING_SLAVE,
    FBE_NDU_PROGRESS_ACTIVATING_SLAVE,
    FBE_NDU_PROGRESS_UNINSTALLING_SLAVE,
    FBE_NDU_PROGRESS_QUIESCING_MASTER,
    FBE_NDU_PROGRESS_DEACTIVATING_MASTER,
    FBE_NDU_PROGRESS_ACTIVATING_MASTER,
    FBE_NDU_PROGRESS_UNINSTALLING_MASTER,
    FBE_NDU_PROGRESS_DMP_DELAY,
    FBE_NDU_PROGRESS_RESTORING_CACHE,
    FBE_NDU_PROGRESS_MAX = FBE_NDU_PROGRESS_RESTORING_CACHE
}fbe_ndu_progress_t;

typedef struct fbe_set_ndu_state_s
{
    fbe_u16_t   primarySP_compat_level;
    fbe_u16_t   targetBundle_compat_level;
    fbe_ndu_admin_opc_manage_t ndu_OP;
    fbe_ndu_progress_t         ndu_progress;
    fbe_bool_t  is_SPA_primarySP;
    fbe_bool_t  is_operation_complete;
}fbe_set_ndu_state_t;

#pragma pack(1)
typedef struct fbe_version_header_s
{
    fbe_u32_t   size;  /* the valid size */
    fbe_u32_t   padded_reserve_space[3]; /* reserve the space */
}fbe_version_header_t;
#pragma pack()

/*! @enum   fbe_database_debug_flags_t
 *  
 *  @brief  Debug flags that allow run-time debug functions.
 *
 *  @note   Typically these flags should only be set for unit or
 *          intergration testing since they could have SEVERE 
 *          side effects.
 */
typedef enum fbe_database_debug_flags_e
{
    FBE_DATABASE_DEBUG_FLAG_NONE                            = 0x00000000,

    /*! Validate database when peer lost.
     */
    FBE_DATABASE_DEBUG_FLAG_VALIDATE_DATABASE_ON_PEER_LOSS  = 0x00000001,

    /*! Validate database after on sp collect
     */
    FBE_DATABASE_DEBUG_FLAG_VALIDATE_DATABASE_ON_SP_COLLECT = 0x00000002,

    /*! Validate database after destroy.
     */
    FBE_DATABASE_DEBUG_FLAG_VALIDATE_DATABASE_ON_DESTROY    = 0x00000004,


    FBE_DATABASE_DEBUG_FLAG_LAST                            = 0x00000008,

} fbe_database_debug_flags_t;
#define FBE_DATABASE_IF_DEBUG_DEFAULT_FLAGS                 (FBE_DATABASE_DEBUG_FLAG_NONE)

/*! @enum   fbe_database_validate_request_type
 *  
 *  @brief  The type of request that is asking for the validation
 *
 */
typedef enum fbe_database_validate_request_type_e
{
    FBE_DATABASE_VALIDATE_REQUEST_TYPE_INVALID,

    /*! The user has requested the validation
     */
    FBE_DATABASE_VALIDATE_REQUEST_TYPE_USER,

    /*! The validation follows a peer lost
     */
    FBE_DATABASE_VALIDATE_REQUEST_TYPE_PEER_LOST,

    /*! The validation follows spcollect
     */
    FBE_DATABASE_VALIDATE_REQUEST_TYPE_SP_COLLECT,

    /*! The validation follows a destroy request
     */
    FBE_DATABASE_VALIDATE_REQUEST_TYPE_DESTROY,

    FBE_DATABASE_VALIDATE_REQUEST_TYPE_LAST,

} fbe_database_validate_request_type_t;

/*! @enum   fbe_database_validate_not_allowed_reason_t
 *  
 *  @brief  Determines why the database validation is not
 *          allowed.
 *
 */
typedef enum fbe_database_validate_not_allowed_reason_e
{
    FBE_DATABASE_VALIDATE_NOT_ALLOWED_REASON_NONE,

    /*! Unexpected error
     */
    FBE_DATABASE_VALIDATE_NOT_ALLOWED_REASON_UNEXPECTED_ERROR,

    /*! Validation is not allowed for external build
     */
    FBE_DATABASE_VALIDATE_NOT_ALLOWED_REASON_EXTERNAL_BUILD,

    /*! Request type (via debug flags) is not enabled
     */
    FBE_DATABASE_VALIDATE_NOT_ALLOWED_REASON_NOT_ENABLED,

    /*! Database is not in the ready state
     */
    FBE_DATABASE_VALIDATE_NOT_ALLOWED_REASON_NOT_READY,

    /*! The SP is in degraded mode
     */
    FBE_DATABASE_VALIDATE_NOT_ALLOWED_REASON_DEGRADED,

    FBE_DATABASE_VALIDATE_NOT_ALLOWED_REASON_LAST,

} fbe_database_validate_not_allowed_reason_t;

/*! @enum   fbe_database_validate_failure_action
 *  
 *  @brief  Determines what action to take if eh validation
 *          fails.
 *
 */
typedef enum fbe_database_validate_failure_action_e
{
    FBE_DATABASE_VALIDATE_FAILURE_ACTION_NONE,

    /*! Failure action is error trace
     */
    FBE_DATABASE_VALIDATE_FAILURE_ACTION_ERROR_TRACE,

    /*! @todo Failure action is correct the bad entries 
     *        Currently not supported
     */
    FBE_DATABASE_VALIDATE_FAILURE_ACTION_CORRECT_ENTRY,

    /*! Failure action is to enter degraded mode
     */
    FBE_DATABASE_VALIDATE_FAILURE_ACTION_ENTER_DEGRADED_MODE,

    /*! Failure action is to enter degraded mode then PANIC
     */
    FBE_DATABASE_VALIDATE_FAILURE_ACTION_PANIC_SP,

    FBE_DATABASE_VALIDATE_FAILURE_ACTION_LAST,

} fbe_database_validate_failure_action_t;

/*FBE_DATABASE_CONTROL_CODE_GET_ALL_LUNS*/

/*this API was created mostly for Admin to be able to do as much discovery as possible in one shot 
and reduce traffic between user and kerenl to minimum*/
typedef struct fbe_database_control_get_all_luns_s {
    fbe_u32_t      number_of_luns_requested;/* INPUT */
    fbe_u32_t      number_of_luns_returned;/* OUTPUT */
}fbe_database_control_get_all_luns_t;

/*FBE_DATABASE_CONTROL_CODE_GET_EMV*/
/*FBE_DATABASE_CONTROL_CODE_SET_EMV*/
#pragma pack(1)
typedef struct fbe_database_emv_s{
    /*shared EMV information which is stored in db raw-3-way mirror region
    * lower 32 bits store shared expected memory
    * higher 32 bits store conversion target memory which is used for in-family conversion*/
    fbe_u64_t   shared_expected_memory_info;
}fbe_database_emv_t;
#pragma pack()

fbe_status_t  fbe_database_get_shared_emv_info(fbe_u64_t *shared_emv_info);
fbe_status_t  fbe_database_set_shared_emv_info(fbe_u64_t shared_emv_info);

/*The structure to contain the system object operation flags */
#define MAX_SYSTEM_OBJECT_NUM_FOR_RECREATION    128
#pragma pack(1)
typedef struct fbe_database_system_object_recreate_flags_s {
    fbe_u32_t   valid_num;  /* the number of valid element in the flags array */
    fbe_u8_t    system_object_op[MAX_SYSTEM_OBJECT_NUM_FOR_RECREATION]; /*the flags array */
}fbe_database_system_object_recreate_flags_t;
#pragma pack()

#define FBE_SYSTEM_OBJECT_OP_RECREATE   0x01    /* This system object need to be recreated */
#define FBE_SYSTEM_OBJECT_OP_NDB_LUN    0x02    /* When recreating system LUN, don't zero the data */
#define FBE_SYSTEM_OBJECT_OP_PVD_ZERO   0x04    /* When recreating system PVD, zero the PVD data */
#define FBE_SYSTEM_OBJECT_OP_PVD_NR     0x08    /* When recreating system PVD, notify Raids to be rebuilt */

typedef struct fbe_database_raid_group_info_s
{
    fbe_object_id_t             rg_object_id;
    fbe_raid_group_number_t     rg_number;
    fbe_raid_group_get_info_t   rg_info;
    fbe_raid_group_get_power_saving_info_t    power_saving_policy;
    fbe_block_transport_control_get_max_unused_extent_size_t extent_size; /* max free contigous capacity. */
    fbe_lba_t                   free_non_contigous_capacity;
    fbe_u32_t                   lun_count;
    fbe_u32_t                   pvd_count;
    fbe_object_id_t            	lun_list[FBE_MAX_UPSTREAM_OBJECTS];
    fbe_object_id_t             pvd_list[FBE_MAX_DOWNSTREAM_OBJECTS];
    fbe_lifecycle_state_t		lifecycle_state;
    fbe_bool_t                  power_save_capable;
} fbe_database_raid_group_info_t;

/*FBE_DATABASE_CONTROL_CODE_GET_ALL_RAID_GROUPS. This usurper code will also contain a buffer of size 
  typedef struct fbe_database_raid_group_info_t multiplied by the number of RGs the user discoverred*/
/*this API was created mostly for Admin to be able to do as much discovery as possible in one shot 
  and reduce traffic between user and kerenl to minimum*/
typedef struct fbe_database_control_get_all_rgs_s {
    fbe_u32_t      number_of_rgs_requested;/* INPUT */
    fbe_u32_t      number_of_rgs_returned;/* OUTPUT */
}fbe_database_control_get_all_rgs_t;

#define MAX_RG_COUNT_ON_TOP_OF_PVD 7
typedef struct fbe_database_pvd_info_s
{
    fbe_object_id_t             pvd_object_id;
    fbe_provision_drive_info_t  pvd_info;
    fbe_u16_t                   zeroing_percentage;
    fbe_u32_t                   pool_id;
    fbe_base_config_object_power_save_info_t power_save_info;
    fbe_u32_t                   rg_count;
    fbe_raid_group_number_t		rg_number_list[MAX_RG_COUNT_ON_TOP_OF_PVD];
    fbe_object_id_t     		rg_list[MAX_RG_COUNT_ON_TOP_OF_PVD];
    fbe_lifecycle_state_t 		lifecycle_state;
} fbe_database_pvd_info_t; 

/*FBE_DATABASE_CONTROL_CODE_GET_ALL_PVDS. This usurper code will also contain a buffer of size 
  typedef struct fbe_database_pvd_info_t multiplied by the number of PVDs the user discoverred*/
/*this API was created mostly for Admin to be able to do as much discovery as possible in one shot 
  and reduce traffic between user and kerenl to minimum*/
typedef struct fbe_database_control_get_all_pvds_s {
    fbe_u32_t      number_of_pvds_requested;/* INPUT */
    fbe_u32_t      number_of_pvds_returned;/* OUTPUT */
}fbe_database_control_get_all_pvds_t;

/*the following defines were taken from dba_export_types.h but we don't want to include it since it will
have lot's of Flare junk we don't want. Yes, it puts us in a bit of risk for getting out of sync but this file has not been chaning for years.*/
#define FBE_LU_SP_ONLY  0x00200000
#define FBE_LU_SYSTEM     0x01000000
#define FBE_LU_USER     0x00800000
#define FBE_LU_PRIVATE   0x04000000
#define FBE_LU_VCX0      0x10000000
#define FBE_LU_VCX1      0x20000000
#define FBE_LU_VCX2      0x40000000
#define FBE_LU_VCX3      0x80000000

#define FBE_LU_ATTR_VCX_0   FBE_LU_VCX0
#define FBE_LU_ATTR_VCX_1   FBE_LU_VCX1
#define FBE_LU_ATTR_VCX_2   (FBE_LU_VCX0 | FBE_LU_VCX1)
#define FBE_LU_ATTR_VCX_3   FBE_LU_VCX2
#define FBE_LU_ATTR_VCX_4   (FBE_LU_VCX0 | FBE_LU_VCX2)
#define FBE_LU_ATTR_VCX_5   (FBE_LU_VCX1 | FBE_LU_VCX2)
//HELGA HACK -- Allow c4admintool to see private LUN 8226
#define FBE_LU_ATTR_VCX_6   0x70000000 
#define FBE_LU_ATTRIBUTES_VCX_MASK         	0xF0000000
#define FBE_LUN_CHARACTERISTIC_CELERRA 		0x40 /*based on FLARE_LUN_CHARACTERISTIC_CELERRA*/
#define DATABASE_SYSTEM_DB_HEADER_MAGIC_NUMBER     20111010

fbe_status_t sep_set_degraded_mode_in_nvram(void);
/*!*******************************************************************
 * @struct database_system_db_header_t
 *********************************************************************
 * @brief
 *  This is the definition of the system db header
 * 
 *  **********************WARNING !!!**********************
 *  * NOTE !!!!!  All new entries must go to the end of this strucure !!!!
 *  **********************WARNING !!!**********************
 *
 *********************************************************************/
#pragma pack(1)
typedef struct database_system_db_header_s {
    fbe_u64_t magic_number;            /* default value: DATABASE_SYSTEM_DB_HEADER_MAGIC_NUMBER */
    fbe_sep_version_header_t    version_header;     /*used to record the size of system db header*/
    fbe_u64_t persisted_sep_version;                /* revision number: 1 */
    
    /* persist the size for object entry */
    fbe_u32_t bvd_interface_object_entry_size;
    fbe_u32_t pvd_object_entry_size;
    fbe_u32_t vd_object_entry_size;
    fbe_u32_t rg_object_entry_size;
    fbe_u32_t lun_object_entry_size;

    /* persist the size for edge entry */
    fbe_u32_t edge_entry_size;

    /* persist the size for user entry */
    fbe_u32_t pvd_user_entry_size;
    fbe_u32_t rg_user_entry_size;
    fbe_u32_t lun_user_entry_size;

    /* persist the size for global info entry */
    fbe_u32_t power_save_info_global_info_entry_size;
    fbe_u32_t spare_info_global_info_entry_size;
    fbe_u32_t generation_info_global_info_entry_size;
    fbe_u32_t time_threshold_info_global_info_entry_size;

    /* persist the nonpaged metadata size */
    fbe_u32_t pvd_nonpaged_metadata_size;
    fbe_u32_t lun_nonpaged_metadata_size;
    fbe_u32_t raid_nonpaged_metadata_size;

    /* persist the size for global info new entry */
    fbe_u32_t encryption_info_global_info_entry_size;
    fbe_u32_t pvd_config_global_info_entry_size;

    fbe_u32_t ext_pool_object_entry_size;
    fbe_u32_t ext_pool_lun_object_entry_size;
    fbe_u32_t ext_pool_user_entry_size;
    fbe_u32_t ext_pool_lun_user_entry_size;
    fbe_u32_t ext_pool_nonpaged_metadata_size;
    fbe_u32_t ext_pool_lun_nonpaged_metadata_size;

    /************************************************** 
     * New entries must go here.
     * We can only add at the end to avoid NDU issues.
     **************************************************/
}database_system_db_header_t;
#pragma pack()
/* This enum indicates the table type in DB SRV.
 */
typedef enum fbe_database_table_type_e
{
    FBE_DATABASE_TABLE_TYPE_INVALID,
    FBE_DATABASE_TABLE_TYPE_OBJECT,
    FBE_DATABASE_TABLE_TYPE_USER,
    FBE_DATABASE_TABLE_TYPE_EDGE
}fbe_database_table_type_t;

/*FBE_DATABASE_CONTROL_CODE_GET_TABLES_IN_RANGE. 
  This usurper code will also contain a buffer from the caller to contain all the entried(object,user and edge)*/
/*this API was created  for FBECLI to dump system configuration in order to restore
   the system when sep is in service mode*/
typedef struct fbe_database_control_get_tables_in_range_s {
    fbe_object_id_t    start_object_id;/* INPUT */
    fbe_object_id_t    end_object_id;/*INPUT*/
    fbe_database_table_type_t table_type; /*input*/
    fbe_u32_t      number_of_objects_returned;/* OUTPUT */
}fbe_database_control_get_tables_in_range_t;
/*This usurper code will also contain a buffer from the caller 
   which contains all the entried(object,user and edge)of the objects user want to persist*/
/*this API was created  for FBECLI to  restore
   the system when sep is in service mode*/
typedef struct fbe_database_control_persist_sep_objects_s {
    fbe_u32_t      number_of_objects_requested;/* INPUT */
    fbe_u32_t      number_of_objects_returned;/* OUTPUT */
}fbe_database_control_persist_sep_objets_t;
/* payload for FBE_DATABASE_CONTROL_CODE_GET_MAX_CONFIGURABLE_OBJECTS */
typedef struct fbe_database_control_get_max_configurable_objects_s {
    fbe_u32_t max_configurable_objects;
}fbe_database_control_get_max_configurable_objects_t;

#define FBE_MAX_DATABASE_RESTORE_SIZE  128 * sizeof (database_object_entry_t)
typedef enum fbe_database_config_restore_type_s{
    FBE_DATABASE_CONFIG_INVALID_RESTORE_TYPE = 0,
    FBE_DATABASE_CONFIG_RESTORE_TYPE_OBJECT,
    FBE_DATABASE_CONFIG_RESTORE_TYPE_USER,
    FBE_DATABASE_CONFIG_RESTORE_TYPE_EDGE,
    FBE_DATABASE_CONFIG_RESTORE_TYPE_GLOBAL_INFO
} fbe_database_config_restore_type_t;

/*the following data structure is for configuration restore*/
typedef struct fbe_database_config_entries_restore_s{
    fbe_u32_t  entries_num;     /*how many entries in this restore operation*/
    fbe_u32_t  entry_size;      /*size of each entry*/
    fbe_database_config_restore_type_t   restore_type; 
    fbe_u8_t   entries[FBE_MAX_DATABASE_RESTORE_SIZE];
}fbe_database_config_entries_restore_t;


typedef struct fbe_database_control_update_system_bg_service_s{
    /* setting 'update_bgs_on_both_sp' to FBE_FALSE will disable background service specified in the bgs_flag
     * to the Local SP only.  Setting it to FBE_TRUE will disable background service specified to both SPs.
     */
    fbe_bool_t update_bgs_on_both_sp;
}fbe_database_control_update_system_bg_service_t;

typedef struct fbe_database_control_bool_s{
    /* setting 'update_bgs_on_both_sp' to FBE_FALSE will disable background service specified in the bgs_flag
     * to the Local SP only.  Setting it to FBE_TRUE will disable background service specified to both SPs.
     */
    fbe_bool_t is_enabled;
}fbe_database_control_bool_t;

typedef struct fbe_database_control_drive_connection_s {
    fbe_u32_t                                  request_size;
    fbe_object_id_t                        PVD_list[FBE_DATABASE_TRANSACTION_MAX_CREATE_OBJECTS_PER_JOB];
}fbe_database_control_drive_connection_t;

typedef struct fbe_database_control_set_invalidate_config_flag_s {
    fbe_u32_t   flag;
}fbe_database_control_set_invalidate_config_flag_t;

/*all hooks that user can set in database service*/
typedef enum fbe_database_hook_type_e
{
    FBE_DATABASE_HOOK_TYPE_INVALID = 0,
    FBE_DATABASE_HOOK_TYPE_WAIT_IN_UPDATE_TRANSACTION = 1,
    FBE_DATABASE_HOOK_TYPE_WAIT_BEFORE_TRANSACTION_PERSIST = 2,
    FBE_DATABASE_HOOK_TYPE_PANIC_IN_UPDATE_TRANSACTION = 3,
    FBE_DATABASE_HOOK_TYPE_PANIC_BEFORE_TRANSACTION_PERSIST = 4,
    FBE_DATABASE_HOOK_TYPE_WAIT_IN_UPDATE_ROLLBACK_TRANSACTION = 5,
    FBE_DATABASE_HOOK_TYPE_PANIC_IN_UPDATE_ROLLBACK_TRANSACTION = 6,
    FBE_DATABASE_HOOK_TYPE_WAIT_BEFORE_ROLLBACK_TRANSACTION_PERSIST = 7,
    FBE_DATABASE_HOOK_TYPE_PANIC_BEFORE_ROLLBACK_TRANSACTION_PERSIST = 8,
    FBE_DATABASE_HOOK_TYPE_LAST
}fbe_database_hook_type_t;

/*all operations that user can perform to a specific hook*/
typedef enum fbe_database_hook_opt_e
{
    FBE_DATABASE_HOOK_OPERATION_INVALID = 0,
    FBE_DATABASE_HOOK_OPERATION_GET_STATE = 1,
    FBE_DATABASE_HOOK_OPERATION_SET_HOOK = 2,
    FBE_DATABASE_HOOK_OPERATION_REMOVE_HOOK = 3,
    FBE_DATABASE_HOOK_OPERATION_LAST
}fbe_database_hook_opt_t;

/*control structure that user can use to operate a hook*/
typedef struct fbe_database_control_hook_s {
    fbe_database_hook_type_t hook_type;
    fbe_database_hook_opt_t  hook_op_code;
    fbe_bool_t  is_set; /*output for FBE_DATABASE_HOOK_OPERATION_GET_STATE*/
    fbe_bool_t  is_triggered; /*output for FBE_DATABASE_HOOK_OPERATION_GET_STATE*/
}fbe_database_control_hook_t;

/*! @enum fbe_database_poll_request
 *  
 *  @brief Enum indicating the type of poll from the database
 */
typedef enum fbe_database_poll_request_e{
    POLL_REQUEST_GET_ALL_LUNS           = 0x1,  /* flag for database_control_get_all_luns */
    POLL_REQUEST_GET_ALL_RAID_GROUPS    = 0x2,  /* flag for database_control_get_all_raid_groups */ 
    POLL_REQUEST_GET_ALL_PVDS           = 0x4,  /* flag for database_control_get_all_pvds */  
    POLL_REQUEST_MASK                   = 0xF,    
}fbe_database_poll_request_t;

/*this is used to make sure we spend as little time as possible collecting information for the
IOCTL_FLARE_GET_RAID_INFO ioctl since there is a system requirement to complete is fast,
but MCR breaks the context to the ioctl thread so we have to make sure we don't make things even worse
by allocating pacets and waiting for memory*/
typedef struct fbe_database_get_sep_shim_raid_info_s{
    fbe_object_id_t				lun_object_id;
    fbe_u8_t                    lun_characteristics;/*most likely outdated but we persist and keep it*/
    fbe_u16_t                   rotational_rate; //0x01 for SSD bound lun
    fbe_block_size_t    		exported_block_size;
    fbe_u32_t					width;
    fbe_block_count_t 			sectors_per_stripe;
    fbe_element_size_t          element_size; /*!< Blocks in each stripe element.*/
    fbe_lba_t                   capacity;/*not including area at the end for cache, this is just what the user sees*/
    fbe_assigned_wwid_t         world_wide_name;   
    fbe_user_defined_lun_name_t user_defined_name;
    fbe_time_t					bind_time;
    fbe_elements_per_parity_t 	elements_per_parity_stripe;
    fbe_raid_group_number_t		raid_group_number;
    fbe_object_id_t             raid_group_id;
    fbe_raid_group_type_t 		raid_type;
    fbe_u32_t                   max_queue_depth;
}fbe_database_get_sep_shim_raid_info_t;

fbe_status_t fbe_database_get_sep_shim_raid_info(fbe_database_get_sep_shim_raid_info_t *sep_shim_info);

/*!*******************************************************************
 * @enum fbe_database_lun_operation_code_e
 *********************************************************************
 * @brief
 *  This is the set of lun operations for which warning message is sent by **FUNCTION function from cli.
 *
 *********************************************************************/
typedef enum fbe_database_lun_operation_code_e {
    FBE_DATABASE_LUN_CREATE_FBE_CLI,
    FBE_DATABASE_LUN_DESTROY_FBE_CLI
} fbe_database_lun_operation_code_t;

/*structure that is used for sending a warning message on the SP when lun is created or destroyed from cli*/
typedef struct fbe_database_set_lu_operation_s
{
    fbe_object_id_t lun_id;
    fbe_status_t     status;
    fbe_database_lun_operation_code_t   operation;
} fbe_database_set_lu_operation_t;

/* FBE_DATABASE_CONTROL_CODE_GET_TOTAL_LOCKED_OBJECTS_OF_CLASS */
typedef struct fbe_database_control_get_total_locked_objects_of_class_s{
    fbe_u32_t number_of_objects;
    database_class_id_t class_id;
}fbe_database_control_get_total_locked_objects_of_class_t;

/* FBE_DATABASE_CONTROL_CODE_GET_LOCKED_OBJECTS_INFO_OF_CLASS */
typedef struct fbe_database_control_get_locked_info_of_class_header_s{
    fbe_u32_t number_of_objects;/* INPUT */
    database_class_id_t class_id;
    fbe_bool_t get_locked_only;
    fbe_u32_t number_of_objects_copied;/* OUTPUT */
}fbe_database_control_get_locked_info_of_class_header_t;

typedef struct fbe_database_control_get_locked_object_info_s{
    fbe_object_id_t						object_id;
    fbe_config_generation_t				generation_number;
    fbe_u32_t							control_number; /* This is object specified number e.g. RG Number, pool ID etc., */
    fbe_u32_t							width; /* For the raid group */
    fbe_base_config_encryption_state_t	encryption_state; /* Current encryption state */
    fbe_raid_group_type_t               raid_type;
}fbe_database_control_get_locked_object_info_t;

typedef struct fbe_database_control_get_object_encryption_key_handle_s{
    fbe_object_id_t                         object_id;
    fbe_encryption_setup_encryption_keys_t *key_handle;
}fbe_database_control_get_object_encryption_key_handle_t;

typedef struct fbe_database_system_encryption_progress_s{
    fbe_u64_t blocks_already_encrypted; 
    fbe_u64_t total_capacity_in_blocks;   /* Overall RG capacity */
}fbe_database_system_encryption_progress_t;

typedef struct fbe_database_system_scrub_progress_s{
    fbe_u64_t blocks_already_scrubbed; 
    fbe_u64_t total_capacity_in_blocks;   /* Overall system capacity */
}fbe_database_system_scrub_progress_t;

typedef struct fbe_database_system_poll_interval_s{
    fbe_u32_t poll_interval; 
}fbe_database_system_poll_interval_t;

typedef struct fbe_database_system_encryption_info_s{
    fbe_system_encryption_state_t system_encryption_state;
    fbe_system_scrubbing_state_t system_scrubbing_state;	
}fbe_database_system_encryption_info_t;

typedef struct fbe_database_backup_info_s{
	fbe_encryption_backup_state_t encryption_backup_state;
	SP_ID primary_SP_ID;
}fbe_database_backup_info_t;

typedef struct fbe_database_be_port_info_s{ 
    fbe_object_id_t port_object_id;
    fbe_u8_t       be_number;
    fbe_u8_t serial_number[FBE_PORT_SERIAL_NUM_SIZE];
    fbe_port_encryption_mode_t port_encrypt_mode;
}fbe_database_be_port_info_t;

typedef struct fbe_database_control_get_be_port_info_s{
    fbe_cmi_sp_id_t sp_id; /* Input which SP ID you want */
    fbe_u32_t num_be_ports;
    fbe_database_be_port_info_t port_info[FBE_MAX_BE_PORTS_PER_SP];
}fbe_database_control_get_be_port_info_t;

typedef enum database_encryption_notification_reason_e {
    DATABASE_ENCRYPTION_NOTIFICATION_REASON_INVALID = 0,
    DATABASE_ENCRYPTION_NOTIFICATION_REASON_PEER_REQUEST_TABLE,
    DATABASE_ENCRYPTION_NOTIFICATION_REASON_PEER_LOST,
}database_encryption_notification_reason_t;

typedef enum fbe_database_control_db_update_on_peer_op_e {
    DATABASE_CONTROL_DB_UPDATE_INVALID = 0,
    DATABASE_CONTROL_DB_UPDATE_PROCEED,
    DATABASE_CONTROL_DB_UPDATE_FAILED,
}fbe_database_control_db_update_on_peer_op_t;

typedef struct fbe_database_control_db_update_peer_s {
    fbe_database_control_db_update_on_peer_op_t  update_op;
}fbe_database_control_db_update_peer_t;

typedef struct fbe_database_control_get_drive_sn_s{
    fbe_object_id_t rg_id;
    fbe_u8_t serial_number[FBE_RAID_MAX_DISK_ARRAY_WIDTH+1][FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1];
}fbe_database_control_get_drive_sn_t;

typedef struct fbe_database_control_set_debug_flags_s {
    fbe_database_debug_flags_t  set_db_debug_flags;
}fbe_database_control_set_debug_flags_t;

typedef struct fbe_database_control_get_debug_flags_s {
    fbe_database_debug_flags_t  get_db_debug_flags;
}fbe_database_control_get_debug_flags_t;

typedef struct fbe_database_control_is_validation_allowed_s {
    fbe_database_validate_request_type_t validate_caller;
    fbe_bool_t                           b_allowed;
    fbe_database_validate_not_allowed_reason_t not_allowed_reason;
}fbe_database_control_is_validation_allowed_t;

typedef struct fbe_database_control_validate_database_s {
    fbe_database_validate_request_type_t validate_caller;
    fbe_database_validate_failure_action_t failure_action;
    fbe_status_t                         validation_status;
}fbe_database_control_validate_database_t;

typedef struct fbe_database_control_enter_degraded_mode_s {
    fbe_database_service_mode_reason_t  degraded_mode_reason;
}fbe_database_control_enter_degraded_mode_t;

typedef struct fbe_database_control_lookup_ext_pool_s
{
    fbe_u32_t pool_id;
    fbe_object_id_t object_id;
} fbe_database_control_lookup_ext_pool_t;

typedef struct fbe_database_control_lookup_ext_pool_lun_s
{
    fbe_u32_t pool_id;
    fbe_u32_t lun_id;
    fbe_object_id_t object_id;
} fbe_database_control_lookup_ext_pool_lun_t;

typedef struct fbe_export_device_info_s {
    fbe_lun_number_t            lun_number;
    fbe_u32_t                   region_id;   /* This is the region that the device represents */
    fbe_u32_t                   offset;
    fbe_u32_t                   size_in_blocks;
    fbe_lba_t                   start_address;   
    fbe_char_t                  exported_device_name[64];
    fbe_u32_t                   flags;
    void *                      export_device;
} fbe_export_device_info_t;

typedef struct fbe_database_control_mirror_update_s {
    fbe_object_id_t rg_obj_id;
    fbe_u16_t       edge_index;
    fbe_u8_t        sn[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1];
} fbe_database_control_mirror_update_t;

typedef struct fbe_database_get_nonpaged_metadata_data_ptr_s {
    fbe_object_id_t object_id;
    void    * data_ptr;
}fbe_database_get_nonpaged_metadata_data_ptr_t;

typedef enum fbe_database_drive_types_supported_e
{
    /* note: this is a bitmap */
    FBE_DATABASE_DRIVE_TYPE_SUPPORTED_LE        = 0x0001,
    FBE_DATABASE_DRIVE_TYPE_SUPPORTED_RI        = 0x0002,
    FBE_DATABASE_DRIVE_TYPE_SUPPORTED_520_HDD   = 0x0004,    /* currently not implemented.  Reserved this bit for Unity*/
}fbe_database_additional_drive_types_supported_t;

typedef struct fbe_database_control_set_supported_drive_type_s {
    fbe_bool_t do_enable;
    fbe_database_additional_drive_types_supported_t type;
}fbe_database_control_set_supported_drive_type_t;

fbe_drive_performance_tier_type_np_t 
fbe_database_determine_drive_performance_tier_type(fbe_physical_drive_mgmt_get_drive_information_t *drive_info_p);

fbe_status_t fbe_database_get_pdo_drive_info(fbe_object_id_t pdo_object_id, 
                                                    fbe_physical_drive_mgmt_get_drive_information_t *pdo_drive_info_p);
void fbe_database_reactivate_pdos_of_drive_type(fbe_database_additional_drive_types_supported_t drive_type);

fbe_status_t fbe_database_invalidate_key(fbe_object_id_t object_id);
fbe_status_t fbe_database_control_get_object_encryption_key_handle(fbe_object_id_t	object_id, fbe_encryption_setup_encryption_keys_t **key_handle);
fbe_status_t fbe_database_get_encryption_paused(fbe_bool_t *encryption_paused);
fbe_status_t fbe_database_cmi_send_mailbomb(void);

fbe_status_t fbe_c4_mirror_check_new_pvd(fbe_object_id_t rg_obj, fbe_u32_t *bitmask);
fbe_status_t fbe_c4_mirror_rginfo_update(fbe_packet_t *pkt, fbe_object_id_t rg_obj, fbe_u32_t edge);
fbe_status_t fbe_c4_mirror_load_rginfo(fbe_object_id_t rg_obj, fbe_packet_t *packet);
fbe_status_t fbe_c4_mirror_load_rginfo_during_boot_path(fbe_object_id_t obj);  /* Load the rginfo synchronously*/
fbe_status_t fbe_c4_mirror_update_pvd(fbe_u32_t slot, fbe_u8_t *sn, fbe_u32_t sn_len);
fbe_status_t fbe_c4_mirror_update_pvd_by_object_id(fbe_object_id_t pvd_obj, fbe_u8_t *sn, fbe_u32_t sn_len);
fbe_status_t fbe_c4_mirror_update_default(fbe_object_id_t rg_obj);
fbe_status_t fbe_c4_mirror_write_default(fbe_packet_t *packet_p, fbe_object_id_t rg_obj);
fbe_status_t fbe_c4_mirror_get_nonpaged_metadata_config(void **raw_mirror_pp, fbe_lba_t *start_lba_p, fbe_block_count_t *capacity_p);

fbe_status_t fbe_c4_mirror_is_load(fbe_object_id_t rg_obj, fbe_bool_t *is_loaded);
fbe_status_t fbe_c4_mirror_rginfo_modify_drive_SN(fbe_object_id_t rg_obj_id, fbe_u16_t edge_index, fbe_u8_t *sn, fbe_u32_t size);
fbe_status_t fbe_c4_mirror_rginfo_get_pvd_mapping(fbe_object_id_t rg_obj_id, fbe_u16_t edge_index, fbe_u8_t *sn, fbe_u32_t size);

fbe_status_t fbe_database_is_object_c4_mirror(fbe_object_id_t object_id);
#endif /* FBE_DATABASE_H */

