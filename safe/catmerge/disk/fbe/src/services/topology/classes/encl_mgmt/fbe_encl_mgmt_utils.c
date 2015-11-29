/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_enclosure_mgmt_utils.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Enclosure Management utils
 *  code.
 * 
 *  This file contains some common utility functions for enclosure object management.
 * 
 * @ingroup enclosure_mgmt_class_files
 * 
 * @version
 *   22-March-2011:  Created. Trupti Ghate
 *
 ***************************************************************************/

#include "fbe/fbe_enclosure.h"
#include "fbe_encl_mgmt_utils.h"
#include "fbe/fbe_eses.h"
#include "fbe_encl_mgmt_private.h"

/*!**************************************************************
 * fbe_encl_mgmt_get_encl_type_string()
 ****************************************************************
 * @brief
 *  This function converts the enclosure type to a text string.
 *
 * @param 
 *        fbe_esp_encl_type_t enclosure_type
 * 
 * @return
 *        fbe_char_t *string for enclosure string
 * 
 * @author
 *  21-March-2011:  Created. Trupti Ghate
 *
 ****************************************************************/

fbe_char_t * fbe_encl_mgmt_get_encl_type_string(fbe_esp_encl_type_t enclosure_type)
{
    switch(enclosure_type)
    {
        case  FBE_ESP_ENCL_TYPE_INVALID:
            return "Invalid";
        case  FBE_ESP_ENCL_TYPE_BUNKER:
            return "Bunker";
        case  FBE_ESP_ENCL_TYPE_CITADEL:
            return "Citadel";
        case  FBE_ESP_ENCL_TYPE_VIPER:
            return "Viper";
        case FBE_ESP_ENCL_TYPE_DERRINGER:
            return "Derringer";
        case FBE_ESP_ENCL_TYPE_VOYAGER:
            return "Voyager";
        case FBE_ESP_ENCL_TYPE_FOGBOW:
            return "Fogbow";
        case FBE_ESP_ENCL_TYPE_BROADSIDE:
            return "Broadside";
        case FBE_ESP_ENCL_TYPE_RATCHET:
            return "Ratchet";
        case FBE_ESP_ENCL_TYPE_SIDESWIPE:
            return "Sideswipe";
        case FBE_ESP_ENCL_TYPE_FALLBACK:
            return "Stratosphere Fallback";
        case FBE_ESP_ENCL_TYPE_BOXWOOD:
            return "Boxwood";
        case FBE_ESP_ENCL_TYPE_KNOT:
            return "Knot";
        case  FBE_ESP_ENCL_TYPE_PINECONE:
            return "Pinecone";
        case  FBE_ESP_ENCL_TYPE_STEELJAW:
            return "Steeljaw";
        case  FBE_ESP_ENCL_TYPE_RAMHORN:
            return "Ramhorn";
        case  FBE_ESP_ENCL_TYPE_ANCHO:
            return "Ancho";
        case  FBE_ESP_ENCL_TYPE_VIKING:
            return "Viking";
        case  FBE_ESP_ENCL_TYPE_CAYENNE:
            return "Cayenne";
        case  FBE_ESP_ENCL_TYPE_NAGA:
            return "Naga";
        case FBE_ESP_ENCL_TYPE_TABASCO:
            return "Tabasco";
        case  FBE_ESP_ENCL_TYPE_PROTEUS:
            return "Proteus";
        case  FBE_ESP_ENCL_TYPE_TELESTO:
            return "Telesto";
        case  FBE_ESP_ENCL_TYPE_CALYPSO:
            return "Calypso";
        case  FBE_ESP_ENCL_TYPE_MIRANDA:
            return "Miranda";
        case  FBE_ESP_ENCL_TYPE_RHEA:
            return "Rhea";
        case FBE_ESP_ENCL_TYPE_MERIDIAN:
            return "Meridian";
        case FBE_ESP_ENCL_TYPE_UNKNOWN:
            return "Unknown";
        default:
            return "Invalid Enclosure Type";
    }
}

/******************************************************
 * end fbe_encl_mgmt_get_encl_type_string() 
 ******************************************************/ 


/*!**************************************************************
 * fbe_encl_mgmt_get_enclShutdownReasonString()
 ****************************************************************
 * @brief
 *  This function converts the enclosure ShutdownReason to
 *  a text string.
 *
 * @param 
 *        fbe_u32_t shutdownReason
 * 
 * @return
 *        fbe_char_t *string for Shutdown Reason string
 * 
 * @author
 *  02-Nov-2011:  Created. Joe Perry
 *
 ****************************************************************/

