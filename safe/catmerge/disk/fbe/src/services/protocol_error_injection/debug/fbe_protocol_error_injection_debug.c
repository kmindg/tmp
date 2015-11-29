/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_protocol_error_injection_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for the protocol error injection service.
 *
 * @author
 *  10/28/2011 - Created. Omer Miranda
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe/fbe_transport.h"
#include "fbe_block_transport.h"
#include "fbe_transport_debug.h"
#include "fbe/fbe_protocol_error_injection_service.h"
#include "fbe_protocol_error_injection_debug.h"
#include "fbe_protocol_error_injection_service_debug.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!*******************************************************************
 * @var fbe_protocol_error_injection_debug_queue_info_t
 *********************************************************************
 * @brief This structure holds info used to display an 
 *        queue using the "queue_element" field of the 
 *        protocol error injection error element.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_QUEUE_INFO(fbe_protocol_error_injection_error_element_t, "queue_element", 
                             FBE_TRUE, /* queue element is the first field in the object. */
                             fbe_protocol_error_injection_debug_queue_info,
                             fbe_protocol_error_injection_debug_display_error_element);


/*!*******************************************************************
 * @var fbe_protocol_error_injection_debug_service_field_info
 *********************************************************************
 * @brief Information about each of the fields of protocol_error_injection service.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_protocol_error_injection_debug_service_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("base_service", fbe_base_service_t, FBE_TRUE, "0x%x",
                                    fbe_protocol_base_service_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_QUEUE("protocol_error_injection_error_queue", fbe_queue_head_t, FBE_FALSE, 
                                       &fbe_protocol_error_injection_debug_queue_info),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_protocol_error_injection_debug_service_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        protocol_error_injection service structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_protocol_error_injection_service_t,
                              fbe_protocol_error_injection_debug_service_struct_info,
                              &fbe_protocol_error_injection_debug_service_field_info[0]); 


/*!**************************************************************
 * fbe_protocol_error_injection_debug()
 ****************************************************************
 * @brief
 *  Display info on protocol error injection structures..
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param protocol_error_service_ptr -
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * 
 * @return fbe_status_t
 *
 * @author
 *  10/28/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_protocol_error_injection_debug(const fbe_u8_t * module_name,
                                                fbe_dbgext_ptr protocol_error_service_ptr,
                                                fbe_trace_func_t trace_func,
                                                fbe_trace_context_t trace_context)
{
    fbe_u32_t spaces_to_indent = 4;
    trace_func(trace_context, "Protocol error injection service info: \n");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 2);
    fbe_debug_display_structure(module_name, trace_func, trace_context, protocol_error_service_ptr,
                                &fbe_protocol_error_injection_debug_service_struct_info,
                                4 /* fields per line */,
                                spaces_to_indent);
    trace_func(trace_context, "\n");
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_protocol_error_injection_debug()
 ******************************************/
/*!*******************************************************************
 * @var fbe_protcol_base_service_field_info
 *********************************************************************
 * @brief Information about each of the fields of base service
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_protcol_base_service_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("fbe_base", fbe_base_t, FBE_TRUE, "0x%x",
                                    fbe_protocol_base_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("service_id", fbe_service_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("trace_level", fbe_trace_level_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("initialized", fbe_bool_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_protocol_base_service_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base service.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_base_service_t,
                              fbe_protocol_base_service_struct_info,
                              &fbe_protcol_base_service_field_info[0]);

/*!**************************************************************
 * fbe_protocol_base_service_debug_trace()
 ****************************************************************
 * @brief
 *  Display info on protocol base service element.
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
 *  28- Oct- 2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_protocol_base_service_debug_trace(const fbe_u8_t * module_name,
                                                   fbe_dbgext_ptr base_ptr,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_debug_field_info_t *field_info_p,
                                                   fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u64_t base_service_p = base_ptr + field_info_p->offset;

    trace_func(NULL,"\n");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "%s: ", field_info_p->name);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, base_service_p + field_info_p->offset,
                                         &fbe_protocol_base_service_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent);

    return status;
}
/******************************************
 * end fbe_protocol_base_service_debug_trace()
 ******************************************/

