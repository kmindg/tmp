//**********************************************************************
//      Copyright (C) EMC Corporation, 2007
//      All rights reserved.
//      Licensed material - Property of EMC Corporation.
//**********************************************************************

//**********************************************************************
//.++
// FILE NAME:
//      MemBroker.h
//
// DESCRIPTION:
//
//**********************************************************************

#ifndef _MEM_BROKER_H_
#define _MEM_BROKER_H_

#include "MemMgr.h"
#include "EmcPAL.h"

typedef PVOID MEM_BROKER_CLIENT_HANDLE;


//  
//  Type:
//      MEM_BROKER_DISCRETIONARY_STATS
//
//  Description:
// 
//  This structure is used to describe the discretionary memory availability. 
//  A client will use this structure as a parameter to both the reclamation and 
//  notification callbacks
//
//  DiscretionarySpaceAvailable     
//      Amount of available discretionary space. Units: Bytes.
//
//  DiscretionarySpaceInUse         
//      Amount of available discretionary space currently 
//          in use by the client. Units: Bytes. 
//
//  SuggestedDiscretionatyAllocAmount     
//      Memory Broker's estimation of the ideal amount of discretionary 
//          memory the client should have. Units: Bytes

typedef struct _MEMMGR_DISCRETIONARY_STATS
{
      ULONG64    DiscretionarySpaceAvailable;
      ULONG64    DiscretionarySpaceInUse;
      ULONG64    SuggestedDiscretionatyAllocAmount;
}MEMMGR_DISCRETIONARY_STATS,*PMEMMGR_DISCRETIONARY_STATS;



typedef VOID (*MEM_BROKER_NOTIFICATION_CALLBACK)(
                   IN PVOID pNotificationCookie,
                   IN MEMMGR_DISCRETIONARY_STATS DiscretionaryStats );


typedef VOID (*MEM_BROKER_RECLAMATION_CALLBACK)(IN PVOID pReclamationCookie,
                                                IN ULONG64 CurrentDiscretionaryUsage,
                                                IN ULONG64 SuggestedDiscretionaryUsage);

typedef ULONG64 (*MEM_BROKER_GET_BALANCE_HINT_CALLBACK)(IN PVOID pReclamationCookie);

////////////////////////////////////////////////////////////////
//
// 
//          Exported Functions
// 
// 
////////////////////////////////////////////////////////////////

EMCPAL_STATUS 
MemMgrConfigureDiscretionaryUse (
        IN  MEM_BROKER_RECLAMATION_CALLBACK pReclamationCallback,
        IN  PVOID   pReclamationCookie,
        IN  MEM_BROKER_GET_BALANCE_HINT_CALLBACK pGetBalanceHintCallback,
        IN  ULONG32 Inertia,
        IN  PCHAR   pClientName,
        OUT PVOID   *Handle );

VOID
MemMgrSetDiscretionaryEmcpalClientPtr (
        IN  PEMCPAL_CLIENT     inEmcpalClientPtr);

EMCPAL_STATUS 
MemMgrDeconfigureDiscretionaryUse ( 
        IN VOID   *Handle );


EMCPAL_STATUS 
MemMgrRequestAvailabilityNotification (
        IN  VOID   *Handle,
        IN  ULONG64  DiscretionaryMemoryLevel,
        IN  MEM_BROKER_NOTIFICATION_CALLBACK pNotificationCallback,
        IN  PVOID pNotificationCookie );


EMCPAL_STATUS 
MemMgrSetInertia ( 
        IN  VOID     *Handle,
        IN  ULONG32   Inertia );

EMCPAL_STATUS
MemMgrGetInertia ( 
        IN  VOID        *Handle,
        OUT PULONG32   pInertia );


EMCPAL_STATUS
MemMgrGetBalanceHint ( 
        IN  VOID       *Handle,
        OUT PULONG64   pBalanceHint );


EMCPAL_STATUS
MemMgrGetDiscretionaryStats ( 
        IN  VOID        *Handle,
        OUT PMEMMGR_DISCRETIONARY_STATS pDiscretionaryStats );


EMCPAL_STATUS
MemMgrAllocateDiscretionaryControlPool ( 
        IN  VOID        *Handle,
        IN  ULONG64   NumberOfBytes,
	OUT PVOID	*pVirtualAddress );


EMCPAL_STATUS
MemMgrFreeDiscretionaryControlPool ( 
        IN  VOID        *Handle,
        IN  PVOID   pVirtualAddress );


MEMMGR_MEMORY_SIZE
MemMgrGetMaxDiscretionaryMemoryInPool( void );


EMCPAL_STATUS
MemMgrGetDiscretionaryAllocAmount(
        IN  VOID      *Handle,
        OUT PULONG64  pNumberOfBytesAllocated );

#endif // _MEM_BROKER_H_
