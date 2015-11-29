/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!*************************************************************************
 * @file fbe_debug_ext.c
 ***************************************************************************
 *
 * @brief
 *  This file contains generic debug extension functions.
 *
 * @version
 *   3/25/2010:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_transport_debug.h"
#include "pp_ext.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!*******************************************************************
 * @def FBE_DEBUG_MAX_SYMBOL_NAME_LENGTH
 *********************************************************************
 * @brief This is the max length we use to size arrays that will hold
 *        a symbol name.
 *
 *********************************************************************/
#define FBE_DEBUG_MAX_SYMBOL_NAME_LENGTH 256

static fbe_u32_t fbe_debug_ptr_size = 0;

fbe_status_t fbe_debug_get_ptr_size(const fbe_u8_t * module_name,
                                    fbe_u32_t *ptr_size)
{
    if (fbe_debug_ptr_size == 0)
    {
        FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &fbe_debug_ptr_size);
    }
    *ptr_size = fbe_debug_ptr_size;
    return FBE_STATUS_OK;
}
fbe_bool_t fbe_debug_display_queue_header = FBE_TRUE;

void fbe_debug_set_display_queue_header(fbe_bool_t b_display)
{
    fbe_debug_display_queue_header = b_display;
}
/*!***************************************************************
 * fbe_debug_display_flag_string()
 ****************************************************************
 *
 * @brief
 *  This function displays a string for a given flag.
 *  The input string array has a string to represent each bit,
 *  and the input max_bit sets a limit on the number of bits/strings
 *  that strArray may represent.
 *
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param flag - flag to display
 * @param max_bit - max bit allowed in strArray
 * @param strArray - array of strings where each index represents a bit
 *                   strArray[i] represents the string for 1 << 1
 *
 * @return
 *  None
 *
 * @author
 *  3/15/2010 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_debug_display_flag_string(fbe_trace_func_t trace_func,
                                   fbe_trace_context_t trace_context, 
                                   fbe_u32_t flag, fbe_u32_t max_bit, const char **strArray )
{
    fbe_u32_t index;
    fbe_u32_t bit = 0;
    
    // Loop through the strings.
    // if we don't find the string, then 
    // we break out with the final string in the array set.
    for (index = 0; bit != max_bit; index++)
    {
        bit = 1 << index;
        if ( flag & bit )
        {
            trace_func(trace_context, " %s ", strArray[index]);
        }
    }
    return;
}
/********************
 * fbe_debug_display_flag_string()
 ********************/

/*!***************************************************************
 * fbe_debug_display_enum_string()
 ****************************************************************
 *
 * @brief
 *  This function displays a string for a given flag.
 *  The input string array has a string to represent each bit,
 *  and the input max_bit sets a limit on the number of bits/strings
 *  that strArray may represent.
 *
 * @param flag - flag to display
 * @param max_bit - max bit allowed in strArray
 * @param string_array - array of strings where each index represents a
 *                       different string.
 *
 * @return
 *  None
 *
 * @author
 *  3/15/2010 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_debug_display_enum_string(fbe_trace_func_t trace_func,
                                   fbe_trace_context_t trace_context, 
                                   fbe_u32_t enum_value, 
                                   fbe_u32_t max_enum, const char **string_array )
{
    if (enum_value < max_enum)
    {
        trace_func(trace_context, " %s ", string_array[enum_value]);
    }
    return;
}
/********************
 * fbe_debug_display_enum_string()
 ********************/
