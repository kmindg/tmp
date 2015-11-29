#ifndef FBE_DATABASE_PRIVATE_H
#define FBE_DATABASE_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_database_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains local definitions for the database service.
 *
 * @version
 *   12/15/2010:  Created. 
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_database.h"
#include "fbe_database_common.h"
#include "fbe_database_persist_interface.h"

#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_provision_drive.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe/fbe_raid_group.h"
#include "fbe/fbe_lun.h"
#include "fbe/fbe_extent_pool.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe_job_service.h"

#include "fbe_base_service.h"
#include "fbe_base_object.h"
#include "fbe_block_transport.h"
#include "fbe_imaging_structures.h"
#include "fbe_fru_descriptor_structure.h"
#include "fbe_fru_signature.h"

#include "fbe_database_homewrecker_fru_descriptor.h"
#include "fbe_database_fru_signature_interface.h"
#include "fbe_private_space_layout.h"
#include "fbe/fbe_environment_limit.h"
#include "fbe/fbe_board.h"
#include "fbe/fbe_esp_encl_mgmt.h"

/*! @def FBE_DATABASE_... 
 *  
 *  @brief This is the ...
 */
#define FBE_DB_NUMBER_OF_RESOURCES_FOR_ICA_OPERATION 3 /*we need 3 packets and 3 sgl*/

#define FBE_DB_NUMBER_OF_RESOURCES_FOR_HOMEWRECKER_OPERATION 3 /* Homewrecker need to check first 3 drvies */

#define DATABASE_NUM_SYS_DRIVES FBE_PRIVATE_SPACE_LAYOUT_MAX_FRUS_PER_REGION

#define DATABASE_WAIT_FOR_OBJECT_STATE_TIME  240000 /* increase to wait for 240 seconds */

#define FBE_DB_FAILED_RAW_IO_RETRIES           200  /*we need 200 retries in case of IO failure
                                                     * hope 200 second retry is enough, otherwise,
                                                     * we need to increase this again.
                                                     */
#define FBE_DB_FAILED_RAW_IO_RETRY_INTERVAL    1000 /*wait 1 second before retrying*/

#define FBE_DATABASE_CONTIGUOUS_POLL_TIMER_SECONDS   2 /* lun, rg, and pvd poll timer as part of a single full poll */
#define FBE_DATABASE_MAX_FULL_POLL_TIMER_SECONDS     3600 /* max seconds for FBE_DATABASE_MAX_POLL_RECORDS full polls */
#define FBE_DATABASE_POLL_REG_KEY                    "DBPollDetect" /* registry to determine whether or not to record polls */
#define FBE_DATABASE_DEFAULT_IS_ACTIVE               1 /* Default action is to allow full poll statistics */
#define FBE_DATABASE_EXTENDED_CACHE_KEY              "ExtendedCacheEnabled" /* registry to analyze vault drive tier */
#define FBE_DATABASE_EXTENDED_CACHE_ENABLED          1 /* Default action in sim is to shoot vault drive if drive tier is incompatible */

/* 
 * 09-10-15:
 *   Change to 0x1 to set drive type support to LE bitmask defined in fbe_database_additional_drive_types_supported_t
 */
#define FBE_DATABASE_ADDITIONAL_DRIVE_TYPE_SUPPORTED_DEFAULT   (FBE_DATABASE_DRIVE_TYPE_SUPPORTED_LE)   /* default drive types supported if reg key not present */

typedef enum fbe_database_thread_flag_e {
    FBE_DATABASE_THREAD_FLAG_INVALID,   /*!< 0x00 No flags are set. */
    FBE_DATABASE_THREAD_FLAG_RUN,       /*!< 0x01 running thread.   */
    FBE_DATABASE_THREAD_FLAG_STOP,      /*!< 0x02 stop thread       */
    FBE_DATABASE_THREAD_FLAG_DONE,      /*!< 0x03 Thread is done. */
    FBE_DATABASE_THREAD_FLAG_LAST
} fbe_database_thread_flag_t;

typedef enum pvd_operation_thread_flag_e{
    PVD_OPERATION_THREAD_INVALID,
    PVD_OPERATION_THREAD_RUN,
    PVD_OPERATION_THREAD_WAIT_RUN,
    PVD_OPERATION_THREAD_STOP,
    PVD_OPERATION_THREAD_DONE
}pvd_operation_thread_flag_t;

typedef struct fbe_database_key_entry_s{
    fbe_queue_element_t queue_element;
    fbe_encryption_setup_encryption_keys_t key_info;
}fbe_database_key_entry_t;

/*!*******************************************************************
 * @struct database_pvd_operation_t
 *********************************************************************
 * @brief
 *  all the stuff related to pvd connect.
 *
 *********************************************************************/
typedef struct database_pvd_operation_s {
    database_table_t            *object_table_ptr;
    fbe_queue_head_t            connect_queue_head;
    fbe_queue_head_t            discover_queue_head;
    fbe_queue_head_t            free_entry_queue_head;
    fbe_spinlock_t              discover_queue_lock;  /* protecting discover requests/resource */
    fbe_semaphore_t             pvd_operation_thread_semaphore;
    pvd_operation_thread_flag_t pvd_operation_thread_flag;
    fbe_homewrecker_fru_descriptor_t system_fru_descriptor;
    fbe_spinlock_t  system_fru_descriptor_lock;
}database_pvd_operation_t;

typedef struct database_physical_drive_info_s
{
    fbe_u32_t port_number; /* IN */
    fbe_u32_t enclosure_number; /* IN */
    fbe_u32_t slot_number; /* IN */
    fbe_block_edge_geometry_t block_geometry; /* IN */
    fbe_object_id_t logical_drive_object_id; /* OUT */
}database_physical_drive_info_t;

/* structs used by Homewrecker */
typedef enum fbe_homewrecker_system_disk_type_e{
    FBE_HW_DISK_TYPE_INVALID    = 0x0,
    FBE_HW_NO_DISK  = 0x1,
    FBE_HW_NEW_DISK = 0x2,
    FBE_HW_OTHER_ARR_SYS_DISK   = 0x3,
    FBE_HW_OTHER_ARR_USR_DISK   = 0x4,
    FBE_HW_CUR_ARR_USR_DISK     = 0x5,
    FBE_HW_CUR_ARR_SYS_DISK     = 0x6,
    FBE_HW_DISK_TYPE_UNKNOWN    = 0xFF,
}fbe_homewrecker_system_disk_type_t;


