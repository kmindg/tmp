//**********************************************************************
// FileHeader
//
//   File:          WorkerThreadManager.h
//
//   Description:   
//        This header file defines the K10WorkerThreadManagers' Library
//        API for the its clients to use.
//        This header should be included by the client in order to make
//        use of the functionality provided by the library.
//
//   Author(s):     Somnath Gulve
//
//   Revision Log:
//      1.00   28-Sept-04   SGulve   Intial Creation 
//
//   Copyright (C) EMC Corporation, 2004 - 2007
//      All rights reserved.
//      Licensed material - Property of EMC Corporation.
//
//**********************************************************************

#ifndef __WORKER_THREAD_MANAGER_H__
#define __WORKER_THREAD_MANAGER_H__


//
// The string size in bytes for a client name
//
#define WTM_CLIENT_NAME_STRING_SIZE                         64

//
// This macro is based on the NT provided macro for ExInitializeWorkItem.
//
#define WTMInitializeWorkItem(Item, Routine, Context)   \
    (Item)->WorkerRoutine = (EMCPAL_WORK_QUEUE_BODY_HANDLER)(Routine);                  \
    (Item)->Parameter = (Context);                      \
    (Item)->List.Flink = NULL;                          \
    (Item)->StartTime.QuadPart = 0;                     \
    (Item)->EndTime.QuadPart = 0;


//
//  Type:           WTM_WORK_QUEUE_TYPE
//
//  Description:    This enumeration describes the different types of
//                  available work queue.
//
//  Members:
//    K10CriticalWorkQueue
//
//    K10DelayedWorkQueue
//
//    K10HyperCriticalWorkQueue
//
//    K10LowRealTimeWorkQueue
//
//    K10PollWorkQueue
//
//    Reserved1 - Reserved10
//
//    CustomQueue1 - CustomQueue10
//
//    MaximumWorkQueue
//
typedef enum _WTM_WORK_QUEUE_TYPE 
{
    K10InvalidWorkQueue     =   -1,
    K10CriticalWorkQueue,
    K10DelayedWorkQueue,
    K10HyperCriticalWorkQueue,
    K10LowRealTimeWorkQueue,
    K10PollWorkQueue,
    Reserved1,                  // 5
    Reserved2,
    Reserved3,
    Reserved4,
    Reserved5,
    Reserved6,                  // 10
    Reserved7,
    Reserved8,
    Reserved9,
    Reserved10,
    CustomQueue1,               // 15
    CustomQueue2,
    CustomQueue3,
    CustomQueue4,
    CustomQueue5,
    CustomQueue6,               // 20
    CustomQueue7,
    CustomQueue8,
    CustomQueue9,
    CustomQueue10,
    K10MaximumWorkQueue
} WTM_WORK_QUEUE_TYPE, *PWTM_WORK_QUEUE_TYPE;


//
//  Type:           WTM_QUEUE_STAT_DATA
//
//  Description:    This structure maintains the statistical information 
//                  for each WTM Queue, and needs to be protected by the
//                  QueueLock.
//
//  Members:
//      TotalWorkItemsServiced
//          Total number of work items that have been processed via this Queue.
// 
//      TotalWaitTime
//          Total wait time of all work items processed via this Queue.
// 
//      MaxWaitTime
//          Maximum wait time of any work item processed via this Queue.
// 
//      MaxQueueLength
//          Maximum length of this Queue at any given point in time.
//
//
typedef struct _WTM_QUEUE_STAT_DATA
{
    ULONG                               TotalWorkItemsServiced;

    EMCPAL_LARGE_INTEGER                       TotalWaitTime;

    EMCPAL_LARGE_INTEGER                       MaxWaitTime;

    ULONG                               MaxQueueLength;

}WTM_QUEUE_STAT_DATA, *PWTM_QUEUE_STAT_DATA;


