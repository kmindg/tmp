/***************************************************************************
 Copyright (C)  EMC Corporation 2008-2009
 All rights reserved.
 Licensed material - property of EMC Corporation.


 File Name:
      ppfdRegistry.c

 Contents:
    Copied these registry functions from Specl driver to support reading "KtraceDebugLevel", 
    "WindbgDebugLevel", and "BreakOnEntry" settings from the registry.

 Internal:

 Exported:
    ppfdCheckBreakOnEntry()
    ppfdInitializeDebugLevel()
    
 Revision History:

    
***************************************************************************/

#include "k10ntddk.h"
//#include <devioctl.h>
//#include <ntstrsafe.h>
#include "generic_utils.h"      // for BOOLEAN_TO_TEXT
#include "ppfdMisc.h"
#include "ppfdKtraceProto.h"
#include "EmcPAL.h"
#include "EmcPAL_DriverShell.h"

/******************************************************************************
 ppfdReadRegistryValue()
 
 Description:
    This function reads a single DWORD from the registry.
    Note: It does NOT create the 'value' if the value does not exist.
 
 Arguments:
    pValueName      - name of the Value entry
    ValueType       - Type of the Value entry
    pValueData      - pointer to storage for the Value once read
    pDefaultValue   - pointer to the default value for the entry
    DefaultLength   - default length requires for the default value
 
 Return Values:
    STATUS_SUCCESS:                 The Entry table was processed successfully
    STATUS_INVALID_PARAMETER:       Invalid table entry specified
    STATUS_OBJECT_NAME_NOT_FOUND:   Path param does not match a valid key
    STATUS_BUFFER_TOO_SMALL:        Buffer passed in, is too small
 
 History:
 
 *****************************************************************************/
EMCPAL_STATUS ppfdReadRegistryValue(const CHAR*	pValueName,
                                     UINT_32*	pValue)
{

    PEMCPAL_DRIVER		driverHandle	= EmcpalGetCurrentDriver();

    return (EmcpalGetRegistryValueAsUint32(EmcpalDriverGetRegistryPath(driverHandle),
										   pValueName,
										   pValue));

} /* END ppfdReadRegistryValue() function */

/******************************************************************************
 Function:
    BOOL ppfdCheckBreakOnEntry(void)

 Description:
    This function reads the BreakOnEntry registry value from the registry
    to determine, whether to BreakPoint during DriverEntry. (Used as a 
    debug tactic).
 
 Arguments:
    None.
 
 Return Values:
    TRUE: If the registry value was found to be asserted
    FALSE: Otherwise
 
 History:

 *****************************************************************************/
BOOL ppfdCheckBreakOnEntry(void)
{
    BOOL        BreakOnEntry = FALSE;
    EMCPAL_STATUS    status;
    UINT_32     Level;
    CHAR        outBuffer[MAX_LOG_CHARACTER_BUFFER] = "";

    /* setup our default level, set to turn off the DbgBreakPoint() */
    Level = 0;

    /* read the key from the registry */
    status = ppfdReadRegistryValue(PPFD_REGISTRY_NAME_BREAK_ENTRY,
                                   &Level);
    if (EMCPAL_IS_SUCCESS(status))
    {
        /* Check the Value to determine if we should break
         */
        if (Level == 1)
        {
            BreakOnEntry = TRUE;
        }
        
        csx_p_snprintf(outBuffer,
                sizeof(outBuffer),
                "PPFD: %s() - BreakOnEntry Value: %s\n",
                __FUNCTION__,
                BOOLEAN_TO_TEXT(BreakOnEntry));

        PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_START, outBuffer);
    }
    else
    {
        /* Key not present - ignore the error status.
         */
        BreakOnEntry = FALSE;
    }

    return (BreakOnEntry);
} /* END ppfdCheckBreakOnEntry() function */


/******************************************************************************
 Function:
    NTSTATUS ppfdCheckKtraceDebugLevel(
                        OUT PULONG pKtraceLevel,
                        OUT PULONG pDefaultLevel)

 Description:
    This function reads the KtraceDebugLevel registry value from the registry
    to determine whether what level of tracing to utilize.
 
 Arguments:
    pKtraceLevel    - a pointer to the ktrace level.
 
 Return Values:
    STATUS_SUCCESS: Value was read successfully
    Any other NTSTATUS, the read failed. 
 
 History:
 
 *****************************************************************************/