fbe_char_t * fbe_encl_mgmt_get_enclShutdownReasonString(fbe_u32_t shutdownReason)
{
    switch(shutdownReason)
    {
    case FBE_ESES_SHUTDOWN_REASON_NOT_SCHEDULED:
        return "ShutdownNotScheduled";
    case FBE_ESES_SHUTDOWN_REASON_CLIENT_REQUESTED_POWER_CYCLE:
        return "ClientReqPowerCycle";
    case FBE_ESES_SHUTDOWN_REASON_CRITICAL_TEMP_FAULT:
        return "CriticalTempFault";
    case FBE_ESES_SHUTDOWN_REASON_CRITICAL_COOLIG_FAULT:
        return "CriticalCoolingFault";
    case FBE_ESES_SHUTDOWN_REASON_PS_NOT_INSTALLED:
        return "PowerSupplyNotInstalled";
    case FBE_ESES_SHUTDOWN_REASON_UNSPECIFIED_HW_NOT_INSTALLED:
        return "UnspecifiedHwNotInstalled";
    case FBE_ESES_SHUTDOWN_REASON_UNSPECIFIED_REASON:
        return "UnspecifiedReason";
    default:
        return "Invalid ShutdownReason";
    }
}

/******************************************************
 * end fbe_encl_mgmt_get_enclShutdownReasonString() 
 ******************************************************/ 

/*!**************************************************************
 * @fn fbe_encl_mgmt_decode_encl_state(fbe_esp_encl_state_t enclState)
 ****************************************************************
 * @brief
 *  This function decodes the ESP encl state.
 *
 * @param enclState - 
 *
 * @return the ESP encl state string.
 *
 * @version
 * 02-Feb-2012:  PHE - Created. 
 *
 ****************************************************************/
char * fbe_encl_mgmt_decode_encl_state(fbe_esp_encl_state_t enclState)
{
    char *enclStateStr;

    switch(enclState)
    {
        case FBE_ESP_ENCL_STATE_MISSING:
            enclStateStr = "MISSING";
            break;
    
        case FBE_ESP_ENCL_STATE_OK:
            enclStateStr = "OK";
            break;

        case FBE_ESP_ENCL_STATE_FAILED:
            enclStateStr = "FAILED";
            break;

        default:
            enclStateStr = "UNDEFINED";
            break;
    }

    return enclStateStr;
} 

/*!**************************************************************
 * @fn fbe_encl_mgmt_decode_encl_fault_symptom(fbe_esp_encl_fault_sym_t enclFaultSymptom)
 ****************************************************************
 * @brief
 *  This function decodes the ESP encl fault symptom.
 *
 * @param enclState - 
 *
 * @return the ESP encl state string.
 *
 * @version
 * 02-Feb-2012:  PHE - Created. 
 *
 ****************************************************************/
char * fbe_encl_mgmt_decode_encl_fault_symptom(fbe_esp_encl_fault_sym_t enclFaultSymptom)
{
    char *enclFaultSymStr;

    switch(enclFaultSymptom)
    {
        case FBE_ESP_ENCL_FAULT_SYM_NO_FAULT:
            enclFaultSymStr = "NONE";
            break;
    
        case FBE_ESP_ENCL_FAULT_SYM_LIFECYCLE_STATE_FAIL:
            enclFaultSymStr = "LIFECYCLE STATE FAIL";
            break;

        case FBE_ESP_ENCL_FAULT_SYM_EXCEEDED_MAX:
            enclFaultSymStr = "EXCEEDED MAX ENCLOSURES";
            break;

        case FBE_ESP_ENCL_FAULT_SYM_CROSS_CABLED:
            enclFaultSymStr = "CROSS CABLED";
            break;

        case FBE_ESP_ENCL_FAULT_SYM_BE_LOOP_MISCABLED:
            enclFaultSymStr = "BE LOOP MISCABLED";
            break;

        case FBE_ESP_ENCL_FAULT_SYM_LCC_INVALID_UID:
            enclFaultSymStr = "LCC INVALID UID";
            break;

        case FBE_ESP_ENCL_FAULT_SYM_UNSUPPORTED:
            enclFaultSymStr = "UNSUPPORTED";
            break;

        case FBE_ESP_ENCL_FAULT_SYM_PS_TYPE_MIX:
            enclFaultSymStr = "MIXED PS TYPES";
            break;

        default:
            enclFaultSymStr = "UNDEFINED";
            break;
    }

    return enclFaultSymStr;
}

