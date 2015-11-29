/***************************************************************************
 * Copyright (C) EMC Corporation 2013-2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  DESCRIPTION
 *     This file defines the interfaces for advanced DMC detection
 *
 *
 *  HISTORY
 *     25-Jun-2013     Created.   - MJC
 *
 * NOTES:
 *   We can reduce the amount of memory we use by removing the 64-bit lock
 *   from DMC_SEQUENCE_CONTROL and creating a new structure called DMA_LOCK_CONTROL.
 *   since we only have 1 lock protecting 128 sectors why have a 64-bit lock
 *   in for every sector.    We can go from using 800 pages per disk to about
 *   210 pages per disk.   ***FIX ME*****
 ***************************************************************************/
#ifndef BVDSIM_DMC_H_
#define BVDSIM_DMC_H_

#include "simulation/shmem.h"
#include "simulation/AbstractDisk.h"
#include "EmcPAL_Interlocked.h"

#define BvdSim_Track_Disk_Limit 10               // Maximum number disks to do advanced DMC checking on.
#define DMC_CONTROL_MEMORY_VERSION_1    1       // Current DMC Control Memory Version

// Format for shared memory name. Where the %d is the volume number and the %x is
// the unique master process processid.
#define DMC_DISK_SHARED_MEMORY_NAME_FORMAT "VirtualDisk%d_%x"

#define DMC_CONTROL_MEMORY_MAGIC 0xF2F3F1F0

const UINT_32 DMC_LOCK_VALUE = 0x00000008;
const UINT_32 DMC_FREE_VALUE = 0x00000000;

const UINT_32 DMC_SECTORS_PER_LOCK = 128; // 0x80 hex

typedef CSX_ALIGN_4 struct _DMC_LBA_LOCK_CONTROL {
    volatile UINT_32 mLock;

    __inline void AcquireLock() {
        ULONG count = 0;
        while(EmcpalInterlockedCompareExchange32((INT_32*) &mLock, (INT_32) DMC_LOCK_VALUE, (INT_32) DMC_FREE_VALUE)) {
            if(!((++count+1) % 100)) {
                csx_p_thr_superyield();
            }
        }
        //FF_ASSERT(HasLock());
    };

    __inline void ReleaseLock() {
        //FF_ASSERT(HasLock());
        mLock = (INT_32) DMC_FREE_VALUE;
    };

    __inline bool HasLock() {
        return (mLock == DMC_LOCK_VALUE);
    };


} DMC_LBA_LOCK_CONTROL, *PDMC_LBA_LOCK_CONTROL;

typedef CSX_ALIGN_N(2) struct _DMC_LBA_SEQUENCE_CONTROL {
    volatile USHORT   mSequence;

    __inline ULONG IncrementSequence() {
        mSequence++;
        if(mSequence == 0xFFFF) {
            mSequence = 1;
        }
        return (ULONG) mSequence;
    };

    __inline bool CheckSequence(ULONG Sequence) const {
        if(Sequence == (ULONG) mSequence) {
            return true;
        }
        if(mSequence == 0) {
            if(Sequence != 0) {
                return false;
            }
        }
        return true;
    };

    __inline ULONG GetSequence() const {
        return (ULONG) mSequence;
    }

    __inline bool    IsZeroSequence() {
        return mSequence == 0;
    }

    __inline void    ResetSequence() {
        mSequence = 0;
    }

} DMC_LBA_SEQUENCE_CONTROL, *PDMC_LBA_SEQUENCE_CONTROL;

// This class is the base for doing advanced data miss compare testing.
// We use this class to manage a block of shared memory for a tracked
// disk.   This memory contains a lock for a range of sectors and
// contains a sequence count which is updated each time a sector is
// modified.  When the sector is read back, a caller can check the
// sequence number in the sector against the one contained in this
// memory block to ensure that the data retrieved is the correct
// sequence.
//
//
class DMC_CONTROL_MEMORY {
public:
    DMC_CONTROL_MEMORY(ULONG diskIndex, UINT_64 diskSize) : mBlocks(diskSize),
      mMagicNumber(DMC_CONTROL_MEMORY_MAGIC),
      mVersion(DMC_CONTROL_MEMORY_VERSION_1),
      mFlags(0),
      mVolumeIndex(diskIndex)
    {
        mLbaLocksOffset = (sizeof(DMC_CONTROL_MEMORY) + sizeof(UINT_64)) & ~(sizeof(UINT_64)-1);
        mLbasOffset = ((mLbaLocksOffset + ((diskSize/(UINT_64) DMC_SECTORS_PER_LOCK)+1) * sizeof(DMC_LBA_LOCK_CONTROL) + sizeof(UINT_64)) & ~(sizeof(UINT64)-1));
    };

