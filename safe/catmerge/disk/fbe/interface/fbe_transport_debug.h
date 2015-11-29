#ifndef FBE_TRANSPORT_DEBUG_H
#define FBE_TRANSPORT_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2008-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_transport_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains declarations of the display functions for block 
 *  transport.
 *
 * @author
 *   02/13/2009:  Created. Nishit Singh
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"

/*************************
 *  FUNCTION DECLARATIONS
 *************************/

fbe_status_t
fbe_block_transport_print_identify(const fbe_u8_t * module_name,
                                   fbe_u64_t block_transport_identify_p,
                                   fbe_trace_func_t trace_func,
                                   fbe_trace_context_t trace_context);

fbe_status_t fbe_block_transport_display_edge(const fbe_u8_t * module_name,
                                              fbe_dbgext_ptr rdgen_ts_ptr,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_u32_t spaces_to_indent);

fbe_status_t
fbe_transport_print_base_edge(const fbe_u8_t * module_name,
                              fbe_u64_t base_edge_p,
                              fbe_trace_func_t trace_func,
                              fbe_trace_context_t trace_context);

fbe_status_t
fbe_discovery_transport_print_detail(const fbe_u8_t * module_name,
                                     fbe_u64_t base_discovered_p,
                                     fbe_trace_func_t trace_func,
                                     fbe_trace_context_t trace_context);

fbe_status_t
fbe_block_transport_print_server_detail(const fbe_u8_t * module_name,
                                        fbe_u64_t block_transport_p,
                                        fbe_trace_func_t trace_func,
                                        fbe_trace_context_t trace_context);

fbe_status_t
fbe_transport_get_queue_size(const fbe_u8_t * module_name,
                             fbe_u64_t queue_p,
                             fbe_trace_func_t trace_func,
                             fbe_trace_context_t trace_context,
                             fbe_u32_t * queue_size);

fbe_status_t
fbe_transport_print_packet_queue(const fbe_u8_t * module_name,
                                 fbe_u64_t queue_p,
                                 fbe_trace_func_t trace_func,
                                 fbe_trace_context_t trace_context,
                                 fbe_u32_t spaces_to_indent);
fbe_status_t fbe_packet_status_debug_trace_no_field_ptr(const fbe_u8_t * module_name,
                                                        fbe_dbgext_ptr base_ptr,
                                                        fbe_trace_func_t trace_func,
                                                        fbe_trace_context_t trace_context,
                                                        fbe_u32_t spaces_to_indent);

fbe_status_t 
fbe_transport_print_fbe_packet(const fbe_u8_t * module_name,
                               fbe_u64_t fbe_packet_p,
                               fbe_trace_func_t trace_func,
                               fbe_trace_context_t trace_context,
                               fbe_u32_t spaces_to_indent);

fbe_status_t fbe_transport_should_display_payload_iots(const fbe_u8_t * module_name, fbe_u64_t packet_p,
                                                       fbe_u64_t payload_p);
fbe_status_t fbe_debug_display_packet_summary(const fbe_u8_t * module_name,
                                              fbe_dbgext_ptr base_ptr,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_debug_field_info_t *field_info_p,
                                              fbe_u32_t spaces_to_indent);
fbe_status_t fbe_transport_print_packet_summary(const fbe_u8_t * module_name,
                                                fbe_u64_t fbe_packet_p,
                                                fbe_trace_func_t trace_func,
                                                fbe_trace_context_t trace_context,
                                                fbe_u32_t spaces_to_indent);

fbe_status_t fbe_transport_print_packet_field_summary(const fbe_u8_t * module_name,
                                                      fbe_u64_t fbe_packet_p,
                                                      fbe_trace_func_t trace_func,
                                                      fbe_trace_context_t trace_context,
                                                      fbe_u32_t spaces_to_indent);

fbe_status_t fbe_memory_request_debug_trace(const fbe_u8_t * module_name,
                                            fbe_dbgext_ptr base_ptr,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context,
                                            fbe_debug_field_info_t *field_info_p,
                                            fbe_u32_t spaces_to_indent);
