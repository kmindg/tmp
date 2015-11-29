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
 *      This file defines the SEP Virtual Drive (VD) class.
 *
 *  History:
 *      20-July-2011 : inital version
 *
 *********************************************************************/

#ifndef T_SEP_VIRTUAL_DRIVE_CLASS_H
#define T_SEP_VIRTUAL_DRIVE_CLASS_H

#ifndef T_SEP_CLASS_H
#include "sep_class.h"
#endif

/*********************************************************************
 *          Virtual Drive class : bSEP base class
 *********************************************************************/

class sepVirtualDrive : public bSEP
{
    protected:
        
        // Every object has an idnumber
        unsigned idnumber;
        unsigned vdrCount;

        // interface object name
        char  * idname;
  
        // Virtual Drive Interface function calls and usage 
        char * sepVirtualDriveInterfaceFuncs; 
        char * usage_format;

        // Data Structures
        fbe_spare_drive_info_t spare_drive_info;
        fbe_spare_swap_command_t copy_request_type;    
        fbe_bool_t unsued_drive_as_spare_flag;
 	 fbe_u64_t swap_in_time_in_sec;
	 fbe_bool_t enable;
	 char enable_or_disable[8];

        // Private methods
        
        // Display virtual drive spare info
        void edit_virtual_drive_spare_info(
            fbe_spare_drive_info_t *spare_drive_info);
        
        // Display unused drive as spare flag
        void edit_unused_drive_as_spare_flag(
            fbe_bool_t unsued_drive_as_spare_flag);

        //initialize variables
        void sep_virtual_drive_initializing_variable();

    public:

        // Constructor/Destructor
        sepVirtualDrive();
        ~sepVirtualDrive(){}

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        void     dumpme(void);

        // Select method to call FBE API
        fbe_status_t select(int index, int argc, char *argv[]); 
       
        // get virtual drive spare info
        fbe_status_t get_virtual_drive_spare_info(
            int argc, 
            char **argv);
      
        // get unused drive as spare flag
        fbe_status_t get_unused_drive_as_spare_flag(
            int argc, 
            char **argv);

        // set unused drive as spare flag
        fbe_status_t set_unused_drive_as_spare_flag(
            int argc, 
            char **argv);

	// update the spare swap in time
	fbe_status_t update_permanent_spare_swap_in_time(
	    int argc,
	    char **argv);

	//control automatic hot spare
	fbe_status_t set_control_automatic_hot_spare(
	    int argc, 
   	    char **argv);

        // ------------------------------------------------------------
};

#endif
