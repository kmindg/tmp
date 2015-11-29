/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               spid_simulation_interface.h
 ***************************************************************************
 *
 * DESCRIPTION:  Maps all spid function calls to the spidSim functions
 *
 * NOTES:
 *               
 *
 * HISTORY:
 *    10/20/2010  Martin Buckley Initial Version
 *
 **************************************************************************/

#ifndef _SPID_SIM_
#define _SPID_SIM_

#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT
# include "generic_types.h"
# include "spid_kernel_interface.h"
# include "generic_types.h"

#if defined(WDMSERVICES_EXPORT)
#define SPIDSIMPUBLIC CSX_MOD_EXPORT
#else
#define SPIDSIMPUBLIC CSX_MOD_IMPORT
#endif

#if defined(__cplusplus)
extern "C"
{
#endif


SPIDSIMPUBLIC EMCPAL_STATUS spidSimGetSpid(PSPID spid_ptr);

SPIDSIMPUBLIC EMCPAL_STATUS spidSimSetArrayWWN(PK10_ARRAY_ID array_wwn_ptr);

SPIDSIMPUBLIC EMCPAL_STATUS spidSimGetHwType(SPID_HW_TYPE *hwtype_ptr);

SPIDSIMPUBLIC EMCPAL_STATUS spidSimGetHwTypeEx(PSPID_PLATFORM_INFO platformInfo_ptr);

SPIDSIMPUBLIC EMCPAL_STATUS spidSimGetHwName(CHAR *hwname_ptr);

SPIDSIMPUBLIC EMCPAL_STATUS spidSimGetPeerInserted(BOOLEAN *peer_inserted_ptr);

SPIDSIMPUBLIC EMCPAL_STATUS spidSimGetFRUInserted(FRU_MODULES fru, BOOLEAN *fru_inserted_ptr);

SPIDSIMPUBLIC EMCPAL_STATUS spidSimGetGpio(PGPIO_REQUEST pGpioReq);

SPIDSIMPUBLIC EMCPAL_STATUS spidSimSetGpio(PGPIO_REQUEST pGpioReq);

SPIDSIMPUBLIC EMCPAL_STATUS spidSimSetGpios(PGPIO_MULTIPLE_REQUEST pGpioMultiReq);

SPIDSIMPUBLIC EMCPAL_STATUS spidSimGetGpioAttr(GPIO_SIGNAL_NAME gpio_signal_name, PGPIO_ATTR_REQUEST gpioReg_ptr);

SPIDSIMPUBLIC EMCPAL_STATUS spidSimGetKernelType(SPID_KERNEL_TYPE *kerneltype_ptr);

SPIDSIMPUBLIC EMCPAL_STATUS spidSimGetKernelName(CHAR *kernelname_ptr);

SPIDSIMPUBLIC EMCPAL_STATUS spidSimIsSingleSPSystem(BOOLEAN * single_sp_ptr);

SPIDSIMPUBLIC BOOLEAN spidSimIsDriverLoadError(void);

SPIDSIMPUBLIC EMCPAL_STATUS spidSimSetDriverLoadError(void);

SPIDSIMPUBLIC EMCPAL_STATUS spidSimClearDriverLoadError(void);

SPIDSIMPUBLIC EMCPAL_STATUS spidSimSetFaultStatusCode(PFLT_STATUS_REQUEST pfltStatusReq);

SPIDSIMPUBLIC EMCPAL_STATUS spidSimSetDegradedModeReason(SPID_DEGRADED_REASON degraded_reason);

SPIDSIMPUBLIC EMCPAL_STATUS spidSimGetDegradedModeReason(PSPID_DEGRADED_REASON p_degraded_reason);

SPIDSIMPUBLIC EMCPAL_STATUS spidSimSetForceDegradedMode(void);

SPIDSIMPUBLIC EMCPAL_STATUS spidSimGetLastRebootType(LAST_REBOOT_TYPE *reboottype_ptr);

SPIDSIMPUBLIC EMCPAL_STATUS spidSimIsIoModuleSupported(HW_MODULE_TYPE fru_id, BOOLEAN *is_supported_ptr);

SPIDSIMPUBLIC EMCPAL_STATUS spidSimIssueNMI(void);

SPIDSIMPUBLIC EMCPAL_STATUS spidSimSpidIsMiniSetupInProgress(BOOLEAN *inProgress_ptr);

SPIDSIMPUBLIC EMCPAL_STATUS spidSimFlushFilesAndReg( void );

SPIDSIMPUBLIC EMCPAL_STATUS spidSimGetPCIBusData(UINT_32    bus,
                                   UINT_32  device,
                                   UINT_32  function, 
                                   BOOLEAN  unmapConfigData, 
                                   UINT_32  sizeInBytes,
                                   csx_p_io_map_t *pIoMapObject,
                                   PVOID    *ppPCIConfigData);

SPIDSIMPUBLIC EMCPAL_STATUS spidSimIsReleaseBuild(BOOLEAN *releaseBuild_ptr);

#if defined(__cplusplus)
}
#endif

#define spidGetSpid(a)                  spidSimGetSpid(a)
#define spidSetArrayWWN(a)              spidSimSetArrayWWN(a)
#define spidGetHwType(a)                spidSimGetHwType(a)
#define spidGetHwTypeEx(a)              spidSimGetHwTypeEx(a)
#define spidGetHwName(a)                spidSimGetHwName(a)
#define spidGetPeerInserted(a)          spidSimGetPeerInserted(a)
#define spidGetFRUInserted(a, b)        spidSimGetFRUInserted(a, b)
#define spidGetGpio(a)                  spidSimGetGpio(a)
#define spidSetGpio(a)                  spidSimSetGpio(a)
#define spidSetGpios(a)                 spidSimSetGpios(a)
#define spidGetGpioAttr(a, b)           spidSimGetGpioAttr(a, b)
#define spidGetKernelType(a)            spidSimGetKernelType(a)
#define spidGetKernelName(a)            spidSimGetKernelName(a)
#define spidIsSingleSPSystem(a)         spidSimIsSingleSPSystem(a)
#define spidIsDriverLoadError()         spidSimIsDriverLoadError()
#define spidSetDriverLoadError()        spidSimSetDriverLoadError()
#define spidClearDriverLoadError()      spidSimClearDriverLoadError()
#define spidSetFaultStatusCode(a)       spidSimSetFaultStatusCode(a)
#define spidSetDegradedModeReason(a)    spidSimSetDegradedModeReason(a)
#define spidGetDegradedModeReason(a)    spidSimGetDegradedModeReason(a)
#define spidSetForceDegradedMode()      spidSimSetForceDegradedMode()
#define spidGetLastRebootType(a)        spidSimGetLastRebootType(a)
#define spidIsIoModuleSupported(a, b)   spidSimIsIoModuleSupported(a, b)
#define spidIssueNMI()                  spidSimIssueNMI()
#define SpidIsMiniSetupInProgress(a)    SpidSimIsMiniSetupInProgress(a)
#define spidFlushFilesAndReg()          spidSimFlushFilesAndReg()
#define spidGetPCIBusData(a,b,c,d,e,f,g)  spidSimGetPCIBusData(a,b,c,d,e,f,g)
#define spidIsReleaseBuild(a)           spidSimIsReleaseBuild(a)

#endif // _SPID_SIM_
