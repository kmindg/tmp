//***************************************************************************
// Copyright (C)  EMC Corporation 2001
// All rights reserved.
// Licensed material - property of EMC Corporation. * All rights reserved.
//**************************************************************************/
#ifndef _K10_NVRAM_H_
#define _K10_NVRAM_H_

//++
// File Name:
//      K10NvRam.h
//
// Description:   This file contains all of the necessary information to utilize the 
//                IOCTL interface to the K10 K10_NVRAM device
//
//                ntddk.h is a required include from kernel space.
//                wtypes.h is a required include from user space.
//
// Exported:
//      K10_NVRAM_DG_PART_NUMBER       
//      K10_NVRAM_DG_REVISION                  
//      K10_NVRAM_DG_SERIAL_NUMBER             
//      K10_NVRAM_VENDOR_NAME                  
//      K10_NVRAM_DATE_OF_MANUFACTURE          
//      K10_NVRAM_LOCALITY_OF_MANUFACTURE      
//      K10_NVRAM_WORLD_WIDE_NAME              
//      K10_NVRAM_RESERVED_FOR_BIOS_FIRMWARE   
//      K10_NVRAM_SINGLE_BIT_ECC_LOG           
//      K10_NVRAM_REBOOT_DATA                  
//
//      K10_NVRAM_IOCTL_OPERATION_READ_BYTES
//      K10_NVRAM_IOCTL_OPERATION_WRITE_BYTES
//
//      K10_NVRAM_READ_BYTES_IN_BUFFER
//      K10_NVRAM_READ_BYTES_OUT_BUFFER
//      K10_NVRAM_WRITE_BYTES_IN_BUFFER
//      K10_NVRAM_WRITE_BYTES_OUT_BUFFER
//
// Revision History:
//      11/7/00    MWagner   Created
//
//--

#include "K10LayeredDefs.h"
#include "EmcPAL_Ioctl.h"

//++
// Macro:
//      K10_NVRAM_CURRENT_VERSION
//
// Description:
//      The "Current Version"
//
// Revision History:
//      11/13/00   MWagner    Created.
//
//--
#define K10_NVRAM_CURRENT_VERSION                  0x1
// .End K10_NVRAM_CURRENT_VERSION

//++
// K10_NVRAM Constants
//
// The intent here is to define some device independent constants for K10_NVRAM
// access. Each Device specific header file must define a macro that this
// file can use to test for the actual physical device. The idea is that all
// code in K10_NVRAMLIB and K10_NVRAM should use the generic K10_NVRAM constants, and that
// all of those will be changes en masse when the Device changes.
//
// Users should use the device independent constant, not the PLX constants.
//
//--
#include "K10Plx.h"


//++
// K10_NVRAM Names
//
//
// Names for Kernel and User Space
//
//--

//++
// Constant:
//      K10_NVRAM_NAME
//
// Description:
//      The "full" name of Non Volatile Ram Driver
//
// Revision History:
//      13-Nov-00   MWagner  Created.
//
//--
#define K10_NVRAM_NAME      "NonVolatileRam"
//.End                                                                        

//++
// Constant:
//      K10_NVRAM_ABBREV
//
// Description:
//      The "abbreviated"  name of Non Volatile Ram Driver
//
// Revision History:
//      13-Nov-00   MWagner  Created.
//
//--
#define K10_NVRAM_ABBREV    "NvRam"
//.End                                                                        


//++
// Constant:
//      K10_NVRAM_ABBREV_W
//
// Description:
//      The wide "abbreviated"  name of Non Volatile Ram Driver 
//
// Revision History:
//      13-Nov-00   MWagner  Created.
//
//--
#define K10_NVRAM_ABBREV_W    L"NvRam"

//.End                                                                        

//++
// Constant:
//      K10_NVRAM_NT_DEVICE_NAME 
//
// Description:
//      The K10_NVRAM Driver NT Device name
//
// Revision History:
//      13-Nov-00   MWagner  Created.
//
//--
#define K10_NVRAM_NT_DEVICE_NAME    "\\Device\\" K10_NVRAM_NAME

//.End                                                                        

//++
// Constant:
//      K10_NVRAM_WIN32_DEVICE_NAME   
//
// Description:
//      The K10_NVRAM Driver Win32 Device name
//
// Revision History:
//      13-Nov-00   MWagner  Created.
//
//--
#define K10_NVRAM_WIN32_DEVICE_NAME   K10_WIN32_DEVICE_PREFIX K10_NVRAM_ABBREV
//.End                                                                        

//++
// Constant:
//      K10_NVRAM_USER_DEVICE_NAME 
//
// Description:
//      The K10_NVRAM Driver User Device name
//
// Revision History:
//      17-Feb-99   MWagner  Created.
//
//--
#define K10_NVRAM_USER_DEVICE_NAME      K10_USER_DEVICE_PREFIX  K10_NVRAM_ABBREV
//.End                                                                        

// End K10_NVRAM Names

           
//++
//
//  IOCTLS
//
//  Define the device type value.  Note that values used by Microsoft
//  Corporation are in the range 0-32767, and 32768-65535 are reserved for use
//  by customers.
//--
#define     FILE_DEVICE_K10_NVRAM             0x0000AF00

//++
//  Define IOCTL index values.  Note that function codes 0-2047 are reserved
//  for Microsoft Corporation, and 2048-4095 are reserved for customers.
//--
#define     IOCTL_INDEX_K10_NVRAM_BASE        0x1200

