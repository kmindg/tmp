/***************************************************************************
 * Copyright (C) EMC Corporation 2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               RBA_option.h
 ***************************************************************************
 *
 * DESCRIPTION: Declaration of the rba_option class
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    02/06/2014  caridm Initial Version
 *
 **************************************************************************/
#ifndef RBA_OPTION_H_
#define RBA_OPTION_H_

#include "csx_ext.h"

#if defined(SHMEM_DLL_EXPORT)
#define SHMEMDLLPUBLIC CSX_MOD_EXPORT
#else
#define SHMEMDLLPUBLIC CSX_MOD_IMPORT
#endif

#include "simulation/Program_Option.h"

class RBA_option: public Bool_option {
public:
    SHMEMDLLPUBLIC RBA_option();
    SHMEMDLLPUBLIC ~RBA_option();
    SHMEMDLLPUBLIC static RBA_option *getCliSingleton();
};

#endif /* RBA_OPTION_H_ */
