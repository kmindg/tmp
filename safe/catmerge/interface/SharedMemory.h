//**********************************************************************
//
//  COMPANY CONFIDENTIAL:
//     Copyright (C) EMC Corporation, 2001,2009
//     All rights reserved.
//     Licensed material - Property of EMC Corporation.
//**********************************************************************

//**********************************************************************
//.++
// FILE NAME:
//      SharedMemory.h
//
// DESCRIPTION:
//      The file contains definitions and structures used by the Shared
//      Memory Driver.
//
// REVISION HISTORY:
//      19-Mar-02  nfaridi      Added Clones/Remote shared globals
//      08-Mar-01  nfaridi      Updated SMD_MEMMGR_NUM_POOL_TYPES.
//      29-Jan-01  nfaridi      Created.
//.--                                                                    
//**********************************************************************

#ifndef _SHAREDMEMORY_H_
#define _SHAREDMEMORY_H_

#include "cmm_types.h"
#include "K10MiscUtil.h"
#include "EmcPAL.h"

//**********************************************************************
//.++
// CONSTANTS:
//      SMD_MEMMGR_NUM_POOL_TYPES
//
// DESCRIPTION:
//      Number of Pool Types used in MemMgr.
//      MaxPoolType is the number of NT Pool Types. MemMgr has 
//      All NT Pool Types plus a multiple of NT Pool Types.
//      This number must change in case new pool types are added to the 
//      MemMgr.
//
// REVISION HISTORY:
//      08-Mar-01  nfaridi      Added MaxPoolType to the MemMgr Pool 
//                              Types.
//      08-Feb-01  nfaridi      Created.
//.--
//**********************************************************************

#define SMD_MEMMGR_NUM_POOL_TYPES  ( EmcpalMaxPoolType + ( EmcpalMaxPoolType * 4 ) )

//**********************************************************************
//.++
// CONSTANTS:
//      SMD_ORG_DEV_NAME
//      SMD_CONTROL_DEV_NAME
//      SMD_DEV_NAME
//      SMD_DEVICE
//
// DESCRIPTION:
//      Device name and its constituents.
//
// REVISION HISTORY:
//      19-Feb-01  nfaridi      Created.
//.--
//**********************************************************************

#define SMD_ORG_DEV_NAME                           "\\Device\\CLARiiON"
#define SMD_CONTROL_DEV_NAME                                "\\Control"
#define SMD_DEV_NAME                                            "\\Smd"

#define SMD_ORG_CONTROL_DEV_NAME                ( SMD_ORG_DEV_NAME     \
                                                  SMD_CONTROL_DEV_NAME )

#define SMD_DEVICE                              (     SMD_ORG_DEV_NAME \
                                                  SMD_CONTROL_DEV_NAME \
                                                          SMD_DEV_NAME )

//**********************************************************************
//.++
// TYPE:
//      MEMMGR_SHARED_STATS
//
// DESCRIPTION:
//      MemMgr Shared stats per pool bases 
//
// MEMBERS:
//      OutstandingAllocated    Outstanding allocated memory
//      MaxAllocated            Maximum allocated at one time
//      TotalAllocated          Total allocated memory until now
//      OutstandingAllocations  Outstanding number of allocations
//      MaxAllocations          Maximum # of allocation at one time
//      TotalAllocations        Total # of allocations until now
//
// REVISION HISTORY:
//      31-Jan-01  nfaridi      Created.
//.--
//**********************************************************************

typedef struct _MEMMGR_SHARED_STATS
{
    CMM_MEMORY_SIZE     OutstandingAllocated;
    CMM_MEMORY_SIZE     MaxAllocated;
    CMM_MEMORY_SIZE     TotalAllocated;
    ULONG64             OutstandingAllocations;
    ULONG64             MaxAllocations;
    ULONG64             TotalAllocations;

} MEMMGR_SHARED_STATS;

