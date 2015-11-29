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
 *      This file defines the SEP Database class.
 *
 *  History:
 *      21-July-2011 : inital version
 *
 *********************************************************************/

#ifndef T_SEP_DATABASE_CLASS_H
#define T_SEP_DATABASE_CLASS_H

#ifndef T_SEP_CLASS_H
#include "sep_class.h"
#endif

/*********************************************************************
 *          Database class : bSEP base class
 *********************************************************************/

class sepDatabase : public bSEP
{
    protected:
        
        // Every object has an idnumber
        unsigned idnumber;
        unsigned sepDatabaseCount;

        // interface object name
        char  * idname;
  
        // Database Interface function calls and usage 
        char * sepDatabaseInterfaceFuncs; 
        char * usage_format;

        // Variables
        fbe_object_id_t object_id;
        fbe_bool_t b_found;
        fbe_bool_t is_enable;
        fbe_database_lun_info_t lun_info;
        fbe_apix_rg_drive_list_t lu_drives;
        
        // Private methods
        
        // Display lun info 
        char * edit_lun_info(
            fbe_database_lun_info_t *lun_info, 
            fbe_apix_rg_drive_list_t *lu_drives);
        
        // Display if the object is system object or not
        void edit_is_system_object(fbe_bool_t b_found);
        void Sep_Database_Intializing_variable();

    public:

        // Constructor/Destructor
        sepDatabase();
        ~sepDatabase(){}

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        void     dumpme(void);

        // Select method to call FBE API
        fbe_status_t select(int index, int argc, char *argv[]); 

        // FBE API LUN (get) methods
        fbe_status_t get_lun_object_id (int argc, char ** argv);
        fbe_status_t get_lun_id_by_obj (int argc, char ** argv);
        fbe_status_t get_lun_info (int argc, char ** argv);

        // FBE API Raid Group (get) methods
        fbe_status_t get_rg_object_id (int argc, char ** argv);
        fbe_status_t get_rg_id_by_obj (int argc, char ** argv);

        // FBE API function to check if object is system object
        fbe_status_t is_system_object(int argc, char **argv);

        //Set load balance
        fbe_status_t  set_load_balance(int argc, char **argv);
        
        // ------------------------------------------------------------
};

#endif