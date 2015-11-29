/***************************************************************************
 * Copyright (C) EMC Corporation 2011-2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                         PersistentMemory.h
 ***************************************************************************
 *
 * DESCRIPTION:  Declaration of class PersistentMemory
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    10/21/2010  Martin Buckley Initial Version
 *
 **************************************************************************/

#ifndef __PERSISTENTDISTRIBUTEDMEMORY__
#define __PERSISTENTDISTRIBUTEDMEMORY__

# include "generic_types.h"
# include "csx_ext.h"

#if defined(WDMSERVICES_EXPORT)
#define WDMSERVICEPUBLIC CSX_MOD_EXPORT
#else
#define WDMSERVICEPUBLIC CSX_MOD_IMPORT
#endif

enum PersistentMemoryId_e {
    SPA_PersistentMemory = 0,
    SPB_PersistentMemory = 1,
    DEFAULTSP_PersistentMemory = 3,
};

enum PersistentMemoryType_e {
    PersistentMemory_Split,
    PersistentMemory_Whole
};

class AbstractPersistentMemory {
public:
    virtual ~AbstractPersistentMemory() {};

    virtual void *getMemory(UINT_64 size, PersistentMemoryId_e id = DEFAULTSP_PersistentMemory) = 0;

    virtual UINT_64  getMemorySize(PersistentMemoryId_e id = DEFAULTSP_PersistentMemory) = 0;
    virtual UINT_8  *getMemoryStart(PersistentMemoryId_e id = DEFAULTSP_PersistentMemory) = 0;
    virtual UINT_8  *getMemoryEnd(PersistentMemoryId_e id = DEFAULTSP_PersistentMemory) = 0;

    virtual void reset(PersistentMemoryId_e id = DEFAULTSP_PersistentMemory, void initMemory(void *memory, UINT_64 size) = NULL) = 0;
};



class PersistentSharedMemory {
public:
    WDMSERVICEPUBLIC PersistentSharedMemory(const char *uniqueSegmentName,
                                                 UINT_64 size,
                                                 void initMemory(void *memory, UINT_64 size) = PersistentSharedMemory::initMemory,
                                                 PersistentMemoryType_e memoryType = PersistentMemory_Split);
    WDMSERVICEPUBLIC ~PersistentSharedMemory();

    WDMSERVICEPUBLIC void *getMemory(UINT_64 size, PersistentMemoryId_e id = DEFAULTSP_PersistentMemory);

    WDMSERVICEPUBLIC UINT_64 getMemorySize(PersistentMemoryId_e id = DEFAULTSP_PersistentMemory);
    WDMSERVICEPUBLIC UINT_8 *getMemoryStart(PersistentMemoryId_e id = DEFAULTSP_PersistentMemory);
    WDMSERVICEPUBLIC UINT_8 *getMemoryEnd(PersistentMemoryId_e id = DEFAULTSP_PersistentMemory);

    WDMSERVICEPUBLIC void reset(PersistentMemoryId_e id = DEFAULTSP_PersistentMemory, void initMemory(void *memory, UINT_64 size) = PersistentSharedMemory::initMemory);

    WDMSERVICEPUBLIC static void initMemory(void *memory, UINT_64 size);

protected:
    static WDMSERVICEPUBLIC PersistentMemoryId_e getDefaultID();

private:
    AbstractPersistentMemory *mImplementation;
};



#endif // __PERSISTENTDISTRIBUTEDMEMORY__


