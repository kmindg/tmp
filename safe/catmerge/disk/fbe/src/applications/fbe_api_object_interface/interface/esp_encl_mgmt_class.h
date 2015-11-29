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
 *      This file defines the ESP Enclosure Management class.
 *
 *  History:
 *      20-July-2011 : inital version
 *
 *********************************************************************/

#ifndef T_ESP_ENCL_MGMT_CLASS_H
#define T_ESP_ENCL_MGMT_CLASS_H

#ifndef T_ESP_CLASS_H
#include "esp_class.h"
#endif

/*********************************************************************
 *          ESP Encl Mgmt class : bESP base class
 *********************************************************************/
class espEnclMgmt : public bESP
{
    protected:

        // Every object has an idnumber
        unsigned idnumber;
        unsigned espEnclMgmtCount;

        // interface object name
        char  * idname;

        // ESP Encl Mgmt function calls and usage
        char * espEnclMgmtInterfaceFuncs;
        char * usage_format;

        // ESP Encl Mgmt fbe api data structures
        fbe_u32_t       enclCount;
        fbe_esp_encl_mgmt_get_encl_loc_t      enclLocationInfo;
        fbe_device_physical_location_t  location; 
        fbe_esp_encl_mgmt_get_eir_info_t eir_info;
        fbe_u32_t           deviceCount ;
        fbe_esp_encl_mgmt_lcc_info_t    lccInfo;
        fbe_esp_encl_mgmt_encl_info_t  EnclInfo;
        char enclFaultReasonString[500];

        // private methods
        void edit_encl_info( fbe_esp_encl_mgmt_get_encl_loc_t* encl_info);
        void edit_eir_info(fbe_esp_encl_mgmt_get_eir_info_t* eirInfo);
        void edit_lcc_info( fbe_esp_encl_mgmt_lcc_info_t* lccInfo_p);
        fbe_status_t get_and_verify_bus_number(char ** argv, 
            fbe_device_physical_location_t*  location_p );
        fbe_status_t get_and_verify_enclosure_number(char ** argv,
            fbe_device_physical_location_t*  location_p );
        fbe_status_t get_and_verify_slot_number(char ** argv,
            fbe_device_physical_location_t*  location_p );
        void edit_encl_mgmt_info( fbe_esp_encl_mgmt_encl_info_t* encl_info);
        void Esp_Encl_Intializing_variable();
        char * decode_encl_fault_led_reason(fbe_encl_fault_led_reason_t enclFaultLedReason);
        char * decode_led_status(fbe_led_status_t ledStatus);
        fbe_char_t * get_encl_type_string(fbe_esp_encl_type_t enclosure_type);
        fbe_char_t * get_enclShutdownReasonString(fbe_u32_t shutdownReason);
        char *  decode_encl_state(fbe_esp_encl_state_t enclState);
        char * decode_encl_fault_symptom(fbe_esp_encl_fault_sym_t enclFaultSymptom);
        
    public:

        // Constructor/Destructor
        espEnclMgmt();
        ~espEnclMgmt(){}

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        void     dumpme(void);

        // Select method to call FBE API
        fbe_status_t select(int index, int argc, char **argv);

        // FBE API EnclMgmt info methods
        fbe_status_t get_total_encl_count(int argc, char**argv);
        fbe_status_t get_encl_location(int argc, char ** argv);
        fbe_status_t get_encl_count_on_bus(int argc, char ** argv);
        fbe_status_t get_eir_info(int argc, char ** argv);
        fbe_status_t get_lcc_count(int argc, char ** argv);
        fbe_status_t get_lcc_info(int argc, char ** argv);
        fbe_status_t get_ps_count(int argc, char ** argv);
        fbe_status_t get_fan_count(int argc, char ** argv);
        fbe_status_t get_drive_slot_count(int argc, char ** argv);
        fbe_status_t get_encl_info(int argc, char ** argv);

};

#endif
