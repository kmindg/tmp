/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               BvdSimIOUtilityBaseClass.h
 ***************************************************************************
 *
 * DESCRIPTION: Definition of utility class BvdSimIOUtilityBaseClass
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    11/22/2009  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef __BVDSIMIOUTILITYBASECLASS__
#define __BVDSIMIOUTILITYBASECLASS__

# include "generic_types.h"
# include "simulation/BvdSimFileIo.h"
# include "simulation/IOReference.h"
# include "mut_assert.h"

/*
 * BvdSimIOUtilityBaseClass provides convenience functions
 * that simplify writting IO Validate tests
 */
class BvdSimIOUtilityBaseClass {
public:

    /*
     * \fn void validateWriteSectors()
     * \Param file              - pointer to a BvdSimFileIo_File descriptor 
     * \Param noSectors         - UINT_64 number of sectors to be written
     * \Param startingSectorLBA - Sector LBA of write operation
     * \Param expectedStatus    - expected Status
     * \Param irpFlags          - IRP flags to send
     *
     * \details 
     *
     * Method validateWriteSectors() generates an incrementing pattern noSectors long, which is
     * written to file at startingSectorLBA.  Function verifies 
     *      - return status is TRUE
     *      - the correct mount of data was written
     *
     * an exception is throwwn when unexpected data is returned
     *
     */
    void validateWriteSectors(BvdSimFileIo_File *file, UINT_64 noSectors, UINT_64 startingSectorLBA, EMCPAL_STATUS expectedStatus = EMCPAL_STATUS_SUCCESS, EMCPAL_IRP_STACK_FLAGS irpFlags = 0) {
        /*
         * confirming that the file's IOTask will construct the appropriate buffer
         * Note, create/use validateWriteSectors signature that includes passing an appropriate IOTask if/when assert fails
         */
        FF_ASSERT(file->getIOTaskID()->getTaskType() == IOTask_Type_512);

        UINT_64 bytesTransferred;
        UINT_32 seed                 = (UINT_32)file->getIOTaskID()->convertLBAToFlareAddress(startingSectorLBA);
        IOReference *outputReference = IOSectorManagement::IOReferenceFactory(file->getIOTaskID(), SectorPatternIncrementing, seed, noSectors);

        file->AcquireLBALocks(startingSectorLBA, noSectors);
        outputReference->generateBufferContents(file->GetVolumeIdentifier(), (UINT_32) startingSectorLBA, (PDMC_CONTROL_MEMORY) file->GetVolumeSharedMemory());

        UINT_8 *outputBuffer         = outputReference->getBuffer();

        UINT_64 transferLength       = file->getIOTaskID()->convertNoSectorsToFlareLength(noSectors);
        UINT_64 address              = file->getIOTaskID()->convertLBAToFlareAddress(startingSectorLBA);

        mut_printf(MUT_LOG_HIGH, "About to write %lld sectors at LBA 0x%llx, starting address 0x%llx ", noSectors, startingSectorLBA, address);
        BOOL status = file->performWrite(outputBuffer, transferLength, &bytesTransferred, address, NULL, irpFlags);

        EMCPAL_STATUS opStatus = (EMCPAL_STATUS)file->getIrpStatus();

        MUT_ASSERT_INTEGER_EQUAL_MSG(expectedStatus, opStatus, "performWrite returned an incorrect status");

        if(EMCPAL_IS_SUCCESS(opStatus)) {
            MUT_ASSERT_TRUE_MSG(status, "performWrite failed, expected status = TRUE");
            MUT_ASSERT_INTEGER_EQUAL_MSG(transferLength, bytesTransferred, "performWrite returned the wrong number of bytes");
        }
        file->ReleaseLBALocks(startingSectorLBA, noSectors);
        IOSectorManagement::destroy(outputReference);
    };

    /*
     * \fn void validateWriteSectorsAsync()
     * \Param file              - pointer to a BvdSimFileIo_File descriptor 
     * \Param noSectors         - UINT_64 number of sectors to be written
     * \Param startingSectorLBA - Sector LBA of write operation
     * \Param asyncBlock        - Asynchronous I/O block
     * \Param expectedStatus    - expected Status
     *
     * \details 
     *
     * Method validateWriteSectorsAsynch() generates an incrementing pattern noSectors long, which is
     * written to file at startingSectorLBA.  
     *
     */
    void validateWriteSectorsAsync(BvdSimFileIo_File *file, UINT_64 noSectors, UINT_64 startingSectorLBA, PBvdSimFile_IoAsyncBlock asyncBlock, 
                                   EMCPAL_STATUS expectedStatus = EMCPAL_STATUS_SUCCESS) {
        /*
         * confirming that the file's IOTask will construct the appropriate buffer
         * Note, create/use validateWriteSectors signature that includes passing an appropriate IOTask if/when assert fails
         */
        FF_ASSERT(file->getIOTaskID()->getTaskType() == IOTask_Type_512);

        UINT_32 seed                 = (UINT_32)file->getIOTaskID()->convertLBAToFlareAddress(startingSectorLBA);
        IOReference *outputReference = IOSectorManagement::IOReferenceFactory(file->getIOTaskID(), SectorPatternIncrementing, seed, noSectors);
        asyncBlock->mIoReference = outputReference;
        
        file->AcquireLBALocks(startingSectorLBA, noSectors);
        outputReference->generateBufferContents(file->GetVolumeIdentifier(), (UINT_32) startingSectorLBA, (PDMC_CONTROL_MEMORY) file->GetVolumeSharedMemory());

        UINT_8 *outputBuffer         = outputReference->getBuffer();

        UINT_64 transferLength       = file->getIOTaskID()->convertNoSectorsToFlareLength(noSectors);
        UINT_64 address              = file->getIOTaskID()->convertLBAToFlareAddress(startingSectorLBA);

        mut_printf(MUT_LOG_HIGH, "About to write Async %lld sectors at LBA 0x%llx, starting address 0x%llx ", noSectors, startingSectorLBA, address);
        (void) file->performWriteAsync(outputBuffer, transferLength, address, file->getIOTaskID(), asyncBlock);

        file->ReleaseLBALocks(startingSectorLBA, noSectors);
    };

    /*
     * \fn void validateReadSectors()
     * \Param file              - pointer to a BvdSimFileIo_File descriptor 
     * \Param noSectors         - UINT_64 number of sectors to be read
     * \Param startingSectorLBA - Sector LBA of read operation
     * \Param expectedStatus    - expected Status
     * \Param irpFlags          - IRP flags to send
     * \Param IOTags            - IO TAG to send
     *
     * \details 
     *
     * Method validateReadSectors() reads noSectors from file.  Function verifies
     *      - return status is TRUE
     *      - the correct mount of data was read
     *      - the returned buffer contents contains data that had been generated bo the IOSectorManagment subsystem
     *
     * an exception is throwwn when unexpected data is returned
     *
     */
    void validateReadSectors(BvdSimFileIo_File *file, UINT_64 noSectors, UINT_64 startingSectorLBA, EMCPAL_STATUS expectedStatus = EMCPAL_STATUS_SUCCESS, EMCPAL_IRP_STACK_FLAGS irpFlags = 0, ULONG IOTags = 0) {
        /*
         * confirming that the file's IOTask will construct the appropriate buffer
         * Note, create/use validateWriteSectors signature that includes passing an appropriate IOTask if/when assert fails
         */
        FF_ASSERT(file->getIOTaskID()->getTaskType() == IOTask_Type_512);

        UINT_64 bytesTransferred;
        UINT_64 transferLength      = file->getIOTaskID()->convertNoSectorsToFlareLength(noSectors);
        UINT_64 bufferLength        = file->getIOTaskID()->convertFlareLengthToBufferLength(transferLength);
        UINT_64 address             = file->getIOTaskID()->convertLBAToFlareAddress(startingSectorLBA);

        IOBuffer *inputIOB          = IOSectorManagement::IOBufferFactory(bufferLength);
        IOReference *inputReference = IOSectorManagement::IOReferenceFactory(file->getIOTaskID(), inputIOB);

        mut_printf(MUT_LOG_HIGH, "About to read %lld sectors at LBA 0x%llx, starting address 0x%llx ", noSectors, startingSectorLBA, address);
        file->AcquireLBALocks(startingSectorLBA, noSectors);
        BOOL status = file->performRead(inputIOB->getBuffer(), transferLength, &bytesTransferred, address, NULL, irpFlags, IOTags);

        EMCPAL_STATUS opStatus = (EMCPAL_STATUS)file->getIrpStatus();

        MUT_ASSERT_INTEGER_EQUAL_MSG(expectedStatus, opStatus, "performRead returned an incorrect status");

        if(EMCPAL_IS_SUCCESS(opStatus)) {
            MUT_ASSERT_TRUE_MSG(status, "performRead failed, expected status = TRUE");
            MUT_ASSERT_INTEGER_EQUAL_MSG(transferLength, bytesTransferred, "performRead returned the wrong number of bytes");
            
            status = inputReference->identify(file->GetVolumeIdentifier(), (PDMC_CONTROL_MEMORY) file->GetVolumeSharedMemory(), startingSectorLBA, "performRead", TRUE);
            MUT_ASSERT_TRUE_MSG(status, "Unable to identify buffer contents after performRead");
        }
        file->ReleaseLBALocks(startingSectorLBA, noSectors);
        IOSectorManagement::destroy(inputIOB);
        IOSectorManagement::destroy(inputReference);
    }

