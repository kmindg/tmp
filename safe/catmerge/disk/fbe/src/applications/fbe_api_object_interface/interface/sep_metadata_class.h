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
 *      This file defines the SEP Metadata class.
 *
 *  History:
 *      27-July-2011 : inital version
 *
 *********************************************************************/

#ifndef T_SEP_METADATA_CLASS_H
#define T_SEP_METADATA_CLASS_H

#ifndef T_SEP_CLASS_H
#include "sep_class.h"
#endif

/*********************************************************************
 *          Metadata class : bSEP base class
 *********************************************************************/

class sepMetadata : public bSEP
{
    protected:
        
        // Every object has an idnumber
        unsigned idnumber;
        unsigned sepMetadataCount;
        char params_temp[250];

        // interface object name
        char  * idname;
  
        // Metadata Interface function calls and usage 
        char * sepMetadataInterfaceFuncs; 
        char * usage_format;

        // Data Structures
        fbe_object_id_t object_id;
        fbe_metadata_element_state_t obj_state;

        // Private methods
        
        // Display virtual drive spare info
        void edit_virtual_drive_spare_info(
            fbe_spare_drive_info_t *spare_drive_info);
        void Sep_Metadata_Intializing_variable();
        
    public:

        // Constructor/Destructor
        sepMetadata();
        ~sepMetadata(){}

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        void     dumpme(void);

        // Select method to call FBE API
        fbe_status_t select(int index, int argc, char *argv[]); 
       
        // FBE API set all objects state
        fbe_status_t set_all_objects_state(int argc, char **argv);

        // FBE API set single object state
        fbe_status_t set_single_object_state(int argc, char **argv);
        
        // ------------------------------------------------------------
};

#endif