#ifndef BASE_CONFIG_PRIVATE_H
#define BASE_CONFIG_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_base_config_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains private defines for the base config object.
 *  This includes the definitions of the
 *  @ref fbe_base_config_t "base config" itself, as well as the associated
 *  structures and defines.
 * 
 * @ingroup base_config_class_files
 * 
 * @revision
 *   05/20/2009:  Created. RPF
 *
 ***************************************************************************/


#include "fbe_base_object.h"
#include "base_object_private.h"
#include "fbe/fbe_base_config.h"
#include "fbe/fbe_transport.h"
#include "fbe_block_transport.h"
#include "fbe_metadata.h"
#include "fbe/fbe_scheduler_interface.h"
#include "fbe/fbe_database_packed_struct.h"
#include "fbe/fbe_encryption.h"

/******************************
 * LITERAL DEFINITIONS.
 ******************************/

 enum fbe_base_config_constants_e {
    FBE_BASE_CONFIG_WIDTH_INVALID  = 0,
    FBE_BASE_CONFIG_DEFAULT_IDLE_TIME_IN_SECONDS  = 120,
    FBE_BASE_CONFIG_HIBERNATION_WAKE_UP_DEFAULT = 1440 /*wake up every 24 hours (1440 seconds) to sniff*/
};

/* Lifecycle definitions
 * Define the base config lifecycle's instance.
 */
extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(base_config);

/* These are the lifecycle condition ids for a sas physical drive object. */

/*! @enum fbe_base_config_lifecycle_cond_id_t  
 *  
 *  @brief These are the lifecycle conditions for the base config object. 
 */
typedef enum fbe_base_config_lifecycle_cond_id_e 
{
   /*  FBE_BASE_CONFIG_LIFECYCLE_COND_NOT_CONFIGURED  */

    /*! Path to downstream object has changed it's state.
     */
    FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_BASE_CONFIG),

    FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_DISABLED,

    /*! In destroy state we will try to detach downstream edges.
     */
    FBE_BASE_CONFIG_LIFECYCLE_COND_DETACH_BLOCK_EDGES,

    /* We should initialize the memory part of metadata to "see" what our body on other side is doing */
    FBE_BASE_CONFIG_LIFECYCLE_COND_METADATA_MEMORY_INIT,

    /*! Will initialize object's nonpaged metadata in memory. */
    FBE_BASE_CONFIG_LIFECYCLE_COND_NONPAGED_METADATA_INIT,

    /*! Will initialize metadata element */
    FBE_BASE_CONFIG_LIFECYCLE_COND_METADATA_ELEMENT_INIT,

    /*! zero out the consumed area before we write the paged metadata. */
    FBE_BASE_CONFIG_LIFECYCLE_COND_ZERO_CONSUMED_AREA,

    /*! paged metadata write default condition. */
    FBE_BASE_CONFIG_LIFECYCLE_COND_WRITE_DEFAULT_PAGED_METADATA,

    /*! nonpaged metadata write default condition. */
    FBE_BASE_CONFIG_LIFECYCLE_COND_WRITE_DEFAULT_NONPAGED_METADATA,

    /*! persist the default non-paged metadata*/
    FBE_BASE_CONFIG_LIFECYCLE_COND_PERSIST_DEFAULT_NONPAGED_METADATA,

    /*! verifies the metadata and set the appropriate condition. */
    FBE_BASE_CONFIG_LIFECYCLE_COND_METADATA_VERIFY,

    /*Check version difference between in-memory non-paged metadata and on-disk non-paged 
      metadata; upgrade it if version different */
    FBE_BASE_CONFIG_LIFECYCLE_COND_CHECK_NON_PAGED_METADATA_VERSION,

    /*! Will fetch the negotiate information. */
    FBE_BASE_CONFIG_LIFECYCLE_COND_NEGOTIATE_BLOCK_SIZE,

    /*! unregister from the metadata service*/
    FBE_BASE_CONFIG_LIFECYCLE_COND_UNREGISTER_METADATA_ELEMENT,

    /*! propagate edge traffic load change*/
    FBE_BASE_CONFIG_LIFECYCLE_COND_TRAFFIC_LOAD_CHANGE,

    /*! It waits until all the disabled edge get transition to ENA/BROKEN state. */
    FBE_BASE_CONFIG_LIFECYCLE_COND_ESTABLISH_EDGE_STATES,

    /*! Validate downstream capacity. */
    FBE_BASE_CONFIG_LIFECYCLE_COND_VALIDATE_DOWNSTREAM_CAPACITY,

    /*! do what needs to be done when we enter hibernation*/
    FBE_BASE_CONFIG_LIFECYCLE_COND_START_HIBERNATION,

    /*! deque pending IO that was waiting for us because we were hibernating (or any other reson)*/
    FBE_BASE_CONFIG_LIFECYCLE_COND_DEQUEUE_PENDING_IO,

    /*! when we wake up from hibernation, we need to wake up our server so he can serve us*/
    FBE_BASE_CONFIG_LIFECYCLE_COND_WAKE_UP_HIBERNATING_SERVERS,

    /*! propagate the power saving enabling to all servers*/
    FBE_BASE_CONFIG_LIFECYCLE_COND_ENABLE_POWER_SAVING_ON_SERVERS,

    /*! check during hibernation that we don't need to stop power saving*/
    FBE_BASE_CONFIG_LIFECYCLE_COND_CHECK_POWER_SAVE_DISABLED,

    /*reset the time for counting when we got to hibernate so we can wake up after a while if needed*/
    FBE_BASE_CONFIG_LIFECYCLE_COND_RESET_HIBERNATION_TIMER,

    /*see if we need to wake up after a while to make sure the system is still alive*/
    FBE_BASE_CONFIG_LIFECYCLE_COND_HIBERNATION_SNIFF_WAKEUP_COND,

    /*! wait for the configuration commit before we allow object to perform any operation. */
    FBE_BASE_CONFIG_LIFECYCLE_COND_WAIT_FOR_CONFIG_COMMIT,

    /*mark the fact we are not saving power*/
    FBE_BASE_CONFIG_LIFECYCLE_COND_SET_NOT_SAVING_POWER,

    /*verify our severs are not saving power anymore*/
    FBE_BASE_CONFIG_LIFECYCLE_COND_WAIT_FOR_SERVERS_WAKEUP,

    /* Updates metadata memory */
    FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_METADATA_MEMORY,

    /*update the time passed since we got the lst io*/
    FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_LAST_IO_TIME,

    /* take object out of hibernation when and edge state goes to enaled or broken/disabled */
    FBE_BASE_CONFIG_LIFECYCLE_COND_EDGE_CHANGE_DURING_HIBERNATION,

    /*persist the non paged metadata*/
    FBE_BASE_CONFIG_LIFECYCLE_COND_PERSIST_NON_PAGED_METADATA,

    /* Wait for the downstream health to be optimal for us to proceed */
    FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_NOT_OPTIMAL,

    /* Quiesce the IOs */
    FBE_BASE_CONFIG_LIFECYCLE_COND_QUIESCE,

    /* Unquiesce the IOs */
    FBE_BASE_CONFIG_LIFECYCLE_COND_UNQUIESCE,

    /* Passive side requests to join the active side.  */
    FBE_BASE_CONFIG_LIFECYCLE_COND_PASSIVE_REQUEST_JOIN,

    /* Active side allows the passive side to join. */
    FBE_BASE_CONFIG_LIFECYCLE_COND_ACTIVE_ALLOW_JOIN,

    /* Common condition to perform any background monitor operation */
    FBE_BASE_CONFIG_LIFECYCLE_COND_BACKGROUND_MONITOR_OPERATION, 
    
    /* Bring both SPs to the ACTIVATE state */ 
    FBE_BASE_CONFIG_LIFECYCLE_COND_CLUSTERED_ACTIVATE,      

    /* sync both SPs to the JOINED state */ 
    FBE_BASE_CONFIG_LIFECYCLE_COND_JOIN_SYNC, 

    /* clear the NP */
    FBE_BASE_CONFIG_LIFECYCLE_COND_CLEAR_NP,

    /* set DRIVE_FAULT */
    FBE_BASE_CONFIG_LIFECYCLE_COND_SET_DRIVE_FAULT,

    /* clear DRIVE_FAULT */
    FBE_BASE_CONFIG_LIFECYCLE_COND_CLEAR_DRIVE_FAULT,

    /* Release all the stripe locks that was waiting on peer 
     */
    FBE_BASE_CONFIG_LIFECYCLE_COND_PEER_DEATH_RELEASE_SL,

    /* reinit the NP */
    FBE_BASE_CONFIG_LIFECYCLE_COND_REINIT_NP,

    FBE_BASE_CONFIG_LIFECYCLE_COND_STRIPE_LOCK_START,
    FBE_BASE_CONFIG_LIFECYCLE_COND_ENCRYPTION,

    FBE_BASE_CONFIG_LIFECYCLE_COND_LAST /* must be last */
} 
fbe_base_config_lifecycle_cond_id_t;

