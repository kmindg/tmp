/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               Segment_option.h
 ***************************************************************************
 *
 * DESCRIPTION: Declaration of the Segment_option class
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    05/12/2011  Martin Buckley Initial Version
 *
 **************************************************************************/

#ifndef __SEGMENT_OPTION__
#define __SEGMENT_OPTION__

#include "csx_ext.h"
#include "spid_enum_types.h"

#if defined(SHMEM_DLL_EXPORT)
#define SHMEMDLLPUBLIC CSX_MOD_EXPORT 
#else
#define SHMEMDLLPUBLIC CSX_MOD_IMPORT 
#endif

#if defined(__cplusplus) || defined(c_plusplus)
#include "simulation/Program_Option.h"

class Master_IcId_option: public Int_option {
public:
    SHMEMDLLPUBLIC Master_IcId_option();
    SHMEMDLLPUBLIC void _set(int v);
    SHMEMDLLPUBLIC void set(char *v);
    SHMEMDLLPUBLIC static Master_IcId_option *getCliSingleton();
};
#endif // defined(__cplusplus) || defined(c_plusplus)


#endif // __SEGMENT_OPTION__

