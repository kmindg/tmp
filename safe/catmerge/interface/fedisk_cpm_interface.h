//**********************************************************************
//.++
//  COMPANY CONFIDENTIAL:
//     Copyright (C) EMC Corporation, 2001
//     All rights reserved.
//     Licensed material - Property of EMC Corporation.
//.--
//**********************************************************************

//**********************************************************************
//.++
// FILE NAME:
//      fedisk_cpm_interface.h
//
// DESCRIPTION:
//      This module contains structure definitions for IOCTLs
//      supported by the FEDisk driver that are called by Cpm.
//
// REVISION HISTORY:
//      24-Aug-04  majmera  Byte aligned the FEDISK_DEVICE_IDENTIFIER. 112785.
//      26-Jul-04  DScott   Changes for HDS/Shark support.
//      09-Jun-04  hchen    Added device spec flag
//                          FEDISK_DEVICE_SPEC_FLAG_SEARCH_SANCOPY_LITE.
//      12-Aug-03  MHolt    Add OriginatorId to dev spec for FAR support.
//      12-Mar-03  MHolt    Added "Verbose Verify" option (DIMS 84099);
//                          removed FEDISK_PORT_SPEC_DEFAULT in favor of
//                          FEDISK_DEVICE_SPEC_FLAG_SEARCH_SINGLE_PORT flag.
//      12-Mar-03  MHolt    Created (from FEDiskIoctl.h)
//.--
//**********************************************************************

#ifndef _FEDISKCPMINTERFACEH_
#define _FEDISKCPMINTERFACEH_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "EmcPAL.h"
#include "k10defs.h"
#include "K10LayeredDefs.h"

#ifdef __cplusplus
}
#endif /* __cplusplus */

//**********************************************************************
//.++
// CONSTANT:
//      FEDISK_MAX_FILENAME_LENGTH
//
// DESCRIPTION:
//      The max length of a windows filename.
//
// REMARKS:
//      This is used for buffers to contain device object names
//
// REVISION HISTORY:
//      14-Jul-06    CMason    Created
//.--
//**********************************************************************

#define FEDISK_MAX_FILENAME_LENGTH 256

//**********************************************************************
//.++
// TYPE:
//      FEDISK_DEVICE_CONNECTION_TYPE
//
// DESCRIPTION:
//      An enumeration of the possible connection types that a device can
//      be created for.
//
// MEMBERS:
//      FEDISK_CONNECTION_TYPE_FC       The device was created on a FC adapter
//      FEDISK_CONNECTION_TYPE_ISCSI    The device was created on an iSCSI adapter
//
// REMARKS:
//      An enum is usually four bytes in size.  To fit it into one byte, we
//      create a similarly named enum and set its max value to the biggest
//      value that will fit in a byte.  Then we define the actual type to be
//      a byte in size.  This is done because information of this type will
//      be held in the CDescriptor and CDestination structures in Cpm admin,
//      and the space for it will come from the built-in pad bytes in each
//      structure, which are a limited resource.
//
// REVISION HISTORY:
//      14-Jun-06   CMason  Created.
//.--
//**********************************************************************

enum fedisk_device_connection_type
{
    FEDISK_CONNECTION_TYPE_FC = 1,
    FEDISK_CONNECTION_TYPE_ISCSI,
    // ...
    FEDISK_CONNECTION_TYPE_MAX = 0xff
};
typedef UCHAR FEDISK_DEVICE_CONNECTION_TYPE;

//**********************************************************************
//.++
// TYPE:
//      FEDISK_CREATE_RETURN_INFO
//
// MEMBERS:
//      Structure returned by FE_DISK_IOCTL_CREATE_DEVICE_OBJECT
//
// DESCRIPTION:
//      SectorCount         Number of sectors on newly created Front End Device
//      SectorSize          Sector size in bytes of newly created Front End Device
//      NameBuff            Buffer for name string of newly created Front End Device
//      ConnectionType      Specifies the connection type of the adapter chosen
//      bZeroFillSupported              Flag used to know if Zero fill is supported
//      bSkipMetaDataSupported     Flag used to know if metadata skipping is supported
//
// REMARKS:
//
// REVISION HISTORY:
//      29-May-08   RDuenez     bZeroFillSupported field added
//      09-Feb-04   DScott      Changed SectorCount to ULONGLONG to
//                              support LUNs >2TB in size (100583).
//      09-Jul-02   MHolt       Created.
//.--
//**********************************************************************

typedef struct _FEDISK_CREATE_RETURN_INFO
{
    ULONGLONG                       SectorCount;
    ULONG                           SectorSize;
    CCHAR                           NameBuff[FEDISK_MAX_FILENAME_LENGTH];
    FEDISK_DEVICE_CONNECTION_TYPE   ConnectionType;
    BOOLEAN                         bZeroFillSupported;
    BOOLEAN                         bSkipMetaDataSupported;

} FEDISK_CREATE_RETURN_INFO, *PFEDISK_CREATE_RETURN_INFO;

