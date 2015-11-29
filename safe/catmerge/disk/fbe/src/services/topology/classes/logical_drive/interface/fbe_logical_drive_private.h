#ifndef LOGICAL_DRIVE_PRIVATE_H
#define LOGICAL_DRIVE_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains private defines for the logical drive object.
 *  This includes the definitions of the
 *  @ref fbe_logical_drive_t "logical drive" itself, as well as the associated
 *  structures and defines.
 * 
 * @ingroup logical_drive_class_files
 * 
 * HISTORY
 *   10/29/2007:  Created. RPF
 *
 ***************************************************************************/

#include "fbe_base_object.h"
#include "base_object_private.h"
#include "fbe/fbe_transport.h"
#include "../../base_discovered/base_discovered_private.h"
#include "fbe_block_transport.h"
#include "fbe/fbe_logical_drive.h"

/******************************
 * LITERAL DEFINITIONS.
 ******************************/

/*!********************************************************************** 
 * @def FBE_LDO_MAX_SG_LENGTH
 *  
 * @brief 
 *  This is the maximum sg list length we will be allowed. This value is
 *  determined by limits in the backend drivers. Eventually we will allow this
 *  to be 2049, once we are allowed to have a pool of memory, but for now we are
 *  limited by the max chunk size.
 ************************************************************************/
#define FBE_LDO_MAX_SG_LENGTH 2049

/*! @def FBE_LDO_BITBUCKET_MAX_BLOCK_SIZE
 *  @brief 
 *   This is the block size that we optimize for with the bitbucket.
 *   Note that we can use block sizes larger or smaller than this with the
 *   bitbucket, but this size and smaller is the size we will optimize for.
 */ 
#define FBE_LDO_BITBUCKET_MAX_BLOCK_SIZE (4 * 1024)

/*! @def FBE_LDO_BITBUCKET_BYTES 
 *  @brief
 *   We define the size of the bitbucket as
 *   2 * @ref FBE_LDO_BITBUCKET_MAX_BLOCK_SIZE in order to optimize for 4k
 *   blocks or smaller. Some 4k I/Os will need nearly 2 * 4k worth of edges.
 *
 *   In general, this should be two times the max block size we want to optimize
 *   for since we might need bitbucket for both the leading and trailing edges.
 */
#define FBE_LDO_BITBUCKET_BYTES (2 * FBE_LDO_BITBUCKET_MAX_BLOCK_SIZE)

/*! @def FBE_LDO_MAX_OPTIMUM_BLOCK_SIZE
 *  @brief
 *   This is the maximum optimum block size we will ever expect to see.
 *   We set this artificially to protect against getting values that
 *   are much larger than we would have ever tested.
 *   Since 4k is the largest optimum block we are testing with, we set it
 *   to the optimum block size for 4k.
 */
#define FBE_LDO_MAX_OPTIMUM_BLOCK_SIZE (512 * 4096)

/*! @struct fbe_ldo_bitbucket_t 
 *  @brief
 *    This bitbucket is used for buffering edges on read operations or for
 *    filling in unused space on "lossy" writes.
 *  
 *    The bitbucket has two uses.
 *  
 *    It is used to buffer read data which is being thrown away. There is no
 *    locking since the data is not used and thus multiple instances of the
 *    logical drive are free to access it at the same time.
 *  
 *    It is also used for "lossy" cases where the imported and exported optimal
 *    block sizes do not match.  In these cases we need "fill" for the unused
 *    area on write operations.  This bitbucket is filled with a pre-determined
 *    pattern at init time and then used on "lossy" write operations to fill in
 *    the unused space.
 */
typedef struct
{
    /*! bytes contains the total @ref FBE_LDO_BITBUCKET_BYTES of the bitbucket.
     */
    fbe_u8_t bytes[FBE_LDO_BITBUCKET_BYTES];
}   
fbe_ldo_bitbucket_t;

/*! @struct fbe_ldo_supported_block_size_entry_t
 * @brief 
 *   This structure represents all the information needed to 
 *   represent/differentiate a block size that we support. 
 */
