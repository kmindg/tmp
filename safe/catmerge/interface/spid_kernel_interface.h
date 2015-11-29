#ifndef __SPID_KERNEL_INTERFACE__
#define __SPID_KERNEL_INTERFACE__
//***************************************************************************
// Copyright (C) EMC Corporation 1989 - 2011
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//		spid_kernel_interface.h
//
// Contents:
//		The SPID Pseudo-device driver's kernel space interface.
//
// Revision History:
//  30-Jun-03   Matt Yellen       Created as part of the spid interface rewrite.
//  25-Jul-06   Joe Ash           Added spidGetLastRebootType interface.
//  19-Sep-06   Joe Ash		      Added interface for getting engine id from hardware.
//  09-Jul-07   Joe Ash		      Added interface for getting IO Module type.
//--

#include "k10ntddk.h"
#include "spid_types.h"
#include "familyfruid.h"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

#ifdef _SPID_
#define SPIDAPI CSX_MOD_EXPORT
#else
#define SPIDAPI CSX_MOD_IMPORT
#endif

#ifdef _SPIDX_
#define SPIDXAPI CSX_MOD_EXPORT
#else
#define SPIDXAPI CSX_MOD_IMPORT
#endif

#if defined(__cplusplus)
extern "C"
{
#endif

/*************************  E X P O R T E D   I N T E R F A C E   F U N C T I O N S  ***************************/

SPIDAPI EMCPAL_STATUS spidGetSpid(PSPID spid_ptr);

SPIDAPI EMCPAL_STATUS spidSetArrayWWN(PK10_ARRAY_ID array_wwn_ptr);

SPIDAPI EMCPAL_STATUS spidGetHwType(SPID_HW_TYPE *hwtype_ptr);

SPIDAPI EMCPAL_STATUS spidGetHwTypeEx(PSPID_PLATFORM_INFO platformInfo_ptr);

SPIDAPI EMCPAL_STATUS spidGetHwName(CHAR *hwname_ptr);

SPIDAPI EMCPAL_STATUS spidGetPeerInserted(BOOLEAN *peer_inserted_ptr);

#ifdef C4_INTEGRATED
SPIDAPI EMCPAL_STATUS spidIsPeerCacheCard (BOOLEAN *peer_is_cache_card_ptr);
#endif /* C4_INTEGRATED - C4HW */

SPIDAPI EMCPAL_STATUS spidGetFRUInserted(FRU_MODULES fru, BOOLEAN *fru_inserted_ptr);

SPIDAPI EMCPAL_STATUS spidGetGpio(PGPIO_REQUEST pGpioReq);

SPIDAPI EMCPAL_STATUS spidSetGpio(PGPIO_REQUEST pGpioReq);

SPIDAPI EMCPAL_STATUS spidSetGpios(PGPIO_MULTIPLE_REQUEST pGpioMultiReq);

SPIDAPI EMCPAL_STATUS spidGetGpioAttr(GPIO_SIGNAL_NAME gpio_signal_name, PGPIO_ATTR_REQUEST gpioReg_ptr);

SPIDAPI EMCPAL_STATUS spidGetKernelType(SPID_KERNEL_TYPE *kerneltype_ptr);

SPIDAPI EMCPAL_STATUS spidGetKernelName(CHAR *kernelname_ptr);

SPIDAPI EMCPAL_STATUS spidIsSingleSPSystem(BOOLEAN * single_sp_ptr);

SPIDAPI BOOLEAN spidIsDriverLoadError(void);

SPIDAPI EMCPAL_STATUS spidSetDriverLoadError(void);

SPIDAPI EMCPAL_STATUS spidClearDriverLoadError(void);

SPIDAPI EMCPAL_STATUS spidSetFaultStatusCode(PFLT_STATUS_REQUEST pfltStatusReq);

SPIDAPI EMCPAL_STATUS spidSetDegradedModeReason(SPID_DEGRADED_REASON degraded_reason);

SPIDAPI EMCPAL_STATUS spidGetDegradedModeReason(PSPID_DEGRADED_REASON p_degraded_reason);

SPIDAPI EMCPAL_STATUS spidSetForceDegradedMode(void);

SPIDAPI EMCPAL_STATUS spidGetLastRebootType(LAST_REBOOT_TYPE *reboottype_ptr);

SPIDAPI EMCPAL_STATUS spidIsIoModuleSupported(HW_MODULE_TYPE fru_id, BOOLEAN *is_supported_ptr);

SPIDAPI EMCPAL_STATUS spidIssueNMI(void);

SPIDAPI EMCPAL_STATUS spidIsMiniSetupInProgress(BOOLEAN *inProgress_ptr);

SPIDAPI EMCPAL_STATUS spidIsOOBEPhaseInProgress(BOOLEAN *inProgress_ptr);

SPIDAPI EMCPAL_STATUS spidClearOOBEPhaseInProgress(void);

SPIDAPI EMCPAL_STATUS spidFlushFilesAndReg( void );

#ifdef C4_INTEGRATED
SPIDXAPI EMCPAL_STATUS SpidxSetGpio(PGPIO_MULTIPLE_REQUEST gpioReqPtr);
SPIDXAPI EMCPAL_STATUS SpidxGetGpio(PGPIO_REQUEST pGpioReq);
SPIDXAPI EMCPAL_STATUS SpidxGetSpid(PSPID spid_ptr);
SPIDXAPI EMCPAL_STATUS SpidxGetHwTypeEx(PSPID_PLATFORM_INFO platformInfo_ptr);
#endif /* C4_INTEGRATED - C4ARCH - use of kernel SPIDX on SIO/SB GPIO access */

#define PCI_EXTENDED_CONFIG_SIZE        4096        // 4K region

SPIDAPI EMCPAL_STATUS spidGetPCIBusData(ULONG    bus,
                                        ULONG    device,
                                        ULONG    function,
                                        BOOLEAN  unmapConfigData,
                                        ULONG    sizeInBytes,
                                        csx_p_io_map_t *pIoMapObject,
                                        PVOID    *ppPCIConfigData);

SPIDAPI EMCPAL_STATUS spidReportPhysicalMemorySize(ULONG dimmPhysicalMemory);

SPIDAPI EMCPAL_STATUS  spidIsReleaseBuild(BOOLEAN *releaseBuild_ptr);

#ifdef C4_INTEGRATED
SPIDAPI BOOLEAN spidValidateC4NvramHeader (ULONG * nvram_area_size);
#endif

#if defined(SIMMODE_ENV)
/*
 * remap spid calls to spidSim calls (must be last)
 */ 
#include "simulation/spid_simulation_interface.h"
#endif

#if defined(__cplusplus)
}
#endif

#endif //__SPID_KERNEL_INTERFACE__