typedef struct{
    fbe_homewrecker_fru_descriptor_t    dp;
    fbe_homewrecker_get_raw_mirror_info_t   dp_report;
    fbe_fru_signature_t     sig;
    fbe_homewrecker_signature_report_t  sig_report;
    fbe_bool_t  path_enable;
    fbe_homewrecker_system_disk_type_t  disk_type;
    fbe_bool_t  is_invalid_system_drive;
    /*The (bus encl and slot) records where this system drive really loacated.*/
    fbe_u32_t actual_bus;
    fbe_u32_t actual_encl;
    fbe_u32_t actual_slot;
}homewrecker_system_disk_table_t;

typedef enum homewrecker_selector_e{
    HOMEWRECKER_SELECTOR_START,
    HOMEWRECER_DISK_DESCRIPTOR = HOMEWRECKER_SELECTOR_START,
    HOMEWRECER_DISK_SIGNATURE,
    HOMEWRECER_SELECTOR_END = HOMEWRECER_DISK_SIGNATURE
}homewrecker_selector_t;

/*figer out which kind of slot does the disk in*/
typedef enum homewrecker_location_e{
    HOMEWRECER_LOCATION_UNKNOW,
    HOMEWRECER_IN_USR_SLOT,
    HOMEWRECER_IN_SYS_SLOT
}homewrecker_location_t;

typedef enum homewrecker_service_mode_type_e{
    CHASSIS_MISMATCHED_ERROR,
    SYS_DRIVES_DISORDERED_ERROR,
    SYS_DRIVES_INTEGRALITY_BROKEN,
    SYS_DRIVES_DOUBLE_INVALID_WITH_SYSTEM_DRIVE_IN_USER_SLOT,
    OPERATION_ON_WWNSEED_CHAOS,
    NO_SERVICE_MODE,
	BOOTFLASH_SERVICE_MODE
}homewrecker_service_mode_type_t;

typedef struct homewrecker_boot_with_event_s{
    fbe_u32_t   slot;
    fbe_char_t  *event_str;
}homewrecker_boot_with_event_t;

typedef struct homewrecker_system_integrity_event_s{
    fbe_char_t* slot_type[FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER];
}homewrecker_system_integrity_event_t;

typedef struct homewrecker_disk_in_wrong_slot_event_s{
    fbe_object_id_t disk_pvd_id;
    fbe_u32_t bus;
    fbe_u32_t enclosure;
    fbe_u32_t slot;
    fbe_u32_t its_correct_bus;
    fbe_u32_t its_correct_encl;
    fbe_u32_t its_correct_slot;
}homewrecker_disk_in_wrong_slot_event_t;

typedef union{
	homewrecker_boot_with_event_t* normal_boot_with_warning;
	fbe_u32_t*			disordered_slot;
	homewrecker_system_integrity_event_t*   integrity_disk_type;
         homewrecker_disk_in_wrong_slot_event_t* disk_in_wrong_slot;      
}event_log_param_selector_t;

/* a poll record for get_all_luns, get_all_raid_groups, and get_all_pvds */
typedef struct fbe_database_poll_record_s{
    fbe_database_poll_request_t poll_request_bitmap;
    fbe_time_t                  time_of_poll;
}fbe_database_poll_record_t;

/* Circular buffer for poll requests */
typedef struct database_poll_ring_buffer_s{
    fbe_spinlock_t                  lock; /* a lock for editing the records in the ring buffer */
    fbe_bool_t                      is_active; /* whether or not to record poll statistics */
    fbe_u32_t                       threshold_count; /* number of times FBE_DATABASE_MAX_POLL_RECORDS was hit */
    fbe_u32_t                       start;  /* index of oldest element */
    fbe_u32_t                       end;    /* index at which to write new element */
    fbe_database_poll_record_t      records[FBE_DATABASE_MAX_POLL_RECORDS];  /* vector of elements */
}database_poll_ring_buffer_t;


typedef enum fbe_database_port_encryption_op_e{
    FBE_DATABASE_PORT_ENCRYPTION_OP_INVALID,
    FBE_DATABASE_PORT_ENCRYPTION_OP_MODE_SET,
    FBE_DATABASE_PORT_ENCRYPTION_OP_MODE_GET,
    FBE_DATABASE_PORT_ENCRYPTION_OP_LAST,
}fbe_database_port_encryption_op_t;

/*!*******************************************************************
 * @struct database_midplane_info_t
 *********************************************************************
 * @brief
 *  This structure is used to track the chassis number and wwn seed
 *  in the system.
 *
 *********************************************************************/
typedef struct database_midplane_info_s
{
    fbe_char_t        emc_part_number[FBE_EMC_PART_NUM_SIZE];
    fbe_char_t        emc_revision[FBE_EMC_ASSEMBLY_REV_SIZE];
    fbe_u8_t          emc_serial_num[FBE_EMC_SERIAL_NUM_SIZE];
    fbe_u32_t         world_wide_name;
}database_midplane_info_t;

/*!*******************************************************************
 * @enum fbe_database_cmi_state_e
 *********************************************************************
 * @brief
 *  This is the previous state of database when becoming ready
 *
 *********************************************************************/
typedef enum fbe_database_cmi_state_e {
    FBE_DATABASE_CMI_STATE_INVALID,
    FBE_DATABASE_CMI_STATE_ACTIVE,
    FBE_DATABASE_CMI_STATE_PASSIVE,
} fbe_database_cmi_state_t;

/*!*******************************************************************
 * @struct fbe_database_service_t
 *********************************************************************
 * @brief
 *  This is the definition of the database service
 *
 *********************************************************************/
