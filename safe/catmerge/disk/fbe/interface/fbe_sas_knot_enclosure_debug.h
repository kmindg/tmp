#ifndef FBE_SAS_KNOT_ENCLOSURE_DEBUG_H
#define FBE_SAS_KNOT_ENCLOSURE_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_sas_knot_enclosure_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the sas knot enclosure debug library.
 *
 * @author
 *   7-Apr-2009:  Created. Dipak Patel
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


fbe_status_t fbe_sas_knot_enclosure_debug_prvt_data(const fbe_u8_t * module_name, 
                                           fbe_dbgext_ptr sas_magnum_enclosure_p, 
                                           fbe_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context);

#endif /* FBE_SAS_KNOT_ENCLOSURE_DEBUG_H */



/*************************
 * end file fbe_sas_knot_enclosure_debug.h
 *************************/