/*!*******************************************************************
 * @var fbe_protocol_base_field_info
 *********************************************************************
 * @brief Information about each of the fields of base 
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_protocol_base_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("magic_number", fbe_u64_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_protocol_base_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base element.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_base_t,
                              fbe_protocol_base_struct_info,
                              &fbe_protocol_base_field_info[0]);


/*!**************************************************************
 * fbe_protocol_base_debug_trace()
 ****************************************************************
 * @brief
 *  Display info on protocol base element.
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
 *  28- Oct- 2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_protocol_base_debug_trace(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr base_ptr,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context,
                                           fbe_debug_field_info_t *field_info_p,
                                           fbe_u32_t spaces_to_indent)
{

    fbe_status_t status = FBE_STATUS_OK;

    trace_func(NULL,"\n");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 4);
    trace_func(trace_context, "%s: ", field_info_p->name);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, base_ptr + field_info_p->offset,
                                         &fbe_protocol_base_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent);
    return status;
}
/******************************************
 * end fbe_protocol_base_debug_trace()
 ******************************************/

/*!*******************************************************************
 * @var fbe_protocol_error_record_field_info
 *********************************************************************
 * @brief Information about each of the fields of protocol error record
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_protocol_error_record_field_info[] =
{

    FBE_DEBUG_DECLARE_FIELD_INFO("object_id", fbe_object_id_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("lba_start", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("lba_end", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("protocol_error_injection_error_type", fbe_protocol_error_injection_error_type_t, FBE_FALSE, "0x%x",
                                    fbe_protocol_error_injection_error_info_debug),
    FBE_DEBUG_DECLARE_FIELD_INFO("num_of_times_to_insert", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("max_times_hit_timestamp", fbe_time_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("secs_to_reactivate", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("num_of_times_to_reactivate", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("times_inserted", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("times_reset", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("frequency", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("b_active",fbe_bool_t,FBE_FALSE,"0x%x",fbe_protocol_error_injection_errors_active_info_debug), 
    FBE_DEBUG_DECLARE_FIELD_INFO("reassign_count", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("b_test_for_media_error", fbe_bool_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("b_inc_error_on_all_pos", fbe_bool_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("tag", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("skip_blks", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_protocol_error_record_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        fbe_protocol_error_injection_error_record_t structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_protocol_error_injection_error_record_t,
                              fbe_protocol_error_record_struct_info,
                              &fbe_protocol_error_record_field_info[0]);

/*!**************************************************************
 * fbe_protocol_error_injection_error_record_trace()
 ****************************************************************
 * @brief
 *  Display info on protocol error record.
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
 *  28- Oct- 2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_protocol_error_injection_error_record_trace(const fbe_u8_t * module_name,
                                                             fbe_dbgext_ptr error_record_ptr,
                                                             fbe_trace_func_t trace_func,
                                                             fbe_trace_context_t trace_context,
                                                             fbe_debug_field_info_t *field_info_p,
                                                             fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;

    trace_func(NULL,"\n");
    fbe_trace_indent(trace_func, trace_context,8 /* spaces_to_indent */);
    trace_func(trace_context, "%s: ", field_info_p->name);

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, error_record_ptr + field_info_p->offset,
                                         &fbe_protocol_error_record_struct_info,
                                         4 /* fields per line */,
                                         8 /*spaces_to_indent */);
    return status;
}
/******************************************
 * end fbe_protocol_error_injection_error_record_trace()
 ******************************************/