typedef struct fbe_base_config_memory_allocation_chunks_s{
    fbe_u32_t number_of_packets;
    fbe_u32_t sg_element_count;
    fbe_u32_t buffer_count;
    fbe_u32_t pre_read_desc_count;
    fbe_u32_t buffer_size;
    fbe_bool_t alloc_pre_read_desc;
}fbe_base_config_memory_allocation_chunks_t;

typedef struct fbe_base_config_memory_alloc_chunks_address_s{
    fbe_u32_t           number_of_packets;
    fbe_u32_t           sg_element_count;
    fbe_u32_t           buffer_count;
    fbe_u32_t           pre_read_desc_count;
    fbe_packet_t        **packet_array_p;
    fbe_sg_element_t    *sg_list_p;
    fbe_u8_t            **buffer_array_p;
    fbe_sg_element_t    **pre_read_sg_list_p;
    fbe_payload_pre_read_descriptor_t   **pre_read_desc_p;
    fbe_u8_t            *pre_read_buffer_p;
    fbe_u32_t           buffer_size;
    fbe_bool_t          assign_pre_read_desc;
}fbe_base_config_memory_alloc_chunks_address_t;

fbe_status_t fbe_base_config_nonpaged_metadata_set_object_id(fbe_base_config_nonpaged_metadata_t * base_config_nonpaged_metadata, 
                                                             fbe_object_id_t object_id);

fbe_status_t fbe_base_config_nonpaged_metadata_set_generation_number(fbe_base_config_nonpaged_metadata_t * base_config_nonpaged_metadata, 
                                                                     fbe_generation_code_t generation_number);
fbe_status_t
fbe_base_config_nonpaged_metadata_set_np_state(fbe_base_config_nonpaged_metadata_t * base_config_nonpaged_metadata_p, 
                                                                     fbe_base_config_nonpaged_metadata_state_t np_state);

/* Signature of base config object. */
#pragma pack(1)
typedef struct fbe_base_config_signature_s  {
    /*! magic number associated with object signature. */
    fbe_u32_t           magic_number;

    /*! computed CRC of the object signature. */
    fbe_u32_t           signature_crc;

    /*! signature object id. */
    fbe_object_id_t     object_id;

    /*! signature class id. */
    fbe_class_id_t      class_id;
    
    /*! Generation number. */
    fbe_config_generation_t generation_number;

    union {
        /*! wwn for the configured objects. */
        fbe_u64_t           wwn;

        /*! Serial number of the drive for the PVD object, this data is applicable 
         *  to only PVD object currently.
         */
        fbe_u8_t            serial_num[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1];
    } u;

} fbe_base_config_signature_t;
#pragma pack()

/*FBE_BASE_CONFIG_CONTROL_CODE_GET_SPINDOWN_QUALIFIED*/
typedef struct fbe_base_config_power_save_capable_info_s{
    fbe_bool_t spindown_qualified;/*!< If the drive is spindown qualified. */
}fbe_base_config_power_save_capable_info_t;

/*!****************************************************************************
 *    
 * @struct fbe_base_config_t
 *  
 * @brief 
 *   This is the definition of the base config object.
 *   This object represents a base config, which is an object which can
 *   perform block size conversions.
 ******************************************************************************/

typedef struct fbe_base_config_s
{    
    /*! This must be first.  This is the base object we inherit from.
     */
    fbe_base_object_t base_object;

    /*! This is the structure which is used for holding the list of attached 
     *  edges from clients.  
     */
    fbe_block_transport_server_t block_transport_server;
    fbe_path_attr_t global_path_attr; /* For the attr which appyed to all clients */

    /*! This is used to store the metadata related information. */
    fbe_metadata_element_t metadata_element;

    /*! This is our storage extent protocol edges to downstream objects. */
    fbe_block_edge_t *block_edge_p;

    /*! This is to queue the event to the base config object (only used if receiving object 
     *  needs monitor context to process the event, if event is queued then downstream object
     *  will be denied certain operation (like proactive copy, sparing etc).
     */
    fbe_queue_head_t    event_queue_head;

    /*! Lock to protect the event queue. 
     */
    fbe_spinlock_t      event_queue_lock;

    /*! power saving decisions*/
    fbe_u64_t   power_saving_idle_time_in_seconds;

    /*! This is used to count the attempts we had to do a background operation */
    fbe_u32_t   background_operation_deny_count;

    /*! This is the number of edges. */
    fbe_u32_t width;

    /*! This is used to determine whether we have commited configuration. */
    /* fbe_bool_t is_configuration_commited; swithed to flag FBE_BASE_CONFIG_FLAG_CONFIGURATION_COMMITED*/

    /*! enable or disbale power saving of this objects*/
    /* fbe_bool_t  power_saving_enabled;  swithed to flag FBE_BASE_CONFIG_FLAG_POWER_SAVING_ENABLED*/

    /*! how long have we been sleeping, and should we wake up to do some sniff*/
    fbe_time_t  hibernation_time;

    /*! flags related to i/o related processing for the object. */
    fbe_base_config_flags_t flags; 

    /*! Generation number. */
    fbe_config_generation_t generation_number; 

    /*! power save capable info. */
    fbe_base_config_power_save_capable_info_t power_save_capable;

    union
    {
        /*! wwn for the configured objects. */
        fbe_u64_t           wwn;

        /*! Serial number is applicable only for the pvd object. */
        /* fbe_u8_t            serial_num[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1]; */
    } u;

    /*! This is used to count the background operations with immediate rescheduling */
    fbe_u32_t   background_operation_count;

	
	/*! Represents the highest memory request priority from the I/O's on terminator queue.
		Valid when object is in quiesce mode, otherwise - 0.
	*/
	fbe_packet_resource_priority_t resource_priority;

    /*! This is used to save the encryption state of the object */
    fbe_base_config_encryption_state_t encryption_state;

    /*! This is used to save the encryption mode of the object */
    fbe_base_config_encryption_mode_t  encryption_mode;

    /*! This is used to save the handle to the keys */
    fbe_encryption_setup_encryption_keys_t *key_handle;

    /* Lifecycle defines. */
    FBE_LIFECYCLE_DEF_INST_DATA;
    FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_BASE_CONFIG_LIFECYCLE_COND_LAST));
}fbe_base_config_t;


