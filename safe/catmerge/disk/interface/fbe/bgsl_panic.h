
//  File: bgsl_panic.h

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


#ifndef BGSL_PANIC_H
#define BGSL_PANIC_H



//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:
//
#include "fbe/bgsl_types.h"



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

#define BGSL_PANIC_BASE          0x06000000  

#define BGSL_BGS_UTILS_OUT_OF_BAND_PACKET_PANIC         (BGSL_PANIC_BASE + 0x0001)
#define BGSL_BGS_OUT_OF_BAND_PANIC                      (BGSL_PANIC_BASE + 0x0002)
#define BGSL_BGS_FATAL_ERROR_PANIC                      (BGSL_PANIC_BASE + 0x0003)


//-----------------------------------------------------------------------------
//  ENUMERATIONS:


//-----------------------------------------------------------------------------
//  TYPEDEFS:



//-----------------------------------------------------------------------------
//  MACROS:


//  BGSL_PANIC()
//
//  The panic macro will print three lines of DebugPrint() info, then
//  calls bgsl_panic() in bgsl_panic_main.c which essentially
//  calls KeBugCheckEx().
//
//  Notes on using BGSL_PANIC() - 
//  * If your code can be engineered to recover from the problem, do that 
//    instead. This is for irrecoverable errors only. That is, only PANIC
//    if the error means there is no possible way to return failure, or
//    if there is risk of data loss.
//  * Thing long and hard before inserting a call to BGSL_PANIC(). 
//
//  Notes on WHO and WHY arguments :
//  * The WHO is comprised of two parts:
//     The upper 16 bits are a facility code, defined below.
//     The lower 16 bits are defined in a header file for that facility.
//  * The WHY is extra information (pointer, variable, etc.) that is
//    dependent on WHO calls panic.

#define BGSL_PANIC(who, why) {                                              \
    KvTrace("***BGSL_PANIC*** File %s\n", __FILE__);                        \
    KvTrace("***BGSL_PANIC*** Function %s\n", __FUNCTION__);                \
    KvTrace("***BGSL_PANIC*** Line #%d\n", __LINE__);                       \
    bgsl_panic(who, why);                                                   \
}


//-----------------------------------------------------------------------------
//  FUNCTION PROTOTYPES: 
//


//  Main function to perform a system panic in the FBE environment.
//  This is OS independent. Details within the function.
//  Note - Inform compiler that bgsl_panic() does not return; prefast fix
CSX_NORETURN VOID bgsl_panic(ULONG in_who, ULONG_PTR in_why);





#endif // BGSL_PANIC_H