    ULONG   mVersion;
    ULONG   mMagicNumber;
    ULONG   mFlags;
    ULONG   mVolumeIndex;
    UINT_64 mBlocks;
    UINT_64 mLbaLocksOffset;
    UINT_64 mLbasOffset;


    // Acquire the LBA lock(s) associated with the starting lba
    // and sectors we are going to write. One lock covers DMC_SECTORS_PER_LOCK
    // sectors.
    __inline void    AcquireLBALocks(UINT_32 lba, UINT_32 sectors) {
        UINT_32 blockLba = lba / DMC_SECTORS_PER_LOCK;
        
        // determine how many locks needed rounding up for unaligned requests
        UINT_32 countLbaLocks = ((lba % DMC_SECTORS_PER_LOCK) + sectors + DMC_SECTORS_PER_LOCK - 1)/ DMC_SECTORS_PER_LOCK;

        DMC_LBA_LOCK_CONTROL* pLbaLocks = (DMC_LBA_LOCK_CONTROL*) ((UINT_64) this + mLbaLocksOffset);
        for(UINT_32 index = 0; index < countLbaLocks; index++) {
            pLbaLocks[blockLba+index].AcquireLock();
        }
    };

    // Release the LBA lock(s) associated with the starting lba
    // and sectors we are going to write. One lock covers DMC_SECTORS_PER_LOCK
    // sectors.
    __inline void    ReleaseLBALocks(UINT_32 lba, UINT_32 sectors) {
        UINT_32 blockLba = lba / DMC_SECTORS_PER_LOCK;
        
        // determine how many locks needed rounding up for unaligned requests
        UINT_32 countLbaLocks = ((lba % DMC_SECTORS_PER_LOCK) + sectors + DMC_SECTORS_PER_LOCK - 1)/ DMC_SECTORS_PER_LOCK;

        DMC_LBA_LOCK_CONTROL* pLbaLocks = (DMC_LBA_LOCK_CONTROL*) ((UINT_64) this + mLbaLocksOffset);
        for(UINT_32 index = 0; index < countLbaLocks; index++) {
            pLbaLocks[blockLba+index].ReleaseLock();
        }
    };

    __inline ULONG   IncrementSequence(UINT_32 lba) {
        DMC_LBA_SEQUENCE_CONTROL*  pLbas = (DMC_LBA_SEQUENCE_CONTROL*) ((UINT_64) this + mLbasOffset);
        return pLbas[lba].IncrementSequence();
    };

    __inline bool CheckSequence(UINT_32 lba, ULONG sequence) {
        DMC_LBA_SEQUENCE_CONTROL*  pLbas = (DMC_LBA_SEQUENCE_CONTROL*) ((UINT_64) this + mLbasOffset);
        return pLbas[lba].CheckSequence(sequence);
    };

    __inline ULONG GetSequence(UINT_32 lba) const {
        DMC_LBA_SEQUENCE_CONTROL*  pLbas = (DMC_LBA_SEQUENCE_CONTROL*) ((UINT_64) this + mLbasOffset);
        return pLbas[lba].GetSequence();
    };

    __inline bool HasLock(UINT_32 lba) {
        UINT_32 blockLba = lba & ~(DMC_SECTORS_PER_LOCK-1);
        DMC_LBA_LOCK_CONTROL* pLbaLocks = (DMC_LBA_LOCK_CONTROL*) ((UINT_64) this + mLbaLocksOffset);
        return pLbaLocks[blockLba].HasLock();
    };

    __inline bool    IsZeroSequence(UINT_32 lba) {
        DMC_LBA_SEQUENCE_CONTROL*  pLbas = (DMC_LBA_SEQUENCE_CONTROL*) ((UINT_64) this + mLbasOffset);
        return pLbas[lba].IsZeroSequence();
    };

