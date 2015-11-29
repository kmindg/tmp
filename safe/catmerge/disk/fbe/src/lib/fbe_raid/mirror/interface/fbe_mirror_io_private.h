#ifndef FBE_MIRROR_COMMON_H
#define FBE_MIRROR_COMMON_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file    fbe_mirror_common.h
 ***************************************************************************
 *
 * @brief   The files contains the definitions for the `common' raid mirror
 *          algorithms.  This means the values that are common for sparing,
 *          RAID-1 and metadata mirror type raid groups.
 * 
 * @ingroup mirror_class_files
 * 
 * @version
 *   05/20/2009:  Created. RPF
 *
 ***************************************************************************/

/******************************
 * INCLUDES
 ******************************/
#include "fbe_raid_library_private.h"
#include "fbe_raid_geometry.h"
#include "fbe/fbe_mirror.h"
#include "fbe/fbe_transport.h"
#include "fbe_block_transport.h"
#include "fbe_raid_fruts.h"

/******************************
 * LITERAL DEFINITIONS.
 ******************************/

/*!*******************************************************************
 * @def FBE_MIRROR_MIN_REQUEST_SIZE_TO_CHECK
 *********************************************************************
 * @brief This is an arbitrary size, beyond which we want to make sure
 *        that we do not exceed the sg limits.
 *        This currently assumes a single block per sg element.
 *
 *********************************************************************/
#define FBE_MIRROR_MIN_REQUEST_SIZE_TO_CHECK FBE_RAID_MAX_SG_DATA_ELEMENTS
 

/*! @enum fbe_mirror_default_position_t
 *  
 *  @brief This is the enumeration of all posible `default'
 *         mirror positions.
 *
 *  @note The definition of which position is the primary, secondary, etc
 *        belongs here.  Don't move them.
 */
typedef enum fbe_mirror_default_position_e
{
    FBE_MIRROR_DEFAULT_PRIMARY_POSITION     = 0,
    FBE_MIRROR_DEFAULT_SECONDARY_POSITION   = 1,
    FBE_MIRROR_DEFAULT_TERTIARY_POSITION    = 2,
} fbe_mirror_default_position_t; 

/*! @enum fbe_mirror_index_t
 *  
 *  @brief This is the enumeration of all posible indexes
 *         into the mirror position array.
 *
 * @note The following are indexes not the actual positions.
 *        The positions are not exposed globally since the macros:
 *        o fbe_mirror_get_primary_position()
 *        o fbe_mirror_get_secondary_position()
 *        o fbe_mirror_get_tertiary_position()
 *        should always be used.
 */
typedef enum fbe_mirror_index_e
{
    FBE_MIRROR_PRIMARY_INDEX    = 0,
    FBE_MIRROR_SECONDARY_INDEX  = 1,
    FBE_MIRROR_TERTIARY_INDEX   = 2,

    FBE_MIRROR_INVALID_INDEX    = -1
} fbe_mirror_index_t;

/*! @enum   fbe_mirror_position_select_qos_t
 *  
 *  @brief  This is the enumeration of the selection criteria for determining
 *          the original or replacement read position.
 *
 *  @note Is this neccessary?
 */
typedef enum fbe_mirror_position_select_qos_e
{
    FBE_MIRROR_QOS_RELIABILITY  = 0,
    FBE_MIRROR_QOS_PERFORMANCE  = 1,
} fbe_mirror_position_select_qos_t;


/******************************
 * MACROS
 ******************************/
/*! fbe_mirror_get_position_from_index()
 *
 * @brief   Uses the siots geometry to return the position
 *          for the index specified.
 *
 * @param   siots_p - Pointer to the siots to get position for
 *          mirror_index - The index to retrieve position for 
 *  
 * @return  postion at the index specified
 */
static __forceinline fbe_u32_t
fbe_mirror_get_position_from_index(fbe_raid_siots_t *siots_p,
                                   fbe_mirror_index_t  mirror_index)

{
    /* We have already set the geometry in the siots so simply return
     * the position.
     */
    return(siots_p->geo.position[mirror_index]);
}
/*! fbe_mirror_set_position_for_index()
 *
 * @brief   Uses the siots geometry to return the position
 *          for the index specified.
 *
 * @param   siots_p - Pointer to the siots to get position for
 *          mirror_index - The index to retrieve position for
 *          position - The position value to set at the index specified 
 *  
 * @return  None
 */