//++
// Enum:
//      K10_NVRAM_IOCTL_OPERATION
//
// Description:
//      The supported operations. 
//
// Revision History:
//      11/13/00   MWagner    Created. 
//
//--
typedef enum _K10_NVRAM_IOCTL_OPERATION
{
        K10_NVRAM_IOCTL_OPERATION_INVALID  = 0,
        K10_NVRAM_IOCTL_OPERATION_READ_BYTES,
        K10_NVRAM_IOCTL_OPERATION_WRITE_BYTES,
        K10_NVRAM_IOCTL_OPERATION_SECTION_INFO,
        K10_NVRAM_IOCTL_OPERATION_MAX
} K10_NVRAM_IOCTL_OPERATION, *PK10_NVRAM_IOCTL_OPERATION;
//.End                                                                        


//++
// Finally, the IOCTLS
//--
#define     K10_NVRAM_BUILD_IOCTL( _x_ )      EMCPAL_IOCTL_CTL_CODE( FILE_DEVICE_K10_NVRAM,  \
                                                  (IOCTL_INDEX_K10_NVRAM_BASE + _x_),  \
                                                  EMCPAL_IOCTL_METHOD_BUFFERED,  \
                                                  EMCPAL_IOCTL_FILE_ANY_ACCESS )

#define IOCTL_K10_NVRAM_READ_BYTES      \
        K10_NVRAM_BUILD_IOCTL(K10_NVRAM_IOCTL_OPERATION_READ_BYTES)
#define IOCTL_K10_NVRAM_WRITE_BYTES     \
        K10_NVRAM_BUILD_IOCTL(K10_NVRAM_IOCTL_OPERATION_WRITE_BYTES)
#define IOCTL_K10_NVRAM_SECTION_INFO    \
        K10_NVRAM_BUILD_IOCTL(K10_NVRAM_IOCTL_OPERATION_SECTION_INFO)
//  End IOCTLS


//++
// IOCTL Buffers
//--

//++
// Type:
//      K10_NVRAM_READ_BYTES_IN_BUFFER
//
// Description:
//      The buffer passed in to an IOCTL_K10_NVRAM_READ_BYTES request
//
// Members:
//      rbiVersion  always set to K10_NVRAM_CURRENT_VERSION
//      rbiOffset   the Offset from the beginning of K10_NVRAM
//      rbiLength   the number of bytes to read
//
// Revision History:
//      11/13/00   MWagner    Created.
//
//--
typedef struct _K10_NVRAM_READ_BYTES_IN_BUFFER
{
    ULONG rbiVersion;
    ULONG rbiOffset;
    ULONG rbiLength;

} K10_NVRAM_READ_BYTES_IN_BUFFER, *PK10_NVRAM_READ_BYTES_IN_BUFFER;
//.End                                                                        

//++
// Type:
//      K10_NVRAM_READ_BYTES_OUT_BUFFER
//
// Description:
//      The buffer passed back in an IOCTL_K10_NVRAM_READ_BYTES response
//      
//      The size of the output buffer should be at least sizeof(K10_NVRAM_READ_BYTES_IN_BUFFER) 
//      plus the "rbiLength" specified in the input buffer minus one byte.
//
// Members:
//      rboBytesRead the number of bytes read
//      rboBytes     the data read
//
// Revision History:
//      11/13/00   MWagner    Created.
//
//--
typedef struct _K10_NVRAM_READ_BYTES_OUT_BUFFER
{
    ULONG  rboBytesRead;
    UCHAR  rboBytes[1];
} K10_NVRAM_READ_BYTES_OUT_BUFFER, *PK10_NVRAM_READ_BYTES_OUT_BUFFER;
//.End                                                                        


//++
// Type:
//      K10_NVRAM_WRITE_BYTES_IN_BUFFER
//
// Description:
//      The buffer passed in to an IOCTL_K10_NVRAM_WRITE_BYTES request
//      
//      The size of the input buffer should be at least sizeof(K10_NVRAM_WRITE_BYTES_IN_BUFFER) 
//      plus wbiLength minus one byte.
//
// Members:
//      wbiVersion  always set to K10_NVRAM_CURRENT_VERSION
//      wbiOffset   the Offset from the beginning of K10_NVRAM
//      wbiLength   the number of bytes to write
//      wbiBytes    the data to be written
//
// Revision History:
//      11/13/00   MWagner    Created.
//
//--
typedef struct _K10_NVRAM_WRITE_BYTES_IN_BUFFER
{
    ULONG wbiVersion;
    ULONG wbiOffset;
    ULONG wbiLength;
    UCHAR wbiBytes[1];
} K10_NVRAM_WRITE_BYTES_IN_BUFFER, *PK10_NVRAM_WRITE_BYTES_IN_BUFFER;
//.End                                                                        


//++
// Type:
//      K10_NVRAM_WRITE_BYTES_OUT_BUFFER
//
// Description:
//      The buffer passed back in an IOCTL_K10_NVRAM_WRITE_BYTES response
//      
//
// Members:
//      wbiBytesWritten  the number of bytes written
//
// Revision History:
//      11/13/00   MWagner    Created.
//
//--
typedef struct _K10_NVRAM_WRITE_BYTES_OUT_BUFFER
{
    ULONG wboBytesWritten;
} K10_NVRAM_WRITE_BYTES_OUT_BUFFER, *PK10_NVRAM_WRITE_BYTES_OUT_BUFFER;


//.End                                                                        
#endif //  _K10_NVRAM_H_

// End K10NvRam.h
