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
 *      This file defines the SEP BVD INTERFACE class.
 *
 *  History:
 *      27-Jun-2011 : inital version
 *
 *********************************************************************/

#ifndef T_SEP_BVD_CLASS_H
#define T_SEP_BVD_CLASS_H

#ifndef T_SEP_CLASS_H
#include "sep_class.h"
#endif

/*********************************************************************
 *          SEP BVD class : bSEP base class
 *********************************************************************/

class sepBVD : public bSEP {

    protected:

        // Every object has an idnumber
        unsigned idnumber;
        unsigned sepBvdCount;

        // interface object name
        char  * idname;

        // SEP BVD Interface function calls and usage
        char * sepBvdInterfaceFuncs;
        char * usage_format;

        // SEP BVD interface fbe api data structures
        fbe_sep_shim_get_perf_stat_t     bvd_perf_stats;
        fbe_object_id_t                  bvd_id;
	 char   spid[2];
        //fbe_sp_performance_statistics_t  bvd_perf_stats;
        //fbe_volume_attributes_flags      downstream_attr;

        // private methods
        char* edit_perf_stats_sp_result(fbe_sep_shim_get_perf_stat_t*);
        void sep_bvd_initializing_variable();

       // void edit_io_stats_sp_result( fbe_bvd_io_stat_t* bvd_stat);

    public:

        // Constructor/Destructor
        sepBVD();
        ~sepBVD(){}

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        void     dumpme(void);

        // Select method to call FBE API
        fbe_status_t select(int index, int argc, char *argv[]);

        // FBE API BVD methods
        fbe_status_t enable_bvd_performance_statistics(int argc,  char ** argv);
        fbe_status_t disable_bvd_performance_statistics(int argc, char ** argv);
        fbe_status_t clear_bvd_performance_statistics(int argc, char ** argv);
        fbe_status_t get_bvd_performance_statistics(int argc, char ** argv);
        fbe_status_t get_bvd_object_id(int argc, char ** argv);

        //fbe_status_t get_bvd_io_statistics(int argc, char ** argv);
        //fbe_status_t get_bvd_downstream_attribute(int argc, char ** argv);

};

#endif

