#ifndef FBE_DRIVE_CONFIG_SERVICE_DEBUG_H
#define FBE_DRIVE_CONFIG_SERVICE_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_drive_configuration_service_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces for the drive configuration service
 *  debugger extensions
 *
 * @author
 *   11/15/2012  Wayne Garrett - Created. 
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"

/*TODO: create a debug trace for dieh and download*/

void fbe_dcs_parameters_debug_trace(const fbe_u8_t * module_name,
                                    fbe_dbgext_ptr dcs_p,
                                    fbe_trace_func_t trace_func,
                                    fbe_trace_context_t trace_context,
                                    fbe_u32_t spaces);



/*************************
 * end file fbe_drive_configuration_service_debug.h
 *************************/ 

#endif /* FBE_DRIVE_CONFIG_SERVICE_DEBUG_H */