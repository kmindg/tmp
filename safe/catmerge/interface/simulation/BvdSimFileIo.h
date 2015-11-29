/**************************************************************************
 * Copyright (C) EMC Corporation 2009-2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               BvdSimFileIo.h
 ***************************************************************************
 *
 * DESCRIPTION:  BvdSim File IO API Declarations
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    07/11/2009  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef __BVDSIMFILIO
#define __BVDSIMFILIO

# include <stdio.h>
# include <stdlib.h>
# include <k10ntddk.h>
# include <EmcPAL.h>

#ifdef __cplusplus
/*
 * all of these functions need standard C linkage.  Otherwise, they can not be referenced from C.
 * Note, thee implementation of these functions is within a cpp file, which allows these functions to
 * access C++ instances, instance methods and class methods
 */
# include "simulation/IOSectorManagement.h"
# include "BasicVolumeDriver/BasicVolumeParams.h"


extern "C" {
#endif

# include "generic_types.h"
# include "k10ntddk.h"
# include "flare_export_ioctls.h"
# include "EmcPAL_DriverShell.h"
# include "EmcPAL_Memory.h"


#if defined(WDMSERVICES_EXPORT)
#define WDMSERVICEPUBLIC CSX_MOD_EXPORT 
#else
#define WDMSERVICEPUBLIC CSX_MOD_IMPORT 
#endif


WDMSERVICEPUBLIC void BvdSim_RegisterCompletionCheck(void (*callback)());

typedef enum BvdSimFileIoBlockSize_e {
    BvdSimFileIoBlockSize_512 = 0x0,
    BvdSimFileIoBlockSize_520 = 0x1,
    BvdSimFileIoBlockSize_as_configured = 3,
} BvdSimFileIoBlockSize_t;


// Async I/O Block.  Used for Async I/O.   DO NOT Mix ASYNC and SYNC with the same file handle.
#define ASYNC_BLOCK_MAGIC_NUMBER 0xAAAABBBB

typedef struct _BvdSimFile_IoAsyncBlock
{
    UINT_32     mMagicNumber;
    void*       mPFO; // pBvdSim_FileObject_t
    HANDLE      mhFile;
    PVOID       mlpBuffer;
    UINT_64     mOffset;
    UINT_64     mnNumberOfBytesToRead;
    PVOID       mlpOverlapped;
    EMCPAL_IRP_STACK_FLAGS  mIrpFlags;
    void*       mTaskID; // PIOTaskId
    BOOL        mWrite;  // True for write, False for Read
    PEMCPAL_IRP mIrp;
    PEMCPAL_DEVICE_OBJECT   mDeviceObject;
    EMCPAL_RENDEZVOUS_EVENT  mCompletion_event;
    EMCPAL_IRP_STATUS_BLOCK mIoStatusBlock;
    void*       mIoReference;
    void*       mIoBuffer;
    EMCPAL_TIME_100NSECS mStartTime;
    EMCPAL_TIME_100NSECS mEndTime;
    
    // Get an IRP to be used with the Request.   
    PEMCPAL_IRP  getIrp() {
        UINT_32 stackSize = EmcpalDeviceGetIrpStackSize(mDeviceObject) + 1;
        UINT_32 irpSize = csx_p_dcall_size(stackSize);
        mIrp = (PEMCPAL_IRP) malloc(irpSize);  // add one extra level??
        if(mIrp == NULL) {
            CSX_PANIC();
        }
        EmcpalIrpInitialize(mIrp, irpSize, stackSize);
        return mIrp;
    }
    
    // Delete the Irp used with the request.
    void     releaseIrp() {
        if(mIrp) {
            free(mIrp);
        }
        mIrp = NULL;
    }


} BvdSimFile_IoAsyncBlock, *PBvdSimFile_IoAsyncBlock;

WDMSERVICEPUBLIC BOOL BvdSimFileIo_IsIoComplete(PBvdSimFile_IoAsyncBlock asyncBlock, UINT_64 * lpNumberOfBytesRead, EMCPAL_STATUS* pStatus);
WDMSERVICEPUBLIC void BvdSimFileIo_WaitForIoComplete(PBvdSimFile_IoAsyncBlock asyncBlock, UINT_64 * lpNumberOfBytesRead, EMCPAL_STATUS* pStatus);
WDMSERVICEPUBLIC void BvdSimFileIo_AsyncComplete(PBvdSimFile_IoAsyncBlock asyncBlock);
WDMSERVICEPUBLIC BOOL BvdSimFileIo_AsyncInitialize(PBvdSimFile_IoAsyncBlock asyncBlock);

/*
 * This function is only able to open existing device files and will only
 * open files which have already bee registered in the BvdSimDeviceRegistry
 * This function issues the appropriate Driver majorFunction[IRP_MJ_CREATE]
 */
WDMSERVICEPUBLIC HANDLE BvdSimFileIo_OpenDeviceFile(
  char * lpFileName,
  IOTaskId * taskID);


WDMSERVICEPUBLIC void BvdSimFileIo_setIRPCancelDelay(HANDLE handle, EMCPAL_TIMEOUT_MSECS delay, bool clearOnAccess = true);

/*
 * BvdSimFileIo_BuildDeviceIocontrolRequest has the same semantics as the Windows
 * BuildDeviceIocontrolRequest. This function constructs an Irp containing the supplied ioctl data
 */
 /*
extern PIRP BvdSimFileIo_BuildDeviceIoControlRequest(
                              ULONG IoControlCode,
                              PDEVICE_OBJECT DeviceObject,
                              PVOID InputBuffer,
                              ULONG InputBufferLength,
                              PVOID OutputBuffer,
                              ULONG OutputBufferLength,
                              BOOL InternalDeviceIoControl,
                              PEMCPAL_RENDEZVOUS_EVENT Event,
                              PIO_STATUS_BLOCK IoStatusBlock);
*/
/*
 * BvdSimFileIo_DeviceIoControl has the same semantics as the Windows DeviceIoControl
 * This function constructs and dispatches an Irp containing the supplied ioctl data
 * by invoking the appropriate Driver majorFunction[IRP_MJ_DEVICE_CONTROL]
 */
WDMSERVICEPUBLIC BOOL BvdSimFileIo_DeviceIoControl(
    HANDLE hDevice,
    UINT_32 dwIoControlCode,
    PVOID lpInBuffer,
    UINT_64 nInBufferSize,
    PVOID lpOutBuffer,
    UINT_64 nOutBufferSize,
    UINT_64 * lpBytesReturned,
    PVOID lpOverlapped); // was LPOVERLAPPED

/*
 * BvdSimFileIo_DeviceIoControlAsync has the same semantics as the Windows DeviceIoControl
 * This function constructs and dispatches an Irp containing the supplied ioctl data
 * by invoking the appropriate Driver majorFunction[IRP_MJ_DEVICE_CONTROL]
 * This is asynchronous operation. asyncBlock->mCompletion_event will be triggered when
 * IRP completes.
 */
WDMSERVICEPUBLIC BOOL BvdSimFileIo_DeviceIoControlAsync(
    HANDLE hDevice,
    UINT_32 dwIoControlCode,
    PVOID lpInBuffer,
    UINT_64 nInBufferSize,
    PVOID lpOutBuffer,
    UINT_64 nOutBufferSize,
    PBvdSimFile_IoAsyncBlock asyncBlock);

/*
 * BvdSimFileIo_WriteFile has the same semantics as the Windows WriteFile
 * This function constructs and dispatches an Irp containing the supplied ioctl data
 * by invoking the appropriate Driver majorFunction[IRP_MJ_WRITE]
 */
WDMSERVICEPUBLIC BOOL BvdSimFileIo_WriteFile(
  HANDLE hFile,
  PVOID lpBuffer,
  UINT_64 nNumberOfBytesToWrite,
  UINT_64 * lpNumberOfBytesWritten,
  PVOID lpOverlapped,  // was LPOVERLAPPED
#ifdef __cplusplus
  EMCPAL_IRP_STACK_FLAGS irpFlags = 0);
#else
  EMCPAL_IRP_STACK_FLAGS irpFlags);
