/*********************************************************************
 * Copyright (C) EMC Corporation, 1989 - 2009
 * All Rights Reserved.
 * Licensed Material-Property of EMC Corporation.
 * This software is made available solely pursuant to the terms
 * of a EMC license agreement which governs its use.
 *********************************************************************/

/*********************************************************************
 *
 *  Description:
 *      This file defines the SEP PANIC SP INTERFACE class.
 *
 *  History:
 *      2011-03-28 : inital version
 *
 *********************************************************************/

#ifndef T_SEP_PANIC_SP_CLASS_H
#define T_SEP_PANIC_SP_CLASS_H

#ifndef T_SEP_CLASS_H
#include "sep_class.h"
#endif

/*********************************************************************
 *          SEP PANIC SP class : bSEP base class
 *********************************************************************/

class sepPanicSP : public bSEP
{
    protected:

        // Every object has an idnumber
        unsigned idnumber;
        unsigned sepPanicSPCount;

        // interface object name
        char  * idname;

        // SEP Panic SP Interface function calls and usage
        char * sepPanicSPInterfaceFuncs;
        char * usage_format;

    public:

        // Constructor/Destructor
        sepPanicSP();
        ~sepPanicSP(){}

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        void     dumpme(void);

        // Select method to call FBE API
        fbe_status_t select(int index, int argc, char *argv[]);

        // FBE API panic sp status method
        fbe_status_t panic_sp(int argc, char ** argv);

        // ------------------------------------------------------------
};

#endif