//**********************************************************************
//.++
// TYPE:
//      FEDISK_DEVICE_ID_TYPE
//
// DESCRIPTION:
//      An enumeration of the possible ways that a device identifier
//      can be interpreted.
//
// MEMBERS:
//      FEDISK_IDTYPE_LUN_WWN           The device is identified by LUN WWN.
//      FEDISK_IDTYPE_FC_PORT_WWN_LUN   The device is identified by
//                                      fibrechannel port WWN and LU number.
//
// REMARKS:
//      An enum is usually four bytes in size.  To fit it into one byte, we
//      create a similarly named enum and set its max value to the biggest
//      value that will fit in a byte.  Then we define the actual type to be
//      a byte in size.  This is done because information of this type will
//      be held in the CDescriptor and CDestination structures in Cpm admin,
//      and the space for it will come from the built-in pad bytes in each
//      structure, which are a limited resource.
//
// REVISION HISTORY:
//      26-Jul-04   DScott  Created.
//.--
//**********************************************************************

enum fedisk_device_id_type
{
    FEDISK_IDTYPE_LUN_WWN = 0,
    FEDISK_IDTYPE_FC_PORT_WWN_LUN,
    // ...
    FEDISK_IDTYPE_MAX = 0xff
};
typedef UCHAR FEDISK_DEVICE_ID_TYPE;

//**********************************************************************
//.++
// CONSTANT:
//      FEDISK_LUN_WWN_LENGTH
//
// DESCRIPTION:
//      The length of a LUN world-wide name.
//
// REMARKS:
//      FEDisk and Cpm will need a pretty substantial overhaul to change this
//      value.
//
// REVISION HISTORY:
//      27-Jul-04    MHolt    Created
//.--
//**********************************************************************

#define FEDISK_LUN_WWN_LENGTH       (16)

//**********************************************************************
//.++
// TYPE:
//      FEDISK_DEVICE_IDENTIFIER
//
// DESCRIPTION:
//      A container for the information that uniquely identifies a device.
//
// MEMBERS:
//      PortIdLUN       The fibrechannel WWN of the port through which the
//                      device is accessed and a logical unit number.
//      LunWWN          The LUN WWN of the device as reported by Inquiry EVPD
//                      page 0x83.
//
// REMARKS:
//      This size of this structure must be sixteen bytes.  It is being used
//      to replace a sixteen byte structure (K10_WWID) in CPM's descriptor
//      database.  Any change to the size of this structure changes the
//      layout of the database.
//
// REVISION HISTORY:
//      24-Aug-04   majmera Byte aligned the structure. 112785.
//      26-Jul-04   DScott  Created.
//.--
//**********************************************************************

#pragma pack(push, FEDISK_DEVICE_IDENTIFIER, 1)
typedef union
{
    struct {
        K10_WWN     FCPortWWN;
        ULONGLONG   Lun;
    } PortIdLUN;

    K10_WWID   LunWWN;
} FEDISK_DEVICE_IDENTIFIER, *PFEDISK_DEVICE_IDENTIFIER;
#pragma pack(pop, FEDISK_DEVICE_IDENTIFIER)

//**********************************************************************
//.++
// CONSTANT:
//      FEDISK_DEVICE_SPEC_FLAG_VERBOSE_VERIFY
//
// DESCRIPTION:
//      Used to specify "verbose" option during VERIFY ioctl
//
// REMARKS:
//
// REVISION HISTORY:
//      27-Feb-03    MHolt    Created
//.--
//**********************************************************************

#define FEDISK_DEVICE_SPEC_FLAG_VERBOSE_VERIFY          (0x00000001)

//**********************************************************************
//.++
// CONSTANT:
//      FEDISK_DEVICE_SPEC_FLAG_SEARCH_SINGLE_PORT
//
// DESCRIPTION:
//      Used to specify particular port to search.
//
// REMARKS:
//
// REVISION HISTORY:
//      11-Mar-03    MHolt    Created
//.--
//**********************************************************************

#define FEDISK_DEVICE_SPEC_FLAG_SEARCH_SINGLE_PORT      (0x00000002)

//**********************************************************************
//.++
// CONSTANT:
//      FEDISK_DEVICE_SPEC_FLAG_SEARCH_SANCOPY_LITE
//
// DESCRIPTION:
//      Used to specify SANCOPY Lite search.
//
// REMARKS:
//
// REVISION HISTORY:
//      09-June-04    hchen    Created
//.--
//**********************************************************************