/*!**************************************************************
 * @fn fbe_encl_mgmt_get_pe_info(fbe_encl_mgmt_t * pEnclMgmt)
 ****************************************************************
 * @brief
 *  This function get processor enclosure info from encl mgmt.
 *
 * @param enclState -
 *
 * @return the ESP encl state string.
 *
 * @version
 * 28-Aug-2012:  Dongz - Created.
 *
 ****************************************************************/
fbe_encl_info_t* fbe_encl_mgmt_get_pe_info(fbe_encl_mgmt_t * pEnclMgmt)
{
    fbe_u32_t enclIndex;

    for(enclIndex = 0; enclIndex < FBE_ESP_MAX_ENCL_COUNT; enclIndex++)
    {
        if(pEnclMgmt->encl_info[enclIndex]->object_id != FBE_OBJECT_ID_INVALID)
        {
            if(pEnclMgmt->encl_info[enclIndex]->processorEncl)
            {
                return pEnclMgmt->encl_info[enclIndex];
            }
        }

    }

    return NULL;
}

/*!**************************************************************
 * @fn fbe_encl_mgmt_validate_serial_number(fbe_u8_t * serial_number)
 ****************************************************************
 * @brief
 *  This function validate serial number
 *
 * @param serial_number -
 *
 * @return the bool value that serial number is valid or not.
 *
 * @version
 * 28-Aug-2012:  Dongz - Created.
 *
 ****************************************************************/
fbe_bool_t fbe_encl_mgmt_validate_serial_number(fbe_u8_t * serial_number)
{
    fbe_u8_t empty_sn[RESUME_PROM_PRODUCT_SN_SIZE] = {0};
    fbe_u8_t *null_sn = "NULL";
    fbe_u32_t index;
    if(serial_number == NULL)
    {
        return FALSE;
    }
    if(strncmp(serial_number, empty_sn, RESUME_PROM_PRODUCT_SN_SIZE) == 0 ||
            strncmp(serial_number, null_sn, strlen(null_sn)) == 0 ||
                   (*serial_number == 0))
    {
        return FALSE;
    }
    for(index=0;index<strlen(serial_number);index++)
    {
        // if it isn't an alphanumeric character
        if(!( ((serial_number[index] >= 48) && (serial_number[index] <= 57)) ||
              ((serial_number[index] >= 65) && (serial_number[index] <= 90)) ||
              (serial_number[index] == 95) ||
              ((serial_number[index] >= 97) && (serial_number[index] <= 122))))
        {
            return FALSE;
        }
    }
    return TRUE;
}

/*!**************************************************************
 * @fn fbe_encl_mgmt_validate_part_number(fbe_u8_t * part_number)
 ****************************************************************
 * @brief
 *  This function validate part number
 *
 * @param part_number -
 *
 * @return the bool value that part number is valid or not.
 *
 * @version
 * 28-Aug-2012:  Dongz - Created.
 *
 ****************************************************************/
fbe_bool_t fbe_encl_mgmt_validate_part_number(fbe_u8_t * part_number)
{
    fbe_bool_t pn_valid;
    fbe_u8_t empty_pn[RESUME_PROM_PRODUCT_PN_SIZE] = {0};
    fbe_u8_t *null_pn = "NULL";
    if(part_number == NULL)
    {
        return FALSE;
    }
    if(strncmp(part_number, empty_pn, RESUME_PROM_PRODUCT_PN_SIZE) == 0 ||
            strncmp(part_number, null_pn, strlen(null_pn)) == 0 ||
        (*part_number == 0))
    {
        pn_valid = FALSE;
    }
    else
    {
        pn_valid = TRUE;
    }
    return pn_valid;
}

/*!**************************************************************
 * @fn fbe_encl_mgmt_validate_rev(fbe_u8_t * rev)
 ****************************************************************
 * @brief
 *  This function validate revision
 *
 * @param rev -
 *
 * @return the bool value that rev is valid or not.
 *
 * @version
 * 28-Aug-2012:  Dongz - Created.
 *
 ****************************************************************/