/*!*******************************************************************
 * @var fbe_protocol_error_injection_debug_error_element_info
 *********************************************************************
 * @brief Information about each of the fields of protocol error element
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_protocol_error_injection_debug_error_element_info[] =
{

    FBE_DEBUG_DECLARE_FIELD_INFO("magic_number", fbe_protocol_error_injection_magic_number_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("protocol_error_injection_error_record", fbe_protocol_error_injection_error_record_t,
                                    FBE_FALSE, "0x%x",fbe_protocol_error_injection_error_record_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_protocol_error_injection_debug_error_element_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        fbe_protocol_error_injection_error_element_t structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_protocol_error_injection_error_element_t,
                              fbe_protocol_error_injection_debug_error_element_struct_info,
                              &fbe_protocol_error_injection_debug_error_element_info[0]); 

/*!**************************************************************
 * fbe_protocol_error_injection_debug_display_error_element()
 ****************************************************************
 * @brief
 *  Display info on protocol_error_injection error elements.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  10/28/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_protocol_error_injection_debug_display_error_element(const fbe_u8_t * module_name,
                                                                      fbe_dbgext_ptr error_element_ptr,
                                                                      fbe_trace_func_t trace_func,
                                                                      fbe_trace_context_t trace_context,
                                                                      fbe_u32_t spaces_to_indent)
{
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context,
	       "protocol_error_injection_error_element_ptr:0x%llx\n",
	       (unsigned long long)error_element_ptr);
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    fbe_debug_display_structure(module_name, trace_func, trace_context, error_element_ptr,
                                &fbe_protocol_error_injection_debug_error_element_struct_info,
                                4 /* fields per line */,
                                spaces_to_indent +4);
    trace_func(trace_context, "\n");
    return FBE_STATUS_OK;
}
/*******************************************************
 * end fbe_protocol_error_injection_debug_display_error_element()
 **********************************************************/

/*!**************************************************************
 * fbe_protocol_error_injection_scsi_command_debug_trace()
 ****************************************************************
 * @brief
 *  Display info about protocol_error_injection scsi command.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  10/31/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_protocol_error_injection_scsi_command_debug_trace(const fbe_u8_t * module_name,
                                                                   fbe_dbgext_ptr scsi_command_ptr,
                                                                   fbe_trace_func_t trace_func,
                                                                   fbe_trace_context_t trace_context,
                                                                   fbe_debug_field_info_t *field_info_p,
                                                                   fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u8_t scsi_command[MAX_CMDS];
    fbe_u8_t i;

    trace_func(NULL,"\n");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "%s: ", field_info_p->name);

    FBE_READ_MEMORY(scsi_command_ptr + field_info_p->offset, &scsi_command, MAX_CMDS);
    for(i = 0; i< MAX_CMDS; i++)
    {
        trace_func(trace_context, "%02x ",scsi_command[i]);
    }

    return status;
}
/*******************************************************
 * end fbe_protocol_error_injection_scsi_command_debug_trace()
 **********************************************************/