#define FEDISK_DEVICE_SPEC_FLAG_SEARCH_SANCOPY_LITE     (0x00000004)

//**********************************************************************
//.++
// TYPE:
//      FEDISK_DEVICE_CONNECTION_TYPE_PREFERENCE
//
// DESCRIPTION:
//      An enumeration of the connection type preferences that FEDisk
//      should use to sort adapters.
//
// MEMBERS:
//      FEDISK_CONNECTION_TYPE_PREFERENCE_INVALID_MIN   Used as a border for the valid prefs
//      FEDISK_CONNECTION_TYPE_PREFERENCE_NONE          No connection type preference
//      FEDISK_CONNECTION_TYPE_PREFERENCE_FC_ONLY       Use FC only
//      FEDISK_CONNECTION_TYPE_PREFERENCE_ISCSI_ONLY    Use iSCSI only
//      FEDISK_CONNECTION_TYPE_PREFERENCE_FC_FIRST      Prefer FC over iSCSI
//      FEDISK_CONNECTION_TYPE_PREFERENCE_ISCSI_FIRST   Prefer iSCSI over FC
//      FEDISK_CONNECTION_TYPE_PREFERENCE_INVALID_MAX   Used as a border for the valid prefs
//      FEDISK_CONNECTION_TYPE_PREFERENCE_BOUNDARY      Used to ensure fitting within 1 byte
//
// REMARKS:
//      An enum is usually four bytes in size.  To fit it into one byte, we
//      create a similarly named enum and set its max value to the biggest
//      value that will fit in a byte.  Then we define the actual type to be
//      a byte in size.  This is done because information of this type will
//      be held in the CDescriptor and CDestination structures in Cpm admin,
//      and the space for it will come from the built-in pad bytes in each
//      structure, which are a limited resource.
//
// REVISION HISTORY:
//      17-Sep-09   patannaa  Modified the names of MIN & MAX values. DIMS 236253
//      14-Jun-06   CMason  Created.
//.--
//**********************************************************************

enum fedisk_device_connection_type_preference
{
    FEDISK_CONNECTION_TYPE_PREFERENCE_INVALID_MIN,
    // ...
    FEDISK_CONNECTION_TYPE_PREFERENCE_NONE,
    FEDISK_CONNECTION_TYPE_PREFERENCE_FC_ONLY,
    FEDISK_CONNECTION_TYPE_PREFERENCE_ISCSI_ONLY,
    FEDISK_CONNECTION_TYPE_PREFERENCE_FC_FIRST,
    FEDISK_CONNECTION_TYPE_PREFERENCE_ISCSI_FIRST,
    // ...
    FEDISK_CONNECTION_TYPE_PREFERENCE_INVALID_MAX,
    FEDISK_CONNECTION_TYPE_PREFERENCE_BOUNDARY = 0xff
};
typedef UCHAR FEDISK_DEVICE_CONNECTION_TYPE_PREFERENCE;

//**********************************************************************
//.++
// TYPE:
//      FEDISK_DEVICE_SPEC
//
// MEMBERS:
//      Front End Device Specifcier
//
// DESCRIPTION:
//      DeviceIdInfo                Identifying information for device.
//      DeviceIdType                Tells whether device ID info is LUN WWN or FC port
//                                  WWN and LUN.
//      PortTag                     Port specifier: either index of port to use,
//                                  or PORT_SPEC_DEFAULT (see above). If port index
//                                  is specifed, FEDisk will search ONLY that port;
//                                  otherwise FEDisk will search all ports in order,
//                                  from least-used to most-used.
//      SearchFlags                 Search options.
//      OriginatorID                Identifies who sent the request so that we can
//                                  send FAR requests to FAR ports, and SANCopy requests
//                                  to all other front end ports.
//      Timeout                     optional timeout to be supplied. If not, value should
//                                  be set to FEDISK_NO_TIMEOUT
//      ConnectionTypePreference    Determines which types of adapters to search on.
//      bUseReservationBypass       If FEDisk finds a SCSI reservation, FEDisk will try to 
//                                  read through it ONLY if this value is TRUE.
//
// REMARKS:
//      NOTE : this is currently only used internally in FEDisk. It is not yet
//      used in the CPM->FEDisk interface. Verify & Create still pass ONLY the
//      WWID.  This structure will be used for those ioctls when Port Selection
//      is implemented at the CPM level.
//
// REVISION HISTORY:
//      21-Sep-06   LVidal  Add bUseReservationBypass for SCSI Reservation
//                          bypass functionality support.
//      26-Jul-04   DScott  Replace DeviceWWID with DeviceIdInfo and add
//                          DeviceIdType (port ID/LUN support).
//      12-Aug-03   MHolt   Add OriginatorId for FAR support.
//      27-Feb-03   MHolt   Add SearchFlags field Verbose option (84099).
//      14-May-02   MHolt   Created.
//.--
//**********************************************************************