fbe_bool_t fbe_encl_mgmt_validate_rev(fbe_u8_t * rev)
{
    fbe_bool_t rev_valid;
    fbe_u8_t empty_rev[RESUME_PROM_PRODUCT_REV_SIZE] = {0};
    fbe_u8_t *null_rev = "NULL";
    if(rev == NULL)
    {
        return FALSE;
    }
    if(strncmp(rev, empty_rev, RESUME_PROM_PRODUCT_REV_SIZE) == 0 ||
            strncmp(rev, null_rev, strlen(null_rev)) == 0 ||
            (*rev == 0))
    {
        rev_valid = FALSE;
    }
    else
    {
        rev_valid = TRUE;
    }
    return rev_valid;
}

/*!**************************************************************
 * fbe_encl_mgmt_check_and_copy()
 ****************************************************************
 * @brief
 *  This function checks the two str, if they are not equal, then copy
 *  the source to dest.
 *
 *
 * @param dest - dest string
 * @param source - source string
 * @param source - length
 * @param copied - did we copied the info?
 *
 * @return
 *
 *
 * @author
 *  28-Aug-2012 - Created. Dongz
 *
 ****************************************************************/
void fbe_encl_mgmt_string_check_and_copy(fbe_u8_t *dest, fbe_u8_t *source, fbe_u32_t len, fbe_bool_t *copied)
{
    *copied = FALSE;
    if(strncmp(dest, source, len) != 0)
    {
        fbe_copy_memory(dest, source, len);
        *copied = TRUE;
    }
}

