/***************************************************************************
 * Copyright (C) EMC Corporation 2014-2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               SplitAffinity_option.h
 ***************************************************************************
 *
 * DESCRIPTION: Declaration of the Split_Affinity_option class
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    12/15/2014  caridm Initial Version
 *
 **************************************************************************/

#ifndef SPLITAFFINITY_OPTION_H_
#define SPLITAFFINITY_OPTION_H_

// Minimum number of CPUs which a system must have before
// we allow the split affinity option to take affect.
//
// Note that Split Affinity overrides the opt_virtual_affinity_mask...
//
#define SPLIT_AFFINITY_MINIMUM_CPU_NUMBER_SUPPORTED 8

#include "csx_ext.h"

#if defined(SHMEM_DLL_EXPORT)
#define SHMEMDLLPUBLIC CSX_MOD_EXPORT
#else
#define SHMEMDLLPUBLIC CSX_MOD_IMPORT
#endif

#include "simulation/Program_Option.h"

class Split_Affinity_option: public Bool_option {
public:
    SHMEMDLLPUBLIC Split_Affinity_option();
    SHMEMDLLPUBLIC ~Split_Affinity_option();
    SHMEMDLLPUBLIC static Split_Affinity_option *getCliSingleton();
};



#endif /* SPLITAFFINITY_OPTION_H_ */