/*!*******************************************************************
 * @var fbe_protocol_error_injection_debug_scsi_error_info
 *********************************************************************
 * @brief Information about each of the fields of protocol error type SCSI
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_protocol_error_injection_debug_scsi_error_info[] =
{

    FBE_DEBUG_DECLARE_FIELD_INFO_FN("scsi_command", fbe_u8_t, FBE_TRUE, "0x%x",
                                    fbe_protocol_error_injection_scsi_command_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO("scsi_sense_key", fbe_scsi_sense_key_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("scsi_additional_sense_code", fbe_scsi_additional_sense_code_t,FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("scsi_additional_sense_code_qualifier", fbe_scsi_additional_sense_code_qualifier_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("port_status", fbe_port_request_status_t, FBE_FALSE, "0x%x",
                                 fbe_protocol_error_injection_port_status_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_protocol_error_injection_debug_scsi_error_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        fbe_protocol_error_injection_scsi_error_t structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_protocol_error_injection_scsi_error_t,
                              fbe_protocol_error_injection_debug_scsi_error_struct_info,
                              &fbe_protocol_error_injection_debug_scsi_error_info[0]);

/*!*******************************************************************
 * @var fbe_protocol_error_injection_debug_fis_error_info
 *********************************************************************
 * @brief Information about each of the fields of protocol error type SATA/FIS
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_protocol_error_injection_debug_fis_error_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("sata_command", fbe_sata_command_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("response_buffer", fbe_u8_t, FBE_FALSE, "0x%x",
                                    fbe_protocol_error_injection_response_buffer_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("port_status", fbe_port_request_status_t, FBE_FALSE, "0x%x",
                                 fbe_protocol_error_injection_port_status_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_protocol_error_injection_debug_fis_error_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        fbe_protocol_error_injection_sata_error_t structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_protocol_error_injection_sata_error_t,
                              fbe_protocol_error_injection_debug_fis_error_struct_info,
                              &fbe_protocol_error_injection_debug_fis_error_info[0]);

/*!**************************************************************
 * fbe_protocol_error_injection_response_buffer_trace()
 ****************************************************************
 * @brief
 *  Display info on protocol_error_injection sata error response buffer.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  10/31/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_protocol_error_injection_response_buffer_trace(const fbe_u8_t * module_name,
                                                                fbe_dbgext_ptr buffer_ptr,
                                                                fbe_trace_func_t trace_func,
                                                                fbe_trace_context_t trace_context,
                                                                fbe_debug_field_info_t *field_info_p,
                                                                fbe_u32_t spaces_to_indent)
{
    fbe_u8_t response_buffer[FBE_PAYLOAD_FIS_RESPONSE_BUFFER_SIZE];
    fbe_u8_t i;

    FBE_READ_MEMORY(buffer_ptr + field_info_p->offset, &response_buffer, FBE_PAYLOAD_FIS_RESPONSE_BUFFER_SIZE);
    trace_func(trace_context, "%s: ", field_info_p->name);
    for(i = 0; i< FBE_PAYLOAD_FIS_RESPONSE_BUFFER_SIZE; i++)
    {
        trace_func(trace_context, "%02x ", response_buffer[i]);
    }

    return FBE_STATUS_OK;
}
/******************************************************
 * end fbe_protocol_error_injection_response_buffer_trace()
 *******************************************************/

