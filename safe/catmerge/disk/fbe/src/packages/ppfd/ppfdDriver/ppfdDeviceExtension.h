#ifndef PPFD_DEVEXT_H
#define PPFD_DEVEXT_H

/***************************************************************************
 Copyright (C)  EMC Corporation 2008-2009
 All rights reserved.
 Licensed material - property of EMC Corporation. * All rights reserved.


 File Name:
      ppfdDeviceExtension.h

 Exported:
      PPFD_DEVICE_EXTENSION

 Revision History:

***************************************************************************/

//
// Include Files
//
#include "k10ntddk.h"
#include "EmcPAL_Irp.h"


//
// Type:
//      PPFD_DEVICE_EXTENSION
//
// Description:
//
// Revision History:

//
//

typedef struct{
    USHORT              Size;
    USHORT              Version;
    ULONG               DeviceFlags;
} PPFD_COMMON_EXTENSION,*PPPFD_COMMON_EXTENSION;

// Make sure the first 3 entries are the same "common" data types
// as defined above for Common extension.
typedef struct _PPFD_DEVICE_EXTENSION
{
    PPFD_COMMON_EXTENSION CommonDevExt;
    PEMCPAL_DEVICE_OBJECT      DeviceObject;
    PEMCPAL_DEVICE_OBJECT      BE0ScsiPortDeviceObject;
} PPFD_DEVICE_EXTENSION, *PPPFD_DEVICE_EXTENSION;

typedef struct _PPFD_DISK_DEVICE_EXTENSION
{
    PPFD_COMMON_EXTENSION   CommonDevExt;
    PEMCPAL_DEVICE_OBJECT      physicalDeviceObject;
    PEMCPAL_DEVICE_OBJECT      targetDiskDeviceObject;
} PPFD_DISK_DEVICE_EXTENSION,*PPPFD_DISK_DEVICE_EXTENSION;

 
#define     PPFDPORT_DEVICE_NAME         "\\Device\\PPFDPort0"

//
// DeviceFlags bit definitions (see PPFD_COMMON_EXTENSION)
// 
#define PPFD_PORT_DEVICE_OBJECT         0x00000004
#define PPFD_DISK_DEVICE_OBJECT         0x00000002
#define PPFD_PNP_DISK_DEVICE_OBJECT     0x00000001
#define PPFD_DEVICE_FLAGS_OBJ_MASK      0x0000000F

//.End                                                                        


// End ppfdDrvDeviceExtension.h

#endif //PPFD_DEVEXT_H
