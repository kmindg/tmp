#ifndef LOGICAL_DRIVE_IO_CMD_H
#define LOGICAL_DRIVE_IO_CMD_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_io_command.h
 ***************************************************************************
 *
 * @brief
 *  This file contains private defines for the logical drive command object.
 * 
 * @ingroup logical_drive_class_files
 *
 * @date 10/29/2007
 * @author RPF
 *
 ***************************************************************************/

#include "fbe_base_object.h"
#include "base_object_private.h"
#include "fbe/fbe_transport.h"
#include "fbe_logical_drive_environment_interface.h"

/*! @enum fbe_ldo_state_status_enum_t
 *  
 * @brief This is the set of states returned by functions of the logical drive 
 *        state machines. 
 */
typedef enum fbe_ldo_state_status_enum_e
{
    FBE_LDO_STATE_STATUS_INVALID = 0, /*!< This should always be first. */

    /*! This means we will go execute the next state now. 
     */
    FBE_LDO_STATE_STATUS_EXECUTING,

    /*! Waiting for a response of some kind, not executing the next state now. 
     */
    FBE_LDO_STATE_STATUS_WAITING,

    /*! Done, but not waiting for a response. 
     */
    FBE_LDO_STATE_STATUS_DONE,

    /*! This should always be last. 
     */
	FBE_LDO_STATE_STATUS_LAST
}
fbe_ldo_state_status_enum_t;

/* Define the state for the io_cmd.  The forward declaration of the struct is necessary
 * in order to define the state.
 */
struct fbe_logical_drive_io_cmd_t;

/*! @typedef fbe_ldo_io_cmd_state_t
 *  
 * @brief 
 *   This defines the state field of the io cmd.
 *   The state field is a pointer to the next state to execute.
 */
typedef fbe_ldo_state_status_enum_t(*fbe_ldo_io_cmd_state_t) (struct fbe_logical_drive_io_cmd_s *const);

/*! @struct fbe_logical_drive_io_cmd_t
 *  
 * @brief Logical drive I/O Command object. This command holds information on a 
 *        given I/O packet including the state of the object, and the ptr to the
 *        object's resources.
 */
typedef struct fbe_logical_drive_io_cmd_s
{
    /*! Reference to the object this cmd operates on.
     */
    fbe_object_handle_t object_handle;

    /*! Imported optimum block size, calculated when command arrives. 
     */
    fbe_block_size_t imported_optimum_block_size;

    /*! Ptr to the packet for this cmd.
     */
    fbe_packet_t *packet_p;

    /*! Current state of this io_cmd.
     */
    fbe_ldo_io_cmd_state_t state;

    /*! Number of sg entries needed.
     */
    fbe_u32_t sg_entries;

    /*! The actual memory we use for sgs. 
     *  We embed this here since along with the
     *  io command we will allocate a max sized sg list.
     */
    fbe_sg_element_t sg_list[FBE_LDO_SG_POOL_MAX_SG_LENGTH];

    /*! Used to enqueue, dequeue from our io cmds pool's list.
     */
    fbe_queue_element_t queue_element;

    /*! Ptr to the scatter gather list we allocated for this command.
     */
    fbe_sg_element_t *sg_p;

} fbe_logical_drive_io_cmd_t;


/* Accessor functions for the state field.
 */
static __forceinline void
fbe_ldo_io_cmd_set_state(fbe_logical_drive_io_cmd_t *const io_cmd_p,
                         const fbe_ldo_io_cmd_state_t state)
{
    io_cmd_p->state = state;
    return;
}
static __forceinline const fbe_ldo_io_cmd_state_t
fbe_ldo_io_cmd_get_state(const fbe_logical_drive_io_cmd_t *const io_cmd_p)
{
     return io_cmd_p->state;
}

/* Accessor functions for the sg field.
 */
static __forceinline fbe_sg_element_t *
fbe_ldo_io_cmd_get_sg_ptr(fbe_logical_drive_io_cmd_t *const io_cmd_p)
{
     return io_cmd_p->sg_list;
}

/* Accessor functions for the sg_entries field.
 */
static __forceinline void
fbe_ldo_io_cmd_set_sg_entries(fbe_logical_drive_io_cmd_t *const io_cmd_p,
                              const fbe_u32_t sg_entries)
{
    io_cmd_p->sg_entries = sg_entries;
    return;
}
static __forceinline fbe_u32_t
fbe_ldo_io_cmd_get_sg_entries(const fbe_logical_drive_io_cmd_t *const io_cmd_p)
{
     return io_cmd_p->sg_entries;
}


