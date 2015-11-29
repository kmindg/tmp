#ifndef FBE_RAID_GROUP_BITMAP_H
#define FBE_RAID_GROUP_BITMAP_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_raid_group_bitmap.h
 ***************************************************************************
 *
 * @brief
 *   This file contains the private defines and function prototypes to access 
 *   the bitmap for the raid group object.
 * 
 * @ingroup raid_group_class_files
 * 
 * @version
 *   10/27/2009:  Created. Jean Montes.
 *
 ***************************************************************************/


//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:
//

#include "fbe_raid_group_object.h"             // for the raid group object 

//-----------------------------------------------------------------------------
//  NAMED CONSTANTS (#defines):


//-----------------------------------------------------------------------------
//  ENUMERATIONS:


//-----------------------------------------------------------------------------
//  TYPEDEFS: 
//

//-----------------------------------------------------------------------------
//  MACROS:


//-----------------------------------------------------------------------------
//  FUNCTION PROTOTYPES: 


//  Read the chunk info for a range of chunks 
fbe_status_t fbe_raid_group_bitmap_read_chunk_info_for_range(
                                    fbe_raid_group_t*                   in_raid_group_p,
                                    fbe_packet_t*                       in_packet_p,
                                    fbe_chunk_index_t                   in_start_chunk_index, 
                                    fbe_chunk_count_t                   in_chunk_count);

//  Get the chunk index corresponding to the given LBA
fbe_status_t fbe_raid_group_bitmap_get_chunk_index_for_lba(
                                    fbe_raid_group_t*                   in_raid_group_p,
                                    fbe_lba_t                           in_lba,           
                                    fbe_chunk_index_t*                  out_chunk_index_p);

//  If needed, update the nonpaged metadata for the RG as part of paged metadata reconstruction
fbe_status_t fbe_raid_group_bitmap_reconstruct_paged_update_nonpaged_if_needed(
                                        fbe_raid_group_t*                    in_raid_group_p,
                                        fbe_chunk_index_t                    in_paged_md_chunk_index,
                                        fbe_chunk_count_t                    in_paged_md_chunk_count,
                                        fbe_raid_group_nonpaged_metadata_t*  out_nonpaged_metadata_p,
                                        fbe_bool_t*                          out_nonpaged_md_changed_b);   
//  Get the chunk range for the given LBA range
fbe_status_t fbe_raid_group_bitmap_get_chunk_range(
                                    fbe_raid_group_t*   in_raid_group_p,
                                    fbe_lba_t           in_lba,
                                    fbe_block_count_t   in_block_count,
                                    fbe_chunk_index_t*  out_chunk_index_p,
                                    fbe_chunk_count_t*  out_chunk_count_p);

//  Get the chunk index in the RAID group corresponding to a paged metadata metadata chunk index 
fbe_status_t fbe_raid_group_bitmap_translate_paged_md_md_index_to_rg_chunk_index(
                                    fbe_raid_group_t*                   in_raid_group_p,
                                    fbe_chunk_index_t                   in_chunk_index, 
                                    fbe_lba_t                           current_checkpoint,
                                    fbe_chunk_index_t*                  out_chunk_index_p);

fbe_status_t fbe_raid_group_bitmap_determine_if_lba_range_span_user_and_paged_metadata(
                                    fbe_raid_group_t *  in_raid_group_p,
                                    fbe_lba_t           start_lba,
                                    fbe_block_count_t   block_count,
                                    fbe_bool_t*         is_range_span_across_b_p);

// Mark needs verify for the specified bitmap chunk
fbe_status_t fbe_raid_group_bitmap_mark_verify_for_chunk(
                                    fbe_packet_t*                       in_packet_p,
                                    fbe_raid_group_t*                   in_raid_group_p,
                                    fbe_chunk_index_t                   in_chunk_index,
                                    fbe_raid_verify_type_t              in_verify_type,
                                    fbe_chunk_count_t                   in_chunk_count);

// Unmark needs verify for the specified bitmap chunk
fbe_status_t fbe_raid_group_bitmap_clear_verify_for_chunk(
                                            fbe_packet_t*          in_packet_p,    
                                            fbe_raid_group_t*      in_raid_group_p);

fbe_status_t fbe_raid_group_verify_handle_chunks(fbe_raid_group_t *raid_group_p,
                                                 fbe_packet_t *packet_p);

// Find the next bitmap chunk to verify
fbe_status_t fbe_raid_group_bitmap_find_next_verify_chunk(
                                    fbe_packet_t*                   in_packet_p,
                                    fbe_raid_group_t*               in_raid_group_p,
                                    fbe_lba_t                       in_checkpoint,
                                    fbe_raid_verify_flags_t         in_verify_flag);