typedef struct fbe_database_service_s {
    fbe_base_service_t        base_service;  /* base service must be the first */

    /************************************ 
     * database service private members *
     ************************************/
    /* the state of the service */
    fbe_database_state_t      state;

    /* the state of the peer service */
    fbe_database_state_t      peer_state;

    /* previouse cmi state of database before ready
     */
    fbe_database_cmi_state_t  prev_cmi_state;

    /* config_thread creates objects, edges */
    fbe_thread_t              config_thread_handle;

    /* drive_monitor_thread waits on pdo notification*/
    fbe_thread_t              drive_monitor_thread_handle;

    /* In-memory tables */
    database_table_t          user_table;
    database_table_t          object_table;
    database_table_t          edge_table;
    database_table_t          system_spare_table;


    /* in-memory system settings */
    database_table_t          global_info;

    /* in-memory system db header */
    database_system_db_header_t system_db_header;
    fbe_u64_t        peer_sep_version;
    fbe_spinlock_t  system_db_header_lock;
    fbe_bool_t      use_transient_commit_level;
    fbe_u64_t       transient_commit_level;

    fbe_database_emv_t  shared_expected_memory_info;  /*shared expected memory info, can only be set and persist by functions in fbe_database_emv_process.c*/

    /* pvd operation context */
    database_pvd_operation_t  pvd_operation;

    /* transaction */
    database_transaction_t    *transaction_ptr;
    database_transaction_t    *transaction_backup_ptr;
    fbe_u32_t                  transaction_alloc_size;
    fbe_u64_t                  transaction_peer_physical_address;

    /*a pointer to the job packet which will do the peer update. it can be completed in different contexts so we must keep it here*/
    fbe_packet_t *                peer_config_update_job_packet;

    /*how many tabele entrries the active side sent*/
    volatile fbe_u64_t            updates_sent_by_active_side;
    
    /*how many tabele entries we got*/
    volatile fbe_u64_t            updates_received; 

    /*what are the limits in the system*/
    fbe_environment_limits_t    logical_objects_limits;

    /*how many objects we can have*/
    fbe_u32_t     max_configurable_objects;

    /* Midplane prom info */
    database_midplane_info_t    prom_info;

    /* database maitenance reason */
    fbe_database_service_mode_reason_t service_mode_reason;

    /* Mark the SP state when db state becomes ready.
        * Be used to distinguish the case that passive SP is promoted to active SP by lost peer event.
        */
    fbe_bool_t  db_become_ready_in_passiveSP;

    fbe_u32_t smallest_psl_rg_number;/*used for generating rg numbers*/
	fbe_u32_t smallest_psl_lun_number;/*used for generating lun numbers*/
	fbe_u64_t *free_lun_number_bitmap;
	fbe_u64_t *free_rg_number_bitmap;

    /* ring buffer to keep track of all poll requests */
    database_poll_ring_buffer_t poll_ring_buffer;

    fbe_encryption_backup_state_t encryption_backup_state;

    FBE_TRI_STATE journal_replayed;

    fbe_database_debug_flags_t  db_debug_flags;

    fbe_u32_t forced_garbage_collection_debounce_timeout;

    fbe_database_additional_drive_types_supported_t supported_drive_types;

}fbe_database_service_t;

static __forceinline fbe_bool_t fbe_database_is_drive_type_le_supported(fbe_database_service_t *database_p)
{
    return (database_p->supported_drive_types & FBE_DATABASE_DRIVE_TYPE_SUPPORTED_LE) ? FBE_TRUE : FBE_FALSE;
}

static __forceinline fbe_bool_t fbe_database_is_drive_type_ri_supported(fbe_database_service_t *database_p)
{
    return (database_p->supported_drive_types & FBE_DATABASE_DRIVE_TYPE_SUPPORTED_RI) ? FBE_TRUE : FBE_FALSE;
}

static __forceinline fbe_bool_t fbe_database_is_drive_type_520_hdd_supported(fbe_database_service_t *database_p)
{
    return (database_p->supported_drive_types & FBE_DATABASE_DRIVE_TYPE_SUPPORTED_520_HDD) ? FBE_TRUE : FBE_FALSE;
}

static __forceinline fbe_bool_t fbe_database_is_debug_flag_set(fbe_database_service_t *database_p,
                                                               fbe_database_debug_flags_t debug_flags)
{
    return ((database_p->db_debug_flags & debug_flags) == debug_flags);
}
static __forceinline void fbe_database_get_debug_flags(fbe_database_service_t *database_p,
                                                       fbe_database_debug_flags_t *debug_flags_p)
{
    *debug_flags_p = database_p->db_debug_flags;
    return;
}
static __forceinline void fbe_database_set_debug_flags(fbe_database_service_t *database_p,
                                                       fbe_database_debug_flags_t debug_flags)
{
    database_p->db_debug_flags = debug_flags;
    return;
}
static __forceinline void fbe_database_set_debug_flag(fbe_database_service_t *database_p,
                                                       fbe_database_debug_flags_t debug_flags)
{
    database_p->db_debug_flags |= debug_flags;
}
static __forceinline void fbe_database_clear_debug_flag(fbe_database_service_t *database_p,
                                                          fbe_database_debug_flags_t debug_flags)
{
    database_p->db_debug_flags &= ~debug_flags;
    return;
}

/*function which is used for validate the read flags, used in fbe_database_read_flags_from_raw_drive*/
typedef fbe_status_t (*fbe_database_validate_read_flags_funcion)(fbe_u8_t *flags_array, fbe_bool_t all_drives_read);

typedef struct database_transaction_commit_context_s
{
    fbe_semaphore_t commit_semaphore;
    fbe_status_t        commit_status;
}database_transaction_commit_context_t;

typedef struct database_ndu_commit_context_s
{
    fbe_job_service_sep_ndu_commit_t ndu_commit_job_request;  /*the job request of the ndu commit job*/
    fbe_semaphore_t commit_semaphore;   /*the semaphore that the ndu commit request (database api) would wait*/
    fbe_status_t        commit_status;    /*the result status of the ndu commit*/
}database_ndu_commit_context_t;


typedef enum fbe_database_cmi_thread_event_e {
    FBE_DB_CMI_EVENT_CONFIG_DONE                = 0x00000001,       /* Peer has send all configuration */
} fbe_database_cmi_thread_event_t;

typedef struct database_init_read_context_s{
    fbe_semaphore_t sem;
    fbe_persist_entry_id_t    next_entry_id;
    fbe_u32_t                entries_read;
    fbe_status_t            status;
}database_init_read_context_t;

typedef struct database_validate_entry_s{
    fbe_bool_t  b_entry_found;
}database_validate_entry_t;

typedef struct database_validate_table_s {
    database_table_size_t       table_size;
    database_validate_entry_t  *validate_entry_p;
}database_validate_table_t;

/* Send a event to database cmi thread */
fbe_status_t fbe_database_cmi_thread_set_event(fbe_u64_t event);

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
void database_trace_at_startup(fbe_trace_level_t trace_level,
                               fbe_trace_message_id_t message_id,
                               const char * fmt, ...)
                               __attribute__((format(__printf_func__,3,4)));
void database_trace(fbe_trace_level_t trace_level,
                    fbe_trace_message_id_t message_id,
                    const char * fmt, ...)
                    __attribute__((format(__printf_func__,3,4)));

void set_database_service_state(fbe_database_service_t *fbe_database_service, 
                                fbe_database_state_t state);