/*!*******************************************************************
 * @var fbe_protocol_error_injection_debug_port_error_info
 *********************************************************************
 * @brief Information about each of the fields of protocol error type PORT
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_protocol_error_injection_debug_port_error_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("port_status", fbe_port_request_status_t, FBE_TRUE, "0x%x",
                                    fbe_protocol_error_injection_port_status_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_protocol_error_injection_debug_port_error_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        fbe_protocol_error_injection_port_error_t structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_protocol_error_injection_port_error_t,
                              fbe_protocol_error_injection_debug_port_error_struct_info,
                              &fbe_protocol_error_injection_debug_port_error_info[0]);

/*!**************************************************************
 * fbe_protocol_error_injection_port_status_trace()
 ****************************************************************
 * @brief
 *  Display info on protocol_error_injection port error port status
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  11/1/11 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_protocol_error_injection_port_status_trace(const fbe_u8_t * module_name,
                                                            fbe_dbgext_ptr port_status_ptr,
                                                            fbe_trace_func_t trace_func,
                                                            fbe_trace_context_t trace_context,
                                                            fbe_debug_field_info_t *field_info_p,
                                                            fbe_u32_t spaces_to_indent)
{
    fbe_port_request_status_t port_status;

    FBE_READ_MEMORY(port_status_ptr + field_info_p->offset, &port_status, sizeof(fbe_port_request_status_t));
    trace_func(trace_context, "%s: ", field_info_p->name);
    switch(port_status)
    {
        case FBE_PORT_REQUEST_STATUS_SUCCESS:
            trace_func(trace_context, "%s", "SUCCESS");
            break;
        case FBE_PORT_REQUEST_STATUS_INVALID_REQUEST:
            trace_func(trace_context, "%s", "INVALID_REQUEST");
            break;
        case FBE_PORT_REQUEST_STATUS_INSUFFICIENT_RESOURCES:
            trace_func(trace_context, "%s", "INSUFFICIENT_RESOURCES");
            break;
        case FBE_PORT_REQUEST_STATUS_DEVICE_NOT_LOGGED_IN:
            trace_func(trace_context, "%s", "DEVICE_NOT_LOGGED_IN");
            break;
        case FBE_PORT_REQUEST_STATUS_BUSY:
            trace_func(trace_context, "%s", "BUSY");
            break;
        case FBE_PORT_REQUEST_STATUS_PROTOCOL_ERROR:
            trace_func(trace_context, "%s", "PROTOCOL_ERROR");
            break;
        case FBE_PORT_REQUEST_STATUS_ABORT_TIMEOUT:
            trace_func(trace_context, "%s", "ABORT_TIMEOUT");
            break;
        case FBE_PORT_REQUEST_STATUS_SELECTION_TIMEOUT:
            trace_func(trace_context, "%s", "SELECTION_TIMEOUT");
            break;
        case FBE_PORT_REQUEST_STATUS_TRANSIENT_ERROR:
            trace_func(trace_context, "%s", "TRANSIENT_ERROR");
            break;
        case FBE_PORT_REQUEST_STATUS_DATA_OVERRUN:
            trace_func(trace_context, "%s", "DATA_OVERRUN");
            break;
        case FBE_PORT_REQUEST_STATUS_DATA_UNDERRUN:
            trace_func(trace_context, "%s", "DATA_UNDERRUN");
            break;
        case FBE_PORT_REQUEST_STATUS_ABORTED_BY_SOFTWARE:
            trace_func(trace_context, "%s", "ABORTED_BY_SOFTWARE");
            break;
        case FBE_PORT_REQUEST_STATUS_ABORTED_BY_DEVICE:
            trace_func(trace_context, "%s", "ABORTED_BY_DEVICE");
            break;
        case FBE_PORT_REQUEST_STATUS_SATA_NCQ_ERROR:
            trace_func(trace_context, "%s", "SATA_NCQ_ERROR");
            break;
        case FBE_PORT_REQUEST_STATUS_REJECTED_NCQ_MODE:
            trace_func(trace_context, "%s", "REJECTED_NCQ_MODE");
            break;
        case FBE_PORT_REQUEST_STATUS_INCIDENTAL_ABORT:
            trace_func(trace_context, "%s", "INCIDENTAL_ABORT");
            break;
        case FBE_PORT_REQUEST_STATUS_ENCRYPTION_NOT_ENABLED:
          trace_func(trace_context, "ENCRYPTION_NOT_ENABLED ");
          break;
        case FBE_PORT_REQUEST_STATUS_ENCRYPTION_BAD_HANDLE:
          trace_func(trace_context, "ENCRYPTION_BAD_HANDLE ");
          break;
        case FBE_PORT_REQUEST_STATUS_ENCRYPTION_KEY_WRAP_ERROR:
          trace_func(trace_context, "ENCRYPTION_KEY_WRAP_ERROR ");
          break;
        case FBE_PORT_REQUEST_STATUS_ERROR:
            trace_func(trace_context, "%s", "ERROR");
            break;
    }
    return FBE_STATUS_OK;
}
/******************************************************
 * end fbe_protocol_error_injection_port_status_trace()
 *******************************************************/

