
//  File: fbe_panic.h

//-----------------------------------------------------------------------------
//  Copyright (C) EMC Corporation 2008
//  All rights reserved.
//  Licensed material -- property of EMC Corporation
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
//  MODULE ABSTRACT:  FBE Panic Handling
//
//    This header file contains the public interfaces which utilize the
//    FBE environment panic handling.
//
//-----------------------------------------------------------------------------


#ifndef FBE_PANIC_H
#define FBE_PANIC_H



//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:
//
#include "fbe/fbe_types.h"



//-----------------------------------------------------------------------------
//  NAMED CONSTANTS (#defines):

// FBE PANIC "who" values

//  Note that due to the many compilation dependencies, in particular with
//  generics.h, it is very difficult to include panic.h without coupling
//  heavily with Flare. The issue is the definition of IMPORT, and a need to 
//  have generics.h first in every BGS/FBE file.
//
//  Since the FBE panic facility is unique and separate from the Flare/Hemi
//  facility, it can define its own panic base. Note that to assist in triage,
//  an attempt was made to have a base that was still unique amongst the entire
//  Flare driver WHILE FBE is still part of the Flare Driver.
//
//  A note of this panic base has been made in panic.h. 
//  BGS_TODO - When FBE disconnects from Flare, remove the mention of this panic
//  base from Flare's panic.h

#define FBE_PANIC_BASE          0x06000000  

#define FBE_BGS_UTILS_OUT_OF_BAND_PACKET_PANIC         (FBE_PANIC_BASE + 0x0001)
#define FBE_BGS_OUT_OF_BAND_PANIC                      (FBE_PANIC_BASE + 0x0002)
#define FBE_BGS_FATAL_ERROR_PANIC                      (FBE_PANIC_BASE + 0x0003)


//-----------------------------------------------------------------------------
//  ENUMERATIONS:


//-----------------------------------------------------------------------------
//  TYPEDEFS:



//-----------------------------------------------------------------------------
//  MACROS:


//  FBE_PANIC()
//
//  The panic macro will print three lines of DebugPrint() info, then
//  calls fbe_panic() in fbe_panic_main.c which essentially
//  calls KeBugCheckEx().
//
//  Notes on using FBE_PANIC() - 
//  * If your code can be engineered to recover from the problem, do that 
//    instead. This is for irrecoverable errors only. That is, only PANIC
//    if the error means there is no possible way to return failure, or
//    if there is risk of data loss.
//  * Thing long and hard before inserting a call to FBE_PANIC(). 
//
//  Notes on WHO and WHY arguments :
//  * The WHO is comprised of two parts:
//     The upper 16 bits are a facility code, defined below.
//     The lower 16 bits are defined in a header file for that facility.
//  * The WHY is extra information (pointer, variable, etc.) that is
//    dependent on WHO calls panic.

#define FBE_PANIC(who, why) {                                              \
    KvTrace("***FBE_PANIC*** File %s\n", __FILE__);                        \
    KvTrace("***FBE_PANIC*** Function %s\n", __FUNCTION__);                \
    KvTrace("***FBE_PANIC*** Line #%d\n", __LINE__);                       \
    fbe_panic(who, why);                                                   \
}


//-----------------------------------------------------------------------------
//  FUNCTION PROTOTYPES: 
//


//  Main function to perform a system panic in the FBE environment.
//  This is OS independent. Details within the function.
//  Note - Inform compiler that fbe_panic() does not return; prefast fix
CSX_NORETURN VOID fbe_panic(ULONG in_who, ULONG_PTR in_why);





#endif // FBE_PANIC_H
