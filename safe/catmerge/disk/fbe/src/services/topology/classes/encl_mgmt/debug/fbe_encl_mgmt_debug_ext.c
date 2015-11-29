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
 *  This file contains the debug functions for the Enclosure Mgmt object.
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
#include "fbe_encl_mgmt_debug.h"
#include "fbe_encl_mgmt_private.h"
#include "fbe_base_environment_fup_private.h"
#include "fbe_base_environment_debug.h"

static fbe_status_t fbe_encl_mgmt_print_encl_lcc_rev_or_status(const fbe_u8_t * module_name,
                                                fbe_trace_func_t trace_func,
                                                fbe_trace_context_t trace_context,
                                                fbe_dbgext_ptr pEnclInfo,
                                                fbe_bool_t   revision_or_status);

/*!**************************************************************
 * @fn fbe_encl_mgmt_debug_trace(const fbe_u8_t * module_name,
 *                                   fbe_dbgext_ptr encl_mgmt_p,
 *                                   fbe_trace_func_t trace_func,
 *                                   fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display all the trace information about the virtual drive object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param encl_mgmt_p - Ptr to Enclosure Mgmt object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * @author 
 *  6-May-2010  Created     Vaibhav Gaonkar
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_debug_trace(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr encl_mgmt_p,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context)
{

    /* Display the object id.
     */
    trace_func(trace_context, " object id: ");
    fbe_base_object_debug_trace_object_id(module_name,
                                          encl_mgmt_p,
                                          trace_func,
                                          trace_context);
    /* Display the class id.
     */
    trace_func(trace_context, "  class id: ");
    fbe_base_object_debug_trace_class_id(module_name,
                                         encl_mgmt_p,
                                         trace_func,
                                         trace_context);
    /* Display the Life Cycle State.
     */
    trace_func(trace_context, "  lifecycle state: ");
    fbe_base_object_debug_trace_state(module_name,
                                      encl_mgmt_p,
                                      trace_func,
                                      trace_context);
    
    trace_func(trace_context, "\n");

   /* Display the Trace Level.
    */
    trace_func(trace_context, "  trace_level: ");
    fbe_base_object_debug_trace_level(module_name,
                                      encl_mgmt_p,
                                      trace_func,
                                      trace_context);

    trace_func(trace_context, "\n");

    return FBE_STATUS_OK;
}
/*!**************************************************************
 * @fn fbe_encl_mgmt_debug_stat(const fbe_u8_t * module_name,
 *                                   fbe_dbgext_ptr encl_mgmt_p,
 *                                   fbe_trace_func_t trace_func,
 *                                   fbe_trace_context_t trace_context
                                     fbe_device_physical_location_t *plocation)
 ****************************************************************
 * @brief
 *  Display all the debug  iformation about the encl mgmt  object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param ps_mgmt_p - Ptr to ENCL Mgmt object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 * @param ps_mgmt_p - Ptr to location.
 * @param revision_or_status - flag for revision or status.
.* @param location_flag - flag for location.
 * @return - FBE_STATUS_OK  
 *
 * @author 
 *  08 -October-2010 - Created. Vishnu Sharma
 *
 ****************************************************************/

