//  File: fbe_panic_sp_kernel.c   Component: FBE Panic sp in Kernel interface

//-----------------------------------------------------------------------------
//  Copyright (C) EMC Corporation 2010
//  All rights reserved.
//  Licensed material -- property of EMC Corporation
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//
//  MODULE ABSTRACT:  FBE Panic sp
//
//    This is the FBE library which is responsible for executing a machine
//    panic. It provides a FBE-specific (and thus OS independent) routine
//    that brings down the system in a controlled manner when the caller had
//    made a  call to FBE_PANIC_SP().
//
//    Credit - Hardik Joshi
//   
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

//-----------------------------------------------------------------------------
//  INTERNALS:


//-----------------------------------------------------------------------------
//  FORWARD DECLARATIONS:


//-----------------------------------------------------------------------------
//  FUNCTIONS:


//-----------------------------------------------------------------------------
//                                fbe_panic_sp()
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

        /* blue screen at this point. Suppress Prefast warning 311 which is a 
        * suggestion to not use KeBugCheckEx() which is proper here.
        */
        #pragma warning(disable: 4068)
        #pragma prefast(disable: 311)
        /*! @todo need to add a proper panic code here.
        */
        EmcpalBugCheck(0, 0, who, why, 0);
        #pragma prefast(default: 311)
        #pragma warning(default: 4068)

} //  End fbe_panic_sp()

//End of fbe_panic_sp_kernel.c