#endif

/*
 * This function constructs an Irp containing the supplied ioctl data
 * for a Driver majorFunction[IRP_MJ_WRITE]
 */
WDMSERVICEPUBLIC PEMCPAL_IRP BvdSimFileIo_BuildWriteFileIRP(
  HANDLE hFile,
  PVOID lpBuffer,
  UINT_64 offset,
  UINT_64 nNumberOfBytesToWrite,
  PBvdSimFile_IoAsyncBlock asyncBlock,
#ifdef __cplusplus
  EMCPAL_IRP_STACK_FLAGS irpFlags = 0);
#else
  EMCPAL_IRP_STACK_FLAGS irpFlags);
#endif

/*
 * BvdSimFileIo_WriteFileAsync has the same semantics as the Windows WriteFile
 * This function constructs and dispatches an Irp containing the supplied ioctl data
 * by invoking the appropriate Driver majorFunction[IRP_MJ_WRITE]
 */
WDMSERVICEPUBLIC BOOL BvdSimFileIo_WriteFileAsync(
  HANDLE hFile,
  PVOID lpBuffer,
  UINT_64 offset,
  UINT_64 nNumberOfBytesToWrite,
  PVOID lpOverlapped,
  PBvdSimFile_IoAsyncBlock asyncBlock,  // was LPOVERLAPPED
#ifdef __cplusplus
  EMCPAL_IRP_STACK_FLAGS irpFlags = 0);
#else
  EMCPAL_IRP_STACK_FLAGS irpFlags);
#endif

/*
 * BvdSimFileIo_ReadFile has the same semantics as the Windows ReadFile
 * This function constructs and dispatches an Irp containing the supplied ioctl data
 * by invoking the appropriate Driver majorFunction[IRP_MJ_READ]
 */
WDMSERVICEPUBLIC BOOL BvdSimFileIo_ReadFile(
  HANDLE hFile,
  PVOID  lpBuffer,
  UINT_64 nNumberOfBytesToRead,
  UINT_64 * lpNumberOfBytesRead,
  PVOID lpOverlapped,
#ifdef __cplusplus
  EMCPAL_IRP_STACK_FLAGS irpFlags = 0,
  ULONG IOTags = 0);
#else
  EMCPAL_IRP_STACK_FLAGS irpFlags,
  ULONG IOTags);
#endif

/*
 * BvdSimFileIo_ReadFileAsync has the same semantics as the Windows ReadFile
 * This function constructs and dispatches an Irp containing the supplied ioctl data
 * by invoking the appropriate Driver majorFunction[IRP_MJ_READ]
 */
