/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_ps_mgmt_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for the Power Supply Mgmt object.
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
#include "fbe_ps_mgmt_debug.h"
#include "fbe_ps_mgmt_private.h"
#include "fbe_base_environment_debug.h"
#include "fbe_base_environment_fup_private.h"


static fbe_status_t fbe_ps_mgmt_print_ps_rev_or_status(const fbe_u8_t * module_name,
                                                fbe_trace_func_t trace_func,
                                                fbe_trace_context_t trace_context,
                                                fbe_dbgext_ptr pEnclPsInfo,
                                                fbe_bool_t   revision_or_status);

/*!**************************************************************
 * @fn fbe_ps_mgmt_debug_trace(const fbe_u8_t * module_name,
 *                                   fbe_dbgext_ptr encl_mgmt_p,
 *                                   fbe_trace_func_t trace_func,
 *                                   fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display all the trace information about the virtual drive object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param ps_mgmt_p - Ptr to PS object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * @author 
 *  06-May-2010 - Created. Vaibhav Gaonkar
 *
 ****************************************************************/
fbe_status_t fbe_ps_mgmt_debug_trace(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr ps_mgmt_p,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context)
{
    /* Display the object id.
     */
    trace_func(trace_context, "  object id: ");
    fbe_base_object_debug_trace_object_id(module_name,
                                          ps_mgmt_p,
                                          trace_func,
                                          trace_context);
    /* Display the class id.
     */
    trace_func(trace_context, "  class id: ");
    fbe_base_object_debug_trace_class_id(module_name,
                                         ps_mgmt_p,
                                         trace_func,
                                         trace_context);
    /* Display the Life Cycle State.
     */
    trace_func(trace_context, "  lifecycle state: ");
    fbe_base_object_debug_trace_state(module_name,
                                      ps_mgmt_p,
                                      trace_func,
                                      trace_context);
    
    trace_func(trace_context, "\n");

   /* Display the Trace Level.
    */
    trace_func(trace_context, "  trace_level: ");
    fbe_base_object_debug_trace_level(module_name,
                                      ps_mgmt_p,
                                      trace_func,
                                      trace_context);

    trace_func(trace_context, "\n");

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_ps_mgmt_debug_stat(const fbe_u8_t * module_name,
 *                                   fbe_dbgext_ptr ps_mgmt_p,
 *                                   fbe_trace_func_t trace_func,
 *                                   fbe_trace_context_t trace_context
                                     fbe_device_physical_location_t *plocation)
 ****************************************************************
 * @brief
 *  Display all the debug  iformation about the ps mgmt  object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param ps_mgmt_p - Ptr to PS Mgmt object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 * @param ps_mgmt_p - Ptr to location.
.* @param revision_or_status - flag for revision or status.
.* @param location_flag - flag for location.
 
 * @return - FBE_STATUS_OK  
 *
 * @author 
 *  08 -October-2010 - Created. Vishnu Sharma
 *
 ****************************************************************/

fbe_status_t fbe_ps_mgmt_debug_stat(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr ps_mgmt_p,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context,
                                           fbe_device_physical_location_t *plocation,
                                           fbe_bool_t   revision_or_status,
                                           fbe_bool_t   location_flag)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_u32_t               encl_count = 0;
    fbe_u32_t               fbe_ps_info_array_offset = 0;
    fbe_u32_t               fbe_encl_ps_info_offset = 0;
    fbe_u32_t               fbe_ps_info_size = 0;
    fbe_dbgext_ptr          fbe_ps_info_array_ptr;
    fbe_dbgext_ptr          fbe_encl_ps_info_array;
    fbe_object_id_t         objectId;
    fbe_device_physical_location_t location = {0};
    
    FBE_GET_TYPE_SIZE(module_name, fbe_encl_ps_info_t, &fbe_ps_info_size);
    FBE_GET_FIELD_OFFSET(module_name, fbe_ps_mgmt_s, "psInfo", &fbe_ps_info_array_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_encl_ps_info_array_t, "fbe_encl_ps_info_t", &fbe_encl_ps_info_offset);

    FBE_READ_MEMORY(ps_mgmt_p + fbe_ps_info_array_offset, &fbe_ps_info_array_ptr, sizeof(fbe_dbgext_ptr));
    fbe_encl_ps_info_array = fbe_ps_info_array_ptr + fbe_encl_ps_info_offset;
    
    if(fbe_encl_ps_info_array !=  NULL)
    {
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

        
        // The enclosure location was specified.
        for(encl_count =0 ;encl_count<FBE_ESP_MAX_ENCL_COUNT;encl_count++)
        {
            FBE_GET_FIELD_DATA(module_name,
                            fbe_encl_ps_info_array,
                            fbe_encl_ps_info_t,
                            location,
                            sizeof(fbe_device_physical_location_t),
                            &location);
            
            FBE_GET_FIELD_DATA(module_name,
                            fbe_encl_ps_info_array,
                            fbe_encl_ps_info_t,
                            objectId,
                            sizeof(fbe_object_id_t),
                            &objectId);

            //Proceed if object id is valid
            if(objectId != FBE_OBJECT_ID_INVALID)
            {
           
                if(!location_flag ||
                  (location.bus == plocation->bus &&
                   location.enclosure == plocation->enclosure))
                {
                    // The specified enclosure was found.
                    fbe_ps_mgmt_print_ps_rev_or_status(module_name,
                                                       trace_func, 
                                                       trace_context,
                                                       fbe_encl_ps_info_array,
                                                       revision_or_status);
                    if(location_flag)
                    {
                        break;
                    }
                }
            }

            fbe_encl_ps_info_array += fbe_ps_info_size;
        }
    }
       
    return status;
}