static __forceinline void
fbe_mirror_set_position_for_index(fbe_raid_siots_t *siots_p,
                                  fbe_mirror_index_t mirror_index,
                                  fbe_u32_t         position)

{
    /* Set the mirror position at the specified index
     */
    siots_p->geo.position[mirror_index] = position;
    return;
}
/*! fbe_mirror_get_primary_position()
 *
 * @brief   Uses the geometry to return the primary mirror position.
 *
 * @param   siots_p - Pointer to the siots to get position for
 *  
 * @return  primary position
 */
static __forceinline fbe_u32_t
fbe_mirror_get_primary_position(fbe_raid_siots_t *siots_p)
{
    /* We have already set the geometry in the siots so simply return
     * the primary position.
     */
    return(siots_p->geo.position[FBE_MIRROR_PRIMARY_INDEX]);
}
/*! fbe_mirror_get_secondary_position()
 *
 * @brief   Uses the geometry to return the secondary mirror position.
 *
 * @param   siots_p - Pointer to the siots to get position for
 *  
 * @return  secondary position
 */
static __forceinline fbe_u32_t
fbe_mirror_get_secondary_position(fbe_raid_siots_t *siots_p)
{
    /* We have already set the geometry in the siots so simply return
     * the secondary position.
     */
    return(siots_p->geo.position[FBE_MIRROR_SECONDARY_INDEX]);
}
/*! fbe_mirror_get_tertiary_position()
 *
 * @brief   Uses the geometry to return the teriary mirror position.
 *
 * @param   siots_p - Pointer to the siots to get position for
 *  
 * @return  teriary position
 */
static __forceinline fbe_u32_t
fbe_mirror_get_tertiary_position(fbe_raid_siots_t *siots_p)
{
    /* We have already set the geometry in the siots so simply return
     * the tertiary position.
     */
    return(siots_p->geo.position[FBE_MIRROR_TERTIARY_INDEX]);
}
static __forceinline fbe_u32_t
fbe_mirror_get_index_from_position(fbe_raid_siots_t *siots_p, fbe_u32_t position)
{
    /* We have already set the geometry in the siots so simply return
     * the tertiary position.
     */
    fbe_u32_t index;

    if(position == siots_p->geo.position[FBE_MIRROR_PRIMARY_INDEX])
    {
        index = FBE_MIRROR_PRIMARY_INDEX;
    }
    else if(position == siots_p->geo.position[FBE_MIRROR_SECONDARY_INDEX])
    {
        index = FBE_MIRROR_SECONDARY_INDEX;    
    }
    else if(position == siots_p->geo.position[FBE_MIRROR_TERTIARY_INDEX])
    {
        index = FBE_MIRROR_TERTIARY_INDEX;    
    }
    else
    {
        index = FBE_MIRROR_INVALID_INDEX;    
    }
    return index;
}

/*! fbe_mirror_get_primary_bitmask()
 *
 * @brief   Uses the geometry to return the primary mirror bitmask.
 *
 * @param   siots_p - Pointer to the siots to get bitmask for
 *  
 * @return  primary bitmask
 */
static __forceinline fbe_u16_t
fbe_mirror_get_primary_bitmask(fbe_raid_siots_t *siots_p)
{
    /* We have already set the geometry in the siots so simply return
     * the primary bitmask.
     */
    return( (1 << siots_p->geo.position[FBE_MIRROR_PRIMARY_INDEX]) );
}
/*! fbe_mirror_get_secondary_bitmask()
 *
 * @brief   Uses the geometry to return the secondary mirror bitmask.
 *
 * @param   siots_p - Pointer to the siots to get bitmask for
 *  
 * @return  secondary bitmask
 */
static __forceinline fbe_u16_t
fbe_mirror_get_secondary_bitmask(fbe_raid_siots_t *siots_p)
{
    /* We have already set the geometry in the siots so simply return
     * the secondary bitmask.
     */
    return( (1 << siots_p->geo.position[FBE_MIRROR_SECONDARY_INDEX]) );
}
/*! fbe_mirror_get_tertiary_bitmask()
 *
 * @brief   Uses the geometry to return the teriary mirror bitmask.
 *
 * @param   siots_p - Pointer to the siots to get bitmask for
 *  
 * @return  teriary bitmask
 */
