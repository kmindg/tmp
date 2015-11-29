// KLogService.h
//**************************************************************************
// Copyright (C) EMC Corporation 1989-2000
// All rights reserved.
// Licensed material -- property of EMC Corporation
//*************************************************************************
//
//  NOTE: Drivers must link to K10EventLogLib.lib and KTrace.lib for proper
//        functionality.
//
//  Revision History
//  ----------------
//  ?? ??? ??   B. Fiske    Initial version.
//  27 Oct 99   D. Zeryck   Strip down for direct-to-log support only: NT_DIRECT
//                          and convert to WCHAR so we can avoid non-DISPATCH Rtl fns
//                          Add bug check wrappers at request of DH
//   8 Dec 99   D. Zeryck   Add Error Code to API
//   7 Jan 00   D. Zeryck   Update w/Navi i/f changes
//   1 Feb 00   D. Zeryck   Final clean-up
//   1 Mar 00   H. Weiner   Add offset to error codes to avoid collisions
//  03 Mar 00   D. Zeryck   New logging interface - again!
//  24 Mar 00   A. Taylor   Renamed the KLogBugCheck[Ex]() functions to internal
//                          routines that get called through a macro.  The macro
//                          provides additional debugging information.
//  01 Jun 00   B. Goudreau Almost completely rewrote.  Added KLogWriteEntry(), and
//                          redefined the older logging interfaces as macros that
//                          invoke the new, more general function.
//
//  02 Nov 01   M. O'Donnell Modified "bugcheck" macros to avoid Ktrace DbgPrint filtering.

#ifndef KLOGSERVICE_H
#define KLOGSERVICE_H

#include <stdarg.h>
#include "generic_types.h"
#include "ktrace.h"

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////////////////////////
//
//  Misc constants...
//
#define KLOG_ERROR_BASE 0x41050000      // To differentiate from other error lists
#define KLOGUSERCREATE  0x40            //  User space connecting to driver
#define KLOGUSERCLOSE   0x41            //  User space disconnecting from driver
#define KLOGUSERBUFFER  0x42            //  User space buffer provided

///////////////// PRESENTLY UNUSED, but may use if we go to a logging driver///////////////
//  Values for the Type field in the above functions
//
#define KLOGNORMAL      0               //  Normal priority
#define KLOGPRIORITY    1               //  High priority, may bump normal off queue
#define KLOGGUARANTEED  2               //  Will not return until completed
#define KLOGNOMIRROR    0x00010000      //  Event not mirrored to partner SP
#define KLOGNOTIFY      0x01000000      //  Notify Navisphere that an event of interest
                                        //  was generated
//
//  Return values for the logging functions described below
//
typedef enum KLog_rc
{
    LRCSuccess = 0,
    LRCMessageFileInvalid = KLOG_ERROR_BASE,    //  Initialize only
    LRCComponentNotInitialized,     //  Terminate only
    LRCMessageResourcesUnavailable, //  Logging and tracing function
    LRCLoggingServiceNotAvailable,  //  Logging service not started
    LRCMessageFormatError,          //  Message block inconsistent
    LRCEventDataOverflow,           //  No queue entry large enough for request
    LRCDriverCancelled,             //  Driver was terminated
    LRCBufferTooSmall,              //  Buffer smaller than a KLOG_MSG structure
    LRCUserRequestTimeout,          //  Time for user request exceeded
    LRCDuplicateRequest,            //  Multiple processes sent buffers, request ignored
    LRCSystemResources,             //  System resources were unavailable
    LRCStatisticsReturned           //  KLOG_STATS returned as data (driver to user space)
} KLOG_RC;

#ifdef KLOGSERVICE_GENERATE_ERROR_LABELS
static char * klogservice_error_text[] =
{
    "success",
    "message source could not be registered",
    "component name invalid or exceeds maximum length",
    "report event request failed",
    "logging service has not been started",
    "message request block format error",
    "total size of strings and data exceeds allowed limit",
    "the logging service driver was cancelled",
    "invalid buffer, smaller than KLOG_MSG structure",
    "request timed out, resend buffer",
    "a buffer is already active, duplicate buffer ignored",
    "system resources were unavailable",
    "statics buffer returned"
};
#endif  //  #ifdef KLOGSERVICE_GENERATE_ERROR_LABELS


