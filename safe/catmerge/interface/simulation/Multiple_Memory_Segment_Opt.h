/***************************************************************************
 * Copyright (C) EMC Corporation 2014-2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               Multiple_Memory_Segment_Opt.h
 ***************************************************************************
 *
 * DESCRIPTION: Declaration of the Multiple_Memory_Segment_option class
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    12/16/2014  caridm Initial Version
 *
 **************************************************************************/

#ifndef MULTIPLE_MEMORY_SEGMENT_OPT_H_
#define MULTIPLE_MEMORY_SEGMENT_OPT_H_

#include "csx_ext.h"

#if defined(SHMEM_DLL_EXPORT)
#define SHMEMDLLPUBLIC CSX_MOD_EXPORT
#else
#define SHMEMDLLPUBLIC CSX_MOD_IMPORT
#endif

#include "simulation/Program_Option.h"

class Multiple_Memory_Segment_option: public Int_option {
public:
    SHMEMDLLPUBLIC Multiple_Memory_Segment_option();
    SHMEMDLLPUBLIC ~Multiple_Memory_Segment_option();
    SHMEMDLLPUBLIC void _set(int v);
    SHMEMDLLPUBLIC void set(char *v);
    SHMEMDLLPUBLIC static Multiple_Memory_Segment_option *getCliSingleton();
};

#endif /* MULTIPLE_MEMORY_SEGMENT_OPT_H_ */
