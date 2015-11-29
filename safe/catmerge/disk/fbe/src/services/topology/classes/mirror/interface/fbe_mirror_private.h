#ifndef MIRROR_PRIVATE_H
#define MIRROR_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_mirror_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains private defines for the mirror object.
 *  This includes the definitions of the
 *  @ref fbe_mirror_t "mirror" itself, as well as the associated
 *  structures and defines.
 * 
 * @ingroup mirror_class_files
 * 
 * @version
 *   05/20/2009:  Created. RPF
 *
 ***************************************************************************/

#include "fbe_raid_group_object.h"
#include "fbe/fbe_transport.h"
#include "fbe_block_transport.h"
#include "fbe/fbe_mirror.h"
#include "fbe_raid_library.h"
#include "fbe_mirror_private.h"

/******************************
 * LITERAL DEFINITIONS.
 ******************************/
/*! @def FBE_MIRROR_MAX_EDGES 
 *  @brief this is the max number of edges we will allow for the mirror. 
 */
#define FBE_MIRROR_MAX_EDGES    (FBE_MIRROR_MAX_WIDTH)


typedef enum fbe_mirror_e
{
    FBE_MIRROR_FLAG_IO_PACKET_CANCELLED = 0x1,
    FBE_MIRROR_FLAG_SIZES_INITIALIZED = 0x2
}
fbe_mirror_enum_t;


/* Lifecycle definitions
 * Define the mirror lifecycle's instance.
 */
extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(mirror);

/* These are the lifecycle condition ids for a sas physical drive object. */

/*! @enum fbe_mirror_lifecycle_cond_id_t  
 *  
 *  @brief These are the lifecycle conditions for the mirror object. 
 */
typedef enum fbe_mirror_lifecycle_cond_id_e 
{
    /*! The identity of the mirror is not known.  
     *  Issue a command to the PDO to fetch the identity and save it in
     *  the mirror.
     */
	FBE_MIRROR_LIFECYCLE_COND_IDENTITY_UNKNOWN = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_MIRROR),

    /*! The edge is not enabled, we will try to enable it.
     */
    FBE_MIRROR_LIFECYCLE_COND_BLOCK_EDGE_DISABLED,

    /*! The block edge is attached.  We will try to detach the edge.
     */
    FBE_MIRROR_LIFECYCLE_COND_BLOCK_EDGE_ATTACHED,

    /*! There are clients connected.  See if they have detached yet.
     */
    FBE_MIRROR_LIFECYCLE_COND_CLIENTS_CONNECTED,

	FBE_MIRROR_LIFECYCLE_COND_LAST /* must be last */
} 
fbe_mirror_lifecycle_cond_id_t;

/*!****************************************************************************
 *    
 * @struct fbe_mirror_t
 *  
 * @brief 
 *   This is the definition of the mirror object.
 *   This object represents a mirror, which is an object which can
 *   perform block size conversions.
 ******************************************************************************/

typedef struct fbe_mirror_s
{    
    /*! This must be first.  This is the object we inherit from.
     */
    fbe_raid_group_t raid_group;

    /*! This is our storage extent protocol edge to the physical drive. 
     */
    /* fbe_block_edge_t *block_edge_p; */

    /*! Storage for block edges is in the mirror object.
     */
    fbe_block_edge_t block_edge_array[FBE_MIRROR_MAX_EDGES];
    
    /*! Storage for mirror optimization database structures.
     */
    fbe_mirror_optimization_t mirror_opt_db;

    /*! Generic flags field.
     */
    fbe_mirror_enum_t flags;

    /* Lifecycle defines.
     */
    FBE_LIFECYCLE_DEF_INST_DATA;
	FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_MIRROR_LIFECYCLE_COND_LAST));
}
fbe_mirror_t;


/*! @fn fbe_mirror_lock(fbe_mirror_t *const mirror_p) 
 *  
 * @brief 
 *   Function which locks the mirror via the base object lock.
 */
static __forceinline void
fbe_mirror_lock(fbe_mirror_t *const mirror_p)
{
    fbe_base_object_lock((fbe_base_object_t *)mirror_p);
    return;
}

/* @fn fbe_mirror_unlock(fbe_mirror_t *const mirror_p) 
 *  
 * @brief 
 *   Function which unlocks the mirror via the base object lock.
 */
static __forceinline void
fbe_mirror_unlock(fbe_mirror_t *const mirror_p)
{
    fbe_base_object_unlock((fbe_base_object_t *)mirror_p);
    return;
}



/* @fn fbe_mirror_set_flag(fbe_mirror_t *const mirror_p,
 *                                const fbe_mirror_flags_t flag)
 *  
 * @brief 
 *   Function which sets a mirror flag.
 */
static __forceinline void
fbe_mirror_set_flag(fbe_mirror_t *const mirror_p,
                           const fbe_mirror_flags_t flag)
{
    mirror_p->flags |= flag;
    return;
}
/* @fn fbe_mirror_clear_flag(fbe_mirror_t *const mirror_p,
 *                                  const fbe_mirror_flags_t flag)
 *  
 * @brief 
 *   Function which clears a mirror flag.
 */
static __forceinline void
fbe_mirror_clear_flag(fbe_mirror_t *const mirror_p,
                           const fbe_mirror_flags_t flag)
{
    mirror_p->flags &= ~flag;
    return;
}
/* @fn fbe_mirror_is_flag_set(fbe_mirror_t *const mirror_p,
 *                                  const fbe_mirror_flags_t flag)
 *  
 * @brief 
 *   Function which determines if a flag is set.
 */
static __forceinline fbe_bool_t
fbe_mirror_is_flag_set(fbe_mirror_t *const mirror_p,
                              const fbe_mirror_flags_t flag)
{
    return((mirror_p->flags & flag) != 0);
}


/* 
 *  Imports for visibility.
 */
/**************************************** 
    fbe_mirror_executor.
 ****************************************/
fbe_status_t fbe_mirror_block_transport_entry(fbe_transport_entry_context_t context, fbe_packet_t * packet);

/**************************************** 
    fbe_mirror_main.c 
 ****************************************/
fbe_status_t fbe_mirror_monitor_load_verify(void);
fbe_status_t fbe_mirror_init(fbe_mirror_t * const mirror_p);
fbe_status_t fbe_mirror_destroy( fbe_object_handle_t object_handle);
fbe_status_t fbe_mirror_destroy_interface( fbe_object_handle_t object_handle);
fbe_status_t fbe_mirror_init_members(fbe_mirror_t * const mirror_p);
static fbe_status_t fbe_mirror_state_change_event_entry(fbe_mirror_t * const mirror_p,
                                                 fbe_event_context_t event_context);

/**************************************** 
    fbe_mirror_monitor.c 
 ****************************************/
fbe_status_t fbe_mirror_monitor(fbe_mirror_t * const mirror_p, 
                                fbe_packet_t * mgmt_packet_p);

/**************************************** 
    fbe_mirror_usurper.c 
 ****************************************/
fbe_status_t fbe_mirror_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_mirror_class_control_entry(fbe_packet_t * packet);

/**************************************** 
    fbe_mirror_event.c 
 ****************************************/
fbe_status_t fbe_mirror_event_entry(fbe_object_handle_t object_handle, 
                                    fbe_event_type_t event,
                                    fbe_event_context_t event_context);

#endif /* MIRROR_PRIVATE_H */

/*******************************
 * end fbe_mirror_private.h
 *******************************/
