/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  DESCRIPTION
 *     This file defines the functional interface to CMM.
 *   
 *
 *  HISTORY
 *     27-Sep-2009     Created.   -Austin Spang
 *
 *
 ***************************************************************************/
#ifndef BVDSIM_CMM_H
#define BVDSIM_CMM_H

#ifdef __cplusplus
extern "C" {
#endif
#include "cmm.h"

/* Define the offset for the "physical" addresses so that they
 * are no longer valid virtual addresses. */
#ifndef _AMD64_
#define BVDSIM_CMM_PHYSICAL_OFFSET (0x80000000)
#else
#define BVDSIM_CMM_PHYSICAL_OFFSET (0x8000000000000000)
#endif

/* Define structure used to simulate CMM memory management. */
typedef struct bvdsimAllocBlkTag bvdsimAllocBlk;

struct bvdsimAllocBlkTag {
    bvdsimAllocBlk *flink;
    bvdsimAllocBlk *blink;
    CMM_MEMORY_SIZE reqBlkSize;
    CMM_MEMORY_SIZE actBlkSize;
    char *vAddr;
    CMM_MEMORY_ADDRESS pAddr;
    CMM_SGL     rawAllocSgl;  // hacked on for cmmGetMemorySGL() use while not disrupting too much else
};

typedef struct {
    bvdsimAllocBlk *head;
    bvdsimAllocBlk *tail;
    ULONG numBlks;
} bvdsimAllocBlkList;

typedef struct bvdsimCmmStatsTag {
    ULONGLONG numReqDataBytesAlloc;
    ULONGLONG numActDataBytesAlloc;
    ULONGLONG numCtlBytesAlloc;
    ULONG numDataAllocs;
    ULONG numCtlAllocs;
    ULONG numDataFrees;
    ULONG numCtlFrees;
} bvdsimCmmStats;

struct cmmClientDescriptor {
    CMM_CLIENT_DESCRIPTOR_LINKS links;
    CMM_POOL_ID pool;
    CMM_MEMORY_RETURN_REQUEST_CALLBACK freeCallback;
    TEXT  clientName[65];
    bool  allocatedClient;
};

typedef struct cmmMemoryDescriptor
{
    CMM_MEMORY_DESCRIPTOR_LINKS     links;

    CMM_MEMORY_ADDRESS              virtualAddress;
    UINT_64                         allocationUnits;
    CMM_MEMORY_SIZE                 clientAllocationSize;
    struct cmmClientDescriptor     *clientPtr;

} CMM_MEMORY_DESCRIPTOR;

CMMAPI void BvdSim_reset_cmm(void);

CMMAPI EMCPAL_STATUS CmmInitialization(const PCHAR RegistryPath);

// Special Entrypoint that MccLxpmp calls to allow Memory to be correctly
// mapped for Dumping and Restoring memory;
CMMAPI EMCPAL_STATUS CmmInitializationForLxpmp(void* pm);

#ifdef __cplusplus
};   // end extern "C"
#endif
#endif  // BVDSIM_CMM_H