/* Accessor functions for the object_handle field.
 */
static __forceinline void
fbe_ldo_io_cmd_set_object_handle(fbe_logical_drive_io_cmd_t *const io_cmd_p,
                                 const fbe_object_handle_t object_handle)
{
    io_cmd_p->object_handle = object_handle;
    return;
}
static __forceinline fbe_object_handle_t
fbe_ldo_io_cmd_get_object_handle(fbe_logical_drive_io_cmd_t *const io_cmd_p)
{
     return io_cmd_p->object_handle;
}
static __forceinline fbe_logical_drive_t *
fbe_ldo_io_cmd_get_logical_drive(fbe_logical_drive_io_cmd_t *const io_cmd_p)
{
     return (fbe_logical_drive_t *)fbe_base_handle_to_pointer(io_cmd_p->object_handle);
}

/* Accessor functions for the packet field.
 */
static __forceinline void
fbe_ldo_io_cmd_set_packet(fbe_logical_drive_io_cmd_t *const io_cmd_p,
                             fbe_packet_t * const packet_p)
{
    io_cmd_p->packet_p = packet_p;
    return;
}
static __forceinline fbe_packet_t *
fbe_ldo_io_cmd_get_packet(fbe_logical_drive_io_cmd_t *const io_cmd_p)
{
     return io_cmd_p->packet_p;
}

/*! @fn __forceinline void fbe_ldo_io_cmd_set_imported_optimum_block_size( 
 *       fbe_logical_drive_io_cmd_t *const io_cmd_p,
 *       fbe_block_size_t block_size_bytes)
 *  
 *  @brief Accessor function for the imported_optimum_block_size field.
 *  @param io_cmd_p - The io cmd ptr.
 *  @param block_size_bytes  - The imported opt block size to set.
 *  @return the imported optimal block size in bytes.
 */
static __forceinline void
fbe_ldo_io_cmd_set_imported_optimum_block_size(fbe_logical_drive_io_cmd_t *const io_cmd_p,
                                        const fbe_block_size_t block_size_bytes)
{
    io_cmd_p->imported_optimum_block_size = block_size_bytes;
    return;
}
/*! @fn fbe_ldo_io_cmd_get_imported_optimum_block_size( 
 *                        fbe_logical_drive_io_cmd_t *const io_cmd_p)
 *  
 *  @brief Accessor function for the imported_optimum_block_size field.  
 *  @param io_cmd_p - The io cmd ptr.
 *  @return the imported optimal block size in bytes.
 */
static __forceinline fbe_block_size_t
fbe_ldo_io_cmd_get_imported_optimum_block_size(fbe_logical_drive_io_cmd_t *const io_cmd_p)
{
     return io_cmd_p->imported_optimum_block_size;
}

/********************************************************************** 
 * Accessor methods for ldo io cmd's lba. 
 **********************************************************************/
#define fbe_ldo_io_cmd_get_lba(m_io_cmd_p)\
( ldo_packet_get_block_cmd_lba( fbe_ldo_io_cmd_get_packet(io_cmd_p) ) )

/********************************************************************** 
 * Accessor methods for ldo io cmd's blocks. 
 **********************************************************************/
#define fbe_ldo_io_cmd_get_blocks(m_io_cmd_p)\
( ldo_packet_get_block_cmd_blocks( fbe_ldo_io_cmd_get_packet(io_cmd_p) ) )

/* Accessor methods for ldo io cmd's block size field. 
 */
#define fbe_ldo_io_cmd_get_block_size(m_io_cmd_p)\
( ldo_packet_get_block_cmd_block_size( fbe_ldo_io_cmd_get_packet(io_cmd_p) ) )

/* Accessor methods for ldo io cmd's optimum block size field. 
 */
#define fbe_ldo_io_cmd_get_optimum_block_size(m_io_cmd_p)\
( ldo_packet_get_block_cmd_optimum_block_size( fbe_ldo_io_cmd_get_packet(io_cmd_p) ) )

/*! @fn fbe_ldo_io_cmd_get_client_sg_ptr( 
 *                        fbe_logical_drive_io_cmd_t *const io_cmd_p)
 *  
 *  @brief Accessor method for ldo io cmd's client sg ptr's field.
 *         This simply returns the sg ptr from the payload.
 */
