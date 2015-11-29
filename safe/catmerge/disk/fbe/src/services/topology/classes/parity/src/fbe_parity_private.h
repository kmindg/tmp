#ifndef PARITY_PRIVATE_H
#define PARITY_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_parity_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains private defines for the parity object.
 *  This includes the definitions of the
 *  @ref fbe_parity_t "parity" itself, as well as the associated
 *  structures and defines.
 * 
 * @ingroup parity_class_files
 * 
 * @version
 *   05/20/2009:  Created. RPF
 *
 ***************************************************************************/

#include "fbe_raid_group_object.h"
#include "fbe/fbe_transport.h"
#include "fbe_block_transport.h"
#include "fbe/fbe_parity.h"
#include "fbe_raid_geometry.h"
#include "fbe_raid_library.h"
#include "fbe_parity_write_log.h"

/******************************
 * LITERAL DEFINITIONS.
 ******************************/

typedef enum fbe_parity_e
{
    FBE_PARITY_FLAG_IO_PACKET_CANCELLED = 0x1,
    FBE_PARITY_FLAG_SIZES_INITIALIZED = 0x2
}
fbe_parity_enum_t;

/* Lifecycle definitions
 * Define the parity lifecycle's instance.
 */
extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(parity);

/* These are the lifecycle condition ids for a sas physical drive object. */

/*! @enum fbe_parity_lifecycle_cond_id_t  
 *  
 *  @brief These are the lifecycle conditions for the parity object. 
 */
typedef enum fbe_parity_lifecycle_cond_id_e 
{
    /*! The identity of the parity is not known.  
     *  Issue a command to the PDO to fetch the identity and save it in
     *  the parity.
     */
	FBE_PARITY_LIFECYCLE_COND_IDENTITY_UNKNOWN = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_PARITY),

    /*! The edge is not enabled, we will try to enable it.
     */
    FBE_PARITY_LIFECYCLE_COND_BLOCK_EDGE_DISABLED,

    /*! The block edge is attached.  We will try to detach the edge.
     */
    FBE_PARITY_LIFECYCLE_COND_BLOCK_EDGE_ATTACHED,

    /*! There are clients connected.  See if they have detached yet.
     */
    FBE_PARITY_LIFECYCLE_COND_CLIENTS_CONNECTED,

	FBE_PARITY_LIFECYCLE_COND_LAST /* must be last */
} 
fbe_parity_lifecycle_cond_id_t;

/* Forward declaration for visibility.
 */
struct fbe_parity_s;

/*! @def FBE_PARITY_MAX_EDGES 
 *  @brief this is the max number of edges we will allow for the parity.
 *         We set this to 16 since flare only supports widths up to 16.
 *         It also bounds the amount of memory we need to allocate for edges.
 */
#define FBE_PARITY_MAX_EDGES (FBE_PARITY_MAX_WIDTH)

/*!****************************************************************************
 *    
 * @struct fbe_parity_t
 *  
 * @brief 
 *   This is the definition of the parity object.
 *   This object represents a parity, which is an object which can
 *   perform block size conversions.
 ******************************************************************************/

typedef struct fbe_parity_s
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
    fbe_object_id_t object_id[FBE_PARITY_MAX_EDGES];

    /*! Generic flags field.
     */
    fbe_parity_enum_t flags;

    /*!Pointer to write log info -- slot array now too big to fit into parity object memory chunk
     */
    fbe_parity_write_log_info_t *write_log_info_p;

    /* Lifecycle defines.
     */
    FBE_LIFECYCLE_DEF_INST_DATA;
	FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_PARITY_LIFECYCLE_COND_LAST));
}
fbe_parity_t;

/*!****************************************************************************
 *    
 * @struct fbe_parity_metadata_positions_s
 *  
 * @brief 
 *   This is the definition of the parity metadata positions.
 *   This structure stores an location of the medatadata on drive for the
 *   raid group object.
 ******************************************************************************/