typedef struct _FEDISK_DEVICE_SPEC
{
    FEDISK_DEVICE_IDENTIFIER                    DeviceIdInfo;
    FEDISK_DEVICE_ID_TYPE                       DeviceIdType;
    ULONG                                       PortTag;
    ULONG                                       SearchFlags;
    K10_LOGICAL_OBJECT_OWNER                    OriginatorId;
    EMCPAL_LARGE_INTEGER                               Timeout;
    FEDISK_DEVICE_CONNECTION_TYPE_PREFERENCE    ConnectionTypePreference;
    BOOLEAN                                     bUseReservationBypass;

} FEDISK_DEVICE_SPEC, *PFEDISK_DEVICE_SPEC;

//**********************************************************************
//.++
// CONSTANT:
//      FEDISK_SENSE_BUFFER_SIZE
//
// DESCRIPTION:
//      Size of request sense buffer in bytes. Increased from NT
//      default to allow for ASC/ASCQ info.
//
// REVISION HISTORY:
//      14-Jul-00   MHolt       Created.
//.--
//**********************************************************************

#define FEDISK_SENSE_BUFFER_SIZE        (32)

//**********************************************************************
//.++
// TYPE:
//      FEDISK_SCSI_PASSTHRU
//
// MEMBERS:
//      Front End Device Specifcier
//
// DESCRIPTION:
//      Cdb               IN; SCSI CDB built by client driver
//      CdbLength         IN; Length of Cdb
//      pData             IN; Pointer to an MDL describing the data buffer for 
//                            read/write operations (NULL otherwise)
//      DataLength        IN; Length of data buffer
//      Timeout           IN; Time limit to wait for IO completion from the target
//      bDataOut          IN; True for write operations
//      bSkipMetaData     IN; True if Splitter indicates that meta data is to be skipped for this IO
//      DataType          IN; How to interpret pData (e.g. MDL, SGL, etc.)
//      status            OUT; Enum of statuses that FEDisk will return to the splitter driver
//      ScsiStatus        OUT; SCSI status returned
//      SenseInfo         OUT; Sense information returned
//      SenseInfoLength   OUT; Length of SenseInfo
// REMARKS:
//      None
//
// REVISION HISTORY:
//      18-Jul-07   majmera   Created.
//.--
//**********************************************************************

typedef enum
{
    FEDISK_DATATYPE_VIRT_ADDR,
    FEDISK_DATATYPE_MDL,
    FEDISK_DATATYPE_SGL_PHYS,
    FEDISK_DATATYPE_SGL_VIRT
} FEDISK_DATATYPE;

typedef struct
{
    // Input
    UCHAR                                       Cdb[16];
    UCHAR                                       CdbLength;
    PVOID                                       pData;
    ULONG                                       DataLength;
    ULONG                                       Timeout;
    BOOLEAN                                     bDataOut;
    BOOLEAN                                     bSkipMetaData;
    FEDISK_DATATYPE                             DataType;

    // Output
    UCHAR                                       status;
    UCHAR                                       ScsiStatus;
    UCHAR                                       SenseInfo[FEDISK_SENSE_BUFFER_SIZE];
    UCHAR                                       SenseInfoLength;
} FEDISK_SCSI_PASSTHRU, *PFEDISK_SCSI_PASSTHRU;

//**********************************************************************
//.++
// TYPE:
//      FEDISK_SCSI_PASSTHRU_STATUS
//
// DESCRIPTION:
//      Set of statuses that FEDisk will return to the Splitter
//      driver on each SCSI Passthru buffer
//      
// REMARKS:
//      None
//
// REVISION HISTORY:
//      28-Aug-07   LVidal   Created.
//.--
//**********************************************************************

typedef enum 
{
    FEDISK_PASSTHRU_STATUS_SUCCESS,
    FEDISK_PASSTHRU_STATUS_INTERNAL_ERROR,
    FEDISK_PASSTHRU_STATUS_TRANSPORT_ERROR,
    FEDISK_PASSTHRU_STATUS_DATA_OVERRUN,
    FEDISK_PASSTHRU_STATUS_TARGET_ERROR
} FEDISK_SCSI_PASSTHRU_STATUS;

//**********************************************************************
//.++
// CONSTANT:
//      FEDISK_NO_TIMEOUT
//
// DESCRIPTION:
//      FEDISK_DEVICE_SPEC.Timeout is set to this to indicate no timeout.
//
// REMARKS:
//
// REVISION HISTORY:
//      17-Aug-05   CMason      Created
//.--
//**********************************************************************

#define FEDISK_NO_TIMEOUT       (0)

#endif
