#ifndef FBE_SAS_VIKING_DRVSXP_ENCLOSURE_DEBUG_H
#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_sas_viking_drvsxp_enclosure_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the sas viking drvsxp enclosure debug library.
 *
 * @author
 *  04-Mar-2010:  Created. Dipak Patel
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"


/*************************
 *   FUNCTION DECLARATIONS
 *************************/
void fbe_sas_viking_drvsxp_enclosure_print_prvt_data(void * enclPrvtData,
                                          fbe_trace_func_t trace_func, 
                                          fbe_trace_context_t trace_context);

fbe_status_t fbe_sas_viking_drvsxp_enclosure_debug_prvt_data(const fbe_u8_t * module_name, 
                                           fbe_dbgext_ptr sas_viking_drvsxp_enclosure_p, 
                                           fbe_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context); 

#endif /* FBE_SAS_VOYAGER_EE_ENCLOSURE_DEBUG_H */



/********************************************
 * end file fbe_sas_viking_drvsxp_enclosure_debug.h
 ********************************************/