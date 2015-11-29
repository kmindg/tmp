/**********************************************************************
 *  Copyright (C)  EMC Corporation 2008
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*************************************************************************
 * @file fbe_sas_enclosure_debug_ext.c
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
 *   09-Dec-2008 bphilbin - Created. 
 *
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_base_object_debug.h"
#include "fbe_sas_enclosure_debug.h"
#include "fbe_base_enclosure_debug.h"
#include "sas_enclosure_private.h"
#include "edal_sas_enclosure_data.h"

/*!**************************************************************
 * @fn fbe_sas_enclosure_debug_basic_info(const fbe_u8_t * module_name, 
 *                                   fbe_dbgext_ptr sas_enclosure,
 *                                   fbe_trace_func_t trace_func,
 *                                   fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display all the trace information about the sas enclosure object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param sas_enclosure_p - Ptr to sas enclosure object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  12/9/2008 - Created. bphilbin
 *
 ****************************************************************/

fbe_status_t fbe_sas_enclosure_debug_basic_info(const fbe_u8_t * module_name, 
                                           fbe_dbgext_ptr sas_enclosure_p, 
                                           fbe_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context)
{
    fbe_status_t status;

    csx_dbg_ext_print("  Class id: ");
    fbe_base_object_debug_trace_class_id(module_name, sas_enclosure_p, trace_func, trace_context);
    
    csx_dbg_ext_print("  Lifecycle State: ");
    fbe_base_object_debug_trace_state(module_name, sas_enclosure_p, trace_func, trace_context);

    csx_dbg_ext_print("\n");

    /* 
     * Display SAS enclosure specific data.
     */
    status = fbe_sas_enclosure_debug_extended_info(module_name, sas_enclosure_p, trace_func, trace_context);


    return status;
}

/*!**************************************************************
 * @fn fbe_sas_enclosure_extended_trace(const fbe_u8_t * module_name, 
 *                                   fbe_dbgext_ptr sas_enclosure,
 *                                   fbe_trace_func_t trace_func,
 *                                   fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display all the detailed trace information about the sas enclosure object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param sas_enclosure_p - Ptr to sas enclosure object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  12/9/2008 - Created. bphilbin
 *
 ****************************************************************/
fbe_status_t fbe_sas_enclosure_debug_extended_info(const fbe_u8_t * module_name, 
                                           fbe_dbgext_ptr sas_enclosure_p, 
                                           fbe_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context)
{
    fbe_sas_address_t           sasEnclosureSMPAddress;
    FBE_GET_FIELD_DATA(module_name, 
                           sas_enclosure_p,
                           fbe_sas_enclosure_s,
                           sasEnclosureSMPAddress,
                           sizeof(fbe_sas_address_t),
                           &sasEnclosureSMPAddress);

    csx_dbg_ext_print("  SAS Enclosure:  SMP Address: 0x%llx", (unsigned long long)sasEnclosureSMPAddress);

    return FBE_STATUS_OK;

}
/*!**************************************************************
 * @fn fbe_sas_enclosure_debug_prvt_data(const fbe_u8_t * module_name, 
 *                                   fbe_dbgext_ptr sas_enclosure_p,
 *                                   fbe_trace_func_t trace_func,
 *                                   fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display private data about the sas enclosure object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param sas_enclosure_p - Ptr to sas enclosure object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  10-Apr-2009 - Created. Dipak Patel
 *
 ****************************************************************/
 fbe_status_t fbe_sas_enclosure_debug_prvt_data(const fbe_u8_t * module_name,
	                                       fbe_dbgext_ptr sas_enclosure_p,
                                           fbe_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context)
{
  fbe_u32_t                        base_enclosure_offset = 0;
  fbe_sas_address_t                sasEnclosureSMPAddress;
  fbe_generation_code_t            smp_address_generation_code;
  fbe_sas_address_t                ses_port_address;
  fbe_generation_code_t            ses_port_address_generation_code;
  fbe_sas_enclosure_product_type_t sasEnclosureProductType;

  trace_func(trace_context, "SAS Private Data:\n");

  FBE_GET_FIELD_DATA(module_name, 
                     sas_enclosure_p,
                     fbe_sas_enclosure_s,
                     sasEnclosureSMPAddress,
                     sizeof(fbe_sas_address_t),
                     &sasEnclosureSMPAddress);

  FBE_GET_FIELD_DATA(module_name, 
                     sas_enclosure_p,
                     fbe_sas_enclosure_s,
                     smp_address_generation_code,
                     sizeof(fbe_generation_code_t),
                     &smp_address_generation_code);

  FBE_GET_FIELD_DATA(module_name, 
                     sas_enclosure_p,
                     fbe_sas_enclosure_s,
                     ses_port_address,
                     sizeof(fbe_sas_address_t),
                     &ses_port_address);

  FBE_GET_FIELD_DATA(module_name, 
                     sas_enclosure_p,
                     fbe_sas_enclosure_s,
                     ses_port_address_generation_code,
                     sizeof(fbe_generation_code_t),
                     &ses_port_address_generation_code);

  FBE_GET_FIELD_DATA(module_name, 
                     sas_enclosure_p,
                     fbe_sas_enclosure_s,
                     sasEnclosureProductType,
                     sizeof(fbe_sas_enclosure_product_type_t),
                     &sasEnclosureProductType);


  fbe_sas_enclosure_print_sasEnclosureSMPAddress(sasEnclosureSMPAddress, trace_func, trace_context);
  fbe_sas_enclosure_print_smp_address_generation_code(smp_address_generation_code, trace_func, trace_context);
  fbe_sas_enclosure_print_ses_port_address(ses_port_address, trace_func, trace_context);
  fbe_sas_enclosure_print_ses_port_address_generation_code(ses_port_address_generation_code, trace_func, trace_context);
  fbe_sas_enclosure_print_sasEnclosureProductType(sasEnclosureProductType, trace_func, trace_context);
  
  FBE_GET_FIELD_OFFSET(module_name, fbe_base_enclosure_t, "base_enclosure", &base_enclosure_offset);
  fbe_base_enclosure_debug_prvt_data(module_name, sas_enclosure_p + base_enclosure_offset ,trace_func,trace_context);
  return FBE_STATUS_OK;

}

/*!**************************************************************
 * @fn fbe_sas_enclosure_debug_encl_port_data(const fbe_u8_t * module_name, 
 *                                   fbe_dbgext_ptr sas_enclosure_p,
 *                                   fbe_trace_func_t trace_func,
 *                                   fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display port data of sas enclosure object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param sas_enclosure_p - Ptr to sas enclosure object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  27-May-2009 - Created. Nayana Chaudhari
 *
 ****************************************************************/
fbe_status_t fbe_sas_enclosure_debug_encl_port_data(const fbe_u8_t * module_name,
	                                       fbe_dbgext_ptr sas_enclosure_p,
                                           fbe_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context)

{
    fbe_u32_t   base_enclosure_offset = 0;

    FBE_GET_FIELD_OFFSET(module_name, fbe_base_enclosure_t, "base_enclosure", &base_enclosure_offset);
    fbe_base_enclosure_debug_encl_port_data(module_name, sas_enclosure_p + base_enclosure_offset ,trace_func,trace_context);

    return FBE_STATUS_OK;
}