EMCPAL_LOCAL EMCPAL_STATUS
ppfdCheckKtraceDebugLevel(OUT UINT_32* pKtraceLevel)
{
    EMCPAL_STATUS    status;
    CHAR        outBuffer[MAX_LOG_CHARACTER_BUFFER] = "";

    // read the key from the registry 
    status = ppfdReadRegistryValue(PPFD_REGISTRY_NAME_KTRACE_LEVEL,
                                   pKtraceLevel);
    if (!EMCPAL_IS_SUCCESS(status))
    {
        csx_p_snprintf(outBuffer,
                sizeof(outBuffer),
                "PPFD: %s() - Return status: 0x%X\n",
                __FUNCTION__,
                status);

        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_START, outBuffer);
    }

    return (status);
} // END ppfdCheckKtraceDebugLevel() function


/******************************************************************************
 Function:
    NTSTATUS ppfdCheckWindbgDebugEnable (PULONG pWindbgEnable)
    

 Description:
    This function reads the WindbgDebugEnable registry value from the registry
    to determine whether to enable Windbg messages. If enabled, the same mesages that are sent to
    ktrace buffers will also be sent to the debugger.
 
 Arguments:
    pWindbgEnable 
 
 Return Values:
    STATUS_SUCCESS: Value was read successfully
    Any other NTSTATUS, the read failed. 
 
 History:
 
 *****************************************************************************/
EMCPAL_LOCAL EMCPAL_STATUS 
ppfdCheckWindbgDebugEnable (UINT_32* pWindbgEnable)
{
    EMCPAL_STATUS    status;
    CHAR        outBuffer[MAX_LOG_CHARACTER_BUFFER] = "";

    // read the key from the registry 
    status = ppfdReadRegistryValue(PPFD_REGISTRY_NAME_WINDBG_ENABLE,
                                   pWindbgEnable);

    if (!EMCPAL_IS_SUCCESS(status))
    {
        csx_p_snprintf(outBuffer,
                sizeof(outBuffer),
                "PPFD: %s() - Return status: 0x%X\n",
                __FUNCTION__,
                status);

        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_START, outBuffer);
    }

    return (status);
}


/******************************************************************************

 Function:
    VOID ppfdInitializeDebugLevel(void)
 
 Description:
    This function initializes the debug print level for Ktrace. This 
    value is also applicable for Windbg output (if enabled). 
 
 Arguments:
    None.
 
 Return Values:
    None 
 
 History:
 
 *****************************************************************************/
VOID ppfdInitializeDebugLevel(void)
{
    EMCPAL_STATUS    status = EMCPAL_STATUS_UNSUCCESSFUL;
    UINT_32     DefaultLevel, KtraceLevel, DefaultWindbgEnable, WindbgEnable;
    CHAR        outBuffer[MAX_LOG_CHARACTER_BUFFER] = "";

    // establish default level 
    DefaultLevel = PPFD_GLOBAL_TRACE_FLAGS_DEFAULT;
    DefaultWindbgEnable = PPFD_ALLOW_DEBUG_PRINT;

    // attempt to read keys from registry
     
    status = ppfdCheckKtraceDebugLevel(&KtraceLevel);

    // if unsuccessful, use the Default
    if (!EMCPAL_IS_SUCCESS(status))
    {
        KtraceLevel = DefaultLevel;
    }

    // set the Ktrace logging level to the new level
    ppfdGlobalTraceFlags = KtraceLevel;

    // Set-up WinDbg Tracing Enable/Disable
    // The ktrace level also applies to the Windbg messages, but we can enable/disable 
    // Windbg messages
    status = ppfdCheckWindbgDebugEnable (&WindbgEnable);

    // if unsuccessful, use the Default
    if (!EMCPAL_IS_SUCCESS(status))
    {
        WindbgEnable = DefaultWindbgEnable;
    }
    ppfdGlobalWindbgTrace = WindbgEnable;

    // display ktrace level
    
    csx_p_snprintf(outBuffer,
                sizeof(outBuffer),
                "PPFD: %s() - Ktrace output level - 0x%X\n", 
                __FUNCTION__,
                 ppfdGlobalTraceFlags);

    PPFD_TRACE(PPFD_TRACE_FLAG_INFO, TRC_K_START, outBuffer);
    
    // display windbg enable
    
    csx_p_snprintf(outBuffer,
                sizeof(outBuffer),
                "PPFD: %s() - Windbg Enable - 0x%X\n", 
                __FUNCTION__,
                ppfdGlobalWindbgTrace);

    PPFD_TRACE(PPFD_TRACE_FLAG_INFO, TRC_K_START, outBuffer);

    return;
} // END ppfdInitializeDebugLevel() function 
