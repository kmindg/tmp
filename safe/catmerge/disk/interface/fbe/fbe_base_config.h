#ifndef FBE_BASE_CONFIG_H
#define FBE_BASE_CONFIG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_base_config.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions that are exported by the base config
 *  object.
 * 
 * @ingroup base_config_class_files
 * 
 * @revision
 *   5/20/2009:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_object.h"
#include "fbe/fbe_limits.h"
#include "fbe/fbe_payload_metadata_operation.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_sector.h"
#include "fbe/fbe_power_save_interface.h"
#include "fbe/fbe_metadata_interface.h"
#include "fbe/fbe_event_interface.h"
#include "fbe_encryption.h"

/*************************
 *   FUNCTION DEFINITIONS 
 *************************/

 /*! @defgroup base_config_class Base Config Class
 *  @brief This is the set of definitions for the base config.
 *  @ingroup fbe_base_object
 */ 
/*! @defgroup base_config_usurper_interface Base Config Usurper Interface
 *  @brief This is the set of definitions that comprise the base config class
 *  usurper interface.
 *  @ingroup fbe_classes_usurper_interface
 * @{
 */ 


typedef enum fbe_base_config_encryption_mode_e{
    FBE_BASE_CONFIG_ENCRYPTION_MODE_INVALID = 0,
    FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED = 16,

    FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS = 20,

	FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS = 24,

    FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED = 32,

    FBE_BASE_CONFIG_ENCRYPTION_MODE_LAST
}fbe_base_config_encryption_mode_t;


/*!********************************************************************* 
 * @enum fbe_base_config_control_code_t 
 *  
 * @brief 
 * This enumeration lists all the base config specific control codes. 
 * These are control codes specific to the base config, which only 
 * the base config object will accept. 
 *         
 **********************************************************************/
typedef enum
{
    FBE_BASE_CONFIG_CONTROL_CODE_INVALID = FBE_OBJECT_CONTROL_CODE_INVALID_DEF(FBE_CLASS_ID_BASE_CONFIG),

    FBE_BASE_CONFIG_CONTROL_CODE_METADATA_PAGED_SET_BITS,
    FBE_BASE_CONFIG_CONTROL_CODE_METADATA_PAGED_CLEAR_BITS,
    FBE_BASE_CONFIG_CONTROL_CODE_METADATA_PAGED_GET_BITS,
    FBE_BASE_CONFIG_CONTROL_CODE_METADATA_PAGED_CLEAR_CACHE,
    FBE_BASE_CONFIG_CONTROL_CODE_METADATA_PAGED_WRITE,

    FBE_BASE_CONFIG_CONTROL_CODE_METADATA_NONPAGED_SET_BITS,
    FBE_BASE_CONFIG_CONTROL_CODE_METADATA_NONPAGED_CLEAR_BITS,
    FBE_BASE_CONFIG_CONTROL_CODE_METADATA_NONPAGED_SET_CHECKPOINT,
    FBE_BASE_CONFIG_CONTROL_CODE_METADATA_NONPAGED_INCR_CHECKPOINT,

    FBE_BASE_CONFIG_CONTROL_CODE_METADATA_GET_INFO,
    FBE_BASE_CONFIG_CONTROL_CODE_GET_UPSTREAM_EDGE_COUNT,
    FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_EDGE_LIST,
    FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_OBJECT_LIST,
    FBE_BASE_CONFIG_CONTROL_CODE_GET_UPSTREAM_OBJECT_LIST,
    FBE_BASE_CONFIG_CONTROL_CODE_CAN_OBJECT_BE_REMOVED,
    FBE_BASE_CONFIG_CONTROL_CODE_GET_CAPACITY,
    FBE_BASE_CONFIG_CONTROL_CODE_IO_CONTROL_OPERATION,
    FBE_BASE_CONFIG_CONTROL_CODE_SET_SYSTEM_POWER_SAVING_INFO,
    FBE_BASE_CONFIG_CONTROL_CODE_GET_SYSTEM_POWER_SAVING_INFO,
    FBE_BASE_CONFIG_CONTROL_CODE_ENABLE_OBJECT_POWER_SAVE,
    FBE_BASE_CONFIG_CONTROL_CODE_DISABLE_OBJECT_POWER_SAVE,
    FBE_BASE_CONFIG_CONTROL_CODE_GET_OBJECT_POWER_SAVE_INFO,
    FBE_BASE_CONFIG_CONTROL_CODE_SET_OBJECT_POWER_SAVE_IDLE_TIME,
    FBE_BASE_CONFIG_CONTROL_CODE_COMMIT_CONFIGURATION,
    FBE_BASE_CONFIG_CONTROL_CODE_GET_OBJECT_POWER_SAVE_STATE,
    FBE_BASE_CONFIG_CONTROL_CODE_GET_WIDTH,
    FBE_BASE_CONFIG_CONTROL_CODE_NONPAGED_PERSIST, /* Will persist nonpaged metadata of the object */
    FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_EDGE_GEOMETRY,
    FBE_BASE_CONFIG_CONTROL_CODE_GET_METADATA_STATISTICS,
    FBE_BASE_CONFIG_CONTROL_CODE_SEND_EVENT,    
    FBE_BASE_CONFIG_CONTROL_CODE_BACKGROUND_OPERATION, /* enable/disable background operation */
    FBE_BASE_CONFIG_CONTROL_CODE_BACKGROUND_OP_ENABLE_CHECK, /* provide enable/disable status of background operation*/
    FBE_BASE_CONFIG_CONTROL_CODE_DISABLE_CMI, /*makes sure we stop sending/receiveing CMI in preperation for destroy*/
    FBE_BASE_CONFIG_CONTROL_CODE_CONTROL_SYSTEM_BG_SERVICE, /* enable/disbale system background services.*/
    FBE_BASE_CONFIG_CONTROL_CODE_GET_SYSTEM_BG_SERVICE_STATUS, /* get system background service enable/disable status.*/
    FBE_BASE_CONFIG_CONTROL_CODE_SET_NONPAGED_METADATA_SIZE,
    FBE_BASE_CONFIG_CONTROL_CODE_NONPAGED_WRITE_VERIFY,/*get the need write verify flag in base config */
    FBE_BASE_CONFIG_CONTROL_CODE_PASSIVE_REQUEST, /*The ACTIVE object suppose to become PASSIVE if the other SP is UP. */
    FBE_BASE_CONFIG_CONTROL_CODE_METADATA_SET_NONPAGED,/*This is used in MD raw mirror load error handling when fixing specialize object.*/
    FBE_BASE_CONFIG_CONTROL_CODE_SET_DENY_OPERATION_PERMISSION, /* `Deny' all background operation permission for a particular object.*/
    FBE_BASE_CONFIG_CONTROL_CODE_CLEAR_DENY_OPERATION_PERMISSION, /* Clear `Deny' all background operation permission for a particular object.*/
    FBE_BASE_CONFIG_CONTROL_CODE_SET_PERMANENT_DESTROY, /* Set permanent destroy. */
    FBE_BASE_CONFIG_CONTROL_CODE_GET_SPINDOWN_QUALIFIED, /*!< Client wants to get the spindown qualified flag from the server. */
	FBE_BASE_CONFIG_CONTROL_CODE_GET_STRIPE_BLOB, /* Get's dual SP stripe lock blob */
	FBE_BASE_CONFIG_CONTROL_CODE_GET_PEER_LIFECYCLE_STATE, /* Get's the peer's state */
    FBE_BASE_CONFIG_CONTROL_CODE_METADATA_NONPAGED_WRITE_PERSIST,
    FBE_BASE_CONFIG_CONTROL_CODE_METADATA_MEMORY_READ,  /* Read memtadata_memory */
    FBE_BASE_CONFIG_CONTROL_CODE_METADATA_MEMORY_UPDATE, /* Update metadata memory */
    FBE_BASE_CONFIG_CONTROL_CODE_GET_ENCRYPTION_STATE, /* Get the encryption state of object */
    FBE_BASE_CONFIG_CONTROL_CODE_SET_ENCRYPTION_STATE, /* Set the encryption state of object */
    FBE_BASE_CONFIG_CONTROL_CODE_GET_ENCRYPTION_MODE, /* Get the encryption mode of the object */
    FBE_BASE_CONFIG_CONTROL_CODE_SET_ENCRYPTION_MODE, /* Set the encryption mode of the object */
    FBE_BASE_CONFIG_CONTROL_CODE_SET_SYSTEM_ENCRYPTION_INFO,
    FBE_BASE_CONFIG_CONTROL_CODE_GET_SYSTEM_ENCRYPTION_INFO, /* Get's the system wide encryption state */
    FBE_BASE_CONFIG_CONTROL_CODE_INIT_KEY_HANDLE, /* Init the objects key handle */
    FBE_BASE_CONFIG_CONTROL_CODE_GET_GENERATION_NUMBER, /* Get the generation number for the object */
    FBE_BASE_CONFIG_CONTROL_CODE_REMOVE_KEY_HANDLE, /* Init the objects key handle */
    FBE_BASE_CONFIG_CONTROL_CODE_UPDATE_DRIVE_KEYS, /* Update keys */
    FBE_BASE_CONFIG_CONTROL_CODE_DISABLE_PEER_OBJECT, /* disable the peer object */
    FBE_BASE_CONFIG_CONTROL_CODE_LAST
}
fbe_base_config_control_code_t;