/***************************************************************
*  fbe_encl_mgmt_isEnclosureSupported()
***************************************************************
 * @brief
 *  This function checks if the enclosure type is supported
 *  by this platform.
 *
 * @param platformInfo - platform info (contains platform type)
 * @param enclosureType - enclosure type
 *
 * @return
 *
 *
 * @author
 *      14-Jan-2013 - Created        Joe Perry
 * 
*****************************************************************/
BOOL fbe_encl_mgmt_isEnclosureSupported(fbe_encl_mgmt_t * pEnclMgmt,
                                        SPID_PLATFORM_INFO *platformInfo,
                                        fbe_esp_encl_type_t enclosureType)
{
    BOOL        enclosureSupported = FALSE;

    switch (platformInfo->platformType)
    {
        // Rockies platforms
        case SPID_PROMETHEUS_M1_HW_TYPE:
        case SPID_PROMETHEUS_S1_HW_TYPE:
        case SPID_DEFIANT_M1_HW_TYPE:
        case SPID_DEFIANT_M2_HW_TYPE:
        case SPID_DEFIANT_M3_HW_TYPE:
        case SPID_DEFIANT_M4_HW_TYPE:
        case SPID_DEFIANT_M5_HW_TYPE:
        case SPID_DEFIANT_S1_HW_TYPE:
        case SPID_DEFIANT_S4_HW_TYPE:
        case SPID_DEFIANT_K2_HW_TYPE:
        case SPID_DEFIANT_K3_HW_TYPE:
            switch (enclosureType)
            {
            // PE's
            case FBE_ESP_ENCL_TYPE_RATCHET:
            case FBE_ESP_ENCL_TYPE_FALLBACK:
            // DAE's
            case FBE_ESP_ENCL_TYPE_VIPER:
            case FBE_ESP_ENCL_TYPE_DERRINGER:
            case FBE_ESP_ENCL_TYPE_VOYAGER:
            case FBE_ESP_ENCL_TYPE_CAYENNE:
            case FBE_ESP_ENCL_TYPE_NAGA:
            case FBE_ESP_ENCL_TYPE_ANCHO:
            case FBE_ESP_ENCL_TYPE_TABASCO:
                enclosureSupported = TRUE;
                break;
            }
            break;

        // Moons platforms
        case SPID_ENTERPRISE_HW_TYPE:
            switch (enclosureType)
            {
            // PE's
            case FBE_ESP_ENCL_TYPE_PROTEUS:
            case FBE_ESP_ENCL_TYPE_TELESTO:
            case FBE_ESP_ENCL_TYPE_CALYPSO:
            // DAE's
            case FBE_ESP_ENCL_TYPE_VIPER:
            case FBE_ESP_ENCL_TYPE_DERRINGER:
            case FBE_ESP_ENCL_TYPE_VOYAGER:
            case FBE_ESP_ENCL_TYPE_ANCHO:
            case FBE_ESP_ENCL_TYPE_TABASCO:
            case FBE_ESP_ENCL_TYPE_CAYENNE:
            case FBE_ESP_ENCL_TYPE_NAGA:
                enclosureSupported = TRUE;
                break;
            case FBE_ESP_ENCL_TYPE_VIKING:
                if(platformInfo->platformType != SPID_DEFIANT_M5_HW_TYPE) 
                {
                    enclosureSupported = TRUE;
                }
                break;
            }
            break;

        // Kitty Hawk platforms
        case SPID_NOVA1_HW_TYPE:
        case SPID_NOVA3_HW_TYPE:
        case SPID_NOVA3_XM_HW_TYPE:
        case SPID_NOVA_S1_HW_TYPE:
        case SPID_BEARCAT_HW_TYPE:
            switch (enclosureType)
            {
            // PE's
            case FBE_ESP_ENCL_TYPE_RAMHORN:
            case FBE_ESP_ENCL_TYPE_STEELJAW:
            case FBE_ESP_ENCL_TYPE_BOXWOOD:             // not shipping, but being used internally - remove
            case FBE_ESP_ENCL_TYPE_KNOT:                // not shipping, but being used internally - remove
            // DAE's
            case FBE_ESP_ENCL_TYPE_PINECONE:
            case FBE_ESP_ENCL_TYPE_DERRINGER:
                enclosureSupported = TRUE;
                break;
            }
            break;


        // Moon platforms
        case SPID_OBERON_1_HW_TYPE:
        case SPID_OBERON_2_HW_TYPE:
        case SPID_OBERON_3_HW_TYPE:
        case SPID_OBERON_4_HW_TYPE:
        case SPID_OBERON_S1_HW_TYPE:
            switch (enclosureType)
            {
                // PE's
            case FBE_ESP_ENCL_TYPE_RHEA:
            case FBE_ESP_ENCL_TYPE_MIRANDA:
            // DAE's
            case FBE_ESP_ENCL_TYPE_ANCHO:
            case FBE_ESP_ENCL_TYPE_TABASCO:
//            case FBE_ESP_ENCL_TYPE_NAGA:
                enclosureSupported = TRUE;
                break;
            }
            break;

        case SPID_HYPERION_1_HW_TYPE:
        case SPID_HYPERION_2_HW_TYPE:
        case SPID_HYPERION_3_HW_TYPE:
            switch (enclosureType)
            {
            // PE's
            case FBE_ESP_ENCL_TYPE_CALYPSO:
            // DAE's
            case FBE_ESP_ENCL_TYPE_ANCHO:
            case FBE_ESP_ENCL_TYPE_TABASCO:
//            case FBE_ESP_ENCL_TYPE_NAGA:
                enclosureSupported = TRUE;
                break;
            }
            break;

        case SPID_TRITON_1_HW_TYPE:
            switch (enclosureType)
            {
            // PE's
            case FBE_ESP_ENCL_TYPE_TELESTO:
            // DAE's
            case FBE_ESP_ENCL_TYPE_ANCHO:
            case FBE_ESP_ENCL_TYPE_TABASCO:
//            case FBE_ESP_ENCL_TYPE_NAGA:
                enclosureSupported = TRUE;
                break;
            }
            break;

        // Virtual platforms
        case SPID_MERIDIAN_ESX_HW_TYPE:
        case SPID_TUNGSTEN_HW_TYPE:
            switch (enclosureType)
            {
            // PE's
            case FBE_ESP_ENCL_TYPE_MERIDIAN:
                enclosureSupported = TRUE;
                break;
            }
            break;

        default:
            break;
    }

    // Allow unsupported enclosures for internal use
    if (!enclosureSupported && pEnclMgmt->ignoreUnsupportedEnclosures)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, allowing Unsupported Enclosure, platform %d, enclType %d\n",
                              __FUNCTION__, platformInfo->platformType, enclosureType);
        enclosureSupported = TRUE;
    }

    return enclosureSupported; 

}   // end of fbe_encl_mgmt_isEnclosureSupported


/*!*************************************************************************
 * @fn fbe_encl_mgmt_getCmiMsgTypeString(fbe_u32_t MsgType)
 **************************************************************************
 *  @brief
 *  This function decodes CMI message code for encl_mgmt.
 *
 *  @param    MsgType - CMI message type
 *
 *  @return   char * - massage type string
 *
 *  @author
 *    1-Apr-2013: Lin Lou - created.
 **************************************************************************/
