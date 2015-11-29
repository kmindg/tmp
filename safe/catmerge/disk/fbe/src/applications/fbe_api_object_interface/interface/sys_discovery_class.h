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
 *      This file defines the SYS DISCOVERY INTERFACE class.
 *
 *  History:
 *      4-July-2011 : initial version
 *
 *********************************************************************/

#ifndef T_SYS_DISCOVERY_CLASS_H
#define T_SYS_DISCOVERY_CLASS_H

#ifndef T_SYS_CLASS_H
#include "sys_class.h"
#endif

/*********************************************************************
 *          System Discovery class : bSYS base class
 *********************************************************************/

class SysDiscovery : public bSYS
{
    protected:
        
        // Every object has an idnumber
        unsigned idnumber;
        unsigned sysDiscoveyCount;
        char params_temp[5000];

        // interface object name
        char  * idname;
        
        // System discovery Interface function calls and usage 
        char * SysDiscoveryInterfaceFuncs; 
        char * usage_format;

        // System discovery Interface fbe api data structure
        fbe_object_id_t object_id;
        fbe_lifecycle_state_t lifecycle_state;
        fbe_object_death_reason_t death_reason;
        const fbe_u8_t * death_reason_str;
        fbe_u32_t total_objects;
        fbe_class_id_t class_id;
        fbe_const_class_info_t *class_info_p;
        fbe_object_id_t *object_list_p;
        fbe_u32_t found_objects_count;      
        fbe_u32_t object_count;
        fbe_u32_t object_index;

    private:
        // private methods

        // display object list of a package
        void edit_objects_of_package(
            fbe_object_id_t *object_list_p,
            fbe_u32_t total_objects);
        
        // Get class the information for object
        fbe_status_t get_class_info(
            fbe_class_id_t class_id,
            fbe_const_class_info_t **class_info_p);
        
        // Get bus, enclosure, slot info for object
        fbe_status_t get_bus_enclosure_slot_info(
            fbe_object_id_t object_id,
            fbe_class_id_t class_id,
            fbe_port_number_t *port_p,
            fbe_enclosure_number_t *enc_p,
            fbe_enclosure_slot_number_t *slot_p,
            fbe_package_id_t package_id);

        void Sys_Discovery_Intializing_variable();

    public:

        // Constructor/Destructor
        SysDiscovery();
        ~SysDiscovery(){}

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        void     dumpme(void);

        // Select method to call FBE API
        fbe_status_t select(int index, int argc, char *argv[]);

        //get object lifecycle state
        fbe_status_t get_object_lifecycle_state(int argc, char **argv);

        //get object death reason
        fbe_status_t get_object_death_reason(int argc, char **argv);

        //get total object 
        fbe_status_t get_total_objects(int argc, char **argv);

        //get object class id
        fbe_status_t get_object_class_id(int argc, char **argv);

        //display objects of a specific package
        fbe_status_t discover_objects_of_package(int argc, char **argv);

        // get the psm lun object id
        fbe_status_t get_psm_lun_object_id(int argc, char **argv);

        // get the vault lun object id
        fbe_status_t get_vault_lun_object_id(int argc, char **argv);

        // get the psm rg object id
        fbe_status_t get_psm_rg_object_id(int argc, char **argv);

        // get the vault rg object id
        fbe_status_t get_vault_rg_object_id(int argc, char **argv);

        //print lifecyclestate id & name
        void edit_object_lifecycle_state(fbe_lifecycle_state_t *lifecycle_state);

        // ------------------------------------------------------------
};

#endif