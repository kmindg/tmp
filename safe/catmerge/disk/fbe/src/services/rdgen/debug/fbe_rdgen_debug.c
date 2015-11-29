/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_rdgen_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for the raid library.
 *
 * @author
 *  11/24/2009 - Created. Rob Foley
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
#include "fbe_rdgen_private.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/
 fbe_status_t fbe_rdgen_debug_display_ts(const fbe_u8_t * module_name,
                                            fbe_dbgext_ptr rdgen_ts_ptr,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context,
                                            fbe_u32_t spaces_to_indent);
 fbe_status_t fbe_rdgen_debug_display_rdgen_status(const fbe_u8_t * module_name,
                                                  fbe_dbgext_ptr base_ptr,
                                                  fbe_trace_func_t trace_func,
                                                  fbe_trace_context_t trace_context,
                                                  fbe_debug_field_info_t *field_info_p,
                                                  fbe_u32_t spaces_to_indent);
fbe_status_t fbe_rdgen_debug_display_object(const fbe_u8_t * module_name,
                                            fbe_dbgext_ptr object_ptr,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context,
                                            fbe_u32_t spaces_to_indent);
fbe_status_t fbe_rdgen_debug_display_request(const fbe_u8_t * module_name,
                                             fbe_dbgext_ptr request_ptr,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_u32_t spaces_to_indent);

fbe_status_t fbe_rdgen_debug_display_specification(const fbe_u8_t * module_name,
                                                   fbe_dbgext_ptr base_ptr,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_debug_field_info_t *field_info_p,
                                                   fbe_u32_t spaces_to_indent);
fbe_status_t fbe_rdgen_ts_debug_trace_packet(const fbe_u8_t * module_name,
                                             fbe_dbgext_ptr base_ptr,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_debug_field_info_t *field_info_p,
                                             fbe_u32_t spaces_to_indent);

fbe_status_t fbe_rdgen_debug_print_io_interface(const fbe_u8_t * module_name,
                                                fbe_dbgext_ptr base_ptr,
                                                fbe_trace_func_t trace_func,
                                                fbe_trace_context_t trace_context,
                                                fbe_debug_field_info_t *field_info_p,
                                                fbe_u32_t spaces_to_indent);

fbe_status_t fbe_rdgen_debug_print_affinity(const fbe_u8_t * module_name,
                                            fbe_dbgext_ptr base_ptr,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context,
                                            fbe_debug_field_info_t *field_info_p,
                                            fbe_u32_t spaces_to_indent);
fbe_status_t fbe_rdgen_debug_print_device_name(const fbe_u8_t * module_name,
                                               fbe_dbgext_ptr base_ptr,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_debug_field_info_t *field_info_p,
                                               fbe_u32_t spaces_to_indent);

/*!*******************************************************************
 * @var fbe_rdgen_debug_trace_display_queues
 *********************************************************************
 * @brief TRUE to display the object ts queues.
 *
 *********************************************************************/
static fbe_bool_t fbe_rdgen_debug_trace_display_queues = FBE_TRUE;

/*!*******************************************************************
 * @var fbe_rdgen_debug_active_ts_queue_info_t
 *********************************************************************
 * @brief This structure holds info used to display an rdgen object
 *        queue using the "queue_element" field of the rdgen object.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_QUEUE_INFO(fbe_rdgen_object_t, "queue_element", 
                             FBE_TRUE, /* queue element is the first field in the object. */
                             fbe_rdgen_debug_active_object_queue_info,
                             fbe_rdgen_debug_display_object);

/*!*******************************************************************
 * @var fbe_rdgen_debug_active_request_queue_info_t
 *********************************************************************
 * @brief This structure holds info used to display an rdgen request
 *        queue using the "queue_element" field of the rdgen request.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_QUEUE_INFO(fbe_rdgen_request_t, "queue_element", 
                             FBE_TRUE, /* queue element is the first field in the object. */
                             fbe_rdgen_debug_active_request_queue_info,
                             fbe_rdgen_debug_display_request);