/*! @fn fbe_base_config_lock(fbe_base_config_t *const base_config_p) 
 *  
 * @brief 
 *   Function which locks the base config via the base object lock.
 */
static __forceinline void
fbe_base_config_lock(fbe_base_config_t *const base_config_p)
{
    fbe_base_object_lock((fbe_base_object_t *)base_config_p);
    return;
}

/* @fn fbe_base_config_unlock(fbe_base_config_t *const base_config_p) 
 *  
 * @brief 
 *   Function which unlocks the base config via the base object lock.
 */
static __forceinline void
fbe_base_config_unlock(fbe_base_config_t *const base_config_p)
{
    fbe_base_object_unlock((fbe_base_object_t *)base_config_p);
    return;
}

static __forceinline fbe_base_config_t  * 
fbe_base_config_metadata_element_to_base_config_object(struct fbe_metadata_element_s * metadata_element)
{
    fbe_base_config_t  * base_config;

    base_config = (fbe_base_config_t  *)((fbe_u8_t *)metadata_element - (fbe_u8_t *)(&((fbe_base_config_t  *)0)->metadata_element));
    return base_config;
}

/* Accessors for flags. */
fbe_bool_t fbe_base_config_is_flag_set(fbe_base_config_t * base_config_p, fbe_base_config_flags_t flags);
void fbe_base_config_init_flags(fbe_base_config_t * base_config_p);
void fbe_base_config_set_flag(fbe_base_config_t * base_config_p, fbe_base_config_flags_t flags);
void fbe_base_config_clear_flag(fbe_base_config_t * base_config_p, fbe_base_config_flags_t flags);
fbe_status_t fbe_base_config_get_flags(fbe_base_config_t * base_config, fbe_base_config_flags_t * flags);

static __forceinline fbe_status_t fbe_base_config_increment_deny_count(fbe_base_config_t * base_config_p)
{
    base_config_p->background_operation_deny_count++;
    return FBE_STATUS_OK;
}

static __forceinline fbe_status_t fbe_base_config_reset_deny_count(fbe_base_config_t * base_config_p)
{
    base_config_p->background_operation_deny_count = 0;
    return FBE_STATUS_OK;
}

static __forceinline fbe_u32_t fbe_base_config_get_deny_count(fbe_base_config_t * base_config_p)
{
    return base_config_p->background_operation_deny_count ;
}

/* Accessors for clustered flags. */
fbe_bool_t fbe_base_config_is_clustered_flag_set(fbe_base_config_t * base_config_p, fbe_base_config_clustered_flags_t flags);
fbe_bool_t fbe_base_config_is_any_clustered_flag_set(fbe_base_config_t * base_config, fbe_base_config_clustered_flags_t flags);
fbe_status_t fbe_base_config_init_clustered_flags(fbe_base_config_t * base_config_p);
fbe_status_t fbe_base_config_set_clustered_flag(fbe_base_config_t * base_config_p, fbe_base_config_clustered_flags_t flags);
fbe_status_t fbe_base_config_clear_clustered_flag(fbe_base_config_t * base_config_p, fbe_base_config_clustered_flags_t flags);
fbe_status_t fbe_base_config_get_clustered_flags(fbe_base_config_t * base_config, fbe_base_config_clustered_flags_t * flags);
fbe_status_t fbe_base_config_get_peer_clustered_flags(fbe_base_config_t * base_config, fbe_base_config_clustered_flags_t * flags);

fbe_bool_t fbe_base_config_is_peer_clustered_flag_set(fbe_base_config_t * base_config, fbe_base_config_clustered_flags_t flags);
fbe_bool_t fbe_base_config_is_any_peer_clustered_flag_set(fbe_base_config_t * base_config, fbe_base_config_clustered_flags_t flags);

/* Accessors for the signature data. */

static __forceinline fbe_status_t
fbe_base_config_signature_get_object_id(fbe_base_config_signature_t * base_config_signature_p, fbe_object_id_t *signature_object_id_p)
{
    *signature_object_id_p = base_config_signature_p->object_id;
    return FBE_STATUS_OK;
}

static __forceinline fbe_status_t
fbe_base_config_signature_get_class_id(fbe_base_config_signature_t * base_config_signature_p, fbe_class_id_t *signature_class_id_p)
{
    *signature_class_id_p = base_config_signature_p->class_id;
    return FBE_STATUS_OK;
}

static __forceinline fbe_status_t
fbe_base_config_signature_get_wwn(fbe_base_config_signature_t * base_config_signature_p, fbe_u64_t *signature_wwn_p)
{
    *signature_wwn_p = base_config_signature_p->u.wwn;
    return FBE_STATUS_OK;
}

static __forceinline fbe_status_t
fbe_base_config_signature_get_magic_number(fbe_base_config_signature_t * base_config_signature_p, fbe_u32_t *signature_magic_number_p)
{
    *signature_magic_number_p = base_config_signature_p->magic_number;
    return FBE_STATUS_OK;
}

static __forceinline fbe_status_t
fbe_base_config_signature_get_signature_crc(fbe_base_config_signature_t * base_config_signature_p, fbe_u32_t *signature_crc_p)
{
    *signature_crc_p = base_config_signature_p->signature_crc;
    return FBE_STATUS_OK;
}

static __forceinline fbe_status_t
fbe_base_config_signature_set_object_id(fbe_base_config_signature_t * base_config_signature_p, fbe_object_id_t signature_object_id)
{
    base_config_signature_p->object_id = signature_object_id;
    return FBE_STATUS_OK;
}

