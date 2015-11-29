/***************************************************************************
 * Copyright (C) EMC Corporation 2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cooling_mgmt_utils.c
 ***************************************************************************
 *
 * @brief
 *  This file contains string generation methods of cooling_mgmt
 *
 * @ingroup ps_mgmt_class_files
 *
 * @revision
 *  10-Mar-2015:  Joe Perry - created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe_cooling_mgmt_util.h"
#include "fbe/fbe_pe_types.h"
#include "specl_types.h"


/*!**************************************************************
 *  fbe_cooling_mgmt_decode_fan_state(fbe_fan_state_t state)
 ****************************************************************
 * @brief
 *  Decode the Fan state.
 *
 * @param state - The FBE Fan state.
 *
 * @return - string of the state. 
 *
 * @author 
 *  26-Feb-2015: Joe Perry - Created.
 *
 ****************************************************************/
fbe_char_t * fbe_cooling_mgmt_decode_fan_state(fbe_fan_state_t state)
{
    switch(state)
    {
    case FBE_FAN_STATE_UNKNOWN:
        return "UNKNOWN";
        break;
    case FBE_FAN_STATE_OK:
        return "OK";
        break;
    case FBE_FAN_STATE_DEGRADED:
        return "DEGRADED";
        break;
    case FBE_FAN_STATE_REMOVED:
        return "REMOVED";
        break;
    case FBE_FAN_STATE_FAULTED:
        return "FAULTED";
        break;
    default:
        return "UNKNOWN";
        break;
    }
}   // end of fbe_cooling_mgmt_decode_fan_state

/*!**************************************************************
 *  fbe_cooling_mgmt_decode_fan_subState(fbe_fan_subState_t subState)
 ****************************************************************
 * @brief
 *  Decode the Fan subState.
 *
 * @param state - The FBE Fan subState.
 *
 * @return - string of the subState. 
 *
 * @author 
 *  26-Feb-2015: Joe Perry - Created.
 *
 ****************************************************************/
fbe_char_t * fbe_cooling_mgmt_decode_fan_subState(fbe_fan_subState_t subState)
{
    switch(subState)
    {
    case FBE_FAN_SUBSTATE_NO_FAULT:
        return "NO FAULT";
        break;
    case FBE_FAN_SUBSTATE_FLT_STATUS_REG_FAULT:
        return "FLT STATUS REG FAULT";
        break;
    case FBE_FAN_SUBSTATE_FUP_FAILURE:
        return "FUP FAIL";
        break;
    case FBE_FAN_SUBSTATE_RP_READ_FAILURE:
        return "RP READ FAIL";
        break;
    case FBE_FAN_SUBSTATE_ENV_INTF_FAILURE:
        return "ENV INTF FAIL";
        break;
    default:
        return "UNKNOWN";
        break;
    }
}   // end of fbe_cooling_mgmt_decode_fan_subState
