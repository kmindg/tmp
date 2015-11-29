/***************************************************************************
 * Copyright (C) EMC Corporation 2011-2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                         PersistentCMMMemory.h
 ***************************************************************************
 *
 * DESCRIPTION:  Declaration of class PersistentCMMMemory
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    03/10/2011  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef __PERSISTENTDISTRIBUTEDCMMMEMORY__
#define __PERSISTENTDISTRIBUTEDCMMMEMORY__

#include "simulation/PersistentSharedMemory.h"

/*
 * The CMM needs to keep track of CMM segments using an array of CMM_SGL structures
 * This tracking structure is allocated at the beginning of persistent memory
 * See BvdSim_CMM.cpp usage of bvdSimPersistMemorySGL for further details
 */
const UINT_64 CMM_MEMORY_SEGMENT_SIZE =  (UINT_64) (64 * 1024 *1024);
const UINT_64 CMM_MEMORY_SEGMENT_SIZE_LARGE = (UINT_64) (96 * 1024 *1024);

/*
 *  GetPlatformDefaultPersistentCMMMemorySizeBytes returns the number of bytes
 *  that is reserved for Persistent CMM Memory on this System.
 */
extern UINT_64 GetPlatformDefaultPersistentCMMMemorySizeBytes(void);

class PersistentCMMMemory: public PersistentSharedMemory {
public:
    PersistentCMMMemory(PersistentMemoryType_e type, UINT_64 size);

    static void configure_CMM(PersistentMemoryType_e memoryConfig, UINT_64 memorySize);

    static PersistentCMMMemory *getInstance();

    static void *allocateMemory(UINT_64 size, PersistentMemoryId_e id = DEFAULTSP_PersistentMemory);
};

#endif // __PERSISTENTDISTRIBUTEDCMMMEMORY__

