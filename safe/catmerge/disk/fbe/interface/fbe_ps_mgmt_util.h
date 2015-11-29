#ifndef FBE_PS_MGMT_UTIL_H
#define FBE_PS_MGMT_UTIL_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_ps_mgmt_util.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the PS Mgmt debug library.
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
#include "specl_types.h"
#include "fbe/fbe_pe_types.h"

/* String generation methods */
fbe_char_t * fbe_ps_mgmt_decode_ACDC_input(AC_DC_INPUT  acdcInput);
fbe_char_t * fbe_ps_mgmt_decode_expected_ps_type(fbe_ps_mgmt_expected_ps_type_t expected_ps_type);

fbe_char_t * fbe_ps_mgmt_decode_ps_state(fbe_ps_state_t state);
fbe_char_t * fbe_ps_mgmt_decode_ps_subState(fbe_ps_subState_t subState);

#endif  /* FBE_PS_MGMT_UTIL_H*/
