#ifndef FBE_SAS_STEELJAW_ENCLOSURE_DEBUG_H
#define FBE_SAS_STEELJAW_ENCLOSURE_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_sas_steeljaw_enclosure_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the sas steeljaw enclosure debug library.
 *
 * @author
 *   13-Nov-2012:  Created. Rui Chang
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

fbe_status_t fbe_sas_steeljaw_enclosure_debug_prvt_data(const fbe_u8_t * module_name, 
                                           fbe_dbgext_ptr sas_magnum_enclosure_p, 
                                           fbe_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context); 

#endif /* FBE_SAS_STEELJAW_ENCLOSURE_DEBUG_H */



/*************************
 * end file fbe_sas_steeljaw_enclosure_debug.h
 *************************/
