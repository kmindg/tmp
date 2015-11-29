#ifndef FBE_SAS_CAYENNE_IOSXP_ENCLOSURE_DEBUG_H
#define FBE_SAS_CAYENNE_IOSXP_ENCLOSURE_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_sas_cayenne_iosxp_enclosure_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the sas cayenne_iosxp enclosure debug library.
 *
 * @author
 *  26-Feb-2010:  Created. Dipak Patel
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_trace.h"
#include "fbe/fbe_dbgext.h"

/*************************
 *   FUNCTION DECLARATIONS
 *************************/

fbe_status_t fbe_sas_cayenne_iosxp_enclosure_debug_prvt_data(const fbe_u8_t * module_name, 
                                           fbe_dbgext_ptr sas_cayenne_iosxp_enclosure_p, 
                                           fbe_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context); 

#endif /* FBE_SAS_CAYENNE_IOSXP_ENCLOSURE_DEBUG_H */



/********************************************
 * end file fbe_sas_cayenne_iosxp_enclosure_debug.h
 ********************************************/