const char *fbe_encl_mgmt_getCmiMsgTypeString(fbe_u32_t MsgType)
{
    const char *cmiMsgString = "Undefined";

    switch (MsgType)
    {
    case FBE_BASE_ENV_CMI_MSG_PERSISTENT_READ_REQUEST:
        cmiMsgString = "PersistentReadRequest";
        break;

    case FBE_BASE_ENV_CMI_MSG_PERSISTENT_READ_COMPLETE:
        cmiMsgString = "PersistentReadComplete";
        break;

    case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_REQUEST:
        cmiMsgString = "FupPeerPermissionRequest";
        break;

    case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_GRANT:
        cmiMsgString = "FupPeerPermissionGrant";
        break;

    case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_DENY:
        cmiMsgString = "FupPeerPermissionDeny";
        break;

    case FBE_BASE_ENV_CMI_MSG_USER_MODIFY_WWN_SEED_FLAG_CHANGE:
        cmiMsgString = "UserModifyWWNSeedFlagChange";
        break;

    case FBE_BASE_ENV_CMI_MSG_USER_MODIFIED_SYS_ID_FLAG:
        cmiMsgString = "UserModifiedSysIdFlag";
        break;

    case FBE_BASE_ENV_CMI_MSG_PERSISTENT_SYS_ID_CHANGE:
        cmiMsgString = "PersistentSysIdChange";
        break;

    case FBE_BASE_ENV_CMI_MSG_ENCL_CABLING_REQUEST:
        cmiMsgString = "EnclCablingRequest";
        break;

    case FBE_BASE_ENV_CMI_MSG_ENCL_CABLING_ACK:
        cmiMsgString = "EnclCablingACK";
        break;

    case FBE_BASE_ENV_CMI_MSG_ENCL_CABLING_NACK:
        cmiMsgString = "EnclCablingNACK";
        break;

    case FBE_BASE_ENV_CMI_MSG_CACHE_STATUS_UPDATE:
        cmiMsgString = "CacheStatusUpdate";
        break;

    case FBE_BASE_ENV_CMI_MSG_PEER_SP_ALIVE:
        cmiMsgString = "PeerSpAlive";
        break;

    default:
        break;
    }

    return cmiMsgString;

}

/*!**************************************************************
 * fbe_encl_mgmt_decode_lcc_state()
 ****************************************************************
 * @brief
 *  This function converts the LCC State to a text string.
 *
 * @param 
 *        fbe_lcc_state_t lccState
 * 
 * @return
 *        fbe_char_t *string for LCC State string
 * 
 * @author
 *  18-May-2015:  Created. Joe Perry
 *
 ****************************************************************/
fbe_char_t * fbe_encl_mgmt_decode_lcc_state(fbe_lcc_state_t lccState)
{
    switch(lccState)
    {
    case FBE_LCC_STATE_UNKNOWN:
        return "UNKNOWN";
    case FBE_LCC_STATE_OK:
        return "OK";
    case FBE_LCC_STATE_REMOVED:
        return "REMOVED";
    case FBE_LCC_STATE_FAULTED:
        return "FAULTED";
    case FBE_LCC_STATE_DEGRADED:
        return "DEGRADED";
    default:
        return "UNDEFINED";
    }
}   // end of fbe_encl_mgmt_decode_lcc_state

/*!**************************************************************
 * fbe_encl_mgmt_decode_lcc_subState()
 ****************************************************************
 * @brief
 *  This function converts the LCC SubState to a text string.
 *
 * @param 
 *        fbe_lcc_subState_t lccSubState
 * 
 * @return
 *        fbe_char_t *string for LCC SubState string
 * 
 * @author
 *  18-May-2015:  Created. Joe Perry
 *
 ****************************************************************/
fbe_char_t * fbe_encl_mgmt_decode_lcc_subState(fbe_lcc_subState_t lccSubState)
{
    switch(lccSubState)
    {
    case FBE_LCC_SUBSTATE_NO_FAULT:
        return "NO FAULT";
    case FBE_LCC_SUBSTATE_LCC_FAULT:
        return "LCC FAULT";
    case FBE_LCC_SUBSTATE_COMPONENT_FAULT:
        return "COMPONENT FAULT";
    case FBE_LCC_SUBSTATE_FUP_FAILURE:
        return "FUP FAULT";
    case FBE_LCC_SUBSTATE_RP_READ_FAILURE:
        return "RP READ FAULT";
    default:
        return "UNDEFINED";
    }
}   // end of fbe_encl_mgmt_decode_lcc_subState

//end of fbe_enclsoure_mgmt_utils.c
