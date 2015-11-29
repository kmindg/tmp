/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_module_mgmt_string_util.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Module Management utility functionalities
 *  for transforming enums to text strings.
 * 
 * @ingroup module_mgmt_class_files
 * 
 * @version
 *   28-Sep-2010 - Created. bphilbin
 *
 ***************************************************************************/

#include "fbe_module_mgmt_private.h"
#include "fbe/fbe_module_info.h"
#include "fbe/fbe_board.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_module_mgmt_mapping.h"

/*!**************************************************************
 * fbe_module_mgmt_slic_type_to_string()
 ****************************************************************
 * @brief
 *  This function converts the specified SLIC type to a text string.
 *
 * @param - slic_type
 * 
 * @return - fbe_char_t * string name for the SLIC
 * 
 * @author
 *  23-Sep-2010:  Created. bphilbin 
 *
 ****************************************************************/
const fbe_char_t * fbe_module_mgmt_slic_type_to_string(fbe_module_slic_type_t slic_type)
{
    fbe_u32_t i = 0;
    for (i = 0; i < fbe_slic_type_property_map_size; i++)
    {
        if (fbe_slic_type_property_map[i].slic_type == slic_type)
        {
            return fbe_slic_type_property_map[i].name;
        }
    }

    return "Unknown";
}

/*!**************************************************************
 * fbe_module_mgmt_device_type_to_string()
 ****************************************************************
 * @brief
 *  This function converts the specified device type to a text string.
 *
 * @param - slic_type
 * 
 * @return - fbe_char_t * string name for the device type
 * 
 * @author
 *  23-Sep-2010:  Created. bphilbin 
 *
 ****************************************************************/
const fbe_char_t * fbe_module_mgmt_device_type_to_string(fbe_u64_t device_type)
{
    switch (device_type)
    {
    case FBE_DEVICE_TYPE_IOMODULE:
        return "SLIC";
        break;
    case FBE_DEVICE_TYPE_MEZZANINE:
        return "Mezzanine";
        break;
    case FBE_DEVICE_TYPE_MGMT_MODULE:
        return "Mgmt Module";
        break;
    case FBE_DEVICE_TYPE_BACK_END_MODULE:
        return "Base Module";
        break;
    default:
        return "Unknown";
        break;
    }
}


/*!**************************************************************
 * fbe_module_mgmt_module_state_to_string()
 ****************************************************************
 * @brief
 *  This function converts the module state to a text string.
 *
 * @param - state - module state
 * 
 * @return - fbe_char_t * string name for the state
 * 
 * @author
 *  23-Sep-2010:  Created. bphilbin 
 *
 ****************************************************************/
const fbe_char_t * fbe_module_mgmt_module_state_to_string(fbe_module_state_t state)
{
    switch(state)
    {
    case MODULE_STATE_UNINITIALIZED:
        return "UNINITI"; //"uninitialized";
        break;
    case MODULE_STATE_EMPTY:
        return "EMPTY"; //"empty";
        break;
    case MODULE_STATE_MISSING:
        return "MISSING"; //missing;
        break;
    case MODULE_STATE_FAULTED:
        return "FAULTED";//"faulted";
        break;
    case MODULE_STATE_ENABLED:
        return "ENABLED"; //"enabled";
        break;
    case MODULE_STATE_UNSUPPORTED_MODULE:
        return "UNSUPP";//"io module unsupported";
        break;
    default:
        return "UNKNOWN";//"unknown module state";
        break;
    }
}


/*!**************************************************************
 * fbe_module_mgmt_module_substate_to_string()
 ****************************************************************
 * @brief
 *  This function converts the module substate to a text string.
 *
 * @param - substate - module substate
 * 
 * @return - fbe_char_t * string name for the substate
 * 
 * @author
 *  23-Sep-2010:  Created. bphilbin 
 *
 ****************************************************************/
