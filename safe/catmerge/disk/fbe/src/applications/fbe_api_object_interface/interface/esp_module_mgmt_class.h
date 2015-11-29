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
 *      This file defines the ESP MODULE MANAGEMENT class.
 *
 *  History:
 *      15-July-2011 : initial version
 *
 *********************************************************************/

#ifndef T_ESP_MODULE_MGMT_CLASS_H
#define T_ESP_MODULE_MGMT_CLASS_H

#ifndef T_ESP_CLASS_H
#include "esp_class.h"
#endif

/*********************************************************************
 *          ESP MODULE MGMT class : bESP base class
 *********************************************************************/
class espModuleMgmt : public bESP
{
    protected:

        // Every object has an idnumber
        unsigned idnumber;
        unsigned espModuleMgmtCount;

        // interface object name
        char  * idname;

        // ESP MODULE MGMT Interface function calls and usage
        char * espModuleMgmtInterfaceFuncs;
        char * usage_format;

        // ESP MODULE interface fbe api data structures
        fbe_u32_t sp; // SP id
        fbe_u64_t type;// MODULE device type 
        fbe_u32_t slot;// Slot Number 
        fbe_esp_module_io_module_info_t io_module_info;
        fbe_esp_module_mgmt_module_status_t module_status_info;
        fbe_esp_module_io_port_info_t io_port_info;
        fbe_esp_module_limits_info_t limits_info;
        fbe_esp_module_mgmt_get_mgmt_port_config_t mgmt_port_config;
        fbe_esp_module_mgmt_get_mgmt_comp_info_t mgmt_comp_info;
        fbe_esp_module_mgmt_set_mgmt_port_config_t set_mgmt_port_config;

        //displays io module info.
        void edit_io_module_info(
                     fbe_esp_module_io_module_info_t *io_module_info);

        //displays io module port info
        void edit_io_module_port_info(
                             fbe_esp_module_io_port_info_t* io_port_info);

        //displays limits info
        void edit_limits_info(fbe_esp_module_limits_info_t *limits_info);

        //displays config port info
        void edit_config_port_info(
                      fbe_esp_module_mgmt_get_mgmt_port_config_t *mgmt_port_config);

        //displays component info
        void edit_comp_info(
                          fbe_esp_module_mgmt_get_mgmt_comp_info_t *mgmt_comp_info);

        //displays module status
        void edit_module_status(
                     fbe_esp_module_mgmt_module_status_t *module_status_info);

        //get module revert action
        fbe_status_t get_module_revert_action(char **argv);

        //get port auto neg action
        fbe_status_t get_port_auto_neg_action(char **argv);

        //get port speed
        fbe_status_t get_port_speed(char **argv);

        //get port duplex mode 
        fbe_status_t get_port_duplex_mode(char **argv);

        void Esp_Module_Intializing_variable();

    public:

        // Constructor/Destructor
        espModuleMgmt();
        ~espModuleMgmt(){}

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        void     dumpme(void);

        // Select method to call FBE API
        fbe_status_t select(int index, int argc, char *argv[]);

        // FBE API for module mgmt interface
        // get io module info
        fbe_status_t get_io_module_info(int argc, char ** argv);

        // get module status
        fbe_status_t get_module_status(int argc, char ** argv);

        // get io module port info
        fbe_status_t get_io_module_port_info(int argc, char ** argv);

        // get limits info
        fbe_status_t get_limits_info(int argc, char ** argv);

        // get config port info
        fbe_status_t get_config_port_info(int argc, char ** argv);

        // get component info
        fbe_status_t get_comp_info(int argc, char ** argv);

        //set port speed
        fbe_status_t set_port_speed(int argc, char ** argv);

};

#endif