/*!*******************************************************************
 * @var fbe_rdgen_debug_service_field_info
 *********************************************************************
 * @brief Information about each of the fields of rdgen service.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_rdgen_debug_service_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("num_objects", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("flags", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("num_requests", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("num_ts", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("num_threads", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO_QUEUE("active_object_head", fbe_queue_head_t, FBE_FALSE, 
                                       &fbe_rdgen_debug_active_object_queue_info),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};
/*!*******************************************************************
 * @var fbe_rdgen_debug_service_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        rdgen service structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_rdgen_service_t,
                              fbe_rdgen_debug_service_struct_info,
                              &fbe_rdgen_debug_service_field_info[0]);


/*!*******************************************************************
 * @var fbe_rdgen_debug_service_request_only_field_info
 *********************************************************************
 * @brief Structure to help display just request queue.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_rdgen_debug_service_request_only_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO_QUEUE("active_request_head", fbe_queue_head_t, FBE_FALSE, 
                                       &fbe_rdgen_debug_active_request_queue_info),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};
    
/*!*******************************************************************
 * @var fbe_rdgen_debug_service_request_only_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        rdgen service structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_rdgen_service_t,
                              fbe_rdgen_debug_svc_req_only_struct_info,
                              &fbe_rdgen_debug_service_request_only_field_info[0]);


/*!*******************************************************************
 * @var fbe_rdgen_debug_active_ts_queue_info_t
 *********************************************************************
 * @brief This structure holds info used to display an rdgen ts queue
 *        using the "queue_element".
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_QUEUE_INFO(fbe_rdgen_ts_t, "queue_element", 
                             FBE_TRUE, /* queue element is the first field in the object. */
                             fbe_rdgen_debug_active_ts_queue_info,
                             fbe_rdgen_debug_display_ts);

