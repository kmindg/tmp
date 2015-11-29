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
 *      This file defines the iArguments class.
 *
 *  History:
 *      12-May-2010 : inital version
 *
 *********************************************************************/

#ifndef T_IOCTL_ARG_CLASS_H
#define T_IOCTL_ARG_CLASS_H

// ioctl includes.
#ifndef IOCTL_PRIVATE_H
#include "ioctl_private.h" 
#endif

// Global variable declarations and assignments
#ifndef  FBEAPIX_H
#include "fbe_apix.h"
#endif

#ifndef T_IOCTL_DRAMCACHE_CLASS_H
#include "ioctl_dramCache_class.h"
#endif

/*********************************************************************
 * Arguments base class : fileUtilClass
 *********************************************************************/

class iArguments : public fileUtilClass
{
    protected:
        // Every object has an idnumber
        unsigned idnumber;
        unsigned argCount;

        // package name
        package_t pkg;

        // interface name
        package_subset_t pkgSubset;

        // index into arguments
        int index;

       // pointer to interface object
       Object * pBase;

    public:

        // Constructor
        iArguments(int &argc, char *argv[]);

        // Destructor
        ~iArguments(){}

        // Accessor methods: object 
        void dumpme(void);
        unsigned MyIdNumIs(void);
        Object * getBase(void);

        // Accessor methods : package 
        package_subset_t getPackageSubset(void);
        package_t getPackage(void);
        
        // // Accessor methods : command line 
        int getIndex(void);
        void List_Arguments(int argc, char *argv[]);
        
        // Validate arguments; select and instantiate interface.
        virtual fbe_status_t Do_Arguments (int &argc, char *argv[]);
};

#endif