///////////////////////////////////////////////////////////////////////////////////////////
//
//  Legacy Interfaces
//
//  The older logging interfaces are now just special cases of the new, more general one.
//  The KLogInitialze() and KLogTerminate() functions have become no-ops.
//


#define KLOG_MAX_COMPONENT_NAME_LEN 32
#define K10_EVENT_LOG_DEFAULT_COMPONENT_NAME    "K10_Subsystem"

#define KLogInitialize(wcsComponent)    EMCPAL_STATUS_SUCCESS
#define KLogTerminate()                 EMCPAL_STATUS_SUCCESS
//
///////////////////////////////////////////////////////////////////////////////////////////


//
// Call instead of KeBugCheck
//      (Duplicate Ktrace calls are to avoid ktrace DbgPrint filtering.)
//
#define KLogBugCheck(_BugCode_)                                                     \
    {                                                                               \
        KvOkToPanic(KT_PANIC_NOW);                                                  \
        KvTraceCopyPeer("KLogBugCheck(0x%08x)\n", (ULONG) (_BugCode_) );            \
        KvTraceCopyPeer("at line %d of %s\n", __LINE__ , __FILE__);                 \
        KvPrintStart("KLogBugCheck(0x%08x)\n", (ULONG) (_BugCode_) );               \
        KvPrintStart("at line %d of %s\n", __LINE__ , __FILE__);                    \
        KLogBugCheckInternal((ULONG) (_BugCode_));                                  \
    }

//
// Call instead of KeBugCheckEx
//      (Duplicate Ktrace calls are to avoid ktrace DbgPrint filtering.)
//
#define KLogBugCheckEx(_BugCode_, _a_, _b_, _c_, _d_)                               \
    {                                                                               \
        KvOkToPanic(KT_PANIC_NOW);                                                  \
        KvTraceCopyPeer("KLogBugCheckEx(0x%08x 0x%llx 0x%llx 0x%llx 0x%llx)\n",     \
            (ULONG) (_BugCode_), (csx_u64_t)(_a_), (csx_u64_t)(_b_), (csx_u64_t)(_c_), (csx_u64_t)(_d_));                       \
        KvTraceCopyPeer("at line %d of %s\n", __LINE__ , __FILE__);                 \
        KvPrintStart("KLogBugCheckEx(0x%08x 0x%llx 0x%llx 0x%llx 0x%llx)\n",        \
            (ULONG) (_BugCode_), (csx_u64_t)(_a_), (csx_u64_t)(_b_), (csx_u64_t)(_c_), (csx_u64_t)(_d_));                       \
        KvPrintStart("at line %d of %s\n", __LINE__ , __FILE__);                    \
        KLogBugCheckExInternal((ULONG) (_BugCode_), (_a_), (_b_), (_c_), (_d_));    \
    }


//
// Call when you know it is the hardware misbehaving
//
void KLogHardwareError(IN ULONG  BugCheckCode);

//
// These internal functions should not be called directly.  Use
// the wrapper macros defined above instead, as they will provide
// additional debugging information.
//
/* inform compiler that KLogBugCheckExInternal() does not return; prefast fix */
extern CSX_NORETURN void KLogBugCheckInternal(IN ULONG  BugCheckCode);
/* inform compiler that KLogBugCheckExInternal() does not return; prefast fix */
extern CSX_NORETURN 
void KLogBugCheckExInternal(IN ULONG  BugCheckCode,
        IN ULONG_PTR  BugCheckParameter1,
        IN ULONG_PTR  BugCheckParameter2,
        IN ULONG_PTR  BugCheckParameter3,
        IN ULONG_PTR  BugCheckParameter4
        );

//
////////////////////////////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
}
#endif

#endif  // KLOGSERVICE_H

