/***************************************************************************
 * Copyright (C) EMC Corporation 2010-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_terminator_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains debugging commands for terminator registry.
 *  This file for terminator registry which will allow us to look at 
 *  Enclosure and Port data Information. 
 *
 * @author
 *  16- May- 2011 - Created. Hari Singh Chauhan
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_port.h"
#include "terminator_sas_port.h"
#include "terminator_fc_port.h"
#include "terminator_base.h"
#include "terminator_enclosure.h"
#include "terminator_fc_enclosure.h"
#include "fbe_transport_debug.h"
#include "fbe_terminator_debug.h"
#include "fbe_terminator_device_registry.h"
#include "terminator_drive.h"


static fbe_status_t fbe_terminator_display_journal_records (const fbe_u8_t * module_name,
                                                            fbe_dbgext_ptr base_ptr,
                                                            fbe_trace_func_t trace_func,
                                                            fbe_trace_context_t trace_context,
                                                            fbe_debug_field_info_t *field_info_p,
                                                            fbe_u32_t spaces_to_indent);

static fbe_s32_t port_number = -1;
static fbe_s32_t enclosure_number = -1;
static fbe_s32_t drive_number = -1;
static fbe_s32_t recursive_number = -1;

/*!**************************************************************
 * fbe_terminator_registry_debug_trace()
 ****************************************************************
 * @brief
 *  Display information of terminator_registry_s structure.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param object_ptr - offset value of terminator registry structure.
 * @param port_number - port number for option.
 * @param encl_number - enclosure number for option.
 * @param drv_number - drive number for option.
 * @param rec_number - recursive number for option.
 * 
 * @return fbe_status_t
 *
 * @author
 *  16- May- 2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_terminator_registry_debug_trace(fbe_trace_func_t trace_func,
                                                 fbe_trace_context_t trace_context,
                                                 fbe_dbgext_ptr object_ptr,
                                                 fbe_s32_t prt_number,
                                                 fbe_s32_t encl_number,
                                                 fbe_s32_t drv_number,
                                                 fbe_s32_t rec_number)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t iterator = 0;
    tdr_u32_t capacity = 0;
    tdr_u32_t registry_element = 0;
    fbe_bool_t registry_info_dispaly = FBE_FALSE;
    fbe_u32_t registry_element_offset = 0;
    fbe_u32_t terminator_device_registry_size = 0;
    fbe_u64_t terminator_device_registry_ptr = 0;

    port_number = prt_number;
    enclosure_number = encl_number;
    drive_number = drv_number;
    recursive_number = rec_number;

    /* Get the registry element of fbe_terminator_device_registry_entry_t structure */
    FBE_GET_TYPE_SIZE_WITHOUT_MODULE_NAME(fbe_terminator_device_registry_entry_t, &terminator_device_registry_size);
    FBE_READ_MEMORY(object_ptr, &terminator_device_registry_ptr, sizeof(fbe_u64_t));
    fbe_trace_indent(trace_func, trace_context, TERMINATOR_REGISTRY_INDENT + 4);
    trace_func(trace_context, "registry: 0x%llx",
           (unsigned long long)terminator_device_registry_ptr);

    /* Get the capacity element of fbe_terminator_device_registry_entry_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_registry_t, "capacity", &registry_element_offset);
    FBE_READ_MEMORY(object_ptr + registry_element_offset, &capacity, sizeof(tdr_u32_t));
    fbe_trace_indent(trace_func, trace_context, TERMINATOR_REGISTRY_INDENT);
    trace_func(trace_context, "capacity: 0x%x", capacity);

    /* Get the initialized element of fbe_terminator_device_registry_entry_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_registry_t, "initialized", &registry_element_offset);
    FBE_READ_MEMORY(object_ptr + registry_element_offset, &registry_element, sizeof(tdr_u32_t));
    fbe_trace_indent(trace_func, trace_context, TERMINATOR_REGISTRY_INDENT);
    trace_func(trace_context, "initialized: 0x%x", registry_element);

    /* Get the device_counter element of fbe_terminator_device_registry_entry_t structure */ 
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_registry_t, "device_counter", &registry_element_offset);
    FBE_READ_MEMORY(object_ptr + registry_element_offset, &registry_element, sizeof(tdr_u32_t));
    fbe_trace_indent(trace_func, trace_context, TERMINATOR_REGISTRY_INDENT);
    trace_func(trace_context, "device_counter: 0x%x\n\n", registry_element);

    if(terminator_device_registry_ptr != NULL)
    {
        fbe_trace_indent(trace_func, trace_context, TERMINATOR_REGISTRY_INDENT);
        trace_func(trace_context, "registry_info:\n");

        for(iterator = 0; iterator < capacity; ++iterator)
        {
            status = fbe_terminator_device_registry_entry_debug_trace(trace_func, trace_context,
                               terminator_device_registry_ptr + (terminator_device_registry_size * iterator), &registry_info_dispaly);
        }
    }

    if(registry_info_dispaly == FBE_FALSE)
    {
        fbe_trace_indent(trace_func, trace_context, TERMINATOR_REGISTRY_INDENT + 4);
        trace_func(trace_context, "No data found According to given port/enclosure/drive information in Registry\n");
    }

    return status;
}

/*!**************************************************************
 * fbe_terminator_device_registry_entry_debug_trace()
 ****************************************************************
 * @brief
 *  Display information of terminator_device_registry_entry_s structure.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param registry_ptr - offset value of terminator_device_registry_entry structure
 * @param registry_info_display - boolen value for registry info.
 * 
 * @return fbe_status_t
 *
 * @author
 *  16- May- 2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_terminator_device_registry_entry_debug_trace(fbe_trace_func_t trace_func,
                                                              fbe_trace_context_t trace_context,
                                                              fbe_dbgext_ptr registry_ptr,
                                                              fbe_bool_t *registry_info_display)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t component_status = false;
    fbe_u32_t spaces_to_indent = 8;
    fbe_u32_t registry_element_offset = 0;
    fbe_u32_t registry_element_size = 0;
    tdr_generation_number_t generation_number = 0;
    tdr_device_type_t device_type = 0;
    tdr_device_ptr_t device_ptr = 0;

    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(fbe_terminator_device_registry_entry_t, "device_ptr", &registry_element_offset);
    FBE_GET_TYPE_SIZE_WITHOUT_MODULE_NAME(tdr_device_ptr_t, &registry_element_size);
    FBE_READ_MEMORY(registry_ptr + registry_element_offset, &device_ptr, registry_element_size);

    if(device_ptr != TDR_INVALID_PTR)
    {
        if(port_number != -1 || enclosure_number != -1 || drive_number != -1)
        {
            component_status = fbe_terminator_device_registry_component_type_port_debug_trace(trace_func, trace_context, device_ptr);
            if(component_status == FBE_FALSE)
            {
                return status;
            }
        }

        if(fbe_terminator_registry_recursive_debug_trace(device_ptr, &spaces_to_indent))
            return status;

        *registry_info_display = FBE_TRUE;

        trace_func(trace_context, "\n");
        FBE_GET_TYPE_SIZE_WITHOUT_MODULE_NAME(tdr_generation_number_t, &registry_element_size);
        FBE_READ_MEMORY(registry_ptr, &generation_number, registry_element_size);
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(trace_context, "generation_num: 0x%x", generation_number);

        FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(fbe_terminator_device_registry_entry_t, "device_type", &registry_element_offset);
        FBE_GET_TYPE_SIZE_WITHOUT_MODULE_NAME(tdr_device_type_t, &registry_element_size);
        FBE_READ_MEMORY(registry_ptr + registry_element_offset, &device_type, registry_element_size);
        fbe_trace_indent(trace_func, trace_context, 4);
        trace_func(trace_context, "device_type: 0x%llx",
           (unsigned long long)device_type);

        fbe_trace_indent(trace_func, trace_context, 4);
        trace_func(trace_context, "device_ptr: 0x%p\n", device_ptr);

        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(trace_context, "device_ptr_info:\n");
        status = fbe_terminator_device_registry_device_ptr_debug_trace(trace_func, trace_context, device_ptr);
    }

    return status;
}

/*!**************************************************************
 * fbe_terminator_registry_recursive_debug_trace()
 ****************************************************************
 * @brief
 *  This function use for recursive process and Space indent process
 *  
 * @param device_ptr - offset value of tdr_device_ptr pointer
 * @param spaces_to_indent - Number of spaces.
 * 
 * @return fbe_bool_t
 *
 * @author
 *  17- May- 2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_bool_t fbe_terminator_registry_recursive_debug_trace(void *device_ptr,
                                                         fbe_u32_t *spaces_to_indent)
{
    fbe_u32_t componenet_type_offset = 0;
    fbe_u32_t componenet_type_size = 0;
    terminator_component_type_t component_type = 0;

    /* Get the component_type of base_component_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(base_component_t, "component_type", &componenet_type_offset);
    FBE_GET_TYPE_SIZE_WITHOUT_MODULE_NAME(terminator_component_type_t, &componenet_type_size);
    FBE_READ_MEMORY((fbe_u64_t)device_ptr + componenet_type_offset, &component_type, componenet_type_size);

    /* This is for Recursive process use */
    if(recursive_number != -1 && component_type == TERMINATOR_COMPONENT_TYPE_BOARD)
    {
        return FBE_TRUE;
    }
    if(recursive_number == 1 && (component_type == TERMINATOR_COMPONENT_TYPE_ENCLOSURE || component_type == TERMINATOR_COMPONENT_TYPE_DRIVE))
    {
        return FBE_TRUE;
    }
    else if(recursive_number == 2 && component_type == TERMINATOR_COMPONENT_TYPE_DRIVE)
    {
        return FBE_TRUE;
    }

    /* This is for Indent process using */
    if(component_type == TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
    {
        *spaces_to_indent = 16;    // Enclosure Space Indent
    }
    else if(component_type == TERMINATOR_COMPONENT_TYPE_DRIVE || component_type == TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        *spaces_to_indent = 24;   // Drive and Virtual Drive Space Indent
    }

    return FBE_FALSE;
}

