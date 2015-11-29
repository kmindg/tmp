/*
/***************************************************************************
 * Copyright (C) EMC Corporation 2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               CacheSize_option.h
 ***************************************************************************
 *
 * DESCRIPTION: Declaration of the CacheSize_option class
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    02/06/2014  caridm Initial Version
 *
 **************************************************************************/

#ifndef CACHESIZE_OPTION_H_
#define CACHESIZE_OPTION_H_


#include "csx_ext.h"

#if defined(SHMEM_DLL_EXPORT)
#define SHMEMDLLPUBLIC CSX_MOD_EXPORT
#else
#define SHMEMDLLPUBLIC CSX_MOD_IMPORT
#endif

#include "simulation/Program_Option.h"

class Cache_Size_option: public Int_option {
public:
    SHMEMDLLPUBLIC Cache_Size_option();
    SHMEMDLLPUBLIC ~Cache_Size_option();
    SHMEMDLLPUBLIC void _set(int v);
    SHMEMDLLPUBLIC void set(char *v);
    SHMEMDLLPUBLIC static Cache_Size_option *getCliSingleton();
};

#endif /* CACHESIZE_OPTION_H_ */