/*! @enum fbe_base_config_clustered_flags_e
 *  
 *  @brief flags to indicate states of the base config object related to dual-SP processing.
 */
enum fbe_base_config_clustered_flags_e {
    FBE_BASE_CONFIG_METADATA_MEMORY_QUIESCE_NOT_STARTED         = 0x0000000000000000,
    FBE_BASE_CONFIG_METADATA_MEMORY_QUIESCE_READY               = 0x0000000000000001,
    FBE_BASE_CONFIG_METADATA_MEMORY_QUIESCE_COMPLETE            = 0x0000000000000002,
    FBE_BASE_CONFIG_METADATA_MEMORY_UNQUIESCE_READY             = 0x0000000000000004,

    FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_MASK          = 0x0000000000000007,
    /*! Signifies a flavor of quiesce, which holds I/Os from entering the object.  */
    FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_HOLD          = 0x0000000000000008,
    FBE_BASE_CONFIG_METADATA_MEMORY_PEER_NONPAGED_REQUEST       = 0x0000000000000020,

    /*! Clustered flag to indicate non-paged metadata is initialized. */
    FBE_BASE_CONFIG_CLUSTERED_FLAG_NONPAGED_INITIALIZED         = 0x0000000000000040,

    /*! The base config is quiesced. Do not start any new I/Os or restart siots. */
    /* !@TODO: multiple flags for quiescing I/O (metadata memory and clustered); try to combine them so using one set of flags */
    FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED                     = 0x0000000000000100,
    /*! The base config object is quiescing. Do not start any new I/Os or restart siots. */
    FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCING                    = 0x0000000000000200,
    FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCE_MASK                 = 0x0000000000000300,

    /*! These are the flags we use to coordinate the passive side joining the active side. */
    FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_REQ                     = 0x0000000000001000,
    FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED                 = 0x0000000000002000,
    FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_SYNCING                 = 0x0000000000004000,
    FBE_BASE_CONFIG_CLUSTERED_FLAG_JOINED                       = 0x0000000000008000,
    FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_MASK                    = 0x000000000000f000,