WDMSERVICEPUBLIC BOOL BvdSimFileIo_ReadFileAsync(
  HANDLE hFile,
  PVOID  lpBuffer,
  UINT_64 offset,
  UINT_64 nNumberOfBytesToRead,
  PVOID lpOverlapped,
  PBvdSimFile_IoAsyncBlock asyncBlock);

/*
 * BvdSimFileIo_DMDestFile has similar semantics as the Windows WriteFile
 * This function constructs and dispatches an Irp containing the supplied ioctl data
 * by invoking the appropriate Driver majorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL]
 * with minorFunction set to FLARE_DM_DESTINATION
 */
WDMSERVICEPUBLIC BOOL BvdSimFileIo_DMDestFile(
  HANDLE hFile,
  PVOID  lpBuffer,
  UINT_64 nNumberOfBytesToRead,
  UINT_64 * lpNumberOfBytesRead,
  PVOID lpOverlapped,
#ifdef __cplusplus
  IOTaskId * taskID = NULL);
#else
  IOTaskId * taskID);
#endif

/*
* This function constructs an Irp containing the supplied ioctl data
* for a Driver [IRP_MJ_INTERNAL_DEVICE_CONTROL]
 * with minorFunction set to FLARE_DCA_WRITE
*/
WDMSERVICEPUBLIC PEMCPAL_IRP BvdSimFileIo_BuildDCAWriteFileIRP(
  HANDLE hFile,
  PVOID  lpBuffer,
  UINT_64 offset,
  UINT_64 nNumberOfBytesToWrite,
#ifdef __cplusplus
  EMCPAL_IRP_STACK_FLAGS irpFlags = 0,
  IOTaskId * taskID = NULL,
  PBvdSimFile_IoAsyncBlock asyncBlock = NULL);
#else
  EMCPAL_IRP_STACK_FLAGS irpFlags,
  IOTaskId * taskID,
  PBvdSimFile_IoAsyncBlock asyncBlock);
#endif

/*
 * BvdSimFileIo_DCAWriteFile has similar semantics as the Windows WriteFile
 * This function constructs and dispatches an Irp containing the supplied ioctl data
 * by invoking the appropriate Driver majorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL]
 * with minorFunction set to FLARE_DCA_WRITE
 */
WDMSERVICEPUBLIC BOOL BvdSimFileIo_DCAWriteFile(
  HANDLE hFile,
  PVOID  lpBuffer,
  UINT_64 nNumberOfBytesToRead,
  UINT_64 * lpNumberOfBytesRead,
  PVOID lpOverlapped,
#ifdef __cplusplus
  EMCPAL_IRP_STACK_FLAGS irpFlags = 0,
  IOTaskId * taskID = NULL);
#else
  EMCPAL_IRP_STACK_FLAGS irpFlags,
  IOTaskId * taskID);
#endif

/*
 * BvdSimFileIo_DCAWriteFileAsync has similar semantics as the Windows WriteFile
 * This function constructs and dispatches an Irp containing the supplied ioctl data
 * by invoking the appropriate Driver majorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL]
 * with minorFunction set to FLARE_DCA_WRITE
 * I/Os done with this API are completed Asynchronously
 */
WDMSERVICEPUBLIC BOOL BvdSimFileIo_DCAWriteFileAsync(
  HANDLE hFile,
  PVOID  lpBuffer,
  UINT_64 offset,
  UINT_64 nNumberOfBytesToRead,
  PVOID lpOverlapped,
#ifdef __cplusplus
  EMCPAL_IRP_STACK_FLAGS irpFlags = 0,
  IOTaskId * taskID = NULL,
  PBvdSimFile_IoAsyncBlock asyncBlock = NULL);
#else
  EMCPAL_IRP_STACK_FLAGS irpFlags,
  IOTaskId * taskID,
  PBvdSimFile_IoAsyncBlock asyncBlock);
#endif

/*
 * BvdSimFileIo_ZeroFillFile has similar semantics as the Windows ioctl
 * This function constructs and dispatches an Irp containing the supplied ioctl data
 * by invoking the appropriate Driver majorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL]
 * with minorFunction set to IOCTL_FLARE_ZERO_FILL
 */
WDMSERVICEPUBLIC BOOL BvdSimFileIo_ZeroFillFile(
  HANDLE hFile,
  UINT_64 nNumberOfBlocksToZero,
  UINT_64 * lpNumberOfBlocksZeroed,
  PVOID lpOverlapped);  // was LPOVERLAPPED

/*
 * BvdSimFileIo_ZeroFillFileAsync has similar semantics as the Windows ioctl
 * This function constructs and dispatches an Irp containing the supplied ioctl data
 * by invoking the appropriate Driver majorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL]
 * with minorFunction set to IOCTL_FLARE_ZERO_FILL
 */
WDMSERVICEPUBLIC BOOL BvdSimFileIo_ZeroFillFileAsync(
    HANDLE hFile,
    UINT_64 nNumberOfBlocksToZero,
    UINT_64 offset,
    PVOID lpInBuffer,
    UINT_64 inBufferSize,
    PVOID lpOutBuffer,
    UINT_64 outBufferSize,
    PBvdSimFile_IoAsyncBlock asyncBlock);  // was LPOVERLAPPED

/*
 * BvdSimFileIo_CorruptCrcFile has similar semantics as the Windows ioctl
 * This function constructs and dispatches an Irp containing the supplied ioctl data
 * by invoking the appropriate Driver majorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL]
 * with minorFunction set to IOCTL_FLARE_MARK_BLOCK_BAD
 */
