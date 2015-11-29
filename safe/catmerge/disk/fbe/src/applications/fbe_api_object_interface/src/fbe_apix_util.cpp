/*********************************************************************
* Copyright (C) EMC Corporation, 1989 - 2011
* All Rights Reserved.
* Licensed Material-Property of EMC Corporation.
* This software is made available solely pursuant to the terms
* of a EMC license agreement which governs its use.
*********************************************************************/

/*********************************************************************
*
*  Description:
*      This file defines methods of the FBEAPIX utilityClass.
*
*  Notes:
*       These methods handle various operations that can be useful
*       for the entire application. Methods that log should go
*       into the init_fbe_api_class.cpp file.
*       
*  History:
*      07-March-2011 : initial version
*
*********************************************************************/

#ifndef T_FBEAPIXUTIL_H
#include "fbe_apix_util.h"
#endif

#include "generic_utils_lib.h"

/*********************************************************************
* fbeApixNamespace
*********************************************************************/

namespace fbeApixUtil {

/*********************************************************************
 * utilityClass:: append_to ()
 *********************************************************************
 *
 *  Description:
 *      append a string onto another string
 *
 *  Input: Parameters
 *      char * stringx - the string to append to
 *      char * stringy - the string to append
 *
 *  Output: 
 *      None
 *
 *  Returns:
 *      char * - pointer to new string
 *
 *  History:
 *       07-March-2011 : initial version.
 *
 *********************************************************************/

char* utilityClass::append_to(char * stringx, char * stringy) {

    // concatenate stringx with stringy
    char * newstring = NULL;
    
    if (stringx != NULL) {
        // calculate size for newstring + "\0"
        newstring = new char[strlen(stringx) + strlen(stringy) + 1];
        strcpy(newstring, stringx);
        strcat(newstring, stringy);
        delete [] stringx;
    
    } else {
        newstring = new char[strlen(stringy) + 1];
        strcpy(newstring, stringy);
    }

    return newstring;
}

/*********************************************************************
*  utilityClass Class ::  speed_capability()   
*********************************************************************
*
*  Description:
*      Mapping to display speed capability
*       
*  Input: Parameters
*      speed_capability - speed capability enum value
*
*  Returns:
*      pointer to string corresponding to enum value
*
*********************************************************************/

const char* utilityClass::speed_capability(fbe_u32_t speed_capability){
    
    switch (speed_capability) {

        CASENAME(FBE_DRIVE_SPEED_UNKNOWN);
        CASENAME(FBE_DRIVE_SPEED_1_5_GB);
        CASENAME(FBE_DRIVE_SPEED_2GB);
        CASENAME(FBE_DRIVE_SPEED_3GB);
        CASENAME(FBE_DRIVE_SPEED_4GB);
        CASENAME(FBE_DRIVE_SPEED_6GB);
        CASENAME(FBE_DRIVE_SPEED_8GB);
        CASENAME(FBE_DRIVE_SPEED_10GB);
        CASENAME(FBE_DRIVE_SPEED_12GB);
        CASENAME(FBE_DRIVE_SPEED_LAST);
        
        DFLTNAME(UNKNOWN_SPEED_CAPABILITY);
    }
}

/*********************************************************************
*  utilityClass Class ::  lifecycle_state()   
*********************************************************************
*
*  Description:
*      Mapping to display lifecycle state
*       
*  Input: Parameters
*      state - state enum value
*
*  Returns:
*      pointer to string corresponding to enum value
*
*********************************************************************/

const char* utilityClass::lifecycle_state(fbe_u32_t state){
    
    switch (state) {

        CASENAME(FBE_LIFECYCLE_STATE_SPECIALIZE);
        CASENAME(FBE_LIFECYCLE_STATE_ACTIVATE);
        CASENAME(FBE_LIFECYCLE_STATE_READY);
        CASENAME(FBE_LIFECYCLE_STATE_HIBERNATE);
        CASENAME(FBE_LIFECYCLE_STATE_OFFLINE);
        CASENAME(FBE_LIFECYCLE_STATE_FAIL);
        CASENAME(FBE_LIFECYCLE_STATE_DESTROY);
        CASENAME(FBE_LIFECYCLE_STATE_NON_PENDING_MAX);
        CASENAME(FBE_LIFECYCLE_STATE_PENDING_ACTIVATE);
        CASENAME(FBE_LIFECYCLE_STATE_PENDING_HIBERNATE);
        CASENAME(FBE_LIFECYCLE_STATE_PENDING_OFFLINE);
        CASENAME(FBE_LIFECYCLE_STATE_PENDING_FAIL);
        CASENAME(FBE_LIFECYCLE_STATE_PENDING_DESTROY);
        CASENAME(FBE_LIFECYCLE_STATE_PENDING_MAX);
        CASENAME(FBE_LIFECYCLE_STATE_INVALID);

        DFLTNAME(UNKNOWN_LIFECYCLE_STATE);
    }
}


/*********************************************************************
* utilityClass Class ::convert_power_status_number_to_string(
* fbe_status_t fbe_status_code)(private method)
*********************************************************************
*
*  Description:
*      Return a pointer to a text string that represents the FBE_status code 
*      that is passed.
*
*  Input: Parameters
*      fbe_status_code: status code

*  Returns:
*      pointer to a string
*********************************************************************/

const char* utilityClass::convert_power_status_number_to_string(
    fbe_u32_t fbe_power_status_code)
{
    switch (fbe_power_status_code) {
        CASENAME(FBE_ENERGY_STAT_INVALID);
        CASENAME(FBE_ENERGY_STAT_UNINITIALIZED);
        CASENAME(FBE_ENERGY_STAT_VALID);
        CASENAME(FBE_ENERGY_STAT_UNSUPPORTED);
        CASENAME(FBE_ENERGY_STAT_FAILED);
        CASENAME(FBE_ENERGY_STAT_NOT_PRESENT);
        CASENAME(FBE_ENERGY_STAT_SAMPLE_TOO_SMALL);
        DFLTNAME(UNKNOWN_ENERGY_STAT);

    }
}

/*********************************************************************
* utilityClass Class :: convert_sp_name_to_index()
*********************************************************************
*
*  Description:
*      This function convert sp name to its index.
*
*  Input: Parameters
*      *argv - pointer to arguments
*
*  Returns:
*      sp index value
*********************************************************************/

fbe_u8_t utilityClass::convert_sp_name_to_index(char ** argv) 
{
    if(strcmp(*argv, "LOCAL") == 0) 
    {
        return(LOCAL_SP);
    }
    if(strcmp(*argv, "PEER") == 0)
    {
        return(PEER_SP);
    }
    return (INVALID_SP);
}

/*********************************************************************
* utilityClass Class :: convert_sp_weekday_to_index()
*********************************************************************
*
*  Description:
*      This function convert weekday name to its index.
*
*  Input: Parameters
*      *argv - pointer to arguments
*
*  Returns:
*      weekday index value
*********************************************************************/

fbe_u16_t utilityClass::convert_sp_weekday_to_index(char ** argv) 
{
    if(strcmp(*argv, "SUNDAY") == 0) 
    {
        return(SUNDAY);
    }
    if(strcmp(*argv, "MONDAY") == 0)
    {
        return(MONDAY);
    }
    if(strcmp(*argv, "TUESDAY") == 0)
    {
        return(TUESDAY);
    }
    if(strcmp(*argv, "WEDNESDAY") == 0)
    {
        return(WEDNESDAY);
    }
    if(strcmp(*argv, "THURSDAY") == 0)
    {
        return(THURSDAY);
    }
    if(strcmp(*argv, "FRIDAY") == 0)
    {
        return(FRIDAY);
    }
    if(strcmp(*argv, "SATURDAY") == 0)
    {
        return(SATURDAY);
    }

    return (NOT_DAY);
}

/*********************************************************************
*  utilityClass Class ::  convert_sp_id_to_string()   
*********************************************************************
*
*  Description:
*      Mapping to display sp id 
*       
*  Input: Parameters
*      sp_id - sp id enum value
*
*  Returns:
*      pointer to string corresponding to enum value
*
*********************************************************************/

const char* utilityClass::convert_sp_id_to_string(fbe_u32_t sp_id)
{
    switch (sp_id) {
        CASENAME(SP_A);
        CASENAME(SP_B);
        DFLTNAME(UNKNOWN_SP_ID);

    }
}

/*********************************************************************
*  utilityClass Class ::  convert_sps_id_to_string()   
*********************************************************************
*
*  Description:
*      Mapping to display sps id 
*       
*  Input: Parameters
*      sps_id - sps id enum value
*
*  Returns:
*      pointer to string corresponding to enum value
*
*********************************************************************/

const char* utilityClass::convert_sps_id_to_string(fbe_u32_t sps_id)
{
    switch (sps_id) {
        CASENAME(FBE_SPS_A);
        CASENAME(FBE_SPS_B);
        DFLTNAME(UNKNOWN_FBE_SPS_ID);

    }
}

/*********************************************************************
*  utilityClass Class ::  convert_ac_dc_input_to_string()   
*********************************************************************
*
*  Description:
*      Mapping to display ac/dc input
*       
*  Input: Parameters
*      input - ac/dc input enum value
*
*  Returns:
*      pointer to string corresponding to enum value
*
*********************************************************************/

const char* utilityClass::convert_ac_dc_input_to_string(fbe_u32_t input)
{
    switch (input) {
        CASENAME(INVALID_AC_DC_INPUT);
        CASENAME(AC_INPUT);
        CASENAME(DC_INPUT);
        DFLTNAME(UNKNOWN_AC_DC_INPUT);
    }
}

/*********************************************************************
*  utilityClass Class :: convert_hw_module_type_to_string()   
*********************************************************************
*
*  Description:
*      Mapping to display hardware module type
*       
*  Input: Parameters
*      type - hw module type enum value
*
*  Returns:
*      pointer to string corresponding to enum value
*
*********************************************************************/

const char* utilityClass::convert_hw_module_type_to_string(fbe_u32_t type)
{
    return decodeFamilyID((FAMILY_ID)type);
}

/*********************************************************************
*  utilityClass Class ::  convert_led_blink_rate_to_string()   
*********************************************************************
*
*  Description:
*      Mapping to display hardware module type
*       
*  Input: Parameters
*      blink_rate - hw module type enum value
*
*  Returns:
*      pointer to string corresponding to enum value
*
*********************************************************************/

const char* utilityClass::convert_led_blink_rate_to_string(
    LED_BLINK_RATE blink_rate)
{
    const char* rtnStr = "blank";

    switch (blink_rate)
    {
        case LED_BLINK_OFF:
            rtnStr = "Off";
            break;
        case LED_BLINK_0_25HZ:
            rtnStr = "1/4 Hz";
            break;
        case LED_BLINK_0_50HZ:
            rtnStr = "1/2 Hz";
            break;
        case LED_BLINK_1HZ:
            rtnStr = "1 Hz";
            break;
        case LED_BLINK_2HZ:
            rtnStr = "2 Hz";
            break;
        case LED_BLINK_3HZ:
            rtnStr = "3 Hz";
            break;
        case LED_BLINK_4HZ:
            rtnStr = "4 Hz";
            break;
        case LED_BLINK_ON:
            rtnStr = "On";
            break;
        default:
            rtnStr = "Unknown";
            break;
    }

    return (rtnStr);
}

/*********************************************************************
*  utilityClass Class :: convert_io_module_location_to_string()   
*********************************************************************
*
*  Description:
*      This function converts the io module location to a text string.
*       
*  Input: Parameters
*      location - io module location enum value
*
*  Returns:
*      pointer to string corresponding to enum value
*
*********************************************************************/

const char* utilityClass::convert_io_module_location_to_string(
    fbe_io_module_location_t location)
{
    const char* rtnStr = "blank";

    switch (location)
    {
        case FBE_IO_MODULE_LOC_ONBOARD_SLOT0:
            rtnStr = "Onboard Slot 0";
            break;
        case FBE_IO_MODULE_LOC_ONBOARD_SLOT1:
            rtnStr = "Onboard Slot 1";
            break;
        case FBE_IO_MODULE_LOC_ONBOARD_SLOT2:
            rtnStr = "Onboard Slot 2";
            break;
        case FBE_IO_MODULE_LOC_ONBOARD_SLOT3:
            rtnStr = "Onboard Slot 3";
            break;
        case FBE_IO_MODULE_LOC_ONBOARD_SLOT4:
            rtnStr = "Onboard Slot 4";
            break;
        case FBE_IO_MODULE_LOC_ANNEX_SLOT0:
            rtnStr = "Annex Slot 0";
            break;
        case FBE_IO_MODULE_LOC_ANNEX_SLOT1:
            rtnStr = "Annex Slot 1";
            break;
        case FBE_IO_MODULE_LOC_UNKNOWN:
            rtnStr = "Unknown";
            break;
        default:
            rtnStr = "N.A.";
            break;
    }

    return (rtnStr);
}

/*********************************************************************
*  utilityClass Class :: convert_resume_prom_attributes_to_string()   
*********************************************************************
*
*  Description:
*      This function converts the resume prom attributes to a text string.
*       
*  Input: Parameters
*      type - resume_prom_attributes enum value
*
*  Returns:
*      pointer to string corresponding to enum value
*
*********************************************************************/

const char* utilityClass::convert_resume_prom_attributes_to_string(
    fbe_pe_resume_prom_id_t type)
{
    const char* rtnStr = "blank";

    switch (type)
    {
        case FBE_PE_SPA_RESUME_PROM:
            rtnStr = "SPA RESUME PROM";
            break;
        case FBE_PE_SPB_RESUME_PROM:
            rtnStr = "SPB RESUME PROM";
            break;
        case FBE_PE_MIDPLANE_RESUME_PROM:
            rtnStr = "MIDPLANE RESUME PROM";
            break;
        case FBE_PE_SPA_PS0_RESUME_PROM:
            rtnStr = "SPA PS0 RESUME PROM";
            break;
        case FBE_PE_SPA_PS1_RESUME_PROM:
            rtnStr = "SPA PS1 RESUME PROM";
            break;
        case FBE_PE_SPB_PS0_RESUME_PROM:
            rtnStr = "SPB PS0 RESUME PROM";
            break;
        case FBE_PE_SPB_PS1_RESUME_PROM:
            rtnStr = "SPB PS1 RESUME PROM";
            break;
        case FBE_PE_SPA_MGMT_RESUME_PROM:
            rtnStr = "SPA MGMT RESUME PROM";
            break;
        case FBE_PE_SPB_MGMT_RESUME_PROM:
            rtnStr = "SPB MGMT RESUME PROM";
            break;
        case FBE_PE_SPA_MEZZANINE_RESUME_PROM:
            rtnStr = "SPA MEZZANINE RESUME PROM";
            break;
        case     FBE_PE_SPB_MEZZANINE_RESUME_PROM:
            rtnStr = "SPB MEZZANINE RESUME PROM";
            break;
        case FBE_PE_SPA_IO_ANNEX_RESUME_PROM:
            rtnStr = "SPA IO ANNEX RESUME PROM";
            break;
        case FBE_PE_SPB_IO_ANNEX_RESUME_PROM:
            rtnStr = "SPB IO ANNEX RESUME PROM";
            break;
        case FBE_PE_SPA_IO_SLOT0_RESUME_PROM:
            rtnStr = "SPA IO SLOT0 RESUME PROM";
            break;
        case FBE_PE_SPA_IO_SLOT1_RESUME_PROM:
            rtnStr = "SPA IO SLOT1 RESUME PROM";
            break;
        case FBE_PE_SPA_IO_SLOT2_RESUME_PROM:
            rtnStr = "SPA IO SLOT2 RESUME PROM";
            break;
        case FBE_PE_SPA_IO_SLOT3_RESUME_PROM:
            rtnStr = "SPA IO SLOT3 RESUME PROM";
            break;
        case FBE_PE_SPA_IO_SLOT4_RESUME_PROM:
            rtnStr = "SPA IO SLOT4 RESUME PROM";
            break;
        case FBE_PE_SPA_IO_SLOT5_RESUME_PROM:
            rtnStr = "SPA IO SLOT5 RESUME PROM";
            break;
        case FBE_PE_SPB_IO_SLOT0_RESUME_PROM:
            rtnStr = "SPB IO SLOT0 RESUME PROM";
            break;
        case FBE_PE_SPB_IO_SLOT1_RESUME_PROM:
            rtnStr = "SPB IO SLOT1 RESUME PROM";
            break;
        case FBE_PE_SPB_IO_SLOT2_RESUME_PROM:
            rtnStr = "SPB IO SLOT2 RESUME PROM";
            break;
        case FBE_PE_SPB_IO_SLOT3_RESUME_PROM:
            rtnStr = "SPB IO SLOT3 RESUME PROM";
            break;
        case FBE_PE_SPB_IO_SLOT4_RESUME_PROM:
            rtnStr = "SPB IO SLOT4 RESUME PROM";
            break;
        case FBE_PE_SPB_IO_SLOT5_RESUME_PROM:
            rtnStr = "SPB IO SLOT5 RESUME PROM";
            break;

        case FBE_PE_MAX_RESUME_PROM_IDS:
        default:
            rtnStr = "UNRECOGNIZED RESUME PROM";
            break;
    }
    return (rtnStr);
}

/*********************************************************************
*  utilityClass Class :: convert_led_color_type_to_string()   
*********************************************************************
*
*  Description:
*      This function converts LED color type to a text string.
*       
*  Input: Parameters
*       ioPortLEDColor -ioport  LED colortype
*
*  Returns:
*      pointer to string corresponding to enum value
*
*********************************************************************/

const char * utilityClass::convert_led_color_type_to_string(
    LED_COLOR_TYPE ioPortLEDColor)
{
    switch (ioPortLEDColor)
    {
        case LED_COLOR_TYPE_BLUE:
            return "BLUE";
            break;
        case LED_COLOR_TYPE_GREEN:
            return "GREEN";
            break;
        case LED_COLOR_TYPE_BLUE_GREEN_ALT:
            return "BLUE GREEN ALT";
            break;
        default:
            return "UNKNOWN";
            break;
    }
}

/*********************************************************************
*  utilityClass Class :: convert_lan_config_mode_to_string()
*********************************************************************
*
*  Description:
*      This function convert the LANConfigMode to a text string.
*       
*  Input: Parameters
*       vLANConfigMode - vLAN Config Mode
*
*  Returns:
*      pointer to string corresponding to enum value
*
*********************************************************************/

const char * utilityClass::convert_lan_config_mode_to_string(
    VLAN_CONFIG_MODE vLANConfigMode)
{
    char *rtnStr = "Unknown vLAN Configuration Mode";

    switch (vLANConfigMode)
    {
        case CUSTOM_VLAN_MODE:
            rtnStr = "Custom vLAN Mode";
            break;
        case DEFAULT_VLAN_MODE:
            rtnStr = "Default vLAN Mode";
            break;
        case CLARiiON_VLAN_MODE:
            rtnStr = "CLARiiON vLAN Mode";
            break;
        case SYMM_VLAN_MODE:
            rtnStr = "Symmetrix vLAN Mode";
            break;
        case SAN_AUX_VLAN_MODE:
            rtnStr = "SAN-AUX vLAN Mode";
            break;
        case UNKNOWN_VLAN_MODE:
        default:
            rtnStr = "Unknown vLAN Mode";
            break;
    }

    return (rtnStr);
}

/*********************************************************************
*  utilityClass Class:: convert_lan_port_speed_to_string()
*********************************************************************
*
*  Description:
*      This function convert the LAN Port Speeds to a text pointer to string.
*       
*  Input: Parameters
*       port_speed - LAN Port Speeds
*
*  Returns:
*      string corresponding to enum value
*
*********************************************************************/

const char * utilityClass::convert_lan_port_speed_to_string(
    PORT_SPEED port_speed)
{
    const char* rtnStr = "blank";

    switch (port_speed)
    {
    case SPEED_10_Mbps:
        rtnStr = "10Mbps";
        break;
    case SPEED_100_Mbps:
        rtnStr = "100Mbps";
        break;
    case SPEED_1000_Mbps:
        rtnStr = "1000Mbps";
        break;
    default:
        rtnStr = "Unknown Speed";
        break;
    }
    return (rtnStr);
} 

/*********************************************************************
*  utilityClass Class:: convert_mgmt_port_speed_to_string()
*********************************************************************
*
*  Description:
*      This function convert the mgmt Port Speeds to a text pointer to string.
*       
*  Input: Parameters
*       port_speed - LAN Port Speeds
*
*  Returns:
*      string corresponding to enum value
*
*********************************************************************/

const char * utilityClass::convert_mgmt_port_speed_to_string(
    fbe_mgmt_port_speed_t port_speed)
{
    const char* rtnStr = "blank";

    switch (port_speed)
    {
    case FBE_MGMT_PORT_SPEED_10MBPS:
        rtnStr = "10Mbps";
        break;
    case FBE_MGMT_PORT_SPEED_100MBPS:
        rtnStr = "100Mbps";
        break;
    case FBE_MGMT_PORT_SPEED_1000MBPS:
        rtnStr = "1000Mbps";
        break;
    default:
        rtnStr = "Unknown Speed";
        break;
    }
    return (rtnStr);
} 

/*********************************************************************
* utilityClass Class :: convert_string_to_device_type() (private method)
*********************************************************************
*
*  Description:
*      convert string to device type 
*
*  Input: Parameters
*      package_str - package string

*  Returns:
*      fbe_u64_t
*********************************************************************/

fbe_u64_t utilityClass::convert_string_to_device_type(
    char *device_type_str)
{
    if (strcmp (device_type_str, "IOMODULE") == 0) {
        return FBE_DEVICE_TYPE_IOMODULE;
    }
    if (strcmp (device_type_str, "BEM") == 0) {
        return FBE_DEVICE_TYPE_BACK_END_MODULE;
    }
    if (strcmp (device_type_str, "MEZZANINE") == 0) {
        return FBE_DEVICE_TYPE_MEZZANINE;
    }
    if (strcmp (device_type_str, "MODULE") == 0) {
        return FBE_DEVICE_TYPE_MGMT_MODULE;
    }
    if (strcmp (device_type_str, "BMC") == 0) {
        return FBE_DEVICE_TYPE_BMC;
    }
	
    return FBE_DEVICE_TYPE_INVALID;
}

/*********************************************************************
* utilityClass Class :: convert_string_to_package_id() (private method)
*********************************************************************
*
*  Description:
*      convert string to package id
*
*  Input: Parameters
*      package_str - package string

*  Returns:
*      fbe_package_id_t
*********************************************************************/

fbe_package_id_t utilityClass::convert_string_to_package_id(char *package_str)
{
    if ((strcmp(package_str, "PHY") == 0) || (strcmp(package_str, "PP") == 0)) {
        return FBE_PACKAGE_ID_PHYSICAL;
    }
    if (strcmp (package_str, "SEP") == 0) {
        return FBE_PACKAGE_ID_SEP_0;
    }
    if (strcmp (package_str, "ESP") == 0) {
        return FBE_PACKAGE_ID_ESP;
    }
    
    return FBE_PACKAGE_ID_INVALID;
}

/*********************************************************************
* utilityClass Class :: convert_string_to_lifecycle_state() (private method)
*********************************************************************
*
*  Description:
*      convert string to lifecycle state
*
*  Input: Parameters
*      lifecycle_state_str - lifecycle state string

*  Returns:
*      fbe_lifecycle_state_t
*********************************************************************/

fbe_lifecycle_state_t utilityClass::convert_string_to_lifecycle_state(
    char *lifecycle_state_str)
{
    if (strcmp (lifecycle_state_str, "SPECIALIZE") == 0) {
        return FBE_LIFECYCLE_STATE_SPECIALIZE;
    }
    if (strcmp (lifecycle_state_str, "ACTIVATE") == 0) {
        return FBE_LIFECYCLE_STATE_ACTIVATE;
    }
    if (strcmp (lifecycle_state_str, "READY") == 0) {
        return FBE_LIFECYCLE_STATE_READY;
    }
    if (strcmp (lifecycle_state_str, "HIBERNATE") == 0) {
        return FBE_LIFECYCLE_STATE_HIBERNATE;
    }
    if (strcmp (lifecycle_state_str, "OFFLINE") == 0) {
        return FBE_LIFECYCLE_STATE_OFFLINE;
    }
    if (strcmp (lifecycle_state_str, "FAIL") == 0) {
        return FBE_LIFECYCLE_STATE_FAIL;
    }
    if (strcmp (lifecycle_state_str, "DESTROY") == 0) {
        return FBE_LIFECYCLE_STATE_DESTROY;
    }
    if (strcmp (lifecycle_state_str, "NON_PENDING_MAX") == 0) {
        return FBE_LIFECYCLE_STATE_NON_PENDING_MAX;
    }
    if (strcmp (lifecycle_state_str, "PENDING_READY") == 0) {
        return FBE_LIFECYCLE_STATE_PENDING_READY;
    }
    if (strcmp (lifecycle_state_str, "PENDING_ACTIVATE") == 0) {
        return FBE_LIFECYCLE_STATE_PENDING_ACTIVATE;
    }
    if (strcmp (lifecycle_state_str, "PENDING_HIBERNATE") == 0) {
        return FBE_LIFECYCLE_STATE_PENDING_HIBERNATE;
    }
    if (strcmp (lifecycle_state_str, "PENDING_OFFLINE") == 0) {
        return FBE_LIFECYCLE_STATE_PENDING_OFFLINE;
    }
    if (strcmp (lifecycle_state_str, "PENDING_FAIL") == 0) {
        return FBE_LIFECYCLE_STATE_PENDING_FAIL;
    }
    if (strcmp (lifecycle_state_str, "PENDING_DESTROY") == 0) {
        return FBE_LIFECYCLE_STATE_PENDING_DESTROY;
    }
    if (strcmp (lifecycle_state_str, "PENDING_MAX") == 0) {
        return FBE_LIFECYCLE_STATE_PENDING_MAX;
    }
    if (strcmp (lifecycle_state_str, "NOT_EXIST") == 0) {
        return FBE_LIFECYCLE_STATE_NOT_EXIST;
    }
    
    return FBE_LIFECYCLE_STATE_INVALID;
}

/*********************************************************************
* utilityClass Class :: convert_rg_type_to_string() (private method)
*********************************************************************
*
*  Description:
*      convert raid group type to string
*
*  Input: Parameters
*      fbe_raid_group_type - raid group type

*  Returns:
*      pointer to string corresponding to enum value
*********************************************************************/

const char * utilityClass::convert_rg_type_to_string(
    fbe_u32_t fbe_raid_group_type)
{
    switch (fbe_raid_group_type) {
    CASENAME(FBE_RAID_GROUP_TYPE_UNKNOWN);
    CASENAME(FBE_RAID_GROUP_TYPE_RAID1);
    CASENAME(FBE_RAID_GROUP_TYPE_RAID10);
    CASENAME(FBE_RAID_GROUP_TYPE_RAID3);
    CASENAME(FBE_RAID_GROUP_TYPE_RAID0);
    CASENAME(FBE_RAID_GROUP_TYPE_RAID5);
    CASENAME(FBE_RAID_GROUP_TYPE_RAID6);
    CASENAME(FBE_RAID_GROUP_TYPE_SPARE);
    CASENAME(FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER);
    CASENAME(FBE_RAID_GROUP_TYPE_INTERNAL_METADATA_MIRROR);

    DFLTNAME(UNKNOWN_RAID_GROUP_TYPE_IN_FBE);
    }
}

/*********************************************************************
* utilityClass Class :: convert_drive_type_to_string() (private method)
*********************************************************************
*
*  Description:
*      convert drive type to string.
*
*  Input: Parameters
*      fbe_drive_type - drive type.

*  Returns:
*      pointer to string corresponding to enum value
*********************************************************************/

const char * utilityClass::convert_drive_type_to_string(
    fbe_u32_t fbe_drive_type)
{
    switch (fbe_drive_type) {
    CASENAME(FBE_DRIVE_TYPE_INVALID);
    CASENAME(FBE_DRIVE_TYPE_FIBRE);
    CASENAME(FBE_DRIVE_TYPE_SAS);
    CASENAME(FBE_DRIVE_TYPE_SATA);
    CASENAME(FBE_DRIVE_TYPE_SAS_FLASH_HE);
    CASENAME(FBE_DRIVE_TYPE_SAS_FLASH_ME);
    CASENAME(FBE_DRIVE_TYPE_SAS_FLASH_LE);
    CASENAME(FBE_DRIVE_TYPE_SAS_FLASH_RI);
    CASENAME(FBE_DRIVE_TYPE_SATA_FLASH_HE);
    CASENAME(FBE_DRIVE_TYPE_SAS_NL);
    CASENAME(FBE_DRIVE_TYPE_SATA_PADDLECARD);

    DFLTNAME(UNKNOWN_DRIVE_TYPE_IN_FBE);
    }
}

/*********************************************************************
* utilityClass Class :: convert_block_size_to_string() (private method)
*********************************************************************
*
*  Description:
*      convert block size to string
*
*  Input: Parameters
*      fbe_block_size - block size

*  Returns:
*      pointer to string corresponding to enum value
*********************************************************************/

const char* utilityClass::convert_block_size_to_string(
    fbe_u32_t fbe_block_size)
{
    switch (fbe_block_size) {

    CASENAME(FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_INVALID);
    CASENAME(FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_520);
    CASENAME(FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_4160);

    DFLTNAME(UNKNOWN_BLOCK_SIZE_IN_FBE);
    }
}

/*********************************************************************
* utilityClass Class :: convert_object_state_to_string() (private method)
*********************************************************************
*
*  Description:
*      convert object state to string
*
*  Input: Parameters
*      fbe_object_state - object state 

*  Returns:
*      pointer to string corresponding to enum value
*********************************************************************/

const char* utilityClass::convert_object_state_to_string(
    fbe_u32_t fbe_object_state)
{
    switch (fbe_object_state) {

    CASENAME(FBE_METADATA_ELEMENT_STATE_INVALID);
    CASENAME(FBE_METADATA_ELEMENT_STATE_ACTIVE);
    CASENAME(FBE_METADATA_ELEMENT_STATE_PASSIVE);

    DFLTNAME(UNKNOWN_OBJECT_STATE_IN_FBE);
    }
}

/*********************************************************************
* utilityClass Class :: convert_string_to_object_state() (private method)
*********************************************************************
*
*  Description:
*      convert string to object state
*
*  Input: Parameters
*      object_state_str - object state string

*  Returns:
*      fbe_metadata_element_state_t
*********************************************************************/

fbe_metadata_element_state_t utilityClass::convert_string_to_object_state(
    char *object_state_str)
{
    if (strcmp (object_state_str, "ACTIVE") == 0) {
        return FBE_METADATA_ELEMENT_STATE_ACTIVE;
    }
    if (strcmp (object_state_str, "PASSIVE") == 0) {
        return FBE_METADATA_ELEMENT_STATE_PASSIVE;
    }

    return FBE_METADATA_ELEMENT_STATE_INVALID;
}

/*********************************************************************
* utilityClass :: convert_diskname_to_bed
*********************************************************************/

fbe_status_t utilityClass::convert_diskname_to_bed(
    fbe_u8_t disk_name_a[], 
    fbe_job_service_bes_t *phys_location,
    char *temp)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_device_physical_location_t location ;
    fbe_esp_encl_mgmt_encl_info_t encl_info;
    fbe_u32_t  bus = 0;
    fbe_u32_t  encl = 0;
    fbe_u32_t  disk = 0;
    fbe_u32_t  scanned = 0;
    fbe_u8_t   *first_ocr;
    fbe_u8_t   *second_ocr;
    fbe_u8_t   *third_ocr;