typedef struct
{
    fbe_block_size_t imported_block_size;  /*!< Size in bytes of block to import. */
    fbe_block_size_t exported_block_size;  /*!< Size in bytes of block to export. */
    /*! Size in number of imported blocks for an optimum block. 
     * An optimum block is the number of blocks needed to perform edgeless I/Os.
     */
    fbe_block_size_t imported_optimum_block_size;

    /*! Size in number of exported blocks for an optimum block. 
     * An optimum block is the number of blocks needed to perform edgeless I/Os.
     */
    fbe_block_size_t exported_optimum_block_size;

    /*! The imported optimum block size to use for this configuration.
     */
    fbe_block_size_t imported_optimum_block_size_to_configure;
} fbe_ldo_supported_block_size_entry_t;


/*! @enum fbe_ldo_supported_block_size_enum_t
 *  @brief
 *  These are the default imported block sizes we support.
 *  FBE_LDO_BLOCK_SIZE_LAST is the last size we support.
 *  This allows us to optimize our calculations for optimal block sizes.
 */
typedef enum fbe_ldo_supported_block_size_enum_e
{
    FBE_LDO_BLOCK_SIZE_520,
    FBE_LDO_BLOCK_SIZE_512,
    FBE_LDO_BLOCK_SIZE_4096,
    FBE_LDO_BLOCK_SIZE_4160,
    FBE_LDO_BLOCK_SIZE_4104,
    FBE_LDO_BLOCK_SIZE_LAST
}
fbe_ldo_supported_block_size_enum_t;

typedef enum fbe_ldo_flags_e
{
    FBE_LDO_FLAG_IO_PACKET_CANCELLED = 0x1,
    FBE_LDO_FLAG_SIZES_INITIALIZED = 0x2
}
fbe_ldo_flags_enum_t;


/* Lifecycle definitions
 * Define the logical drive lifecycle's instance.
 */
extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(logical_drive);
/* These are the lifecycle condition ids for a sas physical drive object. */

/*! @enum fbe_logical_drive_lifecycle_cond_id_t  
 *  
 *  @brief These are the lifecycle conditions for the logical drive object. 
 */
typedef enum fbe_logical_drive_lifecycle_cond_id_e 
{
    /*! The identity of the logical drive is not known.  
     *  Issue a command to the PDO to fetch the identity and save it in
     *  the logical drive.
     */
	FBE_LOGICAL_DRIVE_LIFECYCLE_COND_IDENTITY_UNKNOWN = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_LOGICAL_DRIVE),

    /*! The edge is not attached.  We will try to attach the edge.
     */
    FBE_LOGICAL_DRIVE_LIFECYCLE_COND_BLOCK_EDGE_DETACHED,

    /*! The edge is not enabled, we will try to enable it.
     */
    FBE_LOGICAL_DRIVE_LIFECYCLE_COND_BLOCK_EDGE_DISABLED,

    /*! The identity has not been validated yet, we will 
     *  fetch the identity information and compare it to the
     *  identity information we fetched in the identity_unknown condition.
     */
    FBE_LOGICAL_DRIVE_LIFECYCLE_COND_IDENTITY_NOT_VALIDATED,

    /*! The block edge is attached.  We will try to detach the edge.
     */
    FBE_LOGICAL_DRIVE_LIFECYCLE_COND_BLOCK_EDGE_ATTACHED,

    /*! There are clients connected.  See if they have detached yet.
     */
    FBE_LOGICAL_DRIVE_LIFECYCLE_COND_CLIENTS_CONNECTED,

	/* This condition is preset in activate state and will obtain location
		information such as enclosure and slot numbers */
	FBE_LOGICAL_DRIVE_LIFECYCLE_COND_GET_LOCATION_INFO,

	/* send down any queued IOs we had because of any reason (e.g. hibernation) */
	FBE_LOGICAL_DRIVE_LIFECYCLE_COND_DEQUEUE_PENDING_IO,

    /* special handling when edge attributes changed and no edges are connected */
    FBE_LOGICAL_DRIVE_LIFECYCLE_COND_NO_CLIENT_EDGES_CONNECTED,

	FBE_LOGICAL_DRIVE_LIFECYCLE_COND_LAST /* must be last */
} 
fbe_logical_drive_lifecycle_cond_id_t;

