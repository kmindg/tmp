/***************************************************************************
 * Copyright (C) EMC Corporation 2015 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_board_mgmt_utils.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Board Management utils
 *  code.
 * 
 *  This file contains some common utility functions for board management object.
 * 
 * @ingroup board_mgmt_class_files
 * 
 * @version
 *   19-Jan-2015:  PHE - Created.
 *
 ***************************************************************************/
#include "fbe_board_mgmt_utils.h"
#include "fbe/fbe_eses.h"

/*!**************************************************************
 *  fbe_board_mgmt_decode_dimm_state(fbe_dimm_state_t state)
 ****************************************************************
 * @brief
 *  Decode the dimm state.
 *
 * @param state - The FBE dimm state.
 *
 * @return - string of the state. 
 *
 * @author 
 *  16-Jan-2015: PHE - Created.
 *
 ****************************************************************/
fbe_char_t * fbe_board_mgmt_decode_dimm_state(fbe_dimm_state_t state)
{
    switch(state)
    {
    case FBE_DIMM_STATE_UNKNOWN:
        return "UNKNOWN";
        break;
    case FBE_DIMM_STATE_OK:
        return "OK";
        break;
    case FBE_DIMM_STATE_DEGRADED:
        return "DEGRADED";
        break;
    case FBE_DIMM_STATE_FAULTED:
        return "FAULTED";
        break;
    case FBE_DIMM_STATE_EMPTY:
        return "EMPTY";
        break;
    case FBE_DIMM_STATE_MISSING:
        return "MISSING";
        break;
    case FBE_DIMM_STATE_NEED_TO_REMOVE:
        return "NEED TO REMOVE";
        break;
    default:
        return "UNKNOWN";
        break;
    }
}

/*!**************************************************************
 *  fbe_board_mgmt_decode_dimm_subState(fbe_dimm_subState_t subState)
 ****************************************************************
 * @brief
 *  Decode the dimm subState.
 *
 * @param state - The FBE dimm subState.
 *
 * @return - string of the subState. 
 *
 * @author 
 *  24-Feb-2015: Joe Perry - Created.
 *
 ****************************************************************/
fbe_char_t * fbe_board_mgmt_decode_dimm_subState(fbe_dimm_subState_t subState)
{
    switch(subState)
    {
    case FBE_DIMM_SUBSTATE_NO_FAULT:
        return "NO FAULT";
        break;
    case FBE_DIMM_SUBSTATE_GEN_FAULT:
        return "GEN FLT";
        break;
    case FBE_DIMM_SUBSTATE_FLT_STATUS_REG_FAULT:
        return "FLT STATUS REG FLT";
        break;
    case FBE_DIMM_SUBSTATE_HW_ERR_MON_FAULT:
        return "HW ERR MON FLT";
        break;
    case FBE_DIMM_SUBSTATE_ENV_INTF_FAILURE:
        return "ENV INTF FAIL";
        break;
    default:
        return "UNKNOWN";
        break;
    }
}

/*!**************************************************************
 *  fbe_board_mgmt_decode_suitcase_state(fbe_suitcase_state_t state)
 ****************************************************************
 * @brief
 *  Decode the suitcase state.
 *
 * @param state - The FBE suitcase state.
 *
 * @return - string of the state. 
 *
 * @author 
 *  26-Feb-2015: Joe Perry - Created.
 *
 ****************************************************************/
fbe_char_t * fbe_board_mgmt_decode_suitcase_state(fbe_suitcase_state_t state)
{
    switch(state)
    {
    case FBE_SUITCASE_STATE_UNKNOWN:
        return "UNKNOWN";
        break;
    case FBE_SUITCASE_STATE_OK:
        return "OK";
        break;
    case FBE_SUITCASE_STATE_DEGRADED:
        return "DEGRADED";
        break;
    case FBE_SUITCASE_STATE_REMOVED:
        return "REMOVED";
        break;
    case FBE_SUITCASE_STATE_FAULTED:
        return "FAULTED";
        break;
    default:
        return "UNKNOWN";
        break;
    }
}