const fbe_char_t * fbe_module_mgmt_module_substate_to_string(fbe_module_substate_t substate)
{
    switch(substate)
    {
    case MODULE_SUBSTATE_NOT_PRESENT:
        return "N_PRES"; //"Not Present";
        break;
    case MODULE_SUBSTATE_MISSING:
        return "MISSING"; //"Missing";
        break;
    case MODULE_SUBSTATE_PROM_READ_ERROR:
        return "PROM_RD_ERR"; //"Resume Read Error";
        break;
    case MODULE_SUBSTATE_INCORRECT_MODULE:
        return "INC_IOM";//"Incorrect Type";
        break;
    case MODULE_SUBSTATE_UNSUPPORTED_MODULE:
        return "UNSUP_IOM"; //"Unsupported";
        break;
    case MODULE_SUBSTATE_GOOD:
        return "GOOD"; //"Good";
        break;
    case MODULE_SUBSTATE_POWERED_OFF:
        return "IOM_PWR_OFF"; //"Powered Off";
        break;
    case MODULE_SUBSTATE_POWERUP_FAILED:
        return "IOM_PWR_FAIL";//"Powerup Failed";
        break;
    case MODULE_SUBSTATE_FAULT_REG_FAILED:
        return "FaultReg Fault";//"Fault register Indictment";
        break;
    case MODULE_SUBSTATE_UNSUPPORTED_NOT_COMMITTED:
        return "NSUP_NCOM";//"Bundle not Committed";
        break;
    case MODULE_SUBSTATE_INTERNAL_FAN_FAULTED:
        return "INTERNAL_FAN_FAULT";//"Internal fan faulted";
        break;
    case MODULE_SUBSTATE_EXCEEDED_MAX_LIMITS:
        return "EXCEEDS_LIMIT"; // "exceeds platform limit";
        break;
    case MODULE_SUBSTATE_HW_ERR_MON_FAULT:
        return "HwErrMon Fault"; // "exceeds platform limit";
        break;
    default:
        return "UNKNOWN";//"Unknown Substate";
        break;
    }
}


/*!**************************************************************
 * fbe_module_mgmt_port_state_to_string()
 ****************************************************************
 * @brief
 *  This function converts the port state to a text string.
 *
 * @param - state - port state
 * 
 * @return - fbe_char_t * string name for the state
 * 
 * @author
 *  23-Sep-2010:  Created. bphilbin 
 *
 ****************************************************************/
const fbe_char_t * fbe_module_mgmt_port_state_to_string(fbe_port_state_t state)
{
    switch(state)
    {
    case FBE_PORT_STATE_UNINITIALIZED:
        return "UNINITI"; //"uninitialized";
        break;
    case FBE_PORT_STATE_UNKNOWN:
        return "UNKNOWN"; //"unknown";
        break;
    case FBE_PORT_STATE_EMPTY:
        return "EMPTY"; //"empty";
        break;
    case FBE_PORT_STATE_MISSING:
        return "MISSING"; //"missing";
        break;
    case FBE_PORT_STATE_FAULTED:
        return "FAULTED"; //"faulted";
        break;
    case FBE_PORT_STATE_ENABLED:
        return "ENABLED"; //"enabled";
        break;
    case FBE_PORT_STATE_UNAVAILABLE:
        return "UNAVAIL"; //"unavailable";
        break;
    case FBE_PORT_STATE_DISABLED:
        return "DISABLED"; //"disabled";
        break;
    default:
        return "UNKNOWN";//"unknown port state";
        break;
    }
}

/*!**************************************************************
 * fbe_module_mgmt_port_substate_to_string()
 ****************************************************************
 * @brief
 *  This function converts the port substate to a text string.
 *
 * @param - substate - port substate
 * 
 * @return - fbe_char_t * string name for the substate
 * 
 * @author
 *  23-Sep-2010:  Created. bphilbin 
 *
 ****************************************************************/
