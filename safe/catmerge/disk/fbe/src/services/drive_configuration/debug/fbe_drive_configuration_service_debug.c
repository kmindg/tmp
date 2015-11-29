/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_drive_configuration_service_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for the Drive Configuration
 *  Service.
 *
 * @author
 *  11/15/2012  Wayne Garrett - Created.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe/fbe_drive_configuration_interface.h"
#include "fbe_drive_configuration_service_debug.h" 


static void  fbe_dcs_print_control_flags(fbe_dcs_control_flags_t control_flags, fbe_trace_func_t trace_func, fbe_trace_context_t trace_context);



/*!**************************************************************
 * @fn fbe_drive_configuration_service_debug_trace()
 ****************************************************************
 * @brief
 *  Display all the trace information about the drive configuration
 *  service.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param drive_mgmt_p - Ptr to Drive Mgmt object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 * @param spaces  - name spaces to indent output
 *
 * @return - void
 *
 * @author 11/15/2012  Wayne Garrett - Created.
 *
 ****************************************************************/
void fbe_dcs_parameters_debug_trace(const fbe_u8_t * module_name,
                                    fbe_dbgext_ptr dcs_p,
                                    fbe_trace_func_t trace_func,
                                    fbe_trace_context_t trace_context,
                                    fbe_u32_t spaces)
{
    fbe_dcs_tunable_params_t pdo_params;

    FBE_GET_FIELD_DATA(module_name,
                       dcs_p,
                       fbe_drive_configuration_service_t,
                       pdo_params,
                       sizeof(fbe_dcs_tunable_params_t),
                       &pdo_params);

    fbe_trace_indent(trace_func, trace_context, spaces);
    trace_func(trace_context, "PARAMETERS\n");

    fbe_trace_indent(trace_func, trace_context, spaces);
    trace_func(trace_context, "control_flags:        (0x%x) ", (unsigned int)pdo_params.control_flags);
    fbe_dcs_print_control_flags(pdo_params.control_flags, trace_func, trace_context);
    trace_func(trace_context, "\n");

    fbe_trace_indent(trace_func, trace_context, spaces);
    trace_func(trace_context, "service timeout:       %d ms\n", (fbe_u32_t)pdo_params.service_time_limit_ms);

    fbe_trace_indent(trace_func, trace_context, spaces);
    trace_func(trace_context, "remap service timeout: %d ms\n", (fbe_u32_t)pdo_params.remap_service_time_limit_ms);

    fbe_trace_indent(trace_func, trace_context, spaces);
    trace_func(trace_context, "fw image chunk size: %d bytes\n", (fbe_u32_t)pdo_params.fw_image_chunk_size);
}



/*!**************************************************************
 * fbe_dcs_print_control_flags
 ****************************************************************
 * @brief
 *  Display's the control flag bits
 *
 * @param control_flags - flags to print
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - void  
 *
 * @author
 *  11/15/2012 Wayne Garrett - Created. 
 *
 ****************************************************************/
static void 
fbe_dcs_print_control_flags(fbe_dcs_control_flags_t control_flags,
                            fbe_trace_func_t trace_func,
                            fbe_trace_context_t trace_context)
{
    fbe_dcs_control_flags_t flag = 0;

    if (flag = (control_flags & FBE_DCS_HEALTH_CHECK_ENABLED))
    {
        trace_func(trace_context, "HEALTH_CHECK | ");
        control_flags &= ~flag;
    }

    if (flag = (control_flags & FBE_DCS_REMAP_RETRIES_ENABLED))
    {
        trace_func(trace_context, "REMAP_RETRIES | ");
        control_flags &= ~flag;
    }

    if (flag = (control_flags & FBE_DCS_REMAP_HC_FOR_NON_RETRYABLE))
    {
        trace_func(trace_context, "REMAP_NONRETRY_HC | ");
        control_flags &= ~flag;
    }

    if (flag = (control_flags & FBE_DCS_FWDL_FAIL_AFTER_MAX_RETRIES))
    {
        trace_func(trace_context, "FWDL_FAIL_MAX_RETRY | ");
        control_flags &= ~flag;
    }    

    if (flag = (control_flags & FBE_DCS_ENFORCE_ENHANCED_QUEUING_DRIVES))
    {
        trace_func(trace_context, "ENFORCE_EQ_DRIVES | ");
        control_flags &= ~flag;
    }

    if (flag = (control_flags & FBE_DCS_DRIVE_LOCKUP_RECOVERY))
    {
        trace_func(trace_context, "LOCKUP_RECOVERY | ");
        control_flags &= ~flag;
    }

    if (flag = (control_flags & FBE_DCS_DRIVE_TIMEOUT_QUIESCE_HANDLING))
    {
        trace_func(trace_context, "TIMEOUT_HNDLING | ");
        control_flags &= ~flag;
    }    
    
    if (flag = (control_flags & FBE_DCS_PFA_HANDLING))
    {
        trace_func(trace_context, "PFA | ");
        control_flags &= ~flag;
    }    

    if (flag = (control_flags & FBE_DCS_MODE_SELECT_CAPABILITY))
    {
        trace_func(trace_context, "MS | ");
        control_flags &= ~flag;
    }        

    if (flag = (control_flags & FBE_DCS_BREAK_UP_FW_IMAGE))
    {
        trace_func(trace_context, "BREAKUP_FW | ");
        control_flags &= ~flag;
    }

    if (flag = (control_flags & FBE_DCS_4K_ENABLED))
    {
        trace_func(trace_context, "4K_ENABLED | ");
        control_flags &= ~flag;
    }
    if (flag = (control_flags & FBE_DCS_IGNORE_INVALID_IDENTITY))
    {
        trace_func(trace_context, "IGNORE_INVALID_IDENTITY | ");
        control_flags &= ~flag;
    }
    if (control_flags != 0)
    {
        trace_func(trace_context, "remaining flags: 0x%llx ", control_flags);
    }
}