/*!*******************************************************************
 * @var fbe_rdgen_debug_object_field_info
 *********************************************************************
 * @brief Information about each of the fields of rdgen object.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_rdgen_debug_object_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("object_id", fbe_object_id_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("package_id", fbe_package_id_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("active_ts_count", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("active_request_count", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("num_passes", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("num_ios", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("capacity", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("stripe_size", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("stack_size", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("device_name", fbe_char_t, FBE_FALSE, "%d",
                                    fbe_rdgen_debug_print_device_name),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("device_p", fbe_rdgen_object_t, FBE_FALSE, "0x%x", 
                                    fbe_debug_display_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("file_p", fbe_rdgen_object_t, FBE_FALSE, "0x%x", 
                                    fbe_debug_display_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};
/*!*******************************************************************
 * @var fbe_rdgen_debug_object_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        rdgen objectstructure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_rdgen_object_t,
                              fbe_rdgen_debug_object_struct_info,
                              &fbe_rdgen_debug_object_field_info[0]);


fbe_debug_field_info_t fbe_rdgen_debug_object_active_ts_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_QUEUE("active_ts_head", fbe_queue_head_t, FBE_FALSE, 
                                       &fbe_rdgen_debug_active_ts_queue_info),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_rdgen_debug_object_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        rdgen objectstructure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_rdgen_object_t,
                              fbe_rdgen_debug_object_active_ts_struct_info,
                              &fbe_rdgen_debug_object_active_ts_field_info[0]);

/*!*******************************************************************
 * @var fbe_rdgen_debug_request_field_info
 *********************************************************************
 * @brief Information about each of the fields of rdgen request.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_rdgen_debug_request_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("active_ts_count", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("b_stop", fbe_bool_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("caterpillar_lba", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("pass_count", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("io_count", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("err_count", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("aborted_err_count", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("media_err_count", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("io_failure_err_count", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("invalid_request_err_count", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("inv_blocks_count", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("bad_crc_blocks_count", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("status", fbe_rdgen_status_t, FBE_FALSE, "0x%x", 
                                    fbe_rdgen_debug_display_rdgen_status),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("start_time", fbe_time_t, FBE_FALSE, "0x%x",
                                    fbe_debug_trace_time),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("packet_p", fbe_packet_t, FBE_FALSE, "0x%x", 
                                    fbe_debug_display_field_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("specification", fbe_rdgen_io_specification_t, FBE_FALSE, "0x%x", 
                                    fbe_rdgen_debug_display_specification),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_rdgen_debug_request_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        rdgen request structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_rdgen_request_t,
                              fbe_rdgen_debug_request_struct_info,
                              &fbe_rdgen_debug_request_field_info[0]);

/*!*******************************************************************
 * @var fbe_rdgen_debug_specification_field_info
 *********************************************************************
 * @brief Information about each of the fields of rdgen specification.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_rdgen_debug_specification_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("operation", fbe_u32_t, FBE_TRUE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("threads", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("start_lba", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("min_lba", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("inc_lba", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("max_lba", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("min_blocks", fbe_block_count_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("max_blocks", fbe_block_count_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("inc_blocks", fbe_block_count_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("alignment_blocks", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("max_passes", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("max_number_of_ios", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("msec_to_run", fbe_u64_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("options", fbe_rdgen_options_t, FBE_FALSE, "0x%x"),

    FBE_DEBUG_DECLARE_FIELD_INFO("lba_spec", fbe_rdgen_lba_specification_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("block_spec", fbe_rdgen_block_specification_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("pattern", fbe_rdgen_pattern_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("b_retry_ios", fbe_bool_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("msecs_to_expiration", fbe_u64_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("msecs_to_abort", fbe_u64_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("b_from_peer", fbe_bool_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("peer_options", fbe_rdgen_peer_options_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("affinity", fbe_bool_t, FBE_FALSE, "%d",
                                    fbe_rdgen_debug_print_affinity),
    FBE_DEBUG_DECLARE_FIELD_INFO("core", fbe_rdgen_peer_options_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("io_interface", fbe_rdgen_peer_options_t, FBE_FALSE, "%d",
                                    fbe_rdgen_debug_print_io_interface),

    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_rdgen_debug_specification_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        rdgen specification structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_rdgen_io_specification_t,
                              fbe_rdgen_debug_specification_struct_info,
                              &fbe_rdgen_debug_specification_field_info[0]);
/*!*******************************************************************
 * @var fbe_rdgen_debug_ts_field_info
 *********************************************************************
 * @brief Information about each of the fields of raid geometry.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_rdgen_debug_ts_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("object_p", fbe_rdgen_object_t, FBE_FALSE, "0x%x", 
                                    fbe_debug_display_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("request_p", fbe_rdgen_request_t, FBE_FALSE, "0x%x", 
                                    fbe_debug_display_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("state", fbe_u32_t, FBE_FALSE, "0x%x", 
                                    fbe_debug_display_function_ptr),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("memory_request", fbe_memory_request_t, FBE_FALSE, "0x%x", 
                                    fbe_memory_request_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO("lba", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("blocks", fbe_block_count_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("io_stamp", fbe_rdgen_io_stamp_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("current_lba", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("current_blocks", fbe_block_count_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("blocks_remaining", fbe_block_count_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("block_size", fbe_block_count_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("calling_state", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("thread_num", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("b_outstanding", fbe_bool_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("pass_count", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("io_count", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("peer_pass_count", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("peer_io_count", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("err_count", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("abort_err_count", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("media_err_count", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("io_failure_err_count", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("last_packet_status", fbe_rdgen_status_t, FBE_FALSE, "0x%x", 
                                    fbe_rdgen_debug_display_rdgen_status),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("last_send_time", fbe_time_t, FBE_FALSE, "0x%x",
                                    fbe_debug_trace_time),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("start_time", fbe_time_t, FBE_FALSE, "0x%x",
                                    fbe_debug_trace_time),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("time_to_abort", fbe_time_t, FBE_FALSE, "0x%x",
                                    fbe_debug_display_time),
    FBE_DEBUG_DECLARE_FIELD_INFO("flags", fbe_rdgen_ts_flags_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("b_aborted", fbe_bool_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("read_transport.packet", fbe_packet_t, FBE_FALSE, "0x%x", 
                                    fbe_debug_display_field_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("write_transport.packet", fbe_packet_t, FBE_FALSE, "0x%x", 
                                    fbe_debug_display_field_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("packet_p", fbe_packet_t, FBE_FALSE, "0x%x", 
                                    fbe_rdgen_ts_debug_trace_packet),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_rdgen_debug_ts_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        raid geometry structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_rdgen_ts_t,
                              fbe_rdgen_debug_ts_struct_info,
                              &fbe_rdgen_debug_ts_field_info[0]);

/*!*******************************************************************
 * @var fbe_rdgen_debug_status_field_info
 *********************************************************************
 * @brief Information about each of the fields of raid geometry.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_rdgen_debug_status_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("status", fbe_status_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("block_status", fbe_payload_block_operation_status_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("block_qualifier", fbe_payload_block_operation_qualifier_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_rdgen_debug_status_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        raid geometry structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_rdgen_status_t,
                              fbe_rdgen_debug_status_struct_info,
                              &fbe_rdgen_debug_status_field_info[0]);

/*!**************************************************************
 * fbe_rdgen_debug_display_rdgen_status()
 ****************************************************************
 * @brief
 *  Display the pointer to this field.  Which is just the
 *  base ptr plus the offset to this point in the structure.
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
 *  3/15/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_debug_display_rdgen_status(const fbe_u8_t * module_name,
                                                  fbe_dbgext_ptr base_ptr,
                                                  fbe_trace_func_t trace_func,
                                                  fbe_trace_context_t trace_context,
                                                  fbe_debug_field_info_t *field_info_p,
                                                  fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, 
                                         base_ptr + field_info_p->offset,
                                         &fbe_rdgen_debug_status_struct_info,
                                         4 /* fields per line */,
                                         0);
    return status;
}
/**************************************
 * end fbe_rdgen_debug_display_rdgen_status
 **************************************/