/* Forward declaration for visibility.
 */
struct fbe_logical_drive_s;
struct fbe_logical_drive_io_cmd_s;

/* These are function definitions for the methods of the
 * logical drive object.
 */
typedef fbe_status_t (*fbe_ldo_send_io_packet_fn)(struct fbe_logical_drive_s * const ldo_p,
                                                  fbe_packet_t *packet_p);
typedef fbe_status_t (*fbe_ldo_allocate_mem_fn)(fbe_packet_t *io_packet_p,
                                                fbe_memory_completion_function_t    completion_function,
                                                fbe_memory_completion_context_t completion_context,
                                                const fbe_u32_t number_to_allocate);

typedef fbe_status_t (*fbe_ldo_allocate_io_cmd_fn)( fbe_packet_t * const packet_p,
                                                   fbe_memory_completion_function_t completion_function,
                                                   fbe_memory_completion_context_t completion_context );
typedef void (*fbe_ldo_free_io_packet_fn)(fbe_packet_t *io_packet_p);
typedef void (*fbe_ldo_free_io_cmd_fn)(struct fbe_logical_drive_io_cmd_s *const io_cmd_p);
typedef fbe_status_t (*fbe_ldo_clear_condition_fn)(struct fbe_base_object_s * object_p);
typedef fbe_status_t (*fbe_ldo_set_condition_fn)(fbe_lifecycle_const_t * class_const_p,
                                                 struct fbe_base_object_s * object_p,
                                                 fbe_lifecycle_cond_id_t cond_id);
typedef fbe_status_t (*fbe_ldo_complete_io_packet_fn)(fbe_packet_t *packet_p);

/*!****************************************************************************
 *    
 * @struct fbe_logical_drive_t
 *  
 * @brief 
 *   This is the definition of the logical drive object.
 *   This object represents a logical drive, which is an object which can
 *   perform block size conversions.
 ******************************************************************************/

typedef struct fbe_logical_drive_s
{    
    /*! This must be first.  This is the base object we inherit from.
     */
    fbe_base_discovered_t base_discovered;

    /*! This is the structure which is used for holding the list of attached 
     *  edges from clients.  
     */
    fbe_block_transport_server_t block_transport_server;

    /*! This is our storage extent protocol edge to the physical drive. 
     */
    fbe_block_edge_t block_edge;

    /*! This is the object id of the physical drive. 
     *  The lifecycle and usurper code needs this in order to
     *  issue usurper commands for things like attaching/detaching the edge.
     */
    fbe_object_id_t object_id;

    /*! Generic flags field.
     */
    fbe_ldo_flags_enum_t flags;

    /*! The identity information is used by our lifecycle conditions.
     */
    fbe_block_transport_identify_t identity;
    fbe_block_transport_identify_t last_identity;

	/* Location information */
	fbe_u32_t port_number;
	fbe_u32_t enclosure_number;
	fbe_u32_t slot_number;


    /* Lifecycle defines.
     */
    FBE_LIFECYCLE_DEF_INST_DATA;
	FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_LOGICAL_DRIVE_LIFECYCLE_COND_LAST));
}
fbe_logical_drive_t;

/*This is the completion context for zeroing specific region on one raw disk*/
typedef struct fbe_ldo_zero_region_context_s
{
    fbe_logical_drive_t*    logical_drive_p; /*the ldo object pointer*/
    fbe_lba_t   start_lba;
    fbe_block_count_t   block_count;
    fbe_lba_t   next_lba;
}fbe_ldo_zero_region_context_t;


/* Accessor functions for the block_size_bytes field.
 */
static __forceinline fbe_block_size_t
fbe_ldo_get_imported_block_size(fbe_logical_drive_t * logical_drive_p)
{
	fbe_block_size_t block_size;
	fbe_block_transport_edge_get_exported_block_size(&logical_drive_p->block_edge, &block_size);
    return block_size;
}

/* Accessor functions for the imported_capacity field.
 */
static __forceinline void
fbe_ldo_set_imported_capacity(fbe_logical_drive_t *const logical_drive_p,
                              const fbe_lba_t capacity)
{
    fbe_block_transport_edge_set_capacity(&logical_drive_p->block_edge, capacity);
    return;
}

