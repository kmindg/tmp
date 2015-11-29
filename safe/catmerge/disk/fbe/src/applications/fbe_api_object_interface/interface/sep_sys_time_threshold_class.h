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
*      This file defines the methods of the
       SEP SYSTEM TIME INTERFACE class.
*
*  Notes:
*      The SEP System Time Interface (SepSystemTime) is a derived class of
*      the base class (bSEP).
*
*  Created By : Eugene Shubert
*  History:
*      07-July-2012 : Initial version
*
*********************************************************************/

#ifndef T_SEP_SYS_TIME_THRESHOLD_CLASS_H
#define T_SEP_SYS_TIME_THRESHOLD_CLASS_H

#ifndef T_SEP_CLASS_H
#include "sep_class.h"
#endif

/*********************************************************************
 *          Database class : bSEP base class
 *********************************************************************/

class sepSysTimeThreshold : public bSEP
{
    protected:

        unsigned idnumber;
        unsigned sepSysTimeThresholdCount;

        // Database Interface function calls and usage
        char * sepSysTimeThresholdFuncs;
        char * usage_format;
        char * interfaceName;

        // SEP System Time Threshold interface fbe api data structures
        fbe_system_time_threshold_info_t time_threshold_info;

    public:

        // Constructor/Destructor
        sepSysTimeThreshold();
        ~sepSysTimeThreshold(){}

        // Select method to call FBE API
        fbe_status_t select(int index, int argc, char *argv[]);

        // set System threshold time
        fbe_status_t set_sys_time_threshold(int argc, char** argv);

        // Get system threshold time
        fbe_status_t get_sys_time_threshold(int argc, char** argv);

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        void     dumpme(void);
};

#endif