typedef struct fbe_parity_metadata_positions_s
{
    /*! It holds location of the signature metadata on disk. 
     */
	fbe_lba_t			signature_metadata_lba; 
	
    /*! It holds location of the nonpaged metadata on disk. 
     */    	
	fbe_lba_t			nonpaged_metadata_lba;

    /*! It holds location of the nonpaged metadata block count. 
     */    	
	fbe_block_count_t	nonpaged_metadata_block_count;

    /*! It holds location of the paged metadata on disk. 
     */    	
	fbe_lba_t			paged_metadata_lba;

    /*! It holds location of the paged metadata block count. 
     */    	
    fbe_block_count_t	paged_metadata_block_count;

    /*! If metadata is mirrored this offset is used to 
     * calculate mirrored positions.
     */
	fbe_lba_t			mirror_metadata_offset; 

} fbe_parity_metadata_positions_t;

/*! @fn fbe_parity_lock(fbe_parity_t *const parity_p) 
 *  
 * @brief 
 *   Function which locks the parity via the base object lock.
 */
static __forceinline void
fbe_parity_lock(fbe_parity_t *const parity_p)
{
    fbe_base_object_lock((fbe_base_object_t *)parity_p);
    return;
}

/* @fn fbe_parity_unlock(fbe_parity_t *const parity_p) 
 *  
 * @brief 
 *   Function which unlocks the parity via the base object lock.
 */
static __forceinline void
fbe_parity_unlock(fbe_parity_t *const parity_p)
{
    fbe_base_object_unlock((fbe_base_object_t *)parity_p);
    return;
}
/* @fn fbe_parity_set_flag(fbe_parity_t *const parity_p,
 *                                const fbe_parity_flags_t flag)
 *  
 * @brief 
 *   Function which sets a parity flag.
 */
static __forceinline void
fbe_parity_set_flag(fbe_parity_t *const parity_p,
                     const fbe_parity_flags_t flag)
{
    parity_p->flags |= flag;
    return;
}
/* @fn fbe_parity_clear_flag(fbe_parity_t *const parity_p,
 *                                  const fbe_parity_flags_t flag)
 *  
 * @brief 
 *   Function which clears a parity flag.
 */
static __forceinline void
fbe_parity_clear_flag(fbe_parity_t *const parity_p,
                       const fbe_parity_flags_t flag)
{
    parity_p->flags &= ~flag;
    return;
}
/* @fn fbe_parity_is_flag_set()
 *  
 * @brief 
 *   Function which determines if a flag is set.
 */
static __forceinline fbe_bool_t
fbe_parity_is_flag_set(fbe_parity_t *const parity_p,
                        const fbe_parity_flags_t flag)
{
    return((parity_p->flags & flag) != 0);
}


/* Imports for visibility.
 */

/* fbe_parity_main.c */

fbe_status_t fbe_parity_init(fbe_parity_t * const parity_p);
fbe_status_t fbe_parity_destroy( fbe_object_handle_t object_handle);
fbe_status_t fbe_parity_destroy_interface( fbe_object_handle_t object_handle);
fbe_status_t fbe_parity_init_members(fbe_parity_t * const parity_p);
fbe_status_t fbe_parity_event_entry(fbe_object_handle_t object_handle, 
                                    fbe_event_type_t event,
                                    fbe_event_context_t event_context);
/* fbe_parity_monitor.c */
fbe_status_t fbe_parity_monitor(fbe_parity_t * const parity_p, 
                                     fbe_packet_t * mgmt_packet_p);
fbe_status_t fbe_parity_monitor_load_verify(void);

/* fbe_parity_usurper.c */
fbe_status_t fbe_parity_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);

/* fbe_parity_executor.c
 */
fbe_status_t fbe_parity_block_transport_entry(fbe_transport_entry_context_t context, fbe_packet_t * packet);

fbe_status_t fbe_parity_class_control_entry(fbe_packet_t * packet);

fbe_status_t  
fbe_parity_class_get_metadata_positions (fbe_lba_t edge_capacity, 
        fbe_block_size_t block_size,
        fbe_parity_metadata_positions_t * parity_metadata_positions);

#endif /* PARITY_PRIVATE_H */

/*******************************
 * end fbe_parity_private.h
 *******************************/