const fbe_char_t * fbe_module_mgmt_port_substate_to_string(fbe_port_substate_t substate)
{
    switch(substate)
    {
    case FBE_PORT_SUBSTATE_UNINITIALIZED:
        return "UNINITI";                  //Uninitialized
        break;
    case FBE_PORT_SUBSTATE_SFP_NOT_PRESENT:
        return "SFP_NP";         //SFP not present
        break;
    case FBE_PORT_SUBSTATE_MODULE_NOT_PRESENT:
        return "IOM_NP";        //"IO module not present"
        break;
    case FBE_PORT_SUBSTATE_PORT_NOT_PRESENT:
        return "NP";                //"Port not present"
        break;
    case FBE_PORT_SUBSTATE_MISSING_SFP:
        return  "MISS_SFP";      //"SFP missing"
        break;
    case FBE_PORT_SUBSTATE_MISSING_MODULE:
        return "MISS_IOM";  //IO module missing
        break;
    case FBE_PORT_SUBSTATE_INCORRECT_SFP_TYPE:
        return "INC_SFP_T";    //incorrect SFP type
        break;
    case FBE_PORT_SUBSTATE_INCORRECT_MODULE:
        return "INC_IOM";     //incorrect IO module
        break;
    case FBE_PORT_SUBSTATE_SFP_READ_ERROR:
        return  "SFP_RD_ER";         //SFP read error
        break;
    case FBE_PORT_SUBSTATE_UNSUPPORTED_SFP:
        return  "UNSUP_SFP";            //SFP unsupported
        break;
    case FBE_PORT_SUBSTATE_MODULE_READ_ERROR:
        return "IOM_RD_ER";    //IO module read error
        break;
    case FBE_PORT_SUBSTATE_EXCEEDED_MAX_LIMITS:
        return "EX_MAX_LIM";   //exceeds platform limit
        break;
    case FBE_PORT_SUBSTATE_MODULE_POWERED_OFF:
        return "IOM_PWR_OFF";  //IO module powered off
        break;
    case FBE_PORT_SUBSTATE_UNSUPPORTED_MODULE:
        return "IOM_UNSUP";   //unsupported IO module
        break;
    case FBE_PORT_SUBSTATE_GOOD:
        return "GOOD"; //Good
        break;
    case FBE_PORT_SUBSTATE_DB_READ_ERROR:
        return "DB_RD_ERR";  //persistent data load failure
        break;
    case FBE_PORT_SUBSTATE_FAULTED_SFP:
        return "SFP_FAULT";    //SFP faulted
        break;
    case FBE_PORT_SUBSTATE_HW_FAULT:
        return  "HW_FAULT";  //hardware fault
        break;
    case FBE_PORT_SUBSTATE_UNAVAILABLE:
        return "UNAVAIL";  //unavailable
        break;
    case FBE_PORT_SUBSTATE_DISABLED_USER_INIT:
        return "DIS_USER"; //disabled, user initiated
        break;
    case FBE_PORT_SUBSTATE_DISABLED_ENCRYPTION_REQUIRED:
        return "DIS_ENCRY_REQ";//disabled, encryption required
        break;
    case FBE_PORT_SUBSTATE_DISABLED_HW_FAULT:
        return "DIS_HW_FLT";//disabled, hardware fault
        break;
    default:
        return "UNKWN";  //unknown port substate
        break;
    }
}


const fbe_char_t * fbe_module_mgmt_cmi_msg_to_string(fbe_module_mgmt_cmi_msg_code_t msg_code)
{
    switch(msg_code)
    {
    case FBE_MODULE_MGMT_PORTS_CONFIGURED_MSG:
        return "ports configured";
        break;
    case FBE_MODULE_MGMT_SLIC_UPGRADE_MSG:
        return "slic upgrade";
        break;
    case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_GRANT:
        return "fup peer permissoin grant";
        break;
    case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_DENY:
        return "fup peer permission deny";
        break;
    case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_REQUEST:
        return "fup peer permission request";
    default:
        return "unknown message code";
        break;
    }
}

