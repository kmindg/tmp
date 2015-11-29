/***************************************************************************
 * Copyright (C) EMC Corporation 2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               VirtualAffinity_option.h
 ***************************************************************************
 *
 * DESCRIPTION: Declaration of the Virtual_Affinity_option class
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    02/06/2014  caridm Initial Version
 *
 **************************************************************************/

#ifndef VIRTUALAFFINITY_OPTION_H_
#define VIRTUALAFFINITY_OPTION_H_


#include "csx_ext.h"

#if defined(SHMEM_DLL_EXPORT)
#define SHMEMDLLPUBLIC CSX_MOD_EXPORT
#else
#define SHMEMDLLPUBLIC CSX_MOD_IMPORT
#endif

#include "simulation/Program_Option.h"

class Virtual_Affinity_option: public HexInt64_option {
public:
    SHMEMDLLPUBLIC Virtual_Affinity_option(csx_processor_mask_t mask);
    SHMEMDLLPUBLIC void _set(csx_u64_t v);
    SHMEMDLLPUBLIC void set(char *v);
    SHMEMDLLPUBLIC static Virtual_Affinity_option *getCliSingleton();
};



#endif /* VIRTUALAFFINITY_OPTION_H_ */