    __inline void ResetSequence(UINT_32 lba) {
        DMC_LBA_SEQUENCE_CONTROL*  pLbas = (DMC_LBA_SEQUENCE_CONTROL*) ((UINT_64) this + mLbasOffset);
        pLbas[lba].ResetSequence();
    }

    // lba1 and sectors1 is ASSUMED to represent the lock range currently held, while lba2 and sectors2 are assumed to
    // represent a range that we would like to acquire.   NO CURRENT LOCKS ARE LOOKED AT, THIS CODE JUST INFORMS
    // THE CALLER THAT THE DATA RANGE DESCRIBED BY LBA1/SECTORS1 and LBA2/SECTORS2 .   If the code returns TRUE, it indicates
    // that there would be a lock conflict and neededLock and neededSectors indicates any additional locks that must be acquired
    // to make the operation safe.  If neededLock and NeededSectors are both zero, it indicates that the locks that protected
    // LBA1/SECTOR1 are sufficient to protect LBA2/SECTOR2.
    //
    // Note, this code is not perfect and does not handle every case e.g the range described by LBA2/SECTOR2 encapsulates
    // LBA1/SECTOR1 such that a locked is needed that procedes LBA1 and another lock is needed that follows the range.
    //
    __inline bool   HasLBARangeLockOverlaps(UINT_32 lba1, UINT_32 sectors1, UINT_32 lba2, UINT_32 sectors2, UINT_32* neededLock, UINT_32* neededSectors) {
        UINT_32 blockLba1 = lba1 / DMC_SECTORS_PER_LOCK;
        UINT_32 countLbaLocks1 = ((lba1 % DMC_SECTORS_PER_LOCK) + sectors1 + DMC_SECTORS_PER_LOCK - 1)/ DMC_SECTORS_PER_LOCK;
        UINT_32 blockLba2 = lba2 / DMC_SECTORS_PER_LOCK;
        UINT_32 countLbaLocks2 = ((lba2 % DMC_SECTORS_PER_LOCK) + sectors2 + DMC_SECTORS_PER_LOCK - 1)/ DMC_SECTORS_PER_LOCK;

        *neededLock = 0;
        *neededSectors = 0;

        if(CSX_MAX(blockLba1, blockLba2) <= CSX_MIN(blockLba1+countLbaLocks1-1, blockLba2+countLbaLocks2-1)) {
            if(blockLba1 == blockLba2) {
                if(countLbaLocks1 == countLbaLocks2) {
                    // Indicate we have an overlap but since neededLock and neededSectors are 0, the caller
                    // knows that the same lock is being used and does not have to be acquired.
                    return true;
                }
                // The starting blockLbas match, but the sector counts don't
                if(countLbaLocks1 > countLbaLocks2) {
                    // We own more locks then the 2nd range wants so since neededLock and neededSectors are 0, the caller
                    // knows that the same lock is being used and does not have to be acquired.
                    return true;
                }

                // We need more locks than are current locked.   Tell the caller what they need to lock and how much.
                *neededLock = blockLba1 + (countLbaLocks1 * DMC_SECTORS_PER_LOCK);
                *neededSectors = ((countLbaLocks2 - countLbaLocks1) * DMC_SECTORS_PER_LOCK);
                return true;
            }
            else if(blockLba2 > blockLba1) {
                // blockLba2 is greater than blockLba1, so lba1s' range must overlap lba2's
                CSX_PANIC_1("blockLba2 > blockLba1");
            } else {
                // "blockLba2 < blockLba1" so see if lba2s' range overlaps the current lock.
                if((blockLba2 + countLbaLocks2-1) < blockLba1) {
                    return false;
                }
                // (blockLba2 + countLbaLocks2-1)s' range overlaps blockLba1, so we must determine
                // how much of an overlap there is and handle it.
                if((blockLba2 + countLbaLocks2-1) > (blockLba1 + countLbaLocks1-1)) {
                    // (blockLba2 + countLbaLocks2)s'covers (blockLba1 + countLbaLocks1)s' range so
                    // we don't need to acquire any locks since they are already held.  Since neededLock
                    // and neededSectors are 0, the caller knows that the same lock is being used and does
                    // not have to be acquired
                    return true;
                }
                // So blockLba2 < blockLba1
                return true;

            }
        }

        return false;
    }

private:
    __inline void    AcquireLock(UINT_32 lba) {
        DMC_LBA_LOCK_CONTROL* pLbaLocks = (DMC_LBA_LOCK_CONTROL*) ((UINT_64) this + mLbaLocksOffset);
        pLbaLocks[lba].AcquireLock();
    };

