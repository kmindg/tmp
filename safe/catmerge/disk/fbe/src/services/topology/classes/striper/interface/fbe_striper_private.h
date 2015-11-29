#ifndef STRIPER_PRIVATE_H
#define STRIPER_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_striper_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains private defines for the striper object.
 *  This includes the definitions of the
 *  @ref fbe_striper_t "striper" itself, as well as the associated
 *  structures and defines.
 * 
 * @ingroup striper_class_files
 * 
 * @version
 *   05/20/2009:  Created. RPF
 *
 ***************************************************************************/

#include "fbe_raid_group_object.h"
#include "fbe/fbe_transport.h"
#include "fbe_block_transport.h"
#include "fbe/fbe_striper.h"
#include "fbe_raid_geometry.h"
#include "fbe_raid_library.h"

/******************************
 * LITERAL DEFINITIONS.
 ******************************/

typedef enum fbe_striper_e
{
    FBE_striper_FLAG_IO_PACKET_CANCELLED = 0x1,
    FBE_striper_FLAG_SIZES_INITIALIZED = 0x2
}
fbe_striper_enum_t;


/* Lifecycle definitions
 * Define the striper lifecycle's instance.
 */
extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(striper);

/* These are the lifecycle condition ids for a sas physical drive object. */

/*! @enum fbe_striper_lifecycle_cond_id_t  
 *  
 *  @brief These are the lifecycle conditions for the striper object. 
 */
typedef enum fbe_striper_lifecycle_cond_id_e 
{
    /*! The identity of the striper is not known.  
     *  Issue a command to the PDO to fetch the identity and save it in
     *  the striper.
     */
	FBE_STRIPER_LIFECYCLE_COND_IDENTITY_UNKNOWN = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_STRIPER),

    /*! The edge is not enabled, we will try to enable it.
     */
    FBE_STRIPER_LIFECYCLE_COND_BLOCK_EDGE_DISABLED,

    /*! The block edge is attached.  We will try to detach the edge.
     */
    FBE_STRIPER_LIFECYCLE_COND_BLOCK_EDGE_ATTACHED,

    /*! There are clients connected.  See if they have detached yet.
     */
    FBE_STRIPER_LIFECYCLE_COND_CLIENTS_CONNECTED,

	FBE_STRIPER_LIFECYCLE_COND_LAST /* must be last */
} 
fbe_striper_lifecycle_cond_id_t;

/* Forward declaration for visibility.
 */
struct fbe_striper_s;

/*! @def FBE_STRIPER_MAX_EDGES 
 *  @brief this is the max number of edges we will allow for the striper.
 *         We set this to 16 since flare only supports widths of 16.         
 *         It also bounds the amount of memory we need to allocate for edges.
 */
#define FBE_STRIPER_MAX_EDGES (FBE_STRIPER_MAX_WIDTH)

/*!****************************************************************************
 *    
 * @struct fbe_striper_t
 *  
 * @brief 
 *   This is the definition of the striper object.
 *   This object represents a striper, which is an object which can
 *   perform block size conversions.
 ******************************************************************************/

typedef struct fbe_striper_s
{    
    /*! This must be first.  This is the object we inherit from.
     */
    fbe_raid_group_t raid_group;

    /*! This is our storage extent protocol edge to the physical drive. 
     */
    fbe_block_edge_t *block_edge_p;

    /*! This is the object id of the virtual drive. 
     *  The lifecycle and usurper code needs this in order to
     *  issue usurper commands for things like attaching/detaching the edge.
     */
    /* fbe_object_id_t object_id[FBE_STRIPER_MAX_EDGES]; */

	/* fbe_block_edge_t block_edge_array[FBE_STRIPER_MAX_EDGES]; */

    /*! Generic flags field.
     */
    fbe_striper_enum_t flags;

    /* Lifecycle defines.
     */
    FBE_LIFECYCLE_DEF_INST_DATA;
	FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_STRIPER_LIFECYCLE_COND_LAST));
}
fbe_striper_t;


/*! @fn fbe_striper_lock(fbe_striper_t *const striper_p) 
 *  
 * @brief 
 *   Function which locks the striper via the base object lock.
 */
static __forceinline void
fbe_striper_lock(fbe_striper_t *const striper_p)
{
    fbe_base_object_lock((fbe_base_object_t *)striper_p);
    return;
}

/* @fn fbe_striper_unlock(fbe_striper_t *const striper_p) 
 *  
 * @brief 
 *   Function which unlocks the striper via the base object lock.
 */
