/*********************************************************************
 * Copyright (C) EMC Corporation, 1989 - 2011
 * All Rights Reserved.
 * Licensed Material-Property of EMC Corporation.
 * This software is made available solely pursuant to the terms
 * of a EMC license agreement which governs its use.
 *********************************************************************/

/**********************************************************************
 *                       fbe_apix_util_class.h
 **********************************************************************
 *
 *  Description:
 *    This header file contains class definitions for use with the
 *    fbe_apix_util.cpp module.
 *
 *  Table of Contents:
 *     fbeApixUtilClass
 *
 *  History:
 *      19-April-2011 : initial version
 *
 *********************************************************************/

#ifndef T_FBE_APIX_UTIL_CLASS_H
#define T_FBE_APIX_UTIL_CLASS_H

// fbe includes 
#ifndef PRIVATE_H
#include "private.h" 
#endif

// Global variable declarations and assignments
#ifndef FBEAPIX_H
#include "fbe_apix.h" 
#endif

/**********************************************************************
 *                          fbeApixUtilClass                          
 **********************************************************************
 *
 *  Description:
 *      This class contains various utility methods that are used by
 *      fbe_api_object_interface library functions. 
 *
 *  Notes:
 *      This is a general class for the fbe_api_object_interface  
*       utility methods that do not require logging.
 *
 *  History:
 *      19-April-2011 : initial version
 *
 *********************************************************************/

namespace fbeApixUtil
{
    class utilityClass
    {
        public:

        // Constructor/Destructor
        utilityClass(){};
        ~utilityClass(){};

        // Return string pointer for speed capability
        static const char* speed_capability(fbe_u32_t speed_capability);

        // Return string pointer for lifecycle state
        static const char* lifecycle_state(fbe_u32_t state);

        // Return string pointer for firmware download process state
        static const char* fwdl_process_state(fbe_u32_t state);

        // Return string pointer for firmware download process fail code
        static const char* fwdl_process_fail_code(fbe_u32_t fail_code);

        // Return string pointer for power input status
        static const char* convert_power_status_number_to_string(
            fbe_u32_t fbe_power_status_code);

        // Return string pointer for sp id
        static const char* convert_sp_id_to_string(fbe_u32_t sp_id);

        // Return string pointer for sps id
        static const char* convert_sps_id_to_string(fbe_u32_t sps_id);

        // Return string pointer for ac/dc input
        static const char* convert_ac_dc_input_to_string(
            fbe_u32_t input);

        // Return string pointer for hardware module type
        static const char* convert_hw_module_type_to_string(
            fbe_u32_t type);

        // Return string pointer for led blink rate
        static const char* convert_led_blink_rate_to_string(
            LED_BLINK_RATE blink_rate);

        // Return string pointer for io module location
        static const char* convert_io_module_location_to_string(
            fbe_io_module_location_t location);

        // Return string pointer for resume prom attributes
        static const char* convert_resume_prom_attributes_to_string(
            fbe_pe_resume_prom_id_t type);

        // Return string pointer for led color type
        static const char * convert_led_color_type_to_string(
            LED_COLOR_TYPE ioPortLEDColor);

        // Return string pointer for lan config mode
        static const char * convert_lan_config_mode_to_string(
            VLAN_CONFIG_MODE vLANConfigMode);

        // Return string pointer for lan port speed
        static const char * convert_lan_port_speed_to_string(
            PORT_SPEED port_speed);

        // Return string pointer for mgmt port speed
        static const char * convert_mgmt_port_speed_to_string(
            fbe_mgmt_port_speed_t port_speed);

        //sp name to its index
        static fbe_u8_t convert_sp_name_to_index(char ** argv);

        //weekday name to its index
        static fbe_u16_t convert_sp_weekday_to_index(char ** argv);

        //convert string to device type
        static fbe_u64_t convert_string_to_device_type(
            char *device_type_str);

        // convert string to package id
        static fbe_package_id_t convert_string_to_package_id(
            char *package_str);

        // convert string to lifecycle state
        static fbe_lifecycle_state_t convert_string_to_lifecycle_state(
            char *lifecycle_state_str);

        // Return string pointer for RAID group type
        static const char* convert_rg_type_to_string(
            fbe_u32_t fbe_raid_group_type);

        // Return string pointer for fbe drive type
        static const char* convert_drive_type_to_string(
            fbe_u32_t fbe_drive_type);

        // Return string pointer for fbe configured block size
        static const char* convert_block_size_to_string(
            fbe_u32_t fbe_block_size);

        // Return string pointer for the object state
        static const char* convert_object_state_to_string(
            fbe_u32_t fbe_object_state);

        // convert string to object state
        static fbe_metadata_element_state_t convert_string_to_object_state(
            char *object_state_str);

        // append a string onto another string
        static char* append_to(char * stringx, char * stringy);

        // splits the disk name into bus, encl, slot
        static fbe_status_t convert_diskname_to_bed(
            fbe_u8_t disk_name_a[],
            fbe_job_service_bes_t *phys_location,
            char *temp);

        // Return string pointer for RAID group life cycle state
        static const fbe_char_t * convert_state_to_string(fbe_lifecycle_state_t state);
        
        // convert upercase letter to lower case
        static void convert_upercase_to_lowercase(char * d_p,char * s_p);

        // Check if input is a string or a decimal
        static bool is_string(char * s_p);
    };
}

#endif /* T_FBE_APIX_UTIL_CLASS_H */
