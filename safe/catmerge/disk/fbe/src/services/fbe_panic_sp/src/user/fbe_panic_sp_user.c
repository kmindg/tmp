//  File: fbe_panic_sp_user.c   Component: FBE Panic sp in user interface

//-----------------------------------------------------------------------------
//  Copyright (C) EMC Corporation 2010
//  All rights reserved.
//  Licensed material -- property of EMC Corporation
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//
//  MODULE ABSTRACT:  FBE Panic SP Handling for User Interface
//
//    This is the FBE library which is responsible for executing a panic.in 
//     user mode. It provides a FBE-specific (and thus OS independent) routine
//    that brings down the system in a controlled manner when the caller had
//    made a  call to FBE_PANIC_SP().
//
//    Credit - Hardik Joshi
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:

//  Flare Header files
#include "fbe/fbe_winddk.h"  
#include "fbe/fbe_types.h"
//  FBE Header files
#include "fbe_panic_sp.h"

//-----------------------------------------------------------------------------
//  TYPEDEFS, ENUMS, DEFINES, CONSTANTS, MACROS:


//-----------------------------------------------------------------------------
//  EXTERNALS:

#ifdef _AMD64_
    extern void CallInt3(void);
#endif

//-----------------------------------------------------------------------------
//  INTERNALS:


//-----------------------------------------------------------------------------
//  FORWARD DECLARATIONS:


//-----------------------------------------------------------------------------
//  FUNCTIONS:


//-----------------------------------------------------------------------------
//                                FBE_panic_sp()
//-----------------------------------------------------------------------------
//
//  Description:
//    This function is to be used when the controller software
//     detects an almost fatal error
// 
//  Parameters:
//     in_who - who panicked.
//     in_why - the reason the afore mentioned party panicked.
//
//  Returns: We don't return.
//
//  Notes:
// 
//-----------------------------------------------------------------------------

VOID fbe_panic_sp (ULONG who, ULONG_PTR why)
{
	#ifndef ALAMOSA_WINDOWS_ENV
        fbe_debug_break();
    #else
    #ifdef _AMD64_
        emcutil_callint3();
    #else
        emcutil_callint3();
    #endif
    #endif /* ALAMOSA_WINDOWS_ENV - ARCH - panic behavior */
#ifndef ALAMOSA_WINDOWS_ENV
    CSX_PANIC();
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - panic behavior */
} //  End fbe_panic_sp()

//End of fbe_panic_sp_user.c
