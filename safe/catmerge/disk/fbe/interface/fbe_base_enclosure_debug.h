#ifndef FBE_BASE_ENCLOSURE_DEBUG_H
#define FBE_BASE_ENCLOSURE_DEBUG_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_base_enclosure_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the base enclosure debug library.
 *
 * @author
 *   7-Apr-2009:  Created. Dipak Patel
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/


#include "fbe/fbe_enclosure.h"
#include "fbe_base_object.h"
#include "fbe/fbe_types.h"
#include "fbe_trace.h"

fbe_status_t fbe_base_enclosure_debug_prvt_data(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr base_enclosure_p,
                                           fbe_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context);

fbe_status_t
fbe_base_enclosure_debug_encl_port_data(const fbe_u8_t * module_name,
	                                       fbe_dbgext_ptr base_enclosure_p,
                                              fbe_trace_func_t trace_func, 
                                              fbe_trace_context_t trace_context);

 fbe_status_t fbe_base_enclosure_debug_get_encl_pos(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr base_enclosure_p, fbe_u32_t *encl_pos);

 fbe_status_t fbe_base_enclosure_debug_get_comp_id(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr base_enclosure_p, fbe_u32_t *comp_id);

 fbe_status_t fbe_base_enclosure_debug_get_port_index(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr base_enclosure_p, fbe_u32_t *port_index);

fbe_status_t fbe_enclosure_get_obj_handle_and_class(const fbe_u8_t * module_name,
                                          fbe_dbgext_ptr topology_object_table_ptr,
                                          fbe_dbgext_ptr *control_handle_ptr, 
                                          fbe_class_id_t *class_id);
  
fbe_status_t fbe_enclosure_print_debug_prvt_data(const fbe_u8_t * module_name,
                                            fbe_dbgext_ptr control_handle_ptr, 
                                            fbe_class_id_t class_id,
                                            int i);

void fbe_base_enclosure_print_prvt_data(void * enclPrvtData,
                                 fbe_trace_func_t trace_func,
                                 fbe_trace_context_t trace_context);

void fbe_base_enclosure_print_enclosure_number(fbe_u32_t enclNo, 
                                                     fbe_trace_func_t trace_func, 
                                                     fbe_trace_context_t trace_context);


void fbe_base_enclosure_print_port_number(fbe_u32_t portNo, 
                                                     fbe_trace_func_t trace_func, 
                                                     fbe_trace_context_t trace_context);

void fbe_base_enclosure_print_number_of_slots(fbe_u32_t slots, 
                                                     fbe_trace_func_t trace_func, 
                                                     fbe_trace_context_t trace_context);

void fbe_base_enclosure_print_first_slot_index(fbe_u16_t firstSlotIndex, 
                                                     fbe_trace_func_t trace_func, 
                                                     fbe_trace_context_t trace_context);

void fbe_base_enclosure_print_first_expansion_port_index(fbe_u16_t firstExpPortIndex, 
                                                     fbe_trace_func_t trace_func, 
                                                     fbe_trace_context_t trace_context);

void fbe_base_enclosure_print_number_of_expansion_ports(fbe_u32_t expPorts, 
                                                     fbe_trace_func_t trace_func, 
                                                     fbe_trace_context_t trace_context);

void fbe_base_enclosure_print_time_ride_through_start(fbe_time_t timeRideThroughStart, 
                                                     fbe_trace_func_t trace_func, 
                                                     fbe_trace_context_t trace_context);

void fbe_base_enclosure_print_expected_ride_through_period(fbe_u32_t expectedRideThroughPeriod, 
                                                     fbe_trace_func_t trace_func, 
                                                     fbe_trace_context_t trace_context);

void fbe_base_enclosure_print_default_ride_through_period(fbe_u32_t defaultRideThroughPeriod, 
                                                     fbe_trace_func_t trace_func, 
                                                     fbe_trace_context_t trace_context);

void fbe_base_enclosure_print_number_of_drives_spinningup(fbe_u32_t number_of_drives_spinningup, 
                                                     fbe_trace_func_t trace_func, 
                                                     fbe_trace_context_t trace_context);

void fbe_base_enclosure_print_max_number_of_drives_spinningup(fbe_u32_t max_number_of_drives_spinningup, 
                                                     fbe_trace_func_t trace_func, 
                                                     fbe_trace_context_t trace_context);

void fbe_base_enclosure_print_component_id(fbe_enclosure_component_id_t componentid, 
                                                     fbe_trace_func_t trace_func, 
                                                     fbe_trace_context_t trace_context);
void fbe_base_enclosure_print_encl_fault_symptom(fbe_u32_t flt_symptom, 
                                           fbe_trace_func_t trace_func, 
                                     fbe_trace_context_t trace_context);

void  fbe_base_enclosure_print_discovery_opcode(fbe_payload_discovery_opcode_t discovery_opcode,
                                                        fbe_trace_func_t trace_func, 
                                                        fbe_trace_context_t trace_context);

void  fbe_base_enclosure_print_control_code(fbe_payload_control_operation_opcode_t control_code,
                                                        fbe_trace_func_t trace_func, 
                                                        fbe_trace_context_t trace_context);

fbe_status_t fbe_base_enclosure_stat_debug_basic_info(const fbe_u8_t * module_name, 
                                           fbe_dbgext_ptr base_enclosure_p, 
                                           fbe_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context);

fbe_bool_t fbe_base_enclosure_if_scsi_error(fbe_u32_t scsi_error_code);

fbe_status_t fbe_base_pe_enclosure_stat_debug_basic_info(const fbe_u8_t * module_name, 
                                           fbe_dbgext_ptr base_board_p, 
                                           fbe_edal_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context);

#endif /* FBE_BASE_ENCLOSURE_DEBUG_H */
