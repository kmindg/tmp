/**************************************************************************
 * Copyright (C) EMC Corporation 2013-2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               VfdSimFileIo.h
 ***************************************************************************
 *
 * DESCRIPTION:  VfdSim File IO API Declarations
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    07/11/2009  Martin Buckley Initial Version
 *    08/27/2013  Mark Cariddi Version
 **************************************************************************/
#ifndef __VFDSIMFILIO
#define __VFDSIMFILIO

# include <stdio.h>
# include <stdlib.h>
# include <k10ntddk.h>
# include <EmcPAL.h>
# include "simulation/BvdSimFileIo.h"


# ifdef __cplusplus
class VfdSimFileIo_File : public BvdSimFileIo_File {
public:
    VfdSimFileIo_File(char *path = NULL, IOTaskId * taskID = NULL);
    VfdSimFileIo_File(HANDLE  DeviceHandle);
    VfdSimFileIo_File(PEMCPAL_FILE_OBJECT  FileObject);
    virtual ~VfdSimFileIo_File();

    virtual VolumeIdentifier GetVolumeIdentifier() CSX_CPP_OVERRIDE;

    virtual void AcquireLBALocks(const UINT_64 startingLBA, const UINT_64 sectors) CSX_CPP_OVERRIDE;
    virtual void ReleaseLBALocks(const UINT_64 startingLBA, const UINT_64 sectors) CSX_CPP_OVERRIDE;
    virtual ULONG IncrementSequence(const UINT_64 lba) CSX_CPP_OVERRIDE;
    virtual const ULONG GetSequence(const UINT_64 lba) CSX_CPP_OVERRIDE;
    virtual bool HasLock(const UINT_64 lba) CSX_CPP_OVERRIDE;
    virtual bool IsCheckingSequence() CSX_CPP_OVERRIDE;
    virtual void EnableAdvancedDmcCheckingCapabilities(int volIndex) CSX_CPP_OVERRIDE;
    virtual bool HasLBARangeLockOverlaps(const UINT_64 lba1, const UINT_64 sectors1, const UINT_64 lba2, const UINT_64 sectors2, UINT_64* neededLock, UINT_64* neededSectors);

    /*
     * fillBufferWithIncrementingPattern is a convenience function that 
     * fills buffer with data.  Currently, this function generates 
     * a repeading/incrementing byte pattern.  
     */
    virtual void fillBufferWithIncrementingPattern(UINT_8 *buffer, UINT_64 len, UINT_64 fileOffset = 0) CSX_CPP_OVERRIDE;
    
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
    virtual BOOL validateIncrementingPatternBufferUsingFileOffset(UINT_8 *buffer, UINT_64 len, UINT_64 fileOffset, char *msgPrefix = "", BOOL dbgBreakOnError = FALSE) CSX_CPP_OVERRIDE;

    virtual void * GetVolumeSharedMemory() CSX_CPP_OVERRIDE;

};


typedef VfdSimFileIo_File *PVfdSimFileIo_File;

// This class is used when wanting to perform Disparate Operations on a volume.  It allows the user
// to create Host Irps which frame the Disparate write operation to be performed.  So for example, you
// want to do a 100 sector Write to Volume 0.  You create the irp with this class, then use the
// DisparateWriteOperation class to set up the underlying operations on the volume(s) that hold the
// data/meta data for that operation (i.e. simulation MLU).
class BvdSimDisparateFileIo : public VfdSimFileIo_File {
public:
    BvdSimDisparateFileIo(char *path = NULL, IOTaskId * taskID = NULL) : VfdSimFileIo_File(path, taskID) {};
    BvdSimDisparateFileIo(HANDLE  DeviceHandle) : VfdSimFileIo_File(DeviceHandle) {};
    BvdSimDisparateFileIo(PEMCPAL_FILE_OBJECT  FileObject) : VfdSimFileIo_File(FileObject) {};

    // Builds a Write File Irp that is used to represent a IRP that was received by
    // the Array.    If asyncBlock is not passed in, then we are using the IRP associated
    // with the file object and that means no other I/Os can be performed until
    // we call CompleteWriteFileIrp.
    PEMCPAL_IRP BuildWriteFileIRP(
        PVOID lpBuffer,
        INT_64 offset,
        UINT_64 nNumberOfBytesToWrite,
        PBvdSimFile_IoAsyncBlock asyncBlock = NULL);

    void CompleteWriteFileIRP(PBvdSimFile_IoAsyncBlock asyncBlock = NULL, EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS, UINT_64 bytesReturned = 0);

    // Builds a DCA Write File Irp that is used to represent a IRP that was received by
    // the Array.   Because this IRP is an Internal device io control IRP it is assumed to
    // be delivered from driver to driver.  Therefore the Irp's stack location is set
    // via EmcpalIrpSetNextStackLocation
    //
    // If asyncBlock is not passed in, then we are using the IRP associated
    // with the file object and that means no other I/Os can be performed until
    // we call CompleteDCAWriteFileIrp.
    PEMCPAL_IRP BuildDCAWriteFileIRP(
        PVOID lpBuffer,
        INT_64 offset,
        UINT_64 nNumberOfBytesToWrite,
        EMCPAL_IRP_STACK_FLAGS irpFlags = 0,
        IOTaskId * taskID = NULL,
        PBvdSimFile_IoAsyncBlock asyncBlock = NULL);

    void CompleteDCAWriteFileIRP(PBvdSimFile_IoAsyncBlock asyncBlock = NULL, EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS, UINT_64 bytesReturned = 0);

};

typedef BvdSimDisparateFileIo *PBvdSimDisparateFileIo;

# endif

#undef FILEIODLLEXPORT

#endif
