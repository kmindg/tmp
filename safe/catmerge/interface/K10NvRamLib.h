//***************************************************************************
// Copyright (C)  EMC Corporation 2001
// All rights reserved.
// Licensed material - property of EMC Corporation. * All rights reserved.
//**************************************************************************/

#ifndef _K10_NVRAM_LIB_
#define _K10_NVRAM_LIB_

//++
// File Name:
//      K10NvRamLib.h
//
// Contents:
//      Interface to the Non Volatile Ram Libary. 
//
// Exports:
//      NvRamLibReadWriteBytes()
//      NvRamLibInitSection()
//      NvRamLibCleanupSection()
//      NvRamLibFillFlushBuffer()
//      NvRamLibGetValue()
//
//
// Exported:
//
// Revision History:
//      11/10/00    MWagner   Created
//
//--

//++
// Include Files
//--

#include "k10ntddk.h"

#include "K10NvRam.h"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

//++
// End Includes
//--

#ifdef __cplusplus
/*
 * all of these functions need standard C linkage.  Otherwise, they can not be referenced from C.
 * Note, the implementation of these functions is within a cpp file, which allows these functions to
 * access C++ instances, instance methods and class methods
 */
extern "C" {
#endif


//++
// K10_NVRAM_LIB Constants
//
// The intent here is to define some device independent constants for K10_NVRAM_LIB
// access. Each Device specific header file must define a macro that this
// file can use to test for the actual physical device. The idea is that all
// code in K10_NVRAMLIB and K10_NVRAM should use the generic K10_NVRAM constants, and that
// all of those will be changes en masse when the Device changes.
//
// Users should use the device independent constant, not the PLX constants.
//
//--
#ifdef K10_NVRAM_PLX_DEVICE                 

#define K10_NVRAM_PCI_VENDOR_ID                           K10_NVRAM_PLX_PCI_VENDOR_ID         
#define K10_NVRAM_PCI_DEVICE_ID                           K10_NVRAM_PLX_PCI_DEVICE_ID         
#define K10_NVRAM_PCI_BASE_ADDRESS_REGISTER               K10_NVRAM_PLX_PCI_BASE_ADDRESS_REGISTER         

#endif

//++
// Enum:
//      K10_NVRAM_LIB_READ_WRITE_OPERATION
//
// Description:
//      The operations understood by NvRamLibReadWriteBytes()
//
// Members:
//
//      NvRamLib_ReadWrite_OpInvalid  = -1    An invalid K10_NVRAM_LIB_READ_WRITE_OPERATION
//
// Revision History:
//      11/10/00   MWagner    Created. 
//
//--
typedef enum _K10_NVRAM_LIB_READ_WRITE_OPERATION
{

        NvRamLib_ReadWrite_OpInvalid  = -1,
        NvRamLib_ReadWrite_Read,
        NvRamLib_ReadWrite_Write,
        NvRamLib_ReadWrite_Max

} K10_NVRAM_LIB_READ_WRITE_OPERATION, *PK10_NVRAM_LIB_READ_WRITE_OPERATION;
//.End                                                                        

//++
// Function:
//      NvRamLibReadWriteBytes()
//
// Description:
//      NvRamLibReadWriteBytes() reads or writes UlLength bytes at 
//      UlOffset from NonVolatile RAM
//
// Arguments:
//      UlOffset   the beginning offset, relative NVRAM Base Address
//      UlLength   number of bytes to be read or written
//      Operation  NvRamLib_ReadWrite_Read
//                 NvRamLib_ReadWrite_Write
//      PBytes     data to be read or written
//
// Return Value:
//      STATUS_SUCCESS:  the bytes were read or written
//      Others:          NVRAMLIB_WARNING_READWRITE_UNKNOWN_OP
//                       STATUS_SEMAPHORE_LIMIT_EXCEEDED
// Revision History:
//      11/10/00   MWagner    Created.
//
//--
EMCPAL_STATUS
CSX_MOD_EXPORT
NvRamLibReadWriteBytes(
                      IN ULONG                              UlOffset,
                      IN ULONG                              UlLength,
                      IN K10_NVRAM_LIB_READ_WRITE_OPERATION Operation,
                      IN PUCHAR                             PBytes
                      );

//++
// Function:
//      NvRamLibInitSection()
//
// Description:
//      NvRamLibInitSection() either maps the UlOffset and UlLength in Non
//      Volatile RAM to a Virtual Address, or allocates a buffer of UlLength
//      and fills it with the data in NVRAM starting at UlOffset.
//
// Arguments:
//      UlOffset         the offset within Non Volatile Ram
//      UlLength         size of area to be mapped.
//      BCacheEnable     passed to MmMapIoSpace() (if called)
//      PVirtualAddress  Mapped Virtual Address
//
// Return Value:
//      STATUS_SUCCESS:  an Area was found and mapped
//      Others:          STATUS_INVALID_PARAMETER
//                       STATUS_INSUFFICIENT_RESOURCES
//                       NVRAMLIB_WARNING_COULD_NOT_MAP_PHYSICAL_ADDRESS
//
// Revision History:
//      10/13/08   Joe Ash    Created.
//
//--
EMCPAL_STATUS
CSX_MOD_EXPORT
NvRamLibInitSection(
                    IN ULONG     UlOffset,
                    IN ULONG     UlLength,
                    IN BOOLEAN   BCacheEnable,
                    OUT PUCHAR*  PVirtualAddress
                    );

//++
// Function:
//      NvRamLibCleanupSection()
//
// Description:
//      NvRamLibCleanupSection() either unmaps the a Virtal Address mapped
//      earlier with NvRamLibInitSection() or frees the buffer created in
//      NvRamLibInitSection().
//
// Arguments:
//      VirtualAddress   Address to be unmapped/freed
//      UlLength         Size of the mapped area
//
// Return Value:
//      STATUS_SUCCESS:  the area was unmapped/freed
//
// Revision History:
//      10/13/08   Joe Ash    Created.
//
//--
EMCPAL_STATUS
CSX_MOD_EXPORT
NvRamLibCleanupSection(
                       IN PUCHAR    VirtualAddress,
                       IN ULONG     UlLength
                       );

//++
// Function:
//      NvRamLibGetValue
//
// Description:
//      Please see K10NvRamLib.h for comments
// Arguments:
//      PKey             Name of the key
//      UlKeyLength      Size of the key's name
//      PBytes           Buffer to search in
//      UlLength         Size of buffer
//      PResultAddress   Pointer to the result string
//
// Return Value:
//      STATUS_SUCCESS:  the key was has been successfuly found
//
// Revision History:
//      07/10/15   Alina Stepina    Created.
//
//--

EMCPAL_STATUS
CSX_MOD_EXPORT
NvRamLibGetKeyValue(
                 IN PUCHAR   PKey,
                 IN ULONG    UlKeyLength,
                 IN PUCHAR   PBytes,
                 IN ULONG    UlLength,
                 OUT PUCHAR  *PResultAddress
                );

#if 0  // DEPRECATED
//++
// Function:
//      NvRamLibFillFlushBuffer()
//
// Description:
//      NvRamLibFillFlushBuffer() either calls NvRamLibReadWriteBytes to fill 
//      or flush the buffer, or returns if the buffer is memory mapped.
//
// Arguments:
//      UlOffset   the beginning offset, relative NVRAM Base Address
//      UlLength   number of bytes to be read or written
//      Operation  NvRamLib_ReadWrite_Read
//                 NvRamLib_ReadWrite_Write
//      PBytes     data to be read or written
//
// Return Value:
//      STATUS_SUCCESS:  the bytes were read or written
//
// Revision History:
//      10/13/08   Joe Ash    Created.
//
//--
EMCPAL_STATUS
CSX_MOD_EXPORT
NvRamLibFillFlushBuffer(
                        IN ULONG                                UlOffset,
                        IN ULONG                                UlLength,
                        IN K10_NVRAM_LIB_READ_WRITE_OPERATION   Operation,
                        IN PUCHAR                               PBytes
                        );
#endif // 0

BOOLEAN 
CSX_MOD_EXPORT
NvRamLibGetSectionInfo(
                      IN  NVRAM_SECTIONS nvramSection,
                      OUT PNVRAM_SECTION_RECORD nvramSectionInfo
                      );

#ifdef __cplusplus
};
#endif

                      
#endif // _K10_NVRAM_LIB_

// End K10NvRamLib.h