fbe_status_t fbe_encl_mgmt_debug_stat(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr encl_mgmt_p,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context,
                                           fbe_device_physical_location_t *plocation,
                                           fbe_bool_t   revision_or_status,
                                           fbe_bool_t   location_flag)
{
    fbe_status_t                   status = FBE_STATUS_OK;
    fbe_u32_t                      encl_count = 0;
    fbe_u32_t                      encl_info_offset;
    fbe_dbgext_ptr                 encl_info_p;
    fbe_device_physical_location_t location = {0};
    fbe_u32_t                      ptr_size;
    fbe_object_id_t                object_id;

    FBE_GET_FIELD_OFFSET(module_name, fbe_encl_mgmt_t, "encl_info", &encl_info_offset);

    FBE_READ_MEMORY(encl_mgmt_p + encl_info_offset, &encl_info_p, sizeof(fbe_dbgext_ptr));

    status = fbe_debug_get_ptr_size(module_name, &ptr_size);
    if (status != FBE_STATUS_OK)
    {
        csx_dbg_ext_print("%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return status;
    }

    if(revision_or_status)
    {
        trace_func(trace_context,"\n     Bus     Encl     Slot       Status                 Workstate\n");
        trace_func(trace_context," ==================================================================\n");
    }
    else
    {
        trace_func(trace_context,"\n     Bus     Encl     Slot       Revision \n");
        trace_func(trace_context," ===========================================\n");
    }

    for(encl_count = 0;encl_count < FBE_ESP_MAX_ENCL_COUNT; encl_count++)
    {
        FBE_READ_MEMORY(encl_mgmt_p + encl_info_offset + ptr_size*encl_count, &encl_info_p, sizeof(ULONG64));

        FBE_GET_FIELD_DATA(module_name,
                encl_info_p,
                fbe_encl_info_t,
                location,
                sizeof(fbe_device_physical_location_t),
                &location);

        FBE_GET_FIELD_DATA(module_name,
                encl_info_p,
                fbe_encl_info_t,
                object_id,
                sizeof(fbe_object_id_t),
                &object_id);

        if(object_id != FBE_OBJECT_ID_INVALID)
        {
            if(!location_flag ||
               (location.bus == plocation->bus &&
               location.enclosure == plocation->enclosure))
            {
                // Find the specified enclosure.
                fbe_encl_mgmt_print_encl_lcc_rev_or_status(module_name,
                                                           trace_func,
                                                           trace_context,
                                                           encl_info_p,
                                                           revision_or_status);
        
                if(location_flag)
                {
                    break;
                }
            }
        } // End of - if(encl_info.object_id != FBE_OBJECT_ID_INVALID)
    }//End of for loop for enclosure

    trace_func(trace_context, "\n");
    return status;
}


/*!**************************************************************
 * @fn fbe_encl_mgmt_print_encl_lcc_rev_or_status(const fbe_u8_t * module_name,
 *                                               fbe_trace_func_t trace_func,
 *                                               fbe_trace_context_t trace_context,
 *                                               fbe_encl_info_t * pEnclInfo,
 *                                               fbe_bool_t   revision_or_status)
 ****************************************************************
 * @brief
 *  Print firmware revision or firmware upgrade status for the LCCs in the specified enclosure. 
 *
 * @param module_name - This is the name of the module we are debugging.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 * @param pEnclInfo - Ptr to the enclosure info.
 * @param revision_or_status - flag for revision or status.
 *
 * @return - FBE_STATUS_OK  
 *
 * @author 
 *  12-Aug-2011 PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_print_encl_lcc_rev_or_status(const fbe_u8_t * module_name,
                                                fbe_trace_func_t trace_func,
                                                fbe_trace_context_t trace_context,
                                                fbe_dbgext_ptr pEnclInfo,
                                                fbe_bool_t   revision_or_status)
{
    fbe_u32_t                        lcc_count = 0;
    fbe_u32_t                        fbe_lcc_info_size;
    fbe_u32_t                        fbe_lccFup_info_size;
    fbe_u32_t                        lccInfo_offset;
    fbe_u32_t                        lccFupInfo_offset;
    fbe_u32_t                        lccCount;
    fbe_device_physical_location_t   location = {0};
    fbe_lcc_info_t                   lccInfo = {0};
    fbe_base_env_fup_work_state_t        workState;
    fbe_base_env_fup_completion_status_t completionStatus;
    fbe_dbgext_ptr                       pWorkItem;


    FBE_GET_FIELD_OFFSET(module_name, fbe_encl_info_t, "lccInfo", &lccInfo_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_encl_info_t, "lccFupInfo", &lccFupInfo_offset);
    FBE_GET_TYPE_SIZE(module_name, fbe_lcc_info_t, &fbe_lcc_info_size);
    FBE_GET_TYPE_SIZE(module_name, fbe_base_env_fup_persistent_info_t, &fbe_lccFup_info_size);

    FBE_GET_FIELD_DATA(module_name, pEnclInfo, fbe_encl_info_t, lccCount, sizeof(fbe_u32_t), &lccCount);
    FBE_GET_FIELD_DATA(module_name, pEnclInfo, fbe_encl_info_t, location, sizeof(fbe_device_physical_location_t), &location);

    for(lcc_count =0; lcc_count < lccCount; lcc_count++)
    {
        FBE_READ_MEMORY(pEnclInfo + lccInfo_offset + fbe_lcc_info_size*lcc_count, &lccInfo, sizeof(fbe_lcc_info_t));

        if(lccInfo.inserted)
        {

            if(revision_or_status)
            {
                FBE_GET_FIELD_DATA(module_name,
                        pEnclInfo + lccFupInfo_offset,
                        fbe_base_env_fup_persistent_info_t,
                        completionStatus,
                        sizeof(fbe_base_env_fup_completion_status_t),
                        &completionStatus);

                trace_func(trace_context,"   %3d     %3d     %3d      %s",
                           location.bus,
                           location.enclosure,
                           lcc_count,
                           fbe_base_env_decode_fup_completion_status(completionStatus));

                FBE_GET_FIELD_DATA(module_name,
                        pEnclInfo + lccFupInfo_offset,
                        fbe_base_env_fup_persistent_info_t,
                        pWorkItem,
                        sizeof(fbe_dbgext_ptr),
                        &pWorkItem);
                
                if(pWorkItem != NULL)
                {
                    FBE_GET_FIELD_DATA(module_name,
                                    pWorkItem,
                                    fbe_base_env_fup_work_item_t,
                                    workState,
                                    sizeof(fbe_base_env_fup_work_state_t),
                                    &workState);

                    trace_func(trace_context,"       %s",fbe_base_env_decode_fup_work_state(workState));
                }
            }
            else
            {
                trace_func(trace_context,"   %3d      %3d      %3d      %s",
                           location.bus,
                           location.enclosure,
                           lcc_count,
                           lccInfo.firmwareRev);
            }

            trace_func(trace_context, "\n");

            lccFupInfo_offset += fbe_lccFup_info_size;
        }
    }//End of for loop for lcc

    return FBE_STATUS_OK;
}
            
