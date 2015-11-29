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
 *      This file defines the ESP SPS MANAGEMENT class.
 *
 *  History:
 *      15-July-2011 : initial version
 *
 *********************************************************************/

#ifndef T_ESP_SPS_MGMT_CLASS_H
#define T_ESP_SPS_MGMT_CLASS_H

#ifndef T_ESP_CLASS_H
#include "esp_class.h"
#endif

/*********************************************************************
 *          ESP SPS MGMT class : bESP base class
 *********************************************************************/
class espSpsMgmt : public bESP
{
    protected:

        // Every object has an idnumber
        unsigned idnumber;
        unsigned espSpsMgmtCount;

        // interface object name
        char  * idname;

        // ESP SPS Interface function calls and usage
        char * espSpsMgmtInterfaceFuncs;
        char * usage_format;

        // ESP SPS interface fbe api data structures
        fbe_esp_sps_mgmt_spsInputPower_t spsInputPowerInfo;
        fbe_esp_sps_mgmt_spsStatus_t spsStatusInfo;
        fbe_esp_sps_mgmt_spsManufInfo_t spsManufInfo;
        fbe_esp_sps_mgmt_spsTestTime_t spsTestTimeInfo;
        fbe_status_t bus_status, encl_status;
        
        // private methods
        //displays sps input power info
        void edit_sps_input_power(
                     fbe_esp_sps_mgmt_spsInputPower_t *spsInputPowerInfo);

        //displays sps status
        void edit_sps_status(
                     fbe_esp_sps_mgmt_spsStatus_t *spsStatusInfo);

        //displays sps manufacturer info
        void edit_sps_manufacturer_information(
                     fbe_esp_sps_mgmt_spsManufInfo_t *spsManufInfo);

        //displays sps test time
        void edit_sps_test_time(
                     fbe_esp_sps_mgmt_spsTestTime_t *spsTestTimeInfo);

        void Esp_Sps_Intializing_variable();
        fbe_status_t get_and_verify_bus_number(char ** argv, 
            fbe_device_physical_location_t*  location_p );
        fbe_status_t get_and_verify_enclosure_number(char ** argv,
            fbe_device_physical_location_t*  location_p );
        
    public:

        // Constructor/Destructor
        espSpsMgmt();
        ~espSpsMgmt(){}

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        void     dumpme(void);

        // Select method to call FBE API
        fbe_status_t select(int index, int argc, char *argv[]);

        // FBE API get SPS mgmt info
        // get sps input power info
        fbe_status_t get_sps_input_power(int argc, char ** argv);
        
        // get sps status
        fbe_status_t get_sps_status(int argc, char ** argv);

        // get sps manufacturer info
        fbe_status_t get_sps_manufacturer_information(int argc, char ** argv);

        // get sps test time
        fbe_status_t get_sps_test_time(int argc, char ** argv);

        // power down the sps
        fbe_status_t sps_power_down(int argc, char ** argv);

        // set sps test time
        fbe_status_t set_sps_test_time(int argc, char ** argv);

};

#endif