/*!**************************************************************
 * fbe_rdgen_debug_display_ts()
 ****************************************************************
 * @brief
 *  Display info on rdgen ts.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  3/25/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_rdgen_debug_display_ts(const fbe_u8_t * module_name,
                                            fbe_dbgext_ptr rdgen_ts_ptr,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context,
                                            fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "ts: 0x%llx ", (unsigned long long)rdgen_ts_ptr);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, rdgen_ts_ptr,
                                         &fbe_rdgen_debug_ts_struct_info,
                                         4    /* fields per line */,
                                         spaces_to_indent + 4);
    trace_func(trace_context, "\n");
    return status;
}
/******************************************
 * end fbe_rdgen_debug_display_ts()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_debug_display_object()
 ****************************************************************
 * @brief
 *  Display info on rdgen structures..
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  3/25/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_rdgen_debug_display_object(const fbe_u8_t * module_name,
                                            fbe_dbgext_ptr object_ptr,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context,
                                            fbe_u32_t spaces_to_indent)
{
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "object_p: 0x%llx ",
	       (unsigned long long)object_ptr);

    fbe_debug_display_structure(module_name, trace_func, trace_context, object_ptr,
                                &fbe_rdgen_debug_object_struct_info,
                                4 /* fields per line */,
                                spaces_to_indent + 10);
    if (fbe_rdgen_debug_trace_display_queues)
    {
        fbe_debug_display_structure(module_name, trace_func, trace_context, object_ptr,
                                    &fbe_rdgen_debug_object_active_ts_struct_info,
                                    4 /* fields per line */,
                                    spaces_to_indent + 10);
    }
    trace_func(trace_context, "\n");
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_debug_display_object()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_debug_display_specification()
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
 *  4/7/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_debug_display_specification(const fbe_u8_t * module_name,
                                                   fbe_dbgext_ptr base_ptr,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_debug_field_info_t *field_info_p,
                                                   fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    
    trace_func(trace_context, "spec: 0x%llx ",
	       (unsigned long long)(base_ptr + field_info_p->offset));
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, base_ptr + field_info_p->offset,
                                         &fbe_rdgen_debug_specification_struct_info,
                                         4    /* fields per line */,
                                         spaces_to_indent + 4);
    return status;
}
/**************************************
 * end fbe_rdgen_debug_display_specification
 **************************************/
/*!**************************************************************
 * fbe_rdgen_debug_display_request()
 ****************************************************************
 * @brief
 *  Display info on rdgen structures..
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  3/26/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_rdgen_debug_display_request(const fbe_u8_t * module_name,
                                             fbe_dbgext_ptr request_ptr,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_u32_t spaces_to_indent)
{
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "request_p: 0x%llx ",
	       (unsigned long long)request_ptr);

    fbe_debug_display_structure(module_name, trace_func, trace_context, request_ptr,
                                &fbe_rdgen_debug_request_struct_info,
                                4 /* fields per line */,
                                spaces_to_indent + 11);
    trace_func(trace_context, "\n");
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_debug_display_request()
 ******************************************/