/*! @fn fbe_ldo_set_object_id(fbe_logical_drive_t *const logical_drive_p, 
 *                            const fbe_object_id_t object_id)
 * @brief 
 *   Sets the value of the physical drive's object id.
 *  
 * @param logical_drive_p - The logical drive to set the object id for. 
 *                          We make no assumptions about the other
 *                          values inside the logical drive.
 * @param object_id - The object id of the physical drive. 
 *  
 * @return None. 
 */
static __forceinline void
fbe_ldo_set_object_id(fbe_logical_drive_t *const logical_drive_p,
                      const fbe_object_id_t object_id)
{
    logical_drive_p->object_id = object_id;
    return;
}
/* end fbe_ldo_set_object_id() */

/*! @fn fbe_ldo_get_object_id( 
 *                      const fbe_logical_drive_t *const logical_drive_p)
 * @brief 
 *   Returns the value of the physical drive's object id.
 *  
 * @param logical_drive_p - The logical drive to set the object id for. 
 *                          We make no assumptions about the other
 *                          values inside the logical drive.
 * @return fbe_object_id_t - Object id of the physical drive.
 */
static __forceinline fbe_object_id_t
fbe_ldo_get_object_id(const fbe_logical_drive_t *const logical_drive_p)
{
     return(logical_drive_p->object_id);
}
/* end fbe_ldo_get_object_id() */

/*! @fn fbe_logical_drive_lock(fbe_logical_drive_t *const logical_drive_p) 
 *  
 * @brief 
 *   Function which locks the logical drive via the base object lock.
 */
static __forceinline void
fbe_logical_drive_lock(fbe_logical_drive_t *const logical_drive_p)
{
    fbe_base_object_lock((fbe_base_object_t *)logical_drive_p);
    return;
}

/* @fn fbe_logical_drive_unlock(fbe_logical_drive_t *const logical_drive_p) 
 *  
 * @brief 
 *   Function which unlocks the logical drive via the base object lock.
 */
static __forceinline void
fbe_logical_drive_unlock(fbe_logical_drive_t *const logical_drive_p)
{
    fbe_base_object_unlock((fbe_base_object_t *)logical_drive_p);
    return;
}
/* @fn fbe_logical_drive_set_flag(fbe_logical_drive_t *const logical_drive_p,
 *                                const fbe_logical_drive_flags_t flag)
 *  
 * @brief 
 *   Function which sets a logical drive flag.
 */
static __forceinline void
fbe_logical_drive_set_flag(fbe_logical_drive_t *const logical_drive_p,
                           const fbe_logical_drive_flags_t flag)
{
    logical_drive_p->flags |= flag;
    return;
}
/* @fn fbe_logical_drive_clear_flag(fbe_logical_drive_t *const logical_drive_p,
 *                                  const fbe_logical_drive_flags_t flag)
 *  
 * @brief 
 *   Function which clears a logical drive flag.
 */
static __forceinline void
fbe_logical_drive_clear_flag(fbe_logical_drive_t *const logical_drive_p,
                           const fbe_logical_drive_flags_t flag)
{
    logical_drive_p->flags &= ~flag;
    return;
}
/* @fn fbe_logical_drive_is_flag_set(fbe_logical_drive_t *const logical_drive_p,
 *                                  const fbe_logical_drive_flags_t flag)
 *  
 * @brief 
 *   Function which determines if a flag is set.
 */
static __forceinline fbe_bool_t
fbe_logical_drive_is_flag_set(fbe_logical_drive_t *const logical_drive_p,
                              const fbe_logical_drive_flags_t flag)
{
    return((logical_drive_p->flags & flag) != 0);
}

/* Max number of sgs we allocate for each io cmd.
 * We allocate 2049 each since this is what the miniport can accept. 
 */
#define FBE_LDO_SG_POOL_MAX_SG_LENGTH 2049

/* Max number of io cmds in our pool.
 */
#define FBE_LDO_IO_CMD_POOL_MAX 10

/* Imports for visibility.
 */

/* fbe_logical_drive_main.c */

void fbe_ldo_setup_block_sizes(fbe_logical_drive_t * const logical_drive_p,
                               fbe_block_size_t imported_block_size);