WDMSERVICEPUBLIC BOOL BvdSimFileIo_CorruptCrcFile(
    HANDLE hFile,
    UINT_32 nNumberOfBlocksToInvalidate,
    UINT_64 * lpNumberOfBlocksInvalidated,
    PVOID lpOverlapped);  // was LPOVERLAPPED


/*
 * BvdSimFileIo_DCAReadFile has similar semantics as the Windows ReadFile
 * This function constructs and dispatches an Irp containing the supplied ioctl data
 * by invoking the appropriate Driver majorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL]
 * with minorFunction set to FLARE_DCA_READ
 */
WDMSERVICEPUBLIC BOOL BvdSimFileIo_DCAReadFile(
  HANDLE hFile,
  PVOID  lpBuffer,
  UINT_64 nNumberOfBytesToRead,
  UINT_64 * lpNumberOfBytesRead,
  PVOID lpOverlapped,
#ifdef __cplusplus
  EMCPAL_IRP_STACK_FLAGS irpFlags = 0,
  IOTaskId * taskID = NULL,
  ULONG IOTags = 0,
  BOOL LayoutDontCare = false);
#else
  EMCPAL_IRP_STACK_FLAGS irpFlags,
  IOTaskId * taskID,
  ULONG IOTags,
  BOOL LayoutDontCare);
#endif

/*
 * BvdSimFileIo_DCAReadFileAsync has similar semantics as the Windows ReadFile
 * This function constructs and dispatches an Irp containing the supplied ioctl data
 * by invoking the appropriate Driver majorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL]
 * with minorFunction set to FLARE_DCA_READ
 * I/Os done with this API are completed Asynchronously
 */
WDMSERVICEPUBLIC BOOL BvdSimFileIo_DCAReadFileAsync(
  HANDLE hFile,
  PVOID  lpBuffer,
  UINT_64 offset,
  UINT_64 nNumberOfBytesToRead,
  PVOID lpOverlapped,
#ifdef __cplusplus
  IOTaskId * taskID = NULL,
  PBvdSimFile_IoAsyncBlock asyncBlock = NULL,
  BOOL LayoutDontCare = false);
#else
  IOTaskId * taskID,
  PBvdSimFile_IoAsyncBlock asyncBlock,
  BOOL LayoutDontCare);
#endif

WDMSERVICEPUBLIC BOOL BvdSimFileIo_SGLReadFile(
  HANDLE hFile,
  PVOID  lpInfo,
  UINT_64 nNumberOfBytesToRead,
  UINT_32 Priority,
#ifdef __cplusplus
  UINT_8  irpFlags = 0,
  IOTaskId * taskID = NULL);
#else
  UINT_8  irpFlags,
  IOTaskId * taskID);
#endif

WDMSERVICEPUBLIC BOOL BvdSimFileIo_SGLReadFileAsync(
    HANDLE hFile,
    PVOID  lpInfo,
    UINT_64 nNumberOfBytesToRead,
    UINT_64 offset,
    UINT_32 Priority,
#ifdef __cplusplus
    IOTaskId * taskID = NULL,
    PBvdSimFile_IoAsyncBlock asyncBlock = NULL);
#else
    IOTaskId * taskID,
    PBvdSimFile_IoAsyncBlock asyncBlock);
#endif

WDMSERVICEPUBLIC BOOL BvdSimFileIo_SGLWriteFile(
  HANDLE hFile,
  PVOID  lpInfo,
  UINT_64 nNumberOfBytesToWrite,
  UINT_32 Priority,
#ifdef __cplusplus
  UINT_8  irpFlags = 0,
  IOTaskId * taskID = NULL);
#else
  UINT_8  irpFlags,
  IOTaskId * taskID);
#endif

WDMSERVICEPUBLIC BOOL BvdSimFileIo_SGLWriteFileAsync(
    HANDLE hFile,
    PVOID  lpInfo,
    UINT_64 nNumberOfBytesToWrite,
    UINT_64 offset,
    UINT_32 Priority,
#ifdef __cplusplus
    IOTaskId * taskID = NULL,
    PBvdSimFile_IoAsyncBlock asyncBlock = NULL);
#else
    IOTaskId * taskID,
    PBvdSimFile_IoAsyncBlock asyncBlock);
#endif

/*
 * BvdSimFileIo_SetFilePointer has the same semantics as the Windows SetFilePointer
 * This function updates the PFILE_OBJECT.CurrentByteOffset, as specified by the dmMoveMethod flag
 */
WDMSERVICEPUBLIC BOOL BvdSimFileIo_SetFilePointer(
  HANDLE hFile,
  LONG lDistanceToMove,
  LONG * lpDistanceToMoveHigh,
  DWORD dwMoveMethod);

/*
 * BvdSimFileIo_GetFilePointer returns PFILE_OBJECT->CurrentByteOffset as an unsigned 64b value
 */
WDMSERVICEPUBLIC INT_64 BvdSimFileIo_GetFilePointer(HANDLE hFile);

/*
 * BvdSimFileIo_CloseFile has the same semantics as the Windows CloseHandle
 * This function constructs and dispatches an Irp containing the supplied ioctl data
 * by invoking the appropriate Driver majorFunction[IRP_MJ_CLOSE]
 */
WDMSERVICEPUBLIC BOOL BvdSimFileIo_CloseFile(HANDLE hFile);

/*
 * Allow caller to release irp;
 */
WDMSERVICEPUBLIC VOID BvdSimFileIo_ReleaseIRP(HANDLE hFile);