    if(disk_name_a == NULL)
        return(FBE_STATUS_GENERIC_FAILURE);
    
     //Validate that input buffer is in correct format
     //Correct format is [0-9][0-9]_[0-9][0-9]_[0-9][0-9][0-9]
     
    first_ocr = (fbe_u8_t*) strchr((const char*)disk_name_a, '_');
    if (first_ocr == NULL){
        sprintf(temp, "The disk string is not in B_E_D format");
        return FBE_STATUS_INVALID;
    }

    // go to one position next in the string 
    first_ocr++;
    second_ocr = (fbe_u8_t*) strchr((const char*)first_ocr, '_');
    if (second_ocr == NULL){
        sprintf(temp, "The disk string is not in B_E_D format");
        return FBE_STATUS_INVALID;
    }

    // go to one position next in the string 
    second_ocr++;

    // Make sure that there are no more than 2 _ in input 
    third_ocr = (fbe_u8_t*) strchr((const char*)second_ocr, '_');
    if (third_ocr != NULL){
        sprintf(temp, "The disk string is not in B_E_D format");
        return FBE_STATUS_INVALID;
    }

    scanned = sscanf((const char*)disk_name_a,
              "%d_%d_%d", &(bus), &(encl), &(disk));
    
    if (scanned < 3){
        //sprintf(temp, "Unable to scan the input correctly");
        sprintf(temp, "Invalid disk number %s",disk_name_a);
        return FBE_STATUS_INVALID;
    }
    
