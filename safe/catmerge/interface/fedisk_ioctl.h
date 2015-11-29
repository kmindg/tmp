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
//      fedisk_ioctl.h
//
// DESCRIPTION:
//      This module contains interface definitions for IOCTLs
//      supported by the FEDisk driver.
//
// REVISION HISTORY:
//      09-Feb-04  DScott   Changed SectorCount in FEDISK_CREATE_RETURN_INFO
//                          to ULONGLONG to support LUNs >2TB in size
//                          (DIMS 100583).
//      12-Mar-03  MHolt    Moved DeviceSpec to FEDiskIoctlExport.h
//                          Verbose Verify support (80499).
//      09-Jul-02  MHolt    Added FEDISK_CREATE_RETURN_INFO.
//      14-May-02  MHolt    Add FEDISK_DEVICE_SPEC for port selection
//      19-Sep-01  MHolt    Fixed some comments.
//      14-Jul-00  MHolt    Created.
//.--
//**********************************************************************

#ifndef _FEDISKIOCTLH_
#define _FEDISKIOCTLH_

//**********************************************************************
//.++
// CONSTANT:
//      FEDISK_DRIVER_NAME
//      FEDISK_DRIVER_SYMLINK_NAME
//      FEDISK_DRIVER_USER_NAME
//
// DESCRIPTION:
//      Names for FEDisk Driver object, symbolic link to it, and user
//      space link to it.
//
// REVISION HISTORY:
//      06-Jan-06   MAjmera     Fixed the driver name to the correct format.
//      18-Sep-01   MHolt       Created.
//.--
//**********************************************************************

#define FEDISK_DRIVER_NAME          ( "\\Device\\FEDriver0" )
#define FEDISK_DRIVER_SYMLINK_NAME  ( "\\??\\FEDriver0" )
#define FEDISK_DRIVER_USER_NAME     ( "FEDriver0" )

//**********************************************************************
//.++
// CONSTANT:
//      FILE_DEVICE_FEDISK
//
// DESCRIPTION:
//      Device type for FEDisk device objects and IOCTL base for
//      FEDisk driver ioctls.
//
// REMARKS:
//      This is basically arbitrary, but the high bit must be set
//      so as to not conflict with NT ioctls.
//
// REVISION HISTORY:
//      14-Jul-00   MHolt       Created.
//.--
//**********************************************************************

#define FILE_DEVICE_FEDISK  0x8807

//**********************************************************************
//.++
// CONSTANT:
//      FE_DISK_IOCTL_CREATE_DEVICE_OBJECT
//
// DESCRIPTION:
//      This ioctl instructs the FEDisk driver to search for all Front
//      End adapter ports for a Disk LUN with a particular WWID, create a
//      device object for it, and return the device name string of the
//      new device object
//
// INPUT:
//      K10_WWID        WWID of device to search for
//
// OUTPUT:
//      CCHAR[]         Name string of new device
//
// REVISION HISTORY:
//      14-Jul-00   MHolt       Created.
//.--
//**********************************************************************

#define FE_DISK_IOCTL_CREATE_DEVICE_OBJECT      EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_FEDISK, \
                                                         0x0001,             \
                                                         EMCPAL_IOCTL_METHOD_BUFFERED,    \
                                                         EMCPAL_IOCTL_FILE_ANY_ACCESS )

//**********************************************************************
//.++
// CONSTANT:
//      FE_DISK_IOCTL_REMOVE_DEVICE_OBJECT
//
// DESCRIPTION:
//      This ioctl instructs the FEDisk driver to remove the device
//      object of a Front End disk device.
//
// INPUT:
//      PDEVICEOBJECT   pointer to device object to remove
//
// OUTPUT:
//      none
//
// REVISION HISTORY:
//      14-Jul-00   MHolt       Created.
//.--
//**********************************************************************

#define FE_DISK_IOCTL_REMOVE_DEVICE_OBJECT      EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_FEDISK, \
                                                         0x0002,             \
                                                         EMCPAL_IOCTL_METHOD_BUFFERED,    \
                                                         EMCPAL_IOCTL_FILE_ANY_ACCESS )

//**********************************************************************
//.++
// CONSTANT:
//      FE_DISK_IOCTL_VERIFY_DEVICE_ACCESS
//
// DESCRIPTION:
//      This ioctl instructs the FEDisk driver to search for all Front
//      End adapter ports for a Disk LUN with a particular WWID and
//      verify that the device can be accessed.
//
// INPUT:
//      K10_WWID        WWID of device to search for
//
// OUTPUT:
//      none
//
// REVISION HISTORY:
//      14-Jul-00   MHolt       Created.
//.--
//**********************************************************************

#define FE_DISK_IOCTL_VERIFY_DEVICE_ACCESS      EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_FEDISK, \
                                                         0x0003,             \
                                                         EMCPAL_IOCTL_METHOD_BUFFERED,    \
                                                         EMCPAL_IOCTL_FILE_ANY_ACCESS )