    /*
     * \fn void validateReadSectorsAsync()
     * \Param file              - pointer to a BvdSimFileIo_File descriptor 
     * \Param noSectors         - UINT_64 number of sectors to be read
     * \Param startingSectorLBA - Sector LBA of read operation
     * \Param asyncBlock        - Asynchronous I/O block
     * \Param expectedStatus    - expected Status
     *
     * \details 
     *
     * Method validateReadSectorsAsync() reads noSectors from file.  
     *
     */
    void validateReadSectorsAsync(BvdSimFileIo_File *file, UINT_64 noSectors, UINT_64 startingSectorLBA, PBvdSimFile_IoAsyncBlock asyncBlock, 
                                  EMCPAL_STATUS expectedStatus = EMCPAL_STATUS_SUCCESS) {
        /*
         * confirming that the file's IOTask will construct the appropriate buffer
         * Note, create/use validateWriteSectors signature that includes passing an appropriate IOTask if/when assert fails
         */
        FF_ASSERT(file->getIOTaskID()->getTaskType() == IOTask_Type_512);

        UINT_64 transferLength      = file->getIOTaskID()->convertNoSectorsToFlareLength(noSectors);
        UINT_64 bufferLength        = file->getIOTaskID()->convertFlareLengthToBufferLength(transferLength);
        UINT_64 address             = file->getIOTaskID()->convertLBAToFlareAddress(startingSectorLBA);

        IOBuffer *inputIOB          = IOSectorManagement::IOBufferFactory(bufferLength);
        asyncBlock->mIoBuffer = inputIOB;

        mut_printf(MUT_LOG_LOW, "About to read Async %lld sectors at LBA 0x%llx, starting address 0x%llx ", noSectors, startingSectorLBA, address);
        
        file->AcquireLBALocks(startingSectorLBA, noSectors);
        (void)  file->performReadAsync(inputIOB->getBuffer(), transferLength, address, file->getIOTaskID(), asyncBlock);

        file->ReleaseLBALocks(startingSectorLBA, noSectors);
    }

    /*
     * \fn void validateWriteReadSectors()
     * \Param file              - pointer to a BvdSimFileIo_File descriptor 
     * \Param noSectors         - UINT_64 number of sectors to be written
     * \Param startingSectorLBA - Sector LBA of write operation
     *
     * \details 
     *
     * Method validateWriteReadSectors() performs a validateWriteSectors followed by a validateReadSectors.
     * This convenience medhod performs a complete write/read validation on a region of file.
     *
     */
    void validateWriteReadSectors(BvdSimFileIo_File *file, UINT_64 noSectors, UINT_64 startingSectorLBA, EMCPAL_STATUS expectedStatus = EMCPAL_STATUS_SUCCESS) {
        validateWriteSectors(file, noSectors, startingSectorLBA);
        validateReadSectors(file, noSectors, startingSectorLBA, expectedStatus);
    }

    /*
     * \fn void validateDCAWriteSectors()
     * \Param file              - pointer to a BvdSimFileIo_File descriptor 
     * \Param noSectors         - UINT_64 number of sectors to be written
     * \Param startingSectorLBA - Sector LBA of write operation
     *
     * \details 
     *
     * Method validateDCAWriteSectors() generates an incrementing pattern noSectors long, issues a DCAwrite
     * to write noSectors to file at startingSectorLBA.  Function verifies 
     *      - return status is TRUE
     *      - the correct amount of data was written
     *
     * an exception is throwwn when unexpected data is returned
     *
     */
   void validateDCAWriteSectors(BvdSimFileIo_File *file, UINT_64 noSectors, UINT_64 startingSectorLBA, PIOTaskId task = NULL, EMCPAL_STATUS expectedStatus = EMCPAL_STATUS_SUCCESS) {
       validateDCAWriteSectors(file, noSectors, startingSectorLBA, 0, task, expectedStatus);
   }

    /*
     * \fn void validateDCAWriteSectors()
     * \Param file              - pointer to a BvdSimFileIo_File descriptor 
     * \Param noSectors         - UINT_64 number of sectors to be written
     * \Param startingSectorLBA - Sector LBA of write operation
     * \Param irpFlags          - IRP flags to send
     * \Param task              - task
     * \Param expectedStatus    - expected Status
     *
     * \details 
     *
     * Method validateDCAWriteSectors() generates an incrementing pattern noSectors long, issues a DCAwrite
     * two write noSectorsto file at startingSectorLBA.  Function verifies 
     *      - return status is TRUE
     *      - the correct mount of data was written
     *
     * an exception is throwwn when unexpected data is returned
     *
     */
    void validateDCAWriteSectors(BvdSimFileIo_File *file, UINT_64 noSectors, UINT_64 startingSectorLBA, EMCPAL_IRP_STACK_FLAGS irpFlags, PIOTaskId task, EMCPAL_STATUS expectedStatus) {
        UINT_64 bytesTransferred;
        UINT_64 transferLength       = file->getIOTaskID()->convertNoSectorsToFlareLength(noSectors);
        UINT_64 address              = file->getIOTaskID()->convertLBAToFlareAddress(startingSectorLBA);
        UINT_32 seed                 = (UINT_32)file->getIOTaskID()->convertLBAToFlareAddress(startingSectorLBA);
        VolumeIdentifier diskUniqueIdentfier = file->GetVolumeIdentifier();

        if(task == NULL) {
            task = file->getIOTaskID();
        }

        IOReference *outputReference = IOSectorManagement::IOReferenceFactory(task, SectorPatternIncrementing, seed, noSectors);

        file->AcquireLBALocks(startingSectorLBA, noSectors);
        outputReference->generateBufferContents(diskUniqueIdentfier, (UINT_32) startingSectorLBA, (PDMC_CONTROL_MEMORY) file->GetVolumeSharedMemory());
        UINT_8 *outputBuffer         = outputReference->getBuffer();

        mut_printf(MUT_LOG_HIGH, "About to write %lld sectors at LBA 0x%llx, starting address 0x%llx ", noSectors, startingSectorLBA, address);
        BOOL retStatus = file->performDCAWrite(outputBuffer, transferLength, &bytesTransferred, address, irpFlags, task);

        EMCPAL_STATUS opStatus = (EMCPAL_STATUS)file->getIrpStatus();

        if(opStatus == EMCPAL_STATUS_CANCELLED) {
            mut_printf(MUT_LOG_HIGH, "validateDCAWriteSectors() write operation cancelled, skipping remaining assertions");
        }
        else {
            MUT_ASSERT_INTEGER_EQUAL_MSG(expectedStatus, opStatus, "performDCAWrite returned an incorrect status");

            if(EMCPAL_IS_SUCCESS(opStatus)) {
                MUT_ASSERT_TRUE_MSG(retStatus, "DCA_WRITE returned FALSE, TRUE expected");
                MUT_ASSERT_INTEGER_EQUAL_MSG(transferLength, bytesTransferred, "performDCAWrite returned the wrong number of bytes");
            }
        }
        file->ReleaseLBALocks(startingSectorLBA, noSectors);

        IOSectorManagement::destroy(outputReference);
    };