/*!**************************************************************
 * fbe_module_mgmt_mgmt_status_to_string()
 ****************************************************************
 * @brief
 *  This function converts the mgmt status to a text string.
 *
 * @param - fbe_mgmt_status_t mgmt_status
 * 
 * @return - fbe_char_t * string for mgmt status
 * 
 * @author
 *  18-Jan-2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
const fbe_char_t * fbe_module_mgmt_mgmt_status_to_string(fbe_mgmt_status_t mgmt_status)
{
    switch(mgmt_status)
    {
    case FBE_MGMT_STATUS_FALSE:
        return "N";
        break;
    case FBE_MGMT_STATUS_TRUE:
        return "Y";
        break;
   case FBE_MGMT_STATUS_UNKNOWN:
        return "UNKNOWN";
        break;
   case FBE_MGMT_STATUS_NA:
        return "NA";
        break;
    default:
        return "UNKNOWN";//"unknown status code";
        break;
    }
}

/*!**************************************************************
 * fbe_module_mgmt_port_role_to_string()
 ****************************************************************
 * @brief
 *  This function converts the port role to a text string.
 *
 * @param -  fbe_port_role_t port_role
 * 
 * @return - fbe_char_t * string for port role
 * 
 * @author
 *  18-Jan-2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
const fbe_char_t * fbe_module_mgmt_port_role_to_string(fbe_ioport_role_t port_role)
{
     switch(port_role)
    {
    case FBE_PORT_ROLE_INVALID:
	 return "INVALID";
        break;
    case FBE_PORT_ROLE_FE:
        return "FE";
        break;
   case FBE_PORT_ROLE_BE:
        return "BE";
        break;
    default:
        return "UNKNOWN";
        break;
    }
}

/*!**************************************************************
 * fbe_module_mgmt_port_subrole_to_string()
 ****************************************************************
 * @brief
 *  This function converts the port subrole to a text string.
 *
 * @param - fbe_port_subrole_t port_subrole
 * 
 * @return - fbe_char_t * string for port subrole
 * 
 * @author
 *  18-Jan-2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
const fbe_char_t * fbe_module_mgmt_port_subrole_to_string(fbe_port_subrole_t port_subrole)
{
     switch(port_subrole)
    {
    case FBE_PORT_SUBROLE_UNINTIALIZED:
	 return "INVALID";
        break;
    case FBE_PORT_SUBROLE_SPECIAL:
        return "SPEC";
        break;
   case FBE_PORT_SUBROLE_NORMAL:
        return "NORM";
        break;
    default:
        return "UNKNOWN";
        break;
    }
}

/*!**************************************************************
 * fbe_module_mgmt_protocol_to_string()
 ****************************************************************
 * @brief
 *  This function converts the protocol to a text string.
 *
 * @param - IO_CONTROLLER_PROTOCOL protocol
 * 
 * @return - fbe_char_t * string for protocol
 * 
 * @author
 *  18-Jan-2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
const fbe_char_t * fbe_module_mgmt_protocol_to_string(IO_CONTROLLER_PROTOCOL protocol)
{
     switch(protocol)
    {
     case IO_CONTROLLER_PROTOCOL_FIBRE:
          return "FC";
          break;
     case IO_CONTROLLER_PROTOCOL_ISCSI:
          return "iSCSI";
          break;
     case IO_CONTROLLER_PROTOCOL_SAS:
          return "SAS";
          break;
     case IO_CONTROLLER_PROTOCOL_FCOE:
          return "FCoE";
          break;
     case IO_CONTROLLER_PROTOCOL_AGNOSTIC:
          return "AGNO";
          break;
     case IO_CONTROLLER_PROTOCOL_UNKNOWN:
     default:
          return "UNKNO";
          break;
    }
}

/*!**************************************************************
 * fbe_module_mgmt_power_status_to_string()
 ****************************************************************
 * @brief
 *  This function converts the power status to a text string.
 *
 * @param - fbe_power_status_t power_status
 * 
 * @return - fbe_char_t * string for power_status
 * 
 * @author
 *  18-Jan-2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
const fbe_char_t * fbe_module_mgmt_power_status_to_string(fbe_power_status_t power_status)
{
    switch(power_status)
    {
     case FBE_POWER_STATUS_POWER_ON:
          return "GOOD";
          break;
     case FBE_POWER_STATUS_POWERUP_FAILED:
          return "FAILED";  //"Power failed";
          break;
     case FBE_POWER_STATUS_POWER_OFF:
          return "OFF";  //disabled
          break;
     case FBE_POWER_STATUS_NA:
          return "NA";
          break;
     case FBE_POWER_STATUS_UNKNOWN:
     default:
          return "UNKNOWN";
          break;
    }
}

/*!**************************************************************
 * fbe_module_mgmt_supported_speed_to_string()
 ****************************************************************
 * @brief
 *  This function converts the supported_speed to a text string.
 *
 * @param - fbe_u32_t   sfp_status
 * 
 * @return - fbe_char_t * string for supported_speed
 * 
 * @author
 *  18-Jan-2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
const fbe_char_t * fbe_module_mgmt_supported_speed_to_string(fbe_u32_t supported_speed)
{
    fbe_char_t speedStr[80];
    fbe_char_t *sfpSpeedsPtr=speedStr;
    memset (sfpSpeedsPtr, 0, 80);

    _snprintf(sfpSpeedsPtr, sizeof(speedStr), "0x%x -", supported_speed);

    if (supported_speed & 0x00000010)   //SPEED_TEN_GIGABIT
    {
        strncat(sfpSpeedsPtr, " 10G", sizeof(speedStr)-strlen(speedStr)-1);
    }
    if (supported_speed & 0x00000008)  //SPEED_EIGHT_GIGABIT
    {
        strncat(sfpSpeedsPtr, " 8G", sizeof(speedStr)-strlen(speedStr)-1);
    }
    if (supported_speed & 0x00000004)   //SPEED_FOUR_GIGABIT
    {
        strncat (sfpSpeedsPtr, " 4G", sizeof(speedStr)-strlen(speedStr)-1);
    }
    if (supported_speed & 0x00000002)   //SPEED_TWO_GIGABIT
    {
        strncat (sfpSpeedsPtr, " 2G", sizeof(speedStr)-strlen(speedStr)-1);
    }//
    if (supported_speed & 0x00000001)  //SPEED_ONE_GIGABIT
    {
        strncat (sfpSpeedsPtr, " 1G", sizeof(speedStr)-strlen(speedStr)-1);
    }
    if (supported_speed &  0x00000080)    //SPEED_SIX_GIGABIT
    {
        strncat (sfpSpeedsPtr, " 6G", sizeof(speedStr)-strlen(speedStr)-1);
    }
    return (sfpSpeedsPtr);
}


/*!**************************************************************
 * fbe_module_mgmt_conversion_type_to_string()
 ****************************************************************
 * @brief
 *  This function converts the protocol to a text string.
 *
 * @param - fbe_module_mgmt_conversion_type_t - conversion type
 * 
 * @return - fbe_char_t * string for conversion type
 * 
 * @author
 *  14-March-2011:  Created. bphilbin 
 *
 ****************************************************************/