static __forceinline void
fbe_striper_unlock(fbe_striper_t *const striper_p)
{
    fbe_base_object_unlock((fbe_base_object_t *)striper_p);
    return;
}
/* @fn fbe_striper_set_flag(fbe_striper_t *const striper_p,
 *                                const fbe_striper_flags_t flag)
 *  
 * @brief 
 *   Function which sets a striper flag.
 */
static __forceinline void
fbe_striper_set_flag(fbe_striper_t *const striper_p,
                     const fbe_striper_flags_t flag)
{
    striper_p->flags |= flag;
    return;
}
/* @fn fbe_striper_clear_flag(fbe_striper_t *const striper_p,
 *                                  const fbe_striper_flags_t flag)
 *  
 * @brief 
 *   Function which clears a striper flag.
 */
static __forceinline void
fbe_striper_clear_flag(fbe_striper_t *const striper_p,
                       const fbe_striper_flags_t flag)
{
    striper_p->flags &= ~flag;
    return;
}
/* @fn fbe_striper_is_flag_set(fbe_striper_t *const striper_p,
 *                                  const fbe_striper_flags_t flag)
 *  
 * @brief 
 *   Function which determines if a flag is set.
 */
static __forceinline fbe_bool_t
fbe_striper_is_flag_set(fbe_striper_t *const striper_p,
                        const fbe_striper_flags_t flag)
{
    return((striper_p->flags & flag) != 0);
}

/* Imports for visibility.
 */

/* fbe_striper_main.c */

fbe_status_t fbe_striper_init(fbe_striper_t * const striper_p);
fbe_status_t fbe_striper_destroy( fbe_object_handle_t object_handle);
fbe_status_t fbe_striper_destroy_interface( fbe_object_handle_t object_handle);
fbe_status_t fbe_striper_init_members(fbe_striper_t * const striper_p);
fbe_status_t fbe_striper_event_entry(fbe_object_handle_t object_handle, 
                                    fbe_event_type_t event,
                                    fbe_event_context_t event_context);

fbe_status_t fbe_striper_state_change_event_entry (fbe_striper_t* const lun_p, fbe_event_context_t event_context);
fbe_base_config_downstream_health_state_t fbe_striper_verify_downstream_health (fbe_striper_t * striper_p);
fbe_status_t fbe_striper_set_condition_based_on_downstream_health (fbe_striper_t  *striper_p, 
                                                               fbe_base_config_downstream_health_state_t downstream_health_state);

/* fbe_striper_monitor.c */
fbe_status_t fbe_striper_monitor(fbe_striper_t * const striper_p, 
                                 fbe_packet_t * mgmt_packet_p);
fbe_status_t fbe_striper_monitor_load_verify(void);

/* fbe_striper_usurper.c */
fbe_status_t fbe_striper_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_striper_class_control_entry(fbe_packet_t * packet);
fbe_status_t fbe_striper_create_subpackets_and_send_down_to_mirror_objects(fbe_packet_t * packet_p,
                                                                    fbe_base_object_t*  base_object_p,
                                                                    fbe_u16_t         data_disks);

fbe_status_t fbe_striper_send_down_to_mirror_objects_memory_allocation_completion(fbe_memory_request_t * memory_request_p, 
                                                                                  fbe_memory_completion_context_t in_context);

fbe_status_t fbe_striper_initiate_verify_subpacket_completion(fbe_packet_t* packet_p,
                                                       fbe_packet_completion_context_t context);

fbe_status_t fbe_striper_send_command_to_mirror_objects_completion(fbe_packet_t* packet_p,
                                                       fbe_packet_completion_context_t context);

static fbe_status_t fbe_striper_get_zero_checkpoint_raid10_extent(fbe_raid_group_t * raid_group_p,
                                               fbe_packet_t * packet_p);

static fbe_status_t fbe_striper_get_zero_checkpoint_for_downstream_mirror_objects(fbe_raid_group_t * raid_group_p,
                                                          fbe_packet_t * packet_p);

static fbe_status_t fbe_striper_get_zero_checkpoint_downstream_mirror_object_allocation_completion(fbe_memory_request_t * memory_request_p,
                                                                           fbe_memory_completion_context_t context);

static fbe_status_t fbe_striper_get_zero_checkpoint_downstream_mirror_object_subpacket_completion(fbe_packet_t * packet_p,
                                                                          fbe_packet_completion_context_t context);


/* fbe_striper_executor.c
 */
fbe_status_t fbe_striper_block_transport_entry(fbe_transport_entry_context_t context, fbe_packet_t * packet);

#endif /* STRIPER_PRIVATE_H */

/*******************************
 * end fbe_striper_private.h
 *******************************/
