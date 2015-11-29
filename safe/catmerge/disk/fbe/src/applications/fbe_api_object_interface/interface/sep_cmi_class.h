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
 *      This file defines the SEP CMI INTERFACE class.
 *
 *  History:
 *      22-Jun_2011 : inital version
 *
 *********************************************************************/

#ifndef T_SEP_CMI_CLASS_H
#define T_SEP_CMI_CLASS_H

#ifndef T_SEP_CLASS_H
#include "sep_class.h"
#endif


/*********************************************************************
 *          SEP CMI class : bSEP base class
 *********************************************************************/

class sepCmi : public bSEP
{
    protected:

        // Every object has an idnumber
        unsigned idnumber;
        unsigned sepCmiCount;

        // interface object name
        char  * idname;

        // SEP CMI Interface function calls and usage
        char * sepCmiInterfaceFuncs;
        char * usage_format;

        // structure to capture cmi info from api
        fbe_cmi_service_get_info_t cmi_info;

        //private methods
        void edit_cmi_info(fbe_cmi_service_get_info_t * , char*,
                                         char*, char*);
        void sep_cmi_initializing_variable();

    public:
        // Constructor/Destructor
        sepCmi();
        ~sepCmi(){}

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        void     dumpme(void);

        // Select method to call FBE API
        fbe_status_t select(int index, int argc, char *argv[]);

        // FBE API cmi service status method
        fbe_status_t cmi_get_info(int argc, char ** argv);
};
#endif