const fbe_char_t * fbe_module_mgmt_conversion_type_to_string(fbe_module_mgmt_conversion_type_t type)
{
     switch(type)
    {
     case FBE_MODULE_MGMT_CONVERSION_NONE:
          return "No Conversion";
          break;
     case FBE_MODULE_MGMT_CONVERSION_HCL_TO_SENTRY:
          return "Hellcat Lite to other Sentry";
          break;
     case FBE_MODULE_MGMT_CONVERSION_SENTRY_TO_ARGONAUT:
          return "Sentry to Argonaut";
          break;
     default:
          return "Unknown Conversion Type";
          break;
    }
}

/*!**************************************************************
 * fbe_module_mgmt_convert_general_fault_to_string()
 ****************************************************************
 * @brief
 *  This function convert the mgmtStatus to a text string
 *
 * @param :
 *   mgmtStatus - fbe_mgmt_status_t
 *
 * @return fbe_char_t * string for conversion type
 *
 * @author
 *  15-April-2011: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
const fbe_char_t *
fbe_module_mgmt_convert_general_fault_to_string(fbe_mgmt_status_t  mgmtStatus)
{
    switch(mgmtStatus)
    {
        case FBE_MGMT_STATUS_FALSE:
            return "FALSE";
            break;
            
        case FBE_MGMT_STATUS_TRUE:
            return "TRUE";
            break;
            
        case FBE_MGMT_STATUS_UNKNOWN:
            return "Unknown";
            break;

        case FBE_MGMT_STATUS_NA:
            return "NA";
            break;

        default:
            return "Invalid";
            break;
    }
}
/************************************************************
 *  end of fbe_module_mgmt_convert_general_fault_to_string()
 ***********************************************************/
