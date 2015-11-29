/***************************************************************************
 * Copyright (C) EMC Corporation 2011-2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                         PersistentNvRamMemory.h
 ***************************************************************************
 *
 * DESCRIPTION:  Declaration of class PersistentNvRamMemory
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    03/10/2011  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef __PERSISTENTDISTRIBUTEDNVRAMMEMORY__
#define __PERSISTENTDISTRIBUTEDNVRAMMEMORY__

# include "generic_types.h"
# include "simulation/PersistentSharedMemory.h"
# include "MemoryPersistStruct.h"
# include "csx_ext.h"
# include "K10Plx.h"

#if defined(WDMSERVICES_EXPORT)
#define WDMSERVICEPUBLIC CSX_MOD_EXPORT
#else
#define WDMSERVICEPUBLIC CSX_MOD_IMPORT
#endif

class PersistentNvRamMemory: public PersistentSharedMemory {
public:
    WDMSERVICEPUBLIC PersistentNvRamMemory();

    static MEMPERSIST *getMemPersist(PersistentMemoryId_e id = DEFAULTSP_PersistentMemory) {
        return ((MEMPERSIST *) ((UCHAR*) getNvRamMemory(id) + K10_NVRAM_SIO_MEMORY_PERSISTENCE_HEADER_TRANSFORMERS));
    }

    void SetPersistentStatus(PersistentMemoryId_e id, UINT_32E status) {
        MEMPERSIST * persistSp = getMemPersist(id);
        persistSp->PersistStatus = status;
    }

    static WDMSERVICEPUBLIC UINT_8 *getNvRamMemory(PersistentMemoryId_e id = DEFAULTSP_PersistentMemory);
    WDMSERVICEPUBLIC static void initMemory(void *memory, UINT_64 size);
};

#endif // __PERSISTENTDISTRIBUTEDNVRAMMEMORY__
