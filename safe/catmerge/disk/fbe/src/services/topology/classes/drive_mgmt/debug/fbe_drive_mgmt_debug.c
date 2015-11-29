/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_encl_mgmt_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for the Drive Mgmt object.
 *
 * @author
 *  6-May-2010 - Created. Vaibhav Gaonkar
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_base_object_debug.h"
#include "fbe_base_object.h"
#include "../fbe_drive_mgmt_private.h"
#include "generic_utils_lib.h"
#include "fbe_drive_mgmt_debug.h"
#include "fbe_base_object_trace.h"



/*!**************************************************************
 * @fn fbe_drive_mgmt_debug_trace(const fbe_u8_t * module_name,
 *                                   fbe_dbgext_ptr encl_mgmt_p,
 *                                   fbe_trace_func_t trace_func,
 *                                   fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display all the trace information about the virtual drive object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param drive_mgmt_p - Ptr to Drive Mgmt object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * @author 
 *
 ****************************************************************/
fbe_status_t fbe_drive_mgmt_debug_trace(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr drive_mgmt_p,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context)
{
    /* Display the object id.
     */
    trace_func(trace_context, "  object id: ");
    fbe_base_object_debug_trace_object_id(module_name,
                                          drive_mgmt_p,
                                          trace_func,
                                          trace_context);
    /* Display the class id.
     */
    trace_func(trace_context, "  class id: ");
    fbe_base_object_debug_trace_class_id(module_name,
                                         drive_mgmt_p,
                                         trace_func,
                                         trace_context);
    /* Display the Life Cycle State.
     */
    trace_func(trace_context, "  lifecycle state: ");
    fbe_base_object_debug_trace_state(module_name,
                                      drive_mgmt_p,
                                      trace_func,
                                      trace_context);
    
    trace_func(trace_context, "\n");

   /* Display the Trace Level.
    */
    trace_func(trace_context, "  trace_level: ");
    fbe_base_object_debug_trace_level(module_name,
                                      drive_mgmt_p,
                                      trace_func,
                                      trace_context);

    trace_func(trace_context, "\n");


    trace_func(trace_context, "\n");

    return FBE_STATUS_OK;
}


/*!**********************************************************************************************
 * @fn fbe_status_t fbe_drive_mgmt_drive_info_debug_trace(  const fbe_u8_t *    module_name,
 *                                                          fbe_dbgext_ptr      drive_info_p,
 *                                                          fbe_trace_func_t    trace_func,
 *                                                          fbe_trace_context_t trace_context)
 ************************************************************************************************
 * @brief
 *      Display a drive information for dmo macro.
 *
 * @param   module_name - This is the name of the module we are debugging.
 * @param   drive_info_p - Ptr to a drive info structure
 * @param   trace_func - Function to use when tracing.
 * @param   trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * @author 
 *  Michael Li
 *  7/26/2012
 ****************************************************************/