//
//  Type:           WTM_THREAD_STAT_DATA
//
//  Description:    This structure maintains the statistical information 
//                  for each thread in a work queue
//
//  Members:
//      WokeUpWithWorkToDo
//
//      WokeUpWithNoWorkToDo
//
//      TotalTasksExecuted
//
//      MaxTimeSpentDoingWork       // In 100 nano-seconds
//
//      MaxIdleTime                 // In 100 nano-seconds
//
typedef struct _WTM_THREAD_STAT_DATA
{
    ULONG                               WokeUpWithWorkToDo;

    ULONG                               WokeUpWithNoWorkToDo;

    ULONG                               TotalTasksExecuted;

    EMCPAL_LARGE_INTEGER                       TotalTimeSpentOnTasks;

    EMCPAL_LARGE_INTEGER                       MaxTimeSpentOnATask;

    EMCPAL_LARGE_INTEGER                       MaxIdleTime;

}WTM_THREAD_STAT_DATA, *PWTM_THREAD_STAT_DATA;




//
//  Type:           WTM_WORK_QUEUE_ITEM
//
//  Description:    This structure expands the NT provided structure 
//                  of WORK_QUEUE_ITEM
//
//  Members:
//      List
//
//      WorkerRoutine
//
//      Parameter
//
//      StartTime
//
//      EndTime
//
typedef struct _WTM_WORK_QUEUE_ITEM 
{
    EMCPAL_LIST_ENTRY                          List;

    EMCPAL_WORK_QUEUE_BODY_HANDLER      WorkerRoutine;

    PVOID                               Parameter;

    EMCPAL_LARGE_INTEGER                       StartTime;

    EMCPAL_LARGE_INTEGER                       EndTime;

} WTM_WORK_QUEUE_ITEM, *PWTM_WORK_QUEUE_ITEM;




//
// Function prototypes
//

EMCPAL_STATUS 
WTMCreateWorkerThreads (
         IN const char*             pClientName,
         IN WTM_WORK_QUEUE_TYPE     WorkQueueType, 
         IN ULONG                   ThreadCnt
         );

EMCPAL_STATUS 
WTMCreateWorkerThreadsWithAffinity (
         IN  const char*            pClientName,
         IN  WTM_WORK_QUEUE_TYPE    WorkQueueType, 
         IN  ULONG                  ThreadCnt,
         IN  ULONG_PTR              AffinityMaskValue
         );

EMCPAL_STATUS 
WTMDestroyWorkerThreads (
         IN  WTM_WORK_QUEUE_TYPE    WorkQueueType
         );

VOID
WTMQueueWorkItem (
        IN PWTM_WORK_QUEUE_ITEM     pWorkItem, 
        IN WTM_WORK_QUEUE_TYPE      WorkQueueType
        );

VOID
WTMQueueWorkItemToHead (
        IN PWTM_WORK_QUEUE_ITEM     pWorkItem, 
        IN WTM_WORK_QUEUE_TYPE      WorkQueueType
        );

EMCPAL_STATUS 
WTMQueryQueueDepth (
        IN   WTM_WORK_QUEUE_TYPE    WorkQueueType,
        OUT  PULONG                 pQueueDepth
        );

EMCPAL_STATUS 
WTMQueryQueueTime (
        IN   PWTM_WORK_QUEUE_ITEM    pWorkItem,
        OUT  EMCPAL_PLARGE_INTEGER          pQueueTime
        );

EMCPAL_STATUS 
WTMQueryThreadStatistics (
        IN      WTM_WORK_QUEUE_TYPE    WorkQueueType,
        IN      ULONG                  BufferSize,
        IN OUT  PVOID                  pBuffer
        );

VOID
displayWorkerThreadSummary(char * sDriverName);

VOID 
WTMDestroyAllWorkerThreads (VOID);

BOOLEAN 
WTMHaveWorkerThreadsBeenDestroyed ( 
     IN  WTM_WORK_QUEUE_TYPE    WorkQueueType
     );

BOOLEAN 
WTMHaveAllWorkerThreadsBeenDestroyed (VOID);

#endif // __WORKER_THREAD_MANAGER_H__
