/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    class.h

Abstract:

    These are the structures and defines that are used in the
    SCSI class drivers.

Author:
Revision History:

--*/

#ifndef _SCSI_CLASS_
#define _SCSI_CLASS_

#include "k10ntddk.h"
#include "EmcPAL_DriverShell.h"
#include <ntdddisk.h>
//#include <ntddcdrm.h>
//#include <ntddtape.h>
#include "ntddscsi.h"
#include <stdio.h>
#include "csx_ext_p_scsi.h"
#include "cpd_interface.h"

#define MAXIMUM_RETRIES 4

struct _CLASS_INIT_DATA;

typedef
VOID
(*PCLASS_ERROR) (
    IN PEMCPAL_DEVICE_OBJECT DeviceObject,
    IN PCPD_SCSI_REQ_BLK Srb,
    IN OUT EMCPAL_STATUS *Status,
    IN OUT BOOLEAN *Retry
    );

typedef
BOOLEAN
(*PCLASS_DEVICE_CALLBACK) (
    IN PINQUIRYDATA
    );

typedef
EMCPAL_STATUS
(*PCLASS_READ_WRITE) (
    IN PEMCPAL_DEVICE_OBJECT DeviceObject,
    IN PEMCPAL_IRP Irp
    );

typedef
BOOLEAN
(*PCLASS_FIND_DEVICES) (
    IN PEMCPAL_NATIVE_DRIVER_OBJECT DriverObject,
    IN csx_string_t RegistryPath,
    IN struct _CLASS_INIT_DATA *InitializationData,
    IN PEMCPAL_FILE_OBJECT PortFileObject,
    IN ULONG PortNumber
    );

typedef
EMCPAL_STATUS
(*PCLASS_DEVICE_CONTROL) (
    IN PEMCPAL_DEVICE_OBJECT DeviceObject,
    IN PEMCPAL_IRP Irp
    );

typedef
EMCPAL_STATUS
(*PCLASS_SHUTDOWN_FLUSH) (
    IN PEMCPAL_DEVICE_OBJECT DeviceObject,
    IN PEMCPAL_IRP Irp
    );

typedef
EMCPAL_STATUS
(*PCLASS_CREATE_CLOSE) (
    IN PEMCPAL_DEVICE_OBJECT DeviceObject,
    IN PEMCPAL_IRP Irp
    );

typedef struct _CLASS_INIT_DATA {

    //
    // This structure size - version checking.
    //

    ULONG InitializationDataSize;

    //
    // Bytes needed by the class driver
    // for it's extension.
    //

    ULONG DeviceExtensionSize;

    EMCPAL_IRP_DEVICE_TYPE DeviceType;

    //
    // Device Characteristics flags
    //  eg.:
    //
    //  FILE_REMOVABLE_MEDIA
    //  FILE_READ_ONLY_DEVICE
    //  FILE_FLOPPY_DISKETTE
    //  FILE_WRITE_ONCE_MEDIA
    //  FILE_REMOTE_DEVICE
    //  FILE_DEVICE_IS_MOUNTED
    //  FILE_VIRTUAL_VOLUME
    //

    ULONG DeviceCharacteristics;

    //
    // Device-specific driver routines
    //

    PCLASS_ERROR           ClassError;
    PCLASS_READ_WRITE      ClassReadWriteVerification;
    PCLASS_DEVICE_CALLBACK ClassFindDeviceCallBack;
    PCLASS_FIND_DEVICES    ClassFindDevices;
    PCLASS_DEVICE_CONTROL  ClassDeviceControl;
    PCLASS_SHUTDOWN_FLUSH  ClassShutdownFlush;
    PCLASS_CREATE_CLOSE    ClassCreateClose;
    EMCPAL_RAW_START_IO_HANDLER        ClassStartIo;

} CLASS_INIT_DATA, *PCLASS_INIT_DATA;