static __inline void get_database_service_state(fbe_database_service_t *fbe_database_service, 
                                fbe_database_state_t *state)
{
    *state = fbe_database_service->state;
}

static __inline fbe_bool_t database_has_system_object_to_be_recreated(fbe_database_system_object_recreate_flags_t *recreate_flags)
{
    return (recreate_flags->valid_num != 0);
}
void set_database_service_peer_state(fbe_database_service_t *database_service_ptr, 
                                     fbe_database_state_t peer_state);

void get_database_service_peer_state(fbe_database_service_t *database_service_ptr, 
                                     fbe_database_state_t *peer_state);

fbe_bool_t is_peer_config_synch_allowed(fbe_database_service_t *database_service_ptr);
fbe_bool_t is_peer_update_allowed(fbe_database_service_t *database_service_ptr);

fbe_status_t fbe_database_get_service_table_ptr(database_table_t ** table_ptr, database_config_table_type_t table_type);
fbe_status_t database_get_pvd_operation(database_pvd_operation_t **pvd_operation);

fbe_status_t fbe_database_get_logical_objects_limits(fbe_environment_limits_t *logical_objects_limits);

fbe_status_t fbe_database_drive_discover(database_pvd_operation_t *pvd_operation);
fbe_status_t fbe_database_drive_connection_init(fbe_database_service_t *database_service);
fbe_status_t fbe_database_drive_connection_destroy(fbe_database_service_t *database_service);
fbe_status_t fbe_database_init_lu_and_rg_number_generation(fbe_database_service_t *database_service_ptr);
fbe_status_t fbe_database_destroy_lu_and_rg_number_generation(fbe_database_service_t *database_service_ptr);

/* transaction */
fbe_status_t fbe_database_allocate_transaction(fbe_database_service_t *fbe_database_service);
fbe_status_t fbe_database_free_transaction(fbe_database_service_t *fbe_database_service);


fbe_status_t fbe_database_transaction_init(database_transaction_t *transaction);
fbe_status_t fbe_database_transaction_destroy(database_transaction_t *transaction);
fbe_status_t fbe_database_transaction_start(fbe_database_service_t *fbe_database_service, fbe_database_transaction_info_t *transaction_info);
fbe_status_t fbe_database_transaction_commit(fbe_database_service_t *fbe_database_service);
fbe_status_t fbe_database_transaction_abort(fbe_database_service_t *database_service);
fbe_status_t fbe_database_transaction_rollback(fbe_database_service_t *database_service);
fbe_status_t fbe_database_process_incomplete_transaction_after_peer_died(fbe_database_service_t *database_service);
fbe_status_t fbe_database_transaction_commit_after_peer_died(fbe_database_service_t *fbe_database_service);


fbe_status_t fbe_database_transaction_get_id(database_transaction_t *transaction,
                                             fbe_database_transaction_id_t *transaction_id);

fbe_status_t fbe_database_transaction_get_free_object_entry(database_transaction_t *transaction, 
                                                       database_object_entry_t **out_entry_ptr);
fbe_status_t fbe_database_transaction_add_object_entry(database_transaction_t *transaction, 
                                                       database_object_entry_t *in_entry_ptr);

fbe_status_t fbe_database_transaction_get_free_user_entry(database_transaction_t *transaction, 
                                                       database_user_entry_t **out_entry_ptr);
fbe_status_t fbe_database_transaction_add_user_entry(database_transaction_t *transaction, 
                                                       database_user_entry_t *in_entry_ptr);

fbe_status_t fbe_database_transaction_get_free_edge_entry(database_transaction_t *transaction, 
                                                       database_edge_entry_t **out_entry_ptr);
fbe_status_t fbe_database_transaction_add_edge_entry(database_transaction_t *transaction, 
                                                       database_edge_entry_t *in_entry_ptr);

fbe_status_t fbe_database_transaction_get_free_global_info_entry(database_transaction_t *transaction, 
                                                                 database_global_info_entry_t **out_entry_ptr);

fbe_status_t fbe_database_transaction_add_global_info_entry(database_transaction_t *transaction, 
                                                            database_global_info_entry_t *in_entry_ptr);

fbe_status_t fbe_database_get_user_capacity_limit(fbe_u32_t *capacity_limit);

fbe_status_t fbe_database_transaction_is_valid_id(fbe_database_transaction_id_t transaction_id, 
                                                  fbe_database_transaction_id_t db_transaction_id);
fbe_config_generation_t fbe_database_transaction_get_next_generation_id(fbe_database_service_t *database_service);
fbe_u32_t   fbe_database_transaction_get_num_create_pvds(database_transaction_t *transaction);

fbe_status_t database_update_encryption_mode(fbe_database_control_update_encryption_mode_t  *update_encrption_mode);
fbe_status_t database_update_pvd(fbe_database_control_update_pvd_t  *update_pvd);
fbe_status_t database_update_pvd_block_size(fbe_database_control_update_pvd_block_size_t  *update_pvd);
fbe_status_t database_create_pvd(fbe_database_control_pvd_t * pvd );
fbe_status_t database_destroy_pvd(fbe_database_control_destroy_object_t *pvd, fbe_bool_t confirm_peer);
fbe_status_t database_create_vd(fbe_database_control_vd_t * vd);
fbe_status_t database_update_vd(fbe_database_control_update_vd_t *update_vd);
fbe_status_t database_destroy_vd(fbe_database_control_destroy_object_t *vd, fbe_bool_t confirm_peer);
fbe_status_t database_create_raid(fbe_database_control_raid_t * create_raid);
fbe_status_t database_destroy_raid(fbe_database_control_destroy_object_t * destroy_raid, fbe_bool_t confirm_peer);
fbe_status_t database_update_raid(fbe_database_control_update_raid_t *update_raid);
fbe_status_t database_create_lun(fbe_database_control_lun_t* create_lun);
fbe_status_t database_destroy_lun(fbe_database_control_destroy_object_t * destroy_lun, fbe_bool_t confirm_peer);
fbe_status_t database_update_lun(fbe_database_control_update_lun_t *update_lun);
fbe_status_t database_clone_object(fbe_database_control_clone_object_t *clone_object);
fbe_status_t database_create_edge(fbe_database_control_create_edge_t * create_edge);
fbe_status_t database_destroy_edge(fbe_database_control_destroy_edge_t * destroy_edge);
fbe_status_t database_set_power_save(fbe_database_power_save_t * power_save);
fbe_status_t database_update_system_spare_config(fbe_database_control_update_system_spare_config_t * spare_config);
fbe_status_t database_set_system_encryption_mode(fbe_system_encryption_info_t);
fbe_status_t fbe_database_config_commit_global_info(database_table_t *in_table_ptr);
fbe_status_t fbe_database_config_commit_object_table(database_table_t *in_table_ptr);
fbe_status_t fbe_database_set_user_capacity_limit(fbe_u32_t capacity_limit);
fbe_status_t fbe_database_set_encryption_paused(fbe_bool_t encryption_paused);
fbe_status_t database_create_extent_pool(fbe_database_control_create_ext_pool_t * create_ext_pool);
fbe_status_t database_create_ext_pool_lun(fbe_database_control_create_ext_pool_lun_t * create_ext_pool_lun);
fbe_status_t database_destroy_extent_pool(fbe_database_control_destroy_object_t * destroy_pool, fbe_bool_t confirm_peer);
fbe_status_t database_destroy_ext_pool_lun(fbe_database_control_destroy_object_t * destroy_lun, fbe_bool_t confirm_peer);

