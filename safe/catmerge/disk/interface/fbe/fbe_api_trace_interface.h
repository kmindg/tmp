#ifndef FBE_API_TRACE_INTERFACE_H
#define FBE_API_TRACE_INTERFACE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_trace_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions use for interacting with the trace service
 * @ingroup fbe_api_system_package_interface_class_files
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_trace_interface.h"

//----------------------------------------------------------------
// Define the FBE API TRACE Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_trace_interface_usurper_interface FBE API TRACE Interface Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API TRACE Interface class
 *    usurper interface.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------

FBE_API_CPP_EXPORT_START

fbe_status_t FBE_API_CALL fbe_api_trace_reset_error_counters(fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_trace_err_set_notify_level(fbe_trace_level_t trace_level, fbe_package_id_t package_id);

fbe_status_t FBE_API_CALL fbe_api_trace_enable_backtrace(fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_trace_disable_backtrace(fbe_package_id_t package_id);

/*! @} */ /* end of group fbe_api_lun_interface */

FBE_API_CPP_EXPORT_END
#endif /*FBE_API_TRACE_INTERFACE_H*/
