/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_ps_mgmt_cmds.c
 ***************************************************************************
 *
 * @brief
 *  This file contains string generation methods of psmgmt
 *
 * @ingroup ps_mgmt_class_files
 *
 * @revision
 *  03/29/2011:  Rashmi Sawale - created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe_ps_mgmt_util.h"
#include "fbe/fbe_pe_types.h"
#include "specl_types.h"

/*!**************************************************************
 * fbe_ps_mgmt_decode_ACDC_input()
 ****************************************************************
 * @brief
 *  This function converts the ACDCInput to a text string.
 *
 * @param 
 *        AC_DC_INPUT             acdcInput
 * 
 * @return
 *        fbe_char_t *string for given AC_DC_INPUT
 * 
 * @author
 *  03/29/2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
fbe_char_t * fbe_ps_mgmt_decode_ACDC_input(AC_DC_INPUT  acdcInput)
{
   switch(acdcInput)
    {
    case INVALID_AC_DC_INPUT:
        return "INVALID";
        break;
    case AC_INPUT:
        return "AC";
        break;
   case DC_INPUT:
        return "DC";
        break;
    default:
        return "UNKNOWN";
        break;
    }
}
/******************************************************
 * end fbe_ps_mgmt_decode_ACDC_input() 
 ******************************************************/

fbe_char_t * fbe_ps_mgmt_decode_expected_ps_type(fbe_ps_mgmt_expected_ps_type_t expected_ps_type)
{
    switch(expected_ps_type)
    {
    case FBE_PS_MGMT_EXPECTED_PS_TYPE_NONE:
        return "None";
        break;
    case FBE_PS_MGMT_EXPECTED_PS_TYPE_OCTANE:
        return "Octane";
        break;
    case FBE_PS_MGMT_EXPECTED_PS_TYPE_OTHER:
        return "Other";
        break;
    default:
        return "Unknown";
        break;
    }
}
        
/*!**************************************************************
 *  fbe_ps_mgmt_decode_ps_state(fbe_ps_state_t state)
 ****************************************************************
 * @brief
 *  Decode the PS state.
 *
 * @param state - The FBE PS state.
 *
 * @return - string of the state. 
 *
 * @author 
 *  9-Mar-2015: Joe Perry - Created.
 *
 ****************************************************************/
fbe_char_t * fbe_ps_mgmt_decode_ps_state(fbe_ps_state_t state)
{
    switch(state)
    {
    case FBE_PS_STATE_UNKNOWN:
        return "UNKNOWN";
        break;
    case FBE_PS_STATE_OK:
        return "OK";
        break;
    case FBE_PS_STATE_DEGRADED:
        return "DEGRADED";
        break;
    case FBE_PS_STATE_FAULTED:
        return "FAULTED";
        break;
    case FBE_PS_STATE_REMOVED:
        return "REMOVED";
        break;
    default:
        return "UNDEFINED";
        break;
    }
}   // end of fbe_ps_mgmt_decode_ps_state
        
/*!**************************************************************
 *  fbe_ps_mgmt_decode_ps_subState(fbe_ps_subState_t subState)
 ****************************************************************
 * @brief
 *  Decode the PS subState.
 *
 * @param state - The FBE PS subState.
 *
 * @return - string of the subState. 
 *
 * @author 
 *  9-Mar-2015: Joe Perry - Created.
 *
 ****************************************************************/
fbe_char_t * fbe_ps_mgmt_decode_ps_subState(fbe_ps_subState_t subState)
{
    switch(subState)
    {
    case FBE_PS_SUBSTATE_NO_FAULT:
        return "NO FAULT";
        break;
    case FBE_PS_SUBSTATE_GEN_FAULT:
        return "GEN FAULT";
        break;
    case FBE_PS_SUBSTATE_FAN_FAULT:
        return "FAN FAULT";
        break;
    case FBE_PS_SUBSTATE_UNSUPPORTED:
        return "UNSUPPORTED";
        break;
    case FBE_PS_SUBSTATE_FLT_STATUS_REG_FAULT:
        return "FLT STATUS REG FAULT";
        break;
    case FBE_PS_SUBSTATE_FUP_FAILURE:
        return "FUP FAIL";
        break;
    case FBE_PS_SUBSTATE_RP_READ_FAILURE:
        return "RP READ FAIL";
        break;
    case FBE_PS_SUBSTATE_ENV_INTF_FAILURE:
        return "ENV INTF FAIL";
        break;
    default:
        return "UNDEFINED";
        break;
    }
}   // end of fbe_ps_mgmt_decode_ps_subState

