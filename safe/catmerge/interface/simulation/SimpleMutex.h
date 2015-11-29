/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               SimpleMutex.h
 ***************************************************************************
 *
 * DESCRIPTION:  Definition of a Simple Mutex
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    01/08/2010  Martin Buckley Initial Version
 *
 **************************************************************************/

# include "AbstractMutex.h"
# include "csx_ext.h"

#if defined(SHMEM_DLL_EXPORT)
#define SHMEMDLLPUBLIC CSX_MOD_EXPORT 
#else
#define SHMEMDLLPUBLIC CSX_MOD_IMPORT 
#endif


class SimpleMutex: public AbstractMutex
{
public:
    SHMEMDLLPUBLIC SimpleMutex();
    SHMEMDLLPUBLIC SimpleMutex(const char *name);
    SHMEMDLLPUBLIC ~SimpleMutex();

    SHMEMDLLPUBLIC BOOL lock(UINT_64 millisecondTimeout);
    SHMEMDLLPUBLIC void lock();
    SHMEMDLLPUBLIC void unlock();
private:
    const char *mName;    
    csx_uap_shared_mut_handle_t mHandle;
};