/*!**************************************************************
 *  fbe_board_mgmt_decode_suitcase_subState(fbe_suitcase_subState_t subState)
 ****************************************************************
 * @brief
 *  Decode the suitcase subState.
 *
 * @param state - The FBE suitcase subState.
 *
 * @return - string of the subState. 
 *
 * @author 
 *  26-Feb-2015: Joe Perry - Created.
 *
 ****************************************************************/
fbe_char_t * fbe_board_mgmt_decode_suitcase_subState(fbe_suitcase_subState_t subState)
{
    switch(subState)
    {
    case FBE_SUITCASE_SUBSTATE_NO_FAULT:
        return "NO FAULT";
        break;
    case FBE_SUITCASE_SUBSTATE_FLT_STATUS_REG_FAULT:
        return "FLT STATUS REG FLT";
        break;
    case FBE_SUITCASE_SUBSTATE_HW_ERR_MON_FAULT:
        return "HW ERR MON FLT";
        break;
    case FBE_SUITCASE_SUBSTATE_LOW_BATTERY:
        return "LOW BATTERY FLT";
        break;
    case FBE_SUITCASE_SUBSTATE_ENV_INTF_FAILURE:
        return "ENV INTF FAIL";
        break;
    case FBE_SUITCASE_SUBSTATE_RP_READ_FAILURE:
        return "RESUME READ FAIL";
        break;
    case FBE_SUITCASE_SUBSTATE_INTERNAL_CABLE_MISSING:
        return "INTERNAL CABLE MISSING";
        break;
    case FBE_SUITCASE_SUBSTATE_INTERNAL_CABLE_CROSSED:
        return "INTERNAL CABLE CROSSED";
        break;

    default:
        return "UNKNOWN";
        break;
    }
}

/*!**************************************************************
 *  fbe_board_mgmt_decode_ssd_state(fbe_ssd_state_t state)
 ****************************************************************
 * @brief
 *  Decode the SSD state.
 *
 * @param state - The FBE SSD state.
 *
 * @return - string of the state. 
 *
 * @author 
 *  26-June-2015: PHE - Created.
 *
 ****************************************************************/
fbe_char_t * fbe_board_mgmt_decode_ssd_state(fbe_ssd_state_t state)
{
    switch(state)
    {
    case FBE_SSD_STATE_UNKNOWN:
        return "UNKNOWN";
        break;
    case FBE_SSD_STATE_OK:
        return "OK";
        break;
    case FBE_SSD_STATE_DEGRADED:
        return "DEGRADED";
        break;
    case FBE_SSD_STATE_FAULTED:
        return "FAULTED";
        break;
    default:
        return "UNKNOWN";
        break;
    }
}

/*!**************************************************************
 *  fbe_board_mgmt_decode_ssd_subState(fbe_ssd_subState_t subState)
 ****************************************************************
 * @brief
 *  Decode the SSD subState.
 *
 * @param state - The FBE SSD subState.
 *
 * @return - string of the subState. 
 *
 * @author 
 *  07-Aug-2015: PHE - Created.
 *
 ****************************************************************/
fbe_char_t * fbe_board_mgmt_decode_ssd_subState(fbe_ssd_subState_t subState)
{
    switch(subState)
    {
    case FBE_SSD_SUBSTATE_NO_FAULT:
        return "NO FAULT";
        break;
    case FBE_SSD_SUBSTATE_GEN_FAULT:
        return "GEN FLT";
        break;
    case FBE_SSD_SUBSTATE_FLT_STATUS_REG_FAULT:
        return "FLT STATUS REG FLT";
        break;
    default:
        return "UNKNOWN";
        break;
    }
}

// end of fbe_board_mgmt_utils.c
