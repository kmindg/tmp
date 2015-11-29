/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               BvdSimFileIo.h
 ***************************************************************************
 *
 * DESCRIPTION:  BvdSim File IO Test characterization class
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    09/11/2009  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef __BVDSIMIOTESTCONFIG
#define __BVDSIMIOTESTCONFIG

# include "generic_types.h"
# include "simulation/IOTaskId.h"

# include "simulation/DeviceConstructor.h"
# include "simulation/AbstractDisk.h"
# include "AbstractStreamConfig.h"

class BvdSimIoTestConfig {
public:
    BvdSimIoTestConfig(UINT_32 noThreads,
                        UINT_32 noDevices,
                        AbstractStreamConfig *genericStreamConfig, 
                        UINT_32 startingLunIndex = 0,
                        UINT_32 startingRGIndex  = 0,
                        UINT_32           numRGs = 1, 
                        AbstractDisk_type_t diskType = DiskTypeSSD)
   : mStartingLunIndex(startingLunIndex),
     mStartingRGIndex(startingRGIndex),
     mNoThreads(noThreads),
     mNoDevices(noDevices),
     mGenericStreamConfig(genericStreamConfig),
     mNumRaidGroups(numRGs),
     mDiskType(diskType) { }


    UINT_32                  getStartingLunIndex()  { return mStartingLunIndex; }
    UINT_32                   getStartingRGIndex()  { return mStartingRGIndex; }
    UINT_32                         getNoThreads()  { return mNoThreads; }
    UINT_32                         getNoDevices()  { return mNoDevices; }
    UINT_32                     getNumRaidGroups()  { return mNumRaidGroups; }
    AbstractDisk_type_t             getDriveType()  { return mDiskType; }
    AbstractStreamConfig *getGenericStreamConfig()  { return mGenericStreamConfig; }

private:
    UINT_32 mStartingLunIndex;
    UINT_32 mStartingRGIndex;
    UINT_32 mNoThreads;
    UINT_32 mNoDevices;
    UINT_32 mNumRaidGroups;
    AbstractDisk_type_t mDiskType;
    AbstractStreamConfig *mGenericStreamConfig;
};
#endif
