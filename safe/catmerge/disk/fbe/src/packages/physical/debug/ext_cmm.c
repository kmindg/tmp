#include "dbgext.h"
#include "csx_ext.h"
#include <stdio.h>

#define STD_H
#include "cmm.h"
#include "cmm_pool.h"
#include "cmm_client.h"

// Added Macro's for faster symbol lookups
#define CMMDRV_ADDR(x)  csx_dbg_ext_lookup_symbol_address("cmm!" #x)
#define GETFIELDVALUE_CMM_RET(Addr, Type, Field, OutValue, RetValue) \
if(csx_dbg_ext_read_in_field(Addr, #Type, Field, sizeof(OutValue), (csx_pvoid_t)&(OutValue)))              \
{                                                                   \
    csx_dbg_ext_print("Err reading %s field:%s at %p\n", #Type, Field, Addr);  \
    return RetValue;                                                \
}
#define GETFIELDOFFSET_CMM_RET(Type, Field, pOffset, RetValue)       \
if(csx_dbg_ext_get_field_offset(#Type, Field, pOffset))                    \
{                                                                   \
    csx_dbg_ext_print("Error in reading %s offset of %s \n", Field, #Type);    \
    return RetValue;                                                \
}
#define GETFIELDVALUE_CMM(Addr, Type, Field, OutValue) csx_dbg_ext_read_in_field(Addr, #Type, Field, sizeof(OutValue), (csx_pvoid_t)&(OutValue))
#define GETFIELDOFFSET_CMM(Type, Field, pOffset) csx_dbg_ext_get_field_offset(#Type, Field, pOffset)
#define GETFIELDVALUE_CMM_RT(Addr, Type, Field, OutValue, RetValue) GETFIELDVALUE_CMM_RET(Addr, Type, Field, OutValue, RetValue)
#define GETFIELDOFFSET_CMM_RT(Type, Field, pOffset, RetValue) GETFIELDOFFSET_CMM_RET(Type, Field, pOffset, RetValue)

void printPoolSummary(ULONG64 poolID, ULONG64 pool);
void displayClients(ULONG64 clientQueue, UINT_64 allocUnitSize);
void printClientSummary(ULONG64 clientID, UINT_64 allocUnitSize);
void printClientMemoryDescSummary(ULONG64 memoryDesc, CHAR *location, UINT_64 allocUnitSize);
void printDeferredReqSummary(ULONG64 deferredReq);
void printPoolMemoryDescSummary(ULONG64 memoryDesc, UINT_64 allocUnitSize);
ULONG64 findAddrInPool(ULONG64 poolPtr, ULONG64 targetAddress);
BOOLEAN isAddrInPool(ULONG64 poolPtr, ULONG64 targetAddress);
int ChangeToVerboseDesc(const char * String, ULONG Count);

//
//Generate basic stats about the current memory usage
//
CSX_DBG_EXT_DEFINE_CMD( cmm, "!cmm\n   Displays summary information for all CMM pools and the clients registered to those pools\n" )
{
    ULONG64 currentPoolPtr;
    ULONG64 cmmPools,element;
    ULONG64 nextPtr,commonDescriptorPtr;
    ULONG   Offset;

    CMM_MEMORY_SIZE  allocationUnitSize;

    CMM(cmmPools);

    GETFIELDOFFSET_CMM(CMM_POOLS, "inuse", &Offset);
    cmmPools = cmmPools + Offset;
    //Get first pool pointer
    GETFIELDOFFSET_CMM(CMM_POOL_QUEUE, "links", &Offset);
    GETFIELDVALUE_CMM(cmmPools + Offset, CMM_POOL_DESCRIPTOR_LINKS, "element", element);
    currentPoolPtr = element;

    while (currentPoolPtr != 0)
    {

        //Read the current pool into local variables.
        GETFIELDVALUE_CMM(currentPoolPtr, CMM_POOL_DESCRIPTOR, "commonDescriptorPtr", commonDescriptorPtr);
        GETFIELDVALUE_CMM(commonDescriptorPtr, CMM_POOL_COMMON_DESCRIPTOR, "allocationUnitSize", allocationUnitSize);

        printPoolSummary(currentPoolPtr, commonDescriptorPtr);

        GETFIELDOFFSET_CMM(CMM_POOL_DESCRIPTOR, "inuseClientQueue", &Offset);

        //Now loop through all the clients registered to this pool
        csx_dbg_ext_print("\n");
        displayClients((currentPoolPtr + Offset), allocationUnitSize);
        csx_dbg_ext_print("\n-----------------------------------------------------------------------------\n\n");

        //Get next pool pointer
        GETFIELDVALUE_CMM(currentPoolPtr, CMM_POOL_DESCRIPTOR_LINKS, "nextPtr", nextPtr);
        GETFIELDVALUE_CMM(nextPtr, CMM_POOL_DESCRIPTOR_LINKS, "element", element);
        currentPoolPtr = element;

    }
}
#pragma data_seg ("EXT_HELP$4cmm")
static char cmmUsageMsg[] =
"!cmm\n"
"  Displays summary information for all CMM pools and the clients registered to those pools\n";
#pragma data_seg (".data")



//
// Print a summary line for the specified pool
//
void printPoolSummary(ULONG64 poolID, ULONG64 pool)
{
    UINT_64 totalSize, freeSize;

    CHAR buffer[64];
    CMM_MEMORY_POOL_TYPE        type;
    CMM_MEMORY_POOL_TERM        termOfUsage;
    BOOLEAN                     isUserPool;
    CMM_MEMORY_SIZE             allocationUnitSize;
    UINT_32                     totalAllocationUnits;
    UINT_32                     freeAllocationUnits;

    GETFIELDVALUE_CMM(poolID, CMM_POOL_DESCRIPTOR, "termOfUsage", termOfUsage);
    GETFIELDVALUE_CMM(poolID, CMM_POOL_DESCRIPTOR, "isUserPool", isUserPool);
    GETFIELDVALUE_CMM(pool, CMM_POOL_COMMON_DESCRIPTOR, "type", type);
    GETFIELDVALUE_CMM(pool, CMM_POOL_COMMON_DESCRIPTOR, "totalAllocationUnits", totalAllocationUnits);
    GETFIELDVALUE_CMM(pool, CMM_POOL_COMMON_DESCRIPTOR, "allocationUnitSize", allocationUnitSize);
    GETFIELDVALUE_CMM(pool, CMM_POOL_COMMON_DESCRIPTOR, "freeAllocationUnits", freeAllocationUnits);

    csx_dbg_ext_print("%18s%10s%10s%15s%15s%15s\n", "Pool ID", "Type", "Term", "User/Sys", "Total Size", "Free");
    csx_dbg_ext_print("%18s%10s%10s%15s%15s%15s\n", "-------", "----", "----", "--------", "----------", "----");

    sprintf(buffer, "0x%I64X", poolID);
    csx_dbg_ext_print("%18s", buffer);
    csx_dbg_ext_print("%10s", (CMM_TYPE_EQUALS_CONTROL_POOL_TYPE(type)) ? "CONTROL" : "DATA");
    csx_dbg_ext_print("%10s", (termOfUsage == CMM_TERM_SHORT) ? "SHORT" : "LONG");
    csx_dbg_ext_print("%15s", (isUserPool) ? "USER" : "SYSTEM");

    totalSize = totalAllocationUnits * allocationUnitSize;
    csx_dbg_ext_print("%15I64u", totalSize);

    freeSize = freeAllocationUnits * allocationUnitSize;
    csx_dbg_ext_print("%15I64u", freeSize);
    csx_dbg_ext_print("\n");
}

void displayClients(ULONG64 clientQueue, UINT_64 allocUnitSize)
{
    ULONG64 currentClientPtr;
    ULONG64 element,nextPtr;

    // Find first client
    GETFIELDVALUE_CMM(clientQueue, CMM_CLIENT_DESCRIPTOR_LINKS, "nextPtr", nextPtr);
    GETFIELDVALUE_CMM(nextPtr, CMM_CLIENT_DESCRIPTOR_LINKS, "element", element);
    currentClientPtr = element;

    csx_dbg_ext_print("%18s%20s%20s    %s\n", "Client ID", "Memory Alloc'd", "(Normal/Small)", "Client Name");
    csx_dbg_ext_print("%18s%20s%20s    %s\n", "---------", "--------------", "--------------", "-----------");

    while (currentClientPtr != 0x0)
    {
        // Print out the summary for this client
        printClientSummary(currentClientPtr, allocUnitSize);

        // Find next client
        GETFIELDVALUE_CMM(currentClientPtr, CMM_CLIENT_DESCRIPTOR_LINKS, "nextPtr", nextPtr);
        GETFIELDVALUE_CMM(nextPtr, CMM_CLIENT_DESCRIPTOR_LINKS, "element", element);
        currentClientPtr = element;
    }
}


void printClientSummary(ULONG64 clientID,UINT_64 allocUnitSize)
{
    CHAR buffer[64];
    UINT_32                     currentMemory;
    UINT_32                     currentSmallAlloc;
    TEXT                        clientName[65];

    GETFIELDVALUE_CMM(clientID, CMM_CLIENT_DESCRIPTOR, "currentMemory", currentMemory);
    GETFIELDVALUE_CMM(clientID, CMM_CLIENT_DESCRIPTOR, "currentSmallAlloc", currentSmallAlloc);
    GETFIELDVALUE_CMM(clientID, CMM_CLIENT_DESCRIPTOR, "clientName", clientName);

    sprintf(buffer, "0x%I64X", clientID);
    csx_dbg_ext_print("%18s", buffer);
    csx_dbg_ext_print("%20I64u", currentMemory*allocUnitSize+currentSmallAlloc);
    sprintf(buffer, "(%I64u/%lu)", currentMemory * allocUnitSize, currentSmallAlloc);
    csx_dbg_ext_print("%20s", buffer);
    csx_dbg_ext_print("    %s", clientName);
    csx_dbg_ext_print("\n");

}

//
// Generate detailed stats for a particular client
//
CSX_DBG_EXT_DEFINE_CMD( cmm_client, "!cmm\n    Displays summary information for all CMM pools and the clients registered to those pools\n" )
{
    ULONG verbose;
    ULONG64 clientID, minPoolCommon;
    ULONG64 backingPoolId, backingPoolCommon;
    ULONG64 inuseMemQ, element, clientPtr, nextPtr;
    ULONG64 deferredReq;

    UINT_64 allocUnitSize, currentMemory, maxMem, minTotalMem, minFreeMem;
    char clientName[65];
    UINT_32 bytes, currentSmallAlloc, totalAllocUnits, freeAllocUnits, offset;

    clientID = GetArgument64(args, 1);

    if (clientID == 0)
    {
        csx_dbg_ext_print("Please provide client ID\n");
        return;
    }
    verbose = ChangeToVerboseDesc(args, 2);

    GETFIELDVALUE_CMM(clientID, CMM_CLIENT_DESCRIPTOR, "backingPoolId", backingPoolId);
    GETFIELDVALUE_CMM(clientID, CMM_CLIENT_DESCRIPTOR, "currentMemory", currentMemory);
    GETFIELDVALUE_CMM(clientID, CMM_CLIENT_DESCRIPTOR, "currentSmallAlloc", currentSmallAlloc);
    GETFIELDVALUE_CMM(clientID, CMM_CLIENT_DESCRIPTOR, "maximumMemory", maxMem);
    GETFIELDVALUE_CMM(backingPoolId, CMM_POOL_DESCRIPTOR, "commonDescriptorPtr", backingPoolCommon);
    GETFIELDVALUE_CMM(backingPoolCommon, CMM_POOL_COMMON_DESCRIPTOR, "allocationUnitSize", allocUnitSize);
    
    currentMemory = currentMemory * allocUnitSize;
    
    GETFIELDOFFSET_CMM(CMM_CLIENT_DESCRIPTOR, "clientName", &offset);
    csx_dbg_ext_read_in_len(clientID+offset, &clientName, sizeof(clientName));
    csx_dbg_ext_print("%-25s%s\n", "Name: ", clientName);
    csx_dbg_ext_print("%-25s0x%P\n", "Backing Pool ID: ", backingPoolId);
    csx_dbg_ext_print("%-25s%I64u (%I64u MB)\n", "Memory Alloc'd: ", currentMemory, currentMemory / MEGABYTE);
    csx_dbg_ext_print("%-25s%lu\n", "Small Memory Alloc'd: ", currentSmallAlloc);
    if (maxMem == CMM_MAXIMUM_MEMORY)
    {
        csx_dbg_ext_print("%-25s%s\n","Maximum Memory: ", "CMM_MAXIMUM_MEMORY");
    }
    else
    {
        maxMem = maxMem * allocUnitSize;
        csx_dbg_ext_print("%-25s%I64u (%I64u MB)\n", "Maximum Memory: ", maxMem, maxMem / MEGABYTE);
    }

    GETFIELDOFFSET_CMM(CMM_CLIENT_DESCRIPTOR, "minimumPoolCommon", &offset);
    minPoolCommon = clientID + offset;
    GETFIELDVALUE_CMM(minPoolCommon, CMM_POOL_COMMON_DESCRIPTOR, "totalAllocationUnits", totalAllocUnits);
    GETFIELDVALUE_CMM(minPoolCommon, CMM_POOL_COMMON_DESCRIPTOR, "freeAllocationUnits", freeAllocUnits);

    minTotalMem = totalAllocUnits * allocUnitSize;
    minFreeMem = freeAllocUnits * allocUnitSize;

    csx_dbg_ext_print("%-25s%I64u (%I64u MB)\n", "Min Pool Size: ", minTotalMem, minTotalMem / MEGABYTE);
    csx_dbg_ext_print("%-25s%I64u (%I64u MB)\n", "Min Pool Free Memory: ", minFreeMem, minFreeMem / MEGABYTE);

    if(verbose)
    {
        csx_dbg_ext_print("\nAllocated Memory:\n");

        csx_dbg_ext_print("%38s%15s%23s%20s\n", "Address Range", "Size", "Virtual Addr", "Location");
        csx_dbg_ext_print("%38s%15s%23s%20s\n", "-------------", "----", "------------", "--------");

        //Dump all the memory descriptors in the min pool
        GETFIELDOFFSET_CMM(CMM_POOL_COMMON_DESCRIPTOR, "inuseMemoryQueue", &offset);
        inuseMemQ = minPoolCommon + offset;
        //Get first memory descriptor
        GETFIELDOFFSET_CMM(CMM_MEMORY_DESCRIPTOR_QUEUE, "links", &offset);
        GETFIELDVALUE_CMM(inuseMemQ+offset, CMM_MEMORY_DESCRIPTOR_LINKS, "nextPtr", nextPtr);
        GETFIELDVALUE_CMM(nextPtr, CMM_MEMORY_DESCRIPTOR_LINKS, "element", element);
        while(element != 0)
        {
            printClientMemoryDescSummary(element, "Min Pool", allocUnitSize);
            //Get next memory descriptor
            GETFIELDOFFSET_CMM(CMM_MEMORY_DESCRIPTOR, "links", &offset);
            GETFIELDVALUE_CMM(element+offset, CMM_MEMORY_DESCRIPTOR_LINKS, "nextPtr", nextPtr);
            GETFIELDVALUE_CMM(nextPtr, CMM_MEMORY_DESCRIPTOR_LINKS, "element", element);
        }

        //Dump all the memory descriptors that belong to this client in the backing pool
        GETFIELDOFFSET_CMM(CMM_POOL_COMMON_DESCRIPTOR, "inuseMemoryQueue", &offset);
        inuseMemQ = backingPoolCommon + offset;
        //Get first memory descriptor
        GETFIELDOFFSET_CMM(CMM_MEMORY_DESCRIPTOR_QUEUE, "links", &offset);
        GETFIELDVALUE_CMM(inuseMemQ + offset, CMM_MEMORY_DESCRIPTOR_LINKS, "nextPtr", nextPtr);
        GETFIELDVALUE_CMM(nextPtr, CMM_MEMORY_DESCRIPTOR_LINKS, "element", element);
        while(element != 0)
        {
            GETFIELDVALUE_CMM(element, CMM_MEMORY_DESCRIPTOR, "clientPtr", clientPtr);
            if (clientPtr == clientID)
                printClientMemoryDescSummary(element, "Backing Pool", allocUnitSize);

            //Get next memory descriptor
            GETFIELDOFFSET_CMM(CMM_MEMORY_DESCRIPTOR, "links", &offset);
            GETFIELDVALUE_CMM(element + offset, CMM_MEMORY_DESCRIPTOR_LINKS, "nextPtr", nextPtr);
            GETFIELDVALUE_CMM(nextPtr, CMM_MEMORY_DESCRIPTOR_LINKS, "element", element);
        }

        csx_dbg_ext_print("\nDeferred Requests:\n");

        csx_dbg_ext_print("%10s%15s%20s\n", "Callback", "Size", "Client Private");
        csx_dbg_ext_print("%10s%15s%20s\n", "--------", "----", "--------------");

        //Dump all the deferred requests on this client's queue
        GETFIELDOFFSET_CMM(CMM_CLIENT_DESCRIPTOR, "deferredRequests", &offset);
        deferredReq = clientID + offset;
        //Get first deferred request
        GETFIELDOFFSET_CMM(CMM_REQUEST_QUEUE, "links", &offset);
        GETFIELDVALUE_CMM(deferredReq + offset, CMM_REQUEST_LINKS, "nextPtr", nextPtr);
        GETFIELDVALUE_CMM(nextPtr, CMM_REQUEST_LINKS, "element", element);
        while(element != 0)
        {
            printDeferredReqSummary(element);
            //Get next deferred request
            GETFIELDOFFSET_CMM(CMM_REQUEST, "clientLinks", &offset);
            GETFIELDVALUE_CMM(element + offset, CMM_REQUEST_LINKS, "nextPtr", nextPtr);
            GETFIELDVALUE_CMM(nextPtr, CMM_REQUEST_LINKS, "element", element);
        }
    }
}
#pragma data_seg ("EXT_HELP$4cmm_client")
static char cmm_clientUsageMsg[] =
"!cmm_client <clientID> <verbose>\n"
"  Displays information for a specific CMM client\n";
#pragma data_seg (".data")


void printClientMemoryDescSummary(ULONG64 memoryDesc, CHAR *location, UINT_64 allocUnitSize)
{
    CHAR buffer[40];
    ULONG64 memoryAddr, virtualAddr;
    UINT_32 allocUnits;

    GETFIELDVALUE_CMM(memoryDesc, CMM_MEMORY_DESCRIPTOR, "memoryAddress", memoryAddr);
    GETFIELDVALUE_CMM(memoryDesc, CMM_MEMORY_DESCRIPTOR, "allocationUnits", allocUnits);
    GETFIELDVALUE_CMM(memoryDesc, CMM_MEMORY_DESCRIPTOR, "virtualAddress", virtualAddr);

    sprintf(buffer, "0x%I64X-0x%I64X", memoryAddr, memoryAddr + (allocUnits * allocUnitSize) - 1);
    csx_dbg_ext_print("%38s", buffer);
    csx_dbg_ext_print("%15lu", allocUnits * allocUnitSize);
    sprintf(buffer, "0x%I64X", virtualAddr);
    csx_dbg_ext_print("%23s", buffer);
    csx_dbg_ext_print("%20s", location);
    csx_dbg_ext_print("\n");
}

void printDeferredReqSummary(ULONG64 deferredReq)
{
    ULONG64 callbackSGL, clientPrivate;
    UINT_32 numberOfBytes;

    GETFIELDVALUE_CMM(deferredReq, CMM_REQUEST, "callbackSGL", callbackSGL);
    GETFIELDVALUE_CMM(deferredReq, CMM_REQUEST, "numberOfBytes", numberOfBytes);
    GETFIELDVALUE_CMM(deferredReq, CMM_REQUEST, "clientPrivate", clientPrivate);
    csx_dbg_ext_print("0x%08X", callbackSGL);
    csx_dbg_ext_print("%15lu", numberOfBytes);
    csx_dbg_ext_print("%10s0x%08X", " ", clientPrivate);
    csx_dbg_ext_print("\n");
}

//
// Generate detailed stats for a particular pool
//
CSX_DBG_EXT_DEFINE_CMD( cmm_pool, "!cmm_pool <poolID> <verbose>\n   Displays information for a specific CMM pool\n" )
{
    ULONG64 poolID, poolCommon, poolCommonMemory;
    ULONG64 inuseMemQ;
    ULONG verbose;
    UINT_64 allocUnitSize, totalAllocUnits;
    UINT_64 totalSize, freeSize, sizeSoFar;
    UINT_32 i;
    UINT_64 maxContigMemSize;
    double perFrag;
    UINT_32 termOfUsage, type, isUserPool, freeAllocUnits, allocUnits, numberOfEntries, offset;
    ULONG64 MemSegLength, MemSegAddress, maxContFreeMemPtr, freeMemQ;
    ULONG64  nextPtr, element;

    poolID = GetArgument64(args, 1);

    if (poolID == 0)
    {
        csx_dbg_ext_print("Please provide pool ID\n");
        return;
    }
    verbose = ChangeToVerboseDesc(args, 2);

    GETFIELDVALUE_CMM(poolID, CMM_POOL_DESCRIPTOR, "commonDescriptorPtr", poolCommon);
    GETFIELDVALUE_CMM(poolID, CMM_POOL_DESCRIPTOR, "termOfUsage", termOfUsage);
    GETFIELDVALUE_CMM(poolID, CMM_POOL_DESCRIPTOR, "isUserPool", isUserPool);
    GETFIELDVALUE_CMM(poolCommon, CMM_POOL_COMMON_DESCRIPTOR, "allocationUnitSize", allocUnitSize);
    GETFIELDVALUE_CMM(poolCommon, CMM_POOL_COMMON_DESCRIPTOR, "totalAllocationUnits", totalAllocUnits);
    GETFIELDVALUE_CMM(poolCommon, CMM_POOL_COMMON_DESCRIPTOR, "freeAllocationUnits", freeAllocUnits);
    GETFIELDVALUE_CMM(poolCommon, CMM_POOL_COMMON_DESCRIPTOR, "maxContiguousFreeMemoryPtr", maxContFreeMemPtr);
    GETFIELDVALUE_CMM(poolCommon, CMM_POOL_COMMON_DESCRIPTOR, "type", type);

    totalSize = totalAllocUnits * allocUnitSize;
    freeSize = freeAllocUnits * allocUnitSize;
    if (maxContFreeMemPtr != 0)
    {
        GETFIELDVALUE_CMM(maxContFreeMemPtr, CMM_MEMORY_DESCRIPTOR, "allocationUnits", allocUnits);
        maxContigMemSize = allocUnits * allocUnitSize;
    }
    else
    {
        maxContigMemSize = 0;
    }
    GETFIELDOFFSET_CMM(CMM_POOL_COMMON_DESCRIPTOR, "freeMemoryQueue", &offset);
    freeMemQ = poolCommon + offset;
    GETFIELDVALUE_CMM(freeMemQ, CMM_MEMORY_DESCRIPTOR_QUEUE, "numberOfEntries", numberOfEntries);
    if (numberOfEntries == 0)
    {
        perFrag = 0;
    }
    else
    {
        perFrag = (float)100 * ((float)1 - ((float)maxContigMemSize / (float)freeSize));
    }

    csx_dbg_ext_print("%-25s%s\n", "Term of use: ", (termOfUsage == CMM_TERM_LONG) ? "LONG TERM" : "SHORT TERM");
    csx_dbg_ext_print("%-25s%s\n", "Pool Type: ", (CMM_TYPE_EQUALS_CONTROL_POOL_TYPE(type)) ? "CONTROL" : "DATA");
    csx_dbg_ext_print("%-25s%s\n", "User/System: ", (isUserPool) ? "USER POOL" : "SYSTEM POOL");
    csx_dbg_ext_print("%-25s", "Address Range(s): ");
    sizeSoFar = 0;
    for (i=0; sizeSoFar < totalSize; i++)
    {
        poolCommonMemory = poolCommon + i * (ULONG)csx_dbg_ext_get_type_size("CMM_SGL");
        GETFIELDVALUE_CMM(poolCommonMemory, CMM_SGL, "MemSegLength", MemSegLength);
        sizeSoFar += MemSegLength;
        GETFIELDVALUE_CMM(poolCommonMemory, CMM_SGL, "MemSegAddress", MemSegAddress);
        csx_dbg_ext_print("0x%I64X-0x%I64X", MemSegAddress, MemSegAddress + MemSegLength - 1);
        csx_dbg_ext_print("%s", (sizeSoFar < totalSize) ? ", " : "\n");
    }
    csx_dbg_ext_print("%-25s%I64u (%I64u MB)\n", "Total Pool Memory: ", totalSize, totalSize / MEGABYTE);
    csx_dbg_ext_print("%-25s%I64u (%I64u MB)\n", "Free Pool Memory: ", freeSize, freeSize / MEGABYTE);
    csx_dbg_ext_print("%-25s%I64u\n", "Allocation Unit Size: ", allocUnitSize);
    csx_dbg_ext_print("%-25s%I64u (%I64u MB)\n", "Max Contiguous Free Mem: ", maxContigMemSize, maxContigMemSize / MEGABYTE);
    csx_dbg_ext_print("%-25s%lu.%lu%%\n", "Percent Fragmentation: ", (UINT_32)perFrag, (UINT_32)(perFrag*(float)100)%100);

    if(verbose)
    {
        csx_dbg_ext_print("\nMemory Allocation:\n");

        csx_dbg_ext_print("%38s%15s%23s%23s        %s\n", "Address Range", "Size", "Virtual Addr", "Client ID", "Client Name");
        csx_dbg_ext_print("%38s%15s%23s%23s        %s\n", "-------------", "----", "------------", "---------", "-----------");

        //Dump all the memory descriptors in this pool
        GETFIELDOFFSET_CMM(CMM_POOL_COMMON_DESCRIPTOR, "inuseMemoryQueue", &offset);
        inuseMemQ = poolCommon + offset;
        //Get first memory descriptor
        GETFIELDOFFSET_CMM(CMM_MEMORY_DESCRIPTOR_QUEUE, "links", &offset);
        GETFIELDVALUE_CMM(inuseMemQ + offset, CMM_MEMORY_DESCRIPTOR_LINKS, "nextPtr", nextPtr);
        GETFIELDVALUE_CMM(nextPtr, CMM_MEMORY_DESCRIPTOR_LINKS, "element", element);
        while(element != 0)
        {
            printPoolMemoryDescSummary(element, allocUnitSize);
            //Get next memory descriptor
            GETFIELDOFFSET_CMM(CMM_MEMORY_DESCRIPTOR, "links", &offset);
            GETFIELDVALUE_CMM(element + offset, CMM_MEMORY_DESCRIPTOR_LINKS, "nextPtr", nextPtr);
            GETFIELDVALUE_CMM(nextPtr, CMM_MEMORY_DESCRIPTOR_LINKS, "element", element);
        }
    }
}
#pragma data_seg ("EXT_HELP$4cmm_pool")
static char cmm_poolUsageMsg[] =
"!cmm_pool <poolID> <verbose>\n"
"  Displays information for a specific CMM pool\n";
#pragma data_seg (".data")


void printPoolMemoryDescSummary( ULONG64 memoryDesc, UINT_64 allocUnitSize)
{
    //CMM_CLIENT_DESCRIPTOR client;
    CHAR buffer[40];
    ULONG64 clientPtr, memoryAddr, virtualAddr;
    UINT_32 allocUnits, bytes, offset;
    char clientName[65];

    GETFIELDVALUE_CMM(memoryDesc, CMM_MEMORY_DESCRIPTOR, "memoryAddress", memoryAddr);
    GETFIELDVALUE_CMM(memoryDesc, CMM_MEMORY_DESCRIPTOR, "allocationUnits", allocUnits);
    GETFIELDVALUE_CMM(memoryDesc, CMM_MEMORY_DESCRIPTOR, "virtualAddress", virtualAddr);
    GETFIELDVALUE_CMM(memoryDesc, CMM_MEMORY_DESCRIPTOR, "clientPtr", clientPtr);

    sprintf(buffer, "0x%I64X-0x%I64X", memoryAddr, memoryAddr + (allocUnits * allocUnitSize) - 1);
    csx_dbg_ext_print("%38s", buffer);
    csx_dbg_ext_print("%15lu", allocUnits * allocUnitSize);
    sprintf(buffer, "0x%I64X", virtualAddr);
    csx_dbg_ext_print("%23s", buffer);
    sprintf(buffer, "0x%I64X", clientPtr);
    csx_dbg_ext_print("%23s", buffer);

    GETFIELDOFFSET_CMM(CMM_CLIENT_DESCRIPTOR, "clientName", &offset);
    csx_dbg_ext_read_in_len(clientPtr+offset, &clientName, sizeof(clientName));
    csx_dbg_ext_print("    %s", clientName);
    csx_dbg_ext_print("\n");
}

//
// For a give physical address, find the client and pool that allocated it.
//
CSX_DBG_EXT_DEFINE_CMD( cmm_find_addr, "!cmm_find_addr <physical address>\n   Finds the associated memory descriptor for a particular physical address for\n   memory that was allocated through CMM.  Displays information from that descriptor\n    such as the owning client, pool, and size\n" )
{
    BOOLEAN userPool,isUserPool;
    UINT_64 i,allocationUnitSize;
    ULONG64 targetAddress, minPoolPtr;
    ULONG64 cmmPools,element,currentClientPtr;
    ULONG64 nextPtr,commonDescriptorPtr,virtualAddress,clientPtr,poolPtr,currentPoolPtr,targetMemoryDescPtr = 0;
    ULONG   Offset,allocationUnits;

    if (sscanf(args, "%x", &targetAddress) != 1)
    {
        csx_dbg_ext_print("Invalid number of args provided to cmm_find_addr command\n");
        return;
    }

    targetAddress = (ULONG64)GetArgument64(args,1);

    csx_dbg_ext_print("\nSearching...");

    CMM(cmmPools);
    for (i=0; (i<2) && (targetMemoryDescPtr == 0); i++)
    {
        //Check user pools first, then system pools
        userPool = (i==0);
        GETFIELDOFFSET_CMM(CMM_POOLS, "inuse", &Offset);
        cmmPools = cmmPools + Offset;
        GETFIELDOFFSET_CMM(CMM_POOL_QUEUE, "links", &Offset);
        GETFIELDVALUE_CMM(cmmPools + Offset, CMM_POOL_DESCRIPTOR_LINKS, "element", element);
        currentPoolPtr = element;

        while ((currentPoolPtr != 0) && (targetMemoryDescPtr == 0))
        {
            GETFIELDVALUE_CMM(currentPoolPtr, CMM_POOL_DESCRIPTOR, "commonDescriptorPtr", commonDescriptorPtr);
            GETFIELDVALUE_CMM(currentPoolPtr, CMM_POOL_DESCRIPTOR, "isUserPool", isUserPool);

            if ((isUserPool && userPool) || (!isUserPool && !userPool))
            {
                if (isAddrInPool(currentPoolPtr, targetAddress))
                {
                    //Get first client pointer
                    GETFIELDOFFSET_CMM(CMM_POOL_DESCRIPTOR, "inuseClientQueue", &Offset);
                    GETFIELDVALUE_CMM(currentPoolPtr + Offset, CMM_CLIENT_DESCRIPTOR_LINKS, "nextPtr",nextPtr);
                    GETFIELDVALUE_CMM(nextPtr, CMM_CLIENT_DESCRIPTOR_LINKS, "element", element);
                    currentClientPtr = element;

                    while ((currentClientPtr != 0) && (targetMemoryDescPtr == 0))
                    {
                        //Find the client min pool
                        GETFIELDOFFSET_CMM(CMM_CLIENT_DESCRIPTOR, "minimumPool", &Offset);
                        minPoolPtr = currentClientPtr + Offset;

                        if (isAddrInPool(minPoolPtr, targetAddress))
                        {
                            //Try to find the memory descriptor in the client min pool
                            targetMemoryDescPtr = findAddrInPool(minPoolPtr, targetAddress);
                        }

                        //Get next client pointer
                        GETFIELDVALUE_CMM(currentClientPtr, CMM_CLIENT_DESCRIPTOR_LINKS, "nextPtr", nextPtr);
                        GETFIELDVALUE_CMM(nextPtr, CMM_CLIENT_DESCRIPTOR_LINKS, "element", element);
                        currentClientPtr = element;
                    }

                    //If we didnt find the address in any minimum pools, try the backing pool
                    if (targetMemoryDescPtr == 0)
                    {
                        targetMemoryDescPtr = findAddrInPool(currentPoolPtr, targetAddress);
                    }
                }
            }
            //Get next pool pointer
            GETFIELDOFFSET_CMM(CMM_POOL_QUEUE, "links", &Offset);
            GETFIELDVALUE_CMM(currentPoolPtr + Offset, CMM_POOL_DESCRIPTOR_LINKS, "nextPtr", nextPtr);
            GETFIELDVALUE_CMM(nextPtr, CMM_POOL_DESCRIPTOR_LINKS, "element", element);
            currentPoolPtr = element;
        }
    }

    if (targetMemoryDescPtr != 0)
    {
        GETFIELDVALUE_CMM(targetMemoryDescPtr, CMM_MEMORY_DESCRIPTOR, "virtualAddress", virtualAddress);
        GETFIELDVALUE_CMM(targetMemoryDescPtr, CMM_MEMORY_DESCRIPTOR, "allocationUnits", allocationUnits);
        GETFIELDVALUE_CMM(targetMemoryDescPtr, CMM_MEMORY_DESCRIPTOR, "poolPtr", poolPtr);
        GETFIELDVALUE_CMM(targetMemoryDescPtr, CMM_MEMORY_DESCRIPTOR, "clientPtr", clientPtr);
        GETFIELDVALUE_CMM(commonDescriptorPtr, CMM_POOL_COMMON_DESCRIPTOR, "allocationUnitSize", allocationUnitSize);

        csx_dbg_ext_print("\n");
        csx_dbg_ext_print("Memory Descriptor: 0x%I64X\n", targetMemoryDescPtr);
        csx_dbg_ext_print("Virtual Address:   0x%I64X\n", virtualAddress);
        csx_dbg_ext_print("Allocation Size:   %I64u\n",   allocationUnits * allocationUnitSize);
        csx_dbg_ext_print("Client ID:         0x%I64X\n", clientPtr);
        csx_dbg_ext_print("Pool ID:           0x%I64X\n", poolPtr);
    }
    else
    {
        csx_dbg_ext_print("\nAn in use memory descriptor could not be found in\n");
        csx_dbg_ext_print("any CMM pool for the specified physical address.\n");
    }

}
#pragma data_seg ("EXT_HELP$4cmm_find_addr")
static char cmm_find_addrUsageMsg[] =
"!cmm_find_addr <physical address>\n"
"  Finds the associated memory descriptor for a particular physical address for\n"
"  memory that was allocated through CMM.  Displays information from that descriptor\n"
"  such as the owning client, pool, and size\n";
#pragma data_seg (".data")





ULONG64 getpoolMemory (ULONG64 poolCommon, int count)
{
    ULONG    Offset, Size;
    ULONG64  pPoolCommon = 0;

    GETFIELDOFFSET_CMM_RT(CMM_POOL_COMMON_DESCRIPTOR, "poolMemory", &Offset, pPoolCommon);
    Size = (ULONG)csx_dbg_ext_get_type_size("CMM_SGL");
    pPoolCommon = poolCommon + Offset + (count * Size);

    return pPoolCommon;
}

//Given a target physical address and a pointer to a pool, see if this address exist
//in the range(s) of memory that describe this pool.
BOOLEAN isAddrInPool(ULONG64 poolPtr, ULONG64 targetAddress)
{
    ULONG64 commonDescriptorPtr;
    UINT_64 currFlatAddr;
    //UINT_64 totalPoolCap,totalAllocationUnits,allocationUnitSize;
    UINT_64 totalPoolCap, allocationUnitSize;
    UINT_32 totalAllocationUnits;
    UINT_64 MemSegLength;
    UINT_64 MemSegAddress;
    ULONG64 poolMemory;
    int i = 0;

    GETFIELDVALUE_CMM_RT(poolPtr, CMM_POOL_DESCRIPTOR, "commonDescriptorPtr", commonDescriptorPtr, FALSE);
    GETFIELDVALUE_CMM_RT(commonDescriptorPtr, CMM_POOL_COMMON_DESCRIPTOR, "allocationUnitSize", allocationUnitSize, FALSE);
    GETFIELDVALUE_CMM_RT(commonDescriptorPtr, CMM_POOL_COMMON_DESCRIPTOR, "totalAllocationUnits", totalAllocationUnits, FALSE);

    totalPoolCap = totalAllocationUnits * allocationUnitSize;
    currFlatAddr = 0;
    while((currFlatAddr < totalPoolCap))
    {
        poolMemory = getpoolMemory(commonDescriptorPtr, i);
        if (poolMemory == 0)
        {
            return FALSE;
        }
        GETFIELDVALUE_CMM_RT(poolMemory, CMM_SGL, "MemSegLength", MemSegLength, FALSE);
        GETFIELDVALUE_CMM_RT(poolMemory, CMM_SGL, "MemSegAddress", MemSegAddress, FALSE);

        if ((targetAddress >= MemSegAddress) &&
            (targetAddress < (MemSegAddress + MemSegLength)))
        {
            //The address was found in this SGL entry
            return TRUE;
        }
        currFlatAddr += MemSegLength;
        i++;
    }
    return FALSE;
}

//Given a target physical address and a pointer to a pool, try to find a memory descriptor
//that contains this address in this pool.  If found return a pointer to the memory descriptor
//otherwise return NULL.
ULONG64 findAddrInPool(ULONG64 poolPtr, ULONG64 targetAddress)
{
    ULONG64 commonDescriptorPtr,clientPtr;
    UINT_64 allocationUnitSize;
    const char clientName[65];
    ULONG allocationUnits;
    ULONG64 nextPtr,element,memoryAddress;
    ULONG Offset = 0;

    //Get inuseMemoryQueue from pool's commonDescriptorPtr
    GETFIELDVALUE_CMM_RT(poolPtr, CMM_POOL_DESCRIPTOR, "commonDescriptorPtr", commonDescriptorPtr, 0);
    GETFIELDVALUE_CMM_RT(commonDescriptorPtr, CMM_POOL_COMMON_DESCRIPTOR, "allocationUnitSize", allocationUnitSize, 0);
    GETFIELDOFFSET_CMM_RT(CMM_POOL_COMMON_DESCRIPTOR, "inuseMemoryQueue", &Offset, 0);
    commonDescriptorPtr = commonDescriptorPtr + Offset;
    //Get first memory descriptor
    GETFIELDOFFSET_CMM_RT(CMM_POOL_QUEUE, "links", &Offset, 0);
    GETFIELDVALUE_CMM_RT(commonDescriptorPtr + Offset, CMM_POOL_DESCRIPTOR_LINKS, "nextPtr", nextPtr, 0);
    GETFIELDVALUE_CMM_RT(nextPtr, CMM_POOL_DESCRIPTOR_LINKS, "element", element, 0);

    while(element != 0)
    {
        csx_dbg_ext_print(".");
        GETFIELDVALUE_CMM_RT(element, CMM_MEMORY_DESCRIPTOR, "memoryAddress", memoryAddress, 0);
        GETFIELDVALUE_CMM_RT(element, CMM_MEMORY_DESCRIPTOR, "allocationUnits", allocationUnits, 0);

        if ((targetAddress >= memoryAddress) && (targetAddress < (memoryAddress + ( allocationUnits * allocationUnitSize))))
        {
            //Found it.  Set the target memory desc ptr
            GETFIELDVALUE_CMM_RT(element, CMM_MEMORY_DESCRIPTOR, "clientPtr", clientPtr, 0);
            GETFIELDVALUE_CMM_RT(clientPtr, CMM_CLIENT_DESCRIPTOR, "clientName", clientName, 0);

            csx_dbg_ext_print("\nClient Name:%s",clientName);
            return element;
        }
        //Get next memory descriptor
        GETFIELDOFFSET_CMM_RT(CMM_POOL_QUEUE, "links", &Offset, 0);
        element = element + Offset;
        GETFIELDVALUE_CMM_RT(element, CMM_POOL_DESCRIPTOR_LINKS, "nextPtr", nextPtr, 0);
        GETFIELDVALUE_CMM_RT(nextPtr, CMM_POOL_DESCRIPTOR_LINKS, "element", element, 0);
    }
    csx_dbg_ext_print("\n");
    return 0;
}
// for the check on the argument 2 - short / verbose
// Eg: !cmm_client 0xfffffadf381e1238 v  ==> displays all the mem. allocations for this client
// !cmm_client 0xfffffadf381e1238   ==> displays only the summary for this client
int ChangeToVerboseDesc(const char * String, ULONG Count)
{
    char Buffer[1024];
    char * pBuffer, *p;

    strncpy(Buffer, String, (sizeof(Buffer)-1));
    pBuffer = Buffer;
    p = pBuffer;
    while(*p && *p != ' ' && *p != '\t') p++;
    if (*p) *p++ = '\0';
    while(*p && (*p == ' ' || *p == '\t')) p++;

    if (Count > 1)
    {
        if(*p == 'v' || *p =='V')
            return 1;
    else
        return 0 ;
    }
    return 0;
}