/*!**************************************************************
 * @fn fbe_ps_mgmt_print_ps_rev_or_status(const fbe_u8_t * module_name,
 *                                              fbe_trace_func_t trace_func,
 *                                              fbe_trace_context_t trace_context,
 *                                              fbe_encl_ps_info_t * pEnclPsInfo,
 *                                              fbe_bool_t   revision_or_status)
 ****************************************************************
 * @brief
 *  Print firmware revision or firmware upgrade status for the PS in the specified enclosure. 
 *
 * @param module_name - This is the name of the module we are debugging.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 * @param pEnclPsInfo - Ptr to encl ps info.
 * @param revision_or_status - flag for revision or status.
 *
 * @return - FBE_STATUS_OK  
 *
 * @author 
 *  12-Aug-2011 PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_ps_mgmt_print_ps_rev_or_status(const fbe_u8_t * module_name,
                                                fbe_trace_func_t trace_func,
                                                fbe_trace_context_t trace_context,
                                                fbe_dbgext_ptr pEnclPsInfo,
                                                fbe_bool_t   revision_or_status)
{
    fbe_u32_t                        ps_count = 0;
    fbe_u32_t                        psCount;
    fbe_device_physical_location_t   location = {0};
    fbe_u32_t                        fbe_ps_info_size;
    fbe_u32_t                        fbe_fup_info_size;
    fbe_u32_t                        psInfo_offset;
    fbe_u32_t                        psFupInfo_offset;
    fbe_power_supply_info_t          psInfo = {0};
    fbe_base_env_fup_work_state_t    workState;
    fbe_dbgext_ptr                       pWorkItem;
    fbe_base_env_fup_completion_status_t completionStatus;

    FBE_GET_FIELD_OFFSET(module_name, fbe_encl_ps_info_t, "psInfo", &psInfo_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_encl_ps_info_t, "psFupInfo", &psFupInfo_offset);
    FBE_GET_TYPE_SIZE(module_name, fbe_power_supply_info_t, &fbe_ps_info_size);
    FBE_GET_TYPE_SIZE(module_name, fbe_base_env_fup_persistent_info_t, &fbe_fup_info_size);

    FBE_GET_FIELD_DATA(module_name, pEnclPsInfo, fbe_encl_ps_info_t, psCount, sizeof(fbe_u32_t), &psCount);
    FBE_GET_FIELD_DATA(module_name, pEnclPsInfo, fbe_encl_ps_info_t, location, sizeof(fbe_device_physical_location_t), &location);

    for(ps_count =0; ps_count < psCount; ps_count++)
    {
        FBE_GET_FIELD_DATA(module_name,
                pEnclPsInfo + psInfo_offset,
                fbe_power_supply_info_t,
                bInserted,
                sizeof(fbe_bool_t),
                &psInfo.bInserted);

        if(psInfo.bInserted)
        {
            if(revision_or_status)
            {
                FBE_GET_FIELD_DATA(module_name,
                        pEnclPsInfo + psFupInfo_offset,
                        fbe_base_env_fup_persistent_info_t,
                        completionStatus,
                        sizeof(fbe_base_env_fup_completion_status_t),
                        &completionStatus);

                FBE_GET_FIELD_DATA(module_name,
                        pEnclPsInfo + psFupInfo_offset,
                        fbe_base_env_fup_persistent_info_t,
                        pWorkItem,
                        sizeof(fbe_dbgext_ptr),
                        &pWorkItem);

                trace_func(trace_context,"   %3d     %3d     %3d      %s",
                           location.bus,
                           location.enclosure,
                           ps_count,
                           fbe_base_env_decode_fup_completion_status(completionStatus));

                if(completionStatus == FBE_BASE_ENV_FUP_COMPLETION_STATUS_IN_PROGRESS)
                {

                    if(pWorkItem != NULL)
                    {

                        FBE_GET_FIELD_DATA(module_name,
                                        (fbe_dbgext_ptr)pWorkItem,
                                        fbe_base_env_fup_work_item_t,
                                        workState,
                                        sizeof(fbe_base_env_fup_work_state_t),
                                        &workState);

                        trace_func(trace_context,"      %s",fbe_base_env_decode_fup_work_state(workState));
                    }
                }

            }
            else
            {
                FBE_GET_FIELD_DATA(module_name,
                        pEnclPsInfo + psInfo_offset,
                        fbe_power_supply_info_t,
                        firmwareRev,
                        FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1,
                        &psInfo.firmwareRev);

                trace_func(trace_context,"   %3d      %3d      %3d      %s",
                           location.bus,
                           location.enclosure,
                           ps_count,
                           psInfo.firmwareRev);
            }

            trace_func(trace_context, "\n");
        }

        psInfo_offset += fbe_ps_info_size;
        psFupInfo_offset += fbe_fup_info_size;
    }//End of for loop for lcc

    return FBE_STATUS_OK;
}