fbe_status_t fbe_drive_mgmt_drive_info_debug_trace( const fbe_u8_t *    module_name,
                                                    fbe_dbgext_ptr      drive_info_p,
                                                    fbe_trace_func_t    trace_func,
                                                    fbe_trace_context_t trace_context)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_drive_info_t*   drive_info_ptr = (fbe_drive_info_t*)(fbe_ptrhld_t)drive_info_p;

    trace_func(trace_context, "\n**** DRIVE %d_%d_%d ObjectID:0x%X (%d) ****\n",  drive_info_ptr->location.bus, drive_info_ptr->location.enclosure, drive_info_ptr->location.slot, \
                                                                                  drive_info_ptr->object_id, drive_info_ptr->object_id);
    trace_func(trace_context, "%20s: 0x%X (%d)\n",  "Parent object ID",           drive_info_ptr->parent_object_id, drive_info_ptr->parent_object_id);
    trace_func(trace_context, "%20s: %s\n",         "Lifecycle state",            fbe_base_object_trace_get_state_string(drive_info_ptr->state));
    trace_func(trace_context, "%20s: %d (%s)\n",    "Drive type",                 drive_info_ptr->drive_type, drive_info_ptr->drive_type == FBE_DRIVE_TYPE_SAS ? "SAS" : "");
    trace_func(trace_context, "%20s: %s\n",         "Vendor",                     drive_info_ptr->vendor_id);
    trace_func(trace_context, "%20s: %s\n",         "Product ID",                 drive_info_ptr->product_id);
    drive_info_ptr->tla[FBE_SCSI_INQUIRY_TLA_SIZE] = '\0'; 
    trace_func(trace_context, "%20s: %s\n",         "TLA",                        drive_info_ptr->tla);
    drive_info_ptr->rev[FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE] = '\0';
    trace_func(trace_context, "%20s: %s\n",         "Revision",                   drive_info_ptr->rev);
    drive_info_ptr->sn[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE] = '\0';
    trace_func(trace_context, "%20s: %s\n",         "Serial number",              drive_info_ptr->sn);
    trace_func(trace_context, "%20s: %llu\n",       "Gross capacity",             drive_info_ptr->gross_capacity);
    trace_func(trace_context, "%20s: %llu sec\n",   "Last log",                   drive_info_ptr->last_log/1000);
    trace_func(trace_context, "%20s: %s\n",         "Inserted",                   drive_info_ptr->inserted ? "YES" : "NO");
    trace_func(trace_context, "%20s: %d\n", "Local side ID",            drive_info_ptr->local_side_id);
    trace_func(trace_context, "%20s: %s\n", "Bypassed",                 drive_info_ptr->bypassed ? "YES" : "NO");
    trace_func(trace_context, "%20s: %s\n", "Self_bypassed",            drive_info_ptr->self_bypassed ? "YES" : "NO");
    drive_info_ptr->bridge_hw_rev[FBE_SCSI_INQUIRY_VPD_BRIDGE_HW_REV_SIZE] = '\0';
    trace_func(trace_context, "%20s: %s\n", "Bridge HW revision",       drive_info_ptr->bridge_hw_rev);
    trace_func(trace_context, "%20s: %d\n", "Block size",               drive_info_ptr->block_size);
    trace_func(trace_context, "%20s: %d\n", "Queue depth",              drive_info_ptr->drive_qdepth);
    trace_func(trace_context, "%20s: %d\n", "RPM",                      drive_info_ptr->drive_RPM);    
    drive_info_ptr->drive_description_buff[FBE_SCSI_DESCRIPTION_BUFF_SIZE] = '\0';
    trace_func(trace_context, "%20s: %s",   "Drive desc buff",          drive_info_ptr->drive_description_buff);
    drive_info_ptr->dg_part_number_ascii[FBE_SCSI_INQUIRY_PART_NUMBER_SIZE] = '\0';
    trace_func(trace_context, "%20s: %s\n", "Dg P/N ASCII",             drive_info_ptr->dg_part_number_ascii);
    trace_func(trace_context, "%20s: %d\n", "Speed capability",         drive_info_ptr->speed_capability);
    trace_func(trace_context, "%20s: %d\n", "Spindown qualified",       drive_info_ptr->spin_down_qualified);
    trace_func(trace_context, "%20s: %d\n", "Spinup time in min",       drive_info_ptr->spin_up_time_in_minutes);
    trace_func(trace_context, "%20s: %d\n", "Standby time in min",      drive_info_ptr->stand_by_time_in_minutes);
    trace_func(trace_context, "%20s: %d\n", "Spinup count",             drive_info_ptr->spin_up_count);
    trace_func(trace_context, "%20s: %s\n", "Dbg remove",               drive_info_ptr->dbg_remove ? "YES" : "NO");
    trace_func(trace_context, "%20s: %s\n", "Powered off",              drive_info_ptr->poweredOff ? "YES" : "NO");
    trace_func(trace_context, "\n");
    
    return status;
}