/*!**************************************************************
 * fbe_module_mgmt_convert_env_interface_status_to_string()
 ****************************************************************
 * @brief
 *  This function convert the environment interface status to a text string
 *
 * @param :
 *   envInterfaceStatus - fbe_env_inferface_status_t
 *
 * @return fbe_char_t * string for conversion type
 *
 * @author
 *  15-April-2011: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
const fbe_char_t *
fbe_module_mgmt_convert_env_interface_status_to_string(fbe_env_inferface_status_t   envInterfaceStatus)
{
    switch(envInterfaceStatus)
    {
        case FBE_ENV_INTERFACE_STATUS_GOOD:
            return "Good";
            break;

        case FBE_ENV_INTERFACE_STATUS_XACTION_FAIL:
            return "Transaction Failed";
            break;

        case FBE_ENV_INTERFACE_STATUS_DATA_STALE:
            return "Stale data";
            break;

        case FBE_ENV_INTERFACE_STATUS_NA:
            return "NA";
            break;

        default:
            return "Unknown";
            break;
    }

}
/*******************************************************************
 *  end of fbe_module_mgmt_convert_env_interface_status_to_string()
 ******************************************************************/
/*!**************************************************************
 * fbe_module_mgmt_convert_mgmtPortAutoNeg_to_string()
 ****************************************************************
 * @brief
 *  This function convert the Mgmt port autonegation mode to a text string
 *
 * @param :
 *   portAutoNeg - fbe_module_mgmt_port_auto_neg_t
 *
 * @return fbe_char_t * string for conversion type
 *
 * @author
 *  15-April-2011: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
const fbe_char_t * 
fbe_module_mgmt_convert_mgmtPortAutoNeg_to_string(fbe_mgmt_port_auto_neg_t portAutoNeg)
{
    switch(portAutoNeg)
    {
        case FBE_PORT_AUTO_NEG_OFF:
            return "OFF";
            break;

        case FBE_PORT_AUTO_NEG_ON:
            return "ON";
            break;

        case FBE_PORT_AUTO_NEG_UNSPECIFIED:
            return "UNSPECIFIED";
            break;

        case FBE_PORT_AUTO_NEG_INDETERMIN:
            return "INDETERMINATE";
            break;

        case FBE_PORT_AUTO_NEG_NA:
            return "NA";
            break;

        case FBE_PORT_AUTO_NEG_INVALID:
        default:
            return "INVALID";
            break;
    }
}
/**************************************************************
 *  end of fbe_module_mgmt_convert_mgmtPortAutoNeg_to_string()
 *************************************************************/
/*!**************************************************************
 * fbe_module_mgmt_convert_externalMgmtPortSpeed_to_string()
 ****************************************************************
 * @brief
 *  This function convert the Mgmt port speed to a text string
 *
 * @param :
 *   portSpeed - fbe_module_mgmt_port_speed_t
 *
 * @return fbe_char_t * string for conversion type
 *
 * @author
 *  15-April-2011: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
const fbe_char_t *
fbe_module_mgmt_convert_externalMgmtPortSpeed_to_string(fbe_mgmt_port_speed_t portSpeed)
{
    switch(portSpeed)
    {
        case FBE_MGMT_PORT_SPEED_10MBPS:
            return "10Mbps";
            break;

        case FBE_MGMT_PORT_SPEED_100MBPS:
            return "100Mbps";
            break;

        case FBE_MGMT_PORT_SPEED_1000MBPS:
            return "1000Mbps";
            break;

        case FBE_MGMT_PORT_SPEED_UNSPECIFIED:
            return "UNSPECIFIED";
            break;

        case FBE_MGMT_PORT_SPEED_INDETERMIN:
            return "INDETERMINATE";
            break;

        case FBE_MGMT_PORT_SPEED_NA:
            return "NA";
            break;

        case FBE_MGMT_PORT_SPEED_INVALID:
        default:
            return "INVALID";
            break;
    }
}
/********************************************************************
 *  end of fbe_module_mgmt_convert_externalMgmtPortSpeed_to_string()
 *******************************************************************/