static __forceinline fbe_status_t
fbe_base_config_signature_set_class_id(fbe_base_config_signature_t * base_config_signature_p, fbe_class_id_t signature_class_id)
{
    base_config_signature_p->class_id = signature_class_id;
    return FBE_STATUS_OK;
}

static __forceinline fbe_status_t
fbe_base_config_signature_set_wwn(fbe_base_config_signature_t * base_config_signature_p, fbe_u64_t signature_wwn)
{
    base_config_signature_p->u.wwn = signature_wwn;
    return FBE_STATUS_OK;
}

static __forceinline fbe_status_t
fbe_base_config_signature_set_magic_number(fbe_base_config_signature_t * base_config_signature_p, fbe_u32_t signature_magic_number)
{
    base_config_signature_p->magic_number = signature_magic_number;
    return FBE_STATUS_OK;
}

static __forceinline fbe_status_t
fbe_base_config_signature_set_signature_crc(fbe_base_config_signature_t * base_config_signature_p, fbe_u32_t signature_crc)
{
    base_config_signature_p->signature_crc = signature_crc;
    return FBE_STATUS_OK;
}

static __forceinline fbe_status_t
fbe_base_config_signature_set_generation_num(fbe_base_config_signature_t * base_config_signature_p, fbe_config_generation_t generation_num) 
{
    base_config_signature_p->generation_number = generation_num;
    return FBE_STATUS_OK;
}

static __forceinline fbe_status_t
fbe_base_config_signature_get_generation_num(fbe_base_config_signature_t * base_config_signature_p, fbe_config_generation_t *generation_num_p)
{
    *generation_num_p = base_config_signature_p->generation_number;
    return FBE_STATUS_OK;
}

static __forceinline fbe_status_t
fbe_base_config_signature_set_serial_num(fbe_base_config_signature_t * base_config_signature_p, fbe_u8_t *serial_num_p)
{
    fbe_copy_memory(base_config_signature_p->u.serial_num, serial_num_p, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);
    return FBE_STATUS_OK;
}