static __inline fbe_sg_element_t * fbe_ldo_io_cmd_get_client_sg_ptr(fbe_logical_drive_io_cmd_t *const io_cmd_p)
{
    fbe_sg_element_t *sg_p;
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(fbe_ldo_io_cmd_get_packet(io_cmd_p));
    fbe_payload_ex_get_sg_list(payload_p, &sg_p, NULL);
    return sg_p;
}

/* Accessor methods for ldo io cmd's client edge descriptor ptr field. 
 */
#define fbe_ldo_io_cmd_get_pre_read_desc_ptr(m_io_cmd_p)\
( ldo_packet_get_block_cmd_pre_read_desc_ptr( fbe_ldo_io_cmd_get_packet(io_cmd_p) ) )

/* Accessor methods for ldo io cmd's client edge descriptor lba field. 
 */
#define fbe_ldo_io_cmd_get_pre_read_desc_lba(m_io_cmd_p)\
( ldo_packet_get_block_cmd_pre_read_desc_lba( fbe_ldo_io_cmd_get_packet(io_cmd_p) ) )

/* Accessor methods for ldo io cmd's client edge descriptor sg_ptr field. 
 */
#define fbe_ldo_io_cmd_get_pre_read_desc_sg_ptr(m_io_cmd_p)\
( ldo_packet_get_block_cmd_pre_read_desc_sg_ptr( fbe_ldo_io_cmd_get_packet(io_cmd_p) ) )

/* Read state machine.
 */
fbe_status_t fbe_ldo_handle_read(fbe_logical_drive_t *logical_drive_p, fbe_packet_t * packet_p);
fbe_ldo_state_status_enum_t fbe_ldo_io_cmd_read_state0(fbe_logical_drive_io_cmd_t * const io_cmd_p);
fbe_ldo_state_status_enum_t fbe_ldo_io_cmd_read_state10(fbe_logical_drive_io_cmd_t * const io_cmd_p);
fbe_ldo_state_status_enum_t fbe_ldo_io_cmd_read_state20(fbe_logical_drive_io_cmd_t * const io_cmd_p);
fbe_ldo_state_status_enum_t fbe_ldo_io_cmd_read_state30(fbe_logical_drive_io_cmd_t * const io_cmd_p);
fbe_u32_t fbe_ldo_io_cmd_rd_count_sg_entries(fbe_logical_drive_io_cmd_t * const io_cmd_p);
fbe_status_t fbe_ldo_io_cmd_read_setup_sg(fbe_logical_drive_io_cmd_t * const io_cmd_p);
fbe_sg_element_t * fbe_ldo_io_cmd_setup_sgs_for_client_data(fbe_logical_drive_io_cmd_t * const io_cmd_p,
                                         fbe_sg_element_t * const dest_sg_p,
                                         fbe_ldo_bitbucket_t *const bitbucket_p);
fbe_u32_t fbe_ldo_io_cmd_count_sgs_for_client_data(fbe_logical_drive_io_cmd_t * const io_cmd_p);
fbe_status_t fbe_ldo_read_setup_sgs(fbe_logical_drive_t * const logical_drive_p,
                                    fbe_payload_ex_t * const payload_p,
                                    fbe_u32_t edge0_bytes,
                                    fbe_u32_t edge1_bytes);

/* Write state machine.
 */
fbe_status_t fbe_ldo_handle_write(fbe_logical_drive_t *logical_drive_p, 
                                  fbe_packet_t * packet_p);
fbe_ldo_state_status_enum_t fbe_ldo_io_cmd_write_state0(fbe_logical_drive_io_cmd_t * const io_cmd_p);
fbe_ldo_state_status_enum_t fbe_ldo_io_cmd_write_state10(fbe_logical_drive_io_cmd_t * const io_cmd_p);
fbe_ldo_state_status_enum_t fbe_ldo_io_cmd_write_state11(fbe_logical_drive_io_cmd_t * const io_cmd_p);
fbe_ldo_state_status_enum_t fbe_ldo_io_cmd_write_state20(fbe_logical_drive_io_cmd_t * const io_cmd_p);
fbe_ldo_state_status_enum_t fbe_ldo_io_cmd_write_state30(fbe_logical_drive_io_cmd_t * const io_cmd_p);
fbe_u32_t fbe_ldo_io_cmd_wr_count_sg_entries(fbe_logical_drive_io_cmd_t * const io_cmd_p);
fbe_status_t fbe_ldo_io_cmd_wr_setup_sg(fbe_logical_drive_io_cmd_t * const io_cmd_p);
fbe_status_t fbe_ldo_write_setup_sgs(fbe_logical_drive_t * logical_drive_p, fbe_packet_t *  packet_p);

