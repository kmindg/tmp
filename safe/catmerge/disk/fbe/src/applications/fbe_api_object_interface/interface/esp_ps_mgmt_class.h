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
 *      This file defines the ESP PS INTERFACE class.
 *
 *  History:
 *      29-July-2011 : inital version
 *
 *********************************************************************/

#ifndef T_ESP_PS_MGMT_CLASS_H
#define T_ESP_PS_MGMT_CLASS_H

#ifndef T_ESP_CLASS_H
#include "esp_class.h"
#endif

/*********************************************************************
 *          ESP PS MGMT class : bESP base class
 *********************************************************************/

class espPsMgmt : public bESP
{
    protected:

        // Every object has an idnumber
        unsigned idnumber;
        unsigned espPsMgmtCount;

        // interface object name
        char  * idname;

        // ESP PS Mgmt Interface function calls and usage
        char * espPsMgmtInterfaceFuncs;
        char * usage_format;

        // ESP PS MGMT interface fbe api data structures
        fbe_device_physical_location_t location;
        fbe_u32_t PsCount;
        fbe_esp_ps_mgmt_ps_info_t psInfo;

        // private methods
        void initialize();

        // display ps count
        void edit_ps_count(fbe_u32_t PsCount);

        // display ps count
        void edit_ps_info(fbe_esp_ps_mgmt_ps_info_t *psInfo);

    public:

        // Constructor/Destructor
        espPsMgmt();
        ~espPsMgmt(){}

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        void     dumpme(void);

        // Select method to call FBE API
        fbe_status_t select(int index, int argc, char *argv[]);

        // FBE API get ps count from PS management object
        fbe_status_t get_ps_count(int argc, char ** argv);
        
        // FBE API power down ps
        fbe_status_t power_down_ps(int argc, char ** argv);

        // FBE API get ps info
        fbe_status_t get_ps_info(int argc, char ** argv);

        // ------------------------------------------------------------
};

#endif