extern fbe_debug_queue_info_t fbe_packet_debug_packet_queue_info;

fbe_status_t fbe_memory_request_debug_trace_basic(const fbe_u8_t * module_name,
                                                  fbe_dbgext_ptr base_ptr,
                                                  fbe_trace_func_t trace_func,
                                                  fbe_trace_context_t trace_context,
                                                  fbe_u32_t spaces_to_indent);

fbe_status_t 
fbe_transport_print_fbe_packet_payload(const fbe_u8_t * module_name,
                                       fbe_u64_t fbe_payload_p,
                                       fbe_trace_func_t trace_func,
                                       fbe_trace_context_t trace_context,
                                       fbe_u32_t spaces_to_indent);

fbe_status_t
fbe_tansport_print_fbe_packet_block_payload(const fbe_u8_t * module_name,
                                            fbe_u64_t block_operation_p,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context,
                                            fbe_u32_t spaces_to_indent);

fbe_status_t
fbe_transport_print_block_opcode(fbe_u32_t transport_id,
                                 fbe_trace_func_t trace_func,
                                 fbe_trace_context_t trace_context);

fbe_status_t 
fbe_tansport_print_fbe_packet_cdb_payload(const fbe_u8_t * module_name,
                                            fbe_u64_t cdb_operation_p,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context,
                                            fbe_u32_t spaces_to_indent);

fbe_status_t 
fbe_transport_print_port_request_status(fbe_u32_t  port_request_status,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context);


fbe_status_t 
fbe_tansport_print_fbe_packet_fis_payload(const fbe_u8_t * module_name,
                                            fbe_u64_t fis_operation_p,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context,
                                            fbe_u32_t spaces_to_indent);

fbe_status_t 
fbe_tansport_print_fbe_packet_dmrb_payload(const fbe_u8_t * module_name,
                                            fbe_u64_t dmrb_operation_p,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context,
                                            fbe_u32_t spaces_to_indent);

fbe_status_t fbe_tansport_print_fbe_packet_control_payload(
											const fbe_u8_t * module_name,
                                            fbe_u64_t control_operation_p,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context,
                                            fbe_u32_t spaces_to_indent);
fbe_status_t fbe_transport_print_stripe_lock_payload(const fbe_u8_t * module_name,
                                                     fbe_u64_t stripe_lock_operation_p,
                                                     fbe_trace_func_t trace_func,
                                                     fbe_trace_context_t trace_context,
                                                     fbe_u32_t spaces_to_indent);

fbe_status_t fbe_transport_print_memory_payload(const fbe_u8_t * module_name,
                                                     fbe_u64_t memory_operation_p,
                                                     fbe_trace_func_t trace_func,
                                                     fbe_trace_context_t trace_context,
                                                     fbe_u32_t spaces_to_indent);
fbe_status_t fbe_transport_print_buffer_payload(const fbe_u8_t * module_name,
                                                fbe_u64_t buffer_operation_p,
                                                fbe_trace_func_t trace_func,
                                                fbe_trace_context_t trace_context,
                                                fbe_u32_t spaces_to_indent);
fbe_status_t fbe_metadata_sl_region_display(const fbe_u8_t * module_name,
                                                   fbe_dbgext_ptr base_ptr,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_debug_field_info_t *field_info_p,
                                                   fbe_u32_t spaces_to_indent);
fbe_status_t fbe_metadata_cmi_message_header_display(const fbe_u8_t * module_name,
                                                     fbe_dbgext_ptr base_ptr,
                                                     fbe_trace_func_t trace_func,
                                                     fbe_trace_context_t trace_context,
                                                     fbe_debug_field_info_t *field_info_p,
                                                     fbe_u32_t spaces_to_indent);
fbe_status_t fbe_transport_print_metadata_payload(const fbe_u8_t * module_name,
                                                  fbe_u64_t metadata_operation_p,
                                                  fbe_trace_func_t trace_func,
                                                  fbe_trace_context_t trace_context,
                                                  fbe_u32_t spaces_to_indent);
