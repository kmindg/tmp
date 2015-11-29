#ifndef __NVRAM_STATIC_LIB__
#define __NVRAM_STATIC_LIB__

/***************************************************************************
 *  Copyright (C)  EMC Corporation 1992-2008
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. 
 ***************************************************************************/

/***************************************************************************
 * NvRamStaticLib.h
 ***************************************************************************
 *
 * File Description:
 *      Function prototypes for the NVRAM functions exported as a statically
 *      linked library.  Use of this interface is an exception case built
 *      specifically for the spid driver's crash dump instantiation.
 *      All other components should use the appropriate kernel or user mode
 *      interface.
 *
 * Author:
 *  Joseph Ash
 *  
 * Revision History:
 *  October 14, 2008 - JSA - Created Inital Version
 *
 ***************************************************************************/
CSX_CDECLS_BEGIN

#include "k10defs.h"

/***************************************************************************
 * NVRAM_ACCESS_METHOD
 ***************************************************************************
 *
 * Various methods used to access NVRAM
 ***************************************************************************/
typedef enum _NVRAM_ACCESS_METHOD
{
    NVRAM_ACCESS_INVALID,
    NVRAM_ACCESS_REGISTRY,
    NVRAM_ACCESS_IPMI,
    NVRAM_ACCESS_SSD
} NVRAM_ACCESS_METHOD;

//simulator nvram is located on ssd1
#define SSD_NVRAM_NAME               "/dev/ssd1"
//block size for performing io
#define SSD_NVRAM_BLOCK_SIZE         512
//block io size
#define SSD_NVRAM_MAX_IO_SIZE           32768
/*
SSD1 Partition for NVRAM on Simulator
[Start of File System ...  End of File System] [1Mb Unused Buffer Space] [NVRAM (~500Kb in size)] [500Kb Unused Buffer Space]

Addresses in blocks of 1024 bytes:

Start of File System:        0
End of File System:          129024  (-2048 blocks from end)
Start of NVRAM:              130048  (-1024 blocks from end)
End of Partition:            131072
*/

#define SSD_NVRAM_DISK_OFFSET               0x07F00000



/***************************************************************************
 * NvRamStaticLibGetAccessMethod()
 ***************************************************************************
 *
 * Description:
 *      NvRamStaticLibGetAccessMethod() Determines the method that should be
 *      used to access NVRAM
 *
 * Arguments:
 *      None.
 *
 * Return Value:
 *      NVRAM_ACCESS_METHOD ... yeah, what it sounds like
 *
 * Revision History:
 *      10/14/08   Joe Ash    Created.
 *
 ***************************************************************************/
NVRAM_ACCESS_METHOD
NvRamStaticLibGetAccessMethod(void);


/***************************************************************************
 * NvRamStaticLibSSDRead()
 ***************************************************************************
 *
 * Description:
 *      NvRamStaticLibSSDRead() Reads UlLength Bytes from the UlOffset from
 *      ssd1 partition using block IO
 *
 * Arguments:
 *      UlOffset    The offset within Non Volatile RAM (in ssd1).
 *      UlLength    Size of area to be read.
 *      PBytes      Buffer to fill in.
 *
 * Return Value:
 *      status.
 *
 ***************************************************************************/
EMCPAL_STATUS
NvRamStaticLibSSDRead(IN  ULONG   readOffset,
                      IN  ULONG   readLength,
                      OUT PUCHAR  readBuffer);


/***************************************************************************
 * NvRamStaticLibWriteBytes()
 ***************************************************************************
 *
 * Description:
 *      NvRamStaticLibSSDWrite() Writes UlLength Bytes to the UlOffset in
 *      ssd1 partition using block IO
 *
 * Arguments:
 *      UlOffset    The offset within Non Volatile RAM (in ssd1).
 *      UlLength    Size of area to be written.
 *      PBytes      Data to be written.
 *
 * Return Value:
 *      status.
 *
 ***************************************************************************/
EMCPAL_STATUS
NvRamStaticLibSSDWrite(IN  ULONG   writeOffset,
                       IN  ULONG   writeLength,
                       OUT PUCHAR  writeBuffer);


CSX_CDECLS_END

#endif
/***************************************************************************
 * END NvRamStaticLib.h
 ***************************************************************************/
