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
 *      This file defines the SEP LOGICAL_ERROR_INJECTION INTERFACE class.
 *
 *  History:
 *      16-March-2012 : inital version
 *
 *********************************************************************/

#ifndef T_SEP_LOGICAL_ERROR_INJECTION_CLASS_H
#define T_SEP_LOGICAL_ERROR_INJECTION_CLASS_H


#ifndef T_SEP_CLASS_H
#include "sep_class.h"
#endif

/*********************************************************************
 *          SEP LogicalErrorInjection class : bSEP base class
 *********************************************************************/

class sepLogicalErrorInjection: public bSEP
{
    protected:
        // Every object has an idnumber
        unsigned idnumber;
        unsigned sepLogicalErrorInjectionCount;
        char params_temp[500];

        // interface object name
        char  * idname;

        // SEP LogicalErrorInjection Interface function calls and usage
        char * sepLogicalErrorInjectionInterfaceFuncs;
        char * usage_format;
        char sub_usage_format2[3000];
        char sub_usage_format1[3800];

        // SEP LogicalErrorInjection interface fbe api data structures 
        // and METHODS
        fbe_u32_t error_injection_type, start_index, num_to_clear;
        fbe_api_logical_error_injection_record_t record; 
        fbe_class_id_t  class_id;
        fbe_object_id_t  object_id;
        fbe_api_logical_error_injection_get_stats_t stats;
        fbe_api_logical_error_injection_get_object_stats_t obj_stats;
        fbe_api_logical_error_injection_modify_record_t  modify_record;
        fbe_api_logical_error_injection_delete_record_t del_record;
        fbe_api_logical_error_injection_get_records_t get_records;
        char lei_type[60], lei_mode[60];
        
        fbe_api_logical_error_injection_type_t get_err_injection_type( char** argv);
        char* get_true_or_false(fbe_bool_t);
        void edit_logical_error_injection_stats (
            fbe_api_logical_error_injection_get_stats_t);
        fbe_status_t get_optional_parameters(char **argv);
        fbe_api_logical_error_injection_mode_t get_error_mode (char** argv);
        char* edit_error_injection_mode(fbe_api_logical_error_injection_mode_t mode);
        char * edit_error_injection_type (fbe_api_logical_error_injection_type_t);

    public:

        // Constructor/Destructor
        sepLogicalErrorInjection();
        ~sepLogicalErrorInjection(){}

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        int      MyCountIs(void);
        void     dumpme(void);

        // Select method to call FBE API
        fbe_status_t select(int index, int argc, char *argv[]);

        //Create record
        fbe_status_t get_logical_error_injection_create_record(
            int argc , char ** argv);

        fbe_status_t get_logical_error_injection_create_object_record(
            int argc , char ** argv);

        // Enable error injection
        fbe_status_t get_logical_error_injection_enable(
            int argc , char ** argv);

        //Disable error injection
        fbe_status_t get_logical_error_injection_disable(
            int argc , char ** argv);

        // Enable error injection on class
        fbe_status_t get_logical_error_injection_enable_class(
            int argc , char ** argv);

        // Enable error injection on object
        fbe_status_t get_logical_error_injection_enable_object(
            int argc , char ** argv);

        // Disable error injection on object
        fbe_status_t get_logical_error_injection_disable_object(
            int argc , char ** argv);

        // Disable error injection on class
        fbe_status_t get_logical_error_injection_disable_class(
            int argc , char ** argv);

        // Get stats
        fbe_status_t get_logical_error_injection_stats(
            int argc , char ** argv);

        // Get object stats
        fbe_status_t get_logical_error_injection_object_stats(
            int argc , char ** argv);

        //Modify record
        fbe_status_t get_logical_error_injection_modify_record(
            int argc , char ** argv);

        //delete record
        fbe_status_t get_logical_error_injection_delete_record(
            int argc , char ** argv);

        //disable record
        fbe_status_t get_logical_error_injection_disable_records(
            int argc , char ** argv);

        //Get record
        fbe_status_t get_logical_error_injection_get_records(
            int argc , char ** argv);
        
};

#endif