/*!**************************************************************
 * fbe_debug_initialize_queue_field_info()
 ****************************************************************
 * @brief
 *  Initialize a single field info structure.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param struct_info_p - structure this queue is inside of.
 * @param field_info_p - Field to init.            
 *
 * @return fbe_status_t   
 *
 * @author
 *  3/26/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_debug_initialize_queue_field_info(const fbe_u8_t * module_name,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_debug_struct_info_t *struct_info_p,
                                                   fbe_debug_field_info_t *field_info_p)
{
    fbe_u32_t object_size;
    if (strlen(field_info_p->name) == 0)
    {
        trace_func(trace_context, "%s failure initializing queue info no name\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (strlen(field_info_p->output_format_specifier) != 0)
    {
        trace_func(trace_context, "%s failure initializing %s queue output format specifier should be null\n", __FUNCTION__, field_info_p->name);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Make sure the queue element has non-zero size.
     */
    FBE_GET_TYPE_STRING_SIZE(module_name, field_info_p->type_name, &field_info_p->size);
    if (field_info_p->size == 0)
    {
        trace_func(trace_context, "%s failure initializing %s stucture size is 0\n", __FUNCTION__, field_info_p->name);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* make sure the queue's object type has a size.
     */
    FBE_GET_TYPE_STRING_SIZE(module_name, field_info_p->queue_info_p->type_name, &object_size);
    if (object_size == 0)
    {
        trace_func(trace_context, "%s failure initializing %s object size is 0\n", __FUNCTION__, field_info_p->queue_info_p->type_name);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* See what the offset is from the start of this object to the queue element.
     */
    FBE_GET_FIELD_STRING_OFFSET(module_name, field_info_p->queue_info_p->type_name, 
                                field_info_p->queue_info_p->queue_element_name, 
                                &field_info_p->queue_info_p->queue_element_offset);
    if ((field_info_p->queue_info_p->queue_element_offset != 0) && field_info_p->queue_info_p->b_first_field)
    {
        /* The first field should always have a zero offset.
         */
        trace_func(trace_context, "%s failure offset for field %s in struct %s is not zero (%d)\n", 
                   __FUNCTION__, field_info_p->queue_info_p->queue_element_name, 
                   field_info_p->queue_info_p->type_name, 
                   field_info_p->queue_info_p->queue_element_offset);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if ((field_info_p->queue_info_p->queue_element_offset == 0) && !field_info_p->queue_info_p->b_first_field)
    {
        /* The first field should always have a zero offset.
         */
        trace_func(trace_context, "%s failure offset for field %s in struct %s is zero \n", 
                   __FUNCTION__, field_info_p->queue_info_p->queue_element_name, 
                   field_info_p->queue_info_p->type_name);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_debug_initialize_queue_field_info()
 ******************************************/
/*!**************************************************************
 * fbe_debug_initialize_field_info()
 ****************************************************************
 * @brief
 *  Initialize a single field info structure.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Field to init.            
 *
 * @return fbe_status_t   
 *
 * @author
 *  3/15/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_debug_initialize_field_info(const fbe_u8_t * module_name,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_debug_field_info_t *field_info_p)
{
    if (strlen(field_info_p->name) == 0)
    {
        trace_func(trace_context, "%s failure initializing struct no name\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (strlen(field_info_p->output_format_specifier) == 0)
    {
        trace_func(trace_context, "%s failure initializing %s no output_format_specifier\n", __FUNCTION__, field_info_p->name);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    FBE_GET_TYPE_STRING_SIZE(module_name, field_info_p->type_name, &field_info_p->size);
    if (field_info_p->size == 0)
    {
        trace_func(trace_context, "%s failure initializing %s structure size is 0\n", __FUNCTION__, field_info_p->name);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_debug_initialize_field_info()
 ******************************************/
/*!**************************************************************
 * fbe_debug_initialize_struct_field_info()
 ****************************************************************
 * @brief
 *  Initialize all the field info structures passed in.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param struct_info_p - Struct info to init.            
 * @param field_info_p - Array of field info structures to init.        
 * @param num_fields - The number of fields to initialize.    
 *
 * @return fbe_status_t 
 *
 * @author
 *  3/15/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_debug_initialize_struct_field_info(const fbe_u8_t * module_name,
                                                    fbe_trace_func_t trace_func,
                                                    fbe_trace_context_t trace_context,
                                                    fbe_debug_struct_info_t *struct_info_p,
                                                    fbe_debug_field_info_t *field_info_p,
                                                    fbe_u32_t num_fields)
{
    fbe_u32_t index;
    fbe_status_t status;
    fbe_debug_field_info_t *const orig_field_info_p = field_info_p;
    
    /* Check for presence of a terminator.
     * Determine how many fields we found. 
     * Don't loop beyond an unreasonable number of fields. 
     */
    index = 0;
    while (index < 0xFFFF)
    {
        if ((field_info_p->name[0] != 0) ||
            (field_info_p->type_name[0] != 0) ||
            (field_info_p->b_first_field != 0) ||
            (field_info_p->display_function != 0) ||
            (field_info_p->output_format_specifier[0] != 0) ||
            (field_info_p->offset != 0))
        {
            /* Possibly a valid entry.
             */
        }
        else
        {
            /* Found the terminator.
             */
            break;
        }
        index++;
        field_info_p++;
    }
    struct_info_p->field_count = index;

    if (struct_info_p->field_count == 0)
    {
        trace_func(trace_context, "%s failure initializing %s no fields\n", __FUNCTION__, struct_info_p->name);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    field_info_p = orig_field_info_p;
    for ( index = 0; index < struct_info_p->field_count; index++)
    {
        if (field_info_p->queue_info_p)
        {
            /* Initialize info on a queue to display.
             */
            status = fbe_debug_initialize_queue_field_info(module_name, trace_func, trace_context,
                                                           struct_info_p, field_info_p);
        }
        else if (!strncmp("newline", field_info_p->name, 7) &&
                 !strncmp("newline", field_info_p->type_name, 7))
        {
            /* This is a spacer so it is automatically valid.
             */

            field_info_p++;
            continue;
        }
        else
        {
            /* Just a normal field to initialize.
             */
            status = fbe_debug_initialize_field_info(module_name, trace_func, trace_context,
                                                     field_info_p);
        }
        if (status != FBE_STATUS_OK) { return status; }
        FBE_GET_FIELD_STRING_OFFSET(module_name, struct_info_p->name, 
                                    field_info_p->name, &field_info_p->offset);
        if ((field_info_p->offset != 0) && field_info_p->b_first_field)
        {
            /* The first field should always have a zero offset.
             */
            trace_func(trace_context, "%s failure offset for field %s in struct %s is %d not zero\n", 
                       __FUNCTION__, field_info_p->name, struct_info_p->name, field_info_p->offset);
            return FBE_STATUS_GENERIC_FAILURE;
        }
#if 0 /* why do we need it ? */
        if ((field_info_p->offset == 0) && !field_info_p->b_first_field)
        {
            /* The first field should always have a zero offset.
             */
            trace_func(trace_context, "%s failure offset for field %s in struct %s is zero \n", 
                       __FUNCTION__, field_info_p->name, struct_info_p->name);
            return FBE_STATUS_GENERIC_FAILURE;
        }
#endif

        field_info_p++;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_debug_initialize_struct_field_info()
 ******************************************/

/*!**************************************************************
 * fbe_debug_initialize_struct_info()
 ****************************************************************
 * @brief
 *  Initialize a single struct info structure.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Field to init.            
 *
 * @return fbe_status_t   
 *
 * @author
 *  3/15/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_debug_initialize_struct_info(const fbe_u8_t * module_name,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_debug_struct_info_t *struct_info_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    if (struct_info_p->b_initialized == FBE_FALSE)
    {
        /* Sanity check the structure information.
         */
        if (strlen(struct_info_p->name) == 0)
        {
            trace_func(trace_context, "%s failure initializing struct no name\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (struct_info_p->field_info_array == NULL)
        {
            trace_func(trace_context, "%s failure initializing %s no field_info_array\n", __FUNCTION__, struct_info_p->name);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        FBE_GET_TYPE_STRING_SIZE(module_name, struct_info_p->name, &struct_info_p->size);
        if (struct_info_p->size == 0)
        {
            trace_func(trace_context, "%s failure initializing %s stucture size is 0\n", __FUNCTION__, struct_info_p->name);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        /* Init the info on each field in this structure.
         */
        status = fbe_debug_initialize_struct_field_info(module_name, trace_func, trace_context, struct_info_p, 
                                               &struct_info_p->field_info_array[0], struct_info_p->field_count);
        if (status != FBE_STATUS_OK) { return status; }

        struct_info_p->b_initialized = FBE_TRUE;
    }
    return status;
}
/******************************************
 * end fbe_debug_initialize_struct_info()
 ******************************************/

/*!**************************************************************
 * fbe_debug_display_field()
 ****************************************************************
 * @brief
 *  Display a single field info structure.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param base_ptr - This is the ptr to the structure this field
 *                   is located in.
 * @param field_info_p - Field to init.
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 *
 * @return fbe_status_t   
 *
 * @author
 *  3/15/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_debug_display_field(const fbe_u8_t * module_name,
                                     fbe_trace_func_t trace_func,
                                     fbe_trace_context_t trace_context,
                                     fbe_dbgext_ptr base_ptr,
                                     fbe_debug_field_info_t *field_info_p,
                                     fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    fbe_u8_t format_string[50] = "%s: ";

   /* If size is 8 bytes and format specifier is '0x%x' then change the format specifier to '0x%llx'
       so that it will display complete 64 bit value */
    if (field_info_p->size == 8 && strncmp(field_info_p->output_format_specifier, "0x%x", 4) == 0 ) 
    {
        strncat(format_string, "0x%llx",  7);
    }
    else
    {
        strncat(format_string, field_info_p->output_format_specifier, strlen(field_info_p->output_format_specifier));
    }

    if (field_info_p->queue_info_p)
    {
        /* Do not display payload if active queue display is disabled 
         */
        if(get_active_queue_display_flag() != FBE_FALSE)
        {
            /* Display this queue.
             */
            status = fbe_debug_display_queue(module_name, (base_ptr + field_info_p->offset), 
                                             field_info_p,
                                             trace_func, trace_context, spaces_to_indent + 2);
            return status;
        } 
        else 
        {
            trace_func(trace_context, "Active queue display is disabled, don't display payload.\n");
            return FBE_STATUS_OK;
        }
    }
    else if (field_info_p->display_function)
    {
        status = (field_info_p->display_function)(module_name, base_ptr, trace_func, trace_context, field_info_p,
                                                  spaces_to_indent);
        return status;
    }
    else if ((strncmp(field_info_p->output_format_specifier, "%d", 2) == 0) ||
             (strncmp(field_info_p->output_format_specifier, "0x%x", 4) == 0) ||
             (strncmp(field_info_p->output_format_specifier, "0x%p", 8) == 0)) 
    {
        if (field_info_p->size == 1)
        {
            fbe_u8_t data;
            FBE_READ_MEMORY(base_ptr+field_info_p->offset, &data, field_info_p->size);
            trace_func(trace_context, format_string, field_info_p->name, data);
        }
        else if (field_info_p->size == 2)
        {
            fbe_u16_t data;
            FBE_READ_MEMORY(base_ptr+field_info_p->offset, &data, field_info_p->size);
            trace_func(trace_context, format_string, field_info_p->name, data);
        }
        else if (field_info_p->size == 4)
        {
            fbe_u32_t data;
            FBE_READ_MEMORY(base_ptr+field_info_p->offset, &data, field_info_p->size);
            trace_func(trace_context, format_string, field_info_p->name, data);
        }
        else if (field_info_p->size == 8)
        {
            fbe_u64_t data = 0;
            FBE_READ_MEMORY(base_ptr+field_info_p->offset, &data, field_info_p->size);
            trace_func(trace_context, format_string, field_info_p->name, data);
        }
        else
        {
            trace_func(trace_context, "%s failure field %s invalid size %d for format specifier %s \n", 
                       __FUNCTION__, field_info_p->name, field_info_p->size, field_info_p->output_format_specifier);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else
    {
        trace_func(trace_context, "%s failure field %s invalid format specifier %s \n", 
                   __FUNCTION__, field_info_p->name, field_info_p->output_format_specifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_debug_display_field()
 **************************************/

/*!**************************************************************
 * fbe_debug_display_structure()
 ****************************************************************
 * @brief
 *  Display a structure described by a fbe_debug_struct_info_t.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param struct_ptr - This is the ptr to the structure.
 * @param field_info_p - Field to init. 
 * 
 * @return fbe_status_t   
 *
 * @author
 *  3/15/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_debug_display_structure(const fbe_u8_t * module_name,
                                         fbe_trace_func_t trace_func,
                                         fbe_trace_context_t trace_context, 
                                         fbe_dbgext_ptr struct_ptr,
                                         fbe_debug_struct_info_t *struct_info_p,
                                         fbe_u32_t num_per_line,
                                         fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t index;
    fbe_u32_t lines_since_last_newline = 0;
    
    status = fbe_debug_initialize_struct_info(module_name, trace_func, trace_context, struct_info_p);
    if (status != FBE_STATUS_OK) { return status; }

    for ( index = 0; index < struct_info_p->field_count; index++)
    {
        /* If we see a spacer, just add a newline.
         */
        if (!strncmp("newline", struct_info_p->field_info_array[index].name, 7) &&
            !strncmp("newline", struct_info_p->field_info_array[index].type_name, 7))
        {
            trace_func(trace_context, "\n");
            fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
            lines_since_last_newline = 0;
            continue;
        }
        status = fbe_debug_display_field(module_name, trace_func, trace_context, struct_ptr,
                                         &struct_info_p->field_info_array[index], spaces_to_indent);
        if (status != FBE_STATUS_OK) 
        {
            trace_func(trace_context, "%s failure displaying field %s for struct %s \n", 
                   __FUNCTION__, struct_info_p->field_info_array[index].name, struct_info_p->name);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (struct_info_p->field_info_array[index].queue_info_p)
        {
            if ((index + 1) == struct_info_p->field_count)
            {
                /* Current item is a queue. 
                 * Already inserted new line, but we are at the end, do not insert 
                 * anything. 
                 */
            }
            else
            {
                /* Current item is a queue.  Already inserted new line.
                 */
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
            }
        }
        else if (struct_info_p->field_info_array[index + 1].queue_info_p)
        {
            /* The current or next item is a queue, just display a new line.
             */
            trace_func(trace_context, "\n");
        }
        else if ((((lines_since_last_newline + 1) % num_per_line) == 0) &&
                 ((index + 1) != struct_info_p->field_count))
        {
            /* After num_per_line, add a newline and an indent.
             */
            trace_func(trace_context, "\n");
            fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        }
        else
        {
            /* Put a space in between each field.
             */
            trace_func(trace_context, " ");
        }
        lines_since_last_newline++;
    }
    return status;
}
/******************************************
 * end fbe_debug_display_structure()
 ******************************************/
/*!**************************************************************
 * fbe_debug_get_structure_field_index()
 ****************************************************************
 * @brief
 *  Returns the index of a given field in the field_info_array
 *  of a fbe_debug_struct_info_t
 *
 * @param struct_info_p - contains the field_info_array to search.
 * @param field_name_p - Name to search for.               
 *
 * @return fbe_u32_t - the index of the field in the field_info_array
 *                     that contains the input name.
 *                     If the field is not found we return FBE_U32_MAX.
 *
 * @author
 *  3/15/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_u32_t fbe_debug_get_structure_field_index(fbe_debug_struct_info_t *struct_info_p,
                                                 const char *field_name_p)
{
    fbe_u32_t index;
    /* First get the index of the first field
     */
    for ( index = 0; index < struct_info_p->field_count; index++)
    {
        if (strcmp(struct_info_p->field_info_array[index].name, field_name_p) == 0)
        {
            return index;
        }
    }
    return FBE_U32_MAX;
}
/******************************************
 * end fbe_debug_get_structure_field_index()
 ******************************************/
/*!**************************************************************
 * fbe_debug_display_structure_fields()
 ****************************************************************
 * @brief Display all the fields for a structure in a given range.
 *        We start displaying with the given input "start_field_name_p"
 *        We stop displaying with the
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param struct_info_ptr - Info that describes structure being displayed.
 * @param num_per_line - Number of fields to display per line.
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 *
 * @return fbe_status_t  
 *
 * @author
 *  3/15/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_debug_display_structure_fields(const fbe_u8_t * module_name,
                                                fbe_trace_func_t trace_func,
                                                fbe_trace_context_t trace_context, 
                                                fbe_dbgext_ptr base_ptr,
                                                fbe_debug_struct_info_t *struct_info_p,
                                                fbe_u32_t num_per_line,
                                                const char *start_field_name_p,
                                                const char *end_field_name_p,
                                                fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    fbe_u32_t index;
    fbe_u32_t start_index;
    fbe_u32_t end_index;
    
    status = fbe_debug_initialize_struct_info(module_name, trace_func, trace_context, struct_info_p);
    if (status != FBE_STATUS_OK) { return status; }

    start_index = fbe_debug_get_structure_field_index(struct_info_p, start_field_name_p);
    end_index = fbe_debug_get_structure_field_index(struct_info_p, end_field_name_p);

    /* If start or end index are out of range, then just display an error and display the 
     * entire structure. 
     */
    if ((start_index >= struct_info_p->field_count) || (end_index >= struct_info_p->field_count))
    {
        trace_func(trace_context, "start (%d) and/or end index (%d) out of range for struct %s with %d fields\n",
                   start_index, end_index, struct_info_p->name, struct_info_p->field_count);
        start_index = 0;
        end_index = struct_info_p->field_count;
    }
    for (index = start_index; index <= end_index; index++)
    {
        status = fbe_debug_display_field(module_name, trace_func, trace_context, base_ptr,
                                         &struct_info_p->field_info_array[index], spaces_to_indent);
        if (status != FBE_STATUS_OK) 
        {
            trace_func(trace_context, "%s failure displaying field %s for struct %s \n", 
                   __FUNCTION__, struct_info_p->field_info_array[index].name, struct_info_p->name);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        trace_func(trace_context, "\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    }

    /* After num_per_line, add a newline and an indent.
     * We do not print the new line on the last iteration. 
     */
    if ((((index + 1) % num_per_line) == 0) &&
        ((index + 1) != struct_info_p->field_count))
    {
        trace_func(trace_context, "\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    }
    else
    {
        /* Put a space in between each field.
         */
        trace_func(trace_context, " ");
    }
    return status;
}
/******************************************
 * end fbe_debug_display_structure_fields()
 ******************************************/

/*!**************************************************************
 * fbe_debug_get_queue_size()
 ****************************************************************
 * @brief
 *  Compute the size of a queue.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param queue_p - pointer to queue.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 * @param queue_size - The size of the element in the queue to be returned.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  02/13/2009 - Created. Nishit Singh
 *
 ****************************************************************/
fbe_status_t fbe_debug_get_queue_size(const fbe_u8_t * module_name,
                                      fbe_u64_t queue_p,
                                      fbe_trace_func_t trace_func,
                                      fbe_trace_context_t trace_context,
                                      fbe_u32_t *queue_size)
{
    fbe_u32_t item_count = 0;
    fbe_u64_t curr_queue_element_p, next_queue_element_p;

    FBE_GET_FIELD_DATA(module_name,
                       queue_p,
                       fbe_queue_head_t,
                       next,
                       sizeof(fbe_u64_t),
                       &next_queue_element_p);

    /* Count the no of queue element. 
     */
    while ((next_queue_element_p != queue_p) && (next_queue_element_p != 0))
    {
        if (FBE_CHECK_CONTROL_C())
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }

       curr_queue_element_p = next_queue_element_p;
       FBE_GET_FIELD_DATA(module_name,
                          curr_queue_element_p,
                          fbe_queue_element_t,
                          next,
                          sizeof(fbe_u64_t),
                          &next_queue_element_p);
       item_count += 1;
       if (item_count > 0xffff)
       {
           trace_func(trace_context, "%s queue: %llx is too long\n",
                      __FUNCTION__, (unsigned long long)queue_p);
           break;
       }
    } /* end while (next_queue_element_p != queue_p) */

    *queue_size = item_count;
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_debug_get_queue_size()
 *****************************************/
/*!**************************************************************
 * fbe_debug_display_queue()
 ****************************************************************
 * @brief
 *  Display a queue of objects.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param raid_group_p - ptr to raid_group to display.
 * @param display_fn - Function pointer to display each object
 *                     in the queue.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent the display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  3/25/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_debug_display_queue(const fbe_u8_t * module_name,
                                     fbe_dbgext_ptr input_queue_head_p,
                                     fbe_debug_field_info_t *field_info_p,
                                     fbe_trace_func_t trace_func,
                                     fbe_trace_context_t trace_context,
                                     fbe_u32_t spaces_to_indent)
{
    fbe_u64_t queue_head_p = 0;
    fbe_u64_t queue_element_p = 0;
    fbe_u64_t next_queue_element_p = 0;
    fbe_u32_t ptr_size;
    fbe_u32_t queue_element_offset = field_info_p->queue_info_p->queue_element_offset;
    fbe_debug_display_object_fn_t display_fn = field_info_p->queue_info_p->display_function;
    fbe_u32_t retcode = 0;

    fbe_debug_get_ptr_size(module_name, &ptr_size);

    FBE_READ_MEMORY(input_queue_head_p, &queue_head_p, ptr_size);
    queue_element_p = queue_head_p;

    if (fbe_debug_display_queue_header == FBE_TRUE)
    {
        fbe_u32_t item_count = 0;
        fbe_debug_get_queue_size(module_name,
                                     input_queue_head_p,
                                     trace_func,
                                     trace_context,
                                     &item_count);
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        if (item_count == 0)
        {
            trace_func(trace_context, "queue (%s): EMPTY\n", field_info_p->name);
        }
        else
        {
            trace_func(trace_context, "queue: (%s) (%d items)\n", field_info_p->name,
                       item_count);
        }
    }

    /* Loop over all entries on the terminator queue, displaying each packet and iots. 
     * We stop when we reach the end (head) or NULL. 
     */
    while ((queue_element_p != input_queue_head_p) && (queue_element_p != NULL))
    {
        fbe_u64_t object_p = queue_element_p - queue_element_offset;

        if (FBE_CHECK_CONTROL_C())
        {
            return FBE_STATUS_CANCELED;
        }

        /* Display object.
         */
        retcode = display_fn(module_name, object_p, trace_func, trace_context, spaces_to_indent + 2);
        if( retcode != FBE_STATUS_OK)
        {
            return FBE_STATUS_OK;
        }
        FBE_READ_MEMORY(queue_element_p, &next_queue_element_p, ptr_size);
        queue_element_p = next_queue_element_p;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_debug_display_queue()
 ******************************************/
/*!**************************************************************
 * fbe_debug_display_pointer()
 ****************************************************************
 * @brief
 *  Display a pointer value.  This reads in the pointer located
 *  at this field's offset.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param struct_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  3/15/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_debug_display_pointer(const fbe_u8_t * module_name,
                                       fbe_dbgext_ptr base_ptr,
                                       fbe_trace_func_t trace_func,
                                       fbe_trace_context_t trace_context,
                                       fbe_debug_field_info_t *field_info_p,
                                       fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u64_t data = 0;
    fbe_u32_t ptr_size;

    status = fbe_debug_get_ptr_size(module_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        trace_func(trace_context, "unable to get ptr size status: %d\n", status);
        return status; 
    }
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &data, ptr_size);
    trace_func(trace_context, "%s: 0x%llx", field_info_p->name,
	       (unsigned long long)data);
    return status;
}
/**************************************
 * end fbe_debug_display_pointer
 **************************************/

/*!**************************************************************
 * fbe_debug_display_field_pointer()
 ****************************************************************
 * @brief
 *  Display the pointer to this field.  Which is just the
 *  base ptr plus the offset to this point in the structure.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param base_ptr - This is the ptr to the structure this field.
 *                   is located inside of.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function.
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  3/15/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_debug_display_field_pointer(const fbe_u8_t * module_name,
                                             fbe_dbgext_ptr base_ptr,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_debug_field_info_t *field_info_p,
                                             fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t ptr_size;

    status = fbe_debug_get_ptr_size(module_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        trace_func(trace_context, "unable to get ptr size status: %d\n", status);
        return status; 
    }
    trace_func(trace_context, "%s: 0x%llx", 
               field_info_p->name,
	       (unsigned long long)(base_ptr + field_info_p->offset));
    return status;
}
/**************************************
 * end fbe_debug_display_field_pointer
 **************************************/
/*!**************************************************************
 * fbe_debug_display_function_ptr_symbol()
 ****************************************************************
 * @brief
 *  Display the pointer to this field.  Which is just the
 *  base ptr plus the offset to this point in the structure.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param fn_ptr - function pointer to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function.
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  4/15/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_debug_display_function_ptr_symbol(const fbe_u8_t * module_name,
                                                   fbe_dbgext_ptr fn_ptr,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    csx_string_t symbol_string;

    symbol_string = csx_dbg_ext_lookup_symbol_name(fn_ptr);

    trace_func(trace_context, " 0x%llx %s", (unsigned long long)fn_ptr, symbol_string);

    csx_dbg_ext_free_symbol_name(symbol_string);

    return status;
}
/**************************************
 * end fbe_debug_display_function_ptr_symbol
 **************************************/
/*!**************************************************************
 * fbe_debug_display_function_ptr()
 ****************************************************************
 * @brief
 *  Display the pointer to this field.  Which is just the
 *  base ptr plus the offset to this point in the structure.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param struct_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function.
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  3/15/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_debug_display_function_ptr(const fbe_u8_t * module_name,
                                            fbe_dbgext_ptr base_ptr,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context,
                                            fbe_debug_field_info_t *field_info_p,
                                            fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t ptr_size;
    csx_string_t symbol_string;
    fbe_u64_t state = 0;

    status = fbe_debug_get_ptr_size(module_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        trace_func(trace_context, "unable to get ptr size status: %d\n", status);
        return status; 
    }
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &state, ptr_size);

    symbol_string = csx_dbg_ext_lookup_symbol_name(state);

    trace_func(trace_context, "%s: 0x%llx %s", 
               field_info_p->name, (unsigned long long)state, symbol_string);

    csx_dbg_ext_free_symbol_name(symbol_string);

    return status;
}
/**************************************
 * end fbe_debug_display_function_ptr
 **************************************/
/*!**************************************************************
 * fbe_debug_display_function_symbol()
 ****************************************************************
 * @brief
 *  Display the symbol for a given function ptr.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param struct_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function.
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  4/8/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_debug_display_function_symbol(const fbe_u8_t * module_name,
                                               fbe_dbgext_ptr base_ptr,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_debug_field_info_t *field_info_p,
                                               fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t ptr_size;
    csx_string_t symbol_string;
    fbe_u64_t state = 0;

    status = fbe_debug_get_ptr_size(module_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        trace_func(trace_context, "unable to get ptr size status: %d\n", status);
        return status; 
    }
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &state, ptr_size);

    symbol_string = csx_dbg_ext_lookup_symbol_name(state);

    trace_func(trace_context, "%s: %s", 
               field_info_p->name, symbol_string);

    csx_dbg_ext_free_symbol_name(symbol_string);

    return status;
}
/**************************************
 * end fbe_debug_display_function_symbol
 **************************************/

/*!**************************************************************
 * fbe_debug_display_time()
 ****************************************************************
 * @brief
 *  Display the pointer to this field.  Which is just the
 *  base ptr plus the offset to this point in the structure.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param base_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function.
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  3/15/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_debug_display_time(const fbe_u8_t * module_name,
                                    fbe_dbgext_ptr base_ptr,
                                    fbe_trace_func_t trace_func,
                                    fbe_trace_context_t trace_context,
                                    fbe_debug_field_info_t *field_info_p,
                                    fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_time_t time;
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &time, field_info_p->size);
    
    trace_func(trace_context, "%s: 0x%llx", field_info_p->name,
	       (unsigned long long)time);
    return status;
}
/**************************************
 * end fbe_debug_display_time
 **************************************/
/*!**************************************************************
 * fbe_debug_trace_time()
 ****************************************************************
 * @brief
 *  Display the fbe_time_t.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param struct_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  3/16/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_debug_trace_time(const fbe_u8_t * module_name,
                                  fbe_dbgext_ptr base_ptr,
                                  fbe_trace_func_t trace_func,
                                  fbe_trace_context_t trace_context,
                                  fbe_debug_field_info_t *field_info_p,
                                  fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_time_t time_stamp = 0;
    fbe_time_t seconds;
    fbe_u64_t time_ptr = 0;
    fbe_time_t current_time;

    if (!strncmp(module_name, "NewNeitPackage", 14) ||
        !strncmp(module_name, "new_neit", 14))
    {
        module_name = "sep";
    }
    FBE_GET_EXPRESSION(module_name, idle_thread_time, &time_ptr);
    FBE_READ_MEMORY(time_ptr, &current_time, sizeof(fbe_time_t));
    
    /* display the seconds.
     */
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &time_stamp, sizeof(fbe_time_t));

    if (current_time < time_stamp)
    {
        seconds = 0;
    }
    else
    {
        seconds = current_time - time_stamp;
    }

    trace_func(trace_context, "cur_time: 0x%llx %s: 0x%llx delta ms: %d", 
               current_time, field_info_p->name, time_stamp, (int)seconds);

    return status;
}
/**************************************
 * end fbe_debug_trace_time
 **************************************/
/*!**************************************************************
 * fbe_debug_trace_time_brief()
 ****************************************************************
 * @brief
 *  Display the fbe_time_t.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param struct_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  3/26/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_debug_trace_time_brief(const fbe_u8_t * module_name,
                                        fbe_dbgext_ptr base_ptr,
                                        fbe_trace_func_t trace_func,
                                        fbe_trace_context_t trace_context,
                                        fbe_debug_field_info_t *field_info_p,
                                        fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_time_t time_stamp = 0;
    fbe_time_t seconds;
    fbe_u64_t time_ptr = 0;
    fbe_time_t current_time;

    if (!strncmp(module_name, "NewNeitPackage", 14) ||
        !strncmp(module_name, "new_neit", 14))
    {
        module_name = "sep";
    }
    FBE_GET_EXPRESSION(module_name, idle_thread_time, &time_ptr);
    FBE_READ_MEMORY(time_ptr, &current_time, sizeof(fbe_time_t));
    
    /* display the seconds.
     */
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &time_stamp, sizeof(fbe_time_t));

    if (current_time < time_stamp)
    {
        seconds = 0;
    }
    else
    {
        seconds = (current_time - time_stamp) / (fbe_time_t)1000;
    }

    trace_func(trace_context, "seconds: %d", (int)seconds);

    return status;
}
/**************************************
 * end fbe_debug_trace_time_brief
 **************************************/
/*!**************************************************************
 * fbe_debug_trace_flags_strings()
 ****************************************************************
 * @brief
 *  Display a stripe lock slot flag.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param base_ptr - This is the ptr to the structure this field.
 *                   is located inside of when we recieve a field info ptr.
 *                   Otherwise this is the ptr to the flags field itself.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function.
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 *
 * @return - FBE_STATUS_OK  
 *
 * @author
 *  9/15/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_debug_trace_flags_strings(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr base_ptr,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context,
                                           fbe_debug_field_info_t *field_info_p,
                                           fbe_u32_t spaces_to_indent,
                                           fbe_debug_enum_trace_info_t *flags_info_p)
{
    fbe_u64_t flags = 0;

    if (field_info_p != NULL)
    {
        if (field_info_p->size > sizeof(flags))
        {
            trace_func(trace_context, "ERROR: %s field_info_p->size %d > sizeof(flags) %llu\n",
                       __FUNCTION__, field_info_p->size,
		       (unsigned long long)sizeof(flags));
            return FBE_STATUS_GENERIC_FAILURE;
        }
        FBE_READ_MEMORY(base_ptr + field_info_p->offset, &flags, field_info_p->size);
    }
    else
    {
        FBE_READ_MEMORY(base_ptr, &flags, sizeof(fbe_u32_t));
    }

    if (flags == 0)
    {
        return FBE_STATUS_OK;
    }
    /* Loop over the list of flags info. 
     * Keep going until we hit the terminator or 
     * until there are no more flags set to trace. 
     */
    while ((flags != 0) && (flags_info_p->flag != 0))
    {
        if (flags & flags_info_p->flag)
        {
            trace_func(trace_context, "%s ", flags_info_p->string_p);
        }
        flags &= ~flags_info_p->flag;
        flags_info_p++;
    }

    if (flags != 0)
    {
        trace_func(trace_context, "UNKNOWN flags 0x%llx",
		   (unsigned long long)flags);
    }

    return FBE_STATUS_OK;
}
/********************************************
 * end fbe_debug_trace_flags_strings()
 ********************************************/
/*!**************************************************************
 * fbe_debug_trace_enum_strings()
 ****************************************************************
 * @brief
 *  Display a string for an enum.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param base_ptr - This is the ptr to the structure this field.
 *                   is located inside of when we recieve a field info ptr.
 *                   Otherwise this is the ptr to the flags field itself.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function.
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 *
 * @return - FBE_STATUS_OK  
 *
 * @author
 *  9/15/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_debug_trace_enum_strings(const fbe_u8_t * module_name,
                                          fbe_dbgext_ptr base_ptr,
                                          fbe_trace_func_t trace_func,
                                          fbe_trace_context_t trace_context,
                                          fbe_debug_field_info_t *field_info_p,
                                          fbe_u32_t spaces_to_indent,
                                          fbe_debug_enum_trace_info_t *flags_info_p)
{
    fbe_u64_t enum_value = 0;

    if (field_info_p != NULL)
    {
        if (field_info_p->size > sizeof(enum_value))
        {
            trace_func(trace_context, "ERROR: %s field_info_p->size %d > sizeof(flags) %llu\n",
                       __FUNCTION__, field_info_p->size,
		       (unsigned long long)sizeof(enum_value));
            return FBE_STATUS_GENERIC_FAILURE;
        }
        FBE_READ_MEMORY(base_ptr + field_info_p->offset, &enum_value, field_info_p->size);
    }
    else
    {
        FBE_READ_MEMORY(base_ptr, &enum_value, sizeof(fbe_u32_t));
    }

    /* Loop over the list of flags info.
     */
    while (flags_info_p->string_p != NULL)
    {
        if (enum_value == flags_info_p->flag)
        {
            trace_func(trace_context, "%s ", flags_info_p->string_p);
            return FBE_STATUS_OK;
        }
        flags_info_p++;
    }

    trace_func(trace_context, "UNKNOWN enum 0x%llx",
	       (unsigned long long)enum_value);

    return FBE_STATUS_OK;
}
/********************************************
 * end fbe_debug_trace_enum_strings()
 ********************************************/
/*!**************************************************************
 * fbe_debug_dump_generate_prefix()
 ****************************************************************
 * @brief
 *  Utility functions for generating prefix on dumping SEP objects.
 *
 * @param new_prefix - the prefix to be generated.
 * @param old_prefix - the base prefix to be generated from.
 * @param next_prefix - added prefix.
 *
 * @return - FBE_STATUS_OK  
 *
 * @author
 *  8/20/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t fbe_debug_dump_generate_prefix(fbe_dump_debug_trace_prefix_t *new_prefix, 
                                            fbe_dump_debug_trace_prefix_t *old_prefix,
                                            char *next_prefix)
{
    if (!new_prefix) {
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    if (old_prefix != NULL) {
        memset(new_prefix->prefix, 0, FBE_MAX_DUMP_DEBUG_PREFIX_LEN);
        if (next_prefix != NULL) {
            _snprintf(new_prefix->prefix, FBE_MAX_DUMP_DEBUG_PREFIX_LEN - 1, "%s.%s", 
                      old_prefix->prefix, next_prefix);
        } else {
            _snprintf(new_prefix->prefix, FBE_MAX_DUMP_DEBUG_PREFIX_LEN - 1, "%s", old_prefix->prefix);
        }
    } else if (next_prefix != NULL) {
            _snprintf(new_prefix->prefix, FBE_MAX_DUMP_DEBUG_PREFIX_LEN - 1, "%s", next_prefix);
    } else {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}
/********************************************
 * end fbe_debug_dump_generate_prefix()
 ********************************************/
static fbe_u64_t fbe_debug_idle_thread_time = NULL;
void fbe_debug_get_idle_thread_time(const fbe_u8_t * module_name, fbe_time_t *time_p)
{
    if (fbe_debug_idle_thread_time == NULL){
        FBE_GET_EXPRESSION(module_name, idle_thread_time, &fbe_debug_idle_thread_time);
    }
    FBE_READ_MEMORY(fbe_debug_idle_thread_time, time_p, sizeof(fbe_time_t));
}
/*************************
 * end file fbe_debug_ext.c
 *************************/
