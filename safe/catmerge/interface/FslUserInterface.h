#ifndef __FSL_USER_INTERFACE__
#define __FSL_USER_INTERFACE__
//***************************************************************************
// Copyright (C) EMC Corporation 2009 - 2011
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

/**********************************************************************
 *
 *  File Name: 
 *          FslUserInterface.h
 *
 *  Description:
 *          This is the implementation of a function based interface
 *          to the FSLBus driver for user space applications.  Each of these
 *          interface functions is just a wrapper around an IOCTL to the
 *          FSLBus driver.
 *
 *  Revision History:
 *          06/18/2009 MWH Created based on SPID.
 *          
 **********************************************************************/

#include <windows.h>
//#include <devioctl.h>
#include "FslTypes.h"
#include "flare_export_ioctls.h"
#include<EmcUTIL_Device.h>


#define FSL_BASE_DEVICE_NAME   L"FslBus"
#define FSL_NT_DEVICE_NAME     L"\\Device\\" FSL_BASE_DEVICE_NAME
#define FSL_DOSDEVICES_NAME    L"\\DosDevices\\" FSL_BASE_DEVICE_NAME
#define FSL_WIN32_DEVICE_NAME  L"\\\\.\\" FSL_BASE_DEVICE_NAME

#define DEFAULT_WAIT_TIME_IN_SECONDS  120
#define FSL_
//
// A helper macro used to define our ioctls.  The 0xB00 function code base
// value was chosen for no particular reason other than the fact that it
// lies in the customer-reserved number range (according to <devioctl.h>).
//
#define FSL_CTL_CODE(code, method) (\
    EMCPAL_IOCTL_CTL_CODE(EMCPAL_IOCTL_FILE_DEVICE_UNKNOWN, 0xB00 + (code),   \
              (method), EMCPAL_IOCTL_FILE_ANY_ACCESS) )

#define IOCTL_FSL_GET_STATUS             \
    FSL_CTL_CODE(4, EMCPAL_IOCTL_METHOD_BUFFERED)
    //  Obtain Status and related info (such as VolumeGUID) for a particular FSL

#define FSL_LIB_NICENAME_PATH_MAX_LENGTH          128

typedef enum _FSL_DISK_FORMAT {
    RAW_FORMAT  = 0,
    NTFS_FORMAT = 1
}FSL_DISK_FORMAT;

typedef struct _FSL_LUN_INFO
{
    FSL_STATUS_CODE         Status;
    FLARE_VOLUME_STATE      LunInfo;
    FSLBUS_PERFORMANCE_DATA PerformanceData;
}FSL_LUN_INFO,*PFSL_LUN_INFO;

#if defined(__cplusplus)
extern "C"
{
#endif

//The global spid handle.  This can either be opened explicitly,
//or will be opened implicitly on the first call to an interface function
extern EMCUTIL_RDEVICE_REFERENCE FslGlobalHandle;
FSL_STATUS_CODE Win32ErrorToFSL(DWORD ErrorCode);
/*************************  E X P O R T E D   I N T E R F A C E   F U N C T I O N S  ***************************/

//Note, these 2 functions are unique to the user space interface and are optional.
BOOL fslOpenHandle(void);
BOOL fslCloseHandle(void);

FSL_STATUS_CODE fslBuildFSLDeviceStack(CHAR *FSLDeviceName,
                                       CHAR *ConsumedDeviceName,
                                       FSL_DISK_FORMAT FormatType, /* Only Raw supported for now*/
                                       DWORD Timeout, /* Timeout in seconds, 0 means wait forever*/
                                       CHAR* FSLMountPath);  /* not supported in Inyo */
FSL_STATUS_CODE FslFormatLun(PCHAR FSLDeviceName);

FSL_STATUS_CODE fslDestroyFSLDeviceStack(CHAR *FSLDeviceName, DWORD Timeout); 
BOOL fslIsDeviceExists(PCHAR FSLDeviceName);
FSL_STATUS_CODE fslDoesBusPDOExist(PCHAR FslDeviceName, BOOL  *pPDOExist);
FSL_STATUS_CODE fslInjectDelay(UINT_32 InjectionDelayinMilliSec);

/*
*  Note: This function can be called to get  status of the lun. This function
*  internally sends IOCTL_FSL_BUS_LUN_INFO to the lower level driver to get
*  lun status, and other details.
*
*  If function is successfull then it return FSL_SUCCESS, otherwise
*  it returns error code. If function returns FSL_SUCCESS, then caller can
*  use DeviceLunInfo to get the details about specific lun. DeviceLunInfo 
*  contains status, and FLARE_VOLUME_STATE. Caller can use FLARE_VOLUME_STATE 
*  to get details of a lun.
*
*  DO NOT TRUST DATA INSIDE DeviceLunInfo, IF THIS FUNCTION DOES NOT RETURN FSL_SUCCESS.
*/
FSL_STATUS_CODE fslGetLUNStatus(CHAR *FSLDeviceName,PFSL_LUN_INFO DeviceLunInfo);


#if defined(__cplusplus)
}
#endif

#endif //__FSL_USER_INTERFACE__