    /*! Clustered flag to indicate we are broken */
    FBE_BASE_CONFIG_CLUSTERED_FLAG_BROKEN                       = 0x0000000000010000,

    /* ! These flags are used to coordinate putting the objects in activate state
     */
    FBE_BASE_CONFIG_CLUSTERED_FLAG_ACTIVATE_REQ                 = 0x0000000000100000,
    FBE_BASE_CONFIG_CLUSTERED_FLAG_ACTIVATE_STARTED             = 0x0000000000200000,
    FBE_BASE_CONFIG_CLUSTERED_FLAG_ACTIVATE_MASK                = 0x0000000000300000,

    FBE_BASE_CONFIG_CLUSTERED_FLAG_PASSIVE_REQ                  = 0x0000000001000000, /* Indicates that this side is active */
    FBE_BASE_CONFIG_CLUSTERED_FLAG_PASSIVE_STARTED              = 0x0000000002000000, /* Indicates that this side just become passive */
    FBE_BASE_CONFIG_CLUSTERED_FLAG_PASSIVE_MASK                 = 0x0000000003000000,

    FBE_BASE_CONFIG_CLUSTERED_FLAG_HIBERNATE_READY              = 0x0000000010000000, /* Indicates that this side is ready to go hibernate */
    FBE_BASE_CONFIG_CLUSTERED_FLAG_HIBERNATE_MASK               = 0x0000000010000000, 
    /*! Masks of all peer bits.
     */
    FBE_BASE_CONFIG_CLUSTERED_FLAG_ALL_MASKS = (FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_MASK |
                                                FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCE_MASK |
                                                FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_MASK |
                                                FBE_BASE_CONFIG_CLUSTERED_FLAG_BROKEN |
                                                FBE_BASE_CONFIG_CLUSTERED_FLAG_ACTIVATE_MASK |
                                                FBE_BASE_CONFIG_CLUSTERED_FLAG_HIBERNATE_MASK),

    FBE_BASE_CONFIG_METADATA_MEMORY_INVALID_BITMAP      = 0xFFFFFFFFFFFFFFFFULL,
};
typedef fbe_u64_t fbe_base_config_clustered_flags_t;
typedef fbe_base_config_clustered_flags_t fbe_base_config_metadata_memory_flags_t;


/*! @enum fbe_base_config_flags_e
 *  
 *  @brief flags to indicate general states of the base config object
 */
enum fbe_base_config_flags_e {
    FBE_BASE_CONFIG_FLAG_POWER_SAVING_ENABLED       = 0x0000000000000001,
    FBE_BASE_CONFIG_FLAG_CONFIGURATION_COMMITED     = 0x0000000000000002,
    FBE_BASE_CONFIG_FLAG_USER_INIT_QUIESCE          = 0x0000000000000004,
    FBE_BASE_CONFIG_FLAG_DENY_OPERATION_PERMISSION  = 0x0000000000000008,
    FBE_BASE_CONFIG_FLAG_PERM_DESTROY               = 0x0000000000000010,
    FBE_BASE_CONFIG_FLAG_RESPECIALIZE               = 0x0000000000000020,
    FBE_BASE_CONFIG_FLAG_INITIAL_CONFIGURATION      = 0x0000000000000080,
    FBE_BASE_CONFIG_FLAG_REMOVE_ENCRYPTION_KEYS     = 0x0000000000000100,
    FBE_BASE_CONFIG_FLAG_PEER_NOT_ALIVE             = 0x0000000000000200,
};
typedef fbe_u64_t fbe_base_config_flags_t;


/*!********************************************************************* 
 * @enum fbe_base_config_downstream_health_state_e
 *  
 * @brief 
 * This enumeration lists all the possible base config downstream object state.
 *
 **********************************************************************/
typedef enum fbe_base_config_downstream_health_state_e
{
    FBE_DOWNSTREAM_HEALTH_OPTIMAL,
    FBE_DOWNSTREAM_HEALTH_DEGRADED,
    FBE_DOWNSTREAM_HEALTH_DISABLED,
    FBE_DOWNSTREAM_HEALTH_BROKEN
} fbe_base_config_downstream_health_state_t;

/*!********************************************************************* 
 * @enum fbe_base_config_encryption_state_e
 *  
 * @brief 
 * This enumeration lists all the possible base config encryption state for the objects.
 *
 **********************************************************************/
typedef enum fbe_base_config_encryption_state_e
{
    FBE_BASE_CONFIG_ENCRYPTION_STATE_INVALID = 0,
    FBE_BASE_CONFIG_ENCRYPTION_STATE_UNENCRYPTED = 1,

    /* LOCKED State*/
    FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_KEY_ERROR      = 14,
    FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_KEYS_INCORRECT = 15,
    FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_NEW_KEYS_REQUIRED = 16,
    FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_CURRENT_KEYS_REQUIRED = 20,


    FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_PUSH_KEYS_DOWNSTREAM = 24, /* Raid already got keys from database */
    /* Add new locked state above this line. Please update
     * fbe_base_config_is_object_encryption_state_locked if applicable */
    FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED = 28,
    FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED_FOR_REKEY = 29,

    FBE_BASE_CONFIG_ENCRYPTION_STATE_ENCRYPTION_IN_PROGRESS = 32,
    FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEYING_IN_PROGRESS = 48,

    FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE = 52,
    FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE_PUSH_KEYS = 56,
    FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE_PUSH_DONE = 60,
    FBE_BASE_CONFIG_ENCRYPTION_STATE_PUSH_KEYS_FOR_REKEY = 61,

    FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE_REMOVE_OLD_KEY = 64,

    FBE_BASE_CONFIG_ENCRYPTION_STATE_ENCRYPTED = 128,

    FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_OUT_OLD_KEY_NEED_REMOVED = 132,
    FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_OUT_OLD_KEY_REMOVED = 136,
    FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_IN_NEW_KEY_REQUIRED = 140,
    FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_IN_NEW_KEY_PROVIDED = 144,

    FBE_BASE_CONFIG_ENCRYPTION_STATE_ALL_KEYS_NEED_REMOVED = 256,
    FBE_BASE_CONFIG_ENCRYPTION_STATE_PUSH_ALL_KEYS_REMOVE_DOWNSTREAM = 260,
    FBE_BASE_CONFIG_ENCRYPTION_STATE_ALL_KEYS_REMOVED = 264,


} fbe_base_config_encryption_state_t;

