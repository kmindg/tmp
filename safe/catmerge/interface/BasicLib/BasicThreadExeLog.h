//***************************************************************************
// Copyright (C) EMC Corporation 2005-2011
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

///////////////////////////////////////////////////////////
//  BasicThread.h
//  Definition of the BasicThread class.
//
//  Created on:      26-Jan-2005 4:52:11 PM
///////////////////////////////////////////////////////////

#if !defined(_BasicThreadExeLog_h_)
#define _BasicThreadExeLog_h_

extern "C" {
#include "k10ntddk.h"
};

#include "EmcPAL.h"
#include "EmcPAL_DriverShell.h"

#define BASIC_THREAD_EXE_LOG_MAX_ENTRIES        256
#define BASIC_THREAD_EXE_LOG_MEM_TAG            'isaB'
#define BASIC_THREAD_EXE_LOG_INVALID_THREAD     ((ULONG64) 0xabcdef)

// Forward declaration
class BasicThread;

// An entry for logging BasicThread execution
typedef struct _BasicThreadExeLogEntry
{
    // Time when the thread start executing
    EMCPAL_TIME_USECS startTime;

    // Time when the thread returns
    EMCPAL_TIME_USECS endTime;

    // The thread that is being run
    PVOID pThread;

    // Pointer to vtable of object whose function is running
    PVOID vPtr;
}
BasicThreadExeLogEntry, *PBasicThreadExeLogEntry;

// An array of BasicThread execution entries
class BasicThreadExeLog
{
public:
    // An array of entries to log thread execution
    BasicThreadExeLogEntry* mpEntries;

    // Number of entries in this log
    USHORT mNumEntries;

    // The maxEntry (always 0) contains the entry with the largest run time
    USHORT mMaxEntry;

    // Index of the next entry to be logged
    USHORT mCurrentEntry;

    // Filler
    USHORT mUnused;

    // Constructor
    BasicThreadExeLog () {}

    // Destructor
    ~BasicThreadExeLog () {}

    // Log an entry for execution start to the current index, 
    // returns the index of the entry logged
    USHORT LogStartEntry (BasicThread* Thread, PVOID Ptr);

    // Log an entry on return from execution, to the index specified.  
    // If the threads don't match, it's a no-op, which can happen if the execution
    // returns on a different CPU.
    VOID LogEndEntry (BasicThread* Thread, USHORT Index);

    // Helper to get duration
    ULONG64 Duration (BasicThreadExeLogEntry* Ptr)
    {
        return ((ULONG64) (Ptr->endTime - Ptr->startTime));
    }
};

// A per-cpu history log of thread execution
class BasicThreadPerCpuExeLog
{
public:
    // An array of logs, one per CPU
    BasicThreadExeLog* mpExeLog;

    // A counter to init/destroy the log on first/last thread creation/destruction
    LONG mCount;

    // Number of CPUs in the system
    USHORT mNumCpus;

    // A boolean to indicate the log can be used
    BOOL mUsable;

    // Constructor
    BasicThreadPerCpuExeLog ();

    // Destructor
    ~BasicThreadPerCpuExeLog ();

    // Helper routine to init the array of per-cpu logs
    VOID Init ();

    // Helper routine to destroy the array of per-cpu logs
    VOID Destroy (BOOL SkipCheck = FALSE);

    // Returns a pointer to the exe log for the CPU on which the thread is running
    BasicThreadExeLog* GetExeLog ();

    // Logs an entry to the cpu on which thread execution starts
    USHORT LogExeStart (BasicThread* Thread, PVOID Ptr)
    {
        BasicThreadExeLog* pLog = GetExeLog ();
        USHORT index = 0;
        if (pLog)
        {
            index = pLog->LogStartEntry (Thread, Ptr);
        }
        return (index);
    }

    // Fills an entry when thread execution ends on a cpu
    VOID LogExeEnd (BasicThread* Thread, USHORT Index)
    {
        BasicThreadExeLog* pLog = Index ? GetExeLog () : NULL;
        // Index 0 is used for the max entry
        if (Index && pLog)
        {
            pLog->LogEndEntry (Thread, Index);
        }
        return;
    }
};

#endif // !defined(_BasicThreadExeLog_h_)