// Get the chunk index and chunk count for a given LUN edge
fbe_status_t fbe_raid_group_bitmap_get_lun_edge_extent_data(
                                    fbe_raid_group_t*                   in_raid_group_p,
                                    fbe_block_edge_t*                   in_lun_edge_p,
                                    fbe_chunk_index_t*                  out_chunk_index_p,
                                    fbe_chunk_count_t*                  out_chunk_count_p);

fbe_status_t fbe_raid_group_bitmap_get_chunk_index_and_chunk_count(
                                    fbe_raid_group_t*   in_raid_group_p,
                                    fbe_lba_t           offset,
                                    fbe_lba_t           block_count,
                                    fbe_chunk_index_t*  out_chunk_index_p,
                                    fbe_chunk_count_t*  out_chunk_count_p);

// Determine if a chunk is marked for verify
fbe_bool_t fbe_raid_group_bitmap_is_chunk_marked_for_verify(
                                    fbe_raid_group_t*                   in_raid_group_p,
                                    fbe_chunk_index_t                   in_chunk_index,
                                    fbe_raid_verify_flags_t             in_verify_flags);

fbe_status_t fbe_raid_group_bitmap_set_pmd_err_vr_completion(
                                    fbe_packet_t*                       in_packet_p,
                                    fbe_packet_completion_context_t     in_context);

fbe_status_t fbe_raid_group_bitmap_set_non_paged_metadata_for_verify(fbe_packet_t*   in_packet_p,
                                                              fbe_raid_group_t* in_raid_group_p,
                                                              fbe_chunk_index_t in_chunk_index,
                                                              fbe_chunk_count_t in_chunk_count,
                                                              fbe_packet_completion_function_t completion_function,
                                                              fbe_u64_t metadata_offset,
                                                              fbe_raid_verify_flags_t in_verify_flag);

fbe_status_t fbe_raid_group_verify_get_paged_metadata_completion(fbe_packet_t*  in_packet_p,
                                                          fbe_packet_completion_context_t context);

fbe_status_t fbe_raid_group_bitmap_mark_paged_metadata_verify(
                                    fbe_packet_t*                   in_packet_p,
                                    fbe_packet_completion_context_t in_context);

fbe_status_t fbe_raid_group_bitmap_metadata_populate_stripe_lock(fbe_raid_group_t *raid_group_p,
                                                                 fbe_payload_metadata_operation_t *metadata_operation_p,
                                                                 fbe_chunk_count_t chunk_count);
fbe_status_t fbe_raid_group_bitmap_get_lba_for_chunk_index(
                                    fbe_raid_group_t*                   in_raid_group_p,
                                    fbe_chunk_index_t                   in_chunk_index,
                                    fbe_lba_t*                          out_lba_p);

fbe_status_t fbe_raid_group_update_verify_checkpoint(
                                                  fbe_packet_t*  in_packet_p,
                                                  fbe_raid_group_t* in_raid_group_p,
                                                  fbe_lba_t         current_checkpoint,
												  fbe_lba_t         block_count,
                                                  fbe_raid_verify_flags_t in_verify_flags,
                                                  const char *caller);

fbe_status_t fbe_raid_group_ask_upstream_for_permission(
                                    fbe_packet_t*          in_packet_p,
                                    fbe_raid_group_t*      in_raid_group_p,
                                    fbe_lba_t              verify_lba,                                    
                                    fbe_raid_verify_flags_t in_verify_flags);

fbe_status_t 
fbe_raid_group_bitmap_mark_mdd_verify_block_op(fbe_packet_t* in_packet_p,
                                               fbe_packet_completion_context_t in_context);


fbe_status_t fbe_raid_group_bitmap_mark_metadata_of_metadata_verify(fbe_packet_t* in_packet_p, 
                                                                    fbe_raid_group_t* raid_group_p,
                                                                    fbe_raid_verify_flags_t verify_flags);

fbe_status_t fbe_raid_group_bitmap_initiate_non_paged_metadata_update(
                                    fbe_packet_t*                   in_packet_p,
                                    fbe_raid_group_t*               raid_group_p);

fbe_status_t fbe_raid_group_allocate_block_operation_and_send_to_executor(
                                fbe_raid_group_t*                       in_raid_group_p,
                                fbe_packet_t*                           in_packet_p,
                                fbe_lba_t                               verify_lba,
                                fbe_block_count_t                       block_count,
                                fbe_payload_control_operation_opcode_t  in_opcode);

fbe_status_t fbe_raid_group_get_control_buffer_for_verify(
                                        fbe_raid_group_t*                   in_raid_group_p,
                                        fbe_packet_t*                       in_packet_p,
                                        fbe_payload_control_buffer_length_t in_buffer_length,
                                        fbe_payload_control_buffer_t*       out_buffer_pp);

fbe_status_t fbe_raid_group_np_set_bits(fbe_packet_t * packet_p,                                    
                                        fbe_raid_group_t* raid_group_p,
                                        fbe_raid_position_bitmask_t   nr_pos_bitmask);

