#ifndef __SPID_USER_INTERFACE__
#define __SPID_USER_INTERFACE__
//***************************************************************************
// Copyright (C) EMC Corporation 1989-2011
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      spid_user_interface.h
//
// Contents:
//      The SPID Pseudo-device driver's user space interface.
//
// Revision History:
//  30-Jun-03   Matt Yellen       Created as part of the spid interface rewrite.
//  25-Jul-06   Joe Ash           Added spidGetLastRebootType interface.
//  09-Jul-07   Joe Ash           Added interface for getting IO Module type.
//--

#include <windows.h>
#include "spid_types.h"
#include "familyfruid.h"
#include "EmcUTIL_Device.h"



#define SPID_API __stdcall

#if defined(__cplusplus)
extern "C"
{
#endif

//The global spid handle.  This can either be opened explicitly,
//or will be opened implicitly on the first call to an interface function
extern EMCUTIL_RDEVICE_REFERENCE SpidGlobalHandle;

/*************************  E X P O R T E D   I N T E R F A C E   F U N C T I O N S  ***************************/

//Note, these 2 functions are unique to the user space interface and are optional.
EMCUTIL_STATUS SPID_API spidOpenHandle(void);
BOOL SPID_API spidCloseHandle(void);

DWORD SPID_API spidGetSpid(PSPID spid_ptr);

DWORD SPID_API spidSetArrayWWN(PK10_ARRAY_ID array_wwn_ptr);

DWORD SPID_API spidPanicSP(void);

DWORD SPID_API spidGetHwType(SPID_HW_TYPE *hwtype_ptr);

DWORD SPID_API spidGetHwTypeEx(PSPID_PLATFORM_INFO platformInfo_ptr);

DWORD SPID_API spidGetHwName(CHAR *hwname_ptr);

DWORD SPID_API spidGetPeerInserted(BOOLEAN *peer_inserted_ptr);

DWORD SPID_API spidGetKernelType(SPID_KERNEL_TYPE *kerneltype_ptr);

DWORD SPID_API spidGetKernelName(CHAR *kernelname_ptr);

DWORD SPID_API spidIsSingleSPSystem(BOOLEAN * single_sp_ptr);

DWORD SPID_API spidGetGpio(PGPIO_REQUEST pGpioReq);

DWORD SPID_API spidSetGpio(PGPIO_REQUEST pGpioReq);

DWORD SPID_API spidGetGpioAttr(GPIO_SIGNAL_NAME gpio_signal_name, PGPIO_ATTR_REQUEST gpioAttrReg_ptr);

DWORD SPID_API spidSetFaultStatusCode(PFLT_STATUS_REQUEST pfltStatusReq);

DWORD SPID_API spidGetLastRebootType(LAST_REBOOT_TYPE *reboottype_ptr);

DWORD SPID_API spidIsIoModuleSupported(HW_MODULE_TYPE fru_id, BOOLEAN *is_supported_ptr);

DWORD SPID_API spidSetFlushEventHandle( HANDLE *hFlushEvent_ptr );

DWORD SPID_API spidSetDegradedModeReason(SPID_DEGRADED_REASON degraded_reason);

DWORD SPID_API spidGetDegradedModeReason(PSPID_DEGRADED_REASON p_degraded_reason);

DWORD SPID_API spidSetForceDegradedMode(void);

DWORD SPID_API spidIsOOBEPhaseInProgress(BOOLEAN *inProgress_ptr);

DWORD SPID_API spidClearOOBEPhaseInProgress(void);

DWORD SPID_API spidIsReleaseBuild(BOOLEAN *releaseBuild_ptr);
#if defined(__cplusplus)
}
#endif

#endif //__SPID_USER_INTERFACE__