//**********************************************************************
//.++
// TYPE:
//      SMD_MEMMGR_GLOBALS, PSMD_MEMMGR_GLOBALS
//
// DESCRIPTION:
//      Globals shared by Memory Manager. 
//
//
// MEMBERS:
//      Initialized        If MemMgr is initialized or not
//      MemLockAlloced     Lock for data modifications in MemMgr.
//                         Same lock is used for both the shared data and 
//                         for data local to an instance of MemMgr in a 
//                         driver.
//      MemoryStatistics   MemMgr Shared stats per pool bases
//      MaxMemoryAllowed   Total memory allowed for all the drivers
//                         using MemMgr
//      CurrentMemoryUsage Current memory in use by all the drivers
//                         Using MemMgr
//
// REVISION HISTORY:
//      30-Jan-01  nfaridi      Created.
//.--
//**********************************************************************

typedef struct _SMD_MEMMGR_GLOBALS 
{
    ULONG               Version;
    BOOLEAN             Initialized;
    EMCPAL_SPINLOCK     MemLockAlloced;
    MEMMGR_SHARED_STATS MemoryStatistics[ SMD_MEMMGR_NUM_POOL_TYPES ];
    CMM_MEMORY_SIZE	    MaxMemoryAllowed;
    CMM_MEMORY_SIZE	    CurrentMemoryUsage;
    CMM_POOL_ID         CMMFeaturePoolID;
    CMM_CLIENT_ID       MemMgrClientID;
    
} SMD_MEMMGR_GLOBALS, *PSMD_MEMMGR_GLOBALS;

//**********************************************************************
//.++
// TYPE:
//      SMD_REMOTECLONES_GLOBALS, PSMD_REMOTECLONES_GLOBALS
//
// DESCRIPTION:
//      Globals shared by Clones and RemoteView. 
//
//
// MEMBERS:
//      NumImages           Number of total images created by Remote and 
//                          Clones
//
// REVISION HISTORY:
//      19-Mar-02  nfaridi      Created.
//.--
//**********************************************************************

typedef struct _SMD_REMOTECLONES_GLOBALS 
{
    ULONG               NumImages;
    EMCPAL_SPINLOCK     LockAcquired;
    
} SMD_REMOTECLONES_GLOBALS, *PSMD_REMOTECLONES_GLOBALS;

//**********************************************************************
//.++
// TYPE:
//      SMD_MEMBROKER_GLOBALS, PSMD_MEMBROKER_GLOBALS
//
// DESCRIPTION:
//      Globals shared by the Memory Broker 
//
// MEMBERS:
//      ClientHandleArray           The storage for the client info pointer. This must be 8 bytes aligned
//                                  because of the use of InterlockedXXX function.
//      TotalNumOfClients           Number of registered clients
//      TotalDiscretionaryAlloc     Total amount of memory allocated
//      TotalBalanceHint            Total of all of the client's balance hints
//      StationaryReserve           Minimun amount of free stationary that we will maintain
//      StationaryLowWatermark      Allocation at which we will rebalance discretionary memory
//      StationaryHighWatermark     Allocation level that we will try to at establish after a rebalancing
//      LastRebalancingTime         Last time the discretionary memory was rebalanced
//
// REVISION HISTORY:
//      11-Apr-08  Cbailey      Created.
//.--
//**********************************************************************

#define NUMBER_OF_CLIENTS_SUPPORTED 4
typedef struct _SMD_MEMBROKER_GLOBALS 
{
    CSX_ALIGN_N(8) PVOID   ClientHandleArray[NUMBER_OF_CLIENTS_SUPPORTED];
    CSX_ALIGN_N(8) ULONG64 TotalDiscretionaryAlloc;
    CSX_ALIGN_N(8) ULONG64 TotalBalanceHint;
    ULONG64                   SmallestAllowabledContiguousPieceOfMemory;
    ULONG64                   FragmentationEliminationRecallPercentage;
    EMCPAL_LARGE_INTEGER      LastRebalancingTime;
    ULONG32                   NumFragmentationRecalls;
    ULONG32                   TotalNumOfClients;
    ULONG64                   StationaryReserve; 
    ULONG64                   StationaryLowWatermark;
    ULONG64                   StationaryHighWatermark;
    ULONG32                   FragmentationHighWatermark;
    BOOLEAN                   IsBrokerInitialized;
    csx_s32_t                 StatisticsUpdateLock;
    csx_s32_t                 OutStandingRequests;
    csx_s32_t                 FragmentationEliminationProcessInProgress;
    ULONG64                   ReservedForAlignment; 
    EMCPAL_SPINLOCK           SpinLock;

}SMD_MEMBROKER_GLOBALS, *PSMD_MEMBROKER_GLOBALS;