/* Verify state machine.
 */
fbe_status_t fbe_ldo_handle_verify(fbe_logical_drive_t *logical_drive_p, 
                                   fbe_packet_t * packet_p);

void fbe_ldo_io_state_machine(fbe_logical_drive_io_cmd_t *const io_cmd_p);

fbe_ldo_state_status_enum_t fbe_ldo_io_cmd_finished(fbe_logical_drive_io_cmd_t * const io_cmd_p);
fbe_ldo_state_status_enum_t fbe_ldo_io_cmd_generate(fbe_logical_drive_io_cmd_t *const io_cmd_p);
fbe_ldo_state_status_enum_t fbe_ldo_io_cmd_invalid_opcode(fbe_logical_drive_io_cmd_t *const io_cmd_p);
fbe_ldo_io_cmd_state_t fbe_ldo_io_cmd_get_first_state(fbe_payload_block_operation_t *block_command_p);

/* Write Same state machine.
 */
fbe_status_t fbe_ldo_handle_write_same(fbe_logical_drive_t *const logical_drive_p, 
                                       fbe_packet_t *const packet_p);
fbe_ldo_state_status_enum_t fbe_ldo_io_cmd_write_same_state0(fbe_logical_drive_io_cmd_t * const io_cmd_p);
fbe_ldo_state_status_enum_t fbe_ldo_io_cmd_write_same_state10(fbe_logical_drive_io_cmd_t * const io_cmd_p);
fbe_ldo_state_status_enum_t fbe_ldo_io_cmd_write_same_state20(fbe_logical_drive_io_cmd_t * const io_cmd_p);
fbe_ldo_state_status_enum_t fbe_ldo_io_cmd_write_same_state30(fbe_logical_drive_io_cmd_t * const io_cmd_p);
fbe_status_t fbe_ldo_io_cmd_validate_write_same(fbe_logical_drive_io_cmd_t * const io_cmd_p);
fbe_bool_t fbe_ldo_io_cmd_use_write_same_operation(fbe_logical_drive_io_cmd_t * const io_cmd_p);
void fbe_ldo_io_cmd_write_same_setup_sg(fbe_logical_drive_io_cmd_t * const io_cmd_p);

/* fbe_logical_drive_map.c */

fbe_u32_t fbe_ldo_calculate_pad_bytes(fbe_logical_drive_io_cmd_t *const io_cmd_p);
fbe_block_size_t fbe_ldo_calculate_imported_optimum_block_size(fbe_block_size_t exported_block_size,
                                                               fbe_block_size_t exported_optimum_block_size,
                                                               fbe_block_size_t imported_block_size);
fbe_block_size_t fbe_ldo_calculate_exported_optimum_block_size(fbe_block_size_t requested_block_size,
                                                               fbe_block_size_t requested_optimum_block_size,
                                                               fbe_block_size_t imported_block_size);
fbe_bool_t fbe_ldo_io_cmd_is_padding_required(fbe_logical_drive_io_cmd_t *const io_cmd_p);

/* fbe_logical_drive_io_cmd.c */
void fbe_ldo_io_cmd_map_lba_blocks(fbe_logical_drive_io_cmd_t * const io_cmd_p,
                                   fbe_lba_t *const output_lba_p,
                                   fbe_block_count_t *const output_blocks_p);
void fbe_ldo_io_cmd_free_resources(fbe_logical_drive_io_cmd_t * const io_cmd_p);
fbe_bool_t fbe_ldo_io_cmd_is_aligned(fbe_logical_drive_io_cmd_t * const io_cmd_p);
fbe_bool_t fbe_ldo_io_cmd_is_sg_required(fbe_logical_drive_io_cmd_t * const io_cmd_p);
fbe_bool_t fbe_ldo_io_cmd_is_aligned_for_ws(fbe_logical_drive_io_cmd_t * const io_cmd_p);
void fbe_ldo_io_cmd_build_sub_packet(fbe_logical_drive_io_cmd_t * const io_cmd_p,
                                     fbe_payload_block_operation_opcode_t opcode,
                                     fbe_lba_t imported_lba,
                                     fbe_block_count_t imported_blocks,
                                     fbe_block_size_t imported_optimal_block_size);
