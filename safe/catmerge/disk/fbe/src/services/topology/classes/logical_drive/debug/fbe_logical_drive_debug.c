/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_logical_drive_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for the logical drive.
 *
 * @author
 *   12/3/2008:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_base_object_debug.h"
#include "fbe_logical_drive_private.h"
#include "fbe_base_object_trace.h"
#include "fbe_logical_drive_debug.h"
#include "fbe_transport_debug.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!**************************************************************
 * @fn fbe_logical_drive_debug_trace(const fbe_u8_t * module_name,
 *                                   fbe_dbgext_ptr logical_drive,
 *                                   fbe_trace_func_t trace_func,
 *                                   fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display all the trace information about the logical drive object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param logical_drive_p - Ptr to logical drive object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  12/3/2008 - Created. RPF
 *  02/10/2009 - Modified to display logical drive detail. Nishit Singh
 *
 ****************************************************************/

fbe_status_t fbe_logical_drive_debug_trace(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr logical_drive_p,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context)
{
    fbe_u32_t identity_offset, last_identity_offset;
    fbe_u32_t block_edge_offset;
    fbe_u32_t base_discovered_offset;
    fbe_u32_t transport_server_offset;
    fbe_object_id_t object_id;
    fbe_ldo_flags_enum_t flags;
    fbe_u32_t port_number, enclosure_number, slot_number;

    /* Display the object id.
     */
    trace_func(trace_context, "  object id: ");
    fbe_base_object_debug_trace_object_id(module_name,
                                          logical_drive_p,
                                          trace_func,
                                          trace_context);

    /* Display the class id.
     */
    trace_func(trace_context, "  class id: ");
    fbe_base_object_debug_trace_class_id(module_name,
                                         logical_drive_p,
                                         trace_func,
                                         trace_context);
    
    /* Display the Life Cycle State.
     */
    trace_func(trace_context, "  lifecycle state: ");
    fbe_base_object_debug_trace_state(module_name,
                                      logical_drive_p,
                                      trace_func,
                                      trace_context);
    trace_func(trace_context, "\n");

    /* Display the Trace Level.
     */
    trace_func(trace_context, "  trace_level: ");
    fbe_base_object_debug_trace_level(module_name,
                                      logical_drive_p,
                                      trace_func,
                                      trace_context);

    /* Display the flag.
     */
    FBE_GET_FIELD_DATA(module_name, 
                       logical_drive_p,
                       fbe_logical_drive_t,
                       flags,
                       sizeof(fbe_ldo_flags_enum_t),
                       &flags); 
    trace_func(trace_context, "  flags: 0x%x", flags);

    /* Display the pointer to the object id.
    */
    FBE_GET_FIELD_DATA(module_name, 
                       logical_drive_p,
                       fbe_logical_drive_t,
                       object_id,
                       sizeof(fbe_object_id_t),
                       &object_id); 
    trace_func(trace_context, "  object id(pdo): 0x%x\n", object_id);

    FBE_GET_FIELD_DATA(module_name, 
                       logical_drive_p,
                       fbe_logical_drive_t,
                       port_number,
                       sizeof(fbe_u32_t),
                       &port_number); 
    FBE_GET_FIELD_DATA(module_name, 
                       logical_drive_p,
                       fbe_logical_drive_t,
                       enclosure_number,
                       sizeof(fbe_u32_t),
                       &enclosure_number); 
    FBE_GET_FIELD_DATA(module_name, 
                       logical_drive_p,
                       fbe_logical_drive_t,
                       slot_number,
                       sizeof(fbe_u32_t),
                       &slot_number); 
    trace_func(trace_context, "  bus: 0x%x encl: 0x%x slot: 0x%x\n", 
               port_number, enclosure_number, slot_number);

    /* Display the identity as well as last_identity information.
     */
    FBE_GET_FIELD_OFFSET(module_name,
                         fbe_logical_drive_t,
                         "identity",
                         &identity_offset);
    FBE_GET_FIELD_OFFSET(module_name,
                         fbe_logical_drive_t,
                         "last_identity",
                         &last_identity_offset);
    trace_func(trace_context, "  identity:      ");
    fbe_block_transport_print_identify(module_name,
                                       logical_drive_p + identity_offset,
                                       trace_func,
                                       trace_context);
    trace_func(trace_context, "\n");
    trace_func(trace_context, "  last identity: ");
    fbe_block_transport_print_identify(module_name,
                                       logical_drive_p + last_identity_offset,
                                       trace_func,
                                       trace_context);
    trace_func(trace_context, "\n");
  
    /* Display the Block Edge information.
     */
    FBE_GET_FIELD_OFFSET(module_name,
                         fbe_logical_drive_t,
                         "block_edge",
                         &block_edge_offset);
    fbe_block_transport_display_edge(module_name,
                                         logical_drive_p + block_edge_offset,
                                         trace_func,
                                     trace_context,
                                     2);

    /* Display the Discovery Edge information.
     */
    FBE_GET_FIELD_OFFSET(module_name,
                         fbe_logical_drive_t,
                         "base_discovered",
                         &base_discovered_offset);
    fbe_discovery_transport_print_detail(module_name,
                                         logical_drive_p + base_discovered_offset,
                                         trace_func,
                                         trace_context);

    /* Display the Transport Server information.
     */
    FBE_GET_FIELD_OFFSET(module_name,
                         fbe_logical_drive_t,
                         "block_transport_server",
                         &transport_server_offset);
    fbe_block_transport_print_server_detail(module_name,
                                            logical_drive_p + transport_server_offset,
                                            trace_func,
                                            trace_context);

    return FBE_STATUS_OK;
} 
/**************************************
 * end fbe_logical_drive_debug_trace()
 **************************************/


/*************************
 * end file fbe_logical_drive_debug.c
 *************************/