//**********************************************************************
//.++
// TYPE:
//      SMD_GLOBALS, PSMD_GLOBALS
//
// DESCRIPTION:
//      This is the major global structure containing shared memory for
//      participating drivers. Each entry is a structure for a driver.
//      All the structure included here must be defined inside this
//      header file. No other header should be included.
//
//
// MEMBERS:
//      MemMgrSharedGlobals     Globals shared by MemMgr.
//
// REVISION HISTORY:
//      18-Mar-02  nfaridi      Added Clones/Remote shared globals
//      29-Jan-01  nfaridi      Created.
//.--
//**********************************************************************

typedef struct _SMD_GLOBALS 
{
    SMD_MEMMGR_GLOBALS              MemMgr;
    SMD_REMOTECLONES_GLOBALS        RemoteClones;
    SMD_MEMMGR_GLOBALS              MemMgrNonPaged;
    SMD_MEMMGR_GLOBALS              MemMgrDataPool;
    SMD_MEMBROKER_GLOBALS           MemBroker;

} SMD_GLOBALS, *PSMD_GLOBALS;


//**********************************************************************
//.++
// CONSTANTS:
//      SMD_DEVICE_TYPE
//
// DESCRIPTION:
//      This constant defines a custom device type used by the Windows NT
//      CTL_CODE macro to define our custom IOCTLs. Any value in the 
//      range of 0x8000-0xFFFF is valid.
//
// REVISION HISTORY:
//      29-Jan-01  nfaridi      Created.
//.--
//**********************************************************************

#define SMD_DEVICE_TYPE                                           0xD000


//**********************************************************************
//.++
// CONSTANTS:
//      SMD_SHAREDMEMORY_GET_MEMORY_PTR_FUNCTION_CODE
//
// DESCRIPTION:
//      This constant defines the custom function code used by the
//      Windows NT CTL_CODE macro to define our custom IOCTLs. Any  
//      value in the range of 0x800-0xFFF is valid.
//
// REVISION HISTORY:
//      29-Jan-01  nfaridi      Created.
//.--
//**********************************************************************

#define SMD_SHAREDMEMORY_GET_MEMORY_PTR_FUNCTION_CODE              0xd01

//**********************************************************************
//.++
// CONSTANTS:
//      IOCTL_SHAREDMEMORY_GET_MEMORY_PTR
//
// DESCRIPTION:
//      This IOCTL returns the pointer to the global memory that is
//      being shared. Caller must know the contents of data that
//      are being accessed.
//
//      ***** IMPORTANT NOTE *****
//      There is no protection provided inside Shared Memory Driver. 
//      Caller must protect the data with a lock.
//      ***** IMPORTANT NOTE *****
//
// REVISION HISTORY:
//      29-Jan-01  nfaridi      Created.
//.--
//**********************************************************************

#define IOCTL_SHAREDMEMORY_GET_MEMORY_PTR EMCPAL_IOCTL_CTL_CODE(                     \
                            SMD_DEVICE_TYPE,                            \
                            SMD_SHAREDMEMORY_GET_MEMORY_PTR_FUNCTION_CODE,\
                            EMCPAL_IOCTL_METHOD_BUFFERED,                            \
                            EMCPAL_IOCTL_FILE_ANY_ACCESS )

#endif   // _SHAREDMEMORY_H_
