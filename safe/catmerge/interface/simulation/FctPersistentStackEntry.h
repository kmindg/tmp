/***************************************************************************
 * Copyright (C) EMC Corporation 2011-2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               FctPeristentStackEntry.h
 ***************************************************************************
 *
 * DESCRIPTION: Definition of the FctPersistentStackEntry class
 *
 *    This class issues IOCTL IOCTL_BVD_CREATE_VOLUME to request a LUN
 *      creation
 *
 * FUNCTIONS: 
 *
 * NOTES:
 *
 * HISTORY:
 *    06/23/2011  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef __FCTPERSITENTSTACK__
#define __FCTPERSITENTSTACK__

#include "simulation/AutoInsertFactoryStackEntry.h"
# include "simulation/BvdSimSyncTransaction.h"
# include "simulation/simulation_dynamic_scope.h"

#ifdef ALAMOSA_WINDOWS_ENV
#define FCT_CLASS_EXPORT_MAYBE
#else
#define FCT_CLASS_EXPORT_MAYBE SYMBOL_SCOPE
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - tests need this exported - but only in Linux/GCC case */


class  FCT_CLASS_EXPORT_MAYBE FctPersistentStackEntry: public AutoInsertFactoryStackEntry {
public:
    SYMBOL_SCOPE FctPersistentStackEntry();

    SYMBOL_SCOPE virtual void createDevice(UINT_32 index,
                                char consumedDevicePath[K10_DEVICE_ID_LEN],
                                DiskIdentity * diskId);

    SYMBOL_SCOPE virtual void destroyDevice(UINT_32 index);

private:

    K10_WWID   mStartWwn;
    K10_WWID   mNextWwn;

};

extern "C" {
    // This function is exported for use by MCC to create a persistent stack Entry
    // for FCT.   Because FCT already links with MCC in simulation, we cannot have
    // MCC link with FCT.   Therefore if a MCC regression test needs to use
    // FCT we need to dynamically bind to it.   There for we load the FCT
    // DLL at runtime and then bind to it by getting the routine address
    // for this function and calling dynamically.
CSX_EXPORT FctPersistentStackEntry* FctPersistentStackEntryCreate();
};

#endif