fbe_status_t fbe_ldo_monitor_io_packet_queue(fbe_logical_drive_t * logical_drive_p);

fbe_status_t fbe_logical_drive_init(fbe_logical_drive_t * const logical_drive_p);
fbe_status_t fbe_logical_drive_destroy( fbe_object_handle_t object_handle);
fbe_status_t fbe_logical_drive_init_members(fbe_logical_drive_t * const logical_drive_p);

/* fbe_logical_drive_monitor.c */
fbe_status_t fbe_logical_drive_monitor(fbe_logical_drive_t * const logical_drive_p, 
                                       fbe_packet_t * mgmt_packet_p);
fbe_status_t logical_drive_identity_unknown_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
fbe_status_t logical_drive_identity_not_validated_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
fbe_status_t logical_drive_block_edge_detached_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
fbe_status_t logical_drive_block_edge_attached_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
fbe_status_t logical_drive_block_edge_disabled_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
fbe_status_t fbe_ldo_set_condition(fbe_lifecycle_const_t * class_const_p,
                                                        struct fbe_base_object_s * object_p,
                                                        fbe_lifecycle_cond_id_t cond_id);
fbe_status_t fbe_ldo_clear_condition(struct fbe_base_object_s * object_p);

/* fbe_logical_drive_usurper.c */
fbe_status_t fbe_logical_drive_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);

fbe_status_t fbe_ldo_get_identity(fbe_logical_drive_t * const logical_drive_p, 
                                  fbe_packet_t * const packet_p);
fbe_status_t fbe_ldo_send_get_object_id_command(fbe_logical_drive_t * logical_drive, fbe_packet_t * packet);
fbe_status_t fbe_ldo_send_discovery_packet(fbe_logical_drive_t * logical_drive, fbe_packet_t * packet);
fbe_status_t fbe_ldo_send_control_packet(fbe_logical_drive_t * logical_drive, fbe_packet_t * packet);
fbe_status_t fbe_ldo_open_block_edge(fbe_logical_drive_t * logical_drive_p, fbe_packet_t * packet_p);
fbe_status_t fbe_ldo_detach_block_edge(fbe_logical_drive_t * logical_drive_p, fbe_packet_t * packet_p);
fbe_status_t fbe_ldo_attach_block_edge(fbe_logical_drive_t *logical_drive_p, fbe_packet_t * packet_p); 

/* fbe_logical_drive_bitbucket.c */
fbe_u32_t fbe_ldo_get_bitbucket_bytes(void);
fbe_u32_t fbe_ldo_bitbucket_get_sg_entries(const fbe_u32_t bytes);
fbe_ldo_bitbucket_t * fbe_ldo_get_read_bitbucket_ptr(void);
fbe_ldo_bitbucket_t * fbe_ldo_get_write_bitbucket_ptr(void);
void fbe_ldo_bitbucket_init(void);
fbe_sg_element_t * fbe_ldo_fill_sg_with_bitbucket(fbe_sg_element_t *sg_p,
                                                  fbe_u32_t bytes,
                                                  fbe_ldo_bitbucket_t *const bitbucket_p);

/* fbe_logical_drive_map.c */
void fbe_ldo_get_edge_sizes(fbe_lba_t lba,
                            fbe_block_count_t blocks,
                            fbe_block_size_t exported_block_size,
                            fbe_block_size_t exported_opt_block_size,
                            fbe_block_size_t imported_block_size,
                            fbe_block_size_t imported_opt_block_size,
                            fbe_u32_t *const edge0_size_p,
                            fbe_u32_t *const edge1_size_p);
fbe_lba_t
fbe_ldo_map_imported_lba_to_exported_lba(fbe_block_size_t exported_block_size,
                                         fbe_lba_t exported_opt_block_size,
                                         fbe_lba_t imported_opt_block_size,
                                         fbe_lba_t lba);
fbe_status_t
ldo_get_optimum_block_sizes(fbe_block_size_t imported_block_size,
                            fbe_block_size_t exported_block_size,
                            fbe_block_size_t *imported_optimum_block_size,
                            fbe_block_size_t *exported_optimum_block_size);