/*
 * BvdSimFileIo_SignalIoComplete will be invoked by BasicLib::Complete_Irp
 * after all invoking all stack completion functions, when Irp->OriginalFileObject != NULL
 *
 * This operation passes control back, from the WDM, to the Application,
 * to the BvdSimFileIo method that originally invoked the IO
 */
WDMSERVICEPUBLIC void BvdSimFileIo_SignalIoComplete(PEMCPAL_FILE_OBJECT OriginalFileObject, PEMCPAL_IRP Irp);

#ifdef __cplusplus
};
#endif

# ifdef __cplusplus
class BvdSimFileIo_File {
    friend class BvdSimDisparateFileIo;
public:
    BvdSimFileIo_File(char *path = NULL, IOTaskId * taskID = NULL);
    BvdSimFileIo_File(HANDLE  DeviceHandle);
    BvdSimFileIo_File(PEMCPAL_FILE_OBJECT  FileObject);
    virtual ~BvdSimFileIo_File();

    HANDLE getHandle();
    char *getPath();

    BOOL OpenDeviceFile();

    BOOL WriteFile(
        PVOID lpBuffer,
        UINT_64 nNumberOfBytesToWrite,
        UINT_64 * lpNumberOfBytesWritten,
        PVOID lpOverlapped,  // was LPOVERLAPPED
        EMCPAL_IRP_STACK_FLAGS irpFlags = 0);

    BOOL WriteFile(
        PVOID lpBuffer,
        UINT_64 nNumberOfBytesToWrite,
        UINT_64 * lpNumberOfBytesWritten,
        PVOID lpOverlapped,  // was LPOVERLAPPED
        INT_64 offset,
        EMCPAL_IRP_STACK_FLAGS irpFlags = 0);

    BOOL WriteFileAsync(
        PVOID lpBuffer,
        UINT_64 nNumberOfBytesToWrite,
        INT_64 offset,
        PVOID lpOverlapped,  // was LPOVERLAPPED
        PBvdSimFile_IoAsyncBlock asyncBlock,
        EMCPAL_IRP_STACK_FLAGS irpFlags = 0);

    BOOL ReadFile(
        PVOID  lpBuffer,
        UINT_64 nNumberOfBytesToRead,
        UINT_64 * lpNumberOfBytesRead,
        PVOID lpOverlapped,  // was LPOVERLAPPED
        EMCPAL_IRP_STACK_FLAGS irpFlags = 0,
        ULONG IOTags = 0);

    BOOL ReadFile(
        PVOID  lpBuffer,
        UINT_64 nNumberOfBytesToRead,
        UINT_64 * lpNumberOfBytesRead,
        PVOID lpOverlapped,  // was LPOVERLAPPED
        INT_64 offset,
        EMCPAL_IRP_STACK_FLAGS irpFlags = 0,
        ULONG IOTags = 0);

    BOOL ReadFileAsync(
        PVOID  lpBuffer,
        UINT_64 nNumberOfBytesToRead,
        INT_64 offset,
        PVOID lpOverlapped,  // was LPOVERLAPPED
        PBvdSimFile_IoAsyncBlock asyncBlock);

    BOOL DMDestFile(
        PVOID  lpBuffer,
        UINT_64 nNumberOfBytesToRead,
        UINT_64 * lpNumberOfBytesRead,
        PVOID lpOverlapped,  // was LPOVERLAPPED
        IOTaskId * taskID = NULL);

    BOOL DMDestFile(
        PVOID  lpBuffer,
        UINT_64 nNumberOfBytesToRead,
        UINT_64 * lpNumberOfBytesRead,
        PVOID lpOverlapped,  // was LPOVERLAPPED
        INT_64 offset,
        IOTaskId * taskID = NULL);
;
    BOOL DCAWriteFile(
        PVOID  lpBuffer,
        UINT_64 nNumberOfBytesToRead,
        UINT_64 * lpNumberOfBytesRead,
        PVOID lpOverlapped, // was LPOVERLAPPED
        EMCPAL_IRP_STACK_FLAGS irpFlags = 0,
        IOTaskId * taskID = NULL);

    BOOL DCAWriteFile(
        PVOID  lpBuffer,
        UINT_64 nNumberOfBytesToRead,
        UINT_64 * lpNumberOfBytesRead,
        PVOID lpOverlapped,  // was LPOVERLAPPED
        INT_64 offset,
        EMCPAL_IRP_STACK_FLAGS irpFlags = 0,
        IOTaskId * taskID = NULL);

    BOOL DCAWriteFileAsync(
        PVOID  lpBuffer,
        UINT_64 nNumberOfBytesToRead,
        PVOID lpOverlapped,  // was LPOVERLAPPED
        INT_64 offset,
        PBvdSimFile_IoAsyncBlock asyncBlock,
        EMCPAL_IRP_STACK_FLAGS irpFlags = 0,
        IOTaskId * taskID = NULL);
    
    BOOL DCAReadFile(
        PVOID  lpBuffer,
        UINT_64 nNumberOfBytesToRead,
        UINT_64 * lpNumberOfBytesRead,
        PVOID lpOverlapped,  // was LPOVERLAPPED
        EMCPAL_IRP_STACK_FLAGS irpFlags = 0,
        IOTaskId * taskID = NULL,
        ULONG IOTags = 0,
        BOOL layoutDontCare = false);

