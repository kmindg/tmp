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
 *      This file defines the ESP DRIVE INTERFACE class.
 *
 *  History:
 *      28-Jun-2011 : inital version
 *
 *********************************************************************/

#ifndef T_ESP_DRIVE_MGMT_CLASS_H
#define T_ESP_DRIVE_MGMT_CLASS_H

#ifndef T_ESP_CLASS_H
#include "esp_class.h"
#endif

/*********************************************************************
 *          ESP DRIVE MGMT class : bESP base class
 *********************************************************************/

class espDriveMgmt : public bESP
{
    protected:

        // Every object has an idnumber
        unsigned idnumber;
        unsigned espDriveMgmtCount;

        // interface object name
        char  * idname;

        // ESP Drive Mgmt Interface function calls and usage
        char * espDriveMgmtInterfaceFuncs;
        char * usage_format;

        fbe_job_service_bes_t phys_location;

        // ESP DRIVE MGMT interface fbe api data structures
        fbe_esp_drive_mgmt_drive_info_t drive_info;

        fbe_device_physical_location_t location;

        fbe_base_object_mgmt_drv_dbg_action_t cru_on_off_action;

        // private methods
        void initialize();

        // display drive info
        void edit_drive_mgmt_info(
            fbe_esp_drive_mgmt_drive_info_t *drive_info);

        // display drive debug action
        fbe_status_t get_drive_debug_action(char **argv);

        // send drive debug action
        fbe_status_t drive_mgmt_send_debug_control(int argc , 
            char **argv);
        
    public:

        // Constructor/Destructor
        espDriveMgmt();
        ~espDriveMgmt(){}

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        void     dumpme(void);

        // Select method to call FBE API
        fbe_status_t select(int index, int argc, char *argv[]);

        // FBE API get drive mgmt info
        fbe_status_t get_drive_mgmt_info(int argc, char ** argv);

        // FBE API send drive log command
        fbe_status_t get_drive_mgmt_drive_log(int argc, char ** argv);
        
        // FBE API send drive fuel gauge command
        fbe_status_t get_drive_mgmt_fuel_gauge(int argc, char ** argv);
        
        // FBE API get download drive status
        fbe_status_t get_drive_mgmt_dload_drive_status(
            int argc, 
            char **argv);

        // FBE API get drive management download process info
        fbe_status_t get_drive_mgmt_dload_process_info(
            int argc, 
            char ** argv);

        // FBE API drive mgmt abort download
        fbe_status_t drive_mgmt_abort_dload(int argc, char ** argv);
        // ------------------------------------------------------------
};

#endif
