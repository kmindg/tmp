/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               SimpleSyncMutex.h
 ***************************************************************************
 *
 * DESCRIPTION:  Definition of a Simple Mutexi
 *               implemented using an EMCPAL_SYNC_EVENT
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    07/27/2011  Martin Buckley Initial Version
 *
 **************************************************************************/
# ifndef SIMPLESYNCMUTEX
# define SIMPLESYNCMUTEX

# include "AbstractMutex.h"
# include "csx_ext.h"

#if defined(SHMEM_DLL_EXPORT)
#define SHMEMDLLPUBLIC CSX_MOD_EXPORT 
#else
#define SHMEMDLLPUBLIC CSX_MOD_IMPORT 
#endif

class SimpleSyncMutexData;

class SimpleSyncMutex: public AbstractMutex {
public:
    SHMEMDLLPUBLIC SimpleSyncMutex();
    SHMEMDLLPUBLIC SimpleSyncMutex(const char *name);
    SHMEMDLLPUBLIC ~SimpleSyncMutex();

    SHMEMDLLPUBLIC BOOL lock(UINT_64 millisecondTimeout);
    SHMEMDLLPUBLIC void lock();
    SHMEMDLLPUBLIC void unlock();
private:
    SimpleSyncMutexData *mData;
};

#endif