    BOOL DCAReadFile(
        PVOID  lpBuffer,
        UINT_64 nNumberOfBytesToRead,
        UINT_64 * lpNumberOfBytesRead,
        PVOID lpOverlapped,  // was LPOVERLAPPED
        INT_64 offset,
        EMCPAL_IRP_STACK_FLAGS irpFlags = 0,
        IOTaskId * taskID = NULL,
        ULONG IOTags = 0,
        BOOL layoutDontCare = false);

    BOOL DCAReadFileAsync(
        PVOID  lpBuffer,
        UINT_64 nNumberOfBytesToRead,
        PVOID lpOverlapped,  // was LPOVERLAPPED
        INT_64 offset,
        PBvdSimFile_IoAsyncBlock asyncBlock,
        IOTaskId * taskID = NULL,
        BOOL layoutDontCare = false);

    static BOOL IsAsyncIoComplete(
        PBvdSimFile_IoAsyncBlock asyncBlock, 
        UINT_64 * lpNumberOfBytesReadorWritten, 
        EMCPAL_STATUS* pStatus);
    
    static void WaitForAsyncIoComplete(
        PBvdSimFile_IoAsyncBlock asyncBlock,
        UINT_64 * lpNumberOfBytesReadorWritten,
        EMCPAL_STATUS* pStatus);

    static void AsyncComplete(
        PBvdSimFile_IoAsyncBlock asyncBlock
    );
    
    static BOOL AsyncInitialize(
        PBvdSimFile_IoAsyncBlock asyncBlock
    );

    BOOL SGLWriteFile(
        PVOID  lpInfo,
        UINT_64 nNumberOfBytesToWrite,
        UINT_32 Priority,
        INT_64 offset,
        UINT_8 irpFlags = 0,
        IOTaskId * taskID = NULL);

    BOOL SGLReadFile(
        PVOID  lpInfo,
        UINT_64 nNumberOfBytesToRead,
        UINT_32 Priority,
        INT_64 offset,
        UINT_8 irpFlags = 0,
        IOTaskId * taskID = NULL);

    BOOL SGLWriteFileAsync(
        PVOID  lpInfo,
        UINT_64 nNumberOfBytesToWrite,
        INT_64 offset,
        UINT_32 Priority,
        PBvdSimFile_IoAsyncBlock asyncBlock,
        IOTaskId * taskID = NULL);

    BOOL SGLReadFileAsync(
        PVOID  lpInfo,
        UINT_64 nNumberOfBytesToRead,
        INT_64 offset,
        UINT_32 Priority,
        PBvdSimFile_IoAsyncBlock asyncBlock,        
        IOTaskId * taskID = NULL);

    BOOL ZeroFillFile(
        UINT_64 nNumberOfBytesToZero,
        UINT_64 * lpNumberOfBlocksZeroed,
        PVOID lpOverlapped);  // was LPOVERLAPPED
    
    BOOL ZeroFillFileAsync(
        UINT_64 nNumberOfBytesToZero,
        UINT_64 offset,
        PVOID   lpInBuffer,
        UINT_64 inBufferSize,
        PVOID   lpOutBufer,
        UINT_64 outBufferSize,
        PBvdSimFile_IoAsyncBlock asyncBlock);  // was LPOVERLAPPED
    
    BOOL CorruptCrcFile(
        UINT_32 nNumberOfBytesToInvalidate,
        UINT_64 * lpNumberOfBlocksInvalidated,
        PVOID lpOverlapped);  // was LPOVERLAPPED

    BOOL BlockWriteFile(
        PVOID buffer,
        UINT_64 noOfBlocks,
        UINT_64 *noOfBlocksWritten,
        INT_64 blockOffset);

    BOOL BlockReadFile(
        PVOID buffer,
        UINT_64 noOfBlocks,
        UINT_64 *noOfBlocksRead,
        INT_64 blockOffset);

    BOOL BlockDMDestFile(
        PVOID buffer,
        UINT_64 noOfBlocks,
        UINT_64 *noOfBlocksWritten,
        INT_64 blockOffset,
        IOTaskId * taskID = NULL);

    BOOL BlockDCAWriteFile(
        PVOID buffer,
        UINT_64 noOfBlocks,
        UINT_64 *noOfBlocksWritten,
        INT_64 blockOffset,
        IOTaskId * taskID = NULL);

    BOOL BlockDCAReadFile(
        PVOID buffer,
        UINT_64 noOfBlocks,
        UINT_64 *noOfBlocksRead,
        INT_64 blockOffset,
        IOTaskId * taskID = NULL);

    BOOL SetFilePointer(INT_64 offset);

    BOOL SetFilePointer(
        LONG lDistanceToMove,
        LONG * lpDistanceToMoveHigh,
        DWORD dwMoveMethod);

    INT_64 GetFilePointer();

    BOOL DeviceIoControl(
        UINT_32 dwIoControlCode,
        PVOID lpInBuffer,
        UINT_64 nInBufferSize,
        PVOID lpOutBuffer,
        UINT_64 nOutBufferSize,
        UINT_64 * lpBytesReturned,
        PVOID lpOverlapped);

    BOOL DeviceIoControlAsync(
        UINT_32 dwIoControlCode,
        PVOID lpInBuffer,
        UINT_64 inBufferSize,
        PVOID lpOutBuffer,
        UINT_64 outBufferSize,
        PBvdSimFile_IoAsyncBlock asyncBlock);

    BOOL CloseFile();

    /*
     * tresspass/assign this Device/file
     */
    BOOL trespassDevice();

