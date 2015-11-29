#ifndef FBE_ENCL_MGMT_UTILS_H
#define FBE_ENCL_MGMT_UTILS_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2013
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
 *  05-March-2013 - Created. Chris Gould
 *
 ***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe_trace.h"
#include "fbe/fbe_devices.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"

fbe_char_t * fbe_encl_mgmt_get_encl_type_string(fbe_esp_encl_type_t enclosure_type);
fbe_char_t * fbe_encl_mgmt_get_enclShutdownReasonString(fbe_u32_t shutdownReason);
char * fbe_encl_mgmt_decode_encl_state(fbe_esp_encl_state_t enclState);
char * fbe_encl_mgmt_decode_encl_fault_symptom(fbe_esp_encl_fault_sym_t enclFaultSymptom);
const char *fbe_encl_mgmt_getCmiMsgTypeString(fbe_u32_t MsgType);

fbe_char_t * fbe_encl_mgmt_decode_lcc_state(fbe_lcc_state_t lccState);
fbe_char_t * fbe_encl_mgmt_decode_lcc_subState(fbe_lcc_subState_t lccSubState);

#endif  /* FBE_ENCL_MGMT_UTILS_H*/
