#ifndef __K10ASSERT_H_
#define __K10ASSERT_H_

#include "ktrace.h"

//***************************************************************************
// Copyright (C) EMC Corporation 2000
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/
#ifdef __cplusplus
/*
 * all of these functions need standard C linkage.  Otherwise, they can not be referenced from C.
 * Note, thee implementation of these functions is within a cpp file, which allows these functions to
 * access C++ instances, instance methods and class methods
 */
extern "C" {
#endif

/* special panic code for use by FF_ASSERT and FF_ASSERTMSG macros */
#define FF_ASSERT_PANIC 0x05900000

//**************************************************************************
//  $Id$: Assert macros used by multiple components.
//        Note that the NT "ASSERT" and "ASSERT_MSG" macros 
//        require a checked version of the NT kernel to 
//        trigger.
//**************************************************************************

#if !defined(I_AM_DEBUG_EXTENSION) && !defined(I_WOULD_RATHER_NOT_USE_KTRACE_THANK_YOU)

#if !defined(UMODE_ENV) && !defined(SIMMODE_ENV)
#include "k10ntddk.h"
/* need to inline this functionality because of pragmas used to disable warnings */
static __forceinline VOID FF_Bugcheck(ULONG LineNumber)
{
#ifdef ALAMOSA_WINDOWS_ENV
	KdBreakPoint(); 
	/* blue screen at this point. Suppress Prefast warning 311 which is a 
	 * suggestion to not use KeBugCheckEx() which is proper here.
	 */
#pragma warning(disable: 4068)
#pragma prefast(disable: 311)
    EmcpalBugCheck(0, 0, FF_ASSERT_PANIC, LineNumber, 0);
#pragma prefast(default: 311)
#pragma warning(default: 4068)

#else
    CSX_BREAK();
    CSX_PANIC_2("FF_Bugcheck at %d", LineNumber);
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - PCPC */
}

#else   // UMODE_ENV
#include "EmcPAL_Misc.h"
#define FF_Bugcheck(LineNumber)     EmcpalDebugBreakPoint()
#endif  // UMODE_ENV

// Avoid CopyPeer functionality inside CmiPci drivers
#if CMIPCI_DRIVERS
#define FFPrintCopyPeer CsxKvPrint
#define FF_RT_ASSERT  csx_rt_fast_assert
#define FF_RT_ASSERT_MSG  csx_rt_fast_assert_msg
#else
#define FFPrintCopyPeer CsxKvPrintCopyPeer
#define FF_RT_ASSERT  csx_rt_fast_assert_cp
#define FF_RT_ASSERT_MSG  csx_rt_fast_assert_msg_cp
#endif

// If we are compiling for CPP, USE_CPP_FF_ASSERT is defined and this is not a /clr compile
// then use FF_ASSERT variant macros that take into account the "this" ptr.
#if defined(__cplusplus) && defined(USE_CPP_FF_ASSERT) && !defined(__cplusplus_cli)

#define STRINGIZE(x) STRINGIZE_SIMPLE(x)
#define STRINGIZE_SIMPLE(x) #x

#define FF_ASSERT(exp)                                              \
    if (CSX_NOT_EXPECTED(!(exp))) {                                                   \
        csx_rt_asm_rsave_capture();                                 \
        FF_RT_ASSERT(#exp "$" __FILE__ ":" STRINGIZE(__LINE__), CSX_CAST_TO_PVOID(CSX_CAST_TO_STD(this)));                    \
    }                                                               

#define FF_ASSERT_NO_THIS(exp)                                      \
    if (CSX_NOT_EXPECTED(!(exp))) {                                                   \
        csx_rt_asm_rsave_capture();                                  \
        FF_RT_ASSERT(#exp "$" __FILE__ ":" STRINGIZE(__LINE__), NULL);             \
    }                                                               

#define FF_ASSERTMSG(msg, exp)                                              \
    if (CSX_NOT_EXPECTED(!(exp))) {                                                   \
        csx_rt_asm_rsave_capture();                                 \
        FF_RT_ASSERT_MSG(#exp "$" __FILE__ ":" STRINGIZE(__LINE__), CSX_CAST_TO_PVOID(CSX_CAST_TO_STD(this)), msg);                    \
    }                                                               

#define FF_ASSERTMSG_NO_THIS(msg, exp)                                              \
    if (CSX_NOT_EXPECTED(!(exp))) {                                                   \
        csx_rt_asm_rsave_capture();                                 \
        FF_RT_ASSERT_MSG(#exp "$" __FILE__ ":" STRINGIZE(__LINE__), NULL, msg);                    \
    }                                                               

#else // defined(__cplusplus) && defined(USE_CPP_FF_ASSERT) && !defined(__cplusplus_cli)
// "FastFail" assertions that should remain in free builds.  Dual SP
// systems want one SP to crash on software bug.
#define FF_ASSERT(exp)                                              \
    if (CSX_NOT_EXPECTED(!(exp))) {                                                   \
        FFPrintCopyPeer("Assertion Failed:  (%s) ", #exp);          \
        FFPrintCopyPeer("at %s:%d\n", CsxKvCatmergeFile(__FILE__), __LINE__);           \
        FF_Bugcheck(__LINE__);                                              \
    }

#define FF_ASSERT_NO_THIS(exp)                                             \
    if (CSX_NOT_EXPECTED(!(exp))) {                                                   \
        FFPrintCopyPeer("Assertion Failed:  (%s) ", #exp);          \
        FFPrintCopyPeer("at %s:%d\n", CsxKvCatmergeFile(__FILE__), __LINE__);           \
        FF_Bugcheck(__LINE__);                                              \
    }

#define FF_ASSERTMSG(msg, exp)                                      \
        if (CSX_NOT_EXPECTED(!(exp))) {                                                   \
        FFPrintCopyPeer("Assertion Failed:  (%s) ", #exp);          \
        FFPrintCopyPeer("at %s:%d ", CsxKvCatmergeFile(__FILE__), __LINE__);            \
        FFPrintCopyPeer("(%s).\n", msg);                            \
        FF_Bugcheck(__LINE__);                                              \
    }

#endif // defined(__cplusplus) && defined(USE_CPP_FF_ASSERT) && !defined(__cplusplus_cli)
    
#else   // defined(I_AM_DEBUG_EXTENSION)
// Debugger extension dlls don't want to be bothered with assertions.
#define FF_ASSERT(exp)
#define FF_ASSERT_NO_THIS(exp)
#define FF_ASSERTMSG(msg, exp)
#endif


// Checked build assertions.
#if DBG && (!defined(I_AM_DEBUG_EXTENSION)  && !defined(__DBG_EXT_CSX_H__) && !defined(I_WOULD_RATHER_NOT_USE_KTRACE_THANK_YOU))
#define DBG_ASSERT(exp)                                             \
    if (CSX_NOT_EXPECTED(!(exp))) {                                                   \
        FFPrintCopyPeer("Assertion Failed:  (%s) ", #exp);          \
        FFPrintCopyPeer("at %s:%d\n", CsxKvCatmergeFile(__FILE__), __LINE__);           \
        EmcpalDebugBreakPoint();                                            \
    }

#define DBG_ASSERTMSG(msg, exp)                                     \
    if (CSX_NOT_EXPECTED(!(exp))) {                                                   \
        FFPrintCopyPeer("Assertion Failed:  (%s) ", #exp);          \
        FFPrintCopyPeer("at %s:%d ", CsxKvCatmergeFile(__FILE__), __LINE__);            \
        FFPrintCopyPeer("(%s).\n", msg);                            \
        EmcpalDebugBreakPoint();                                            \
    }
#else
#define DBG_ASSERT(exp) 
#define DBG_ASSERTMSG(msg, exp) 
#endif

#ifdef __cplusplus
};
#endif

#endif  // __K10ASSERT_H_

