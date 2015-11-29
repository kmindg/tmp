#ifndef __SPID_STATIC_LIB__
#define __SPID_STATIC_LIB__

//***************************************************************************
// Copyright (C) EMC Corporation 1989 - 2008
// All rights reserved.
// Licensed material -- property of Data General Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      spid_static_lib.h
//
// Contents:
//      Function prototypes for the SPID functions exported as a statically
//      linked library.  Use of this interface is an exception case built
//      specifically for the dual-mode driver's crash dump instantiation.
//      All other components should use the appropriate kernel or user mode
//      interface.
//
// Revision History:
//  27-Jun-03   Matt Yellen    Created.
//  01-Feb-06   Phil Leef      Adding SIO/SB defines, used exclusively by the static lib.
//--

CSX_CDECLS_BEGIN

#include "spid_types.h"
#include "intel_ICH_header.h"
#include "superIO_header.h"


#define NVRAM_IO_ACCESS_ADDR_REG                    0x6A8
#define NVRAM_IO_ACCESS_DATA_REG                    0x6AC

#define FWH_GPI_REG                             0xFFBC0100  /* Firmware Hub General Purpose Input Register */

/***************************
 * struct: ICH_BASE_OFFSET
 *
 * Defines the Base address
 * offsets
 ***************************/
typedef enum _ICH_BASE_OFFSET
{
    PMBASE,     // ACPI Base Address
    GPIOBASE,   // GPIO Base Address
    TCOBASE,    // ACPI TCO Base Address
} ICH_BASE_OFFSET;

/***************************
 * struct: SUPERIO_LDN
 *
 * Defines the SuperIO LDNs
 ***************************/
typedef enum _SUPERIO_LDN
{
    SWC,        // System Wake-up Control
    GPIO,       // General Purpose I/O
    XBUS,       // X-bus Extension
} SUPERIO_LDN;



/* Alt SP ID Engine Numbr
 * Note: These are defined locally, so conditionally
 * compiled header files do not have to get pulled in
 */

#ifdef C4_INTEGRATED
#define SMBUS_GPIO_SET_PIN  0
#define SMBUS_GPIO_SET_LED  1
#define SMBUS_GPIO_SET_FLT  2
typedef struct _SMBUS_GPIO_REQUEST
{
    csx_u32_t  reqPinNum;
    csx_u32_t  activeLow;
    csx_u32_t  blinkRate;
    csx_u32_t  pinValue;
    csx_u32_t  DevType;
    csx_u32_t  WriteType;
} SMBUS_GPIO_REQUEST;
#endif /* C4_INTEGRATED - C4HW - C4BUG - sounds suspicious */

extern ULONG SUPER_IO_BASE_ADDR_GPIO_PORT;
extern ULONG SUPER_IO_BASE_ADDR_SWC_PORT;

extern ULONG SB_IO_BASE_ADDR_GPIO_PORT;
extern ULONG SB_PMBASE_ADDR_GPIO_PORT;
extern ULONG SB_TCOBASE_ADDR;

VOID spidStaticLibInitAllGpioPins(HW_CPU_MODULE cpuModule, 
                                  SPID_HW_TYPE  platformType);

VOID spidStaticLibGpioInitAttr (GPIO_SIGNAL_NAME  gpioName,
                                char              gpioStr[],
                                ULONG             pinNum,
                                BOOLEAN           activeLow,
                                GPIO_DEVICE_TYPES devType,
                                BOOLEAN           writeable,
                                BOOLEAN           outputOnly);

GPIO_ATTR_REQUEST spidStaticLibGetGpioAttrFromNiceName (GPIO_SIGNAL_NAME gpioName);

ICH_TYPE spidStaticLibGetICHType(void);

ULONG spidGetSouthBridgeBasePort(ICH_BASE_OFFSET baseOffset);

VOID spidGetSouthBridgeGPIOData(ULONG       reqPinNum,
                                PULONG      pPinBitmask,
                                PULONG      pRegOffset,
                                PULONG_PTR  pBaseAddrGpioPort);

EMCPAL_STATUS spidGetGpioPin(PGPIO_REQUEST pGpioReq);

EMCPAL_STATUS spidSetGpioPin(PGPIO_MULTIPLE_REQUEST pGpioReq);

VOID spidSetFaultExpanderGpioBits(UCHAR code);

VOID spidIssueNMIToICH(void);

VOID spidStaticLibPanicSP(ULONG data);

ULONG spidGetSuperIOBasePort(SUPERIO_LDN logical_device);

csx_phys_address_t spidGetPcieMemAddr(ULONG     bus,
                                      ULONG     device,
                                      ULONG     function,
                                      ULONG     offset);

csx_bool_t  spidPciDeviceMatch(csx_u16_t   bus, 
                               csx_u16_t   device, 
                               csx_u16_t   function, 
                               csx_u16_t   vendorid, 
                               csx_u16_t   deviceid);

CSX_CDECLS_END

#endif // __SPID_STATIC_LIB__