/*!*******************************************************************
 * @var fbe_rdgen_debug_trace_packet_verbose
 *********************************************************************
 * @brief Tracks if we are in verbose mode or not.
 *        We need this global since the code that displays the packets
 *        is separate from the code that handles the interface.
 *
 *********************************************************************/
static fbe_bool_t fbe_rdgen_debug_trace_packet_verbose = FBE_FALSE;
/*!**************************************************************
 * fbe_rdgen_ts_debug_trace_packet()
 ****************************************************************
 * @brief
 *  Display the packet ptr or the packet contents.
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
 *  5/11/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_ts_debug_trace_packet(const fbe_u8_t * module_name,
                                             fbe_dbgext_ptr base_ptr,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_debug_field_info_t *field_info_p,
                                             fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u64_t packet_p = 0;
    fbe_u32_t ptr_size;

    status = fbe_debug_get_ptr_size(module_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        trace_func(trace_context, "unable to get ptr size status: %d\n", status);
        return status; 
    }
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &packet_p, ptr_size);

    /* If we have a packet and verbose mode is on, display the packet.
     */
    if ((packet_p != 0) && fbe_rdgen_debug_trace_packet_verbose)
    {
        trace_func(trace_context, "%s: 0x%llx\n", field_info_p->name,
		   (unsigned long long)packet_p);
        status = fbe_transport_print_fbe_packet(module_name, packet_p, trace_func, trace_context, spaces_to_indent);
    }
    else
    {
        status = fbe_debug_display_pointer(module_name, base_ptr, trace_func, trace_context, field_info_p, spaces_to_indent);
    }
    return status;
}
/**************************************
 * end fbe_rdgen_ts_debug_trace_packet
 **************************************/
/*!**************************************************************
 * fbe_rdgen_debug_display()
 ****************************************************************
 * @brief
 *  Display info on rdgen structures..
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * @param b_display_packets - TRUE to display all packets that
 *                            are outstanding.
 * 
 * @return fbe_status_t
 *
 * @author
 *  3/25/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_rdgen_debug_display(const fbe_u8_t * module_name,
                                     fbe_trace_func_t trace_func,
                                     fbe_trace_context_t trace_context,
                                     fbe_u32_t spaces_to_indent,
                                     fbe_bool_t b_display_packets,
                                     fbe_bool_t b_display_queues)
{
    fbe_status_t status;
    fbe_dbgext_ptr rdgen_service_pp = 0;
    fbe_dbgext_ptr rdgen_service_ptr = 0;
    fbe_u32_t ptr_size = 0;

    status = fbe_debug_get_ptr_size(module_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        trace_func(trace_context, "%s unable to get ptr size status: %d\n", 
                   __FUNCTION__, status);
        return status; 
    }

    /* Save the current verbosity mode for later use.
     */
    fbe_rdgen_debug_trace_packet_verbose = b_display_packets;
    fbe_rdgen_debug_trace_display_queues = b_display_queues;
    /* Display basic rdgen information.
     */
	FBE_GET_EXPRESSION(module_name, fbe_rdgen_service_p, &rdgen_service_pp);
    FBE_READ_MEMORY(rdgen_service_pp, &rdgen_service_ptr, ptr_size);
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "rdgen service ptr: 0x%llx/0x%llx\n", rdgen_service_pp, rdgen_service_ptr);
    trace_func(trace_context, "rdgen info: \n");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 2);
    fbe_debug_display_structure(module_name, trace_func, trace_context, rdgen_service_ptr,
                                &fbe_rdgen_debug_service_struct_info,
                                4 /* fields per line */,
                                spaces_to_indent + 2);

    if (fbe_rdgen_debug_trace_display_queues)
    {
        fbe_debug_display_structure(module_name, trace_func, trace_context, rdgen_service_ptr,
                                    &fbe_rdgen_debug_svc_req_only_struct_info,
                                    4 /* fields per line */,
                                    spaces_to_indent + 2);
    }
    trace_func(trace_context, "\n");
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_debug_display()
 ******************************************/
/*!*******************************************************************
 * @struct fbe_rdgen_debug_affinity_trace_info
 *********************************************************************
 * @brief Provides a mapping of enum to string
 *
 *********************************************************************/