fbe_status_t fbe_payload_metadata_operation_union_debug(const fbe_u8_t * module_name,
                                                        fbe_dbgext_ptr base_ptr,
                                                        fbe_trace_func_t trace_func,
                                                        fbe_trace_context_t trace_context,
                                                        fbe_debug_field_info_t *field_info_p,
                                                        fbe_u32_t spaces_to_indent);
fbe_status_t fbe_block_edge_debug_trace(const fbe_u8_t * module_name,
                                       fbe_dbgext_ptr object_ptr,
                                       fbe_trace_func_t trace_func,
                                       fbe_trace_context_t trace_context,
                                       fbe_debug_field_info_t *field_info_p,
                                       fbe_u32_t spaces_to_indent);
fbe_status_t fbe_base_edge_debug_trace(const fbe_u8_t * module_name,
                                       fbe_dbgext_ptr object_ptr,
                                       fbe_trace_func_t trace_func,
                                       fbe_trace_context_t trace_context,
                                       fbe_debug_field_info_t *field_info_p,
                                                     fbe_u32_t spaces_to_indent);
 fbe_status_t fbe_block_edge_info_debug_trace(const fbe_u8_t * module_name,
                                                   fbe_dbgext_ptr object_ptr,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_debug_field_info_t *field_info_p,
                                                   fbe_u32_t spaces_to_indent);
  fbe_status_t fbe_base_edge_info_debug_trace(const fbe_u8_t * module_name,
                                                   fbe_dbgext_ptr object_ptr,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_debug_field_info_t *field_info_p,
                                                   fbe_u32_t spaces_to_indent);
fbe_status_t fbe_payload_block_debug_print_summary(const fbe_u8_t * module_name,
                                                   fbe_dbgext_ptr base_ptr,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_debug_field_info_t *field_info_p,
                                                   fbe_u32_t spaces_to_indent);
fbe_status_t fbe_block_transport_dump_edge(const fbe_u8_t * module_name,
                                              fbe_dbgext_ptr rdgen_ts_ptr,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_u32_t spaces_to_indent);
fbe_status_t fbe_base_transport_server_dump_debug_trace(const fbe_u8_t * module_name,
                                                   fbe_dbgext_ptr object_ptr,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_debug_field_info_t *field_info_p,
                                                   fbe_u32_t spaces_to_indent);
fbe_status_t fbe_block_edge_dump_debug_trace(const fbe_u8_t * module_name,
                                       fbe_dbgext_ptr object_ptr,
                                       fbe_trace_func_t trace_func,
                                       fbe_trace_context_t trace_context,
                                       fbe_debug_field_info_t *field_info_p,
                                       fbe_u32_t spaces_to_indent);
fbe_status_t fbe_base_edge_dump_debug_trace(const fbe_u8_t * module_name,
                                       fbe_dbgext_ptr object_ptr,
                                       fbe_trace_func_t trace_func,
                                       fbe_trace_context_t trace_context,
                                       fbe_debug_field_info_t *field_info_p,
                                                     fbe_u32_t spaces_to_indent);
fbe_status_t fbe_block_edge_info_dump_debug_trace(const fbe_u8_t * module_name,
                                                   fbe_dbgext_ptr object_ptr,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_debug_field_info_t *field_info_p,
                                                   fbe_u32_t spaces_to_indent);
fbe_status_t fbe_base_edge_info_dump_debug_trace(const fbe_u8_t * module_name,
                                                   fbe_dbgext_ptr object_ptr,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_debug_field_info_t *field_info_p,
                                                   fbe_u32_t spaces_to_indent);

fbe_status_t fbe_block_transport_dump_server_detail(const fbe_u8_t * module_name,
                                                     fbe_u64_t block_transport_p,
                                                     fbe_trace_func_t trace_func,
                                                     fbe_trace_context_t trace_context,
                                                     int spaces_to_indent);

#endif /* FBE_TRANSPORT_DEBUG_H */
