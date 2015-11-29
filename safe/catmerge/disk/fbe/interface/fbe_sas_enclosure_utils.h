#ifndef FBE_SAS_ENCLOSURE_UTILS_H
#define FBE_SAS_ENCLOSURE_UTILS_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_sas_enclosure_utils.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the various sas enclosure utils libraries.
 *
 * @author
 *   5-March-2013:  Created. Chris Gould
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_trace.h"


/*************************
 *   FUNCTION DECLARATIONS
 *************************/
void fbe_sas_boxwood_enclosure_print_prvt_data(void * enclPrvtData,
                                          fbe_trace_func_t trace_func, 
                                          fbe_trace_context_t trace_context);

void fbe_sas_bunker_enclosure_print_prvt_data(void * enclPrvtData,
                                          fbe_trace_func_t trace_func, 
                                          fbe_trace_context_t trace_context);

void fbe_sas_citadel_enclosure_print_prvt_data(void * enclPrvtData,
                                          fbe_trace_func_t trace_func, 
                                          fbe_trace_context_t trace_context);

void fbe_sas_derringer_enclosure_print_prvt_data(void * enclPrvtData,
                                          fbe_trace_func_t trace_func, 
                                          fbe_trace_context_t trace_context);

void fbe_sas_fallback_enclosure_print_prvt_data(void * enclPrvtData,
                                          fbe_trace_func_t trace_func, 
                                          fbe_trace_context_t trace_context);

void fbe_sas_knot_enclosure_print_prvt_data(void * enclPrvtData,
                                          fbe_trace_func_t trace_func, 
                                          fbe_trace_context_t trace_context);

void fbe_sas_magnum_enclosure_print_prvt_data(void * enclPrvtData,
                                          fbe_trace_func_t trace_func, 
                                          fbe_trace_context_t trace_context);

void fbe_sas_pinecone_enclosure_print_prvt_data(void * enclPrvtData,
                                          fbe_trace_func_t trace_func, 
                                          fbe_trace_context_t trace_context);

void fbe_sas_ramhorn_enclosure_print_prvt_data(void * enclPrvtData,
                                          fbe_trace_func_t trace_func, 
                                          fbe_trace_context_t trace_context);

void fbe_sas_steeljaw_enclosure_print_prvt_data(void * enclPrvtData,
                                          fbe_trace_func_t trace_func, 
                                          fbe_trace_context_t trace_context);

void fbe_sas_viper_enclosure_print_prvt_data(void * enclPrvtData,
                                          fbe_trace_func_t trace_func, 
                                          fbe_trace_context_t trace_context);

void fbe_sas_voyager_ee_enclosure_print_prvt_data(void * enclPrvtData,
                                          fbe_trace_func_t trace_func, 
                                          fbe_trace_context_t trace_context);

void fbe_sas_voyager_icm_enclosure_print_prvt_data(void * enclPrvtData,
                                          fbe_trace_func_t trace_func, 
                                          fbe_trace_context_t trace_context);

void fbe_sas_calypso_enclosure_print_prvt_data(void * enclPrvtData,
                                               fbe_trace_func_t trace_func, 
                                               fbe_trace_context_t trace_context);

void fbe_sas_rhea_enclosure_print_prvt_data(void * enclPrvtData,
                                            fbe_trace_func_t trace_func, 
                                            fbe_trace_context_t trace_context);

void fbe_sas_miranda_enclosure_print_prvt_data(void * enclPrvtData,
                                               fbe_trace_func_t trace_func, 
                                               fbe_trace_context_t trace_context);

void fbe_sas_tabasco_enclosure_print_prvt_data(void * enclPrvtData,
                                          fbe_trace_func_t trace_func, 
                                          fbe_trace_context_t trace_context);

void fbe_sas_ancho_enclosure_print_prvt_data(void * enclPrvtData,
                                          fbe_trace_func_t trace_func, 
                                          fbe_trace_context_t trace_context);

void fbe_sas_viking_drvsxp_enclosure_print_prvt_data(void * enclPrvtData,
                                        fbe_trace_func_t trace_func, 
                                        fbe_trace_context_t trace_context);

void fbe_sas_viking_iosxp_enclosure_print_prvt_data(void * pEnclPrvtData,
                                        fbe_trace_func_t trace_func, 
                                        fbe_trace_context_t trace_context);
void fbe_sas_naga_iosxp_enclosure_print_prvt_data(void * pEnclPrvtData,
                                        fbe_trace_func_t trace_func, 
                                        fbe_trace_context_t trace_context);

void fbe_sas_cayenne_drvsxp_enclosure_print_prvt_data(void * enclPrvtData,
                                        fbe_trace_func_t trace_func, 
                                        fbe_trace_context_t trace_context);

void fbe_sas_cayenne_iosxp_enclosure_print_prvt_data(void * pEnclPrvtData,
                                          fbe_trace_func_t trace_func, 
                                          fbe_trace_context_t trace_context);

#endif /* FBE_SAS_ENCLOSURE_UTILS_H */

/*************************
 * end file fbe_sas_enclosure_utils.h
 *************************/