    __inline void    ReleaseLock(UINT_32 lba) {
        DMC_LBA_LOCK_CONTROL* pLbaLocks = (DMC_LBA_LOCK_CONTROL*) ((UINT_64) this + mLbaLocksOffset);
        pLbaLocks[lba].ReleaseLock();
    };

    // Below is how the different arrays will be laid out in memory.
    //DMC_LBA_LOCK_CONTROL mLbaLocks[(DISK_SIZE_IN_SECTORS/DMC_SECTORS_PER_LOCK)+1];
    //DMC_LBA_SEQUENCE_CONTROL mLbas[DISK_SIZE_IN_SECTORS];
};

typedef DMC_CONTROL_MEMORY* PDMC_CONTROL_MEMORY;

class CacheDMCSharedMemory {
public:
    CacheDMCSharedMemory() : mMemoryAddress(0), mMemorySize(0) {}
    ~CacheDMCSharedMemory() {}


    enum MEM_STATE {
        MEM_ERROR =  1,
        MEM_NO_ADDRESS = 2,
        MEM_EXISTS = 2,
        MEM_NEW    = 3,
    } ;

    // Create of block of shared memory if it does not already exist.  If
    // it already exists return a pointer to the existing block.
    // @param name of the shared memory region to create
    // @param memSize - size of the shared memory block created in bytes.
    enum MEM_STATE Create(char* const memName,UINT_32 memSize)
    {
        enum MEM_STATE result = MEM_ERROR;

        if(!memName || !memSize) {
            return result;
        }

        // See if we have already mapped memory with this class.
        if(mMemoryAddress)
        {
            if(memSize != mMemorySize) {
                return result;
            }
            return MEM_EXISTS;
        }

        // Try to open up a block of shared memory that already exists
        shmem_segment_desc* pSegDesc =
                shmem_get_segment_using_id(shmem_format_segment_id(memName,memSize));

        // If the segment does not already exist then we will try to create it.  If we can't we fail.
        if(!pSegDesc) {
            // Try to create the shared memory segment because it does not exist.
            pSegDesc = shmem_segment_create(memName, memSize);
            if(!pSegDesc) {
                return result;
            }
            result = MEM_NEW;
        } else {
            result = MEM_EXISTS;
        }

        // Get the base address of where the shared memory starts
        mMemoryAddress = shmem_segment_get_base(pSegDesc);

        if(mMemoryAddress) {
            mMemorySize = memSize;
            return result;
        }

        return MEM_NO_ADDRESS;
    }

    // Attempt to get an address to an already existing block of shared memory
    // @param name of the shared memory region to create
    BOOL OpenExisting(char* const memName, UINT_32 memSize)
    {
        if(!memName || !memSize) {
            return false;
        }

        // Make sure that we have not already mapped memory with this class
        if(mMemoryAddress) {
            if(memSize != mMemorySize) {
                return false;
            }
            return true;
        }

        // Try to open up a block of shared memory that already exists
        shmem_segment_desc* pSegDesc =
            shmem_get_segment_using_id(shmem_format_segment_id(memName, memSize));

        // Get the base address of where the shared memory starts
        mMemoryAddress = shmem_segment_get_base(pSegDesc);

        if(mMemoryAddress) {
            mMemorySize = memSize;
            return true;
        }

        return false;
    }

    // Return the address of the shared memory block that was allocated.
    void* GetMemoryAddress()
    {
        return mMemoryAddress;
    }

private:
    void*   mMemoryAddress;
    UINT_32 mMemorySize;
};

// Size of DMC_CONTROL_MEMORY To be allocated.   Add and extra 128 bytes so that LOCK and SEQUENCE lists can be aligned on 8 byte boundary
#define DMC_CONTROL_MEMORY_SIZE  (sizeof(DMC_CONTROL_MEMORY) + ((DEFAULT_DISK_SIZE_IN_SECTORS/DMC_SECTORS_PER_LOCK)+1) + DEFAULT_DISK_SIZE_IN_SECTORS + 16)


#endif /* BVDSIM_DMC_H_ */