    /*
     * \fn void validateDCAWriteSectorsAsync()
     * \Param file              - pointer to a BvdSimFileIo_File descriptor 
     * \Param noSectors         - UINT_64 number of sectors to be written
     * \Param startingSectorLBA - Sector LBA of write operation
     * \Param asyncBlock        - Asynchronous I/O block
     * \Param expectedStatus    - expected Status
     * \Param task              - task
     *
     * \details 
     *
     * Method validateDCAWriteSectorsAsync() generates an incrementing pattern noSectors long, issues a performDCAWriteAsync
     * two write noSectorsto file at startingSectorLBA.  
     *
     */

    void validateDCAWriteSectorsAsync(BvdSimFileIo_File *file, UINT_64 noSectors, UINT_64 startingSectorLBA, PBvdSimFile_IoAsyncBlock asyncBlock, 
                                      PIOTaskId task = NULL, 
                                      EMCPAL_STATUS expectedStatus = EMCPAL_STATUS_SUCCESS) {
        UINT_64 transferLength       = file->getIOTaskID()->convertNoSectorsToFlareLength(noSectors);
        UINT_64 address              = file->getIOTaskID()->convertLBAToFlareAddress(startingSectorLBA);
        UINT_32 seed                 = (UINT_32)file->getIOTaskID()->convertLBAToFlareAddress(startingSectorLBA);
        VolumeIdentifier diskUniqueIdentfier = file->GetVolumeIdentifier();

        if(task == NULL) {
            task = file->getIOTaskID();
        }

        IOReference *outputReference = IOSectorManagement::IOReferenceFactory(task, SectorPatternIncrementing, seed, noSectors);

        file->AcquireLBALocks(startingSectorLBA, noSectors);
        outputReference->generateBufferContents(diskUniqueIdentfier, (UINT_32) startingSectorLBA, (PDMC_CONTROL_MEMORY) file->GetVolumeSharedMemory());
        UINT_8 *outputBuffer         = outputReference->getBuffer();
        asyncBlock->mIoReference = outputReference;

        mut_printf(MUT_LOG_HIGH, "About to write %lld sectors at LBA 0x%llx, starting address 0x%llx ", noSectors, startingSectorLBA, address);
        (void)  file->performDCAWriteAsync(outputBuffer, transferLength, address, 0, task, asyncBlock);

        file->ReleaseLBALocks(startingSectorLBA, noSectors);

    };

    
    /*
     * \fn void validateDCAReadSectors()
     * \Param file              - pointer to a BvdSimFileIo_File descriptor 
     * \Param noSectors         - UINT_64 number of sectors to be read
     * \Param startingSectorLBA - Sector LBA of read operation
     *
     * \details 
     *
     * Method validateDCAReadSectors() issues a DCARead of noSectors at startingSectorLBA from file.  Function verifies
     *      - return status is TRUE
     *      - the correct mount of data was read
     *      - the returned buffer contents contains data that had been generated bo the IOSectorManagment subsystem
     *
     * an exception is throwwn when unexpected data is returned
     *
     */

    void validateDCAReadSectors(BvdSimFileIo_File *file, UINT_64 noSectors, UINT_64 startingSectorLBA, PIOTaskId task = NULL, char *msgPrefix = "performDCARead", EMCPAL_STATUS expectedStatus = EMCPAL_STATUS_SUCCESS) {
        validateDCAReadSectors(file, noSectors, startingSectorLBA, 0, task, msgPrefix, expectedStatus);
    }

    /*
     * \fn void validateDCAReadSectors()
     * \Param file              - pointer to a BvdSimFileIo_File descriptor 
     * \Param noSectors         - UINT_64 number of sectors to be read
     * \Param startingSectorLBA - Sector LBA of read operation
     * \Param irpFlags          - IRP flags to send
     * \Param task              - task
     * \Param expectedStatus    - expected Status
     * \Param IOTags            - IO TAGs to send
     *
     * \details 
     *
     * Method validateDCAReadSectors() issues a DCARead of noSectors at startingSectorLBA from file.  Function verifies
     *      - return status is TRUE
     *      - the correct mount of data was read
     *      - the returned buffer contents contains data that had been generated bo the IOSectorManagment subsystem
     *
     * an exception is throwwn when unexpected data is returned
     *
     */

    void validateDCAReadSectors(BvdSimFileIo_File *file, UINT_64 noSectors, UINT_64 startingSectorLBA, EMCPAL_IRP_STACK_FLAGS irpFlags, PIOTaskId task, char *msgPrefix, EMCPAL_STATUS expectedStatus, 
                                ULONG IOTags = 0, BOOL layoutDontCare = false) {
        if(task == NULL) {
            task = file->getIOTaskID();
        }

        UINT_64 bytesTransferred;
        UINT_64 transferLength      = task->convertNoSectorsToFlareLength(noSectors);
        UINT_64 bufferLength        = task->convertFlareLengthToBufferLength(transferLength);
        UINT_64 address             = task->convertLBAToFlareAddress(startingSectorLBA);

        IOReference *ior;

        IOBuffer *inputIOB          = IOSectorManagement::IOBufferFactory(bufferLength);

        file->AcquireLBALocks(startingSectorLBA, noSectors);
        mut_printf(MUT_LOG_HIGH, "About to read %lld sectors at LBA 0x%llx, starting address 0x%llx ", noSectors, startingSectorLBA, address);
        (void)file->performDCARead(inputIOB->getBuffer(), transferLength, &bytesTransferred, address, irpFlags, task, IOTags, layoutDontCare);

        EMCPAL_STATUS opStatus = (EMCPAL_STATUS)file->getIrpStatus();

        MUT_ASSERT_INTEGER_EQUAL_MSG(expectedStatus, opStatus, "performDCARead returned an incorrect status");

        if(opStatus == EMCPAL_STATUS_CANCELLED) {
            mut_printf(MUT_LOG_HIGH, "validateDCAReadSectors() read operation cancelled, skipping remaining assertions");
        }
        else {
            if(EMCPAL_IS_SUCCESS(opStatus)) {
                MUT_ASSERT_INTEGER_EQUAL_MSG(transferLength, bytesTransferred, "performDCARead returned the wrong number of bytes");
                
                ior = IOSectorManagement::IOReferenceFactory(task, inputIOB);
                BOOL identifyStatus = ior->identify(file->GetVolumeIdentifier(), (PDMC_CONTROL_MEMORY) file->GetVolumeSharedMemory(), startingSectorLBA, msgPrefix, TRUE);
                MUT_ASSERT_TRUE_MSG(identifyStatus, "Unable to identify buffer contents after performDCARead");
                
                IOSectorManagement::destroy(ior);
            }
        }
        file->ReleaseLBALocks(startingSectorLBA, noSectors);

        IOSectorManagement::destroy(inputIOB);
    }

    /*
     * \fn void validateDCAReadSectorsLayoutDontCare()
     * \Param file              - pointer to a BvdSimFileIo_File descriptor 
     * \Param noSectors         - UINT_64 number of sectors to be read
     * \Param startingSectorLBA - Sector LBA of read operation
     * \Param irpFlags          - IRP flags to send
     * \Param task              - task
     * \Param expectedStatus    - expected Status
     * \Param IOTags            - IO TAGs to send
     *
     * \details 
     *
     * Method validateDCAReadSectorsLayoutDontCare() issues a DCARead of noSectors at startingSectorLBA from file.  
     * AvoidSkipOverheadBytes will be set to DONT_CARE in the DCA_TABLE.
     * Function verifies
     *      - return status is TRUE
     *      - the correct mount of data was read
     *      - the returned buffer contents contains data that had been generated bo the IOSectorManagment subsystem
     *
     * an exception is thrown when unexpected data is returned
     *
     */