    location.bus = bus;
    location.enclosure = encl;
    status = fbe_api_esp_encl_mgmt_get_encl_info(
        &location, 
        &encl_info);
    if (status != FBE_STATUS_OK){
        sprintf(temp, "Unable to obtain enclosure information "\
                      "for bus :%d ,encl:%d", 
                      location.bus,location.enclosure);
        return status;
    }

    if (encl_info.encl_type == FBE_ESP_ENCL_TYPE_UNKNOWN ){
        sprintf(temp, "Invalid Bus or Enclosure value %d_%d",
            location.bus,location.enclosure);
        return FBE_STATUS_INVALID;
    }

    if(encl_info.driveSlotCount <= disk){
        sprintf(temp, "Disk value exceeds number of slots supported "\
                    "in enclosure %d_%d",
                     location.bus,location.enclosure);
        return FBE_STATUS_INVALID;
    }

    phys_location->bus = bus;
    phys_location->enclosure = encl;
    phys_location->slot = disk;

    return FBE_STATUS_OK;
}
/*********************************************************************
* utilityClass Class :: convert_state_to_string()
*********************************************************************
*
*  Description:
*      convert object state to string.
*
*  Input: Parameters
*      fbe_lifecycle_state_t -  state

*  Returns:
*      const fbe_char_t* - constant string
*********************************************************************/
const fbe_char_t*
utilityClass::convert_state_to_string(fbe_lifecycle_state_t state)
{
    const fbe_char_t * pp_state_name ;
    switch (state) {
        case FBE_LIFECYCLE_STATE_SPECIALIZE:
            pp_state_name = "SPECIALIZE";
            break;
        case FBE_LIFECYCLE_STATE_ACTIVATE:
            pp_state_name = "ACTIVATE";
            break;
        case FBE_LIFECYCLE_STATE_READY:
            pp_state_name = "READY";
            break;
        case FBE_LIFECYCLE_STATE_HIBERNATE:
            pp_state_name = "HIBERNATE";
            break;
        case FBE_LIFECYCLE_STATE_OFFLINE:
            pp_state_name = "OFFLINE";
            break;
        case FBE_LIFECYCLE_STATE_FAIL:
            pp_state_name = "FAIL";
            break;
        case FBE_LIFECYCLE_STATE_DESTROY:
            pp_state_name = "DESTROY";
            break;
        case FBE_LIFECYCLE_STATE_PENDING_READY:
            pp_state_name = "PENDING_READY";
            break;
        case FBE_LIFECYCLE_STATE_PENDING_ACTIVATE:
            pp_state_name = "PENDING_ACTIVATE";
            break;
        case FBE_LIFECYCLE_STATE_PENDING_HIBERNATE:
            pp_state_name = "PENDING_HIBERNATE";
            break;
        case FBE_LIFECYCLE_STATE_PENDING_OFFLINE:
            pp_state_name = "PENDING_OFFLINE";
            break;
        case FBE_LIFECYCLE_STATE_PENDING_FAIL:
            pp_state_name = "PENDING_FAIL";
            break;
        case FBE_LIFECYCLE_STATE_PENDING_DESTROY:
            pp_state_name = "PENDING_DESTROY";
            break;
        case FBE_LIFECYCLE_STATE_NOT_EXIST:
            pp_state_name = "NOT_EXIST";
            break;
        case FBE_LIFECYCLE_STATE_INVALID:
            pp_state_name = "INVALID";
            break;
        default:
            pp_state_name = "INVALID";
    }
    return pp_state_name;
}

