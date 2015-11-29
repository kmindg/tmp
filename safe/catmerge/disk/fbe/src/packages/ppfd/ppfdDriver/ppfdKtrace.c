/*********************************************************************************
 Copyright (C)  EMC Corporation 2008-2009
 All rights reserved.
 Licensed material - property of EMC Corporation.

 File Name:
        ppfdKtrace.c

 Contents:
    PPFD ktrace implementation patterned after some other EMC kernel drivers (specl, spm, etc.)
    Allows turning off/on ktrace messages "per zone" using registry flags. Also can 
    enable windbg versus ktrace messages.

 Internal:

 Exported:
    ppfdKvTrace 

 Revision History:
   August 29, 2008 - ECQ - Created Inital Version

***************************************************************************************/

#include "stddef.h" 
#include "EmcPAL_Misc.h"
#include "stdarg.h" 
#include "k10ntddk.h"
//#include "storport.h"
#include "scsiclass.h"
#include "ppfd_panic.h"
#include "ppfdMisc.h"
#include "ppfdKtraceProto.h"

ULONG ppfdGlobalTraceFlags; /* SAFERELOCATED */
ULONG ppfdGlobalWindbgTrace; /* SAFERELOCATED */

/***************************************************************************
 Function:
    SCSIPORT_API VOID ScsiDebugPrint (IN ULONG DebugPrintLevel,
                                  IN PCCHAR DebugMessage,
                                  ...)
 
 Routine Description:
    This routine prints out debug messages.  The DDK defines the DebugPrint()
    macro to be equivalent to this function.  All DebugPrints whose level is
    set to <= DEBUG_PRINT_LEVEL_DEFUALT will appear in the WinDbg output log.
    This function calls EmcpalDbgPrint() to actually display the message.
 
 Arguments:
 
    DebugPrintLevel -   The PPFD drive uses bitwise enable of ktrace/debug messages
                       If the DebugPrintLevel bits are set in the global flags, then the message is printed
    DebugMessage    -   Normal ASCII character string to display
   
    These arguments may be followed by up to MAX_ARGS additional arguments;
    they are handled by EmcpalDbgPrint() consistant with the normal definition 
    of printf(), sprintf(), etc.
 
  Return Value:
    None

 ***************************************************************************/

SCSIPORT_API VOID ScsiDebugPrint (IN ULONG DebugPrintLevel,
                                  IN PCCHAR DebugMessage,
                                  ...)
{
    va_list ap;
    ULONG_PTR arg[MAX_ARGS];
    ULONG index;

    // If windbg messages are enabled..and the trace bit is enabled..send it
    if ( (ppfdGlobalWindbgTrace) && (DebugPrintLevel & ppfdGlobalTraceFlags ) )
    {
        va_start (ap, DebugMessage);

        for (index = 0; index < MAX_ARGS; index ++)
        {
            arg[index] = va_arg (ap, ULONG_PTR);
        }

        va_end (ap);

        EmcpalDbgPrint (DebugMessage,
                  arg[0], arg[1], arg[2], arg[3], arg[4], arg[5]);
    }

}   /* end of ScsiDebugPrint() */

