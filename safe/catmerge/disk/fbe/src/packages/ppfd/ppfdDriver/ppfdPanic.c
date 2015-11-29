/***************************************************************************
 Copyright (C)  EMC Corporation 2008-2009
 All rights reserved.
 Licensed material - property of EMC Corporation.


 File Name:
      ppfdPanic.c

 Contents:
    Support for panic (bugcheck).  PPFD may panic the system if an error occurs which would
    prevent the system from booting.

 Exported:
      ppfdPanic
     
 Revision History:
    
***************************************************************************/
#include "k10ntddk.h"
#include "ppfd_panic.h"


BOOLEAN panicDbgBreakpoint = TRUE;


VOID ppfdPanic(UINT_PTR who, UINT_PTR why)
{
    /* display panic message */
    KvTraceCopyPeer("PPFD: PANIC 0x%llx, 0x%llx (%llu)\n", (csx_u64_t)who, (csx_u64_t)why, (csx_u64_t)why);

    /* blue screen at this point. Suppress Prefast warning 311 which is a
     * suggestion not to use KeBugCheckEx() which is entirely proper here.
     */
    #pragma warning(disable: 4068)
    #pragma prefast(disable: 311)
    EmcpalBugCheck(0, 0, who, why, 0);
    #pragma prefast(default: 311)
    #pragma warning(default: 4068)

} /* end of function ppfdPanic() */