fbe_status_t fbe_ldo_io_cmd_send_io(fbe_logical_drive_io_cmd_t * const io_cmd_p);
fbe_status_t fbe_ldo_complete_packet(fbe_packet_t * packet);
void fbe_ldo_io_cmd_init( fbe_logical_drive_io_cmd_t *const io_cmd_p,
                                 fbe_object_handle_t logical_drive_p, 
                                 fbe_packet_t * io_packet_p );
void fbe_ldo_io_cmd_destroy( fbe_logical_drive_io_cmd_t *const io_cmd_p );
fbe_status_t fbe_ldo_io_cmd_get_pre_read_desc_offsets( fbe_logical_drive_io_cmd_t * const io_cmd_p,
                                                   fbe_u32_t *first_edge_offset_p,
                                                   fbe_u32_t *last_edge_offset_p,
                                                   fbe_u32_t first_edge_size );
fbe_status_t fbe_ldo_io_cmd_validate_pre_read_desc( fbe_logical_drive_io_cmd_t * const io_cmd_p );
void fbe_ldo_io_cmd_set_status(fbe_logical_drive_io_cmd_t * const io_cmd_p,
                               fbe_status_t status,
                               fbe_u32_t qualifier);
void fbe_ldo_io_cmd_set_payload_status(fbe_logical_drive_io_cmd_t * const io_cmd_p,
                                       fbe_status_t status,
                                       fbe_u32_t qualifier);
void fbe_ldo_io_cmd_set_status_from_sub_io_packet(fbe_logical_drive_io_cmd_t * const io_cmd_p);

/* fbe_logical_drive_io_cmd_memory.c   */
fbe_status_t fbe_ldo_allocate_io_cmd( fbe_packet_t * const packet_p,
                                      fbe_memory_completion_function_t completion_function,
                                      fbe_memory_completion_context_t completion_context);
fbe_status_t fbe_ldo_io_cmd_allocate_memory_callback(fbe_memory_request_t * memory_request_p, 
                                                     fbe_memory_completion_context_t context);
fbe_status_t fbe_ldo_io_cmd_pool_init(void);
void fbe_ldo_io_cmd_pool_destroy(void);
fbe_u32_t fbe_ldo_io_cmd_pool_get_free_count(void);
void fbe_ldo_free_io_cmd(fbe_logical_drive_io_cmd_t *const io_cmd_p);
fbe_logical_drive_io_cmd_t * 
fbe_ldo_io_cmd_queue_element_to_pool_element(fbe_queue_element_t * queue_element_p);

/* fbe_logical_drive_kernel and user
 * The below are defined for kernel and user.  
 */
void fbe_ldo_setup_defaults(fbe_logical_drive_t * logical_drive_p);
fbe_status_t fbe_ldo_send_io_packet(fbe_logical_drive_t * const logical_drive_p,
                                    fbe_packet_t *packet_p);
void fbe_ldo_io_cmd_free(fbe_logical_drive_io_cmd_t *const io_cmd_p);
fbe_status_t fbe_ldo_io_cmd_allocate( fbe_packet_t * const packet_p,
                                      fbe_memory_completion_function_t completion_function,
                                      fbe_memory_completion_context_t completion_context);

/* fbe_logical_drive_user.c */

fbe_status_t fbe_logical_drive_initialize_for_test(fbe_logical_drive_t * const logical_drive_p);
void fbe_logical_drive_reset_methods(void);
void fbe_ldo_set_send_io_packet_fn(const fbe_ldo_send_io_packet_fn send_io_packet_fn);
void fbe_ldo_set_complete_io_packet_fn(const fbe_ldo_complete_io_packet_fn complete_io_packet_fn);
void fbe_ldo_set_free_io_cmd_fn(const fbe_ldo_free_io_cmd_fn free_io_cmd_fn);
void fbe_ldo_set_allocate_io_cmd_fn(const fbe_ldo_allocate_io_cmd_fn allocate_io_cmd_fn);
void fbe_ldo_set_clear_condition_fn(const fbe_ldo_clear_condition_fn clear_condition_fn);
void fbe_ldo_set_condition_set_fn(const fbe_ldo_set_condition_fn set_condition_fn);

/* fbe_logical_drive_kernel.c */

#endif /* LOGICAL_DRIVE_IO_CMD_H */

/*******************************
 * end fbe_logical_drive_io_command.h
 *******************************/