/*!**************************************************************
 * fbe_protocol_error_injection_error_info_debug()
 ****************************************************************
 * @brief
 *  Display info on protocol_error_injection error type.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  10/31/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_protocol_error_injection_error_info_debug(const fbe_u8_t * module_name,
                                                           fbe_dbgext_ptr error_type_ptr,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_debug_field_info_t *field_info_p,
                                                           fbe_u32_t spaces_to_indent)
{
    fbe_protocol_error_injection_error_type_t error_type;
    fbe_u32_t offset;

    trace_func(NULL,"\n");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "%s: ", field_info_p->name);

    FBE_READ_MEMORY(error_type_ptr + field_info_p->offset, &error_type, sizeof(fbe_protocol_error_injection_error_type_t));

    FBE_GET_FIELD_OFFSET(module_name, fbe_protocol_error_injection_error_record_t, "protocol_error_injection_error", &offset); 

    switch(error_type)
    {
        case FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI:
            trace_func(trace_context, "%s", "ERROR_TYPE_SCSI");
            fbe_debug_display_structure(module_name, trace_func, trace_context, error_type_ptr + offset,
                                &fbe_protocol_error_injection_debug_scsi_error_struct_info,
                                4 /* fields per line */,
                                spaces_to_indent +4);
            break;
        case FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_FIS:
            trace_func(trace_context, "%s", "ERROR_TYPE_FIS");
            fbe_debug_display_structure(module_name, trace_func, trace_context, error_type_ptr + offset,
                                &fbe_protocol_error_injection_debug_fis_error_struct_info,
                                4 /* fields per line */,
                                spaces_to_indent +4);
            break;
        case FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_PORT:
            trace_func(trace_context, "%s", "ERROR_TYPE_PORT");
            fbe_debug_display_structure(module_name, trace_func, trace_context, error_type_ptr + offset,
                                &fbe_protocol_error_injection_debug_port_error_struct_info,
                                4 /* fields per line */,
                                spaces_to_indent +4);
            break;
        case FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_INVALID:
            trace_func(trace_context, "%s", "ERROR_TYPE_INVALID");
            break;
    }

    return FBE_STATUS_OK;
}
/******************************************************
 * end fbe_protocol_error_injection_error_info_debug()
 *******************************************************/

/*!**************************************************************
 * fbe_protocol_error_injection_errors_active_info_debug()
 ****************************************************************
 * @brief
 *  Display info on protocol_error_injection active errors.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  10/31/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_protocol_error_injection_errors_active_info_debug(const fbe_u8_t * module_name,
                                                                   fbe_dbgext_ptr errors_active_ptr,
                                                                   fbe_trace_func_t trace_func,
                                                                   fbe_trace_context_t trace_context,
                                                                   fbe_debug_field_info_t *field_info_p,
                                                                   fbe_u32_t spaces_to_indent)
{
    fbe_u32_t offset;
    fbe_bool_t media_error;
    fbe_bool_t b_active[PROTOCOL_ERROR_INJECTION_MAX_ERRORS];
    fbe_u32_t i;

    FBE_GET_FIELD_OFFSET(module_name, fbe_protocol_error_injection_error_record_t, "b_test_for_media_error", &offset);
    FBE_READ_MEMORY(errors_active_ptr + offset, &media_error, sizeof(fbe_bool_t));
    if(media_error == FBE_TRUE)
    {
        trace_func(NULL,"\n");
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(trace_context, "%s:\n", field_info_p->name);
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        FBE_READ_MEMORY(errors_active_ptr + field_info_p->offset, &b_active, sizeof(fbe_bool_t)* PROTOCOL_ERROR_INJECTION_MAX_ERRORS);
        trace_func(trace_context, "(0-31):");
        for(i = 0;i < PROTOCOL_ERROR_INJECTION_MAX_ERRORS; i++)
        {
            trace_func(trace_context, "%d ", b_active[i]);
            if(i == 32)
            {
                trace_func(NULL,"\n");
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
                trace_func(trace_context, "(32-63):");
            }
            else if(i == 64)
            {
                trace_func(NULL,"\n");
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
                trace_func(trace_context, "(64-95):");
            }
            else if(i == 96)
            {
                trace_func(NULL,"\n");
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
                trace_func(trace_context, "(96-127):");
            }
            else if(i == 128)
            {
                trace_func(NULL,"\n");
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
                trace_func(trace_context, "(128-159):");
            }
            else if(i == 160)
            {
                trace_func(NULL,"\n");
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
                trace_func(trace_context, "(160-191):");
            }
            else if(i == 192)
            {
                trace_func(NULL,"\n");
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
                trace_func(trace_context, "(192-223):");
            }
            else if(i == 224)
            {
                trace_func(NULL,"\n");
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
                trace_func(trace_context, "(224-255):");
            }
        }
    }

    return FBE_STATUS_OK;
}
/******************************************************
 * end fbe_protocol_error_injection_errors_active_info_debug()
 *******************************************************/

/***********************************************
 * end file fbe_protocol_error_injection_debug.c
 ***********************************************/