/*!**************************************************************
 * fbe_module_mgmt_convert_externalMgmtPortDuplexMode_to_string()
 ****************************************************************
 * @brief
 *  This function convert the Mgmt port duplex mode to a text string
 *
 * @param :
 *   portDuplex - fbe_module_mgmt_port_duplex_mode_t
 *
 * @return fbe_char_t * string for conversion type
 *
 * @author
 *  15-April-2011: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
const fbe_char_t * 
fbe_module_mgmt_convert_externalMgmtPortDuplexMode_to_string(fbe_mgmt_port_duplex_mode_t portDuplex)
{
    switch(portDuplex)
    {
        case FBE_PORT_DUPLEX_MODE_HALF:
            return "HALF DUPLEX";
            break;

        case FBE_PORT_DUPLEX_MODE_FULL:
            return "FULL DUPLEX";
            break;

        case FBE_PORT_DUPLEX_MODE_UNSPECIFIED:
            return "UNSPECIFIED";
            break;

        case FBE_PORT_DUPLEX_MODE_INDETERMIN:
            return "INDETERMINATE";
            break;

        case FBE_PORT_DUPLEX_MODE_NA:
            return "NA";
            break;

        case FBE_PORT_DUPLEX_MODE_INVALID:
        default:
            return "INVALID";
            break;
    }
}

/*!**************************************************************
 * fbe_module_mgmt_convert_link_state_to_string()
 ****************************************************************
 * @brief
 *  This function convert the port link state to a text string
 *
 * @param :
 *   link_state - port link state
 *
 * @return fbe_char_t * string for conversion type
 *
 * @author
 *  18-October-2011: Created  bphilbin
 *
 ****************************************************************/
const fbe_char_t * 
fbe_module_mgmt_convert_link_state_to_string(fbe_port_link_state_t link_state)
{
    switch(link_state)
    {
    case FBE_PORT_LINK_STATE_DEGRADED:
        return "degraded";
        break;
    case FBE_PORT_LINK_STATE_DOWN:
        return "down";
        break;
    case FBE_PORT_LINK_STATE_MAX:
        return "max";
        break;
    case FBE_PORT_LINK_STATE_NOT_INITIALIZED:
        return "uninitialized";
        break;
    case FBE_PORT_LINK_STATE_UP:
        return "up";
        break;
    default:
    case FBE_PORT_LINK_STATE_INVALID:
        return "invalid";
        break;
    }
}

/*!**************************************************************
 * fbe_module_mgmt_get_port_logical_number_string()
 ****************************************************************
 * @brief
 *  This function converts the port logical number to a string.
 *
 * @param :
 *   logical_number - port logical number
 *   log_num_string - this is where we put the string output of
 *                    the function
 *
 * @return n/a
 *
 * @author
 *  18-October-2011: Created  bphilbin
 *
 ****************************************************************/
void
fbe_module_mgmt_get_port_logical_number_string(fbe_u32_t logical_number, fbe_char_t *log_num_string)
{
    if(logical_number == INVALID_PORT_U8)
    {
        fbe_sprintf(log_num_string, 255, "UNC");
    }
    else
    {
        fbe_sprintf(log_num_string, 255, "%d", logical_number);
    }
    return;
}


/*************************************************************************
 *  end of fbe_module_mgmt_convert_externalMgmtPortDuplexMode_to_string()
 ************************************************************************/