/*!*******************************************************************
 * @struct fbe_base_config_upstream_edge_count_s
 *********************************************************************
 * @brief
 *  This structure is used to get the upstream edge count. list.
 *
 *********************************************************************/
typedef struct fbe_base_config_upstream_edge_count_s
{
    /*! @note This field is used to return the number of clients associated 
     *        base config object.
     */
    fbe_u32_t number_of_upstream_edges;
}
fbe_base_config_upstream_edge_count_t;

/*!*******************************************************************
 * @struct fbe_base_config_downstream_edge_list_s
 *********************************************************************
 * @brief
 *  This structure is used to get the downstream edge list.
 *
 *********************************************************************/
typedef struct fbe_base_config_downstream_edge_list_s
{

    /*! This field is used to return the number of downstream edges
     *  information.
     */
    fbe_u32_t number_of_downstream_edges;

    /*! This field is used to return the number of downstream object
     *  information.
     */
    fbe_base_edge_t downstream_edge_list[FBE_MAX_DOWNSTREAM_OBJECTS];
}
fbe_base_config_downstream_edge_list_t;

/*!*******************************************************************
 * @struct fbe_base_config_upstream_object_list_s
 *********************************************************************
 * @brief
 *  This structure is used to get the upstream object list.
 *
 *********************************************************************/
typedef struct fbe_base_config_upstream_object_list_s
{
    /*! @note This field is used to return the number of clients associated 
     *        base config object.
     */
    fbe_u32_t number_of_upstream_objects;
    fbe_u32_t current_upstream_index;
    fbe_object_id_t upstream_object_list [FBE_MAX_UPSTREAM_OBJECTS];
}
fbe_base_config_upstream_object_list_t;

/*!*******************************************************************
 * @struct fbe_base_config_downstream_object_list_s
 *********************************************************************
 * @brief
 *  This structure is used to get the downstream object list.
 *
 *********************************************************************/
typedef struct fbe_base_config_downstream_object_list_s
{
    /*! This field is used to return the number of downstream object
     *  information.
     */
    fbe_u32_t number_of_downstream_objects;
    fbe_object_id_t downstream_object_list[FBE_MAX_DOWNSTREAM_OBJECTS];
}
fbe_base_config_downstream_object_list_t;

/*!*******************************************************************
 * @struct fbe_base_config_commit_configuration_s
 *********************************************************************
 * @brief
 *  This structure is used to commit the configuration.
 *
 *********************************************************************/
typedef struct fbe_base_config_commit_configuration_s
{
    /*! This field is used to determine whether configuration commited 
     *  or not.
     */
    fbe_bool_t is_configuration_commited;

    /*! This field is used to indicate whether this object is the very
     *  first time to do commit (first created)*/
    fbe_bool_t is_initial_commit;
}
fbe_base_config_commit_configuration_t;

/*!*******************************************************************
 * @struct fbe_base_config_can_object_be_removed_s
 *********************************************************************
 * @brief
 *  This structure is used to determine if an object can be
 *  removed
 *
 *********************************************************************/
typedef struct fbe_base_config_can_object_be_removed_s
{
    fbe_object_id_t  object_id;
    fbe_bool_t       object_to_remove;
}
fbe_base_config_can_object_be_removed_t;

/*!*******************************************************************
 * @struct fbe_base_config_block_edge_geometry_s
 *********************************************************************
 * @brief
 *  This structure is used to obtain an edge's block geometry
 *
 *********************************************************************/
typedef struct fbe_base_config_block_edge_geometry_t
{
    /*! This field is used to obtain an edge's block geometry
    */
    fbe_block_edge_geometry_t block_geometry;
}
fbe_base_config_block_edge_geometry_t;


/* FBE_BASE_CONFIG_CONTROL_CODE_METADATA_PAGED_SET_BITS */
/* FBE_BASE_CONFIG_CONTROL_CODE_METADATA_PAGED_CLEAR_BITS */
typedef struct fbe_base_config_control_metadata_paged_change_bits_s {
    fbe_u64_t   metadata_offset;
    fbe_u8_t    metadata_record_data[FBE_PAYLOAD_METADATA_MAX_DATA_SIZE];
    fbe_u32_t   metadata_record_data_size;
    fbe_u64_t   metadata_repeat_count;
    fbe_u64_t   metadata_repeat_offset;
}fbe_base_config_control_metadata_paged_change_bits_t;

/* FBE_BASE_CONFIG_CONTROL_CODE_METADATA_PAGED_GET_BITS */
enum fbe_base_config_control_metadata_paged_get_bits_flags_e {
    FBE_BASE_CONFIG_CONTROL_METADATA_PAGED_GET_BITS_FLAGS_NONE      = 0x00000000, /* Default */
    FBE_BASE_CONFIG_CONTROL_METADATA_PAGED_GET_BITS_FLAGS_FUA_READ  = 0x00000001, /* Indicates that we must clear cache for the block before reading from disk*/
};
typedef fbe_u32_t fbe_base_config_control_metadata_paged_get_bits_flags_t;
typedef struct fbe_base_config_control_metadata_paged_get_bits_s {
    fbe_u64_t   metadata_offset;
    fbe_u8_t    metadata_record_data[FBE_PAYLOAD_METADATA_MAX_DATA_SIZE];
    fbe_u32_t   metadata_record_data_size;
    fbe_base_config_control_metadata_paged_get_bits_flags_t get_bits_flags;
}fbe_base_config_control_metadata_paged_get_bits_t;