    void validateDCAReadSectorsLayoutDontCare(BvdSimFileIo_File *file, UINT_64 noSectors, UINT_64 startingSectorLBA, EMCPAL_IRP_STACK_FLAGS irpFlags = 0, PIOTaskId task = NULL, 
                                              char *msgPrefix = "performDCARead", EMCPAL_STATUS expectedStatus = EMCPAL_STATUS_SUCCESS, ULONG IOTags = 0) {
        validateDCAReadSectors(file, noSectors, startingSectorLBA, irpFlags, task, msgPrefix, expectedStatus, IOTags, true);
    }

    /*
     * \fn void validateDCAReadSectorsAsync()
     * \Param file              - pointer to a BvdSimFileIo_File descriptor 
     * \Param noSectors         - UINT_64 number of sectors to be read
     * \Param startingSectorLBA - Sector LBA of read operation
     * \Param asyncBlock        - Asynchronous I/O block
     * \Param expectedStatus    - expected Status
     * \Param task              - task
     *
     * \details 
     *
     * Method validateDCAReadSectors() issues a DCARead of noSectors at startingSectorLBA from file. 
     *
     */
    
    void validateDCAReadSectorsAsync(BvdSimFileIo_File *file, UINT_64 noSectors, UINT_64 startingSectorLBA, PBvdSimFile_IoAsyncBlock asyncBlock,
                                     PIOTaskId task = NULL, 
                                     EMCPAL_STATUS expectedStatus = EMCPAL_STATUS_SUCCESS) {
        
        if(task == NULL) {
            task = file->getIOTaskID();
        }
    
        UINT_64 transferLength      = task->convertNoSectorsToFlareLength(noSectors);
        UINT_64 bufferLength        = task->convertFlareLengthToBufferLength(transferLength);
        UINT_64 address             = task->convertLBAToFlareAddress(startingSectorLBA);
    
        IOBuffer *inputIOB          = IOSectorManagement::IOBufferFactory(bufferLength);
        asyncBlock->mIoBuffer = inputIOB;
    
        mut_printf(MUT_LOG_HIGH, "About to read %lld sectors at LBA 0x%llx, starting address 0x%llx ", noSectors, startingSectorLBA, address);
        (void)file->performDCAReadAsync(inputIOB->getBuffer(), transferLength, address, task, asyncBlock);

    }
    
    /*
     * \fn void validateCompletedDCAReadSectorsAsync()
     * \Param file              - pointer to a BvdSimFileIo_File descriptor 
     * \Param asyncBlock        - Asynchronous I/O block
     *
     * \details 
     *
     * Method validateCompletedDCAReadSectorsAsync() validates that a
     *    Asynchronously issued DCARead  completed successfully
     *
     */
    
    void validateCompletedDCAReadSectorsAsync(BvdSimFileIo_File *file, PBvdSimFile_IoAsyncBlock asyncBlock) 
    {
        // Make sure this is a read request completing.
        if(!asyncBlock->mWrite) {
            // Get the task ID used for the operation.
            PIOTaskId pTask = (PIOTaskId) asyncBlock->mTaskID;
            
            // Get the I/O reference for this operations. Then lock the I/O range to ensure it does not change while we are checking it.
            IOReference* ior = IOSectorManagement::IOReferenceFactory(pTask, (IOBuffer*) asyncBlock->mIoBuffer);
            file->AcquireLBALocks(pTask->convertSectorAddressToLBA(asyncBlock->mOffset), pTask->convertFlareLengthToNoSectors(asyncBlock->mnNumberOfBytesToRead));
            
            // Validate the read data.
            BOOL identifyStatus = ior->identify(file->GetVolumeIdentifier(), (PDMC_CONTROL_MEMORY) file->GetVolumeSharedMemory(), 
                                                pTask->convertSectorAddressToLBA(asyncBlock->mOffset), "AsyncRead", TRUE);            
            MUT_ASSERT_TRUE_MSG(identifyStatus, "Unable to identify buffer contents after performDCARead");
            
            // If we passed the assert then clean up
            IOSectorManagement::destroy(ior);
            file->ReleaseLBALocks(pTask->convertSectorAddressToLBA(asyncBlock->mOffset), pTask->convertFlareLengthToNoSectors(asyncBlock->mnNumberOfBytesToRead));
        }
    }

    void validateDCAReadSectors(BvdSimFileIo_File *file, UINT_64 noSectors, UINT_64 startingSectorLBA, EMCPAL_STATUS expectedStatus) {
        validateDCAReadSectors(file, noSectors, startingSectorLBA, file->getIOTaskID(), "performDCARead", expectedStatus);
    }

    void validateDCAReadSectors(BvdSimFileIo_File *file, UINT_64 noSectors, UINT_64 startingSectorLBA, char *msgPrefix) {
        validateDCAReadSectors(file, noSectors, startingSectorLBA, file->getIOTaskID(), msgPrefix, EMCPAL_STATUS_SUCCESS);
    }
    
    /*
     * \fn BOOL IsAsyncIoComplete()
     * \Param file              - pointer to a BvdSimFileIo_File descriptor 
     * \Param asyncBlock        - Asynchronous I/O block
     * \Param lpNumberOfBytesReadOrWritten - number of bytes or blocks read or written
     * \Param pStatus           - completion status if completed
     *
     * \returns
     *    True if I/O completed false otherwise
     *      
     * \details 
     *
     * Method IsAsyncIoComplete() determines if an Asynchronous operation has completed
     *
     */
    
    BOOL IsAsyncIoComplete(BvdSimFileIo_File *file,
        PBvdSimFile_IoAsyncBlock asyncBlock, 
        UINT_64 * lpNumberOfBytesReadorWritten, 
        EMCPAL_STATUS* pStatus)
    {
        FF_ASSERT(asyncBlock);
        return file->IsAsyncIoComplete(
                   asyncBlock,
                   lpNumberOfBytesReadorWritten,
                   pStatus);
    }
    
    /*
     * \fn void AsyncComplete()
     * \Param file              - pointer to a BvdSimFileIo_File descriptor 
     * \Param asyncBlock        - Asynchronous I/O block
     *
     * \returns
     *      
     * \details 
     *
     * Method AsyncComplete() Called when an Asynchronous I/O operation has
     *  completed.  This cleans up used information.
     *
     */
    
    void AsyncComplete(BvdSimFileIo_File *file,
        PBvdSimFile_IoAsyncBlock asyncBlock)
    {
        FF_ASSERT(asyncBlock);
        if(asyncBlock->mWrite) {
            IOSectorManagement::destroy((IOReference*) asyncBlock->mIoReference);
        }
        else 
        {
            IOSectorManagement::destroy((IOBuffer*) asyncBlock->mIoBuffer);
        }
        file->AsyncComplete(asyncBlock);
    }
    
    /*
     * \fn BOOL AsyncInitialize()
     * \Param file              - pointer to a BvdSimFileIo_File descriptor 
     * \Param asyncBlock        - Asynchronous I/O block
     *
     * \returns
     *  TRUE if initialized, false if issue.
     *
     * \details 
     *
     * Method AsyncInitialize() Called to initialize a Asynchronous block
     * before use.
     *
     */
    
    BOOL AsyncInitialize(BvdSimFileIo_File *file,
        PBvdSimFile_IoAsyncBlock asyncBlock)
    {
        FF_ASSERT(asyncBlock);
        return file->AsyncInitialize(
                   asyncBlock);
        
    }
    
    /*
     * \fn void validateDCAWriteDCAReadSectors()
     * \Param file              - pointer to a BvdSimFileIo_File descriptor 
     * \Param noSectors         - UINT_64 number of sectors to be written
     * \Param startingSectorLBA - Sector LBA of write operation
     *
     * \details 
     *
     * Method validateDCAWriteDCAReadSectors() performs a validateDCAWriteSectors followed by a validateDCAReadSectors.
     * This convenience medhod performs a complete DCAwrite/DCAread validation on a region of file.
     *
     */
    void validateDCAWriteDCAReadSectors(BvdSimFileIo_File *file, UINT_64 noSectors, UINT_64 startingSectorLBA, PIOTaskId task = NULL, EMCPAL_STATUS expectedStatus = EMCPAL_STATUS_SUCCESS) {
        validateDCAWriteSectors(file, noSectors, startingSectorLBA, task);
        validateDCAReadSectors(file, noSectors, startingSectorLBA, task, "performDCARead", expectedStatus);
    }

