/**********************************************************************
 *  Copyright (C)  EMC Corporation 2008
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*************************************************************************
 * @file fbe_sas_naga_iosxp_enclosure_debug_ext.c
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
 *   26-Feb-2010: Dipak Patel - Created. 
 *
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_base_object_debug.h"
#include "fbe_eses_enclosure_debug.h"
#include "fbe_sas_naga_iosxp_enclosure_debug.h"


/*!**************************************************************
 * @fn fbe_sas_naga_iosxp_enclosure_debug_prvt_data(const fbe_u8_t * module_name, 
 *                                   fbe_dbgext_ptr sas_naga_enclosure_p,
 *                                   fbe_trace_func_t trace_func,
 *                                   fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display private data about the sas voyager_icm enclosure object.
 *
 * @param  pModuleName - This is the name of the module we are debugging.
 * @param pSasNagaEnclosure - Ptr to sas voyager_icm enclosure object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  26-Feb-2010: Created. Dipak Patel
 *
 ****************************************************************/
fbe_status_t fbe_sas_naga_iosxp_enclosure_debug_prvt_data(const fbe_u8_t * pModuleName, 
                                           fbe_dbgext_ptr pSasNagaEnclosure, 
                                           fbe_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context)

{
  fbe_u32_t eses_enclosure_offset = 0;  
  
  FBE_GET_FIELD_OFFSET(pModuleName, fbe_eses_enclosure_t, "eses_enclosure", &eses_enclosure_offset);
  fbe_eses_enclosure_debug_prvt_data(pModuleName, pSasNagaEnclosure + eses_enclosure_offset ,trace_func,trace_context);
  return FBE_STATUS_OK;
}