/*!**************************************************************
 * fbe_terminator_device_registry_device_ptr_debug_trace()
 ****************************************************************
 * @brief
 *  Display information of terminator device component type.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param device_ptr - offset value of tdr_device_ptr pointer.
 * 
 * 
 * @return fbe_status_t
 *
 * @author
 *  17- May- 2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_terminator_device_registry_device_ptr_debug_trace(fbe_trace_func_t trace_func,
                                                                   fbe_trace_context_t trace_context,
                                                                   void *device_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t componenet_type_offset = 0;
    fbe_u32_t componenet_type_size = 0;
    fbe_s32_t recursive_count = 3;	
    terminator_component_type_t component_type = 0;

    /* Get the component_type of base_component_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(base_component_t, "component_type", &componenet_type_offset);
    FBE_GET_TYPE_SIZE_WITHOUT_MODULE_NAME(terminator_component_type_t, &componenet_type_size);
    FBE_READ_MEMORY((fbe_u64_t)device_ptr + componenet_type_offset, &component_type, componenet_type_size);

    fbe_trace_indent(trace_func, trace_context, TERMINATOR_PORT_INDENT + 4);

    switch(component_type)
    {
        case TERMINATOR_COMPONENT_TYPE_BOARD :
                trace_func(trace_context, "component_type: %s\n", "TERMINATOR_COMPONENT_TYPE_BOARD");
                break;
        case TERMINATOR_COMPONENT_TYPE_PORT :
                status = fbe_terminator_device_registry_port_type_debug_trace(trace_func, trace_context, device_ptr);
                break;
        case TERMINATOR_COMPONENT_TYPE_ENCLOSURE :
                fbe_trace_indent(trace_func, trace_context, 8);
                status = fbe_terminator_device_registry_enclosure_type_debug_trace(trace_func, trace_context, device_ptr);
                break;
        case TERMINATOR_COMPONENT_TYPE_DRIVE :
                fbe_trace_indent(trace_func, trace_context, 16);
                status = fbe_terminator_device_registry_drive_type_debug_trace(trace_func, trace_context, device_ptr);
                break;
        case TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY :
                trace_func(trace_context, "\n");
                fbe_trace_indent(trace_func, trace_context, TERMINATOR_PORT_INDENT + 4);

                if((drive_number == -1 && recursive_number == -1) || (drive_number == -1 && recursive_number == recursive_count))
                    trace_func(trace_context, "component_type: %s\n", "TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY");
                break;
        default :
                trace_func(trace_context, "component_type: %s\n", "TERMINATOR_COMPONENT_TYPE_INVALID");
                break;
    }

    return status;
}

/*!**************************************************************
 * fbe_terminator_device_registry_component_type_port_debug_trace()
 ****************************************************************
 * @brief
 *  Here, we are checking port number for port option.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param device_ptr - offset value of terminator_device_registry_entry structure.
 * 
 * @return fbe_bool_t
 *
 * @author
 *  30- May- 2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_bool_t fbe_terminator_device_registry_component_type_port_debug_trace(fbe_trace_func_t trace_func,
                                                                          fbe_trace_context_t trace_context,
                                                                          void *device_ptr)
{
    fbe_u32_t componenet_type_offset = 0;
    fbe_u32_t componenet_type_size = 0;
    fbe_u32_t port_element_offset = 0;
    fbe_u32_t attributes_element_offset = 0;
    fbe_u32_t io_port_number = 0;
    fbe_u64_t attributes_ptr = 0;
    fbe_port_type_t port_type = 0;
    terminator_component_type_t component_type = 0;

    /* Get the component_type of base_component_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(base_component_t, "component_type", &componenet_type_offset);
    FBE_GET_TYPE_SIZE_WITHOUT_MODULE_NAME(terminator_component_type_t, &componenet_type_size);
    FBE_READ_MEMORY((fbe_u64_t)device_ptr + componenet_type_offset, &component_type, componenet_type_size);

    if(port_number != -1 && component_type == TERMINATOR_COMPONENT_TYPE_PORT)
    {
        FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_port_t, "port_type", &port_element_offset);
        FBE_READ_MEMORY((fbe_u64_t)device_ptr + port_element_offset, &port_type, sizeof(fbe_port_type_t));

        FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(base_component_t, "attributes", &attributes_element_offset);
        FBE_READ_MEMORY((fbe_u64_t)device_ptr + attributes_element_offset, &attributes_ptr, sizeof(fbe_u64_t));

        if(port_type == FBE_PORT_TYPE_SAS_LSI || port_type == FBE_PORT_TYPE_SAS_PMC)
        {
            FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_sas_port_info_t, "io_port_number", &attributes_element_offset);
            FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &io_port_number, sizeof(fbe_u32_t));
        }
        else if(port_type == FBE_PORT_TYPE_FC_PMC || port_type == FBE_PORT_TYPE_ISCSI || port_type == FBE_PORT_TYPE_FCOE)
        {
            FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_fc_port_info_t, "io_port_number", &attributes_element_offset);
            FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &io_port_number, sizeof(fbe_u32_t));
        }

        if(port_number == io_port_number)
        {
            return FBE_TRUE;
        }
    }
    else if(enclosure_number != -1 && component_type == TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
    {
        return fbe_terminator_device_registry_component_type_enclosure_debug_trace(trace_func, trace_context, device_ptr);
    }
    else if(drive_number != -1 && component_type == TERMINATOR_COMPONENT_TYPE_DRIVE)
    {
        return fbe_terminator_device_registry_component_type_drive_debug_trace(trace_func, trace_context, device_ptr);
    }

    return FBE_FALSE;
}

/*!**************************************************************
 * fbe_terminator_device_registry_port_type_debug_trace()
 ****************************************************************
 * @brief
 *  Display information of terminator device port type.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param port_ptr - offset value of tdr_device_ptr pointer.
 * 
 * 
 * @return fbe_status_t
 *
 * @author
 *  20- May- 2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_terminator_device_registry_port_type_debug_trace(fbe_trace_func_t trace_func,
                                                                  fbe_trace_context_t trace_context,
                                                                  void *port_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t port_element_offset = 0;
    fbe_u32_t port_elements = 0;
//    fbe_u32_t spaces_to_indent = spaces;
    fbe_port_type_t port_type = 0;
    fbe_port_speed_t port_speed = 0;
    fbe_bool_t port_reset_elements = 0;

    trace_func(trace_context, "component_type: %s\n", "TERMINATOR_COMPONENT_TYPE_PORT");

    /* Get the port_type element of terminator_port_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_port_t, "port_type", &port_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)port_ptr + port_element_offset, &port_type, sizeof(fbe_port_type_t));

    /* Get the attributes element of base_component_t structure */
    fbe_trace_indent(trace_func, trace_context, TERMINATOR_PORT_INDENT + 4);
    trace_func(trace_context, "attributes: \n");
    fbe_base_component_port_attributes_debug_trace(trace_func, trace_context, (fbe_u32_t)port_type, port_ptr);

    fbe_trace_indent(trace_func, trace_context, TERMINATOR_PORT_INDENT + 4);
    fbe_port_type_string_debug_trace(trace_func, trace_context, (fbe_u32_t)port_type);

    /* Get the port_speed element of terminator_port_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_port_t, "port_speed", &port_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)port_ptr + port_element_offset, &port_speed, sizeof(fbe_port_speed_t));
    fbe_trace_indent(trace_func, trace_context, 4);
    fbe_port_speed_string_debug_trace(trace_func, trace_context, (fbe_u32_t)port_speed);

    /* Get the maximum_transfer_bytes element of terminator_port_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_port_t, "maximum_transfer_bytes", &port_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)port_ptr + port_element_offset, &port_elements, sizeof(fbe_u32_t));
    fbe_trace_indent(trace_func, trace_context, 4);
    trace_func(trace_context, "maximum_transfer_bytes: 0x%x", port_elements);

    trace_func(trace_context, "\n");

    /* Get the maximum_sg_entries element of terminator_port_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_port_t, "maximum_sg_entries", &port_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)port_ptr + port_element_offset, &port_elements, sizeof(fbe_u32_t));
    fbe_trace_indent(trace_func, trace_context, TERMINATOR_PORT_INDENT + 4);
    trace_func(trace_context, "maximum_sg_entries: 0x%x", port_elements);

    /* Get the miniport_port_index element of terminator_port_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_port_t, "miniport_port_index", &port_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)port_ptr + port_element_offset, &port_elements, sizeof(fbe_u32_t));
    fbe_trace_indent(trace_func, trace_context, 4);
    trace_func(trace_context, "miniport_port_index: 0x%x", port_elements);

    /* Get the reset_begin element of terminator_port_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_port_t, "reset_begin", &port_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)port_ptr + port_element_offset, &port_reset_elements, sizeof(fbe_bool_t));
    fbe_trace_indent(trace_func, trace_context, 4);
    trace_func(trace_context, "reset_begin: 0x%x", port_reset_elements);

    /* Get the reset_completed element of terminator_port_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_port_t, "reset_completed", &port_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)port_ptr + port_element_offset, &port_reset_elements, sizeof(fbe_bool_t));
    fbe_trace_indent(trace_func, trace_context, 4);
    trace_func(trace_context, "reset_completed: 0x%x", port_reset_elements);

    trace_func(trace_context, "\n");

    return status;
}

/*!**************************************************************
 * fbe_port_type_string_debug_trace()
 ****************************************************************
 * @brief
 *  Display information of port type.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param port_type - enum value of port type.
 * 
 * @return fbe_status_t
 *
 * @author
 *  18- May- 2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_port_type_string_debug_trace(fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_u32_t port_type)
{
    fbe_status_t status = FBE_STATUS_OK;

    switch(port_type)
    {
        case FBE_PORT_TYPE_FIBRE :
            trace_func(trace_context, "port_type: %s", "FBE_PORT_TYPE_FIBRE");
            break;
        case FBE_PORT_TYPE_SAS_LSI :
            trace_func(trace_context, "port_type: %s", "FBE_PORT_TYPE_SAS_LSI");
            break;
        case FBE_PORT_TYPE_SAS_PMC :
            trace_func(trace_context, "port_type: %s", "FBE_PORT_TYPE_SAS_PMC");
            break;
        case FBE_PORT_TYPE_FC_PMC :
            trace_func(trace_context, "port_type: %s", "FBE_PORT_TYPE_FC_PMC");
            break;
        case FBE_PORT_TYPE_ISCSI :
            trace_func(trace_context, "port_type: %s", "FBE_PORT_TYPE_ISCSI");
            break;
        case FBE_PORT_TYPE_FCOE :
            trace_func(trace_context, "port_type: %s", "FBE_PORT_TYPE_FCOE");
            break;
        default :
            trace_func(trace_context, "port_type: %s", "FBE_PORT_TYPE_INVALID");
            break;
    }

    return status;
}

/*!**************************************************************
 * fbe_port_speed_string_debug_trace()
 ****************************************************************
 * @brief
 *  Display information of port speed.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param port_speed - enum value of port speed.
 * 
 * @return fbe_status_t
 *
 * @author
 *  18- May- 2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_port_speed_string_debug_trace(fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_u32_t port_speed)
{
    fbe_status_t status = FBE_STATUS_OK;

    switch(port_speed)
    {
        case FBE_PORT_SPEED_FIBRE_FIRST :
            trace_func(trace_context, "port_speed: %s", "FBE_PORT_SPEED_FIBRE_FIRST");
            break;
        case FBE_PORT_SPEED_FIBRE_1_GBPS :
            trace_func(trace_context, "port_speed: %s", "FBE_PORT_SPEED_FIBRE_1_GBPS");
            break;
        case FBE_PORT_SPEED_FIBRE_2_GBPS :
            trace_func(trace_context, "port_speed: %s", "FBE_PORT_SPEED_FIBRE_2_GBPS");
            break;
        case FBE_PORT_SPEED_FIBRE_4_GBPS :
            trace_func(trace_context, "port_speed: %s", "FBE_PORT_SPEED_FIBRE_4_GBPS");
            break;
        case FBE_PORT_SPEED_SAS_FIRST :
            trace_func(trace_context, "port_speed: %s", "FBE_PORT_SPEED_SAS_FIRST");
            break;
        case FBE_PORT_SPEED_SAS_1_5_GBPS :
            trace_func(trace_context, "port_speed: %s", "FBE_PORT_SPEED_SAS_1_5_GBPS");
            break;
        case FBE_PORT_SPEED_SAS_3_GBPS :
            trace_func(trace_context, "port_speed: %s", "FBE_PORT_SPEED_SAS_3_GBPS");
            break;
        case FBE_PORT_SPEED_SAS_6_GBPS :
            trace_func(trace_context, "port_speed: %s", "FBE_PORT_SPEED_SAS_6_GBPS");
            break;
        default :
            trace_func(trace_context, "port_speed: %s", "FBE_PORT_SPEED_INVALID");
            break;
    }

    return status;
}

/*!**************************************************************
 * fbe_terminator_sas_port_attributes_debug_trace()
 ****************************************************************
 * @brief
 *  Display information of sas port attributes.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param attributes_ptr - offset value of attributes element.
 *
 * 
 * @return fbe_status_t
 *
 * @author
 *  27- May- 2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_terminator_sas_port_attributes_debug_trace(fbe_trace_func_t trace_func,
                                                            fbe_trace_context_t trace_context,
                                                            fbe_u64_t attributes_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t attributes_element_offset = 0;
    fbe_u32_t attributes_elements = 0;

    /* Get the io_port_number element of terminator_sas_port_info_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_sas_port_info_t, "io_port_number", &attributes_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &attributes_elements, sizeof(fbe_u32_t));
    fbe_trace_indent(trace_func, trace_context, TERMINATOR_PORT_INDENT + 8);
    trace_func(trace_context, "io_port_number: 0x%x", attributes_elements);

    /* Get the portal_number element of terminator_sas_port_info_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_sas_port_info_t, "portal_number", &attributes_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &attributes_elements, sizeof(fbe_u32_t));
    fbe_trace_indent(trace_func, trace_context, 4);
    trace_func(trace_context, "portal_number: 0x%x", attributes_elements);

    trace_func(trace_context, "\n");

    /* Get the backend_number element of terminator_sas_port_info_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_sas_port_info_t, "backend_number", &attributes_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &attributes_elements, sizeof(fbe_u32_t));
    fbe_trace_indent(trace_func, trace_context, TERMINATOR_PORT_INDENT + 8);
    trace_func(trace_context, "backend_number: 0x%x", attributes_elements);

    /* Get the miniport_sas_device_table_index element of terminator_sas_port_info_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_sas_port_info_t, "miniport_sas_device_table_index", &attributes_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &attributes_elements, sizeof(fbe_u32_t));
    fbe_trace_indent(trace_func, trace_context, 4);
    trace_func(trace_context, "miniport_sas_device_table_index: 0x%x", attributes_elements);

    trace_func(trace_context, "\n");

    return status;
}

/*!**************************************************************
 * fbe_terminator_fc_port_attributes_debug_trace()
 ****************************************************************
 * @brief
 *  Display information of fc port attributes.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param attributes_ptr - offset value of attributes element.
 * 
 * 
 * @return fbe_status_t
 *
 * @author
 *  27- May- 2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_terminator_fc_port_attributes_debug_trace(fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_u64_t attributes_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t attributes_element_offset = 0;
    fbe_u32_t attributes_elements = 0;

    /* Get the io_port_number element of terminator_fc_port_info_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_fc_port_info_t, "io_port_number", &attributes_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &attributes_elements, sizeof(fbe_u32_t));
    fbe_trace_indent(trace_func, trace_context, TERMINATOR_PORT_INDENT + 8);	
    trace_func(trace_context, "io_port_number: 0x%x", attributes_elements);

    /* Get the portal_number element of terminator_fc_port_info_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_fc_port_info_t, "portal_number", &attributes_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &attributes_elements, sizeof(fbe_u32_t));
    fbe_trace_indent(trace_func, trace_context, 4);
    trace_func(trace_context, "portal_number: 0x%x", attributes_elements);

    trace_func(trace_context, "\n");

    /* Get the backend_number element of terminator_fc_port_info_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_fc_port_info_t, "backend_number", &attributes_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &attributes_elements, sizeof(fbe_u32_t));
    fbe_trace_indent(trace_func, trace_context, TERMINATOR_PORT_INDENT + 4);
    trace_func(trace_context, "backend_number: 0x%x", attributes_elements);

    trace_func(trace_context, "\n");

    return status;
}

/*!**************************************************************
 * fbe_base_component_port_attributes_debug_trace()
 ****************************************************************
 * @brief
 *  Display information all port attributes.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param port_type - enum value of port type.
 * @param port_ptr - offset value of port attributes element.
 * 
 * 
 * @return fbe_status_t
 *
 * @author
 *  27- May- 2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_base_component_port_attributes_debug_trace(fbe_trace_func_t trace_func,
                                                            fbe_trace_context_t trace_context,
                                                            fbe_u32_t port_type,
                                                            void *port_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t attributes_offset = 0;
    fbe_u64_t attributes_ptr = 0;

    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(base_component_t, "attributes", &attributes_offset);
    FBE_READ_MEMORY((fbe_u64_t)port_ptr + attributes_offset, &attributes_ptr, sizeof(fbe_u64_t));

    switch(port_type)
    {
        case FBE_PORT_TYPE_SAS_LSI :
        case FBE_PORT_TYPE_SAS_PMC :
            fbe_terminator_sas_port_attributes_debug_trace(trace_func, trace_context, attributes_ptr);
            break;
        case FBE_PORT_TYPE_FC_PMC :
        case FBE_PORT_TYPE_ISCSI :
        case FBE_PORT_TYPE_FCOE :
            fbe_terminator_fc_port_attributes_debug_trace(trace_func, trace_context, attributes_ptr);
            break;
    }

    return status;
}

/*!**************************************************************
 * fbe_terminator_device_registry_enclosure_type_debug_trace()
 ****************************************************************
 * @brief
 *  Display information of terminator device enclosure type.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param enclosure_ptr - offset value of tdr_device_ptr pointer.
 * 
 * 
 * @return fbe_status_t
 *
 * @author
 *  20- May- 2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_terminator_device_registry_enclosure_type_debug_trace(fbe_trace_func_t trace_func,
                                                                       fbe_trace_context_t trace_context,
                                                                       void *enclosure_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t encl_offset = 0;
//    fbe_u32_t spaces_to_indent = spaces;
    fbe_enclosure_types_t enclosure_type = 0;

    trace_func(trace_context, "component_type: %s\n", "TERMINATOR_COMPONENT_TYPE_ENCLOSURE");

    /* Get the encl_protocol element of terminator_enclosure_t structure */
    fbe_trace_indent(trace_func, trace_context, TERMINATOR_ENCLOSURE_INDENT + 4);
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_enclosure_t, "encl_protocol", &encl_offset);
    FBE_READ_MEMORY((fbe_u64_t)(enclosure_ptr) + encl_offset, &enclosure_type, sizeof(fbe_enclosure_type_t));
    fbe_enclosure_type_string_debug_trace(trace_func, trace_context, (fbe_u32_t)enclosure_type);

