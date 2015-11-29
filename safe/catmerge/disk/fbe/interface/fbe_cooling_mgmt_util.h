#ifndef FBE_COOLING_MGMT_UTIL_H
#define FBE_COOLING_MGMT_UTIL_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_cooling_mgmt_util.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the Cooling Mgmt debug library.
 *
 * @author
 *  10-March-2015 - Created. Joe Perry
 *
 ***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/

//#include "fbe/fbe_types.h"
//#include "fbe_trace.h"
//#include "fbe/fbe_devices.h"
//#include "specl_types.h"
#include "fbe/fbe_pe_types.h"

/* String generation methods */
fbe_char_t * fbe_cooling_mgmt_decode_fan_state(fbe_fan_state_t state);
fbe_char_t * fbe_cooling_mgmt_decode_fan_subState(fbe_fan_subState_t subState);

#endif  /* FBE_COOLING_MGMT_UTIL_H*/
