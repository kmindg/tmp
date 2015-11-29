#ifndef FBE_ENCL_MGMT_DEBUG_H
#define FBE_ENCL_MGMT_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_encl_mgmt_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the Enclosure Mgmt debug library.
 *
 * @author
 *  06-May-2010 - Created. Vaibhav Gaonkar
 *
 ***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"
#include "fbe/fbe_devices.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"

fbe_status_t fbe_encl_mgmt_debug_trace(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr encl_mgmt_p,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context);
fbe_status_t fbe_encl_mgmt_debug_stat(const fbe_u8_t * module_name,
                                            fbe_dbgext_ptr encl_mgmt_p,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context,
                                            fbe_device_physical_location_t *plocation,
                                            fbe_bool_t   revision_or_status,
                                            fbe_bool_t   location_flag);
fbe_char_t * fbe_encl_mgmt_get_encl_type_string(fbe_esp_encl_type_t enclosure_type);
fbe_char_t * fbe_encl_mgmt_get_enclShutdownReasonString(fbe_u32_t shutdownReason);
char * fbe_encl_mgmt_decode_encl_state(fbe_esp_encl_state_t enclState);
char * fbe_encl_mgmt_decode_encl_fault_symptom(fbe_esp_encl_fault_sym_t enclFaultSymptom);
const char *fbe_encl_mgmt_getCmiMsgTypeString(fbe_u32_t MsgType);
#endif  /* FBE_ENCL_MGMT_DEBUG_H*/
