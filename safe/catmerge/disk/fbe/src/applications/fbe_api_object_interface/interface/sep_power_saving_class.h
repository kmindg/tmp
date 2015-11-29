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
 *      This file defines the SEP POWER SAVING INTERFACE class.
 *
 *  History:
 *      13-Jun-2011 : inital version
 *
 *********************************************************************/

#ifndef T_SEP_POWER_SAVING_CLASS_H
#define T_SEP_POWER_SAVING_CLASS_H

#ifndef T_SEP_CLASS_H
#include "sep_class.h"
#endif

/*********************************************************************
 *          SEP POWER SAVING class : bSEP base class
 *********************************************************************/

class sepPowerSaving : public bSEP
{
    protected:

        // Every object has an idnumber
        unsigned idnumber;
        unsigned sepPowerSavingCount;
        char params_temp[500];

        // interface object name
        char  * idname;
    
        // SEP POWER SAVING Interface function calls and usage
        char * sepPowerSavingInterfaceFuncs;
        char * usage_format;
    
        // SEP POWER SAVING interface fbe api data structures
        fbe_system_power_saving_info_t system_psi;
        fbe_base_config_object_power_save_info_t object_psi;
        fbe_object_id_t object_id;
        fbe_u64_t wakeup_minutes;
        fbe_u64_t idle_time_seconds;

        // private methods
        void edit_system_power_save_info(
            fbe_system_power_saving_info_t *power_save_info);

        void edit_object_power_save_info(
            fbe_base_config_object_power_save_info_t *power_save_info);

        void Sep_Powersaving_Intializing_variable();

    public:

        // Constructor/Destructor
        sepPowerSaving();
        ~sepPowerSaving(){}

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        void     dumpme(void);
        
        // Select method to call FBE API
        fbe_status_t select(int index, int argc, char *argv[]);

        // get system power save info
        fbe_status_t get_system_power_save_info(int argc, char **argv);
        
        // enable system power save
        fbe_status_t enable_system_power_save(int argc, char **argv);

        // disable system power save
        fbe_status_t disable_system_power_save(int argc, char **argv);

        // get object power save info
        fbe_status_t get_object_power_save_info(int argc, char **argv);

        // enable object power save
        fbe_status_t enable_object_power_save(int argc, char **argv);

        // disable object power save
        fbe_status_t disable_object_power_save(int argc, char **argv);

        // set object power save idle time
        fbe_status_t set_object_power_save_idle_time(
            int argc, 
            char **argv);

        // enable raid group power save
        fbe_status_t enable_raid_group_power_save(
            int argc,
            char **argv);

        // disable raid group power save
        fbe_status_t disable_raid_group_power_save(
            int argc,
            char **argv);
        
        // set raid group power save idle time
        fbe_status_t set_raid_group_power_save_idle_time(
            int argc, 
            char **argv);

        // set power save periodic wakeup 
        fbe_status_t set_power_save_periodic_wakeup(
            int argc, 
            char **argv);

        // ------------------------------------------------------------
};

#endif