    /*
     * Issue trespass ownership loss IOCTL for this Device/file
     */
    BOOL ownershipLossDevice();

    /*
     * Issue IOCTL_BVD_ENUM_VOLUMES and retrieve
     * the list of WWID that identify available volumes
     */
    BasicVolumeEnumVolumesResp *EnumerateDevices();

    /*
     * performWrite invokes BvdSimFileIo_WriteFile to send buffer to this device/file
     */
    BOOL performWrite(PVOID buffer, UINT_64 len, UINT_64 *bytesWritten, INT_64 offset, IOTaskId * taskID = NULL, EMCPAL_IRP_STACK_FLAGS irpFlags = 0);
    BOOL performWriteAsync(PVOID buffer, UINT_64 len, INT_64 offset, IOTaskId * taskID = NULL, PBvdSimFile_IoAsyncBlock asyncBlock = NULL, EMCPAL_IRP_STACK_FLAGS irpFlags = 0);

    /*
     * performRead invokes BvdSimFileIo_ReadFile to send buffer to this device/file
     */
    BOOL performRead(PVOID buffer, UINT_64 len, UINT_64 *bytesRead, INT_64 offset, IOTaskId * taskID = NULL, EMCPAL_IRP_STACK_FLAGS irpFlags = 0, ULONG IOTags = 0);
    BOOL performReadAsync(PVOID buffer, UINT_64 len, INT_64 offset, IOTaskId * taskID = NULL, PBvdSimFile_IoAsyncBlock asyncBlock = NULL);

    /*
     * performDCAWrite invokes BvdSimFileIo_DCAWriteFile to send buffer to this device/file
     */
    BOOL performDCAWrite(PVOID buffer, UINT_64 len, UINT_64 *bytesWritten, INT_64 offset, EMCPAL_IRP_STACK_FLAGS irpFlags = 0, IOTaskId * taskID = NULL);
    BOOL performDCAWriteAsync(PVOID buffer, UINT_64 len, INT_64 offset, EMCPAL_IRP_STACK_FLAGS irpFlags = 0, IOTaskId * taskID = NULL, PBvdSimFile_IoAsyncBlock asyncBlock = NULL);

    /*
     * performDCARead invokes BvdSimFileIo_DCAReadFile to send buffer to this device/file
     */
    BOOL performDCARead(PVOID buffer, UINT_64 len, UINT_64 *bytesRead, INT_64 offset, EMCPAL_IRP_STACK_FLAGS irpFlags = 0, IOTaskId * taskID = NULL, ULONG IOTags = 0, BOOL layoutDontCare = false);
    BOOL performDCAReadAsync(PVOID buffer, UINT_64 len, INT_64 offset, IOTaskId * taskID = NULL, PBvdSimFile_IoAsyncBlock asyncBlock = NULL);

    /*
     * performDMDest invokes BvdSimFileIo_DMDestFile to send buffer to this device/file
     */
    BOOL performDMDest(PVOID buffer, UINT_64 len, UINT_64 *bytesWritten, INT_64 offset, IOTaskId * taskID = NULL);

     /*
     * performZeroFill invokes BvdSimFileIo_ZeroFillFile to send zerofill to this device/file
     */
    BOOL performZeroFill(UINT_64 lengthInBlocks, UINT_64 *blocksZeroed, INT_64 offset);
    BOOL performZeroFillAsync(UINT_64 lengthInBlocks, INT_64 offset, PVOID lpInBuffer, UINT_64 inLen, PVOID lpOutBuffer, UINT_64 outLen, PBvdSimFile_IoAsyncBlock asyncBlock = NULL);
    
    /*
     * performZeroFill invokes BvdSimFileIo_CorruptCrcFile to send CorruptCrc to this device/file
     */
    BOOL performCorruptCrc(UINT_32 len, UINT_64 *bytesWritten, INT_64 offset);
    
    BOOL performSGLWrite(PVOID  lpInfo, UINT_64 len, INT_64 offset, UINT_8 irpFlags = 0, IOTaskId * taskID = NULL);
    BOOL performSGLWriteAsync(PVOID  lpInfo, UINT_64 len, INT_64 offset, IOTaskId * taskID = NULL, PBvdSimFile_IoAsyncBlock asyncBlock = NULL);

    BOOL performSGLRead(PVOID  lpInfo, UINT_64 len, INT_64 offset, UINT_8 irpFlags = 0, IOTaskId * taskID = NULL);
    BOOL performSGLReadAsync(PVOID  lpInfo, UINT_64 len, INT_64 offset, IOTaskId * taskID = NULL, PBvdSimFile_IoAsyncBlock asyncBlock = NULL);

    /*
     * perform 512 byte block write operations to this device/file
     */
    BOOL performBlockWrite(PVOID buffer, UINT_64 noOfBlocks, UINT_64 *noOfBlocksWritten, INT_64 blockOffset, IOTaskId * taskID = NULL);
    BOOL performBlockWriteAsync(PVOID buffer, UINT_64 noOfBlocks, INT_64 blockOffset, IOTaskId * taskID = NULL, PBvdSimFile_IoAsyncBlock asyncBlock = NULL);

    /*
     * perform 512 byte block read operations to this device/file
     */
    BOOL performBlockRead(PVOID buffer, UINT_64 noOfBlocks, UINT_64 *noOfBlocksRead, INT_64 blockOffset, IOTaskId * taskID = NULL);
    BOOL performBlockReadAsync(PVOID buffer, UINT_64 noOfBlocks, INT_64 blockOffset, IOTaskId * taskID = NULL, PBvdSimFile_IoAsyncBlock asyncBlock = NULL);

