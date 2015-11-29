/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               SimpleGlobalSyncEvent.h
 ***************************************************************************
 *
 * DESCRIPTION:  Definition of a Simple Mutex using a global Windows SyncEvent
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    09/19/2011  Martin Buckley Initial Version
 *
 **************************************************************************/

#ifndef __SIMPLEGLOBALSYNCEVENT__
#define __SIMPLEGLOBALSYNCEVENT__

# include "AbstractMutex.h"
# include "csx_ext.h"

#if defined(SHMEM_DLL_EXPORT)
#define SHMEMDLLPUBLIC CSX_MOD_EXPORT 
#else
#define SHMEMDLLPUBLIC CSX_MOD_IMPORT 
#endif

class SimpleGlobalSyncEvent_Data;

class SimpleGlobalSyncEvent: public AbstractMutex {
public:
    SHMEMDLLPUBLIC SimpleGlobalSyncEvent(const char *name);
    SHMEMDLLPUBLIC ~SimpleGlobalSyncEvent();

    SHMEMDLLPUBLIC BOOL lock(UINT_64 millisecondTimeout);
    SHMEMDLLPUBLIC void lock();
    SHMEMDLLPUBLIC void unlock();

private:

    SimpleGlobalSyncEvent_Data *mData;
};

#endif
