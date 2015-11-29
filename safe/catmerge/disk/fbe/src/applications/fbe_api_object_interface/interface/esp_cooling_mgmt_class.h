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
 *      This file defines the COOLING MGMT class.
 *
 *  History:
 *      05-Aug-2011 : inital version
 *
 *********************************************************************/

#ifndef T_ESP_COOLING_MGMT_CLASS_H
#define T_ESP_COOLING_MGMT_CLASS_H

#ifndef T_ESP_CLASS_H
#include "esp_class.h"
#endif

/*********************************************************************
 *          ESP COOLING MGMT class : bESP base class
 *********************************************************************/
class espCoolingMgmt : public bESP
{
    protected:

        // Every object has an idnumber
        unsigned idnumber;
        unsigned espCoolingMgmtCount;

        // interface object name
        char  * idname;

        // ESP COOLING MGMT function calls and usage
        char * espCoolingMgmtInterfaceFuncs;
        char * usage_format;

        // ESP COOLING MGMT fbe api data structures
        fbe_esp_cooling_mgmt_fan_info_t  fan_info;
        fbe_device_physical_location_t location;
        char env_status_str[40];

        // private methods
        fbe_status_t get_and_verify_bus_number(char ** argv, 
            fbe_device_physical_location_t*  location_p );
        fbe_status_t get_and_verify_enclosure_number(char ** argv,
            fbe_device_physical_location_t*  location_p );
        fbe_status_t get_and_verify_slot_number(char ** argv,
            fbe_device_physical_location_t*  location_p );

    public:

        // Constructor/Destructor
        espCoolingMgmt();
        ~espCoolingMgmt(){}

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        void     dumpme(void);

        // Select method to call FBE API
        fbe_status_t select(int index, int argc, char *argv[]);

        // FBE API get Fan info
        fbe_status_t get_esp_fan_info(int argc, char ** argv);

};

#endif