fbe_status_t fbe_raid_group_mark_verify(fbe_packet_t * packet_p,
                                        fbe_packet_completion_context_t context);

fbe_status_t fbe_raid_group_verify_read_paged_metadata(fbe_packet_t * packet_p,
                                         fbe_packet_completion_context_t context);

fbe_status_t fbe_raid_group_bitmap_determine_if_chunks_in_data_area(
                                    fbe_raid_group_t*                   in_raid_group_p,
                                    fbe_chunk_index_t                   in_start_chunk_index, 
                                    fbe_chunk_count_t                   in_chunk_count,
                                    fbe_bool_t*                         out_is_in_data_area_b_p);

//  Get the chunk index in the paged metadata metadata corresponding to a "normal", rg-based chunk index 
fbe_status_t fbe_raid_group_bitmap_translate_rg_chunk_index_to_paged_md_md_index(
                                    fbe_raid_group_t*                   in_raid_group_p,
                                    fbe_chunk_index_t                   in_chunk_index, 
                                    fbe_chunk_index_t*                  out_chunk_index_p);

//  Perform checks and retrive the data when the paged metadata chunk info is read, via the nonpaged metadata
fbe_status_t fbe_raid_group_bitmap_process_chunk_info_read_using_nonpaged_done(
                                    fbe_raid_group_t*                   in_raid_group_p,
                                    fbe_packet_t*                       in_packet_p,
                                    fbe_chunk_index_t                   in_start_chunk_index, 
                                    fbe_chunk_count_t                   in_chunk_count,
                                    fbe_raid_group_paged_metadata_t*    out_chunk_entry_p);

fbe_status_t fbe_raid_group_bitmap_get_phy_chunks_per_md_of_md(fbe_raid_group_t *raid_group_p,
                                                        fbe_chunk_count_t *chunk_count_p);

fbe_status_t fbe_raid_group_bitmap_populate_iots_chunk_info(fbe_raid_group_t *raid_group_p,
                                                            fbe_chunk_index_t  start_chunk_index,
                                                            fbe_chunk_count_t chunk_count,
                                                            fbe_raid_group_paged_metadata_t*    out_chunk_entry_p);

fbe_status_t fbe_raid_group_reconstruct_md_acquire_lock_completion(fbe_packet_t* in_packet_p,
                                                                   fbe_packet_completion_context_t in_context);

fbe_status_t fbe_raid_group_bitmap_reconstruct_paged_md_nonpaged_write_completion(
                                            fbe_packet_t*                      in_packet_p,
                                            fbe_packet_completion_context_t    in_context);

fbe_status_t fbe_raid_group_bitmap_reconstruct_paged_md_update_paged(
                                fbe_raid_group_t*                   in_raid_group_p,
                                fbe_packet_t*                       in_packet_p);

fbe_status_t fbe_raid_group_bitmap_reconstruct_paged_md_paged_write_completion(
                                            fbe_packet_t*                      in_packet_p,
                                            fbe_packet_completion_context_t    in_context);

fbe_status_t fbe_raid_group_bitmap_get_num_of_bits_we_can_clear_in_mdd(
                                    fbe_raid_group_t*      raid_group_p,
                                    fbe_chunk_index_t      chunk_index, 
                                    fbe_chunk_count_t      chunk_count,
                                    fbe_bool_t*            clear_nr_bits_p);

fbe_status_t fbe_raid_group_mark_journal_for_verify(
                                            fbe_packet_t*               packet_p,
                                            fbe_raid_group_t*           raid_group_p,
                                            fbe_lba_t                   raid_lba);

fbe_status_t fbe_raid_group_mark_journal_for_verify_completion(
                                                           fbe_packet_t*       packet_p,
                                                           fbe_packet_completion_context_t  context);

fbe_status_t fbe_raid_group_journal_verify(fbe_packet_t * packet_p,                                    
                                        fbe_packet_completion_context_t context);

fbe_status_t fbe_raid_group_plant_sgl_with_zero_buffer(fbe_sg_element_t * sg_list,
                                                       fbe_u32_t sg_list_count,
                                                       fbe_block_count_t block_count,
                                                       fbe_u32_t * used_sg_elements_p);

fbe_status_t fbe_raid_group_handle_metadata_error(fbe_raid_group_t*    raid_group_p,
                                                  fbe_packet_t*        packet_p,
                                                  fbe_payload_metadata_status_t   metadata_status); 

fbe_status_t fbe_rg_bitmap_update_chkpt_for_unconsumed_chunks(fbe_packet_t * packet_p,                                    
                                        fbe_packet_completion_context_t context);

