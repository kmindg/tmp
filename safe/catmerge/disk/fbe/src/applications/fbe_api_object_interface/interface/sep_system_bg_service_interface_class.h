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
 *      This file defines the SEP SYSTEM BG SERVICE INTERFACE class.
 *
 *  History:
 *      09-April-2012 : inital version
 *
 *********************************************************************/

#ifndef T_SEP_SYSTEM_BG_SERVICE_INTERFACE_CLASS_H
#define T_SEP_SYSTEM_BG_SERVICE_INTERFACE_CLASS_H

#ifndef T_SEP_CLASS_H
#include "sep_class.h"
#endif

/*********************************************************************
 *          SystemBgService class : bSEP base class
 *********************************************************************/

class sepSystemBgService : public bSEP
{
    protected:
        // Every object has an idnumber
        unsigned idnumber;
        unsigned systemBgServiceCount;

        // interface object name
        char  * idname;
  
        // Provision Drive Interface function calls and usage 
        char *  sepSystemBgServiceInterfaceFuncs; 
        char * usage_format;

        fbe_bool_t verify_enabled;
        
    public:
        
        // Constructor/Destructor
        sepSystemBgService();
        ~sepSystemBgService(){}

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        int      MyCountIs(void);
        void     dumpme(void);

        // Select method to call FBE API
        fbe_status_t select(int index, int argc, char *argv[]); 

        // sniff verify methods
        fbe_status_t set_system_disable_sniff_verify(
            int argc, char ** argv);
        fbe_status_t set_system_enable_sniff_verify(
            int argc, char ** argv);
        fbe_status_t get_is_system_sniff_verify_enabled(
            int argc, char ** argv);

};

#endif
