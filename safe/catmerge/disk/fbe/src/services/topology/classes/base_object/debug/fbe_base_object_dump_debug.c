
/***************************************************************************
 * Copyright (C) EMC Corporation 2008-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_base_object_dump_debug.c
 ****************************************************************************
 *
 * @brief
 *  This file contains the debug functions for Base Object.
 *
 * @author
 *   08/15/2012: - Created. Jingcheng Zhang
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


extern fbe_status_t
fbe_base_object_trace_object_id(fbe_object_id_t object_id,
                                fbe_trace_func_t trace_func,
                                fbe_trace_context_t trace_context);
extern fbe_status_t
fbe_base_object_trace_class_id(fbe_class_id_t class_id, 
                               fbe_trace_func_t trace_func, 
                               fbe_trace_context_t trace_context);

extern char *
fbe_base_object_trace_get_state_string(fbe_lifecycle_state_t lifecycle_state);

extern fbe_status_t
fbe_base_object_trace_state(fbe_lifecycle_state_t lifecycle_state,
                            fbe_trace_func_t trace_func,
                            fbe_trace_context_t trace_context);
extern char *
fbe_base_object_trace_get_level_string(fbe_trace_level_t trace_level);

extern fbe_status_t
fbe_base_object_trace_level(fbe_trace_level_t trace_level,
                            fbe_trace_func_t trace_func,
                            fbe_trace_context_t trace_context);


/*************************
 *   FUNCTION DEFINITIONS
 *************************/
fbe_status_t fbe_lifecycle_inst_base_dump_debug_trace(const fbe_u8_t * module_name,
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
fbe_debug_field_info_t fbe_lifecycle_inst_base_dump_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("state", fbe_lifecycle_state_t, FBE_FALSE, "0x%x",
                                    fbe_lifecycle_state_dump_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!**************************************************************
 * fbe_trace_level_dump_debug_trace()
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
 *  8/20/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/

fbe_status_t fbe_trace_level_dump_debug_trace(const fbe_u8_t * module_name,
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
        trace_func(trace_context, "%s size is %d not %d\n",
                   field_info_p->name, field_info_p->size, (int)sizeof(fbe_trace_level_t));
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &trace_level, field_info_p->size);

    trace_func(trace_context, "%s: %d ", field_info_p->name, trace_level);
    fbe_base_object_trace_level(trace_level, trace_func, NULL);
    return status;
}



/*!*******************************************************************
 * @var fbe_lifecycle_inst_base_dump_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base object.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_lifecycle_inst_base_t,
                              fbe_lifecycle_inst_base_dump_struct_info,
                              &fbe_lifecycle_inst_base_dump_field_info[0]);

/*!*******************************************************************
 * @var fbe_base_object_dump_field_info
 *********************************************************************
 * @brief Information about each of the fields of base object.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_base_object_dump_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("object_id", fbe_object_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("class_id", fbe_class_id_t, FBE_FALSE, "0x%x",
                                    fbe_class_id_dump_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("trace_level", fbe_trace_level_t, FBE_FALSE, "0x%x",
                                    fbe_trace_level_dump_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("lifecycle", fbe_lifecycle_inst_base_t, FBE_FALSE, "0x%x",
                                    fbe_lifecycle_inst_base_dump_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_base_object_dump_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base object.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_base_object_t,
                              fbe_base_object_dump_struct_info,
                              &fbe_base_object_dump_field_info[0]);

/*!**************************************************************
 * fbe_lifecycle_inst_base_dump_debug_trace()
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
 *  8/15/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/

fbe_status_t fbe_lifecycle_inst_base_dump_debug_trace(const fbe_u8_t * module_name,
                                                 fbe_dbgext_ptr base_ptr,
                                                 fbe_trace_func_t trace_func,
                                                 fbe_trace_context_t trace_context,
                                                 fbe_debug_field_info_t *field_info_p,
                                                 fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    fbe_dump_debug_trace_prefix_t new_prefix;

    fbe_debug_dump_generate_prefix(&new_prefix, (fbe_dump_debug_trace_prefix_t *)trace_context,
                                   field_info_p->name);

    status = fbe_debug_display_structure(module_name, trace_func, &new_prefix, 
                                         base_ptr + field_info_p->offset,
                                         &fbe_lifecycle_inst_base_dump_struct_info,
                                         1 /* fields per line */,
                                         0);

    return status;
}
/**************************************
 * end fbe_lifecycle_inst_base_dump_debug_trace
 **************************************/
