
//  File: fbe_panic_sp.h

//-----------------------------------------------------------------------------
//  Copyright (C) EMC Corporation 2010.
//  All rights reserved.
//  Licensed material -- property of EMC Corporation
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
//  MODULE ABSTRACT:  fbe panic sp
//
//    This header file contains the public interfaces which utilize the
//    FBE environment panic handling.
//
//-----------------------------------------------------------------------------


#ifndef FBE_PANIC_SP_H
#define FBE_PANIC_SP_H



//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:
//



//-----------------------------------------------------------------------------
//  NAMED CONSTANTS (#defines):

// FBE PANIC "who" values

//  Note that due to the many compilation dependencies, in particular with
//  generics.h, it is very difficult to include panic.h without coupling
//  heavily with Flare. The issue is the definition of IMPORT
//
//  Since the fbe panic facility is unique and separate from the Flare/Hemi
//  facility, it can define its own panic base. Note that to assist in triage,
//  an attempt was made to have a base that was still unique for fbe panic
//

#define FBE_PANIC_SP_BASE          0x06000000  

//-----------------------------------------------------------------------------
//  ENUMERATIONS:


//-----------------------------------------------------------------------------
//  TYPEDEFS:




//-----------------------------------------------------------------------------
//  MACROS:


//  FBE_PANIC_SP()
//
//  The panic macro will print three lines of DebugPrint() info, then
//  calls fbe_panic() in fbe_panic_main.c which essentially
//  calls KeBugCheckEx().
//
//  Notes on using FBE_PANIC_SP() - 
//  * If your code can be engineered to recover from the problem, do that 
//    instead. This is for irrecoverable errors only. That is, only PANIC
//    if the error means there is no possible way to return failure, or
//    if there is risk of data loss.
//  * Thing long and hard before inserting a call to FBE_PANIC_SP(). 
//
//  Notes on WHO and WHY arguments :
//  * The WHO is comprised of two parts:
//     The upper 16 bits are a facility code, defined below.
//     The lower 16 bits are defined in a header file for that facility.
//  * The WHY is extra information (pointer, variable, etc.) that is
//    dependent on WHO calls panic.

#define FBE_PANIC_SP(who, why) {                                              \
    fbe_KvTrace("***FBE_PANIC_SP*** File %s\n", __FILE__);                        \
    fbe_KvTrace("***FBE_PANIC_SP*** Function %s\n", __FUNCTION__);                \
    fbe_KvTrace("***FBE_PANIC_SP*** Line #%d\n", __LINE__);                       \
    fbe_panic_sp(who, why);                                                   \
}

//-----------------------------------------------------------------------------
//  FUNCTION PROTOTYPES: 
//


//  Main function to perform a system panic in the FBE environment.
//  This is OS independent. Details within the function.
//  Note - Inform compiler that fbe_panic_sp() does not return; prefast fix
VOID fbe_panic_sp (ULONG who, ULONG_PTR why);

#endif // FBE_PANIC_SP_H