typedef struct _DEVICE_EXTENSION {

    //
    // Back pointer to device object
    //

    PEMCPAL_DEVICE_OBJECT DeviceObject;

    //
    // Pointer to port device object
    //

    PEMCPAL_DEVICE_OBJECT PortDeviceObject;

    //
    // Length of partition in bytes
    //

    EMCPAL_LARGE_INTEGER PartitionLength;

    //
    // Number of bytes before start of partition
    //

    EMCPAL_LARGE_INTEGER StartingOffset;

    //
    // Bytes to skew all requests, since DM Driver has been placed on an IDE drive.
    //

    ULONG DMByteSkew;

    //
    // Sectors to skew all requests.
    //

    ULONG DMSkew;

    //
    // Flag to indicate whether DM driver has been located on an IDE drive.
    //

    BOOLEAN DMActive;

    //
    // Class driver routines
    //

    PCLASS_ERROR ClassError;
    PCLASS_READ_WRITE ClassReadWriteVerification;
    PCLASS_FIND_DEVICES ClassFindDevices;
    PCLASS_DEVICE_CONTROL ClassDeviceControl;
    PCLASS_SHUTDOWN_FLUSH ClassShutdownFlush;
    PCLASS_CREATE_CLOSE ClassCreateClose;
    EMCPAL_RAW_START_IO_HANDLER     ClassStartIo;

    //
    // SCSI port driver capabilities
    //

    CPD_S_IO_CAPABILITIES *PortCapabilities;

    //
    // Buffer for drive parameters returned in IO device control.
    //

    PDISK_GEOMETRY DiskGeometry;

    //
    // Back pointer to device object of physical device
    // If this is equal to DeviceObject then this is the physical device
    //

    PEMCPAL_DEVICE_OBJECT PhysicalDevice;

    //
    // Request Sense Buffer
    //

    PSENSE_DATA SenseData;

    //
    // Request timeout in seconds;
    //

    ULONG TimeOutValue;

    //
    // System device number
    //

    ULONG DeviceNumber;

    //
    // Add default Srb Flags.
    //

    ULONG SrbFlags;

    //
    // Total number of SCSI protocol errors on the device.
    //

    ULONG ErrorCount;

    //
    // Spinlock for split requests
    //

    EMCPAL_SPINLOCK SplitRequestSpinLock;

    //
    // Lookaside listhead for srbs.
    //

#ifndef UMODE_ENV
    EMCPAL_LOOKASIDE SrbLookasideListHead;
#endif

    //
    // Lock count for removable media.
    //

    LONG LockCount;

    //
    // Scsi port number
    //

    UCHAR PortNumber;

    //
    // SCSI path id
    //

    UCHAR PathId;

    //
    // SCSI bus target id
    //

    UCHAR TargetId;

    //
    // SCSI bus logical unit number
    //

    UCHAR Lun;

    //
    // Log2 of sector size
    //

    UCHAR SectorShift;
    UCHAR   ReservedByte;

    //
    // Values for the flags are below.
    //

    USHORT  DeviceFlags;

    //
    // Pointer to the media change event.  If MediaChange is TRUE, then
    // this event is to be signaled on all media change check conditions.
    // If MediaChangeNoMedia is true then we've already determined that there
    // isn't any media in the drive so we shouldn't signal the event again
    //

    PEMCPAL_RENDEZVOUS_EVENT MediaChangeEvent;
    HANDLE  MediaChangeEventHandle;
    BOOLEAN MediaChangeNoMedia;

    //
    // Count of media changes.  This field is only valid for the root partition
    // (ie. if PhysicalDevice == NULL).
    //

    ULONG MediaChangeCount;

	//TCD expects this to be 16 byte aligned
    CSX_ALIGN_16 PVOID pad;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//TCD expects this to be 16 byte aligned
CSX_ASSERT_AT_COMPILE_TIME_GLOBAL_SCOPE((sizeof(DEVICE_EXTENSION) & (sizeof(PVOIDPVOID) - 1)) == 0);

//
// Indicates that the device has write caching enabled.
//

#define DEV_WRITE_CACHE     0x00000001


//
// Build SCSI 1 or SCSI 2 CDBs
//

#define DEV_USE_SCSI1       0x00000002

//
// Indicates whether is is safe to send StartUnit commands
// to this device. It will only be off for some removeable devices.
//

#define DEV_SAFE_START_UNIT 0x00000004

//
// Indicates whether it is unsafe to send SCSIOP_MECHANISM_STATUS commands to
// this device.  Some devices don't like these 12 byte commands
//

#define DEV_NO_12BYTE_CDB 0x00000008

//
// Define context structure for asynchronous completions.
//

typedef struct _COMPLETION_CONTEXT {
    PEMCPAL_DEVICE_OBJECT DeviceObject;
    CPD_SCSI_REQ_BLK Srb;
}COMPLETION_CONTEXT, *PCOMPLETION_CONTEXT;

#define SCSIPORT_API

//
// Class dll routines called by class drivers
//

SCSIPORT_API
ULONG
ScsiClassInitialize(
    IN  PVOID            Argument1,
    IN  PVOID            Argument2,
    IN  PCLASS_INIT_DATA InitializationData
    );

SCSIPORT_API
EMCPAL_STATUS
ScsiClassCreateDeviceObject(
    IN PEMCPAL_NATIVE_DRIVER_OBJECT DriverObject,
    IN csx_pchar_t                  ObjectNameBuffer,
    IN OPTIONAL PEMCPAL_DEVICE_OBJECT PhysicalDeviceObject,
    IN OUT PEMCPAL_DEVICE_OBJECT *DeviceObject,
    IN PCLASS_INIT_DATA InitializationData
    );

SCSIPORT_API
ULONG
ScsiClassFindUnclaimedDevices(
    IN PCLASS_INIT_DATA InitializationData,
    IN PSCSI_ADAPTER_BUS_INFO  AdapterInformation
    );

SCSIPORT_API
EMCPAL_STATUS
ScsiClassGetCapabilities(
    IN PEMCPAL_FILE_OBJECT PortFileObject,
    OUT CPD_S_IO_CAPABILITIES **pPortCapabilities
    );

SCSIPORT_API
EMCPAL_STATUS
ScsiClassGetInquiryData(
    IN PEMCPAL_FILE_OBJECT PortFileObject,
    IN PSCSI_ADAPTER_BUS_INFO *ConfigInfo
    );

SCSIPORT_API
EMCPAL_STATUS 
ScsiClassGetScsiAddress(
    IN PEMCPAL_FILE_OBJECT PortFileObject,
    IN CPD_SCSI_ADDRESS *pScsiAddress
    );

SCSIPORT_API
PEMCPAL_DEVICE_OBJECT 
ScsiClassGetIoctlDeviceObject(
    PEMCPAL_DEVICE_OBJECT pPortDeviceObjectDisk
    );

SCSIPORT_API
EMCPAL_STATUS
ScsiClassReadDriveCapacity(
    IN PEMCPAL_DEVICE_OBJECT DeviceObject
    );

SCSIPORT_API
VOID
ScsiClassReleaseQueue(
    IN PEMCPAL_DEVICE_OBJECT DeviceObject
    );

SCSIPORT_API
EMCPAL_STATUS
ScsiClassRemoveDevice(
    IN PEMCPAL_FILE_OBJECT PortFileObject,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun
    );

SCSIPORT_API
EMCPAL_STATUS
ScsiClassAsynchronousCompletion(
    PEMCPAL_DEVICE_OBJECT DeviceObject,
    PEMCPAL_IRP Irp,
    PVOID Context
    );

SCSIPORT_API
VOID
ScsiClassSplitRequest(
    IN PEMCPAL_DEVICE_OBJECT DeviceObject,
    IN PEMCPAL_IRP Irp,
    IN ULONG MaximumBytes
    );

SCSIPORT_API
EMCPAL_STATUS
ScsiClassDeviceControl(
    PEMCPAL_DEVICE_OBJECT DeviceObject,
    PEMCPAL_IRP Irp
    );

SCSIPORT_API
EMCPAL_STATUS
ScsiClassIoComplete(
    IN PEMCPAL_DEVICE_OBJECT DeviceObject,
    IN PEMCPAL_IRP Irp,
    IN PVOID Context
    );

SCSIPORT_API
EMCPAL_STATUS
ScsiClassCheckVerifyComplete(
    IN PEMCPAL_DEVICE_OBJECT DeviceObject,
    IN PEMCPAL_IRP Irp,
    IN PVOID Context
    );

SCSIPORT_API
EMCPAL_STATUS
ScsiClassIoCompleteAssociated(
    IN PEMCPAL_DEVICE_OBJECT DeviceObject,
    IN PEMCPAL_IRP Irp,
    IN PVOID Context
    );

SCSIPORT_API
BOOLEAN
ScsiClassInterpretSenseInfo(
    IN PEMCPAL_DEVICE_OBJECT DeviceObject,
    IN PCPD_SCSI_REQ_BLK Srb,
    IN UCHAR MajorFunctionCode,
    IN ULONG IoDeviceCode,
    IN ULONG RetryCount,
    OUT EMCPAL_STATUS *Status
    );

SCSIPORT_API
EMCPAL_STATUS
ScsiClassSendSrbSynchronous(
        PEMCPAL_DEVICE_OBJECT DeviceObject,
        PCPD_SCSI_REQ_BLK Srb,
        PVOID BufferAddress,
        ULONG BufferLength,
        BOOLEAN WriteToDevice
        );

SCSIPORT_API
EMCPAL_STATUS
ScsiClassSendSrbAsynchronous(
        PEMCPAL_DEVICE_OBJECT DeviceObject,
        PCPD_SCSI_REQ_BLK Srb,
        PEMCPAL_IRP Irp,
        PVOID BufferAddress,
        ULONG BufferLength,
        BOOLEAN WriteToDevice
        );

SCSIPORT_API
VOID
ScsiClassBuildRequest(
    PEMCPAL_DEVICE_OBJECT DeviceObject,
    PEMCPAL_IRP Irp
    );

SCSIPORT_API
ULONG
ScsiClassModeSense(
    IN PEMCPAL_DEVICE_OBJECT DeviceObject,
    IN PCHAR ModeSenseBuffer,
    IN ULONG Length,
    IN UCHAR PageMode
    );

SCSIPORT_API
BOOLEAN
ScsiClassModeSelect(
    IN PEMCPAL_DEVICE_OBJECT DeviceObject,
    IN PCHAR ModeSelectBuffer,
    IN ULONG Length,
    IN BOOLEAN SavePage
    );

SCSIPORT_API
PVOID
ScsiClassFindModePage(
    IN PCHAR ModeSenseBuffer,
    IN ULONG Length,
    IN UCHAR PageMode,
    IN BOOLEAN Use6Byte
    );

SCSIPORT_API
EMCPAL_STATUS
ScsiClassClaimDevice(
    IN PEMCPAL_DEVICE_OBJECT PortDeviceObject,
    IN PSCSI_INQUIRY_DATA LunInfo,
    IN BOOLEAN Release,
    OUT PEMCPAL_DEVICE_OBJECT *NewPortDeviceObject OPTIONAL
    );

SCSIPORT_API
EMCPAL_STATUS
ScsiClassInternalIoControl (
    IN PEMCPAL_DEVICE_OBJECT DeviceObject,
    IN PEMCPAL_IRP Irp
    );

SCSIPORT_API
VOID
ScsiClassInitializeSrbLookasideList(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN ULONG NumberElements
    );

SCSIPORT_API
ULONG
ScsiClassQueryTimeOutRegistryValue(
    IN csx_cstring_t RegistryPath
    );

#endif /* _SCSI_CLASS_ */