/*!**************************************************************
 * fbe_class_id_dump_debug_trace()
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
 *  8/15/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/

fbe_status_t fbe_class_id_dump_debug_trace(const fbe_u8_t * module_name,
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
        trace_func(trace_context, "%s size is %d not %d\n",
                   field_info_p->name, field_info_p->size, (int)sizeof(fbe_class_id_t));
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &class_id, field_info_p->size);
    trace_func(trace_context, "%s: 0x%x ", field_info_p->name, class_id);
    fbe_base_object_trace_class_id(class_id, trace_func, NULL);
    return status;
}
/**************************************
 * end fbe_class_id_dump_debug_trace
 **************************************/
/*!**************************************************************
 * fbe_lifecycle_state_dump_debug_trace()
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
 *  8/15/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/

fbe_status_t fbe_lifecycle_state_dump_debug_trace(const fbe_u8_t * module_name,
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
        trace_func(trace_context, "%s size is %d not %d\n",
                   field_info_p->name, field_info_p->size, (int)sizeof(fbe_lifecycle_state_t));
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &lifecycle_state, field_info_p->size);
    trace_func(trace_context, "%s: 0x%x ", field_info_p->name, lifecycle_state);
    fbe_base_object_trace_state(lifecycle_state, trace_func, NULL);
    return status;
}
/**************************************
 * end fbe_lifecycle_state_dump_debug_trace
 **************************************/
/*!**************************************************************


/*!**************************************************************
 * fbe_base_object_dump_debug_trace()
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
 *  8/15/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t fbe_base_object_dump_debug_trace(const fbe_u8_t * module_name,
                                         fbe_dbgext_ptr object_ptr,
                                         fbe_trace_func_t trace_func,
                                         fbe_trace_context_t trace_context,
                                         fbe_debug_field_info_t *field_info_p,
                                         fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    fbe_dump_debug_trace_prefix_t new_prefix;

    fbe_debug_dump_generate_prefix(&new_prefix, (fbe_dump_debug_trace_prefix_t *)trace_context,
                                   field_info_p->name);

    status = fbe_debug_display_structure(module_name, trace_func, &new_prefix, 
                                         object_ptr + field_info_p->offset,
                                         &fbe_base_object_dump_struct_info,
                                         1 /* fields per line */,
                                         0 /*spaces_to_indent*/);
    return status;
}
/******************************************
 * end fbe_base_object_dump_debug_trace()
 ******************************************/
/*!***************************************************************************
 * @fn fbe_base_object_dump_debug_trace_object_id(const fbe_u8_t * module_name,
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
 *  8/15/2012 - Created  Jingcheng Zhang 
 *
 ****************************************************************/
fbe_status_t
fbe_base_object_dump_debug_trace_object_id(const fbe_u8_t * module_name,
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
 * end fbe_base_object_dump_debug_trace_object_id()
 ******************************************/

/*!**********************************************************************
 * @fn fbe_base_object_dump_debug_trace_class_id(const fbe_u8_t * module_name,
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
 *  08/15/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t
fbe_base_object_dump_debug_trace_class_id(const fbe_u8_t * module_name,
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
 * end fbe_base_object_dump_debug_trace_class_id()
 ******************************************/

/*!**************************************************************
 * @fn fbe_base_object_dump_debug_trace_state(const fbe_u8_t * module_name,
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
 *  08/15/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t
fbe_base_object_dump_debug_trace_state(const fbe_u8_t * module_name,
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
 * end fbe_base_object_dump_debug_trace_state()
 ******************************************/

/*!**************************************************************
 * @fn fbe_base_object_dump_debug_trace_level(const fbe_u8_t * module_name,
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
 *  08/15/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t
fbe_base_object_dump_debug_trace_level (const fbe_u8_t * module_name,
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
 * end fbe_base_object_dump_debug_trace_level()
 ******************************************/

/*************************
 * end file fbe_base_object_dump_debug.c
 *************************/