//    spaces_to_indent = spaces;
    /* Get the attributes element of base_component_t structure */
    trace_func(trace_context, "\n");
    fbe_trace_indent(trace_func, trace_context, TERMINATOR_ENCLOSURE_INDENT + 4);
    trace_func(trace_context, "attributes: \n");
    fbe_base_component_enclosure_attributes_debug_trace(trace_func, trace_context, (fbe_u32_t)enclosure_type, enclosure_ptr);

    trace_func(trace_context, "\n");

    return status;
}

/*!**************************************************************
 * fbe_terminator_device_registry_component_type_enclosure_debug_trace()
 ****************************************************************
 * @brief
 *  Here, we are checking enclosure number for enclosure option.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param device_ptr - offset value of terminator_device_registry_entry structure.
 * 
 * @return fbe_bool_t
 *
 * @author
 *  30- May- 2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_bool_t fbe_terminator_device_registry_component_type_enclosure_debug_trace(fbe_trace_func_t trace_func,
                                                                               fbe_trace_context_t trace_context,
                                                                               void *device_ptr)
{
    fbe_u32_t componenet_type_offset = 0;
    fbe_u32_t componenet_type_size = 0;
    fbe_u32_t encl_element_offset = 0;
    fbe_u32_t attributes_element_offset = 0;
    fbe_u64_t encl_number = 0;
    fbe_u64_t attributes_ptr = 0;
    fbe_enclosure_types_t enclosure_type = 0;
    terminator_component_type_t component_type = 0;

    /* Get the component_type of base_component_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(base_component_t, "component_type", &componenet_type_offset);
    FBE_GET_TYPE_SIZE_WITHOUT_MODULE_NAME(terminator_component_type_t, &componenet_type_size);
    FBE_READ_MEMORY((fbe_u64_t)device_ptr + componenet_type_offset, &component_type, componenet_type_size);

    if(enclosure_number != -1 && component_type == TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
    {
        FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_enclosure_t, "encl_protocol", &encl_element_offset);
        FBE_READ_MEMORY((fbe_u64_t)(device_ptr) + encl_element_offset, &enclosure_type, sizeof(fbe_enclosure_type_t));
  
        FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(base_component_t, "attributes", &attributes_element_offset);
        FBE_READ_MEMORY((fbe_u64_t)device_ptr + attributes_element_offset, &attributes_ptr, sizeof(fbe_u64_t));

        if(enclosure_type == FBE_ENCLOSURE_TYPE_FIBRE)
        {
            FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_fc_enclosure_info_t, "encl_number", &attributes_element_offset);
            FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &encl_number, sizeof(fbe_u32_t));
        }
        else if(enclosure_type == FBE_ENCLOSURE_TYPE_SAS)
        {
            FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_sas_enclosure_info_t, "encl_number", &attributes_element_offset);
            FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &encl_number, sizeof(fbe_u64_t));
        }
        else
        {
            trace_func(trace_context, "UNKOWN ENCLOSURE TYPE");
            return FBE_FALSE;
        }

        if(enclosure_number == encl_number)
        {
            return FBE_TRUE;
        }
    }
      
    return FBE_FALSE;
}

/*!**************************************************************
 * fbe_enclosure_type_string_debug_trace()
 ****************************************************************
 * @brief
 *  Display information of enclosure type.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param enclosure_type - enum value of enclosure type.
 * 
 * @return fbe_status_t
 *
 * @author
 *  23- May- 2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_enclosure_type_string_debug_trace(fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_u32_t enclosure_type)
{
    fbe_status_t status = FBE_STATUS_OK;

    switch(enclosure_type)
    {
        case FBE_ENCLOSURE_TYPE_FIBRE :
            trace_func(trace_context, "encl_protocol: %s", "FBE_ENCLOSURE_TYPE_FIBRE");
            break;
        case FBE_ENCLOSURE_TYPE_SAS :
            trace_func(trace_context, "encl_protocol: %s", "FBE_ENCLOSURE_TYPE_SAS");
            break;
        default :
            trace_func(trace_context, "encl_protocol: %s", "FBE_ENCLOSURE_TYPE_INVALID");
            break;
    }

    return status;
}

/*!**************************************************************
 * fbe_sas_enclosure_type_string_debug_trace()
 ****************************************************************
 * @brief
 *  Display information of sas enclosure type.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param enclosure_type - enum value of enclosure type.
 * 
 * @return fbe_status_t
 *
 * @author
 *  31- May- 2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_sas_enclosure_type_string_debug_trace(fbe_trace_func_t trace_func,
                                                       fbe_trace_context_t trace_context,
                                                       fbe_u32_t enclosure_type)
{
    fbe_status_t status = FBE_STATUS_OK;

    switch(enclosure_type)
    {
        case FBE_SAS_ENCLOSURE_TYPE_BULLET :
            trace_func(trace_context, "enclosure_type: %s", "FBE_SAS_ENCLOSURE_TYPE_BULLET");
            break;
        case FBE_SAS_ENCLOSURE_TYPE_VIPER :
            trace_func(trace_context, "enclosure_type: %s", "FBE_SAS_ENCLOSURE_TYPE_VIPER");
            break;
        case FBE_SAS_ENCLOSURE_TYPE_MAGNUM :
            trace_func(trace_context, "enclosure_type: %s", "FBE_SAS_ENCLOSURE_TYPE_MAGNUM");
            break;
        case FBE_SAS_ENCLOSURE_TYPE_CITADEL :
            trace_func(trace_context, "enclosure_type: %s", "FBE_SAS_ENCLOSURE_TYPE_CITADEL");
            break;
        case FBE_SAS_ENCLOSURE_TYPE_BUNKER :
            trace_func(trace_context, "enclosure_type: %s", "FBE_SAS_ENCLOSURE_TYPE_BUNKER");
            break;
        case FBE_SAS_ENCLOSURE_TYPE_DERRINGER :
            trace_func(trace_context, "enclosure_type: %s", "FBE_SAS_ENCLOSURE_TYPE_DERRINGER");
            break;
        case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM :
            trace_func(trace_context, "enclosure_type: %s", "FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM");
            break;
        case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE :
            trace_func(trace_context, "enclosure_type: %s", "FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE");
            break;
        case FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP:
            trace_func(trace_context, "enclosure_type: %s", "FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP");
            break;
        case FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP :
            trace_func(trace_context, "enclosure_type: %s", "FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP");
            break;
        case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP:
            trace_func(trace_context, "enclosure_type: %s", "FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP");
            break;
        case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_DRVSXP :
            trace_func(trace_context, "enclosure_type: %s", "FBE_SAS_ENCLOSURE_TYPE_CAYENNE_DRVSXP");
            break;
        case FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP:
            trace_func(trace_context, "enclosure_type: %s", "FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP");
            break;
        case FBE_SAS_ENCLOSURE_TYPE_NAGA_DRVSXP :
            trace_func(trace_context, "enclosure_type: %s", "FBE_SAS_ENCLOSURE_TYPE_NAGA_DRVSXP");
            break;
        case FBE_SAS_ENCLOSURE_TYPE_FALLBACK :
            trace_func(trace_context, "enclosure_type: %s", "FBE_SAS_ENCLOSURE_TYPE_FALLBACK");
            break;
        case FBE_SAS_ENCLOSURE_TYPE_BOXWOOD :
            trace_func(trace_context, "enclosure_type: %s", "FBE_SAS_ENCLOSURE_TYPE_BOXWOOD");
            break;
        case FBE_SAS_ENCLOSURE_TYPE_KNOT :
            trace_func(trace_context, "enclosure_type: %s", "FBE_SAS_ENCLOSURE_TYPE_KNOT");
            break;
        case FBE_SAS_ENCLOSURE_TYPE_PINECONE :
            trace_func(trace_context, "enclosure_type: %s", "FBE_SAS_ENCLOSURE_TYPE_PINECONE");
            break;
        case FBE_SAS_ENCLOSURE_TYPE_STEELJAW :
            trace_func(trace_context, "enclosure_type: %s", "FBE_SAS_ENCLOSURE_TYPE_STEELJAW");
            break;
        case FBE_SAS_ENCLOSURE_TYPE_RAMHORN :
            trace_func(trace_context, "enclosure_type: %s", "FBE_SAS_ENCLOSURE_TYPE_RAMHORN");
            break;
        case FBE_SAS_ENCLOSURE_TYPE_CALYPSO :
            trace_func(trace_context, "enclosure_type: %s", "FBE_SAS_ENCLOSURE_TYPE_CALYPSO");
            break;
        case FBE_SAS_ENCLOSURE_TYPE_RHEA :
            trace_func(trace_context, "enclosure_type: %s", "FBE_SAS_ENCLOSURE_TYPE_RHEA");
            break;
        case FBE_SAS_ENCLOSURE_TYPE_MIRANDA :
            trace_func(trace_context, "enclosure_type: %s", "FBE_SAS_ENCLOSURE_TYPE_MIRANDA");
            break;
        case FBE_SAS_ENCLOSURE_TYPE_TABASCO :
            trace_func(trace_context, "enclosure_type: %s", "FBE_SAS_ENCLOSURE_TYPE_TABASCO");
            break;
        default :
            trace_func(trace_context, "enclosure_type: %s", "FBE_SAS_ENCLOSURE_TYPE_INVALID");
            break;
    }

    return status;
}

/*!**************************************************************
 * fbe_terminator_sas_enclosure_attributes_debug_trace()
 ****************************************************************
 * @brief
 *  Display information sas enclosure attributes.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param attributes_ptr - offset value of terminator registry structure.
 *
 * 
 * @return fbe_status_t
 *
 * @author
 *  27- May- 2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_terminator_sas_enclosure_attributes_debug_trace(fbe_trace_func_t trace_func,
                                                                 fbe_trace_context_t trace_context,
                                                                 fbe_u64_t attributes_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t attributes_element_offset = 0;
    fbe_u32_t attributes_elements = 0;
    fbe_u64_t encl_number = 0;
    fbe_sas_enclosure_type_t enclosure_type = 0;
   
    /* Get the enclosure_type element of terminator_sas_enclosure_info_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_sas_enclosure_info_t, "enclosure_type", &attributes_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &enclosure_type, sizeof(fbe_sas_enclosure_type_t));
    fbe_trace_indent(trace_func, trace_context, TERMINATOR_ENCLOSURE_INDENT + 8);
    fbe_sas_enclosure_type_string_debug_trace(trace_func, trace_context, (fbe_u32_t)enclosure_type);

    /* Get the encl_number element of terminator_sas_enclosure_info_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_sas_enclosure_info_t, "encl_number", &attributes_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &encl_number, sizeof(fbe_u64_t));
    fbe_trace_indent(trace_func, trace_context, 4);
    trace_func(trace_context, "encl_number: 0x%llx",
           (unsigned long long)encl_number);

    trace_func(trace_context, "\n");

    /* Get the backend element of terminator_sas_enclosure_info_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_sas_enclosure_info_t, "backend", &attributes_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &attributes_elements, sizeof(fbe_u32_t));
    fbe_trace_indent(trace_func, trace_context, TERMINATOR_ENCLOSURE_INDENT + 8);
    trace_func(trace_context, "backend: 0x%x", attributes_elements);

    /* Get the miniport_sas_device_table_index element of terminator_sas_enclosure_info_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_sas_enclosure_info_t, "miniport_sas_device_table_index", &attributes_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &attributes_elements, sizeof(fbe_u32_t));
    fbe_trace_indent(trace_func, trace_context, 4);
    trace_func(trace_context, "miniport_sas_device_table_index: 0x%x", attributes_elements);

    return status;
}

/*!**************************************************************
 * fbe_fc_enclosure_type_string_debug_trace()
 ****************************************************************
 * @brief
 *  Display information of fc enclosure type.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param enclosure_type - enum value of enclosure type.
 * 
 * @return fbe_status_t
 *
 * @author
 *  31- May- 2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_fc_enclosure_type_string_debug_trace(fbe_trace_func_t trace_func,
                                                      fbe_trace_context_t trace_context,
                                                      fbe_u32_t enclosure_type)
{
    fbe_status_t status = FBE_STATUS_OK;

    switch(enclosure_type)
    {
        case FBE_FC_ENCLOSURE_TYPE_STILLETO :
            trace_func(trace_context, "enclosure_type: %s", "FBE_FC_ENCLOSURE_TYPE_STILLETO");
            break;
        case FBE_FC_ENCLOSURE_TYPE_KLONDIKE :
            trace_func(trace_context, "enclosure_type: %s", "FBE_FC_ENCLOSURE_TYPE_KLONDIKE");
            break;
        case FBE_FC_ENCLOSURE_TYPE_KATANA :
            trace_func(trace_context, "enclosure_type: %s", "FBE_FC_ENCLOSURE_TYPE_KATANA");
            break;
        default :
            trace_func(trace_context, "enclosure_type: %s", "FBE_FC_ENCLOSURE_TYPE_INVALID");
            break;
    }

    return status;
}

/*!**************************************************************
 * fbe_terminator_fc_enclosure_attributes_debug_trace()
 ****************************************************************
 * @brief
 *  Display information fc enclosure attributes.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param attributes_ptr - offset value of attributes element.
 * 
 * 
 * @return fbe_status_t
 *
 * @author
 *  27- May- 2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_terminator_fc_enclosure_attributes_debug_trace(fbe_trace_func_t trace_func,
                                                                fbe_trace_context_t trace_context,
                                                                fbe_u64_t attributes_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t attributes_element_offset = 0;
    fbe_u32_t attributes_elements = 0;
    fbe_fc_enclosure_type_t enclosure_type = 0;

    /* Get the enclosure_type element of terminator_fc_enclosure_info_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_fc_enclosure_info_t, "enclosure_type", &attributes_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &enclosure_type, sizeof(fbe_sas_enclosure_type_t));
    fbe_trace_indent(trace_func, trace_context, TERMINATOR_ENCLOSURE_INDENT + 8);
    fbe_fc_enclosure_type_string_debug_trace(trace_func, trace_context, (fbe_u32_t)enclosure_type);

    /* Get the encl_number element of terminator_fc_enclosure_info_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_fc_enclosure_info_t, "encl_number", &attributes_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &attributes_elements, sizeof(fbe_u32_t));
    fbe_trace_indent(trace_func, trace_context, 4);
    trace_func(trace_context, "encl_number: 0x%x", attributes_elements);

    /* Get the encl_state_change element of terminator_fc_enclosure_info_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_fc_enclosure_info_t, "encl_state_change", &attributes_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &attributes_elements, sizeof(fbe_bool_t));
    fbe_trace_indent(trace_func, trace_context, 4);
    trace_func(trace_context, "encl_state_change: 0x%x", attributes_elements);

    trace_func(trace_context, "\n");

    /* Get the backend element of terminator_fc_enclosure_info_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_fc_enclosure_info_t, "backend", &attributes_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &attributes_elements, sizeof(fbe_u32_t));
    fbe_trace_indent(trace_func, trace_context, TERMINATOR_ENCLOSURE_INDENT + 8);
    trace_func(trace_context, "backend: 0x%x", attributes_elements);

    return status;	
}

/*!**************************************************************
 * fbe_base_component_enclosure_attributes_debug_trace()
 ****************************************************************
 * @brief
 *  Display information all enclosure attributes.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param enclosure_type - enum value of enclosure type.
 * @param enclosure_ptr -offset value of enclosure attributes element.
 * 
 * 
 * @return fbe_status_t
 *
 * @author
 *  27- May- 2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_base_component_enclosure_attributes_debug_trace(fbe_trace_func_t trace_func,
                                                                 fbe_trace_context_t trace_context,
                                                                 fbe_u32_t enclosure_type,
                                                                 void *enclosure_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t attributes_offset = 0;
    fbe_u64_t attributes_ptr = 0;

    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(base_component_t, "attributes", &attributes_offset);
    FBE_READ_MEMORY((fbe_u64_t)enclosure_ptr + attributes_offset, &attributes_ptr, sizeof(fbe_u64_t));

    switch(enclosure_type)
    {
        case FBE_ENCLOSURE_TYPE_FIBRE :
            fbe_terminator_fc_enclosure_attributes_debug_trace(trace_func, trace_context, attributes_ptr);
            break;
        case FBE_ENCLOSURE_TYPE_SAS :
            fbe_terminator_sas_enclosure_attributes_debug_trace(trace_func, trace_context, attributes_ptr);
            break;
        default:
            trace_func(trace_context, "Unkown elclosure type\n");
            return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!**************************************************************
 * fbe_terminator_device_registry_drive_type_debug_trace()
 ****************************************************************
 * @brief
 *  Display information of terminator device drive type.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param drive_ptr - offset value of tdr_device_ptr pointer.
 * 
 * 
 * @return fbe_status_t
 *
 * @author
 *  23- May- 2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_terminator_device_registry_drive_type_debug_trace(fbe_trace_func_t trace_func,
                                                                   fbe_trace_context_t trace_context,
                                                                   void *drive_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t drive_element_offset = 0;
//    fbe_u32_t spaces_to_indent = spaces;
    fbe_drive_type_t drive_type = 0;
    /* Use blank module name so no matter executible name, it will just search for symbols.
     */
    char * module_name = "";

    trace_func(trace_context, "component_type: %s\n", "TERMINATOR_COMPONENT_TYPE_DRIVE");

    /* Get the drive_protocol element of terminator_drive_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_drive_t, "drive_protocol", &drive_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)drive_ptr + drive_element_offset, &drive_type, sizeof(fbe_drive_type_t));

    fbe_trace_indent(trace_func, trace_context, TERMINATOR_DRIVE_INDENT);
    trace_func(trace_context, "attributes: \n");
    fbe_base_component_drive_attributes_debug_trace(trace_func, trace_context, (fbe_u32_t)drive_type, drive_ptr);

    fbe_trace_indent(trace_func, trace_context, TERMINATOR_DRIVE_INDENT);
    fbe_drive_type_string_debug_trace(trace_func, trace_context, (fbe_u32_t)drive_type);

    trace_func(trace_context, "\n");
    fbe_trace_indent(trace_func, trace_context, TERMINATOR_DRIVE_INDENT);

    fbe_terminator_drive_debug_trace(module_name, (fbe_dbgext_ptr)drive_ptr,
                                     trace_func, trace_context, NULL, TERMINATOR_DRIVE_INDENT, FBE_TRUE /* display journal */);

    trace_func(trace_context, "\n");

    return status;
}

