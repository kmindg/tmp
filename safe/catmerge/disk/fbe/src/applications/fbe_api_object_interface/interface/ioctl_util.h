/*********************************************************************
 * Copyright (C) EMC Corporation, 1989 - 2011
 * All Rights Reserved.
 * Licensed Material-Property of EMC Corporation.
 * This software is made available solely pursuant to the terms
 * of a EMC license agreement which governs its use.
 *********************************************************************/

/**********************************************************************
 *                       ioctl_util_class.h
 **********************************************************************
 *
 *  Description:
 *    This header file contains class definitions for use with the
 *    ioctl_util.cpp module.
 *
 *  Table of Contents:
 *     iUtilClass
 *
 *  History:
 *      19-April-2011 : initial version
 *
 *********************************************************************/

#ifndef T_IOCTL_UTIL_CLASS_H
#define T_IOCTL_UTIL_CLASS_H

#define PRIVATE_H
#define T_FBE_APIX_UTIL_CLASS_H

#ifndef T_FILEUTILCLASS_H
#include "file_util_class.h"
#endif

/**********************************************************************
 *                          iUtilityClass                          
 **********************************************************************
 *
 *  Description:
 *      This class contains various utility methods used by 
 *      fbe_ioctl_object_interface library functions.
 *
 *  Notes:
 *      This is a general class for Ioctl utility methods.
 *
 *  History:
 *      19-April-2011 : initial version
 *
 *********************************************************************/

class iUtilityClass
{
    public:

    // Constructor/Destructor
    iUtilityClass(){};
    ~iUtilityClass(){};

    // Converts wwn into K10_LU_ID data type.
    fbe_status_t EditWWn(K10_LU_ID *Wwn, char **argv, char *temp);
};

#endif /* T_IOCTL_UTIL_CLASS_H */