/* table entry functions */
fbe_status_t database_common_init_user_entry(database_user_entry_t *in_entry_ptr);
fbe_status_t database_common_init_object_entry(database_object_entry_t *in_entry_ptr);
fbe_status_t database_common_init_edge_entry(database_edge_entry_t *in_entry_ptr);
fbe_status_t database_common_init_global_info_entry(database_global_info_entry_t *in_entry_ptr);
fbe_status_t database_common_init_system_spare_entry(database_system_spare_entry_t *in_entry_ptr);

fbe_persist_sector_type_t database_common_map_table_type_to_persist_sector(database_config_table_type_t table_type);


/* class id mapping functions */
database_class_id_t database_common_map_class_id_fbe_to_database(fbe_class_id_t fbe_class);
fbe_class_id_t database_common_map_class_id_database_to_fbe(database_class_id_t database_class);

fbe_status_t database_common_create_edge_from_table_entry(database_edge_entry_t *in_entry_ptr);
fbe_status_t database_common_destroy_edge_from_table_entry(database_edge_entry_t *in_table_ptr);
fbe_status_t database_common_create_edge_from_table(database_table_t *in_table_ptr, fbe_u32_t start_index, fbe_u32_t number_of_entries);

fbe_status_t database_common_create_global_info_from_table(database_global_info_entry_t *in_table_ptr, 
                                                           database_table_size_t size);
fbe_status_t database_common_update_global_info_from_table_entry(database_global_info_entry_t *in_table_ptr);

fbe_status_t database_common_create_object_from_table(database_table_t *in_table_ptr, fbe_u32_t start_index, fbe_u32_t number_of_entries);
fbe_status_t database_common_create_object_from_table_entry(database_object_entry_t *in_entry_ptr);
fbe_status_t database_common_destroy_object_from_table_entry(database_object_entry_t *in_entry_ptr);
fbe_status_t database_common_wait_destroy_object_complete(void);

fbe_status_t database_common_set_config_from_table(database_table_t *in_table_ptr, fbe_u32_t start_index, fbe_u32_t number_of_entries);
fbe_status_t database_common_set_config_from_table_entry(database_object_entry_t *in_entry_ptr);
fbe_status_t database_common_update_config_from_table_entry(database_object_entry_t *in_entry_ptr);
fbe_status_t database_common_update_encryption_mode_from_table_entry(database_object_entry_t *in_entry_ptr);
fbe_status_t database_common_commit_object_from_table(database_table_t *in_table_ptr, fbe_u32_t start_index, fbe_u32_t number_of_entries, fbe_bool_t is_initial_creation);

fbe_status_t database_common_connect_pvd_from_table(database_table_t *in_table_ptr, fbe_u32_t start_index, fbe_u32_t number_of_entries);

/* return the size of the data structure of the entry*/
fbe_u32_t database_common_object_entry_size(database_class_id_t class_id);
fbe_u32_t database_common_user_entry_size(database_class_id_t class_id);
fbe_u32_t database_common_edge_entry_size(void);
fbe_u32_t database_common_global_info_entry_size(database_global_info_type_t type);
fbe_u32_t database_common_system_spare_entry_size(void);

fbe_status_t database_common_get_committed_object_entry_size(database_class_id_t db_class_id, fbe_u32_t *size);
fbe_status_t database_common_get_committed_user_entry_size(database_class_id_t class_id, fbe_u32_t *size);
fbe_status_t database_common_get_committed_edge_entry_size(fbe_u32_t *size);
fbe_status_t database_common_get_committed_global_info_entry_size(database_global_info_type_t type, fbe_u32_t *size);

/* system object functions */
fbe_status_t database_system_objects_create_and_commit_system_spare_drives(void);
fbe_status_t database_system_objects_create_system_drives(fbe_database_service_t *fbe_database_service);
fbe_status_t database_system_objects_commit_system_drives(fbe_database_service_t *fbe_database_service, fbe_bool_t is_initial_create);
fbe_status_t fbe_database_switch_to_raw_mirror_object(void);

void update_system_pvd_to_vd_edge_table(database_table_t *in_edge_table_ptr, database_table_t * in_object_table_ptr);
fbe_status_t database_system_objects_create_sep_internal_objects_from_persist(fbe_database_service_t *fbe_database_service);
fbe_status_t database_system_objects_create_sep_internal_objects(fbe_database_service_t *fbe_database_service, fbe_bool_t invalidate);
fbe_status_t database_system_objects_create_export_objects_from_persist(fbe_database_service_t *fbe_database_service);
fbe_status_t database_system_objects_create_export_objects(fbe_database_service_t *fbe_database_service, fbe_bool_t invalidate);
fbe_status_t database_check_and_make_up_system_object_configuration(fbe_database_service_t *fbe_database_service, fbe_bool_t *some_object_missing);


fbe_status_t database_system_objects_get_db_lun_id(fbe_object_id_t *object_id);
fbe_lba_t database_system_objects_get_db_lun_capacity(void);

fbe_status_t database_system_objects_enumerate_objects(fbe_object_id_t *object_ids_p, 
                                                       fbe_u32_t result_list_size,
                                                       fbe_u32_t *num_objects_p);
fbe_status_t database_system_objects_get_count(fbe_u32_t *system_object_count);
fbe_status_t database_system_objects_get_expected_count(fbe_u32_t *system_object_count);
fbe_status_t database_system_objects_get_reserved_count(fbe_u32_t *system_object_count);
fbe_status_t database_system_objects_get_system_db_offset_capacity(fbe_lba_t *offset_p, 
                                                                   fbe_lba_t *rg_offset_p,
                                                                   fbe_block_count_t *capacity_p);

