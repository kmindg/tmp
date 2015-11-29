#ifndef EMCUTIL_DEVICE_H_
#define EMCUTIL_DEVICE_H_

//***************************************************************************
// Copyright (C) EMC Corporation 2011-2012
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

/*! @file  EmcUTIL_Device.h */
//
// Contents:
//      Header file for the EMCUTIL_Device APIs.

/*! @addtogroup emcutil_devioctl
 *  @{
 */

#include "EmcUTIL.h"

CSX_CDECLS_BEGIN

////////////////////////////////////

#if defined(EMCUTIL_CONFIG_BACKEND_WU)
typedef HANDLE EMCUTIL_RDEVICE_REFERENCE;
typedef HANDLE EMCUTIL_DEVICE_REFERENCE;
#define EMCUTIL_DEVICE_REFERENCE_INVALID INVALID_HANDLE_VALUE
#elif defined(EMCUTIL_CONFIG_BACKEND_CSX)
typedef csx_p_rdevice_reference_t EMCUTIL_RDEVICE_REFERENCE;
typedef csx_p_device_reference_t  EMCUTIL_DEVICE_REFERENCE;
#define EMCUTIL_DEVICE_REFERENCE_INVALID CSX_P_RDEVICE_REFERENCE_INVALID_VALUE
#else // not actually supported in WK case, so just needs to build
typedef void* EMCUTIL_RDEVICE_REFERENCE;		/*!< Rdevice reference type */
typedef void* EMCUTIL_DEVICE_REFERENCE;			/*!< Device reference type */
#define EMCUTIL_DEVICE_REFERENCE_INVALID (0)	/*!< Invalid reference to device */
#endif

#define EMCUTIL_DREF_IS_INVALID(d) (d == EMCUTIL_DEVICE_REFERENCE_INVALID) /*!< Test for Invalid reference to device */

////////////////////////////////////

/*!
 * @brief 
 *   Open a device which exists in a different container than the container
 *   the caller is running in.
 *   Does NOT support Windows overlapped I/O (FILE_FLAG_OVERLAPPED)
 *   Does NOT enforce any sharing restrictions - in CSX such restrictions are
 *   determined at device creation time (see csx_p_device_create...with_attributes(), 
 *   specifically CSX_P_DEVICE_ATTRIBUTE_EXCLUSIVE)
 *   Does NOT provide any special handling for the (future) case where the remote
 *   container has crashed.
 * @param DeviceName Device name without the special Windows prefix (\\\\.\\)
 * @param ContainerName ignored in Windows case, set to NULL to use default container name on CSX or
 *   caller can specify a specific container
 * @param RemoteDeviceReference Handle to opened device
 * @return 
 *   Returns error status as appropriate from OS. Some of these map 1:1 to an
 *   EMCUTIL_STATUS error code define, some do not.
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC
EmcutilRemoteDeviceOpen(csx_device_name_t DeviceName,
                        csx_string_t ContainerName,
                        EMCUTIL_RDEVICE_REFERENCE *RemoteDeviceReference);

/*!
 * @brief 
 *   Open a device which exists in a different container than the container
 *   the caller is running in - same as EmcutilRemoteDeviceOpen - but allows
 *   certain flags to control the exact parameters used in the Windows case
 *   on the CreateFile API.
 * @param DeviceName Device name without the special Windows prefix (\\\\.\\)
 * @param ContainerName ignored in Windows case, set to NULL to use default container name on CSX or
 *   caller can specify a specific container
 * @param OpenFlags see flags list below
 * @param RemoteDeviceReference Handle to opened device
 * @return 
 *   Returns error status as appropriate from OS. Some of these map 1:1 to an
 *   EMCUTIL_STATUS error code define, some do not.
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC
EmcutilRemoteDeviceOpenWithFlags(csx_device_name_t DeviceName,
                        csx_string_t ContainerName,
                        csx_nuint_t OpenFlags,
                        EMCUTIL_RDEVICE_REFERENCE *RemoteDeviceReference);

/* use default open style */
#define EMCUTIL_REMOTE_DEVICE_OPEN_FLAGS_NONE           0x0
/* open read-only - use GENERIC_READ instead of GENERIC_READ | GENERIC_WRITE on Windows CreateFile dwDesiredAccess */
#define EMCUTIL_REMOTE_DEVICE_OPEN_FLAGS_READONLY       0x1
/* open shared - use FILE_SHARE_READ | FILE_SHARE_WRITE instead of 0 on Windows CreateFile dwShareMode */
#define EMCUTIL_REMOTE_DEVICE_OPEN_FLAGS_SHARED         0x2
/* do not serialize IOCTLs - unimplemented as of now */
#define EMCUTIL_REMOTE_DEVICE_OPEN_FLAGS_NONSERIALIZED  0x4

