/***************************************************************************
 * Copyright (C) EMC Corporation 2008-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_base_object_debug.c
 ****************************************************************************
 *
 * @brief
 *  This file contains the debug functions for Base Object.
 *
 * @author
 *   02/13/2009: - Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "base_object_private.h"
#include "fbe_base_object_trace.h"
#include "fbe_base_object_debug.h"

#include "../fbe_base_object_trace.c"
/*************************
 *   FUNCTION DEFINITIONS
 *************************/
fbe_status_t fbe_lifecycle_inst_base_debug_trace(const fbe_u8_t * module_name,
                                                 fbe_dbgext_ptr base_ptr,
                                                 fbe_trace_func_t trace_func,
                                                 fbe_trace_context_t trace_context,
                                                 fbe_debug_field_info_t *field_info_p,
                                                 fbe_u32_t spaces_to_indent);

/*!*******************************************************************
 * @var fbe_lifecycle_inst_base_field_info
 *********************************************************************
 * @brief Information about each of the fields of base object.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_lifecycle_inst_base_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("state", fbe_lifecycle_state_t, FBE_FALSE, "0x%x",
                                    fbe_lifecycle_state_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_lifecycle_inst_base_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base object.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_lifecycle_inst_base_t,
                              fbe_lifecycle_inst_base_struct_info,
                              &fbe_lifecycle_inst_base_field_info[0]);

/*!*******************************************************************
 * @var fbe_base_object_field_info
 *********************************************************************
 * @brief Information about each of the fields of base object.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_base_object_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("object_id", fbe_object_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("class_id", fbe_class_id_t, FBE_FALSE, "0x%x",
                                    fbe_class_id_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("trace_level", fbe_trace_level_t, FBE_FALSE, "0x%x",
                                    fbe_trace_level_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("lifecycle", fbe_lifecycle_inst_base_t, FBE_FALSE, "0x%x",
                                    fbe_lifecycle_inst_base_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_base_object_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base object.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_base_object_t,
                              fbe_base_object_struct_info,
                              &fbe_base_object_field_info[0]);

/*!**************************************************************
 * fbe_lifecycle_inst_base_debug_trace()
 ****************************************************************
 * @brief
 *  Display the class id.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param struct_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param struct_ptr - This is the ptr to the structure.
 * @param field_info_p - Field to init. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  9/7/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_lifecycle_inst_base_debug_trace(const fbe_u8_t * module_name,
                                                 fbe_dbgext_ptr base_ptr,
                                                 fbe_trace_func_t trace_func,
                                                 fbe_trace_context_t trace_context,
                                                 fbe_debug_field_info_t *field_info_p,
                                                 fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);

    trace_func(trace_context, "%s: ", field_info_p->name);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, 
                                         base_ptr + field_info_p->offset,
                                         &fbe_lifecycle_inst_base_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent);
    return status;
}
/**************************************
 * end fbe_lifecycle_inst_base_debug_trace
 **************************************/
/*!**************************************************************
 * fbe_class_id_debug_trace()
 ****************************************************************
 * @brief
 *  Display the class id.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param struct_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param struct_ptr - This is the ptr to the structure.
 * @param field_info_p - Field to init. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  9/7/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_class_id_debug_trace(const fbe_u8_t * module_name,
                                      fbe_dbgext_ptr base_ptr,
                                      fbe_trace_func_t trace_func,
                                      fbe_trace_context_t trace_context,
                                      fbe_debug_field_info_t *field_info_p,
                                      fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_class_id_t class_id;
    if (field_info_p->size != sizeof(fbe_class_id_t))
    {
        trace_func(trace_context, "%s size is %d not %llu\n",
                   field_info_p->name, field_info_p->size,
		   (unsigned long long)sizeof(fbe_class_id_t));
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &class_id, field_info_p->size);

    trace_func(trace_context, "%s: ", field_info_p->name);
    fbe_base_object_trace_class_id(class_id, trace_func, trace_context);
    return status;
}
/**************************************
 * end fbe_class_id_debug_trace
 **************************************/
/*!**************************************************************
 * fbe_lifecycle_state_debug_trace()
 ****************************************************************
 * @brief
 *  Display the lifecycle state.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param struct_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param struct_ptr - This is the ptr to the structure.
 * @param field_info_p - Field to init. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  9/7/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_lifecycle_state_debug_trace(const fbe_u8_t * module_name,
                                             fbe_dbgext_ptr base_ptr,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_debug_field_info_t *field_info_p,
                                             fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lifecycle_state_t lifecycle_state;
    if (field_info_p->size != sizeof(fbe_lifecycle_state_t))
    {
        trace_func(trace_context, "%s size is %d not %llu\n",
                   field_info_p->name, field_info_p->size,
		   (unsigned long long)sizeof(fbe_lifecycle_state_t));
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &lifecycle_state, field_info_p->size);

    trace_func(trace_context, "%s: ", field_info_p->name);
    fbe_base_object_trace_state(lifecycle_state, trace_func, trace_context);
    return status;
}
/**************************************
 * end fbe_lifecycle_state_debug_trace
 **************************************/
