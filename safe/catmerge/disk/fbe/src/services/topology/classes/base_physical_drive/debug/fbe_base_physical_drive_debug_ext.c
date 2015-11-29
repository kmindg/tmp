/**********************************************************************
 *  Copyright (C)  EMC Corporation 2008
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*************************************************************************
 * @file fbe_base_physical_drive_debug_ext.c
 ***************************************************************************
 *
 * @brief
 *  The routines in this file contains physical drive object's debugger
 *  extension related functions.
 *
 *
 * NOTE: 
 *
 * HISTORY
 *   01-Apr-2009 chenl6 - Created. 
 *
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_base_object_debug.h"
#include "fbe_base_physical_drive_debug.h"
#include "base_physical_drive_private.h"
#include "fbe_transport_debug.h"


/*!**************************************************************
 * @fn fbe_base_physical_drive_debug_trace(const fbe_u8_t * module_name, 
 *                                   fbe_dbgext_ptr physical_drive_p,
 *                                   fbe_trace_func_t trace_func,
 *                                   fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display all the trace information about the base physical drive object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param physical_drive_p - Ptr to base physical drive object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  01/Apr/2008 - Created. chenl6
 *
 ****************************************************************/

fbe_status_t fbe_base_physical_drive_debug_trace(const fbe_u8_t * module_name, 
                                           fbe_dbgext_ptr physical_drive_p, 
                                           fbe_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context)
{
    fbe_base_physical_drive_info_t drive_info;
    fbe_sas_address_t sas_address;
    fbe_lba_t block_count;
    fbe_block_size_t  block_size;
    fbe_u32_t transport_server_offset;
    fbe_base_physical_drive_local_state_t local_state;
    fbe_base_pdo_flags_t   flags;
    fbe_pdo_maintenance_mode_flags_t maintenance_mode_bitmap;
    fbe_drive_configuration_handle_t drive_configuration_handle;
    fbe_drive_error_t   drive_error;
    fbe_drive_dieh_state_t  dieh_state;
    
    fbe_port_number_t       port_number;
    fbe_enclosure_number_t  enclosure_number;
    fbe_slot_number_t       slot_number;

    /* Display physical location.
     */
    FBE_GET_FIELD_DATA(module_name,
                       physical_drive_p,
                       fbe_base_physical_drive_t,
                       port_number,
                       sizeof(fbe_port_number_t),
                       &port_number);
    FBE_GET_FIELD_DATA(module_name,
                       physical_drive_p,
                       fbe_base_physical_drive_t,
                       enclosure_number,
                       sizeof(fbe_enclosure_number_t),
                       &enclosure_number);
    FBE_GET_FIELD_DATA(module_name,
                       physical_drive_p,
                       fbe_base_physical_drive_t,
                       slot_number,
                       sizeof(fbe_slot_number_t),
                       &slot_number);
    trace_func(trace_context, "  port: %d, encl: %d, slot: %d", port_number, 
                                                                (fbe_enclosure_number_t)enclosure_number,
                                                                (fbe_slot_number_t)slot_number);    
    trace_func(trace_context, "\n");
    
    /* Display the object id.
     */
    trace_func(trace_context, "  object id: ");
    fbe_base_object_debug_trace_object_id(module_name,
                                          physical_drive_p,
                                          trace_func,
                                          trace_context);

    /* Display the class id.
     */
    trace_func(trace_context, "  class id: ");
    fbe_base_object_debug_trace_class_id(module_name,
                                         physical_drive_p,
                                         trace_func,
                                         trace_context);
    
    
    /* Display the Life Cycle State.
     */
    trace_func(trace_context, "  lifecycle state: ");
    fbe_base_object_debug_trace_state(module_name,
                                      physical_drive_p,
                                      trace_func,
                                      trace_context);
    trace_func(trace_context, "\n");

    /* Display SAS address.
     */
    FBE_GET_FIELD_DATA(module_name, 
                       physical_drive_p,
                       fbe_base_physical_drive_t,
                       address,
                       sizeof(fbe_sas_address_t),
                       &sas_address);
    trace_func(trace_context, "  SAS address: 0x%llx\n",
	       (unsigned long long)sas_address);

    /* Display physical drive information.
     */
    FBE_GET_FIELD_DATA(module_name,
                       physical_drive_p,
                       fbe_base_physical_drive_t,
                       drive_info,
                       sizeof(fbe_base_physical_drive_info_t),
                       &drive_info);
    /* trace_func(trace_context, "  vendor: %s", drive_info.vendor_id); */
    trace_func(trace_context, "  TLA: %s, Rev %s, SN: %s\n", drive_info.tla, drive_info.revision, drive_info.serial_number);

    /* Display block size and count.
     */
    FBE_GET_FIELD_DATA(module_name,
                       physical_drive_p,
                       fbe_base_physical_drive_t,
                       block_size,
                       sizeof(fbe_block_size_t),
                       &block_size);

    FBE_GET_FIELD_DATA(module_name,
                       physical_drive_p,
                       fbe_base_physical_drive_t,
                       block_count,
                       sizeof(fbe_lba_t),
                       &block_count);
    trace_func(trace_context, "  block size: %d, block count: 0x%llx\n", block_size, (unsigned long long)block_count);


    /* local state & flags
     */
    FBE_GET_FIELD_DATA(module_name,
                       physical_drive_p,
                       fbe_base_physical_drive_t,
                       local_state,
                       sizeof(fbe_base_physical_drive_local_state_t),
                       &local_state);

    FBE_GET_FIELD_DATA(module_name,
                       physical_drive_p,
                       fbe_base_physical_drive_t,
                       flags,
                       sizeof(fbe_base_pdo_flags_t),
                       &flags);

    FBE_GET_FIELD_DATA(module_name,
                       physical_drive_p,
                       fbe_base_physical_drive_t,
                       maintenance_mode_bitmap,
                       sizeof(fbe_pdo_maintenance_mode_flags_t),
                       &maintenance_mode_bitmap);

    trace_func(trace_context, "  local_state: 0x%x   flags: 0x%x   mmode: 0x%x\n", local_state, flags, maintenance_mode_bitmap);


    /* Display drive error information.
     */
    FBE_GET_FIELD_DATA(module_name,
                       physical_drive_p,
                       fbe_base_physical_drive_t,
                       drive_configuration_handle,
                       sizeof(fbe_drive_configuration_handle_t),
                       &drive_configuration_handle);

    trace_func(trace_context, "  DIEH handle: 0x%x\n", drive_configuration_handle);

    FBE_GET_FIELD_DATA(module_name,
                       physical_drive_p,
                       fbe_base_physical_drive_t,
                       drive_error,
                       sizeof(fbe_drive_error_t),
                       &drive_error);
    
    FBE_GET_FIELD_DATA(module_name,
                       physical_drive_p,
                       fbe_base_physical_drive_t,
                       dieh_state,
                       sizeof(fbe_drive_dieh_state_t),
                       &dieh_state);
            
    trace_func(trace_context, "  IO Count:               0x%llx\n", (unsigned long long)drive_error.io_counter);
    trace_func(trace_context, "  Cummulative - Error Tag:0x%llx, Tripped Actions:0x%x\n", (unsigned long long)drive_error.io_error_tag,          dieh_state.category.io.tripped_bitmap);
    trace_func(trace_context, "  Recovered -   Error Tag:0x%llx, Tripped Actions:0x%x\n", (unsigned long long)drive_error.recovered_error_tag,   dieh_state.category.recovered.tripped_bitmap);
    trace_func(trace_context, "  Media -       Error Tag:0x%llx, Tripped Actions:0x%x\n", (unsigned long long)drive_error.media_error_tag,       dieh_state.category.media.tripped_bitmap);
    trace_func(trace_context, "  Hardware -    Error Tag:0x%llx, Tripped Actions:0x%x\n", (unsigned long long)drive_error.hardware_error_tag,    dieh_state.category.hardware.tripped_bitmap);
    trace_func(trace_context, "  HealthCheck - Error Tag:0x%llx, Tripped Actions:0x%x\n", (unsigned long long)drive_error.healthCheck_error_tag, dieh_state.category.healthCheck.tripped_bitmap);
    trace_func(trace_context, "  Data -        Error Tag:0x%llx, Tripped Actions:0x%x\n", (unsigned long long)drive_error.data_error_tag,        dieh_state.category.data.tripped_bitmap);
    trace_func(trace_context, "  Link -        Error Tag:0x%llx, Tripped Actions:0x%x\n", (unsigned long long)drive_error.link_error_tag,        dieh_state.category.link.tripped_bitmap);
    trace_func(trace_context, "  Reset -       Error Tag:0x%llx\n", (unsigned long long)drive_error.reset_error_tag);

    trace_func(trace_context, "  DIEH EOL call home sent: %d\n", dieh_state.eol_call_home_sent);
    trace_func(trace_context, "  DIEH KILL call home sent: %d\n", dieh_state.kill_call_home_sent);


    /* Display the Transport Server information.
     */
    FBE_GET_FIELD_OFFSET(module_name,
                         fbe_base_physical_drive_t,
                         "block_transport_server",
                         &transport_server_offset);
    fbe_block_transport_print_server_detail(module_name,
                                            physical_drive_p + transport_server_offset,
                                            trace_func,
                                            trace_context);

    return FBE_STATUS_OK;
}
