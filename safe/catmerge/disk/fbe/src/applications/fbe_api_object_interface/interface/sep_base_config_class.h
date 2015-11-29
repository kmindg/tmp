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
 *      This file defines the SEP Base Config class.
 *
 *  History:
 *      21-July-2011 : inital version
 *
 *********************************************************************/

#ifndef T_SEP_BASE_CONFIG_CLASS_H
#define T_SEP_BASE_CONFIG_CLASS_H

#ifndef T_SEP_CLASS_H
#include "sep_class.h"
#endif

/*********************************************************************
 *          Database class : bSEP base class
 *********************************************************************/

class sepBaseConfig : public bSEP
{
    protected:
        
        // Every object has an idnumber
        unsigned idnumber;
        unsigned sepBaseConfigCount;

        // interface object name
        char  * idname;
  
        // Database Interface function calls and usage 
        char * sepBaseConfigInterfaceFuncs; 
        char * usage_format;

        // Variables
        fbe_object_id_t object_id;
        fbe_bool_t is_enabled; 
        fbe_u32_t index;
        fbe_base_config_background_operation_t background_op;          
        fbe_api_base_config_upstream_object_list_t upstream_object_list_p;
        fbe_api_base_config_downstream_object_list_t downstream_object_list_p;
        fbe_metadata_info_t  metadata_info;
        void Sep_Base_Intializing_variable();
        char* edit_metadata_info( fbe_metadata_element_state_t);
        
    public:

        // Constructor/Destructor
        sepBaseConfig();
        ~sepBaseConfig(){}

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        void     dumpme(void);

        // Select method to call FBE API
        fbe_status_t select(int index, int argc, char *argv[]); 
       
        char * edit_downstream_object_list(
            fbe_api_base_config_downstream_object_list_t * downstream_object_list_p);
        fbe_status_t disable_BgOp (int argc, char ** argv);
        char * edit_upstream_object_list(
            fbe_api_base_config_upstream_object_list_t * upstream_object_list_p);
        fbe_status_t enable_BgOp (int argc, char ** argv);
        fbe_status_t get_BgOp_Status (int argc, char ** argv);
        fbe_status_t get_downstream_object_list(int argc , char ** argv);
        fbe_status_t get_upstream_object_list(int argc , char ** argv);
        fbe_status_t get_metadata_info(int argc , char ** argv);

        //clear metadata cache
        fbe_status_t clear_metadata_cache(int argc , char ** argv);

        fbe_status_t post_passive_obj_request(int argc , char ** argv);
};

#endif