fbe_status_t 
fbe_ldo_map_get_pre_read_desc_offsets(fbe_lba_t lba,
                                      fbe_block_count_t blocks,
                                      fbe_lba_t pre_read_desc_lba,
                                      fbe_block_size_t exported_block_size,
                                      fbe_block_size_t exported_opt_block_size,
                                      fbe_block_size_t imported_block_size,
                                      fbe_u32_t first_edge_size,
                                      fbe_u32_t *const first_edge_offset_p,
                                      fbe_u32_t *const last_edge_offset_p );

fbe_bool_t fbe_ldo_io_is_aligned(fbe_lba_t lba,
                                 fbe_block_count_t blocks,
                                 fbe_block_size_t opt_blk_size);

fbe_status_t 
fbe_ldo_validate_pre_read_desc(fbe_payload_pre_read_descriptor_t * const pre_read_desc_p,
                               fbe_block_size_t exported_block_size,
                               fbe_block_size_t exported_opt_block_size,
                               fbe_block_size_t imported_block_size,
                               fbe_block_size_t imported_opt_block_size,
                               fbe_lba_t exported_lba,
                               fbe_block_count_t exported_blocks,
                               fbe_lba_t imported_lba,
                               fbe_block_count_t imported_blocks);
fbe_bool_t
fbe_ldo_is_padding_required(fbe_block_size_t exported_block_size,
                            fbe_block_size_t exported_opt_block_size,
                            fbe_block_size_t imported_block_size,
                            fbe_block_size_t imported_opt_block_size);


/* fbe_logical_drive_io.c   */

fbe_status_t ldo_forward_packet( fbe_packet_t * mgmt_packet_p,
                                 fbe_logical_drive_t * logical_drive_p, 
                                 fbe_packet_completion_function_t completion_function,
                                 fbe_packet_completion_context_t  completion_context );
void fbe_ldo_build_sub_packet(fbe_packet_t * const packet_p,
                              fbe_payload_block_operation_opcode_t opcode,
                              fbe_lba_t lba,
                              fbe_block_count_t blocks,
                              fbe_block_size_t block_size,
                              fbe_block_size_t optimal_block_size,
                              fbe_sg_element_t * const sg_p,
                              fbe_packet_completion_context_t context);
void fbe_ldo_build_packet(fbe_packet_t * const packet_p,
                          fbe_payload_block_operation_opcode_t opcode,
                          fbe_lba_t lba,
                          fbe_block_count_t blocks,
                          fbe_block_size_t block_size,
                          fbe_block_size_t optimal_block_size,
                          fbe_sg_element_t * const sg_p,
                          fbe_packet_completion_function_t completion_function,
                          fbe_packet_completion_context_t context,
                          fbe_u32_t repeat_count);
fbe_status_t fbe_ldo_packet_completion(fbe_packet_t * packet_p, 
                                       fbe_packet_completion_context_t context);
fbe_status_t fbe_ldo_io_cmd_packet_completion(fbe_packet_t * packet,
                                              fbe_packet_completion_context_t context);

fbe_status_t fbe_ldo_resources_required(fbe_logical_drive_t *logical_drive_p, 
                                        fbe_packet_t * packet_p);
/* fbe_logical_drive_executor.
 */
fbe_status_t fbe_ldo_calculate_negotiate_info(fbe_logical_drive_t * const logical_drive_p,
                                              fbe_block_transport_negotiate_t * const negotiate_info_p);
fbe_status_t fbe_logical_drive_block_transport_entry(fbe_transport_entry_context_t context, fbe_packet_t * packet);

fbe_status_t fbe_ldo_process_attributes(fbe_logical_drive_t * const logical_drive_p, fbe_path_attr_t attribute);
fbe_status_t fbe_logical_drive_get_location_info(fbe_logical_drive_t * const logical_drive_p, fbe_packet_t * packet_p);
fbe_status_t fbe_logical_drive_block_transport_handle_io(fbe_logical_drive_t *logical_drive_p, 
                                                         fbe_packet_t * packet_p);
#endif /* LOGICAL_DRIVE_PRIVATE_H */

/*******************************
 * end fbe_logical_drive_private.h
 *******************************/
