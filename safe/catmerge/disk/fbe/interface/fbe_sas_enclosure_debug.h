#ifndef FBE_SAS_ENCLOSURE_DEBUG_H
#define FBE_SAS_ENCLOSURE_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_sas_enclosure_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the sas enclosure debug library.
 *
 * @author
 *   12/9/2008:  Created. bphilbin
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

#include "fbe/fbe_types.h"
#include "fbe_trace.h"

fbe_status_t fbe_sas_enclosure_debug_basic_info(const fbe_u8_t * module_name, 
                                           fbe_dbgext_ptr eses_enclosure_p, 
                                           fbe_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context);

fbe_status_t fbe_sas_enclosure_debug_extended_info(const fbe_u8_t * module_name, 
                                           fbe_dbgext_ptr sas_enclosure_p, 
                                           fbe_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context);

fbe_status_t fbe_sas_enclosure_debug_prvt_data(const fbe_u8_t * module_name,
	                                       fbe_dbgext_ptr sas_enclosure_p,
                                           fbe_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context);
fbe_status_t fbe_sas_enclosure_debug_encl_port_data(const fbe_u8_t * module_name,
	                                       fbe_dbgext_ptr sas_enclosure_p,
                                           fbe_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context);

void fbe_sas_enclosure_print_prvt_data(void * enclPrvtData,
	                                       fbe_trace_func_t trace_func,
	                                       fbe_trace_context_t trace_context);


void fbe_sas_enclosure_print_sasEnclosureSMPAddress(fbe_sas_address_t address, 
                                                                                                    fbe_trace_func_t trace_func, 
                                                                                                    fbe_trace_context_t trace_context);

void fbe_sas_enclosure_print_smp_address_generation_code(fbe_generation_code_t code, 
                                                                                                    fbe_trace_func_t trace_func, 
                                                                                                    fbe_trace_context_t trace_context);


void fbe_sas_enclosure_print_sasEnclosureProductType(fbe_u8_t ProductType, 
                                                                                                    fbe_trace_func_t trace_func, 
                                                                                                    fbe_trace_context_t trace_context);

void fbe_sas_enclosure_print_ses_port_address(fbe_sas_address_t portAddress, 
                                                                                                    fbe_trace_func_t trace_func, 
                                                                                                    fbe_trace_context_t trace_context);

void fbe_sas_enclosure_print_ses_port_address_generation_code(fbe_generation_code_t address, 
                                                                                                    fbe_trace_func_t trace_func, 
                                                                                                    fbe_trace_context_t trace_context);

char * fbe_sas_enclosure_print_sasEnclosureType(fbe_u8_t EnclosureType, 
                                            fbe_trace_func_t trace_func, 
                                            fbe_trace_context_t trace_context);
 

#endif /* FBE_SAS_ENCLOSURE_DEBUG_H */

/*************************
 * end file fbe_sas_enclosure_debug.h
 *************************/