    /*
     * perform 512 byte block DCAwrite operations this device/file
     */
    BOOL performBlockDCAWrite(PVOID buffer,
                                UINT_64 noOfBlocks,
                                UINT_64 *noOfBlocksWritten,
                                UINT_64 blockOffset,
                                EMCPAL_IRP_STACK_FLAGS irpFlags = 0,
                                IOTaskId * taskID = NULL);


    BOOL performBlockDCAWriteAsync(PVOID buffer,
                                UINT_64 noOfBlocks,
                                UINT_64 blockOffset,
                                EMCPAL_IRP_STACK_FLAGS irpFlags = 0,
                                IOTaskId * taskID = NULL,
                                PBvdSimFile_IoAsyncBlock asyncBlock = NULL);

    /*
     * perform 512 byte block DCAread operations this device/file
     */
    BOOL performBlockDCARead(PVOID buffer,
                                UINT_64 noOfBlocks,
                                UINT_64 *noOfBlocksRead,
                                INT_64 blockOffset,
                                IOTaskId * taskID = NULL);

    BOOL performBlockDCAReadAsync(PVOID buffer,
                                UINT_64 noOfBlocks,
                                INT_64 blockOffset,
                                IOTaskId * taskID = NULL,
                                PBvdSimFile_IoAsyncBlock asyncBlock = NULL,
                                BOOL layoutDontCare = false);


    /*
    ** perform 512 byte block SGLwrite operations this device/file
    */
    BOOL performBlockSGLWrite(PVOID buffer,
                                UINT_64 noOfBlocks,
                                UINT_64 *noOfBlocksWritten,
                                UINT_64 blockOffset,
                                IOTaskId * taskID = NULL);

    BOOL performBlockSGLWriteAsync(PVOID buffer,
                                UINT_64 noOfBlocks,
                                UINT_64 blockOffset,
                                IOTaskId * taskID = NULL,
                                PBvdSimFile_IoAsyncBlock asyncBlock = NULL);

    /*
    ** perform 512 byte block SGLread operations this device/file
    */
    BOOL performBlockSGLRead(LPVOID buffer,
                            UINT_64 noOfBlocks,
                            UINT_64 *noOfBlocksRead,
                            INT_64 blockOffset,
                            IOTaskId * taskID = NULL);

    BOOL performBlockSGLReadAsync(LPVOID buffer,
                            UINT_64 noOfBlocks,
                            INT_64 blockOffset,
                            IOTaskId * taskID = NULL,
                            PBvdSimFile_IoAsyncBlock asyncBlock = NULL);

    /*
     * fillBufferWithIncrementingPattern is a convenience function that 
     * fills buffer with data.  Currently, this function generates 
     * a repeading/incrementing byte pattern.  
     */
    virtual void fillBufferWithIncrementingPattern(UINT_8 *buffer, UINT_64 len, UINT_64 fileOffset = 0);
    
    /*
     * validateIncrementingPatternBufferUsingOffset is a convenience function that 
     * confirms that the buffer contains an incrementing byte pattern.
     * This function can/should be used to confirm that Read operations 
     * contain the expected contents
     * Note this function should only be used on buffers retrieved from 
     * device stacks built upon BvdSimIncrementingPatternedDisk devices
     *
     * (BvdSimPersistentDisk devices contain real/arbitrary data and
     *  can not be valdated using this method
     */
    virtual BOOL validateIncrementingPatternBufferUsingFileOffset(UINT_8 *buffer, UINT_64 len, UINT_64 fileOffset, char *msgPrefix = "", BOOL dbgBreakOnError = FALSE);

    BOOL validateZeroPatternBuffer(UINT_8 *buffer, UINT_64 len);

    void setIRPCancelDelay(EMCPAL_TIMEOUT_MSECS delay, bool clearOnAccess = true);


    /* GetDeviceObject() to send IOCTLs directly */
    void *GetDeviceObject();

    /* GetDeviceExtension() to get CacheVolume for statistics */
    void *GetDeviceExtension();

    /* Return the TaskID associated with this file */
    IOTaskId * getIOTaskID();

    INT_64 getIrpStatus();

    virtual VolumeIdentifier GetVolumeIdentifier();

    virtual void AcquireLBALocks(const UINT_64 startingLBA, const UINT_64 sectors);
    virtual void ReleaseLBALocks(const UINT_64 startingLBA, const UINT_64 sectors);
    virtual ULONG IncrementSequence(const UINT_64 lba);
    virtual const ULONG GetSequence(const UINT_64 lba);
    virtual bool HasLock(const UINT_64 lba);
    virtual bool IsCheckingSequence();
    virtual void EnableAdvancedDmcCheckingCapabilities(int volIndex);
    virtual bool HasLBARangeLockOverlaps(const UINT_64 lba1, const UINT_64 sectors1, const UINT_64 lba2, const UINT_64 sectors2, UINT_64* neededLock, UINT_64* neededSectors);

    virtual void * GetVolumeSharedMemory();

private:
    char *mFilePath;
    HANDLE mDeviceHandle;
    IOTaskId * mTaskID;
    bool mTaskIDInput;
};


typedef BvdSimFileIo_File *PBvdSimFileIo_File;

# endif

#undef FILEIODLLEXPORT

#endif