fbe_debug_enum_trace_info_t fbe_rdgen_debug_affinity_trace_info[] = 
{
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RDGEN_AFFINITY_INVALID),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RDGEN_AFFINITY_NONE),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RDGEN_AFFINITY_SPREAD),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RDGEN_AFFINITY_FIXED),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RDGEN_AFFINITY_LAST),
    FBE_DEBUG_ENUM_TRACE_INFO_TERMINATOR,
};
/*!**************************************************************
 * fbe_rdgen_debug_print_affinity()
 ****************************************************************
 * @brief
 *  Display the decode of the enum
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
 *  1/26/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_debug_print_affinity(const fbe_u8_t * module_name,
                                            fbe_dbgext_ptr base_ptr,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context,
                                            fbe_debug_field_info_t *field_info_p,
                                            fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_rdgen_affinity_t affinity = 0;

    /* Display affinity if they are set.
     */
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &affinity, sizeof(fbe_rdgen_affinity_t));

    trace_func(trace_context, "%s: 0x%x ", field_info_p->name, affinity);

    status = fbe_debug_trace_enum_strings(module_name, base_ptr, trace_func, trace_context, 
                                          field_info_p, spaces_to_indent, fbe_rdgen_debug_affinity_trace_info);
    return status;
}
/**************************************
 * end fbe_rdgen_debug_print_affinity
 **************************************/

/*!*******************************************************************
 * @struct fbe_rdgen_debug_io_interface_trace_info
 *********************************************************************
 * @brief Provides a mapping of enum to string
 *
 *********************************************************************/
fbe_debug_enum_trace_info_t fbe_rdgen_debug_io_interface_trace_info[] = 
{
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RDGEN_INTERFACE_TYPE_INVALID),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RDGEN_INTERFACE_TYPE_IRP_DCA),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RDGEN_INTERFACE_TYPE_IRP_MJ),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RDGEN_INTERFACE_TYPE_IRP_SGL),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RDGEN_INTERFACE_TYPE_PACKET),
    FBE_DEBUG_ENUM_TRACE_FIELD_INFO(FBE_RDGEN_INTERFACE_TYPE_LAST),
    FBE_DEBUG_ENUM_TRACE_INFO_TERMINATOR,
};
/*!**************************************************************
 * fbe_rdgen_debug_print_io_interface()
 ****************************************************************
 * @brief
 *  Display the decode of the enum
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
 *  1/26/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_debug_print_io_interface(const fbe_u8_t * module_name,
                                                fbe_dbgext_ptr base_ptr,
                                                fbe_trace_func_t trace_func,
                                                fbe_trace_context_t trace_context,
                                                fbe_debug_field_info_t *field_info_p,
                                                fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_rdgen_interface_type_t io_interface = 0;

    /* Display io_interface if they are set.
     */
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &io_interface, sizeof(fbe_rdgen_interface_type_t));

    trace_func(trace_context, "%s: 0x%x ", field_info_p->name, io_interface);

    status = fbe_debug_trace_enum_strings(module_name, base_ptr, trace_func, trace_context, 
                                          field_info_p, spaces_to_indent, fbe_rdgen_debug_io_interface_trace_info);
    return status;
}
/**************************************
 * end fbe_rdgen_debug_print_io_interface
 **************************************/
/*!**************************************************************
 * fbe_rdgen_debug_print_device_name()
 ****************************************************************
 * @brief
 *  Display device string.
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
 *  3/21/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_debug_print_device_name(const fbe_u8_t * module_name,
                                               fbe_dbgext_ptr base_ptr,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_debug_field_info_t *field_info_p,
                                               fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t index;
    fbe_char_t device_name[FBE_RDGEN_DEVICE_NAME_CHARS] = {'\0'};

    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &device_name, sizeof(fbe_char_t) * FBE_RDGEN_DEVICE_NAME_CHARS);

    /* See if there is a terminator.
     */
    for (index = 0; index < FBE_RDGEN_DEVICE_NAME_CHARS; index++)
    {
        if (device_name[index] == 0)
        {
            break;
        }
    }

    if (index == FBE_RDGEN_DEVICE_NAME_CHARS)
    {
        trace_func(trace_context, "string is too long\n");
    }

    trace_func(trace_context, "%s: (%s)", field_info_p->name, device_name);
    return status;
}
/**************************************
 * end fbe_rdgen_debug_print_device_name
 **************************************/
/*************************
 * end file fbe_rdgen_debug.c
 *************************/