fbe_status_t fbe_raid_group_bitmap_determine_nr_bits_from_nonpaged(
                                        fbe_raid_group_t*                   in_raid_group_p,
                                        fbe_chunk_index_t                   in_rg_user_data_chunk_index,
                                        fbe_chunk_count_t                   in_rg_user_data_chunk_count,
                                        fbe_raid_position_bitmask_t *needs_rebuild_bits_p);

fbe_status_t fbe_raid_group_bitmap_get_next_marked_paged_metadata(
                                        fbe_raid_group_t*                   raid_group_p,
                                        fbe_packet_t*                       packet_p,
                                        fbe_chunk_index_t                   start_chunk_index,
                                        fbe_chunk_count_t                   chunk_count,
                                        metadata_search_fn_t                search_fn,
                                        void*                               context);
fbe_status_t fbe_raid_group_bitmap_get_next_marked(fbe_raid_group_t*                   raid_group_p,
                                                   fbe_packet_t*                       packet_p,
                                                   fbe_chunk_index_t                   start_chunk_index,
                                                   fbe_chunk_count_t                   chunk_count,
                                                   metadata_search_fn_t                search_fn,
                                                   void*                               context);

//  Read the metadata for chunks representing the paged data, which is done via the nonpaged metadata
fbe_status_t fbe_raid_group_bitmap_read_chunk_info_using_nonpaged(
                                    fbe_raid_group_t*                   in_raid_group_p,
                                    fbe_packet_t*                       in_packet_p,
                                    fbe_chunk_index_t                   in_start_chunk_index,
                                    fbe_chunk_count_t                   in_chunk_count);

//  Write the metadata for chunks representing the paged data, which is done via the nonpaged metadata
fbe_status_t fbe_raid_group_bitmap_write_chunk_info_using_nonpaged(
                                    fbe_raid_group_t*                   raid_group_p,
                                    fbe_packet_t*                       packet_p,
                                    fbe_chunk_index_t                   in_start_chunk_index,
                                    fbe_chunk_count_t                   in_chunk_count,
                                    fbe_raid_group_paged_metadata_t*    in_chunk_data_p, 
                                    fbe_bool_t                          in_perform_clear_bits_b);

//  Update the paged metadata as specified for a chunk or range of chunks 
fbe_status_t fbe_raid_group_bitmap_update_paged_metadata(
                                        fbe_raid_group_t*                   in_raid_group_p,
                                        fbe_packet_t*                       packet_p,
                                        fbe_chunk_index_t                   in_start_chunk_index,
                                        fbe_chunk_count_t                   in_chunk_count,
                                        fbe_raid_group_paged_metadata_t*    in_chunk_data_p, 
                                        fbe_bool_t                          in_perform_clear_bits_b);


//  Completion function when a paged metadata update completes
fbe_status_t fbe_raid_group_bitmap_update_paged_metadata_completion(
                                    fbe_packet_t*                       in_packet_p,
                                    fbe_packet_completion_context_t     in_context); 

fbe_status_t fbe_raid_group_bitmap_translate_metadata_status_to_packet_status(
                                    fbe_raid_group_t*                   in_raid_group_p,
                                    fbe_payload_metadata_status_t       in_metadata_status,                    
                                    fbe_status_t*                       out_packet_status_p);
fbe_status_t fbe_raid_group_initiate_lun_extent_init_metadata(fbe_raid_group_t * const raid_group_p, 
                                                              fbe_packet_t * const packet_p);
fbe_status_t fbe_raid_group_bitmap_initiate_reinit_metadata(
                                    fbe_packet_t*                   packet_p,
                                    fbe_packet_completion_context_t in_context);
fbe_status_t fbe_raid_group_bitmap_paged_metadata_block_op_completion(
                                    fbe_packet_t*                   in_packet_p,
                                    fbe_packet_completion_context_t in_context);
                
//  Determine the user data chunk range from the given paged metadata chunk range for the RG
fbe_status_t fbe_raid_group_bitmap_determine_user_chunk_range_from_paged_md_chunk_range(
                                        fbe_raid_group_t*                   in_raid_group_p,
                                        fbe_chunk_index_t                   in_paged_md_chunk_index,
                                        fbe_chunk_count_t                   in_paged_md_chunk_count,
                                        fbe_chunk_index_t*                  out_rg_user_data_start_chunk_index,
                                        fbe_chunk_count_t*                  out_rg_user_data_chunk_count); 

//  Set the given paged metadata bits based on the nonpaged metadata
fbe_status_t fbe_raid_group_bitmap_set_paged_bits_from_nonpaged(
                                        fbe_raid_group_t*                   in_raid_group_p,
                                        fbe_chunk_index_t                   in_rg_user_data_chunk_index,
                                        fbe_chunk_count_t                   in_rg_user_data_chunk_count,
                                        fbe_raid_group_paged_metadata_t*    out_paged_md_bits_p);

#endif // FBE_RAID_GROUP_BITMAP_H

