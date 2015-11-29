#ifndef __WDT_DRIVER_INTERFACE_H__
#define __WDT_DRIVER_INTERFACE_H__
//***************************************************************************
// Copyright (C) EMC Corporation 2006
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//++
// FILE:
//      WdtDriverInterface.h
//
// DESCRIPTION:
//      This file contains the IOCTL codes and data structures which 
//      define the WDT Driver's interface.  
//
// NOTES:
//
// HISTORY:
//      March, 2006 - Created, Ian McFadries
//
//      7/20/10     scz   Add hook for HBD to set WDT timeout period
//--


#include "WatchdogCommon.h"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT


//++
// Device name constants
//--
#define WDTDRV_BASE_DEVICE_NAME	  "WdtDrv"
#define WDTDRV_NT_DEVICE_NAME	  "\\Device\\" WDTDRV_BASE_DEVICE_NAME
#define WDTDRV_DOSDEVICES_NAME    "\\DosDevices\\" WDTDRV_BASE_DEVICE_NAME
#define WDTDRV_WIN32_DEVICE_NAME  "\\\\.\\" WDTDRV_BASE_DEVICE_NAME

//++
// Registry Key name constants
// Name strings used by WDT in the Registry.
//--

// The name of the WDTDRV Registry subkey, below WDTDRV_PARAMETERS_REGISTRY_KEY_NAME, 
// that contains the control for enable/disable of the WDT driver.
#define WDTDRV_ENABLED_REGISTRY_KEY_NAME      "Enabled"
// The name of the WDTDRV Registry subkey, below WDTDRV_PARAMETERS_REGISTRY_KEY_NAME, 
// that contains the timeout (in seconds) for the WDT driver.
#define WDTDRV_TIMEOUT_REGISTRY_KEY_NAME      "TimeoutSeconds"
// The name of the WDTDRV Registry subkey, below WDTDRV_PARAMETERS_REGISTRY_KEY_NAME, 
// that causes a breakpoint at start of WDT DriverEntry function in checked builds.
#define WDTDRV_BREAK_ENTRY_REGISTRY_KEY_NAME  "BreakOnEntry"
// The name of the WDTDRV Registry subkey, below WDTDRV_PARAMETERS_REGISTRY_KEY_NAME, 
// that provides the level of traces to be used.
#define WDTDRV_TRACEFLAGS_REGISTRY_KEY_NAME  "TraceFlags"

//++
// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-32767, and 32768-65535 are reserved for use
// by customers.  
//--
#define FILE_DEVICE_WDT_ICH_TCO    0x90FB    // unique but arbitrary  0x90FB = 37115.

#define WDT_PERIOD_USE_REGISTRY 0xFFFF

//  The following control codes (IOCTLs) are for operations related to the ICH TCO WDT
#define IOCTL_WDT_ICH_TCO_BASE  FILE_DEVICE_WDT_ICH_TCO
#define IOCTL_WDT_START         EMCPAL_IOCTL_CTL_CODE(IOCTL_WDT_ICH_TCO_BASE, 0x801, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)
#define IOCTL_WDT_STOP          EMCPAL_IOCTL_CTL_CODE(IOCTL_WDT_ICH_TCO_BASE, 0x802, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)
#define IOCTL_WDT_REFRESH       EMCPAL_IOCTL_CTL_CODE(IOCTL_WDT_ICH_TCO_BASE, 0x803, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)
#define IOCTL_WDT_SETTIMEOUT    EMCPAL_IOCTL_CTL_CODE(IOCTL_WDT_ICH_TCO_BASE, 0x804, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)
#define IOCTL_WDT_GETTIMEOUT    EMCPAL_IOCTL_CTL_CODE(IOCTL_WDT_ICH_TCO_BASE, 0x805, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

//++
// typedefs
//--

// structure containing input data to set WDT timeout value
typedef struct _WDT_SETTIMEOUT_IOCTL_INPUT_INFO
{
   ULONG          WdtTimeoutSecs;
} WDT_SETTIMEOUT_IOCTL_INPUT_INFO, * PWDT_SETTIMEOUT_IOCTL_INPUT_INFO;

// structure containing output data to get WDT timeout value
typedef struct _WDT_GETTIMEOUT_IOCTL_OUTPUT_INFO
{
   ULONG          WdtTimeoutSecs;
} WDT_GETTIMEOUT_IOCTL_OUTPUT_INFO, * PWDT_GETTIMEOUT_IOCTL_OUTPUT_INFO;


//++
// typedefs and exported interface functions to maintain 
// consistency with original software watchdog interface
//--

#ifdef _WDT_
#define WDTAPI CSX_MOD_EXPORT
#else
#define WDTAPI CSX_MOD_IMPORT
#endif

// WDT available for use if debugger is not enabled
#define WDT_AVAILABLE (!EmcpalIsDebuggerEnabled())

//++
// Exported Interface Functions
//--
WDTAPI ULONG WdtAllocateWatchdog(IN PWATCHDOG pHandle);
WDTAPI VOID  WdtReleaseWatchdog(IN WATCHDOG handle);
WDTAPI VOID  WdtRefreshWatchdog(IN WATCHDOG handle, IN ULONG timeout);
WDTAPI VOID  WdtSetWatchdog(IN WATCHDOG handle, IN ULONG timeout);
#endif // End __WDT_DRIVER_INTERFACE_H__
//++
// End WdtDriverInterface.h
//--
