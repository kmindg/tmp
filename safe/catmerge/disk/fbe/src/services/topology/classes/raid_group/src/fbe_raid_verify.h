// File: fbe_raid_verify.h

#ifndef FBE_RAID_VERIFY_H
#define FBE_RAID_VERIFY_H

//-----------------------------------------------------------------------------
// Copyright (C) EMC Corporation 2009-2010
// All rights reserved.
// Licensed material -- property of EMC Corporation
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
//  MODULE ABSTRACT: 
//
//  This header file defines the interface to the RAID group monitor's  
//  background verify source module.
//    
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:
//


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

// Run the next background verify cycle
fbe_status_t fbe_raid_group_run_user_verify(
                                        fbe_raid_group_t*   in_raid_group_p, 
                                        fbe_packet_t*       in_packet_p,
                                        fbe_lba_t           in_checkpoint,
                                        fbe_raid_verify_flags_t in_verify_flag);

// Get the next LBA to verify
fbe_status_t fbe_raid_get_next_verify_lba(
                                        fbe_raid_group_t*   in_raid_group_p, 
                                        fbe_packet_t*       in_packet_p,
                                        fbe_lba_t*          out_lba_p,
                                        fbe_block_edge_t**  out_block_edge_pp);

fbe_status_t fbe_raid_group_ask_verify_permission(
                                                  fbe_raid_group_t * in_raid_group_p,
                                                  fbe_packet_t * in_packet_p,                                                  
                                                  fbe_lba_t in_starting_lba,
                                                  fbe_block_count_t in_block_count);

//  Check if a verify I/O is currently allowed based on the I/O load 
fbe_status_t fbe_raid_group_verify_check_permission_based_on_load(
                                        fbe_raid_group_t*   in_raid_group_p,
                                        fbe_bool_t*         out_ok_to_proceed_b_p);

fbe_status_t fbe_raid_verify_update_verify_bitmap_completion(
                                                             fbe_packet_t*  in_packet_p,
                                                             fbe_packet_completion_context_t in_context);

/* check if verify background operation needs to run */
fbe_bool_t fbe_raid_group_background_op_is_verify_need_to_run(fbe_raid_group_t*    in_raid_group_p);

fbe_bool_t fbe_raid_group_background_op_is_journal_verify_need_to_run(fbe_raid_group_t*    raid_group_p);

fbe_status_t fbe_raid_group_verify_get_next_metadata_of_metadata_chunk_index(fbe_raid_group_t*           in_raid_group_p,
                                                                        fbe_chunk_index_t*      out_chunk_index_p,
                                                                        fbe_raid_verify_flags_t verify_flags);

fbe_status_t fbe_raid_group_verify_convert_block_opcode_to_verify_flags_without_obj_ptr(fbe_payload_block_operation_opcode_t in_opcode,
                                                                          fbe_raid_verify_flags_t*   out_verify_type_p);

fbe_status_t fbe_raid_group_verify_convert_verify_flags_to_opcode(fbe_raid_group_t*    raid_group_p,
                                                                  fbe_payload_block_operation_opcode_t *opcode,
                                                                  fbe_raid_verify_flags_t verify_type);

fbe_status_t fbe_raid_verify_initiate_error_verify_event(fbe_raid_group_t*    raid_group_p,
                                                       fbe_packet_t*        packet_p,
                                                       fbe_chunk_index_t    chunk_index,
                                                       fbe_chunk_count_t    chunk_count,
                                                         fbe_data_event_type_t  event_type);

fbe_status_t fbe_raid_verify_initiate_error_verify_event_completion(fbe_event_t*   event_p,
                                                               fbe_packet_completion_context_t context);

fbe_bool_t fbe_raid_group_get_next_chunk_marked_for_verify(fbe_u8_t*     search_data,
                                                            fbe_u32_t    search_size,
                                                            context_t    context);

fbe_lifecycle_status_t fbe_raid_group_monitor_run_journal_verify(fbe_base_object_t*  in_object_p,
                                                                  fbe_packet_t*       in_packet_p);

fbe_lifecycle_status_t fbe_raid_group_monitor_initialize_journal(fbe_base_object_t*  in_object_p,
                                                                 fbe_packet_t*       packet_p);
fbe_status_t fbe_raid_group_journal_memory_allocation_completion(fbe_memory_request_t * memory_request_p, 
                                                          fbe_memory_completion_context_t in_context);

fbe_status_t fbe_raid_group_journal_verify_subpacket_completion(fbe_packet_t* packet_p,
                                                       fbe_packet_completion_context_t context);

fbe_status_t fbe_raid_group_journal_remap_memory_allocation_completion(fbe_memory_request_t * memory_request_p, 
                                                                       fbe_memory_completion_context_t in_context);

fbe_status_t fbe_raid_group_journal_remap_subpacket_completion(fbe_packet_t* packet_p,
                                                               fbe_packet_completion_context_t context);

fbe_status_t fbe_raid_group_monitor_run_journal_verify_completion(fbe_packet_t *packet_p,
                                                              fbe_packet_completion_context_t context);

fbe_status_t fbe_raid_group_set_journal_checkpoint_to_invalid(fbe_packet_t * packet_p,
                                                                    fbe_packet_completion_context_t context);

fbe_status_t fbe_raid_group_verify_convert_type_to_verify_flags(fbe_raid_group_t*    in_raid_group_p,
                                                                fbe_raid_verify_type_t verify_type,
                                                                fbe_raid_verify_flags_t*   verify_flags_p);

fbe_status_t fbe_raid_group_verify_convert_block_opcode_to_verify_flags(fbe_raid_group_t*    raid_group_p,
                                                                  fbe_payload_block_operation_opcode_t opcode,
                                                                  fbe_raid_verify_flags_t*   verify_flags_p);

fbe_status_t fbe_striper_send_command_to_mirror_objects(fbe_packet_t * packet_p,
                                                        fbe_base_object_t * base_object_p,
                                                        fbe_raid_geometry_t * raid_geometry_p);

fbe_status_t  fbe_raid_group_verify_initiate_non_paged_metadata_clear_bits(fbe_packet_t*     in_packet_p,
                                                                           fbe_raid_group_t* in_raid_group_p,
                                                                           fbe_lba_t verify_lba,
                                                                           fbe_block_count_t blocks);


#endif // FBE_RAID_VERIFY_H