static __forceinline fbe_u16_t
fbe_mirror_get_tertiary_bitmask(fbe_raid_siots_t *siots_p)
{
    /* We have already set the geometry in the siots so simply return
     * the tertiary bitmask.
     */
    return( (1 << siots_p->geo.position[FBE_MIRROR_TERTIARY_INDEX]) );
}

/******************************
 * MIRROR LOCAL IMPORTS
 ******************************/

/**************************************** 
    fbe_mirror_generate.c
 ****************************************/
fbe_status_t fbe_mirror_generate_recovery_verify(fbe_raid_siots_t *siots_p,
                                                 fbe_bool_t *b_is_recovery_verify_p);
fbe_status_t fbe_mirror_generate_validate_siots(fbe_raid_siots_t *siots_p);

/**************************************** 
    fbe_mirror_geometry.c
 ****************************************/
fbe_status_t fbe_mirror_update_position_map(fbe_raid_siots_t *siots_p,
                                            fbe_u32_t new_primary_position);
fbe_status_t fbe_mirror_optimization_update_position_map(fbe_raid_siots_t *siots_p,
                                            fbe_u32_t new_primary_position);

/**************************************** 
 *  fbe_mirror_util.c 
 ****************************************/
fbe_payload_block_operation_opcode_t fbe_mirror_get_write_opcode(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_mirror_set_degraded_positions(fbe_raid_siots_t *siots_p,
                                               fbe_bool_t b_ok_to_split);
fbe_status_t fbe_mirror_handle_degraded_positions(fbe_raid_siots_t *siots_p,
                                                  fbe_bool_t b_write_request,
                                                  fbe_bool_t b_write_degraded);
fbe_status_t fbe_mirror_refresh_degraded_positions(fbe_raid_siots_t *siots_p,
                                                   fbe_raid_fruts_t *fruts_p);
fbe_raid_fru_error_status_t fbe_mirror_get_fruts_error(fbe_raid_fruts_t *fruts_p,
                                                       fbe_raid_fru_eboard_t * eboard_p);
fbe_raid_state_status_t fbe_mirror_handle_fruts_error(fbe_raid_siots_t *siots_p,
                                                      fbe_raid_fruts_t *fruts_p,
                                                      fbe_raid_fru_eboard_t * eboard_p,
                                                      fbe_raid_fru_error_status_t fruts_error_status);
fbe_status_t fbe_mirror_process_dead_fruts_error(fbe_raid_siots_t *siots_p, 
                                                 fbe_raid_fru_eboard_t *eboard_p,
                                                 fbe_raid_fru_error_status_t *fru_error_status_p);
fbe_bool_t fbe_mirror_is_sufficient_fruts(fbe_raid_siots_t *siots_p);
fbe_status_t fbe_mirror_get_degraded_range_for_position(fbe_raid_siots_t *siots_p,
                                                        fbe_u32_t position,               
                                                        fbe_lba_t *degraded_offset_p,
                                                        fbe_block_count_t *degraded_blocks_p,
                                                        fbe_bool_t *rebuild_log_needed_p);
fbe_bool_t fbe_mirror_is_primary_position_degraded(fbe_raid_siots_t *siots_p);
fbe_u32_t fbe_mirror_get_parity_position(fbe_raid_siots_t *siots_p);
fbe_status_t fbe_mirror_reinit_fruts_for_mining(fbe_raid_siots_t * siots_p);
fbe_raid_status_t fbe_mirror_read_process_fruts_error(fbe_raid_siots_t *siots_p, 
                                                      fbe_raid_fru_eboard_t *eboard_p);
fbe_bool_t fbe_mirror_is_ok_to_use_parent_sgs(fbe_raid_siots_t *siots_p);
fbe_status_t fbe_mirror_count_nonuniform_sgs(fbe_raid_siots_t *siots_p,
                                             fbe_block_count_t blocks_to_count,
                                             fbe_block_count_t *blks_remaining_p,
                                             fbe_u32_t *sg_count_p);
fbe_status_t fbe_mirror_count_sgs_with_offset(fbe_raid_siots_t *siots_p,
                                              fbe_u32_t *sg_count_p);
fbe_status_t fbe_mirror_plant_sg_with_offset(fbe_raid_siots_t *siots_p,
                                             fbe_raid_fru_info_t *dst_info_p,
                                             fbe_sg_element_t *dst_sgl_p);
fbe_status_t fbe_mirror_get_primary_fru_info(fbe_raid_siots_t *siots_p,
                                             fbe_raid_fru_info_t *fru_info_p,
                                             fbe_raid_fru_info_t **primary_info_pp);
fbe_status_t fbe_mirror_setup_reconstruct_request(fbe_raid_siots_t * siots_p,
                                                  fbe_xor_mirror_reconstruct_t *reconstruct_p);
void fbe_mirror_region_complete_update_iots(fbe_raid_siots_t *siots_p);
fbe_status_t fbe_mirror_limit_request(fbe_raid_siots_t *siots_p,
                                      fbe_u16_t num_fruts_to_allocate,
                                      fbe_block_count_t num_of_blocks_to_allocate,
                                      fbe_status_t get_fru_info_fn(fbe_raid_siots_t *siots_p,                
                                                                   fbe_raid_fru_info_t *read_info_p,         
                                                                   fbe_raid_fru_info_t *write_info_p,        
                                                                   fbe_u16_t *num_sgs_p,
                                                                   fbe_bool_t b_log));
fbe_status_t fbe_mirror_validate_fruts(fbe_raid_siots_t *siots_p);

/**************************************** 
 *  fbe_mirror_read_util.c
 ****************************************/
fbe_status_t fbe_mirror_read_validate(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_mirror_read_reinit_fruts_from_bitmask(fbe_raid_siots_t *siots_p, 
                                                       fbe_raid_position_bitmask_t read_bitmask);
fbe_status_t fbe_mirror_read_remove_degraded_read_fruts(fbe_raid_siots_t *siots_p,
                                                         fbe_bool_t b_write_degraded);
fbe_status_t fbe_mirror_read_find_read_position(fbe_raid_siots_t *siots_p,
                                                fbe_u32_t known_errored_position,
                                                fbe_u32_t *new_primary_position_p,
                                                fbe_bool_t b_log);
fbe_status_t fbe_mirror_read_setup_sgs_normal(fbe_raid_siots_t * siots_p,
                                              fbe_raid_memory_info_t *memory_info_p);
fbe_status_t fbe_mirror_read_get_fru_info(fbe_raid_siots_t * siots_p,
                                          fbe_raid_fru_info_t *read_info_p,
                                          fbe_raid_fru_info_t *write_info_p,
                                          fbe_u16_t *num_sgs_p,
                                          fbe_bool_t b_log);
fbe_status_t fbe_mirror_read_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                   fbe_raid_fru_info_t *fruts_info_p);
fbe_status_t fbe_mirror_read_setup_resources(fbe_raid_siots_t * siots_p, 
                                             fbe_raid_fru_info_t *fruts_info_p);
fbe_status_t fbe_mirror_read_limit_request(fbe_raid_siots_t *siots_p);
fbe_status_t fbe_mirror_read_update_read_fruts_position(fbe_raid_siots_t *siots_p,
                                                        fbe_u32_t known_errored_position,
                                                        fbe_u32_t new_primary_position,
                                                        fbe_bool_t b_log);

fbe_status_t fbe_mirror_read_set_read_optimization(fbe_raid_siots_t *siots_p);

/**************************************** 
 *  fbe_mirror_read.c
 ****************************************/
fbe_raid_state_status_t fbe_mirror_read_state0(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_read_state1(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_read_state3(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_read_state4(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_read_state5(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_read_state6(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_read_state7(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_read_state9(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_read_state20(fbe_raid_siots_t *siots_p);

/**************************************** 
 *  fbe_mirror_write_util.c
 ****************************************/
fbe_status_t fbe_mirror_write_reinit_fruts_from_bitmask(fbe_raid_siots_t * siots_p, 
                                                        fbe_raid_position_bitmask_t write_bitmask);
fbe_status_t fbe_mirror_write_reset_fruts_from_write_chain(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_mirror_write_remove_degraded_write_fruts(fbe_raid_siots_t *siots_p,
                                                          fbe_bool_t b_write_degraded);
fbe_status_t fbe_mirror_write_setup_sgs_normal(fbe_raid_siots_t * siots_p,
                                               fbe_raid_memory_info_t *memory_info_p);
fbe_raid_status_t fbe_mirror_write_process_fruts_error(fbe_raid_siots_t *siots_p, 
                                                       fbe_raid_fru_eboard_t *eboard_p);
fbe_status_t fbe_mirror_write_validate(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_mirror_write_get_fru_info(fbe_raid_siots_t * siots_p,
                                           fbe_raid_fru_info_t *preread_info_p,
                                           fbe_raid_fru_info_t *write_info_p,
                                           fbe_u16_t *num_sgs_p,
                                           fbe_bool_t b_log);
fbe_status_t fbe_mirror_write_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                    fbe_raid_fru_info_t *preread_info_p,
                                                    fbe_raid_fru_info_t *write_info_p);
fbe_status_t fbe_mirror_write_setup_resources(fbe_raid_siots_t * siots_p, 
                                              fbe_raid_fru_info_t *preread_info_p,
                                              fbe_raid_fru_info_t *write_info_p);
fbe_status_t fbe_mirror_write_limit_request(fbe_raid_siots_t *siots_p);
fbe_status_t fbe_mirror_write_validate_write_buffers(fbe_raid_siots_t *siots_p);
fbe_status_t fbe_mirror_write_populate_buffered_request(fbe_raid_siots_t *const siots_p);
fbe_status_t fbe_mirror_write_handle_corrupt_data(fbe_raid_siots_t * const siots_p);
fbe_status_t fbe_mirror_validate_unaligned_write(fbe_raid_siots_t *siots_p);

/**************************************** 
 *  fbe_mirror_write.c
 ****************************************/
fbe_raid_state_status_t fbe_mirror_write_state0(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_write_state0a(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_write_state1(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_write_state1a(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_write_state1b(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_write_state2(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_write_state2a(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_write_state2b(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_write_state2c(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_write_state2d(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_write_state2e(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_write_state2f(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_write_state3(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_write_state4(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_write_state7(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_write_state9(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_write_state20(fbe_raid_siots_t *siots_p);

/**************************************** 
 *  fbe_mirror_rebuild.c
 ****************************************/
fbe_raid_state_status_t fbe_mirror_rebuild_state0(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_rebuild_state1(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_rebuild_state3(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_rebuild_state4(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_rebuild_state5(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_rebuild_state6(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_rebuild_state7(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_rebuild_state8(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_rebuild_state9(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_rebuild_state10(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_rebuild_state18(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_rebuild_state21(fbe_raid_siots_t *siots_p);


/**************************************** 
 *  fbe_mirror_rebuild_util.c
 ****************************************/
fbe_status_t fbe_mirror_rebuild_validate(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_mirror_rebuild_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                      fbe_raid_fru_info_t *read_info_p,
                                                      fbe_raid_fru_info_t *write_info_p);
fbe_status_t fbe_mirror_rebuild_setup_resources(fbe_raid_siots_t * siots_p, 
                                                fbe_raid_fru_info_t *read_info_p,
                                                fbe_raid_fru_info_t *write_info_p);
fbe_status_t fbe_mirror_rebuild_setup_sgs_for_next_pass(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_mirror_rebuild_extent(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_mirror_rebuild_get_fru_info(fbe_raid_siots_t *siots_p,
                                             fbe_raid_fru_info_t *read_info_p,
                                             fbe_raid_fru_info_t *write_info_p,
                                             fbe_u16_t *num_sgs_p,
                                             fbe_bool_t b_log);
fbe_status_t fbe_mirror_rebuild_limit_request(fbe_raid_siots_t *siots_p);
fbe_status_t fbe_mirror_rebuild_invalidate_sectors(fbe_raid_siots_t * siots_p);

/**************************************** 
 *  fbe_mirror_verify.c
 ****************************************/
fbe_raid_state_status_t fbe_mirror_recovery_verify_state0(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_verify_state0(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_verify_state1(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_verify_state3(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_verify_state4(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_verify_state5(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_verify_state6(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_verify_state7(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_verify_state8(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_verify_state9(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_verify_state10(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_verify_state18(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_verify_state19(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_verify_state20(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_verify_state21(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_verify_state22(fbe_raid_siots_t * siots_p);

/**************************************** 
 *  fbe_mirror_verify_util.c
 ****************************************/
fbe_status_t fbe_mirror_verify_validate(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_mirror_verify_get_fru_info(fbe_raid_siots_t *siots_p,
                                            fbe_raid_fru_info_t *read_info_p,
                                            fbe_raid_fru_info_t *write_info_p,
                                            fbe_u16_t *num_sgs_p,
                                            fbe_bool_t b_log);
fbe_status_t fbe_mirror_verify_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                     fbe_raid_fru_info_t *read_info_p,
                                                     fbe_raid_fru_info_t *write_info_p);
fbe_status_t fbe_mirror_verify_setup_sgs_for_next_pass(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_mirror_verify_setup_resources(fbe_raid_siots_t *siots_p, 
                                               fbe_raid_fru_info_t *read_info_p,
                                               fbe_raid_fru_info_t *write_info_p);
fbe_status_t fbe_mirror_verify_extent(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_mirror_verify_limit_request(fbe_raid_siots_t *siots_p);
fbe_status_t fbe_mirror_report_errors(fbe_raid_siots_t * siots_p, fbe_bool_t b_update_eboard);
fbe_status_t fbe_mirror_report_errors_from_error_region(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_mirror_report_errors_from_eboard(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_mirror_log_correctable_error(fbe_raid_siots_t * siots_p,
                                     fbe_u32_t correctable_err_bits,
                                     fbe_u32_t region_idx,
                                     fbe_u16_t array_pos,
                                     fbe_u16_t relative_pos,
                                     fbe_lba_t data_pba);
fbe_status_t fbe_mirror_log_uncorrectable_error(fbe_raid_siots_t  * siots_p,
                                       fbe_u32_t uncorrectable_err_bits,
                                       fbe_u32_t region_idx,
                                       fbe_u16_t array_pos,
                                       fbe_u16_t relative_pos,
                                       fbe_lba_t data_pba);
fbe_bool_t fbe_mirror_only_invalidated_in_error_regions(fbe_raid_siots_t * siots_p);
fbe_u32_t fbe_raid_get_correctable_status_bits( const fbe_raid_vrts_t * const vrts_p, 
                                          fbe_u16_t position_key );
fbe_u32_t fbe_raid_get_uncorrectable_status_bits( const fbe_xor_error_t * const eboard_p, fbe_u16_t position_key );
void fbe_mirror_set_raw_mirror_status(fbe_raid_siots_t * siots_p, fbe_xor_status_t xor_status);
fbe_u16_t fbe_mirror_verify_get_write_bitmap(fbe_raid_siots_t * const siots_p);

/**************************************** 
 *  fbe_mirror_zero.c
 ****************************************/
fbe_raid_state_status_t fbe_mirror_zero_state0(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_mirror_zero_state1(fbe_raid_siots_t *siots_p);

/**************************************** 
 * fbe_mirror_read_optimization.c 
 ****************************************/
fbe_raid_state_status_t fbe_mirror_nway_mirror_optimize_fruts_start(fbe_raid_siots_t * siots_p,
                                                                  fbe_raid_fruts_t *fruts_p);
fbe_raid_state_status_t fbe_mirror_optimize_fruts_finish(fbe_raid_siots_t * siots_p,
                                                       fbe_raid_fruts_t *fruts_p);

/**************************************** 
 *  fbe_mirror_rekey.c
 ****************************************/
fbe_raid_state_status_t fbe_mirror_rekey_state0(fbe_raid_siots_t *siots_p);

/**************************************** 
 *  fbe_mirror_rekey_util.c
 ****************************************/
fbe_bool_t fbe_mirror_rekey_validate(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_mirror_rekey_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                     fbe_raid_fru_info_t *read_info_p,
                                                     fbe_u32_t *num_pages_to_allocate_p);
fbe_status_t fbe_mirror_rekey_setup_resources(fbe_raid_siots_t * siots_p, 
                                              fbe_raid_fru_info_t *read_p);

#endif /* FBE_MIRROR_COMMON_H */

/*******************************
 * end fbe_mirror_common.h
 *******************************/