/* FBE_BASE_CONFIG_CONTROL_CODE_METADATA_NONPAGED_SET_BITS */
/* FBE_BASE_CONFIG_CONTROL_CODE_METADATA_NONPAGED_CLEAR_BITS */
/* FBE_BASE_CONFIG_CONTROL_CODE_METADATA_NONPAGED_SET_CHECKPOINT */
/* FBE_BASE_CONFIG_CONTROL_CODE_METADATA_NONPAGED_INCR_CHECKPOINT */
typedef struct fbe_base_config_control_metadata_nonpaged_change_s {
    fbe_u64_t   metadata_offset;
    fbe_u8_t    metadata_record_data[FBE_PAYLOAD_METADATA_MAX_DATA_SIZE]; /* May be used as fbe_lba_t checkpoint */
    fbe_u32_t   metadata_record_data_size;
    fbe_u64_t   metadata_repeat_count;
    fbe_u64_t   metadata_repeat_offset;
    fbe_u64_t   second_metadata_offset;
}fbe_base_config_control_metadata_nonpaged_change_t;

/*FBE_BASE_CONFIG_CONTROL_CODE_METADATA_GET_INFO*/
typedef struct fbe_base_config_control_metadata_get_info_s{
    fbe_metadata_info_t     metadata_info;
    fbe_metadata_nonpaged_data_t   nonpaged_data;

    /*return if the nonpaged metadata is initialized*/
    fbe_bool_t                     nonpaged_initialized;
    /*return the current nonpaged metadata data structure size*/
    fbe_u32_t                      nonpaged_data_size;         
    /*return the non-paged metadata structure size of latest committed*/
    fbe_u32_t                      committed_nonpaged_data_size;
    
}fbe_base_config_control_metadata_get_info_t;

/* FBE_BASE_CONFIG_CONTROL_CODE_METADATA_MEMORY_READ */
typedef struct fbe_base_config_control_metadata_memory_read_s {
    /* Read from peer memory or local memory */
    fbe_bool_t  is_peer;
    /* Buffer to store the memory data */
    fbe_u8_t    memory_data[FBE_METADATA_MEMORY_MAX_SIZE];
    /* Return size of data read */
    fbe_u32_t   memory_size;
} fbe_base_config_control_metadata_memory_read_t;

/* FBE_BASE_CONFIG_CONTROL_CODE_METADATA_MEMORY_UPDATE */
typedef struct fbe_base_config_control_metadata_memory_update_s {
    /* The memory data for update */
    fbe_u8_t    memory_data[FBE_METADATA_MEMORY_MAX_SIZE];
    /* Control which bits to update. '1' means we need update */
    fbe_u8_t    memory_mask[FBE_METADATA_MEMORY_MAX_SIZE];
    /* Offset of memory_data we need to update */
    fbe_u32_t   memory_offset;
    /* Size of update data.
     * The update range is: [memory_offset, memory_offset + memory_size) */
    fbe_u32_t   memory_size;
} fbe_base_config_control_metadata_memory_update_t;

/* FBE_BASE_CONFIG_CONTROL_CODE_GET_CAPACITY  */
typedef struct fbe_base_config_control_get_capacity_s
{
    fbe_lba_t capacity;
} fbe_base_config_control_get_capacity_t;

/*FBE_BASE_CONFIG_CONTROL_CODE_GET_WIDTH */
typedef struct fbe_base_config_control_get_width_s
{
    fbe_u32_t width;
} fbe_base_config_control_get_width_t;

/*FBE_BASE_CONFIG_CONTROL_CODE_SET_NONPAGED_METADATA_SIZE*/
typedef struct fbe_base_config_control_metadata_set_size_s{
    /*identify which size we should update: 
      0: only change size in metadata element;
      1: update the size in both metadata element and version header
    */
    fbe_u32_t  set_ver_header;     
    fbe_u32_t  new_size;         /*the new size*/
    fbe_u32_t  old_size;         /*return the old size for the caller*/
}fbe_base_config_control_metadata_set_size_t;
/* FBE_BASE_CONFIG_CONTROL_CODE_GET_CAPACITY  */
typedef struct fbe_base_config_control_set_nonpaged_metadata_s
{
    fbe_bool_t set_default_nonpaged_b;
    fbe_u8_t   np_record_data[FBE_PAYLOAD_METADATA_MAX_DATA_SIZE];
} fbe_base_config_control_set_nonpaged_metadata_t;


typedef struct fbe_base_config_permit_request_s
{
    fbe_permit_event_type_t          permit_event_type;
    fbe_object_id_t                       object_id; 
    fbe_bool_t                              is_start_b;             
    fbe_bool_t                              is_end_b;              
    fbe_block_count_t          block_count;  /* The permit request will use this to get the remain block count 
                                              * for R10.  This is being calculated when the striper received 
                                              * the permit is consumed request event. 
                                              */
}fbe_base_config_permit_request_t;

typedef struct fbe_base_config_data_request_s
{
   fbe_data_event_type_t    data_event_type;
   // add other fields specific to data request
}fbe_base_config_data_request_t;

typedef struct fbe_base_config_verify_report_s
{
    fbe_bool_t                      pass_completed_b;   // TRUE if pass completed
    fbe_raid_verify_error_counts_t  error_counts;       // The counts from the current pass
    fbe_u16_t                       data_disks;         // Needed for RAID10 verify report
}fbe_base_config_verify_report_t;