    /*
     * \fn void validateBlockWriteSectors()
     * \Param file              - pointer to a BvdSimFileIo_File descriptor 
     * \Param noSectors         - UINT_64 number of sectors to be written
     * \Param startingSectorLBA - Sector LBA of write operation
     *
     * \details 
     *
     * Method validateBlockWriteSectors() generates an incrementing pattern noSectors long, issues a Blockwrite
     * two write noSectorsto file at startingSectorLBA.  Function verifies 
     *      - return status is TRUE
     *      - the correct mount of data was written
     *
     * an exception is throwwn when unexpected data is returned
     *
     */
    void validateBlockWriteSectors(BvdSimFileIo_File *file, UINT_64 noSectors, UINT_64 startingSectorLBA, PIOTaskId task = NULL, EMCPAL_STATUS expectedStatus = EMCPAL_STATUS_SUCCESS) {
        if(task == NULL) {
            task = file->getIOTaskID();
        }

        UINT_64 blocksTransferred;
        UINT_32 seed                 = (UINT_32)task->convertLBAToFlareAddress(startingSectorLBA);
        UINT_64 address              = task->convertLBAToFlareAddress(startingSectorLBA);
        IOReference *outputReference = IOSectorManagement::IOReferenceFactory(task, SectorPatternIncrementing, seed, noSectors);

        file->AcquireLBALocks(startingSectorLBA, noSectors);
        outputReference->generateBufferContents(file->GetVolumeIdentifier(), (UINT_32) startingSectorLBA, (PDMC_CONTROL_MEMORY) file->GetVolumeSharedMemory());

        UINT_8 *outputBuffer         = outputReference->getBuffer();

        mut_printf(MUT_LOG_HIGH, "About to write %lld sectors at LBA 0x%llx, starting address 0x%llx ", noSectors, startingSectorLBA, address);
        file->performBlockWrite(outputBuffer, noSectors, &blocksTransferred, startingSectorLBA);

        EMCPAL_STATUS opStatus = (EMCPAL_STATUS)file->getIrpStatus();

        MUT_ASSERT_INTEGER_EQUAL_MSG(expectedStatus, opStatus, "validateBlockWriteSectors returned an incorrect status");

        if(EMCPAL_IS_SUCCESS(opStatus)) {
            MUT_ASSERT_INTEGER_EQUAL_MSG(noSectors, blocksTransferred, "performBlockWrite returned the wrong number of blocks");
        }
        file->ReleaseLBALocks(startingSectorLBA, noSectors);

        IOSectorManagement::destroy(outputReference);
    };

    /*
     * \fn void validateBlockReadSectors()
     * \Param file              - pointer to a BvdSimFileIo_File descriptor 
     * \Param noSectors         - UINT_64 number of sectors to be read
     * \Param startingSectorLBA - Sector LBA of read operation
     *
     * \details 
     *
     * Method validateBlockReadSectors() issues a BlockRead of noSectors at startingSectorLBA from file.  Function verifies
     *      - return status is TRUE
     *      - the correct mount of data was read
     *      - the returned buffer contents contains data that had been generated bo the IOSectorManagment subsystem
     *
     * an exception is throwwn when unexpected data is returned
     *
     */
    void validateBlockReadSectors(BvdSimFileIo_File *file, UINT_64 noSectors, UINT_64 startingSectorLBA, PIOTaskId task = NULL, char *msgPrefix = "performBlockRead", EMCPAL_STATUS expectedStatus = EMCPAL_STATUS_SUCCESS) {
        if(task == NULL) {
            task = file->getIOTaskID();
        }

        UINT_64 blocksTransferred;
        UINT_64 transferLength      = task->convertNoSectorsToFlareLength(noSectors);
        UINT_64 bufferLength        = task->convertFlareLengthToBufferLength(transferLength);
        UINT_64 address             = task->convertLBAToFlareAddress(startingSectorLBA);

        IOBuffer *inputIOB          = IOSectorManagement::IOBufferFactory(bufferLength);
        IOReference *inputReference = IOSectorManagement::IOReferenceFactory(task, inputIOB);

        file->AcquireLBALocks(startingSectorLBA, noSectors);
        mut_printf(MUT_LOG_HIGH, "About to read %lld sectors at LBA 0x%llx, starting address 0x%llx ", noSectors, startingSectorLBA, address);
        file->performBlockRead(inputIOB->getBuffer(), noSectors, &blocksTransferred, startingSectorLBA);

        EMCPAL_STATUS opStatus = (EMCPAL_STATUS)file->getIrpStatus();

        MUT_ASSERT_INTEGER_EQUAL_MSG(expectedStatus, opStatus, "validateBlockReadSectors returned an incorrect status");

        if(EMCPAL_IS_SUCCESS(opStatus)) {
            MUT_ASSERT_INTEGER_EQUAL_MSG(noSectors, blocksTransferred, "performBlockRead returned the wrong number of blocks");
            
            BOOL status = inputReference->identify(file->GetVolumeIdentifier(), (PDMC_CONTROL_MEMORY) file->GetVolumeSharedMemory(), startingSectorLBA, msgPrefix, TRUE);
            MUT_ASSERT_TRUE_MSG(status, "Unable to identify buffer contents after performBlockRead");
        }
        file->ReleaseLBALocks(startingSectorLBA, noSectors);
        IOSectorManagement::destroy(inputReference);
        IOSectorManagement::destroy(inputIOB);
    }
    
    void validateBlockReadSectors(BvdSimFileIo_File *file, UINT_64 noSectors, UINT_64 startingSectorLBA, char *msgPrefix) {
        validateBlockReadSectors(file, noSectors, startingSectorLBA, file->getIOTaskID(), msgPrefix);
    }
    void validateBlockReadSectors(BvdSimFileIo_File *file, UINT_64 noSectors, UINT_64 startingSectorLBA, EMCPAL_STATUS expectedStatus) {
        validateBlockReadSectors(file, noSectors, startingSectorLBA, file->getIOTaskID(), "performBlockRead", expectedStatus);
    }

    /*
     * \fn void validateBlockWriteBlockReadSectors()
     * \Param file              - pointer to a BvdSimFileIo_File descriptor 
     * \Param noSectors         - UINT_64 number of sectors to be written
     * \Param startingSectorLBA - Sector LBA of write operation
     *
     * \details 
     *
     * Method validateBlockWriteBlockReadSectors() performs a validateBlockWriteSectors followed by a validateBlockReadSectors.
     * This convenience medhod performs a complete DCAwrite/DCAread validation on a region of file.
     *
     */
    void validateBlockWriteBlockReadSectors(BvdSimFileIo_File *file, UINT_64 noSectors, UINT_64 startingSectorLBA, PIOTaskId task = NULL, EMCPAL_STATUS expectedStatus = EMCPAL_STATUS_SUCCESS) {
        validateBlockWriteSectors(file, noSectors, startingSectorLBA, task);
        validateBlockReadSectors(file, noSectors, startingSectorLBA, task, "performBlockRead", expectedStatus);
    }