static __forceinline fbe_status_t
fbe_base_config_signature_get_serial_num(fbe_base_config_signature_t * base_config_signature_p, fbe_u8_t **serial_num_p)
{
    *serial_num_p = base_config_signature_p->u.serial_num;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_config_set_capacity(fbe_base_config_t *base_config_p, fbe_lba_t capacity);
fbe_status_t fbe_base_config_get_capacity(fbe_base_config_t *base_config_p, fbe_lba_t *capacity_p);

/* Accessors for the width. */
fbe_status_t fbe_base_config_set_width(fbe_base_config_t *base_config_p, fbe_u32_t width);
fbe_status_t fbe_base_config_get_width(fbe_base_config_t *base_config_p, fbe_u32_t *width_p);

/* Accessors for the block_edge_p.*/
fbe_status_t fbe_base_config_set_block_edge_ptr(fbe_base_config_t *base_config_p, fbe_block_edge_t *block_edge_p);
fbe_status_t fbe_base_config_get_block_edge_ptr(fbe_base_config_t *base_config_p, fbe_block_edge_t **block_edge_p_p);

static __forceinline fbe_status_t 
fbe_base_config_get_block_edge(fbe_base_config_t *base_config_p, fbe_block_edge_t **block_edge_p_p, fbe_u32_t edge_index)
{
    /* Peter Puhov
        We have to check that edge_index are in range 
    */
    *block_edge_p_p = &base_config_p->block_edge_p[edge_index];
    return FBE_STATUS_OK;
}

/* Accessors for the commit. */
fbe_status_t fbe_base_config_set_configuration_committed(fbe_base_config_t * base_config_p);
fbe_status_t fbe_base_config_unset_configuration_committed(fbe_base_config_t * base_config_p);
fbe_bool_t   fbe_base_config_is_configuration_commited(fbe_base_config_t * base_config_p);

/* Accessors to determine if this is the initial creation of the object or not */
fbe_bool_t   fbe_base_config_is_initial_configuration(fbe_base_config_t * base_config_p);

/* Accessors for the base config signature. */
fbe_status_t fbe_base_config_get_signature(fbe_base_config_t * base_config_p, fbe_base_config_signature_t ** base_config_signature_p);

/* Accessors for the generation number. */
fbe_status_t fbe_base_config_set_generation_num(fbe_base_config_t * base_config_p, fbe_config_generation_t generation_number);
fbe_status_t fbe_base_config_get_generation_num(fbe_base_config_t * base_config_p, fbe_config_generation_t *generation_number);

/* Accessors the serial number. */
//fbe_status_t fbe_base_config_set_serial_num(fbe_base_config_t * base_config_p, fbe_u8_t *serial_num);
//fbe_status_t fbe_base_config_get_serial_num(fbe_base_config_t * base_config_p, fbe_u8_t **serial_num);

/* Accessors the default offset. */
fbe_status_t fbe_base_config_set_default_offset(fbe_base_config_t *base_config_p);

/* Accessors for global_path_attr. */
fbe_bool_t fbe_base_config_is_global_path_attr_set(fbe_base_config_t * base_config, fbe_path_attr_t global_path_attr);
void fbe_base_config_set_global_path_attr(fbe_base_config_t * base_config, fbe_path_attr_t global_path_attr);
void fbe_base_config_clear_global_path_attr(fbe_base_config_t * base_config, fbe_path_attr_t global_path_attr);

/* Accessors for encryption state. */
fbe_status_t fbe_base_config_set_encryption_state(fbe_base_config_t * base_config, fbe_base_config_encryption_state_t encryption_state);
fbe_status_t fbe_base_config_set_encryption_state_with_notification(fbe_base_config_t * base_config_p, 
                                                                    fbe_base_config_encryption_state_t encryption_state);
fbe_status_t fbe_base_config_encryption_state_notification(fbe_base_config_t * base_config_p);
fbe_status_t fbe_base_config_get_encryption_state(fbe_base_config_t * base_config, fbe_base_config_encryption_state_t *encryption_state);
fbe_bool_t fbe_base_config_is_encryption_state_push_keys_downstream(fbe_base_config_t * base_config);

/* Accessors for encryption mode. */
fbe_status_t 
fbe_base_config_get_encryption_mode(fbe_base_config_t * base_config, fbe_base_config_encryption_mode_t *encryption_mode);
fbe_status_t 
fbe_base_config_set_encryption_mode(fbe_base_config_t * base_config, fbe_base_config_encryption_mode_t encryption_mode);
fbe_bool_t fbe_base_config_is_encrypted_mode(fbe_base_config_t * base_config);
fbe_bool_t fbe_base_config_is_rekey_mode(fbe_base_config_t * base_config);
fbe_bool_t fbe_base_config_is_encrypted_state(fbe_base_config_t * base_config);
fbe_status_t 
fbe_base_config_set_key_handle(fbe_base_config_t * base_config, fbe_encryption_setup_encryption_keys_t *key_handle);
fbe_status_t 
fbe_base_config_get_key_handle(fbe_base_config_t * base_config, fbe_encryption_setup_encryption_keys_t **key_handle);


/* Imports for visibility.*/

/* fbe_base_config_main.c */
fbe_status_t fbe_base_config_create_object(fbe_packet_t * packet_p, fbe_object_handle_t * object_handle);
fbe_status_t fbe_base_config_destroy_object( fbe_object_handle_t object_handle);

fbe_status_t fbe_base_config_init(fbe_base_config_t * const base_config_p);


fbe_status_t fbe_base_config_event_entry(fbe_object_handle_t object_handle, 
                                         fbe_event_type_t event_type,
                                         fbe_event_context_t event_context);

fbe_status_t fbe_base_config_detach_edge(fbe_base_config_t * base_config, fbe_packet_t * packet, fbe_block_edge_t *edge_p);

fbe_status_t fbe_base_config_detach_edges(fbe_base_config_t * base_config, fbe_packet_t * packet);

fbe_status_t fbe_base_config_bouncer_entry(fbe_base_config_t * base_config, fbe_packet_t * packet);

fbe_status_t fbe_base_config_set_block_transport_const(fbe_base_config_t * base_config, fbe_block_transport_const_t * block_transport_const);

fbe_status_t fbe_base_config_set_outstanding_io_max(fbe_base_config_t * base_config, fbe_u32_t outstanding_io_max);
fbe_status_t fbe_base_config_set_stack_limit(fbe_base_config_t * base_config);

fbe_status_t fbe_base_config_metadata_init_memory(fbe_base_config_t * base_config,
                                                     fbe_packet_t * packet,
                                                     void * metadata_memory_ptr,
                                                     void * metadata_memory_peer_ptr, 
                                                     fbe_u32_t metadata_memory_size);

fbe_status_t fbe_base_config_metadata_nonpaged_init(fbe_base_config_t * base_config, fbe_u32_t data_size, fbe_packet_t * packet);
fbe_status_t fbe_base_config_metadata_nonpaged_zero(fbe_base_config_t * base_config_p);
fbe_status_t fbe_base_config_metadata_nonpaged_is_initialized(fbe_base_config_t * base_config_p, fbe_bool_t * is_metadata_initialized_p);
fbe_status_t fbe_base_config_metadata_is_initialized(fbe_base_config_t * base_config_p, fbe_bool_t * is_metadata_initialized_p);
fbe_status_t fbe_base_config_metadata_is_nonpaged_state_initialized(fbe_base_config_t * base_config_p, fbe_bool_t * is_magic_num_initialized_p);
fbe_status_t fbe_base_config_metadata_is_nonpaged_state_valid(fbe_base_config_t * base_config_p, fbe_bool_t * is_magic_num_valid_p);
fbe_status_t fbe_base_config_set_default_nonpaged_metadata(fbe_base_config_t * base_config_p, fbe_base_config_nonpaged_metadata_t *nonpaged_metadata_p);
fbe_lifecycle_status_t fbe_base_config_persist_non_paged_metadata_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
fbe_status_t fbe_base_config_metadata_set_paged_record_start_lba(fbe_base_config_t * base_config_p, fbe_lba_t paged_record_start_lba);
fbe_status_t fbe_base_config_metadata_get_paged_record_start_lba(fbe_base_config_t * base_config_p, fbe_lba_t * paged_record_start_lba_p);

fbe_status_t fbe_base_config_metadata_set_paged_mirror_metadata_offset(fbe_base_config_t * base_config_p, fbe_lba_t paged_mirror_metadata_offset);
fbe_status_t fbe_base_config_metadata_get_paged_mirror_metadata_offset(fbe_base_config_t * base_config_p, fbe_lba_t * paged_mirror_metadata_offset_p);

fbe_status_t fbe_base_config_metadata_set_paged_metadata_capacity(fbe_base_config_t * base_config_p, fbe_lba_t paged_metadata_capacity);
fbe_status_t fbe_base_config_metadata_get_paged_metadata_capacity(fbe_base_config_t * base_config_p, fbe_lba_t * paged_metadata_capacity_p);

fbe_status_t fbe_base_config_metadata_set_number_of_stripes(fbe_base_config_t * base_config_p, fbe_u64_t number_of_stripes);
fbe_status_t fbe_base_config_metadata_get_number_of_stripes(fbe_base_config_t * base_config_p, fbe_u64_t * number_of_stripes);

fbe_status_t fbe_base_config_metadata_set_number_of_private_stripes(fbe_base_config_t * base_config_p, fbe_u64_t number_of_stripes);
fbe_status_t fbe_base_config_metadata_get_number_of_private_stripes(fbe_base_config_t * base_config_p, fbe_u64_t * number_of_stripes);

fbe_status_t fbe_base_config_metadata_get_power_save_state(fbe_base_config_t * base_config_p, fbe_power_save_state_t *get_sate);
fbe_status_t fbe_base_config_metadata_set_power_save_state(fbe_base_config_t * base_config_p, fbe_power_save_state_t set_sate);
fbe_status_t fbe_base_config_metadata_get_peer_power_save_state(fbe_base_config_t * base_config_p, fbe_power_save_state_t *get_sate);
fbe_status_t fbe_base_config_metadata_get_peer_last_io_time(fbe_base_config_t * base_config_p, fbe_time_t *last_io_time);
fbe_status_t fbe_base_config_metadata_get_peer_lifecycle_state(fbe_base_config_t * base_config_p, fbe_lifecycle_state_t *peer_state);
fbe_status_t fbe_base_config_metadata_get_lifecycle_state(fbe_base_config_t * base_config_p, fbe_lifecycle_state_t *my_state);
fbe_status_t fbe_base_config_metadata_set_last_io_time(fbe_base_config_t * base_config_p, fbe_time_t set_time);
fbe_status_t fbe_base_config_metadata_get_last_io_time(fbe_base_config_t * base_config_p, fbe_time_t *get_time);


fbe_status_t fbe_base_config_metadata_set_quiesce_state_local_and_update_peer(fbe_base_config_t * base_config_p, 
                                                                              fbe_base_config_metadata_memory_flags_t state);
fbe_status_t fbe_base_config_metadata_get_quiesce_state_local(fbe_base_config_t * base_config_p, 
                                                              fbe_base_config_metadata_memory_flags_t * out_state_p);
fbe_status_t fbe_base_config_metadata_get_quiesce_state_peer(fbe_base_config_t * base_config_p, 
                                                             fbe_base_config_metadata_memory_flags_t * out_state_p);
fbe_status_t fbe_base_config_abort_monitor_ops(fbe_base_config_t * const base_config_p);

fbe_status_t fbe_base_config_nonpaged_metadata_get_version_size(fbe_base_config_nonpaged_metadata_t *nonpaged_metadata_p, fbe_u32_t *size);

fbe_status_t fbe_base_config_get_nonpaged_metadata_state(fbe_base_config_t * base_config_p, fbe_base_config_nonpaged_metadata_state_t* out_state_p);
fbe_status_t fbe_base_config_set_nonpaged_metadata_state(fbe_base_config_t * base_config_p, 
                                                         fbe_packet_t * packet_p, fbe_base_config_nonpaged_metadata_state_t state);
fbe_status_t fbe_base_config_encryption_check_key_state(fbe_base_config_t * base_config, fbe_packet_t * packet_p);
fbe_status_t fbe_base_config_get_edges_with_attribute(fbe_base_config_t * base_config, 
                                                      fbe_path_attr_t path_attrib,
                                                      fbe_u32_t * path_attr_count_p);
    
void fbe_base_config_class_trace(fbe_trace_level_t trace_level,
                         fbe_u32_t message_id,
                         const fbe_char_t * fmt, ...)
                         __attribute__((format(__printf_func__,3,4)));


typedef struct fbe_base_config_path_state_counters_s{
    fbe_u32_t  enabled_counter;
    fbe_u32_t  disabled_counter;
    fbe_u32_t  broken_counter;
    fbe_u32_t  invalid_counter;
}fbe_base_config_path_state_counters_t;

typedef struct fbe_base_config_operation_permission_s{
    fbe_traffic_priority_t  operation_priority;
    fbe_scheduler_credit_t  credit_requests;
    fbe_u32_t               io_credits;
}fbe_base_config_operation_permission_t;

/*data structure to hold the context information for non-paged metadata 
  version check and upgrade on background*/
typedef struct fbe_base_config_np_metadata_upgrade_context_s{
    fbe_base_config_t *base_config_p;
    fbe_sep_version_header_t version_header;
}fbe_base_config_np_metadata_upgrade_context_t;

fbe_status_t fbe_base_config_get_path_state_counters(fbe_base_config_t * base_config, 
                                                     fbe_base_config_path_state_counters_t * path_state_counters);

fbe_status_t 
fbe_base_config_send_event(fbe_base_config_t *base_config_p, fbe_event_type_t event_type, fbe_event_t * event);


fbe_status_t
fbe_base_config_metadata_nonpaged_write(fbe_base_config_t * base_config,
                                        fbe_packet_t * packet,
                                        fbe_u64_t   metadata_offset,
                                        fbe_u8_t *  metadata_record_data,
                                        fbe_u32_t metadata_record_data_size);

fbe_status_t fbe_base_config_metadata_nonpaged_write_persist(fbe_base_config_t * base_config,
                                                 fbe_packet_t * packet,
                                                 fbe_u64_t   metadata_offset,
                                                 fbe_u8_t *  metadata_record_data,
                                                 fbe_u32_t metadata_record_data_size);

fbe_status_t fbe_base_config_metadata_nonpaged_set_checkpoint_persist(fbe_base_config_t * base_config,
                                                                      fbe_packet_t * packet,
                                                                      fbe_u64_t   metadata_offset,                                                                      
                                                                      fbe_u64_t   second_metadata_offset,                                                                      
                                                                      fbe_u64_t   checkpoint);

fbe_status_t fbe_base_config_metadata_nonpaged_persist(fbe_base_config_t * base_config, fbe_packet_t * packet);
fbe_status_t fbe_base_config_metadata_nonpaged_persist_sync(fbe_base_config_t * base_config, fbe_packet_t * packet);

fbe_status_t fbe_base_config_metadata_nonpaged_update(fbe_base_config_t * base_config, fbe_packet_t * packet);
fbe_status_t fbe_base_config_metadata_nonpaged_read_persist(fbe_base_config_t * base_config, fbe_packet_t * packet);

fbe_status_t
fbe_base_config_metadata_paged_set_bits(fbe_base_config_t * base_config,
                                        fbe_packet_t * packet,
                                        fbe_u64_t   metadata_offset,
                                        fbe_u8_t *  metadata_record_data,
                                        fbe_u32_t metadata_record_data_size,
                                        fbe_u64_t   metadata_repeat_count);

fbe_status_t
fbe_base_config_metadata_paged_clear_bits(fbe_base_config_t * base_config,
                                        fbe_packet_t * packet,
                                        fbe_u64_t   metadata_offset,
                                        fbe_u8_t *  metadata_record_data,
                                        fbe_u32_t metadata_record_data_size,
                                        fbe_u64_t   metadata_repeat_count);

fbe_status_t
fbe_base_config_metadata_paged_get_bits(fbe_base_config_t * base_config,
                                        fbe_packet_t * packet,
                                        fbe_u64_t   metadata_offset,
                                        fbe_u8_t *  metadata_record_data,
                                        fbe_u32_t metadata_record_data_size,
                                        fbe_base_config_control_metadata_paged_get_bits_flags_t get_bits_flags);

fbe_status_t
fbe_base_config_metadata_paged_write(fbe_base_config_t * base_config,
                                        fbe_packet_t * packet,
                                        fbe_u64_t   metadata_offset,
                                        fbe_u8_t *  metadata_record_data,
                                        fbe_u32_t   metadata_record_data_size,
                                        fbe_u64_t   metadata_repeat_count);

fbe_status_t 
fbe_base_config_metadata_paged_clear_cache(fbe_base_config_t * base_config);

fbe_status_t
fbe_base_config_metadata_nonpaged_set_bits(fbe_base_config_t * base_config,
                                        fbe_packet_t * packet,
                                        fbe_u64_t   metadata_offset,
                                        fbe_u8_t *  metadata_record_data,
                                        fbe_u32_t metadata_record_data_size,
                                        fbe_u64_t   metadata_repeat_count);

fbe_status_t fbe_base_config_metadata_nonpaged_set_bits_persist(fbe_base_config_t * base_config,
                                                                fbe_packet_t * packet,
                                                                fbe_u64_t   metadata_offset,
                                                                fbe_u8_t *  metadata_record_data,
                                                                fbe_u32_t metadata_record_data_size,
                                                                fbe_u64_t   metadata_repeat_count);

fbe_status_t
fbe_base_config_metadata_nonpaged_clear_bits(fbe_base_config_t * base_config,
                                        fbe_packet_t * packet,
                                        fbe_u64_t   metadata_offset,
                                        fbe_u8_t *  metadata_record_data,
                                        fbe_u32_t metadata_record_data_size,
                                        fbe_u64_t   metadata_repeat_count);

fbe_status_t fbe_base_config_metadata_nonpaged_set_checkpoint(fbe_base_config_t * base_config,
                                                              fbe_packet_t * packet,
                                                              fbe_u64_t   metadata_offset,
                                                              fbe_u64_t     second_metadata_offset,
                                                              fbe_u64_t   checkpoint);

fbe_status_t fbe_base_config_metadata_nonpaged_force_set_checkpoint(fbe_base_config_t * base_config,
                                                                    fbe_packet_t * packet,
                                                                    fbe_u64_t   metadata_offset,
                                                                    fbe_u64_t     second_metadata_offset,
                                                                    fbe_u64_t   checkpoint);

fbe_status_t
fbe_base_config_metadata_nonpaged_incr_checkpoint(fbe_base_config_t * base_config,
                                                fbe_packet_t * packet,
                                                fbe_u64_t   metadata_offset,
                                                fbe_u64_t   second_metadata_offset,
                                                fbe_u64_t   checkpoint,
                                                fbe_u64_t   repeat_count);
fbe_status_t
fbe_base_config_metadata_nonpaged_incr_checkpoint_no_peer(fbe_base_config_t * base_config,
                                                          fbe_packet_t * packet,
                                                          fbe_u64_t   metadata_offset,
                                                          fbe_u64_t   second_metadata_offset,
                                                          fbe_u64_t   checkpoint,
                                                          fbe_u64_t   repeat_count);

fbe_status_t fbe_base_config_metadata_get_statistics(fbe_base_config_t * base_config, fbe_packet_t * packet_p);

fbe_status_t fbe_base_config_metadata_memory_read(fbe_base_config_t *base_config,
                                                  fbe_bool_t is_peer,
                                                  fbe_u8_t *buffer, fbe_u32_t buffer_size,
                                                  fbe_u32_t *copy_size);
fbe_status_t fbe_base_config_metadata_memory_update(fbe_base_config_t *base_config,
                                                    fbe_u8_t *buffer, fbe_u8_t *mask,
                                                    fbe_u32_t offset, fbe_u32_t size);

fbe_bool_t fbe_base_config_is_active(fbe_base_config_t * base_config);
fbe_medic_action_priority_t fbe_base_config_get_downstream_edge_priority(fbe_base_config_t * base_config);
fbe_status_t fbe_base_config_set_upstream_edge_priority(fbe_base_config_t * base_config, fbe_medic_action_priority_t priority);
fbe_status_t fbe_base_config_get_downstream_geometry(fbe_base_config_t * base_config, fbe_block_edge_geometry_t * block_edge_geometry);
fbe_status_t fbe_base_config_attach_upstream_block_edge(fbe_base_config_t * base_config_p, fbe_packet_t * packet);
fbe_status_t fbe_base_config_detach_upstream_block_edge(fbe_base_config_t * base_config_p, fbe_packet_t * packet);
fbe_traffic_priority_t fbe_base_config_get_downstream_edge_traffic_priority(fbe_base_config_t * base_config_p);
fbe_status_t fbe_base_config_set_deny_operation_permission(fbe_base_config_t *base_config_p);
fbe_status_t fbe_base_config_clear_deny_operation_permission(fbe_base_config_t *base_config_p);
fbe_status_t fbe_base_config_set_permanent_destroy(fbe_base_config_t *base_config_p);
fbe_bool_t fbe_base_config_is_permanent_destroy(fbe_base_config_t *base_config_p);
fbe_status_t fbe_base_config_get_operation_permission(fbe_base_config_t * base_config_p,
                                                      fbe_base_config_operation_permission_t *operation_permission_p,
                                                      fbe_bool_t count_as_io,/*is this operation an IO or not (for power savign purposes)*/
                                                      fbe_bool_t *ok_to_proceed);

fbe_status_t fbe_base_config_get_nonpaged_metadata_ptr(fbe_base_config_t * base_config, void ** nonpaged_metadata_ptr);
fbe_status_t fbe_base_config_get_nonpaged_metadata_size(fbe_base_config_t * base_config, fbe_u32_t * nonpaged_metadata_size);

fbe_status_t fbe_base_config_set_nonpaged_metadata_ptr(fbe_base_config_t * base_config, void * nonpaged_metadata_ptr);
fbe_bool_t fbe_base_config_stripe_lock_is_started(fbe_base_config_t * base_config);

fbe_status_t fbe_base_config_event_queue_pop_event(fbe_base_config_t * base_config_p, fbe_event_t ** event_p);
fbe_status_t fbe_base_config_event_queue_push_event(fbe_base_config_t *   base_config_p, fbe_event_t * event_p);

fbe_status_t fbe_base_config_event_queue_coalesce_and_push_event(fbe_base_config_t * base_config_p, fbe_event_t * event_p, 
                                                                 fbe_u64_t chunk_range, fbe_bool_t*    queued, fbe_lba_t coalesce_boundary);
                                                    
fbe_bool_t fbe_base_config_event_queue_is_empty(fbe_base_config_t * base_config_p);
fbe_bool_t fbe_base_config_event_queue_is_empty_no_lock(fbe_base_config_t * base_config_p);
fbe_status_t fbe_base_config_event_queue_front_event(fbe_base_config_t * base_config_p, fbe_event_t ** event_p);

fbe_status_t fbe_base_config_event_queue_front_event_in_use(fbe_base_config_t * base_config_p, fbe_event_t ** event_p);

/* fbe_base_config_monitor.c */
fbe_status_t fbe_base_config_monitor(fbe_base_config_t * const base_config_p, fbe_packet_t * mgmt_packet_p);
fbe_status_t fbe_base_config_monitor_load_verify(void);
fbe_status_t fbe_base_config_check_hook(fbe_base_config_t *base_config_p,
                                       fbe_u32_t monitor_state,
                                       fbe_u32_t monitor_substate,
                                       fbe_u64_t val2,
                                       fbe_scheduler_hook_status_t *status);
fbe_lifecycle_status_t fbe_base_config_wait_for_configuration_commit_function(fbe_base_object_t * base_object_p, fbe_packet_t * packet_p);

fbe_status_t
fbe_base_config_monitor_crank_object(fbe_lifecycle_const_t * p_class_const,
									fbe_base_object_t * p_object,
									fbe_packet_t * packet);

/* fbe_base_config_usurper.c */
fbe_status_t fbe_base_config_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_base_config_create_edge(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p);
fbe_status_t fbe_base_config_destroy_edge(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p);
fbe_status_t fbe_base_config_send_attach_edge(fbe_base_config_t *base_config_p, fbe_packet_t * packet_p,
                                              fbe_block_transport_control_attach_edge_t * attach_edge);

/* fbe_base_config_executer.c */
fbe_status_t fbe_base_config_send_functional_packet(fbe_base_config_t * base_config, fbe_packet_t * packet_p, fbe_edge_index_t edge_index);

/*fbe_base_config_power_save.c*/
fbe_status_t fbe_base_config_check_if_need_to_power_save(fbe_base_config_t *base_config_p, fbe_bool_t *need_to_power_save);
fbe_status_t base_config_send_hibernation_usurper_to_servers(fbe_base_config_t *base_config_p, fbe_packet_t * packet);
fbe_bool_t base_config_is_object_power_saving_enabled(fbe_base_config_t *base_config_p);
fbe_status_t base_config_enable_object_power_save(fbe_base_config_t *base_config_p);
fbe_status_t base_config_disable_object_power_save(fbe_base_config_t *base_config_p);
fbe_status_t fbe_base_config_set_system_power_saving_info(fbe_system_power_saving_info_t *set_info);
fbe_status_t fbe_base_config_get_system_power_saving_info(fbe_system_power_saving_info_t *get_info);
fbe_status_t fbe_base_config_set_power_saving_idle_time(fbe_base_config_t * base_config_p, fbe_u64_t idle_time_in_seconds);
fbe_status_t fbe_base_config_get_power_saving_idle_time(fbe_base_config_t * base_config_p, fbe_u64_t *idle_time_in_seconds);
fbe_status_t fbe_base_config_enable_object_power_save(fbe_base_config_t* base_config_p);
fbe_status_t fbe_base_config_disable_object_power_save(fbe_base_config_t* base_config_p);
fbe_status_t fbe_base_config_usurper_get_system_power_saving_info(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p);
fbe_status_t fbe_base_config_usurper_set_system_power_saving_info(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p);
fbe_status_t fbe_base_config_get_object_power_save_info(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p);
fbe_status_t fbe_base_config_send_wake_up_usurper_to_servers(fbe_base_config_t *base_config_p, fbe_packet_t * packet);
fbe_status_t fbe_base_config_send_power_save_enable_usurper_to_servers(fbe_base_config_t *base_config_p, fbe_packet_t * packet);
fbe_status_t fbe_base_config_set_object_power_save_idle_time(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p);
fbe_status_t fbe_base_config_power_save_verify_servers_not_slumber(fbe_base_config_t *base_config_p);
fbe_status_t base_config_send_spindown_qualified_usurper_to_servers(fbe_base_config_t *base_config_p, fbe_packet_t * packet);
fbe_status_t fbe_base_config_set_power_saving_idle_time_usurper_to_servers(fbe_base_config_t *base_config_p, fbe_packet_t * packet);
fbe_status_t fbe_base_config_set_system_encryption_info(fbe_system_encryption_info_t *set_info);
fbe_status_t fbe_base_config_get_system_encryption_info(fbe_system_encryption_info_t *get_info);
fbe_status_t fbe_base_config_usurper_get_system_encryption_info(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p);
fbe_status_t fbe_base_config_usurper_set_system_encryption_info(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p);

/* fbe_base_config_io_quiesce.c */
fbe_status_t fbe_base_config_quiesce_io_requests(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p, fbe_bool_t b_is_object_in_pass_thru, fbe_bool_t * is_io_request_quiesced_b);
fbe_status_t fbe_base_config_quiesce_io_is_metadata_memory_updated(fbe_base_config_t * base_config_p, fbe_bool_t * out_md_mem_updated_p);
fbe_status_t fbe_base_config_unquiesce_io_requests(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p, fbe_bool_t * is_io_request_unquiesce_ready_b);
fbe_status_t fbe_base_config_get_quiesced_io_count(fbe_base_config_t  *base_config_p,
                                                   fbe_u32_t *number_of_ios_outstanding_p);
fbe_bool_t fbe_base_config_should_quiesce_drain(fbe_base_config_t * base_config_p);

/*entry for events that come directly from the block trasnport and not the edge(e.g., we have an IO we need to deque*/
fbe_status_t fbe_base_config_process_block_transport_event(fbe_block_transport_event_type_t event_type, fbe_block_trasnport_event_context_t context);

/* fbe_base_config_memory.c */
fbe_status_t fbe_base_config_memory_allocate_chunks(fbe_base_config_t * base_config_p,
                                                    fbe_packet_t * packet_p,
                                                    fbe_base_config_memory_allocation_chunks_t mem_alloc_chunks,
                                                    fbe_memory_completion_function_t memory_completion_function);

fbe_status_t fbe_base_config_memory_assign_address_from_allocated_chunks(fbe_base_config_t * base_config_p,
                                                                         fbe_memory_request_t * memory_request_p,
                                                                         fbe_base_config_memory_alloc_chunks_address_t *mem_alloc_chunks_addr);

fbe_status_t fbe_base_config_get_default_offset(fbe_base_config_t * base_config_p, fbe_lba_t *default_offset);
fbe_bool_t fbe_base_config_is_peer_present(fbe_base_config_t *base_config_p);
fbe_bool_t fbe_base_config_has_peer_joined(fbe_base_config_t *base_config_p);

fbe_status_t fbe_base_config_nonpaged_metadata_set_background_op_bitmask(fbe_base_config_nonpaged_metadata_t *base_config_nonpaged_metadata_p, 
                                                                        fbe_base_config_background_operation_t background_op_bitmask);
fbe_status_t fbe_base_config_nonpaged_metadata_get_background_op_bitmask(fbe_base_config_t *base_config_p, 
                                                                        fbe_base_config_background_operation_t *background_op_bitmask);
fbe_status_t fbe_base_config_enable_background_operation(fbe_base_config_t *base_config_p,
                                                                  fbe_packet_t*  packet_p, fbe_base_config_background_operation_t background_op_bitmask);
fbe_status_t fbe_base_config_disable_background_operation(fbe_base_config_t *base_config_p,
                                                                  fbe_packet_t*  packet_p, fbe_base_config_background_operation_t background_op_bitmask);
fbe_status_t fbe_base_config_is_background_operation_enabled(fbe_base_config_t *base_config_p,
                                                               fbe_base_config_background_operation_t background_op_bitmask, fbe_bool_t*  is_enabled);
fbe_status_t fbe_base_config_release_np_distributed_lock(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p);
fbe_status_t fbe_base_config_get_np_distributed_lock(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p);

fbe_bool_t fbe_base_config_is_system_background_zeroing_enabled(void);
fbe_status_t fbe_base_config_disable_system_background_zeroing(void);
fbe_bool_t fbe_base_config_is_system_bg_service_enabled(fbe_base_config_control_system_bg_service_flags_t bgs_flag);
fbe_status_t fbe_base_config_get_system_bg_service_status(fbe_base_config_control_system_bg_service_t* system_bg_service_p);
fbe_status_t fbe_base_config_write_verify_non_paged_metadata_without_NP_lock(fbe_base_config_t *base_config_p,fbe_packet_t * packet_p);

/* completion context passing through fbe_base_config_write_verify */
typedef struct fbe_base_config_write_verify_context_s{
    fbe_bool_t                            write_verify_op_success_b;
    fbe_bool_t           write_verify_operation_retryable_b;
    fbe_base_config_t * base_config_p;
}fbe_base_config_write_verify_context_t;
fbe_status_t fbe_base_config_ex_metadata_nonpaged_write_verify(fbe_base_config_write_verify_context_t* base_config_write_verify_context,fbe_packet_t * packet);


fbe_status_t fbe_base_config_init_nonpaged_metadata_version_header(fbe_base_config_nonpaged_metadata_t *nonpaged_metadata_p, 
                                                                   fbe_class_id_t class_id);

fbe_status_t fbe_base_config_passive_request(fbe_base_config_t * base_config);


fbe_bool_t fbe_base_config_is_load_balance_enabled(void);

fbe_lifecycle_status_t base_config_check_power_save_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

fbe_status_t fbe_base_config_send_specialize_notification(fbe_base_config_t * base_config);

#endif /* BASE_CONFIG_PRIVATE_H */

/*******************************
 * end fbe_base_config_private.h
 *******************************/