/*FBE_BASE_CONFIG_CONTROL_CODE_GET_ENCRYPTION_STATE */
typedef struct fbe_base_config_control_get_encryption_state_s
{
    fbe_base_config_encryption_state_t encryption_state;
} fbe_base_config_control_get_encryption_state_t;

/*FBE_BASE_CONFIG_CONTROL_CODE_SET_ENCRYPTION_STATE */
typedef struct fbe_base_config_control_set_encryption_state_s
{
    fbe_base_config_encryption_state_t encryption_state;
} fbe_base_config_control_set_encryption_state_t;

/*FBE_BASE_CONFIG_CONTROL_CODE_GET_ENCRYPTION_MODE */
typedef struct fbe_base_config_control_encryption_mode_s
{
    fbe_base_config_encryption_mode_t encryption_mode;
} fbe_base_config_control_encryption_mode_t;

typedef union fbe_base_config_event_data_u
{
    fbe_base_config_permit_request_t  permit_request;
    fbe_base_config_data_request_t    data_request;
    fbe_base_config_verify_report_t   verify_report;
}fbe_base_config_event_data_t;

/* following structure supports to send all types of events from test
    code to event service queue so that we have flexibility to extend our
    testing for event service functionality */

/* FBE_BASE_CONFIG_CONTROL_CODE_SEND_EVENT */
typedef struct fbe_base_config_control_send_event_s {
    fbe_lba_t                                   lba;
    fbe_block_count_t                       block_count;
    fbe_event_type_t                        event_type;
    fbe_base_config_event_data_t     event_data;
    fbe_u32_t                        medic_action_priority;

}fbe_base_config_control_send_event_t;

/*! Signature mirror index. */
typedef enum fbe_base_config_signature_mirror_index_e
{
    FBE_BASE_CONFIG_SIGNATURE_MIRROR_INDEX_INVALID      = -1,
    FBE_BASE_CONFIG_SIGNATURE_MIRROR_INDEX_PRIMARY      = 0,
    FBE_BASE_CONFIG_SIGNATURE_MIRROR_INDEX_SECONDARY    = 1,
    FBE_BASE_CONFIG_SIGNATURE_MIRROR_INDEX_WIDTH        = 2
} fbe_base_config_signature_mirror_index_t;

/*!*******************************************************************
 * @struct fbe_base_config_io_control_operation_t
 *********************************************************************
 * @brief
 *  This structure is used with the
 *  FBE_BASE_CONFIG_CONTROL_CODE_IO_CONTROL_OPERATION control code.
 *
 *********************************************************************/
typedef struct fbe_base_config_io_control_operation_s
{
    /*! Logical Block address where it issues I/O request. 
     */
    fbe_lba_t           lba;    

    /*! Number of block count, currently it supports maximum 4 blocks. 
     */
    fbe_block_count_t   block_count;

    /*! block operation code. */
    fbe_payload_block_operation_opcode_t block_operation_code;

    /*! Data buffer which holds around 4 blocks of I/O. 
     */
    fbe_u8_t            data_buffer[FBE_BE_BYTES_PER_BLOCK * 4];
} fbe_base_config_io_control_operation_t;

/*!*******************************************************************
 * @struct fbe_base_config_object_power_save_info_t
 *********************************************************************
 * @brief
 *  This structure is used with the
 *  FBE_BASE_CONFIG_CONTROL_CODE_GET_OBJECT_POWER_SAVE_INFO control code.
 *
 *********************************************************************/
typedef struct fbe_base_config_object_power_save_info_s
{
    /*! How long was this object Idle*/
    fbe_u32_t   seconds_since_last_io;   

    /*! Is power saving enabled on this object */
    fbe_bool_t  power_saving_enabled;

    /*! what is the idle time this object is configured to */
    fbe_u64_t   configured_idle_time_in_seconds;

    /*! what state of power saving is the object in */
    fbe_power_save_state_t  power_save_state;

    /*! how long have we been sleeping */
    fbe_time_t  hibernation_time;

    /*! flags that can contains power saving info */
    fbe_base_config_flags_t flags;


} fbe_base_config_object_power_save_info_t;

/*!*******************************************************************
 * @struct fbe_base_config_object_power_save_set_idle_time_t
 *********************************************************************
 * @brief
 *  This structure is used with the
 *  FBE_BASE_CONFIG_CONTROL_CODE_SET_OBJECT_POWER_SAVE_IDLE_TIME control code.
 *
 *********************************************************************/
typedef struct fbe_base_config_set_object_idle_time_s
{
   /*! how many seconds to wait before saving power */
    fbe_u64_t   power_save_idle_time_in_seconds;

} fbe_base_config_set_object_idle_time_t;

/*!*******************************************************************
 * @struct fbe_base_config_object_get_generation_number_t
 *********************************************************************
 * @brief
 *  This structure is used with the
 *  FBE_BASE_CONFIG_CONTROL_CODE_GET_GENERATION_NUMBER control code.
 *
 *********************************************************************/
typedef struct fbe_base_config_object_get_generation_number_s
{
       /*! Generation number. */
    fbe_config_generation_t generation_number; 

} fbe_base_config_object_get_generation_number_t;

fbe_object_id_t fbe_base_config_metadata_element_to_object_id(struct fbe_metadata_element_s * metadata_element);
fbe_class_id_t fbe_base_config_metadata_element_to_class_id(struct fbe_metadata_element_s *metadata_element);
fbe_bool_t fbe_base_config_metadata_element_is_nonpaged_initialized(struct fbe_metadata_element_s * metadata_element);
fbe_bool_t fbe_base_config_metadata_element_is_nonpaged_state_initialized(struct fbe_metadata_element_s * metadata_element);