    /*
     * \fn void validateBlockDCAWriteSectors()
     * \Param file              - pointer to a BvdSimFileIo_File descriptor 
     * \Param noSectors         - UINT_64 number of sectors to be written
     * \Param startingSectorLBA - Sector LBA of write operation
     *
     * \details 
     *
     * Method validateBlockDCAWriteSectors() generates an incrementing pattern noSectors long, issues a BlockDCAwrite
     * two write noSectorsto file at startingSectorLBA.  Function verifies 
     *      - return status is TRUE
     *      - the correct mount of data was written
     *
     * an exception is throwwn when unexpected data is returned
     *
     */
    void validateBlockDCAWriteSectors(BvdSimFileIo_File *file, UINT_64 noSectors, UINT_64 startingSectorLBA, EMCPAL_IRP_STACK_FLAGS irpFlags = 0, PIOTaskId task = NULL, EMCPAL_STATUS expectedStatus = EMCPAL_STATUS_SUCCESS) {
        if(task == NULL) {
            task = file->getIOTaskID();
        }

        UINT_64 blocksTransferred;
        UINT_32 seed                 = (UINT_32)task->convertLBAToFlareAddress(startingSectorLBA);
        UINT_64 address              = task->convertLBAToFlareAddress(startingSectorLBA);
        IOReference *outputReference = IOSectorManagement::IOReferenceFactory(task, SectorPatternIncrementing, seed, noSectors);

        outputReference->generateBufferContents(file->GetVolumeIdentifier(), (UINT_32) startingSectorLBA, (PDMC_CONTROL_MEMORY) file->GetVolumeSharedMemory());

        UINT_8 *outputBuffer         = outputReference->getBuffer();


        mut_printf(MUT_LOG_HIGH, "About to write %lld sectors at LBA 0x%llx, starting address 0x%llx ", noSectors, startingSectorLBA, address);
        file->AcquireLBALocks(startingSectorLBA, noSectors);
        file->performBlockDCAWrite(outputBuffer, noSectors, &blocksTransferred, startingSectorLBA, irpFlags);

        EMCPAL_STATUS opStatus = (EMCPAL_STATUS)file->getIrpStatus();

        MUT_ASSERT_INTEGER_EQUAL_MSG(expectedStatus, opStatus, "performBlockDCAWriteSectors returned an incorrect status");

        if(EMCPAL_IS_SUCCESS(opStatus)) {
            MUT_ASSERT_INTEGER_EQUAL_MSG(noSectors, blocksTransferred, "performBlockDCAWrite returned the wrong number of blocks");
        }
        file->ReleaseLBALocks(startingSectorLBA, noSectors);

        IOSectorManagement::destroy(outputReference);
    };

    /*
     * \fn void validateBlockDCAReadSectors()
     * \Param file              - pointer to a BvdSimFileIo_File descriptor 
     * \Param noSectors         - UINT_64 number of sectors to be read
     * \Param startingSectorLBA - Sector LBA of read operation
     *
     * \details 
     *
     * Method validateBlockDCAReadSectors() issues a BlockDCARead of noSectors at startingSectorLBA from file.
     * Function verifies
     *      - return status is TRUE
     *      - the correct mount of data was read
     *      - the returned buffer contents contains data that had been generated bo the IOSectorManagment subsystem
     *
     * an exception is throwwn when unexpected data is returned
     *
     */
    void validateBlockDCAReadSectors(BvdSimFileIo_File *file, UINT_64 noSectors, UINT_64 startingSectorLBA, PIOTaskId task = NULL, char *msgPrefix = "performBlockDCARead", EMCPAL_STATUS expectedStatus = EMCPAL_STATUS_SUCCESS) {
        if(task == NULL) {
            task = file->getIOTaskID();
        }

        UINT_64 blocksTransferred;
        UINT_64 transferLength      = task->convertNoSectorsToFlareLength(noSectors);
        UINT_64 bufferLength        = task->convertFlareLengthToBufferLength(transferLength);
        UINT_64 address             = file->getIOTaskID()->convertLBAToFlareAddress(startingSectorLBA);

        IOBuffer *inputIOB          = IOSectorManagement::IOBufferFactory(bufferLength);
        IOReference *inputReference = IOSectorManagement::IOReferenceFactory(task, inputIOB);

        file->AcquireLBALocks(startingSectorLBA, noSectors);

        mut_printf(MUT_LOG_HIGH, "About to read %lld sectors at LBA 0x%llx, starting address 0x%llx ", noSectors, startingSectorLBA, address);
        file->performBlockDCARead(inputIOB->getBuffer(), noSectors, &blocksTransferred, startingSectorLBA);

        EMCPAL_STATUS opStatus = (EMCPAL_STATUS)file->getIrpStatus();

        MUT_ASSERT_INTEGER_EQUAL_MSG(expectedStatus, opStatus, "performBlockDCARead returned an incorrect status");

        if(EMCPAL_IS_SUCCESS(opStatus)) {
            MUT_ASSERT_INTEGER_EQUAL_MSG(noSectors, blocksTransferred, "performBlockDCARead returned the wrong number of blocks");

            BOOL status = inputReference->identify(file->GetVolumeIdentifier(), (PDMC_CONTROL_MEMORY) file->GetVolumeSharedMemory(), startingSectorLBA, msgPrefix, TRUE);
            MUT_ASSERT_TRUE_MSG(status, "Unable to identify buffer contents after performBlockDCARead");
        }
        file->ReleaseLBALocks(startingSectorLBA, noSectors);

        IOSectorManagement::destroy(inputReference);
        IOSectorManagement::destroy(inputIOB);
    }

    void validateBlockDCAReadSectors(BvdSimFileIo_File *file, UINT_64 noSectors, UINT_64 startingSectorLBA, char *msgPrefix) {
        validateBlockDCAReadSectors(file, noSectors, startingSectorLBA, file->getIOTaskID(), msgPrefix, EMCPAL_STATUS_SUCCESS);
    }

    void validateBlockDCAReadSectors(BvdSimFileIo_File *file, UINT_64 noSectors, UINT_64 startingSectorLBA, EMCPAL_STATUS expectedStatus) {
        validateBlockDCAReadSectors(file, noSectors, startingSectorLBA, file->getIOTaskID(), "performBlockDCARead", expectedStatus);
    }

    /*
     * \fn void validateBlockDCAWriteBlockDCAReadSectors()
     * \Param file              - pointer to a BvdSimFileIo_File descriptor 
     * \Param noSectors         - UINT_64 number of sectors to be written
     * \Param startingSectorLBA - Sector LBA of write operation
     *
     * \details 
     *
     * Method validateBlockDCAWriteBlockDCAReadSectors() performs 
     * a validateBlockDCAWriteSectors followed by a validateBlockDCAReadSectors.
     * This convenience medhod performs a complete BlockDCAwrite/BlockDCAread validation on a region of file.
     *
     */
    void validateBlockDCAWriteBlockDCAReadSectors(BvdSimFileIo_File *file, UINT_64 noSectors, UINT_64 startingSectorLBA, PIOTaskId task = NULL, EMCPAL_STATUS expectedStatus = EMCPAL_STATUS_SUCCESS) {
        validateBlockDCAWriteSectors(file, noSectors, startingSectorLBA, 0, task);
        validateBlockDCAReadSectors(file, noSectors, startingSectorLBA, task, "performBlockDCARead", expectedStatus);
    }    