/*!**************************************************************
 * fbe_terminator_device_registry_component_type_drive_debug_trace()
 ****************************************************************
 * @brief
 *  Here, we are checking drive number for drive option.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param device_ptr - offset value of terminator_device_registry_entry structure.
 * 
 * @return fbe_bool_t
 *
 * @author
 *  30- May- 2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_bool_t fbe_terminator_device_registry_component_type_drive_debug_trace(fbe_trace_func_t trace_func,
                                                                           fbe_trace_context_t trace_context,
                                                                           void *device_ptr)
{
    fbe_u32_t componenet_type_offset = 0;
    fbe_u32_t componenet_type_size = 0;
    fbe_u32_t drive_element_offset = 0;
    fbe_u32_t attributes_element_offset = 0;
    fbe_u32_t attributes_elements = 0;
    fbe_u64_t attributes_ptr = 0;
    fbe_drive_type_t drive_type = 0;
    terminator_component_type_t component_type = 0;

    /* Get the component_type of base_component_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(base_component_t, "component_type", &componenet_type_offset);
    FBE_GET_TYPE_SIZE_WITHOUT_MODULE_NAME(terminator_component_type_t, &componenet_type_size);
    FBE_READ_MEMORY((fbe_u64_t)device_ptr + componenet_type_offset, &component_type, componenet_type_size);

    if(drive_number != -1 && component_type == TERMINATOR_COMPONENT_TYPE_DRIVE)
    {
        FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_drive_t, "drive_protocol", &drive_element_offset);
        FBE_READ_MEMORY((fbe_u64_t)device_ptr + drive_element_offset, &drive_type, sizeof(fbe_drive_type_t));

        FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(base_component_t, "attributes", &attributes_element_offset);
        FBE_READ_MEMORY((fbe_u64_t)device_ptr + attributes_element_offset, &attributes_ptr, sizeof(fbe_u64_t));

        if(drive_type == FBE_DRIVE_TYPE_FIBRE)
        {
            FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_fc_drive_info_t, "slot_number", &attributes_element_offset);
            FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &attributes_elements, sizeof(fbe_u32_t));
        }
        else if(drive_type == FBE_DRIVE_TYPE_SAS)
        {
            FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_sas_drive_info_t, "slot_number", &attributes_element_offset);
            FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &attributes_elements, sizeof(fbe_u32_t));
        }
        else if(drive_type == FBE_DRIVE_TYPE_SATA)
        {
            FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_sata_drive_info_t, "slot_number", &attributes_element_offset);
            FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &attributes_elements, sizeof(fbe_u32_t));
        }
        else
        {
            trace_func(trace_context, "Drive type %d not supported.\n",drive_type);
            return FBE_FALSE;
        }

        if(drive_number == attributes_elements)
        {
            return FBE_TRUE;
        }
    }
   
    return FBE_FALSE;
}

/*!**************************************************************
 * fbe_drive_type_string_debug_trace()
 ****************************************************************
 * @brief
 *  Display information of drive type.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param drive_type - enum value of drive type.
 * 
 * @return fbe_status_t
 *
 * @author
 *  24- May- 2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_drive_type_string_debug_trace(fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_u32_t drive_type)
{
    fbe_status_t status = FBE_STATUS_OK;

    switch(drive_type)
    {
        case FBE_DRIVE_TYPE_FIBRE :
            trace_func(trace_context, "drive_protocol: %s", "FBE_DRIVE_TYPE_FIBRE");
            break;
        case FBE_DRIVE_TYPE_SAS :
            trace_func(trace_context, "drive_protocol: %s", "FBE_DRIVE_TYPE_SAS");
            break;
        case FBE_DRIVE_TYPE_SATA :
            trace_func(trace_context, "drive_protocol: %s", "FBE_DRIVE_TYPE_SATA");
            break;
        case FBE_DRIVE_TYPE_SAS_FLASH_HE :
            trace_func(trace_context, "drive_protocol: %s", "FBE_DRIVE_TYPE_SAS_FLASH_HE");
            break;
        case FBE_DRIVE_TYPE_SATA_FLASH_HE :
            trace_func(trace_context, "drive_protocol: %s", "FBE_DRIVE_TYPE_SATA_FLASH_HE");
            break;
        case FBE_DRIVE_TYPE_SAS_NL :
            trace_func(trace_context, "drive_protocol: %s", "FBE_DRIVE_TYPE_SAS_NL");
            break;
        case FBE_DRIVE_TYPE_SATA_PADDLECARD :
            trace_func(trace_context, "drive_protocol: %s", "FBE_DRIVE_TYPE_SATA_PADDLECARD");
            break;
        case FBE_DRIVE_TYPE_SAS_FLASH_ME :
            trace_func(trace_context, "drive_protocol: %s", "FBE_DRIVE_TYPE_SAS_FLASH_ME");
            break;            
        case FBE_DRIVE_TYPE_SAS_FLASH_LE :
            trace_func(trace_context, "drive_protocol: %s", "FBE_DRIVE_TYPE_SAS_FLASH_LE");
            break;            
        case FBE_DRIVE_TYPE_SAS_FLASH_RI :
            trace_func(trace_context, "drive_protocol: %s", "FBE_DRIVE_TYPE_SAS_FLASH_RI");
            break;            
        default :
            trace_func(trace_context, "drive_protocol: %s", "FBE_DRIVE_TYPE_INVALID");
            break;
    }

    return status;
}

/*!**************************************************************
 * fbe_drive_type_string_debug_trace()
 ****************************************************************
 * @brief
 *  Display information of fc drive type.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param drive_type - enum value of drive type.
 * 
 * @return fbe_status_t
 *
 * @author
 *  31- May- 2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_fc_drive_type_string_debug_trace(fbe_trace_func_t trace_func,
                                                  fbe_trace_context_t trace_context,
                                                  fbe_u32_t drive_type)
{
    fbe_status_t status = FBE_STATUS_OK;

    switch(drive_type)
    {
        case FBE_FC_DRIVE_CHEETA_15K :
            trace_func(trace_context, "drive_type: %s", "FBE_FC_DRIVE_CHEETA_15K");
            break;
        case FBE_FC_DRIVE_UNICORN_512 :
            trace_func(trace_context, "drive_type: %s", "FBE_FC_DRIVE_UNICORN_512");
            break;
        case FBE_FC_DRIVE_UNICORN_4096 :
            trace_func(trace_context, "drive_type: %s", "FBE_FC_DRIVE_UNICORN_4096");
            break;
        case FBE_FC_DRIVE_UNICORN_4160 :
            trace_func(trace_context, "drive_type: %s", "FBE_FC_DRIVE_UNICORN_4160");
            break;
        case FBE_FC_DRIVE_SIM_520 :
            trace_func(trace_context, "drive_type: %s", "FBE_FC_DRIVE_SIM_520 ");
            break;
        case FBE_FC_DRIVE_SIM_512 :
            trace_func(trace_context, "drive_type: %s", "FBE_FC_DRIVE_SIM_512");
            break;
        default :
            trace_func(trace_context, "drive_type: %s", "FBE_FC_DRIVE_INVALID");
            break;
    }

    return status;
}

/*!**************************************************************
 * fbe_terminator_fc_drive_attributes_debug_trace()
 ****************************************************************
 * @brief
 *  Display information fc drive attributes.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param attributes_ptr - offset value of attributes element.
 * 
 * 
 * @return fbe_status_t
 *
 * @author
 *  27- May- 2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_terminator_fc_drive_attributes_debug_trace(fbe_trace_func_t trace_func,
                                                            fbe_trace_context_t trace_context,
                                                            fbe_u64_t attributes_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t attributes_element_offset = 0;
    fbe_u32_t attributes_elements = 0;
    fbe_fc_drive_type_t drive_type = 0;

    /* Get the drive_type element of terminator_fc_drive_info_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_fc_drive_info_t, "drive_type", &attributes_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &drive_type, sizeof(fbe_fc_drive_type_t));
    fbe_trace_indent(trace_func, trace_context, TERMINATOR_DRIVE_INDENT + 4);
    fbe_fc_drive_type_string_debug_trace(trace_func, trace_context, (fbe_u32_t)drive_type);	

    /* Get the encl_number element of terminator_fc_drive_info_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_fc_drive_info_t, "encl_number", &attributes_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &attributes_elements, sizeof(fbe_u32_t));
    fbe_trace_indent(trace_func, trace_context, 4);
    trace_func(trace_context, "encl_number: 0x%x", attributes_elements);

    /* Get the slot_number element of terminator_fc_drive_info_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_fc_drive_info_t, "slot_number", &attributes_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &attributes_elements, sizeof(fbe_u32_t));
    fbe_trace_indent(trace_func, trace_context, 4);
    trace_func(trace_context, "slot_number: 0x%x", attributes_elements);

    trace_func(trace_context, "\n");

    /* Get the backend_number element of terminator_fc_drive_info_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_fc_drive_info_t, "backend_number", &attributes_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &attributes_elements, sizeof(fbe_u32_t));
    fbe_trace_indent(trace_func, trace_context, TERMINATOR_DRIVE_INDENT + 4);
    trace_func(trace_context, "backend_number: 0x%x", attributes_elements);

    trace_func(trace_context, "\n");

    return status;	
}

/*!**************************************************************
 * fbe_sas_drive_type_string_debug_trace()
 ****************************************************************
 * @brief
 *  Display information of sas drive type.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param drive_type - enum value of drive type.
 * 
 * @return fbe_status_t
 *
 * @author
 *  31- May- 2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_sas_drive_type_string_debug_trace(fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_u32_t drive_type)
{
    fbe_status_t status = FBE_STATUS_OK;

    switch(drive_type)
    {
        case FBE_SAS_DRIVE_CHEETA_15K :
            trace_func(trace_context, "drive_type: %s", "FBE_SAS_DRIVE_CHEETA_15K");
            break;
        case FBE_SAS_DRIVE_UNICORN_512 :
            trace_func(trace_context, "drive_type: %s", "FBE_SAS_DRIVE_UNICORN_512");
            break;
        case FBE_SAS_DRIVE_UNICORN_4096 :
            trace_func(trace_context, "drive_type: %s", "FBE_SAS_DRIVE_UNICORN_4096");
            break;
        case FBE_SAS_DRIVE_UNICORN_4160 :
            trace_func(trace_context, "drive_type: %s", "FBE_SAS_DRIVE_UNICORN_4160");
            break;
        case FBE_SAS_DRIVE_SIM_520 :
            trace_func(trace_context, "drive_type: %s", "FBE_SAS_DRIVE_SIM_520 ");
            break;
        case FBE_SAS_DRIVE_SIM_520_FLASH_HE :
            trace_func(trace_context, "drive_type: %s", "FBE_SAS_DRIVE_SIM_520_FLASH_HE ");
            break;
        case FBE_SAS_DRIVE_SIM_520_FLASH_UNMAP :
            trace_func(trace_context, "drive_type: %s", "FBE_SAS_DRIVE_SIM_520_FLASH_UNMAP ");
            break;
        case FBE_SAS_DRIVE_SIM_4160_FLASH_UNMAP :
            trace_func(trace_context, "drive_type: %s", "FBE_SAS_DRIVE_SIM_4160_FLASH_UNMAP ");
            break;
        case FBE_SAS_DRIVE_SIM_520_UNMAP :
            trace_func(trace_context, "drive_type: %s", "FBE_SAS_DRIVE_SIM_520_UNMAP ");
            break;
        case FBE_SAS_DRIVE_UNICORN_4160_UNMAP :
            trace_func(trace_context, "drive_type: %s", "FBE_SAS_DRIVE_UNICORN_4160_UNMAP");
            break;
        case FBE_SAS_NL_DRIVE_SIM_520 :
            trace_func(trace_context, "drive_type: %s", "FBE_SAS_NL_DRIVE_SIM_520 ");
            break;
        case FBE_SAS_DRIVE_SIM_512 :
            trace_func(trace_context, "drive_type: %s", "FBE_SAS_DRIVE_SIM_512");
            break;
        case FBE_SAS_DRIVE_COBRA_E_10K :
            trace_func(trace_context, "drive_type: %s", "FBE_SAS_DRIVE_COBRA_E_10K");
            break;
        case FBE_SAS_DRIVE_SIM_520_12G:
            trace_func(trace_context, "drive_type: %s", "FBE_SAS_DRIVE_SIM_520_12G");
            break;
        case FBE_SAS_DRIVE_SIM_520_FLASH_ME :
            trace_func(trace_context, "drive_type: %s", "FBE_SAS_DRIVE_SIM_520_FLASH_ME");
            break;
        case FBE_SAS_DRIVE_SIM_520_FLASH_LE :
            trace_func(trace_context, "drive_protocol: %s", "FBE_SAS_DRIVE_SIM_520_FLASH_LE");
            break;
        case FBE_SAS_DRIVE_SIM_520_FLASH_RI :
            trace_func(trace_context, "drive_protocol: %s", "FBE_SAS_DRIVE_SIM_520_FLASH_RI");
            break;
        default :
            trace_func(trace_context, "drive_type: %s", "FBE_SAS_DRIVE_INVALID");
            break;
    }

    return status;
}

/*!**************************************************************
 * fbe_terminator_sas_drive_attributes_debug_trace()
 ****************************************************************
 * @brief
 *  Display information sas drive attributes.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param attributes_ptr - offset value of attributes element.
 * 
 * 
 * @return fbe_status_t
 *
 * @author
 *  27- May- 2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_terminator_sas_drive_attributes_debug_trace(fbe_trace_func_t trace_func,
                                                             fbe_trace_context_t trace_context,
                                                             fbe_u64_t attributes_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t attributes_element_offset = 0;
    fbe_u32_t attributes_elements = 0;
    fbe_sas_drive_type_t drive_type = 0;

    /* Get the drive_type element of terminator_sas_drive_info_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_sas_drive_info_t, "drive_type", &attributes_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &drive_type, sizeof(fbe_sas_drive_type_t));
    fbe_trace_indent(trace_func, trace_context, TERMINATOR_DRIVE_INDENT + 4);
    fbe_sas_drive_type_string_debug_trace(trace_func, trace_context, (fbe_u32_t)drive_type);	

    /* Get the encl_number element of terminator_sas_drive_info_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_sas_drive_info_t, "encl_number", &attributes_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &attributes_elements, sizeof(fbe_u32_t));
    fbe_trace_indent(trace_func, trace_context, 4);
    trace_func(trace_context, "encl_number: 0x%x", attributes_elements);

    /* Get the slot_number element of terminator_sas_drive_info_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_sas_drive_info_t, "slot_number", &attributes_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &attributes_elements, sizeof(fbe_bool_t));
    fbe_trace_indent(trace_func, trace_context, 4);
    trace_func(trace_context, "slot_number: 0x%x", attributes_elements);

    trace_func(trace_context, "\n");

    /* Get the backend_number element of terminator_sas_drive_info_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_sas_drive_info_t, "backend_number", &attributes_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &attributes_elements, sizeof(fbe_u32_t));
    fbe_trace_indent(trace_func, trace_context, TERMINATOR_DRIVE_INDENT + 4);
    trace_func(trace_context, "backend_number: 0x%x", attributes_elements);

    /* Get the miniport_sas_device_table_index element of terminator_sas_drive_info_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_sas_drive_info_t, "miniport_sas_device_table_index", &attributes_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &attributes_elements, sizeof(fbe_u32_t));
    fbe_trace_indent(trace_func, trace_context, 4);
    trace_func(trace_context, "miniport_sas_device_table_index: 0x%x", attributes_elements);	

    trace_func(trace_context, "\n");

    return status;	
}

/*!**************************************************************
 * fbe_sata_drive_type_string_debug_trace()
 ****************************************************************
 * @brief
 *  Display information of sata drive type.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param drive_type - enum value of drive type.
 * 
 * @return fbe_status_t
 *
 * @author
 *  31- May- 2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_sata_drive_type_string_debug_trace(fbe_trace_func_t trace_func,
                                                    fbe_trace_context_t trace_context,
                                                    fbe_u32_t drive_type)
{
    fbe_status_t status = FBE_STATUS_OK;

    switch(drive_type)
    {
        case FBE_SATA_DRIVE_HITACHI_HUA :
            trace_func(trace_context, "drive_type: %s", "FBE_SATA_DRIVE_HITACHI_HUA");
            break;
        case FBE_SATA_DRIVE_SIM_512 :
            trace_func(trace_context, "drive_type: %s", "FBE_SATA_DRIVE_SIM_512");
            break;
        case FBE_SATA_DRIVE_SIM_512_FLASH:
            trace_func(trace_context, "drive_type: %s", "FBE_SATA_DRIVE_SIM_512_FLASH");
            break;

        default :
            trace_func(trace_context, "drive_type: %s", "FBE_SATA_DRIVE_INVALID");
            break;
    }

    return status;
}

/*!**************************************************************
 * fbe_terminator_sata_drive_attributes_debug_trace()
 ****************************************************************
 * @brief
 *  Display information sata drive attributes.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param attributes_ptr - offset value of attributes element.
 * 
 * 
 * @return fbe_status_t
 *
 * @author
 *  27- May- 2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_terminator_sata_drive_attributes_debug_trace(fbe_trace_func_t trace_func,
                                                              fbe_trace_context_t trace_context,
                                                              fbe_u64_t attributes_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t attributes_element_offset = 0;
    fbe_u32_t attributes_elements = 0;
    fbe_sata_drive_type_t drive_type = 0;

    /* Get the port_type element of terminator_port_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_sata_drive_info_t, "drive_type", &attributes_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &drive_type, sizeof(fbe_sata_drive_type_t));
    fbe_trace_indent(trace_func, trace_context, TERMINATOR_DRIVE_INDENT + 4);
    fbe_sata_drive_type_string_debug_trace(trace_func, trace_context, (fbe_u32_t)drive_type);	

    /* Get the port_speed element of terminator_port_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_sata_drive_info_t, "encl_number", &attributes_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &attributes_elements, sizeof(fbe_u32_t));
    fbe_trace_indent(trace_func, trace_context, 4);
    trace_func(trace_context, "encl_number: 0x%x", attributes_elements);

    /* Get the maximum_transfer_bytes element of terminator_port_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_sata_drive_info_t, "slot_number", &attributes_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &attributes_elements, sizeof(fbe_bool_t));
    fbe_trace_indent(trace_func, trace_context, 4);
    trace_func(trace_context, "slot_number: 0x%x", attributes_elements);

    trace_func(trace_context, "\n");

    /* Get the maximum_sg_entries element of terminator_port_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_sata_drive_info_t, "backend_number", &attributes_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &attributes_elements, sizeof(fbe_u32_t));
    fbe_trace_indent(trace_func, trace_context, TERMINATOR_DRIVE_INDENT + 4);
    trace_func(trace_context, "backend_number: 0x%x", attributes_elements);

    /* Get the maximum_sg_entries element of terminator_port_t structure */
    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(terminator_sata_drive_info_t, "miniport_sas_device_table_index", &attributes_element_offset);
    FBE_READ_MEMORY((fbe_u64_t)attributes_ptr + attributes_element_offset, &attributes_elements, sizeof(fbe_u32_t));
    fbe_trace_indent(trace_func, trace_context, 4);
    trace_func(trace_context, "miniport_sas_device_table_index: 0x%x", attributes_elements);

    trace_func(trace_context, "\n");

    return status;
}