/* FBE_BASE_CONFIG_CONTROL_CODE_GET_METADATA_STATISTICS */
typedef struct fbe_base_config_control_get_metadata_statistics_s{
    fbe_u64_t           stripe_lock_count;
    fbe_u64_t           local_collision_count;
    fbe_u64_t           peer_collision_count;
    fbe_u64_t           cmi_message_count;
}fbe_base_config_control_get_metadata_statistics_t;


/*!*******************************************************************
 * @struct fbe_base_config_drive_to_mark_nr_s
 *********************************************************************
 * @brief   This structure is used with the following control codes
 *          o   FBE_RAID_GROUP_CONTROL_CODE_MARK_NR
 *
 *********************************************************************/
typedef struct fbe_base_config_drive_to_mark_nr_s
{
    /*! Object id of the drive to mark NR
    */
    fbe_object_id_t      pvd_object_id;

    /*! VD Object id of the drive to mark NR
    */
    fbe_object_id_t      vd_object_id;

    /*! Boolean to indicate if it is force NR for recovery purposes
     */
    fbe_bool_t          force_nr;

    /*! boolean flag to indicate whether marking of NR was successful or not
    */
    fbe_bool_t           nr_status;

}fbe_base_config_drive_to_mark_nr_t;

/*!******************************************************************
 * @typedef fbe_base_config_background_operation_t
 ********************************************************************
 * @brief Defines a bitmask of background operations for the object.
 *        When set to TRUE, signifies that the background operation is
 *        disabled.
 *
 *        Bitwise Truth table:
 *          Bit value -  Detail
 *          1         -  Background operation is disabled
 *          0         -  Background operation is enabled(By default value)
 * 
 *********************************************************************/
typedef fbe_u16_t  fbe_base_config_background_operation_t;

/* FBE_BASE_CONFIG_CONTROL_CODE_BACKGROUND_OPERATION */
typedef struct fbe_base_config_control_background_op_s {
    fbe_base_config_background_operation_t background_operation;
    /* setting 'enable' to FBE_FALSE will disable background operations for given object, setting to FBE_TRUE will let
     * background operation to run which is enabled to run as usual. 
     */
    fbe_bool_t      enable; /*INPUT */
}fbe_base_config_control_background_op_t;

/* FBE_BASE_CONFIG_CONTROL_CODE_BACKGROUND_OPERATION_ENABLE_CHECK */
typedef struct fbe_base_config_background_op_enable_check_s {
    fbe_base_config_background_operation_t background_operation;
    /* When returns FBE_TRUE, signifies that the background operation is enabled otherwise it is disabled
     */
    fbe_bool_t is_enabled; /* OUTPUT */
}fbe_base_config_background_op_enable_check_t;

/* for non-paged metadata cloning */
fbe_status_t fbe_base_config_class_set_nonpaged_metadata(fbe_generation_code_t generation_number,
                                                         fbe_object_id_t       object_id, 
                                                         fbe_u8_t *            nonpaged_data_p);


/* Metadata memory structure populated in an object's metadata memory */
typedef struct fbe_base_config_metadata_memory_s{
    /*version header defines the valid size of the versioned
      metadata memory. All the metadata memory data structures
      should carry this header to handle the version difference on CMI.
      It MUST be at the begining of each metadata memory*/
    fbe_sep_version_header_t    version_header;

    fbe_lifecycle_state_t               lifecycle_state;
    fbe_power_save_state_t              power_save_state;
    fbe_time_t                          last_io_time;
    fbe_base_config_clustered_flags_t   flags;

}fbe_base_config_metadata_memory_t;


#pragma pack(1)


/*!********************************************************************* 
 * @enum fbe_base_config_nonpaged_metadata_state_t
 *  
 * @brief 
 * This enumeration lists all the possible base config nonpaged metadata state.
 *
 **********************************************************************/
enum fbe_base_config_nonpaged_metadata_state_e
{
    FBE_NONPAGED_METADATA_UNINITIALIZED,
    FBE_NONPAGED_METADATA_INITIALIZED,
    FBE_NONPAGED_METADATA_INVALID
};
typedef fbe_u32_t fbe_base_config_nonpaged_metadata_state_t;


/* non paged metadata for the base config. */
typedef struct fbe_base_config_nonpaged_metadata_s
{
    /*version header defines the valid size of the versioned 
      non-paged metadata. All the non-paged metadata data structures
      should carry this header to handle the version difference. It
      MUST be at the begining of each non-paged metadata*/
    fbe_sep_version_header_t    version_header;

    fbe_generation_code_t   generation_number;
    fbe_object_id_t         object_id;
    
    /* Add commong nonpaged metadata fields here. */
    /* state of base config nonpaged metadata .
     1) UNINITIALIZED means the non-paged metadata is not initialized yet.
          It will be set when zero the nonpaged MD when MD finds the object NP is not initialize yet
        2) INTIALIZED means the non-paged metadata is already initialized and has valid data. 
        3) INVALID means that metadata service failed to load it from disk*/
    fbe_base_config_nonpaged_metadata_state_t  nonpaged_metadata_state;

    /* bitmask of background operations to enable/disable it */
    fbe_base_config_background_operation_t operation_bitmask;
} fbe_base_config_nonpaged_metadata_t;
#pragma pack()



/*! @enum fbe_base_config_control_system_bg_service_flags_e
 *  
 *  @brief flags to control system background services of the base config object.
 */