    /*
     * \fn void validateSGLWriteSectors()
     * \Param file              - pointer to a BvdSimFileIo_File descriptor.
     * \Param noSectors         - UINT_64 number of sectors to be written.
     * \Param startingSectorLBA - Sector LBA of write operation.
     * \Param sglElementCount   - Number of elements in the SGL.
     * \sglFlags                - Optional.
     * \expectedStatus          - Optional.
     *
     * \details 
     *
     * Method validateSGLWriteSectors() generates an incrementing pattern noSectors 
     * long into an SGL with sglElementCount elements.  The data is written to a file
     * at startingSectorLBA. 
     *
     * Function verifies 
     *      - return status is TRUE.
     *
     * An exception is thrown when unexpected data is returned.
     *
     */
    void validateSGLWriteSectors(BvdSimFileIo_File *file, UINT_64 noSectors, UINT_64 startingSectorLBA,
        UINT_16 sglElementCount, UINT_16 sglFlags = 0, UINT_8 irpFlags = 0,
        UINT_8 sglType = SGL_VIRTUAL_ADDRESSES,
        EMCPAL_STATUS expectedStatus = EMCPAL_STATUS_SUCCESS)
    {
        // The back end could be either 520 (typical VNX/VNXe deployment)
        // or 512 (e.g., virtual VNX).
        if (file->getIOTaskID()->getTaskType() == IOTask_Type_520)
        {
            sglType |= SGL_SKIP_SECTOR_OVERHEAD_BYTES;
        }
        VolumeIdentifier uniqueDiskIdentifier = file->GetVolumeIdentifier();


        // There must be at least one sector per SGL element.
        if (noSectors < sglElementCount)
        {
            mut_printf(MUT_LOG_HIGH, "Incorrect sector / SG Element count, %lld, %d\n", noSectors, sglElementCount);
            FF_ASSERT(false);
        }

        // Variable definitions.
        UINT_32 seed = (UINT_32)file->getIOTaskID()->convertLBAToFlareAddress(startingSectorLBA);
        UINT_64 transferLength = file->getIOTaskID()->convertNoSectorsToFlareLength(noSectors);
        UINT_64 address = file->getIOTaskID()->convertLBAToFlareAddress(startingSectorLBA);

        FLARE_SGL_INFO sglInfo;
        IOReference *pOutputReference = IOSectorManagement::IOReferenceFactory(file->getIOTaskID(),
                                                       SectorPatternIncrementing, seed, noSectors);

        file->AcquireLBALocks(startingSectorLBA, noSectors);

        // Build SGL IO buffer.
        pOutputReference->generateSglBufferContents(uniqueDiskIdentifier, (UINT_32) startingSectorLBA, (PDMC_CONTROL_MEMORY) file->GetVolumeSharedMemory(), IOTask_OP_SglWrite, sglElementCount);

        // Setup Flare SGL info.
        EmcpalZeroMemory(&sglInfo, sizeof(FLARE_SGL_INFO));
        sglInfo.SglType = sglType;
        sglInfo.Flags = sglFlags;
        sglInfo.SGList = (PSGL) pOutputReference->getSGLBuffer()->getBuffer();

        // Send an SGL write.
        mut_printf(MUT_LOG_HIGH, "About to write %d element SGL of %lld sectors at LBA 0x%llx, start address 0x%llx ", 
                                                             sglElementCount, (long long)noSectors, (unsigned long long)startingSectorLBA, (unsigned long long)address);

        BOOL status = file->performSGLWrite(&sglInfo, transferLength, address, irpFlags);

        // Handle status.  
        EMCPAL_STATUS opStatus = (EMCPAL_STATUS)file->getIrpStatus();

        MUT_ASSERT_INTEGER_EQUAL_MSG(expectedStatus, opStatus, "performSGLWrite returned an incorrect status");

        if(EMCPAL_IS_SUCCESS(opStatus))
        {
            MUT_ASSERT_TRUE_MSG(status, "performSGLWrite failed, expected status = TRUE");
        }
        file->ReleaseLBALocks(startingSectorLBA, noSectors);

        // Cleanup.
        IOSectorManagement::destroy(pOutputReference);

        return;

    };  /* End validateSGLWriteSectors() */

    /*
     * \fn void validateSGLWriteSectorsAsync()
     * \Param file              - pointer to a BvdSimFileIo_File descriptor.
     * \Param noSectors         - UINT_64 number of sectors to be written.
     * \Param startingSectorLBA - Sector LBA of write operation.
     * \Param sglElementCount   - Number of elements in the SGL.
     * \Param asyncBlock        - Async I/O Block
     * \sglFlags                - Optional.
     * \expectedStatus          - Optional.
     *
     * \details 
     *
     * Method validateSGLWriteSectorsAsync() generates an incrementing pattern noSectors 
     * long into an SGL with sglElementCount elements.  The data is written to a file
     * at startingSectorLBA. 
     *
     *
     */
    void validateSGLWriteSectorsAsync(BvdSimFileIo_File *file, UINT_64 noSectors, UINT_64 startingSectorLBA,
                                      UINT_16 sglElementCount, PBvdSimFile_IoAsyncBlock asyncBlock,
                                 UINT_16 sglFlags = 0,
                                 UINT_8 sglType = SGL_VIRTUAL_ADDRESSES | SGL_SKIP_SECTOR_OVERHEAD_BYTES,
                                 EMCPAL_STATUS expectedStatus = EMCPAL_STATUS_SUCCESS)
    {
        // IOTask uses 520 byte sectors.
        FF_ASSERT(file->getIOTaskID()->getTaskType() == IOTask_Type_520);
        VolumeIdentifier uniqueDiskIdentifier = file->GetVolumeIdentifier();
    
    
        // There must be at least one sector per SGL element.
        if (noSectors < sglElementCount)
        {
            mut_printf(MUT_LOG_HIGH, "Incorrect sector / SG Element count, %lld, %d\n", noSectors, sglElementCount);
            FF_ASSERT(false);
        }
    
        // Variable definitions.
        UINT_32 seed = (UINT_32)file->getIOTaskID()->convertLBAToFlareAddress(startingSectorLBA);
        UINT_64 transferLength = file->getIOTaskID()->convertNoSectorsToFlareLength(noSectors);
        UINT_64 address = file->getIOTaskID()->convertLBAToFlareAddress(startingSectorLBA);
    
        FLARE_SGL_INFO sglInfo;
        IOReference *pOutputReference = IOSectorManagement::IOReferenceFactory(file->getIOTaskID(),
                                        SectorPatternIncrementing, seed, noSectors);
        asyncBlock->mIoReference = pOutputReference;
        
        file->AcquireLBALocks(startingSectorLBA, noSectors);
    
        // Build SGL IO buffer.
        pOutputReference->generateSglBufferContents(uniqueDiskIdentifier, (UINT_32) startingSectorLBA, (PDMC_CONTROL_MEMORY) file->GetVolumeSharedMemory(), IOTask_OP_SglWrite, sglElementCount);
    
        // Setup Flare SGL info.
        EmcpalZeroMemory(&sglInfo, sizeof(FLARE_SGL_INFO));
        sglInfo.SglType = sglType;
        sglInfo.Flags = sglFlags;
        sglInfo.SGList = (PSGL) pOutputReference->getSGLBuffer()->getBuffer();
    
        // Send an SGL write.
        mut_printf(MUT_LOG_HIGH, "About to write ASYNC %d element SGL of %lld sectors at LBA 0x%llx, start address 0x%llx ", 
                   sglElementCount, (long long)noSectors, (unsigned long long)startingSectorLBA, (unsigned long long)address);
    
        (void) file->performSGLWriteAsync(&sglInfo, transferLength, address, file->getIOTaskID(), asyncBlock);
        
    };  /* End validateSGLWriteSectors() */
    