fbe_status_t database_system_objects_get_homewrecker_db_offset_capacity(fbe_lba_t *offset_p, fbe_block_count_t *capacity_p);

fbe_status_t fbe_database_system_objects_generate_config_for_parity_rg_and_lun_from_psl(fbe_bool_t ndb);
fbe_status_t fbe_database_system_objects_generate_config_for_system_object_and_persist(fbe_object_id_t object_id, fbe_bool_t invalidate, fbe_bool_t recreate);
fbe_status_t fbe_database_system_objects_generate_config_for_all_system_rg_and_lun(void);

void fbe_database_generate_wwn(fbe_assigned_wwid_t *wwn, fbe_object_id_t object_id, database_midplane_info_t *prom_info);

/* notification related functions */
fbe_status_t database_common_register_notification(fbe_notification_element_t *notification_element,
                                                   fbe_package_id_t            package_id);
fbe_status_t database_common_unregister_notification(fbe_notification_element_t *notification_element,
                                                     fbe_package_id_t            package_id);
fbe_status_t fbe_database_send_encryption_change_notification(database_encryption_notification_reason_t reason);

/* CMI related functions */
fbe_bool_t database_common_cmi_is_active(void);
fbe_bool_t database_common_peer_is_alive(void);
void fbe_database_initiate_update_db_on_peer(void);

/* boot related functions */
fbe_status_t fbe_database_init_persist(void);
fbe_status_t fbe_database_load_global_info_table(void);
fbe_status_t fbe_database_load_topology(fbe_database_service_t *database_service_ptr, fbe_bool_t load_from_disk);
fbe_bool_t fbe_database_system_db_interface_read_and_check_system_db_header(fbe_database_service_t *database_service_ptr,
                                                                            fbe_u32_t valid_db_drive_mask);
fbe_status_t fbe_database_system_create_topology(fbe_database_service_t *database_service);
fbe_status_t fbe_database_system_db_persist_entries(fbe_database_service_t *database_service_ptr);
fbe_status_t fbe_database_control_setup_encryption_key(fbe_packet_t * packet);
fbe_status_t fbe_database_control_setup_kek(fbe_packet_t * packet);
fbe_status_t fbe_database_control_destroy_kek(fbe_packet_t * packet);
fbe_status_t fbe_database_control_setup_encryption_rekey(fbe_packet_t * packet);
fbe_status_t fbe_database_control_setup_kek_kek(fbe_packet_t * packet);
fbe_status_t fbe_database_control_destroy_kek_kek(fbe_packet_t * packet);
fbe_status_t fbe_database_control_reestablish_kek_kek(fbe_packet_t * packet);
fbe_status_t fbe_database_control_remove_encryption_keys(fbe_packet_t * packet);
fbe_status_t fbe_database_control_set_port_encryption_mode(fbe_packet_t * packet);
fbe_status_t fbe_database_control_get_port_encryption_mode(fbe_packet_t * packet);
fbe_status_t fbe_database_control_get_system_encryption_info(fbe_packet_t * packet);
fbe_status_t fbe_database_control_get_system_encryption_progress(fbe_packet_t * packet);
fbe_status_t fbe_database_control_update_drive_keys(fbe_packet_t * packet);
fbe_status_t fbe_database_control_scrub_old_user_data(fbe_packet_t * packet);
fbe_status_t fbe_database_control_get_system_scrub_progress(fbe_packet_t * packet);
fbe_status_t fbe_database_control_set_poll_interval(fbe_packet_t * packet);
fbe_status_t fbe_database_shoot_drive(fbe_object_id_t pdo_object_id);
fbe_status_t fbe_database_send_kill_drive_request_to_peer(fbe_object_id_t pdo_object_id);
fbe_bool_t fbe_database_process_killed_drive_needed(fbe_object_id_t * pdo_id_p);

static  __forceinline fbe_bool_t database_system_object_entry_valid(database_table_header_t *header)
{
    return (header->state != DATABASE_CONFIG_ENTRY_INVALID);

}

/* fbe_database_main.c */
fbe_database_service_t *fbe_database_get_database_service(void);


/*The following functions are related to io on single raw drive during during system booting*/

fbe_status_t fbe_database_read_data_from_single_raw_drive(fbe_u8_t* data,
                                                                 fbe_u64_t  data_length,
                                                                 database_physical_drive_info_t* drive_location_info,
                                                                 fbe_private_space_layout_region_id_t pls_id);

fbe_status_t fbe_database_write_data_on_single_raw_drive(fbe_u8_t* data,
                                                                fbe_u64_t  data_length,
                                                                database_physical_drive_info_t* drive_location_info,
                                                                fbe_private_space_layout_region_id_t pls_id);
/*********************************************************************************/


/*The following functions are related to flag operations on the raw drives*/
fbe_status_t fbe_database_read_flags_from_raw_drive(database_physical_drive_info_t* drive_locations,
                                                    fbe_u32_t number_of_drives_for_operation,
                                                    fbe_private_space_layout_region_id_t  pls_id,
                                                    fbe_database_validate_read_flags_funcion validation_function,
                                                    fbe_bool_t force_validation);
fbe_status_t fbe_database_write_flags_on_raw_drive(database_physical_drive_info_t* drive_locations,
                                                          fbe_u32_t number_of_drives_for_operation,
                                                          fbe_private_space_layout_region_id_t  pls_id,
                                                          fbe_u8_t*  flag, fbe_u32_t flag_length);
fbe_status_t fbe_database_determine_block_count_for_io(database_physical_drive_info_t *database_drive_info_p,
                                                       fbe_u64_t data_length,
                                                       fbe_block_count_t *block_count_for_io_p);


/*********************************************************************************/

/*
fbe_status_t database_homewrecker_db_interface_general_read
                                            (fbe_u8_t *data_ptr, fbe_lba_t start_lba, 
                                             fbe_u64_t count, fbe_u64_t *done_count, 
                                             fbe_u16_t drive_mask,
                                             fbe_raid_verify_raw_mirror_errors_t * raw_mirror_error_report);


fbe_status_t database_homewrecker_db_interface_general_write
                                                        (fbe_u8_t *data_ptr, fbe_lba_t start_lba, fbe_u64_t count, 
                                                        fbe_u64_t *done_count, fbe_bool_t is_clear_write, 
                                                        fbe_raid_verify_raw_mirror_errors_t * raw_mirror_error_report);
*/
fbe_status_t fbe_database_homewrecker_process(fbe_database_service_t *database_service_ptr);
fbe_status_t fbe_database_system_disk_fru_descriptor_init(fbe_database_service_t *database_service_ptr);
fbe_status_t fbe_database_system_disk_fru_descriptor_inmemory_init(fbe_homewrecker_fru_descriptor_t *out_fru_descriptor_mem);
fbe_status_t database_homewrecker_send_event_log(fbe_u32_t msg_id, event_log_param_selector_t* param);