enum fbe_base_config_control_system_bg_service_flags_e {
    FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_NONE               = 0x0000000000000000ULL,
    FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_SNIFF_VERIFY       = 0x0000000000000001ULL,
    FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_ZEROING            = 0x0000000000000002ULL,
    FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_REBUILD            = 0x0000000000000004ULL,
    FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_VERIFY             = 0x0000000000000008ULL,
    FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_USER_VERIFY        = 0x0000000000000010ULL,    
    FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_ZERO_CHKPT_UPDATE  = 0x0000000000000020ULL,    
    FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_ENCRYPTION_REKEY   = 0x0000000000000040ULL,    
    /* Please make sure to add any new bits to _ENABLE_ALL below */

    FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_ALL  = (FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_SNIFF_VERIFY |
                                                             FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_ZEROING |
                                                             FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_REBUILD |
                                                             FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_VERIFY |
                                                             FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_USER_VERIFY |
                                                             FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_ZERO_CHKPT_UPDATE |
                                                             FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_ENCRYPTION_REKEY),

     FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_NDU  = (FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_SNIFF_VERIFY |
                                                             FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_ZEROING |
                                                             FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_VERIFY |
                                                             FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_USER_VERIFY |
                                                             FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_ZERO_CHKPT_UPDATE|
                                                             FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_ENCRYPTION_REKEY),

    /* This is the initial set database enables first.
     */
    FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_INITIAL = (FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_REBUILD           |   
                                                                FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_VERIFY            |   
                                                                FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_USER_VERIFY       |   
                                                                FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_ZERO_CHKPT_UPDATE |
                                                                FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_ENCRYPTION_REKEY  ),
};
typedef fbe_u64_t fbe_base_config_control_system_bg_service_flags_t;

/*FBE_BASE_CONFIG_CONTROL_CODE_CONTROL_SYSTEM_BG_SERVICE*/
typedef struct fbe_base_config_control_system_bg_service_s{
    /*setting 'enable' to FBE_FALSE will disable background service specified in the bgs_flag, 
      setting to FBE_TRUE will enable bg service.*/
    fbe_bool_t      enable;
    fbe_base_config_control_system_bg_service_flags_t bgs_flag;
    fbe_bool_t 		enabled_externally;  /* The enable flag is per service, so we need one filed for the get part that tells us this was enabled by management side at one point */
    fbe_bool_t 		issued_by_ndu;       /* was this issued by NDU? We need to know that in order to ask for the scheduler to keep an eye on the time that passed */
    fbe_bool_t          issued_by_scheduler; /* was this issued by the scheduler? If so, we need to reset everything. */
    fbe_base_config_control_system_bg_service_flags_t base_config_control_system_bg_service_flag;
}fbe_base_config_control_system_bg_service_t;
/*!*******************************************************************
 * @struct fbe_base_config_write_verify_info_s
 *********************************************************************
 * @brief
 *  This structure is used to record system object's write_verify infomation.
 *
 *********************************************************************/
typedef struct fbe_base_config_write_verify_info_s
{
    fbe_bool_t           write_verify_op_success_b;
    fbe_bool_t           write_verify_operation_retryable_b;
    fbe_bool_t           need_clear_rebuild_count;
}fbe_base_config_write_verify_info_t;

/* FBE_BASE_CONFIG_CONTROL_CODE_GET_STRIPE_BLOB */
typedef struct fbe_base_config_control_get_stripe_blob_s{
	fbe_u32_t size; 
	fbe_u32_t write_count;
	fbe_u64_t user_slot_size;
	fbe_u64_t private_slot;
	fbe_u64_t nonpaged_slot;
	fbe_metadata_lock_blob_flags_t flags;
	fbe_u8_t  slot[FBE_MEMORY_CHUNK_SIZE]; /* MUST be LAST */
}fbe_base_config_control_get_stripe_blob_t;

/* FBE_BASE_CONFIG_CONTROL_CODE_GET_PHYSICAL_DRIVE_LOCATION */
typedef struct fbe_base_config_physical_drive_location_s {
    fbe_u32_t   port_number; 
    fbe_u32_t   enclosure_number; 
    fbe_u32_t   slot_number;
}fbe_base_config_physical_drive_location_t;

/*FBE_BASE_CONFIG_CONTROL_CODE_GET_PEER_LIFECYCLE_STATE*/
typedef struct fbe_base_config_get_peer_lifecycle_state_s {
    fbe_lifecycle_state_t peer_state;
}fbe_base_config_get_peer_lifecycle_state_t;

/*!*******************************************************************
 * @struct fbe_base_config_init_key_handle_s
 *********************************************************************
 * @brief
 *  This structure is used to pass the handles to the key for each object.
 *
 *  @notes - We dont pass the keys around but just the handle for security
 *  purposes.
 *
 *********************************************************************/
/*FBE_BASE_CONFIG_CONTROL_CODE_INIT_KEY_HANDLE */
typedef struct fbe_base_config_init_key_handle_s {
    fbe_encryption_setup_encryption_keys_t *key_handle;
}fbe_base_config_init_key_handle_t;

/*FBE_BASE_CONFIG_CONTROL_CODE_UPDATE_DRIVE_KEYS */
typedef struct fbe_base_config_update_keys_s {
    fbe_encryption_setup_encryption_keys_t *key_handle;
    fbe_u32_t mask;
}fbe_base_config_update_drive_keys_t;

fbe_status_t fbe_base_config_enable_system_background_zeroing(void);
fbe_status_t fbe_base_config_control_system_bg_service(fbe_base_config_control_system_bg_service_t *system_bg_service);
void fbe_base_config_set_load_balance(fbe_bool_t is_enabled);
fbe_bool_t fbe_base_config_all_bgs_started_externally(void);
fbe_bool_t fbe_base_config_is_object_encryption_state_locked(fbe_base_config_encryption_state_t encryption_state);




#endif /* FBE_BASE_CONFIG_H */



/*************************
 * end file fbe_base_config.h
 *************************/