/*!**************************************************************
 * fbe_trace_level_debug_trace()
 ****************************************************************
 * @brief
 *  Display the trace level.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param struct_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param struct_ptr - This is the ptr to the structure.
 * @param field_info_p - Field to init. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  9/8/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_trace_level_debug_trace(const fbe_u8_t * module_name,
                                         fbe_dbgext_ptr base_ptr,
                                         fbe_trace_func_t trace_func,
                                         fbe_trace_context_t trace_context,
                                         fbe_debug_field_info_t *field_info_p,
                                         fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_trace_level_t trace_level;
    if (field_info_p->size != sizeof(fbe_trace_level_t))
    {
        trace_func(trace_context, "%s size is %d not %llu\n",
                   field_info_p->name, field_info_p->size,
		   (unsigned long long)sizeof(fbe_trace_level_t));
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &trace_level, field_info_p->size);

    trace_func(trace_context, "%s: ", field_info_p->name);
    fbe_base_object_trace_level(trace_level, trace_func, trace_context);
    return status;
}
/**************************************
 * end fbe_trace_level_debug_trace
 **************************************/
/*!**************************************************************
 * fbe_base_object_debug_trace()
 ****************************************************************
 * @brief
 *  Display info on base object.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t
 *
 * @author
 *  9/8/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_base_object_debug_trace(const fbe_u8_t * module_name,
                                         fbe_dbgext_ptr object_ptr,
                                         fbe_trace_func_t trace_func,
                                         fbe_trace_context_t trace_context,
                                         fbe_debug_field_info_t *field_info_p,
                                         fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    fbe_trace_indent(trace_func, trace_context, 8 /*spaces_to_indent*/);

    trace_func(trace_context, "%s: ", field_info_p->name);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, 
                                         object_ptr + field_info_p->offset,
                                         &fbe_base_object_struct_info,
                                         4 /* fields per line */,
                                         8 /*spaces_to_indent*/);
    return status;
}
/******************************************
 * end fbe_base_object_debug_trace()
 ******************************************/
/*!***************************************************************************
 * @fn fbe_base_object_debug_trace_object_id(const fbe_u8_t * module_name,
                                             fbe_dbgext_ptr base_object,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context)
 ******************************************************************************
 * @brief
 *  Display the information of the base object related to object id
 *
 * @param  base_object - base object ptr.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK 
 *
 * HISTORY:
 *  02/13/2009 - Created  Nishit Singh 
 *
 ****************************************************************/
fbe_status_t
fbe_base_object_debug_trace_object_id(const fbe_u8_t * module_name,
                                      fbe_dbgext_ptr base_object,
                                      fbe_trace_func_t trace_func,
                                      fbe_trace_context_t trace_context)
{
    fbe_object_id_t object_id;

    FBE_GET_FIELD_DATA(module_name,
                       base_object,
                       fbe_base_object_s,
                       object_id,
                       sizeof(fbe_object_id_t),
                       &object_id);

    fbe_base_object_trace_object_id(object_id, trace_func, trace_context);

    return FBE_STATUS_OK;
}

/******************************************
 * end fbe_base_object_debug_trace_object_id()
 ******************************************/

/*!**********************************************************************
 * @fn fbe_base_object_debug_trace_class_id(const fbe_u8_t * module_name,
                                            fbe_dbgext_ptr base_object,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context)
 ************************************************************************
 * @brief
 *  Display the Information of the base object related to class id
 *
 * @param  base_object - base object ptr.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  02/13/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t
fbe_base_object_debug_trace_class_id(const fbe_u8_t * module_name,
                                     fbe_dbgext_ptr base_object,
                                     fbe_trace_func_t trace_func,
                                     fbe_trace_context_t trace_context)
{
    fbe_class_id_t class_id;

    FBE_GET_FIELD_DATA(module_name, 
                       base_object,
                       fbe_base_object_s,
                       class_id,
                       sizeof(fbe_class_id_t),
                       &class_id);

    fbe_base_object_trace_class_id(class_id, trace_func, trace_context);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_base_object_debug_trace_class_id()
 ******************************************/

/*!**************************************************************
 * @fn fbe_base_object_debug_trace_state(const fbe_u8_t * module_name,
                                         fbe_dbgext_ptr base_object,
                                         fbe_trace_func_t trace_func,
                                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 * Display the information of the base object related to lifecycle state.
 *
 * @param base_object - base object ptr.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  02/13/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t __cdecl
fbe_base_object_debug_trace_state(const fbe_u8_t * module_name,
                                  fbe_dbgext_ptr base_object,
                                  fbe_trace_func_t trace_func,
                                  fbe_trace_context_t trace_context)
{
    fbe_lifecycle_state_t lifecycle_state;

    FBE_GET_FIELD_DATA(module_name,
                       base_object,
                       fbe_base_object_s,
                       lifecycle.state,
                       sizeof(fbe_lifecycle_state_t),
                       &lifecycle_state);

    fbe_base_object_trace_state(lifecycle_state, trace_func, trace_context);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_base_object_debug_trace_state()
 ******************************************/

/*!**************************************************************
 * @fn fbe_base_object_debug_trace_level(const fbe_u8_t * module_name,
                                         fbe_dbgext_ptr base_object,
                                         fbe_trace_func_t trace_func,
                                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display the Information of the base object related to Trace Level
 *
 * @param base_object - base object ptr.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  02/13/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t
fbe_base_object_debug_trace_level (const fbe_u8_t * module_name,
                                   fbe_dbgext_ptr base_object,
                                   fbe_trace_func_t trace_func,
                                   fbe_trace_context_t trace_context)
{
    fbe_trace_level_t trace_level;

    FBE_GET_FIELD_DATA(module_name, 
                       base_object,
                       fbe_base_object_t,
                       trace_level,
                       sizeof(fbe_trace_level_t),
                       &trace_level);
 
    fbe_base_object_trace_level(trace_level, trace_func, trace_context);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_base_object_debug_trace_level()
 ******************************************/

/*************************
 * end file fbe_base_object_debug.c
 *************************/