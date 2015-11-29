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
 *      This file defines the SEP JOB SERVICE INTERFACE class.
 *
 *  History:
 *      26-Aug-2011 : inital version
 *
 *********************************************************************/

#ifndef T_SEP_JOB_SERVICE_CLASS_H
#define T_SEP_JOB_SERVICE_CLASS_H

#ifndef T_SEP_CLASS_H
#include "sep_class.h"
#endif

/*********************************************************************
 *          SEP JOB SERVICE class : bSEP base class
 *********************************************************************/

class sepJobService : public bSEP
{
    protected:

        // Every object has an idnumber
        unsigned idnumber;
        unsigned sepJobServiceCount;
        
        // interface object name
        char  * idname;
    
        // SEP JOB SERVICE Interface function calls and usage
        char * sepJobServiceInterfaceFuncs;
        char * usage_format;
    
        // SEP JOB SERVICE interface fbe api data structures
        fbe_api_job_service_update_pvd_sniff_verify_status_t
            update_pvd_sniff_verify_state;
        fbe_object_id_t pvd_object_id;
        fbe_bool_t sniff_verify_state;
        fbe_u32_t state;

        // private methods

        void edit_pvd_sniff_verify(
            fbe_api_job_service_update_pvd_sniff_verify_status_t
                *update_pvd_sniff_verify_stat);

    public:

        // Constructor/Destructor
        sepJobService();
        ~sepJobService(){}

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        void     dumpme(void);
        
        // Select method to call FBE API
        fbe_status_t select(int index, int argc, char *argv[]);

        // FBE API update the provision drive sniff verify state
        fbe_status_t update_pvd_sniff_verify(int argc, char **argv);

        //FBE API enable the provision drive sniff verify 
        fbe_status_t enable_pvd_sniff_verify(int argc, char **argv);
        
        // ------------------------------------------------------------
};

#endif
