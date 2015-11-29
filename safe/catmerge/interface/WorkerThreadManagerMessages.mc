;//++
;// Copyright (C) EMC Corporation, 2004 - 2005
;// All rights reserved.
;// Licensed material -- property of EMC Corporation
;//--
;
;//++
;// File:            WorkerThreadManagerMessages.h (MC)
;//
;// Description:     This file contains Worker thread manager bug check codes.
;//
;// The CLARiiON standard for how to classify errors according to their range
;// is as follows:
;//
;// "Information"       0x0000-0x3FFF
;// "Warning"           0x4000-0x7FFF
;// "Error"             0x8000-0xBFFF
;// "Critical"          0xC000-0xFFFF
;//
;//  Author(s):     Somnath Gulve
;//
;// History:
;//                  27-Dec-2004       SGulve		Created
;//--
;
;#ifndef __WORKER_THREAD_MANAGER_MESSAGES__
;#define __WORKER_THREAD_MANAGER_MESSAGES__
;

MessageIdTypedef = EMCPAL_STATUS


FacilityNames = ( WorkerThreadManager           = 0x159         : FACILITY_WORKER_THREAD_MANAGER )


SeverityNames = (
                  Success       = 0x0           : STATUS_SEVERITY_SUCCESS
                  Informational = 0x1           : STATUS_SEVERITY_INFORMATIONAL
                  Warning       = 0x2           : STATUS_SEVERITY_WARNING
                  Error         = 0x3           : STATUS_SEVERITY_ERROR
                )


;//------------------------------------------------------------------------------------------------------------
;// Introduced In: Release-22
;//
;// Severity: Error
;//
;// Usage: Returned to WTM clients
;//
MessageId    = 0x8000
Severity     = Error
Facility     = WorkerThreadManager
SymbolicName = WTM_STATUS_INVALID_THREAD_COUNT
Language     = English
.

;
;#define WTM_ADMIN_STATUS_INVALID_THREAD_COUNT         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(WTM_STATUS_INVALID_THREAD_COUNT)

;//------------------------------------------------------------------------------------------------------------
;// Introduced In: Release-22
;//
;// Severity: Error
;//
;// Usage: Returned to WTM clients
;//
MessageId    = +1
Severity     = Error
Facility     = WorkerThreadManager
SymbolicName = WTM_STATUS_INVALID_WORK_QUEUE_TYPE
Language     = English
.

;
;#define WTM_ADMIN_STATUS_INVALID_WORK_QUEUE_TYPE         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(WTM_STATUS_INVALID_WORK_QUEUE_TYPE)

;//------------------------------------------------------------------------------------------------------------
;// Introduced In: Release-22
;//
;// Severity: Error
;//
;// Usage: Returned to WTM clients
;//
MessageId    = +1
Severity     = Error
Facility     = WorkerThreadManager
SymbolicName = WTM_STATUS_WORK_ITEM_NOT_INITIALIZED
Language     = English
.

;
;#define WTM_ADMIN_STATUS_WORK_ITEM_NOT_INITIALIZED         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(WTM_STATUS_WORK_ITEM_NOT_INITIALIZED)

;//------------------------------------------------------------------------------------------------------------
;// Introduced In: Release-22
;//
;// Severity: Error
;//
;// Usage: Returned to WTM clients/BugCheck
;//
MessageId    = +1
Severity     = Error
Facility     = WorkerThreadManager
SymbolicName = WTM_STATUS_NO_THREADS_CREATED_ON_QUEUE_TYPE
Language     = English
The client queued a work item to the queue type for no threads have been created.
[1] WorkQueueType
[2] Client Name
[3] Work Item
[4] __LINE__
.

;
;#define WTM_ADMIN_STATUS_NO_THREADS_CREATED_ON_QUEUE_TYPE         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(WTM_STATUS_NO_THREADS_CREATED_ON_QUEUE_TYPE)

;//------------------------------------------------------------------------------------------------------------
;// Introduced In: Release-22
;//
;// Severity: Error
;//
;// Usage: Returned to WTM clients
;//
MessageId    = +1
Severity     = Error
Facility     = WorkerThreadManager
SymbolicName = WTM_STATUS_THREAD_CREATION_TIMEOUT
Language     = English
.

;
;#define WTM_ADMIN_STATUS_THREAD_CREATION_TIMEOUT         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(WTM_STATUS_THREAD_CREATION_TIMEOUT)

;//------------------------------------------------------------------------------------------------------------
;// Introduced In: Release-22
;//
;// Severity: Error
;//
;// Usage: Returned to WTM clients
;//
MessageId    = +1
Severity     = Error
Facility     = WorkerThreadManager
SymbolicName = WTM_STATUS_THREADS_OF_THIS_TYPE_ALREADY_EXIST
Language     = English
.

;
;#define WTM_ADMIN_STATUS_THREADS_OF_THIS_TYPE_ALREADY_EXIST         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(WTM_STATUS_THREADS_OF_THIS_TYPE_ALREADY_EXIST)


;//------------------------------------------------------------------------------------------------------------
;// Introduced In: Release-22
;//
;// Severity: Error
;//
;// Usage: Bugcheck
;//
MessageId    = +1
Severity     = Error
Facility     = WorkerThreadManager
SymbolicName = WTM_STATUS_GENERIC_BUGCHECK
Language     = English
The worker thread manager stumbled upon an unexpected situation.
[1] WorkQueueType
[2] Client Name
[3] Work Item
[4] __LINE__
.

;
;#define WTM_ADMIN_STATUS_GENERIC_BUGCHECK         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(WTM_STATUS_GENERIC_BUGCHECK)

;//------------------------------------------------------------------------------------------------------------
;// Introduced In: Release-22
;//
;// Severity: Error
;//
;// Usage: Returned to the WTM client
;//
MessageId    = +1
Severity     = Error
Facility     = WorkerThreadManager
SymbolicName = WTM_STATUS_INVALID_BUFFER_SIZE
Language     = English
The buffer size of the provided buffer is less than the suggested buffer size.
.

;
;#define WTM_ADMIN_STATUS_INVALID_BUFFER_SIZE         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(WTM_STATUS_INVALID_BUFFER_SIZE)


;//------------------------------------------------------------------------------------------------------------
MessageIdTypedef = EMCPAL_STATUS

;
;#endif // __WORKER_THREAD_MANAGER_MESSAGES__
;


