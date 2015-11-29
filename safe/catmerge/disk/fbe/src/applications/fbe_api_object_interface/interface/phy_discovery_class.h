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
 *      This file defines the Physical Discovery Interface class.
 *
 *  History:
 *      29-July-2011 : inital version
 *
 *********************************************************************/

#ifndef T_PHYDISCOVERYCLASS_H
#define T_PHYDISCOVERYCLASS_H

#ifndef T_PHYCLASS_H
#include "phy_class.h"
#endif

/*********************************************************************
 *          Physical Drive class : bPhysical base class
 *********************************************************************/

class PhyDiscovery : public bPhysical
{
    protected:
        
        // Every object has an idnumber
        unsigned idnumber;
        unsigned phyDiscoveryCount;
        char params_temp[250];

        // interface object name
        char  * idname;
  
        // Physical discovery Interface function calls and usage 
        char * PhyDiscoveryInterfaceFuncs; 
        char * usage_format;

        // Physical discovery Interface fbe api data structure
        fbe_port_number_t port_num;
        fbe_enclosure_number_t encl_num;
        fbe_enclosure_slot_number_t drive_num;
        fbe_lifecycle_state_t lifecycle_state;
        fbe_package_id_t package_id;

    public:

        // Constructor/Destructor
        PhyDiscovery();
        ~PhyDiscovery(){}

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        void     dumpme(void);

        // Select method to call FBE API
        fbe_status_t select(int index, int argc, char *argv[]);

        // get object port number
        fbe_status_t get_object_port_number(int argc, char **argv);

        // get object enclosure number
        fbe_status_t get_object_encl_number(int argc, char **argv);

        // get object drive number number
        fbe_status_t get_object_drive_number(int argc, char **argv);
        
        // FBE API set object to specific state
        fbe_status_t set_object_to_state(int argc, char **argv);

        // ------------------------------------------------------------
};

#endif