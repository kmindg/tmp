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
 *      This file defines the System base class.
 *
 *  History:
 *      04-May-2011 : initial version
 *
 *********************************************************************/

#ifndef T_SYS_CLASS_H
#define T_SYS_CLASS_H

#ifndef T_OBJECT_H
#include "object.h"
#endif

/*********************************************************************
 * bSYS base class : Object Base class : fileUtilClass Base class
 *********************************************************************/

class bSYS : public Object
{
    protected:

        // Every object has an idnumber
        unsigned idnumber;
        unsigned sysCount;

        // object fbe api data
        fbe_package_id_t package_id;

        // status indicator and flags
        fbe_status_t status, passFail;

        // Index into argument list. Gives visability to all
        // class methods. The select method assigns the value of
        // the passed in argument [index] to [c].
        int c;
        void Sys_Intializing_variable();

    public:

        // Constructor
        bSYS();
        bSYS(FLAG);

        //Destructor
        ~bSYS(){}

        // Accessor methods (object  data)
        virtual unsigned MyIdNumIs(void);
        virtual char * MyIdNameIs(void) {return 0;}
        virtual void dumpme(void);

        // Select and call interface method
        virtual fbe_status_t select(int i, int c, char *a[]){
             return FBE_STATUS_INVALID;
        }

        // help
        virtual void HelpCmds(char *);

       // ------------------------------------------------------------
};

#endif
