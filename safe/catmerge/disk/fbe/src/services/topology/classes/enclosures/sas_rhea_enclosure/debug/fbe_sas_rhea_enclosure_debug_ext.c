/**********************************************************************
 *  Copyright (C)  EMC Corporation 2014
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*************************************************************************
 * @file fbe_sas_rhea_enclosure_debug_ext.c
 ***************************************************************************
 *
 * @brief
 *  The routines in this file contains the enclosure object's debugger
 *  extension related functions.
 *
 *
 * NOTE: 
 *
 * HISTORY
 *   03/28/2014: Joe Perry - Created from fbe_sas_bunker_enclosure_debug_ext.c
 *
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_base_object_debug.h"
#include "fbe_eses_enclosure_debug.h"
#include "fbe_sas_rhea_enclosure_debug.h"


/*!**************************************************************
 * @fn fbe_sas_rhea_enclosure_debug_prvt_data(const fbe_u8_t * module_name, 
 *                                   fbe_dbgext_ptr sas_rhea_enclosure_p,
 *                                   fbe_trace_func_t trace_func,
 *                                   fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display private data about the sas rhea enclosure object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param sas_rhea_enclosure_p - Ptr to sas rhea enclosure object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  10-Apr-2009 - Created. Dipak Patel
 *
 ****************************************************************/
fbe_status_t fbe_sas_rhea_enclosure_debug_prvt_data(const fbe_u8_t * module_name, 
                                           fbe_dbgext_ptr sas_rhea_enclosure_p, 
                                           fbe_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context)

{
  fbe_u32_t eses_enclosure_offset = 0;  
  
  FBE_GET_FIELD_OFFSET(module_name, fbe_eses_enclosure_t, "eses_enclosure", &eses_enclosure_offset);
  fbe_eses_enclosure_debug_prvt_data(module_name, sas_rhea_enclosure_p + eses_enclosure_offset ,trace_func,trace_context);
  return FBE_STATUS_OK;
}