/*!**************************************************************
 * fbe_base_component_drive_attributes_debug_trace()
 ****************************************************************
 * @brief
 *  Display information all drive attributes.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param drive_type - enum value of drive type.
 * @param drive_ptr - offset value of drive attributes element.
 * 
 * 
 * @return fbe_status_t
 *
 * @author
 *  27- May- 2011 - Created. Hari Singh Chauhan
 *
 ****************************************************************/
fbe_status_t fbe_base_component_drive_attributes_debug_trace(fbe_trace_func_t trace_func,
                                                             fbe_trace_context_t trace_context,
                                                             fbe_u32_t drive_type,
                                                             void *drive_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t attributes_offset = 0;
    fbe_u64_t attributes_ptr = 0;

    FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(base_component_t, "attributes", &attributes_offset);
    FBE_READ_MEMORY((fbe_u64_t)drive_ptr + attributes_offset, &attributes_ptr, sizeof(fbe_u64_t));

    switch(drive_type)
    {
        case FBE_DRIVE_TYPE_FIBRE:
            fbe_terminator_fc_drive_attributes_debug_trace(trace_func, trace_context, attributes_ptr);
            break;
        case FBE_DRIVE_TYPE_SAS:
        case FBE_DRIVE_TYPE_SAS_NL:
        case FBE_DRIVE_TYPE_SAS_FLASH_HE:
        case FBE_DRIVE_TYPE_SAS_FLASH_ME:
        case FBE_DRIVE_TYPE_SAS_FLASH_LE:
        case FBE_DRIVE_TYPE_SAS_FLASH_RI:
            fbe_terminator_sas_drive_attributes_debug_trace(trace_func, trace_context, attributes_ptr);
            break;
        case FBE_DRIVE_TYPE_SATA:
        case FBE_DRIVE_TYPE_SATA_FLASH_HE:
        case FBE_DRIVE_TYPE_SATA_PADDLECARD:
            fbe_terminator_sata_drive_attributes_debug_trace(trace_func, trace_context, attributes_ptr);
            break;
        default:
            trace_func(trace_context, "Unlnown drive type\n");
            return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*******************************************************************
 * @var terminator_drive_field_info
 *********************************************************************
 * @brief Information about each of the fields of terminator drive.
 *
 *********************************************************************/
fbe_debug_field_info_t terminator_drive_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("backend_number", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("encl_number", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("slot_number", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("capacity", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("block_size", fbe_block_size_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("drive_handle", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("reset_count", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("drive_debug_flags", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("error_count", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("maximum_transfer_bytes", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("journal_num_records", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("journal_blocks_allocated", fbe_block_count_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("journal_queue_head", fbe_queue_head_t, FBE_FALSE, "0x%x",
                                    fbe_terminator_display_journal_records),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_raid_group_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base config.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(terminator_drive_t,
                              terminator_drive_struct_info,
                              &terminator_drive_field_info[0]);


/*!*******************************************************************
 * @var journal_record_field_info
 *********************************************************************
 * @brief Information about each of the fields of terminator drive.
 *
 *********************************************************************/
fbe_debug_field_info_t journal_record_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("record_type", terminator_journal_record_type_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("lba", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("block_count", fbe_block_count_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("block_size", fbe_block_size_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("data_ptr", fbe_u8_t, FBE_FALSE, "0x%x", 
                                    fbe_debug_display_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO("data_size", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("is_compressed", fbe_bool_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var journal_record_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        journal record.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(journal_record_t,
                              journal_record_struct_info,
                              &journal_record_field_info[0]);

/*!**************************************************************
 * fbe_terminator_drive_debug_trace()
 ****************************************************************
 * @brief
 *  Display all the trace information about the raid group object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param terminator_drive_p - Ptr to object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 * @param spaces_to_indent - Spaces to indent this display.
 *
 * @return - FBE_STATUS_OK  
 *
 ****************************************************************/

fbe_status_t fbe_terminator_drive_debug_trace(const fbe_u8_t * module_name,
                                              fbe_dbgext_ptr terminator_drive_p,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_debug_field_info_t *field_info_p,
                                              fbe_u32_t overall_spaces_to_indent,
                                              fbe_bool_t b_display_journal)
{
    fbe_status_t status;
    fbe_u32_t spaces_to_indent = overall_spaces_to_indent + 2;

    if (field_info_p != NULL)
    {
        /* If field info is provided, the ptr passed in is a ptr to the base structure. 
         * We need to add the offset to get to the actual raid group ptr.
         */
        terminator_drive_p += field_info_p->offset;
    }
    
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, terminator_drive_p,
                                         &terminator_drive_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent + 2);
    trace_func(trace_context, "\n");
    return status;
} 
/**************************************
 * end fbe_terminator_drive_debug_trace()
 **************************************/
/*!**************************************************************
 * fbe_terminator_display_journal_records()
 ****************************************************************
 * @brief
 *  Display the raid siots structure.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param queue_p - ptr to journal queue to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  5/9/2012 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_terminator_display_journal_records (const fbe_u8_t * module_name,
                                                            fbe_dbgext_ptr base_ptr,
                                                            fbe_trace_func_t trace_func,
                                                            fbe_trace_context_t trace_context,
                                                            fbe_debug_field_info_t *field_info_p,
                                                            fbe_u32_t overall_spaces_to_indent)
{
    fbe_status_t status            = FBE_STATUS_GENERIC_FAILURE;
    fbe_dbgext_ptr queue_p         = 0;
    fbe_u64_t queue_head_ptr       = 0;
    fbe_u64_t queue_head_p         = 0;
    fbe_u64_t queue_element_p      = 0;
    fbe_u64_t next_queue_element_p = 0;
    fbe_u32_t ptr_size             = 0;
    fbe_u32_t spaces_to_indent     = overall_spaces_to_indent + 2;

    if (field_info_p != NULL)
    {
        /* If field info is provided, the ptr passed in is a ptr to the base structure. 
         * We need to add the offset to get to the actual raid group ptr.
         */
        queue_p = base_ptr + field_info_p->offset;
    }

    FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr_size);

    queue_head_ptr = queue_p;

    if (queue_head_ptr == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    FBE_READ_MEMORY(queue_head_ptr, &queue_head_p, ptr_size);
    queue_element_p = queue_head_p;

    if ((queue_element_p == queue_head_ptr) || (queue_element_p == NULL))
    {
        return FBE_STATUS_OK;
    }
    trace_func(trace_context, "\n");
    fbe_trace_indent(trace_func, trace_context, overall_spaces_to_indent);
    /* Now that we have the head of the list, iterate over it, displaying each siots.
     * We finish when we reach the end of the list (head) or null. 
     */
    while ((queue_element_p != queue_head_ptr) && (queue_element_p != NULL))
    {
        fbe_u64_t journal_rec_p = queue_element_p;
        if (FBE_CHECK_CONTROL_C())
        {
            return FBE_STATUS_CANCELED;
        }
        status = fbe_debug_display_structure(module_name, trace_func, trace_context, journal_rec_p,
                                         &journal_record_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent + 2);
        trace_func(trace_context, "\n");
        fbe_trace_indent(trace_func, trace_context, overall_spaces_to_indent);
        FBE_READ_MEMORY(queue_element_p, &next_queue_element_p, ptr_size);
        queue_element_p = next_queue_element_p;
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_terminator_display_journal_records()
 ******************************************/
/*************************
 * end file fbe_terminator_debug.c
 *************************/
