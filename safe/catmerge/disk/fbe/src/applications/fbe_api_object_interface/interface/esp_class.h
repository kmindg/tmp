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
 *      This file defines the Environment Service base class.
 *
 *  History:
 *      27-Jun-2011 : initial version
 *
 *********************************************************************/

#ifndef T_ESP_CLASS_H
#define T_ESP_CLASS_H

#ifndef T_OBJECT_H
#include "object.h"
#endif

#include "fbe/fbe_limits.h"

/*********************************************************************
 * bESP base class : Object Base class : fileUtilClass Base class
 *********************************************************************/

class bESP : public Object
{
    protected:

        // Every object has an idnumber
        unsigned idnumber;
        unsigned espCount;

        // object fbe api data
        fbe_object_id_t    lu_object_id;

         // status indicator and flags
        fbe_status_t status, passFail;

    public:

        // Constructor
        bESP();
        bESP(FLAG);

        //Destructor
        ~bESP(){}

        // Accessor methods (object  data)
        virtual unsigned MyIdNumIs(void);
        virtual char * MyIdNameIs(void) {return 0;}
        virtual void dumpme(void);

        // Accessor methods (FBE API data)

        // Select and call interface method
        virtual fbe_status_t select(int i, int c, char *a[]){
             return FBE_STATUS_INVALID;
        }

        // help
        virtual void HelpCmds(char *);

};

#endif