/*!
 * @brief
 *   Close a device which exists in a different container than the container
 *   the caller is running in.
 * @param
 *   RemoteDeviceReference Handle to device to close
 * @return
 *   Returns error status as appropriate from OS
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC
EmcutilRemoteDeviceClose(EMCUTIL_RDEVICE_REFERENCE RemoteDeviceReference);

/*!
 * @brief
 *   Synchronously send an IOCTL to a device which exists in a different 
 *   container than the container the caller is running in.
 * @param RemoteDeviceReference Handle to opened device
 * @param IoctlCode IOCTL operation code
 * @param pInputBuffer Pointer to input buffer
 * @param InputSize Input buffer size (in bytes)
 * @param pOutputBuffer Pointer to output buffer
 * @param OutputSize Output buffer size (in bytes)
 * @param BytesReturned [OUT] Size of returned data (in bytes)
 * @return
 *   Returns error status as appropriate from OS
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC
EmcutilRemoteDeviceIoctl(EMCUTIL_RDEVICE_REFERENCE RemoteDeviceReference,
                         csx_nuint_t IoctlCode,
                         csx_pvoid_t pInputBuffer,
                         csx_size_t InputSize,
                         csx_pvoid_t pOutputBuffer,
                         csx_size_t OutputSize,
                         csx_size_t *BytesReturned);

/*!
 * @brief
 *   Reads a raw device which exists in a different container than the container 
 *   the caller is running in.
 * @param RemoteDeviceReference Handle to opened device
 * @param offset Offset in bytes with respect to beginning of the file
 * @param iobuffer Pointer to buffer in which data is copied on successfull read
 * @param RequestedSize buffer size (in bytes)
 * @param BytesReturned [OUT] Size of read data (in bytes)
 * @return
 *   Returns error status as appropriate from OS
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC
EmcutilRemoteDeviceRead (EMCUTIL_RDEVICE_REFERENCE RemoteDeviceReference,  // handle to device
                         csx_offset_t offset,
                         csx_pvoid_t iobuffer,
                         csx_size_t RequestedSize,
                         csx_size_t *BytesReturned);

/*!
 * @brief
 *   Writes to a raw device which exists in a different container than the container 
 *   the caller is running in.
 * @param RemoteDeviceReference Handle to opened device
 * @param offset Offset in bytes with respect to beginning of the file
 * @param iobuffer Pointer to buffer from which data is copied to file on successfull write
 * @param RequestedSize buffer size (in bytes)
 * @param BytesReturned [OUT] Size of written data (in bytes)
 * @return
 *   Returns error status as appropriate from OS
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC
EmcutilRemoteDeviceWrite (EMCUTIL_RDEVICE_REFERENCE RemoteDeviceReference,  // handle to device
                         csx_offset_t offset,
                         csx_pvoid_t iobuffer,
                         csx_size_t RequestedSize,
                         csx_size_t *BytesReturned);


CSX_CDECLS_END

/* @} END emcutil_devioctl */

//++
//.End file EmcUTIL_Device.h
//--

#endif                                     /* EMCUTIL_DEVICE_H_ */