fbe_status_t fbe_database_mini_homewrecker_process(fbe_database_service_t *database_service_ptr, homewrecker_system_disk_table_t *sdt);



fbe_status_t fbe_database_get_ica_flags(fbe_bool_t *invalidate_configuration);
fbe_status_t fbe_database_reset_ica_flags(void);
fbe_status_t fbe_database_read_and_process_ica_flags(fbe_bool_t *invalidate_configuration);
fbe_status_t fbe_database_get_imaging_flags(fbe_imaging_flags_t *imaging_flags);

fbe_status_t fbe_database_set_invalidate_configuration_flag(fbe_u32_t flag);


fbe_status_t fbe_database_wait_for_ready_system_drives(fbe_bool_t wait_for_all);
fbe_status_t fbe_database_get_system_pdo_object_id_by_pvd_id(fbe_object_id_t pvd_object_id,fbe_object_id_t *pdo_object_id);
fbe_status_t database_control_complete_update_peer_packet(fbe_status_t status);
fbe_status_t fbe_database_wait_for_object_state(fbe_object_id_t object_id, fbe_lifecycle_state_t lifecycle_state, fbe_u32_t timeout_ms);
fbe_status_t fbe_database_generic_get_object_state(fbe_object_id_t object_id, 
                                                           fbe_lifecycle_state_t* lifecycle_state, 
                                                           fbe_package_id_t package_id);


fbe_status_t fbe_database_check_and_recreate_system_object(fbe_database_system_object_recreate_flags_t *sys_obj_op_flags_p);


/*config thread related function*/
void fbe_database_config_thread(void *context);
void fbe_database_config_thread_init_passive(void);
void fbe_db_thread_stop(fbe_database_service_t *database_Service_ptr);
void fbe_database_reload_tables_in_mid_update(void);
fbe_status_t fbe_database_get_board_resume_info(fbe_board_mgmt_get_resume_t *board_info);
fbe_status_t fbe_database_persist_prom_info(fbe_board_mgmt_get_resume_t *board_info, database_midplane_info_t *prom_info);
void fbe_database_enter_service_mode(fbe_database_service_mode_reason_t reason);
fbe_status_t fbe_database_config_reload(fbe_database_service_t *database_service_ptr);
void fbe_db_thread_wakeup(void);
void fbe_db_create_objects_passive_in_cmi_thread(void);

fbe_status_t fbe_database_system_db_header_init(database_system_db_header_t *system_db_header);

fbe_status_t database_set_time_threshold(fbe_database_time_threshold_t *time_threshold);
fbe_status_t fbe_database_unregister_system_drive_copy_back_notification(void);

fbe_status_t database_stop_all_background_services(fbe_packet_t *packet);
fbe_status_t database_restart_all_background_services(fbe_packet_t *packet);

/*configuration backup/restore routines*/
fbe_status_t fbe_database_cleanup_and_reinitialize_persist_service(fbe_bool_t clean_up);
fbe_status_t fbe_database_no_tran_persist_entries(fbe_u8_t *entries, fbe_u32_t entry_num,
                                          fbe_u32_t entry_size, 
                                          fbe_persist_sector_type_t type);

/*Expected Memory Value Process Routines*/
fbe_status_t database_set_shared_expected_memory_info(fbe_database_service_t *database_service_p, fbe_database_emv_t semv_info);

fbe_status_t database_initialize_shared_expected_memory_info(fbe_database_service_t *service_ptr);

fbe_status_t database_check_system_memory_configuration(fbe_database_service_t *database_service_p);

fbe_status_t database_make_planned_drive_online(fbe_database_control_online_planned_drive_t* online_drive);

fbe_status_t fbe_database_system_entries_load_from_persist(void);

/* set the in-memory fru descriptor and persist the update on disk*/
fbe_status_t fbe_database_set_homewrecker_fru_descriptor(fbe_database_service_t *db_service, fbe_homewrecker_fru_descriptor_t *in_fru_descriptor);
fbe_status_t fbe_database_get_homewrecker_fru_descriptor(fbe_homewrecker_fru_descriptor_t* out_fru_descriptor,
                                                                            fbe_homewrecker_get_raw_mirror_info_t* report,
                                                                            fbe_homewrecker_get_fru_disk_mask_t disk_mask);
fbe_status_t fbe_database_homewrecker_determine_standard_fru_descriptor(
                          homewrecker_system_disk_table_t* system_disk_table,
                          homewrecker_system_disk_table_t* standard_disk);
void fbe_database_display_fru_descriptor(fbe_homewrecker_fru_descriptor_t* fru_descriptor);
void fbe_database_display_fru_signature(fbe_fru_signature_t* fru_signature);

fbe_status_t database_perform_drive_connection(fbe_database_control_drive_connection_t* drive_connection_req);

/* Set the degraded mode reason to SPID module. */
void fbe_database_set_degraded_mode_reason_in_SPID(fbe_database_service_mode_reason_t reason);


/* Database polling functions */
void fbe_database_poll_initialize_poll_ring_buffer(database_poll_ring_buffer_t *database_ring_buffer_p); 
fbe_bool_t fbe_database_poll_is_recordkeeping_active(void);
void fbe_database_poll_complain(void); 
void fbe_database_poll_record_poll(fbe_database_poll_request_t poll_flag);
fbe_u32_t fbe_database_poll_get_next_index(fbe_u32_t index);

/* extended cache vault drive tier functions */
fbe_bool_t fbe_database_is_vault_drive_tier_enabled(void);

