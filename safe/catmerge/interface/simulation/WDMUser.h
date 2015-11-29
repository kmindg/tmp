/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               WDMUser.h
 ***************************************************************************
 *
 * DESCRIPTION:  Function prototypes and Preprocessor directives to remap
 *               the Windows Device Model functions to simulation aware versions
 *
 *               This include file is provided for easy conversion of standalone
 *               test programs for inclusion in the simulation environment
 *               WDMUser.h and WDMInterface.h can not be included together in a module
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    10/29/2010  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef _WDMUSER_
#define _WDMUSER_

# include "csx_ext.h"
/*
 * only configured when targetmode is simulation
 */
#if defined(SIMMODE_ENV)

//# include <windows.h>
#ifdef __cplusplus
extern "C" {
#endif



#if defined(WDMSERVICES_EXPORT)
#define WDMSERVICEPUBLIC CSX_MOD_EXPORT
#else
#define WDMSERVICEPUBLIC CSX_MOD_IMPORT
#endif

/*
 * simulation versions of the standard Microsoft Windows File IO functions
 */

WDMSERVICEPUBLIC HANDLE WDMUser_OpenDeviceFile(
  char * lpFileName
);

WDMSERVICEPUBLIC BOOL WDMUser_DeviceIoControl(
  HANDLE hDevice,
  DWORD dwIoControlCode,
  void * lpInBuffer,
  DWORD nInBufferSize,
  void * lpOutBuffer,
  DWORD nOutBufferSize,
  DWORD * lpBytesReturned,
  void * lpOverlapped
);

WDMSERVICEPUBLIC BOOL WDMUser_CloseHandle( HANDLE hObject);


/*
 * Preprocessor remappings 
 */

#ifdef ENABLE_WDMI

#undef CreateFile
#define CreateFile(a,b,c,d,e,f,g) WDMUser_CreateFile(a,b,c,d,e,f,g)

#undef DeviceIoControl
#define DeviceIoControl(a,b,c,d,e,f,g,h) WDMUser_DeviceIoControl(a,b,c,d,e,f,g,h)

#undef CloseHandle
#define CloseHandle(a)  WDMUser_CloseHandle(a)

#endif // ENABLE_WDMI

#ifdef __cplusplus
};
#endif


#endif // defined(SIMMODE_ENV)

#endif //_WDMUSER_