/*********************************************************************
* utilityClass Class :: convert_upercase_to_lowercase() (private method)
*********************************************************************
*
*  Description:
*      convert uppercase letter to lowercase letter.
*
*  Input: Parameters
*      *d_p - destination string pointer.
*      *s_p - source string pointer.
*  Returns:
*      void
*********************************************************************/
void utilityClass::convert_upercase_to_lowercase(char * d_p,char * s_p)
{
    fbe_u32_t fvar;
    size_t length;
    char temp;

    // First, take the length.
    length = strlen(s_p);

    for (fvar = 0; fvar < length; fvar++)
    {

        temp = s_p[fvar];

        if ((temp >= 'A') && (temp <= 'Z'))
        {
            d_p[fvar] = (32 + temp );//convert to lower case
        }
        else
        {
            d_p[fvar] =  temp ;
        }
    }                           //  End of "for..." loop.  
    d_p[length] = '\0';

    // That's it.  Return  
    return ;
}

/*********************************************************************
* utilityClass Class :: is_string(private method)
*********************************************************************
*
*  Description:
*      check input string is decimal or character string.
*
*  Input: Parameters
*      *s_p - destination string pointer.
*  Returns:
*      void
*********************************************************************/
bool utilityClass::is_string(char * s_p)
{
    bool isStr = FBE_TRUE;
    char * tmp = s_p;
    char* tmp1 = s_p;
    
    // Check for hex value.
    if(*tmp == '0' && (*(tmp+1) == 'x' || *(tmp+1) == 'X'))
    {
        isStr = FBE_FALSE;
        tmp+=2;
        while(*tmp!=CSX_EOS){
            if(!isxdigit(*tmp)){
                isStr = FBE_TRUE;
                break;
            }
            tmp++;
        }
    }

    //Check for decimal value
    else if(isdigit(*tmp1)){
        isStr = FBE_FALSE; 
        while(*tmp1!= CSX_EOS){
            if(!isdigit(*tmp1)){
                isStr = FBE_TRUE;
                break;
            }
            tmp1++;
        }
    }

    return isStr;
}


} /* fbeApixUtil end of namespace */

/*********************************************************************
* fbeApixUtil end of file
*********************************************************************/