/* encryption related functions */
fbe_status_t fbe_database_encryption_init(fbe_u32_t total_mem_mb, fbe_u8_t *virtual_addr);
fbe_status_t fbe_database_encryption_destroy(void);
fbe_status_t database_setup_encryption_key(fbe_database_control_setup_encryption_key_t *encryption_info);
fbe_status_t database_rekey_encryption_key(fbe_database_control_setup_encryption_key_t *encryption_info);
fbe_status_t database_remove_encryption_keys(fbe_database_control_remove_encryption_keys_t *encryption_info);
fbe_status_t database_update_drive_keys(fbe_database_control_update_drive_key_t *encryption_info);
fbe_u32_t    fbe_database_encryption_get_local_table_size(void);
fbe_u8_t *   fbe_database_encryption_get_local_table_virtual_addr(void);
fbe_status_t fbe_database_encryption_set_peer_table_addr(fbe_u64_t peer_physical_address);
fbe_u64_t    fbe_database_encryption_get_peer_table_addr(void);
fbe_database_key_entry_t *fbe_database_encryption_get_next_active_key_entry( fbe_database_key_entry_t *current_entry);
fbe_status_t fbe_database_encryption_store_key_entry(fbe_encryption_setup_encryption_keys_t *encryption_key);
fbe_status_t fbe_database_encryption_reconstruct_key_tables(void);

fbe_status_t database_system_objects_generate_lun_from_psl(fbe_database_service_t *fbe_database_service,
                                                           fbe_object_id_t object_id, 
                                                           fbe_private_space_layout_lun_info_t *lun_info,
                                                           fbe_bool_t invalidate,
                                                           fbe_bool_t recreate_sys_obj);
fbe_status_t database_system_objects_create_object_and_its_edges(fbe_database_service_t *fbe_database_service, 
                                                                 fbe_object_id_t object_id,
                                                                 fbe_bool_t    is_initial_created);
fbe_status_t fbe_database_get_list_be_port_object_id(fbe_object_id_t *port_obj_id_buffer, fbe_u32_t *port_count, fbe_u32_t buffer_count);
fbe_status_t fbe_database_get_be_port_info(fbe_database_control_get_be_port_info_t *be_port_info);
fbe_status_t fbe_database_setup_kek(fbe_database_control_setup_kek_t *key_encryption_info);
fbe_status_t fbe_database_destroy_kek(fbe_database_control_destroy_kek_t *key_encryption_info);
fbe_status_t fbe_database_setup_kek_kek(fbe_database_control_setup_kek_kek_t *key_encryption_info);
fbe_status_t fbe_database_destroy_kek_kek(fbe_database_control_destroy_kek_kek_t *key_encryption_info);
fbe_status_t fbe_database_reestablish_kek_kek(fbe_database_control_reestablish_kek_kek_t *key_encryption_info);
fbe_status_t fbe_database_set_port_encryption_mode(fbe_database_control_port_encryption_mode_t *port_mode);
fbe_status_t fbe_database_get_port_encryption_mode(fbe_database_control_port_encryption_mode_t *port_mode);
fbe_status_t fbe_database_cmi_send_get_be_port_info_to_peer(fbe_database_service_t * fbe_database_service,
                                                            fbe_database_control_get_be_port_info_t  *be_port_info);
fbe_status_t fbe_database_cmi_send_encryption_backup_state_to_peer(fbe_encryption_backup_state_t  encryption_backup_state);
fbe_status_t fbe_database_cmi_send_setup_kek_to_peer (fbe_database_service_t * fbe_database_service,
                                                      fbe_database_control_setup_kek_t *key_encryption_info);
fbe_status_t fbe_database_cmi_send_destroy_kek_to_peer (fbe_database_service_t * fbe_database_service,
                                                      fbe_database_control_destroy_kek_t *key_encryption_info);
fbe_status_t fbe_database_cmi_send_setup_kek_kek_to_peer (fbe_database_service_t * fbe_database_service,
                                                      fbe_database_control_setup_kek_kek_t *key_encryption_info);
fbe_status_t fbe_database_cmi_send_destroy_kek_kek_to_peer (fbe_database_service_t * fbe_database_service,
                                                            fbe_database_control_destroy_kek_kek_t *key_encryption_info);
fbe_status_t fbe_database_cmi_send_reestablish_kek_kek_to_peer (fbe_database_service_t * fbe_database_service,
                                                                fbe_database_control_reestablish_kek_kek_t *key_encryption_info);
fbe_status_t fbe_database_cmi_send_port_encryption_mode_to_peer (fbe_database_service_t * fbe_database_service,
                                                                 fbe_database_control_port_encryption_mode_t *mode,
                                                                 fbe_database_port_encryption_op_t op);
fbe_status_t database_system_objects_check_for_encryption(void);
fbe_status_t fbe_database_cmi_send_kill_drive_request_to_peer(fbe_object_id_t pdo_object_id);
fbe_bool_t fbe_database_get_extended_cache_enabled(void);
fbe_status_t fbe_database_mark_pvd_swap_pending(fbe_object_id_t object_id, fbe_provision_drive_swap_pending_reason_t set_offline_reason);

/* fbe_database_util.c */
fbe_status_t fbe_database_util_get_persist_entry_info(database_table_header_t *in_entry_ptr,
                                                      fbe_bool_t *b_is_entry_in_use_p);
fbe_bool_t fbe_database_is_validation_allowed(fbe_database_validate_request_type_t validate_caller,
                                              fbe_database_validate_not_allowed_reason_t *not_allowed_reason_p);
fbe_status_t fbe_database_validate_database_if_enabled(fbe_database_validate_request_type_t validate_caller,
                                                       fbe_database_validate_failure_action_t failure_action,
                                                       fbe_status_t *validation_status_p);
fbe_status_t fbe_database_start_validate_database_job(fbe_database_validate_request_type_t validate_caller,
                                                      fbe_database_validate_failure_action_t failure_action);


/* fbe_database_export_lun.c */
fbe_status_t fbe_database_layout_get_export_lun_region_by_raid_group_id(fbe_u32_t rg_id, fbe_private_space_layout_region_t *region);
fbe_status_t database_system_objects_create_export_lun_objects(fbe_database_service_t *fbe_database_service, 
                                                               homewrecker_system_disk_table_t* system_disk_table);
fbe_bool_t fbe_database_mini_mode(void);
fbe_status_t fbe_database_reconstruct_export_lun_tables(fbe_database_service_t *fbe_database_service);
fbe_status_t fbe_database_layout_get_export_lun_region_by_raid_object_id(fbe_object_id_t obj,
                                                                         fbe_private_space_layout_region_t *region);
fbe_status_t fbe_database_get_platform_info(fbe_board_mgmt_platform_info_t *platform_info);
fbe_status_t fbe_database_set_sp_fault_led(LED_BLINK_RATE blink_rate, fbe_u32_t status_condition);
/***********************************************
 * end file fbe_database_private.h
 ***********************************************/
#endif /* end FBE_DATABASE_PRIVATE_H */