    /*
     * \fn void validateSGLReadSectors()
     * \Param file              - pointer to a BvdSimFileIo_File descriptor.
     * \Param noSectors         - UINT_64 number of sectors to be read.
     * \Param startingSectorLBA - Sector LBA of read operation.
     * \Param sglElementCount   - Number of elements in the SGL.
     * \sglFlags                - Optional.
     * \expectedStatus          - Optional.
     *
     * \details 
     *
     * Method validateSGLReadSectors() reads noSectors into an SGL with sglElementCount elements 
     * from a file at startingSectorLBA.
     *
     * Function verifies
     *      - return status is TRUE.
     *      - the returned buffer contents contains data that had been generated by the 
     *        IOSectorManagment subsystem.
     *
     * An exception is thrown when unexpected data is returned.
     *
     */
    void validateSGLReadSectors(BvdSimFileIo_File *file, UINT_64 noSectors, UINT_64 startingSectorLBA,
                            UINT_16 sglElementCount, UINT_16 sglFlags = 0, UINT_8 irpFlags = 0,
                            UINT_8 sglType = SGL_VIRTUAL_ADDRESSES,
                            EMCPAL_STATUS expectedStatus = EMCPAL_STATUS_SUCCESS)
    {
        // The back end could be either 520 (typical VNX/VNXe deployment)
        // or 512 (e.g., virtual VNX).
        if (file->getIOTaskID()->getTaskType() == IOTask_Type_520)
        {
            sglType |= SGL_SKIP_SECTOR_OVERHEAD_BYTES;
        }

        // There must be at least one sector per SGL element.
        if (noSectors < sglElementCount)
        {
            mut_printf(MUT_LOG_HIGH, "Incorrect sector / SG Element count, %lld, %d\n", noSectors, sglElementCount);
            FF_ASSERT(false);
        }

        VolumeIdentifier uniqueDiskIdentifier = file->GetVolumeIdentifier();

        // Variable definitions.
        UINT_64 transferLength = file->getIOTaskID()->convertNoSectorsToFlareLength(noSectors);
        UINT_64 bufferLength = file->getIOTaskID()->convertFlareLengthToBufferLength(transferLength);
        UINT_64 address = file->getIOTaskID()->convertLBAToFlareAddress(startingSectorLBA);

        FLARE_SGL_INFO sglInfo;
        IOBuffer *pInputIOB = IOSectorManagement::IOBufferFactory(bufferLength);
        IOReference *pInputReference = IOSectorManagement::IOReferenceFactory(file->getIOTaskID(), pInputIOB);

        file->AcquireLBALocks(startingSectorLBA, noSectors);
        
        // Build SGL IO buffer.
        pInputReference->generateSglBufferContents(uniqueDiskIdentifier, (UINT_32) startingSectorLBA, (PDMC_CONTROL_MEMORY) file->GetVolumeSharedMemory(), IOTask_OP_SglRead, sglElementCount);

        // Setup Flare SGL info.
        EmcpalZeroMemory(&sglInfo, sizeof(FLARE_SGL_INFO));
        sglInfo.SglType = sglType;
        sglInfo.Flags = sglFlags;
        sglInfo.SGList = (PSGL) pInputReference->getSGLBuffer()->getBuffer();

        // Send an SGL read.
        mut_printf(MUT_LOG_HIGH, "About to read %d element SGL of %lld sectors at LBA 0x%llx, start address 0x%llx ",
                                                            sglElementCount, (long long)noSectors, (unsigned long long)startingSectorLBA, (unsigned long long)address);

        BOOL status = file->performSGLRead(&sglInfo, transferLength, address, irpFlags);


        // Handle status.
        EMCPAL_STATUS opStatus = (EMCPAL_STATUS)file->getIrpStatus();

        MUT_ASSERT_INTEGER_EQUAL_MSG(expectedStatus, opStatus, "performSGLRead returned an incorrect status");
        
        if(EMCPAL_IS_SUCCESS(opStatus))
        {
            MUT_ASSERT_TRUE_MSG(status, "performSGLRead failed, expected status = TRUE");

            status = pInputReference->identify(file->GetVolumeIdentifier(), (PDMC_CONTROL_MEMORY) file->GetVolumeSharedMemory(), startingSectorLBA, "performSGLRead", TRUE);

            MUT_ASSERT_TRUE_MSG(status, "Unable to identify buffer contents after performSGLRead");
        }

        // Cleanup.
        file->ReleaseLBALocks(startingSectorLBA, noSectors);
        IOSectorManagement::destroy(pInputReference);
        IOSectorManagement::destroy(pInputIOB);
        return;

    } /* validateSGLReadSectors() */

    /*
     * \fn void validateSGLReadSectorsAsync()
     * \Param file              - pointer to a BvdSimFileIo_File descriptor.
     * \Param noSectors         - UINT_64 number of sectors to be read.
     * \Param startingSectorLBA - Sector LBA of read operation.
     * \Param asyncBlock        - Async I/O Block
     * \Param sglElementCount   - Number of elements in the SGL.
     * \sglFlags                - Optional.
     * \expectedStatus          - Optional.
     *
     * \details 
     *
     * Method validateSGLReadSectors() reads noSectors into an SGL with sglElementCount elements 
     * from a file at startingSectorLBA.
     *
     */
    void validateSGLReadSectorsAsync(BvdSimFileIo_File *file, UINT_64 noSectors, UINT_64 startingSectorLBA,
                                UINT_16 sglElementCount, PBvdSimFile_IoAsyncBlock asyncBlock, UINT_16 sglFlags = 0,
                                UINT_8 sglType = SGL_VIRTUAL_ADDRESSES | SGL_SKIP_SECTOR_OVERHEAD_BYTES,
                                EMCPAL_STATUS expectedStatus = EMCPAL_STATUS_SUCCESS)
    {
        // IOTask uses 520 byte sectors.
        FF_ASSERT(file->getIOTaskID()->getTaskType() == IOTask_Type_520);
    
        // There must be at least one sector per SGL element.
        if (noSectors < sglElementCount)
        {
            mut_printf(MUT_LOG_HIGH, "Incorrect sector / SG Element count, %lld, %d\n", noSectors, sglElementCount);
            FF_ASSERT(false);
        }
    
        VolumeIdentifier uniqueDiskIdentifier = file->GetVolumeIdentifier();
    
        // Variable definitions.
        UINT_64 transferLength = file->getIOTaskID()->convertNoSectorsToFlareLength(noSectors);
        UINT_64 bufferLength = file->getIOTaskID()->convertFlareLengthToBufferLength(transferLength);
        UINT_64 address = file->getIOTaskID()->convertLBAToFlareAddress(startingSectorLBA);
    
        FLARE_SGL_INFO sglInfo;
        IOBuffer *pInputIOB = IOSectorManagement::IOBufferFactory(bufferLength);
        IOReference *pInputReference = IOSectorManagement::IOReferenceFactory(file->getIOTaskID(), pInputIOB);
        asyncBlock->mIoBuffer = pInputIOB;
        
        file->AcquireLBALocks(startingSectorLBA, noSectors);
        
        // Build SGL IO buffer.
        pInputReference->generateSglBufferContents(uniqueDiskIdentifier, (UINT_32) startingSectorLBA, (PDMC_CONTROL_MEMORY) file->GetVolumeSharedMemory(), IOTask_OP_SglRead, sglElementCount);
    
        // Setup Flare SGL info.
        EmcpalZeroMemory(&sglInfo, sizeof(FLARE_SGL_INFO));
        sglInfo.SglType = sglType;
        sglInfo.Flags = sglFlags;
        sglInfo.SGList = (PSGL) pInputReference->getSGLBuffer()->getBuffer();
    
        // Send an SGL read.
        mut_printf(MUT_LOG_HIGH, "About to read ASYNC%d element SGL of %lld sectors at LBA 0x%llx, start address 0x%llx ",
                   sglElementCount, (long long)noSectors, (unsigned long long)startingSectorLBA, (unsigned long long)address);
    
        (void) file->performSGLReadAsync(&sglInfo, transferLength, address, file->getIOTaskID(), asyncBlock);
    
    
        // Cleanup.
        file->ReleaseLBALocks(startingSectorLBA, noSectors);
        return;
    
    } /* validateSGLReadSectors() */
    
    /*
     * \fn void validateSGLWriteSGLReadSectors()
     * \Param file              - pointer to a BvdSimFileIo_File descriptor 
     * \Param noSectors         - UINT_64 number of sectors to be written
     * \Param startingSectorLBA - Sector LBA of operation.
     * \Param sglElementCount   - Number of elements in the SGL.
     * \sglFlags                - Optional.
     * \expectedStatus          - Optional.
     *
     * \details 
     *
     * Method validateSGLWriteSGLReadSectors() performs a validateSGLWriteSectors followed
     * by a validateSGLReadSectors.  This convenience method performs a complete SGL Write /
     * Read request on an lba range.  
     *
     */
    void validateSGLWriteSGLReadSectors(BvdSimFileIo_File *file, UINT_64 noSectors, UINT_64 startingSectorLBA,
                                        UINT_8 sglElementCount, UINT_16 sglFlags = 0,
                                        UINT_8 sglType = SGL_VIRTUAL_ADDRESSES | SGL_SKIP_SECTOR_OVERHEAD_BYTES,
                                        EMCPAL_STATUS expectedStatus = EMCPAL_STATUS_SUCCESS)
    {
        validateSGLWriteSectors(file, noSectors, startingSectorLBA, sglElementCount, sglFlags, 0, sglType, expectedStatus);
        validateSGLReadSectors(file, noSectors, startingSectorLBA, sglElementCount, sglFlags, 0, sglType, expectedStatus);
        return;

    } /* End validateSGLWriteSGLReadSectors() */

};


#endif  //__BVDSIMIOUTILITYBASECLASS__