//**********************************************************************
//.++
// CONSTANT:
//      FE_DISK_IOCTL_MODIFY_IO_COUNT
//
// DESCRIPTION:
//      This ioctl instructs the FEDisk driver to allocate or deallocate
//      request blocks used to perform I/O to Front End Disk devices.
//
// INPUT:
//      LONG    Number of Request blocks to alloc/dealloc;
//                - positive numbers == allocate
//                - negative numbers == deallocate
//
// OUTPUT:
//      none
//
// REVISION HISTORY:
//      14-Jul-00   MHolt       Created.
//.--
//**********************************************************************

#define FE_DISK_IOCTL_MODIFY_IO_COUNT           EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_FEDISK, \
                                                         0x0004,             \
                                                         EMCPAL_IOCTL_METHOD_BUFFERED,    \
                                                         EMCPAL_IOCTL_FILE_ANY_ACCESS )

//**********************************************************************
//.++
// CONSTANT:
//      FE_DISK_IOCTL_GET_MAX_XFER_LENGTH
//
// DESCRIPTION:
//      This ioctl instructs the FEDisk driver to return the size (in bytes)
//      of the largest I/O request that the FEDisk driver can process.
//
// INPUT:
//      none
//
// OUTPUT:
//      ULONG   Maximum Transfer Size in bytes
//
// REVISION HISTORY:
//      14-Jul-00   MHolt       Created.
//.--
//**********************************************************************

#define FE_DISK_IOCTL_GET_MAX_XFER_LENGTH       EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_FEDISK, \
                                                         0x0005,             \
                                                         EMCPAL_IOCTL_METHOD_BUFFERED,    \
                                                         EMCPAL_IOCTL_FILE_ANY_ACCESS )

//**********************************************************************
//.++
// CONSTANT:
//      FE_DISK_IOCTL_VERIFY_ZONE_PATH
//
// DESCRIPTION:
//
//      This IOCTL instructs FEDisk to attempt to establish an iSCSI
//      login session with the specified target device via the
//      specified front end port. This IOCTL is used to verify path
//      accessibility and authorization credentials.
//
// INPUT:
//      FEDISK_VERIFY_ZONE_PATH_IN_BUFF
//
// OUTPUT:
//      STATUS_SUCCESS
//      STATUS_NOT_FOUND
//      STATUS_INVALID_PARAMETER
//      FEDISK_STATUS_ALREADY_LOGGED_IN
//      FEDISK_STATUS_REMOTE_AUTHENTICATION_FAILED
//      FEDISK_STATUS_LOCAL_AUTHENTICATION_FAILED
//      FEDISK_STATUS_NO_REMOTE_PASSWORD_FOUND
//      FEDISK_STATUS_NO_LOCAL_PASSWORD_FOUND
//      FEDISK_STATUS_LOGIN_DEVICE_ERROR
//
// REVISION HISTORY:
//      26-Jul-04   MHolt       Created.
//.--
//**********************************************************************

#define FE_DISK_IOCTL_VERIFY_ZONE_PATH          EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_FEDISK, \
                                                         0x0006,             \
                                                         EMCPAL_IOCTL_METHOD_BUFFERED,    \
                                                         EMCPAL_IOCTL_FILE_ANY_ACCESS )

//**********************************************************************
//.++
// CONSTANT:
//      FEDISK_IOCTL_DEBUG
//
// DESCRIPTION:
//
//      This IOCTL is used to perfom FEDsik debug tasks 
//      that require an interface with user space. 
//
// INPUT:
//      None
//
// OUTPUT:
//      STATUS_SUCCESS     
//      STATUS_INVALID_PARAMETER
//
// REMARKS:
//      Currently it's being used only to instruct FEDisk to update  
//      trace level flags from the registry.  
//      Please update this section if current use changes!
//      
// REVISION HISTORY:
//      05-Mar-07   LVidal       Created.
//.--
//**********************************************************************

#define FEDISK_IOCTL_DEBUG          EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_FEDISK, \
                                          0x0007,                \
                                          EMCPAL_IOCTL_METHOD_BUFFERED,       \
                                          EMCPAL_IOCTL_FILE_ANY_ACCESS )

//**********************************************************************
//.++
// CONSTANT:
//      FE_DISK_IOCTL_SCSI_PASSTHRU
//
// DESCRIPTION:
//
//      This IOCTL is used to issue pre built SCSI CDBs to FEDisk.
//
// INPUT:
//      A pointer to a FEDISK_SCSI_PASSTHRU structure
//
// OUTPUT:
//      STATUS_PENDING     
//      STATUS_INVALID_BUFFER_SIZE
//      
// REMARKS:
//      
// REVISION HISTORY:
//      18-Jul-07   majmera       Created.
//.--
//**********************************************************************
#define FE_DISK_IOCTL_SCSI_PASSTHRU          EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_FEDISK, \
                                                      0x0008,             \
                                                      EMCPAL_IOCTL_METHOD_BUFFERED,    \
                                                      EMCPAL_IOCTL_FILE_ANY_ACCESS )

#endif
