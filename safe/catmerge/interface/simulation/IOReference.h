/**************************************************************************
 * Copyright (C) EMC Corporation 2009-2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               IOSectorManagement.h
 ***************************************************************************
 *
 * DESCRIPTION:  Definitions for the BVDSim IO Sector Management Facility
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    01/27/2010  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef IOREFERENCE
#define IOREFERENCE

# include "generic_types.h"
# include "simulation/IOBuffer.h"
# include "simulation/IOSectorIdentity.h"
# include "simulation/BvdSim_dmc.h"

#if defined(WDMSERVICES_EXPORT)
#define WDMSERVICEPUBLIC CSX_MOD_EXPORT
#else
#define WDMSERVICEPUBLIC CSX_MOD_IMPORT
#endif
# include "simulation/Runnable.h"

typedef enum IOReference_State_e {
    IORefState_initialized,
    IORefState_In_Identify,
    IORefState_In_Compare
} IOReference_State_t;


class IOSector_DM;
class IOReference;

// The _SECTOR_LAYOUT structure defines the layout of each sector written to disk.
// The information contained within the structure is used to verify the sector when
// it is read.   As of today the sector contains the following information:
//
// Identifier - this is a 128 bit field consisting of a sector identifier and a generation
//  number.   The sector identifier uniquely identifies the creator of the sector (i.e. user).
//  The generation number is intended to be incremented each time an SP vaults to ensure that
//  the correct version of the data was vaulted.
//
// DiskIdentifier - this is the WWN of the volume to which this sector was written by the
//  creator of the sector (i.e. the user).   If the sector was never written the it will contain a WWN of 0.
//
// SequenceNo - this is intended to be used as a sequence number which will keep track of the
//  number of times this sector was written to disk by the user.
//
// Seed - this is the 32 bit lba of the sector to ensure that the sector was written to the
//  intended location on the volume.
typedef struct _SECTOR_LAYOUT {
    IOSectorIdentity_ID SectorIdentifier;   // Identification of writer task
    VolumeIdentifier    DiskIdentifier;     // WWN of volume that this sector was targeted to
    UINT_32             SequenceNo;         // Sequence No (Future)
    UINT_32             Seed;               // Seed (i.e. lba of sector);

    static IOSectorIdentity_ID* getSectorIdentifierPtr(UINT_8 *buffer)
    {
        struct _SECTOR_LAYOUT* pSector = (struct _SECTOR_LAYOUT*) buffer;
        return &pSector->SectorIdentifier;
    }

    static UINT_32* getSeedPtr(UINT_8 *buffer)
    {
        struct _SECTOR_LAYOUT* pSector = (struct _SECTOR_LAYOUT*) buffer;
        return &pSector->Seed;
    }

    static VolumeIdentifier* getDiskIdentifierPtr(UINT_8 *buffer)
    {
        struct _SECTOR_LAYOUT* pSector = (struct _SECTOR_LAYOUT*) buffer;
        return (VolumeIdentifier*) &pSector->DiskIdentifier;
    }

    static UINT_32* getSequenceNumberPtr(UINT_8 *buffer)
    {
        struct _SECTOR_LAYOUT* pSector = (struct _SECTOR_LAYOUT*) buffer;
        return &pSector->SequenceNo;
    }

    static IOSectorIdentity_ID getSectorIdentifier(UINT_8 *buffer)
    {
        struct _SECTOR_LAYOUT* pSector = (struct _SECTOR_LAYOUT*) buffer;
        return pSector->SectorIdentifier;
    }

    static UINT_32 getSeed(UINT_8 *buffer)
    {
        struct _SECTOR_LAYOUT* pSector = (struct _SECTOR_LAYOUT*) buffer;
        return pSector->Seed;
    }

    static VolumeIdentifier getDiskIdentifier(UINT_8 *buffer)
    {
        struct _SECTOR_LAYOUT* pSector = (struct _SECTOR_LAYOUT*) buffer;
        return pSector->DiskIdentifier;
    }

    static UINT_32 getSequenceNumber(UINT_8 *buffer)
    {
        struct _SECTOR_LAYOUT* pSector = (struct _SECTOR_LAYOUT*) buffer;
        return pSector->SequenceNo;
    }

    static void setSectorIdentifier(UINT_8 *buffer, IOSectorIdentity_ID sectorIdentifier)
    {
        struct _SECTOR_LAYOUT* pSector = (struct _SECTOR_LAYOUT*) buffer;
        pSector->SectorIdentifier = sectorIdentifier;
    }

    static void setSeed(UINT_8 *buffer, UINT_32 seed)
    {
        struct _SECTOR_LAYOUT* pSector = (struct _SECTOR_LAYOUT*) buffer;
        pSector->Seed = seed;
    }

    static void setDiskIdentifier(UINT_8 *buffer, VolumeIdentifier diskIdentifier)
    {
        struct _SECTOR_LAYOUT* pSector = (struct _SECTOR_LAYOUT*) buffer;
        pSector->DiskIdentifier = diskIdentifier;
    }

    static void setSequenceNumber(UINT_8 *buffer, UINT_32 sequenceNumber)
    {
        struct _SECTOR_LAYOUT* pSector = (struct _SECTOR_LAYOUT*) buffer;
        pSector->SequenceNo = sequenceNumber;
    }
} SECTOR_LAYOUT, *PSECTOR_LAYOUT;

typedef IOReference *PIOReference;

class IOReference: Runnable
{
private:
    friend class  IOSector_DM;
    WDMSERVICEPUBLIC IOReference(PIOSectorIdentity *iosMemory,
                                 PIOTaskId taskId,
                                 IOBuffer *iob = NULL);
    WDMSERVICEPUBLIC IOReference(PIOSectorIdentity *iosMemory,
                                 PIOTaskId taskID,
                                 IOSectorPattern_t Pattern,
                                 UINT_32 Seed,
                                 UINT_64 NoSectors,
                                 IOBuffer *iob = NULL);

public:
    WDMSERVICEPUBLIC ~IOReference();

    WDMSERVICEPUBLIC PIOTaskId           getTaskID() const;
    WDMSERVICEPUBLIC UINT_32             getSeed() const;
    WDMSERVICEPUBLIC IOSectorPattern_t   getPattern() const;

    WDMSERVICEPUBLIC IOBuffer            *getIOBuffer() const;
    WDMSERVICEPUBLIC UINT_8              *getBuffer() const;   //convenient short-cut for getIOBuffer()->getBuffer();

    WDMSERVICEPUBLIC IOBuffer            *getSGLBuffer() { return mSGLBuffer; }

    WDMSERVICEPUBLIC void                AddSectorIdentity(UINT_32 index, IOSectorIdentity *sector);

    WDMSERVICEPUBLIC UINT_64             getIOSectorIdentityCount() const;
    WDMSERVICEPUBLIC PIOSectorIdentity   getIOSectorIdentity(UINT_32 index) const;

    WDMSERVICEPUBLIC void                AllocateBuffer(void);
    WDMSERVICEPUBLIC void                generateBufferContents(VolumeIdentifier uniqueDiskIdentifier, UINT_32 startLba, PDMC_CONTROL_MEMORY pDMC);
    WDMSERVICEPUBLIC void                generateBufferContents(VolumeIdentifier uniqueDiskIdentifier, UINT_32 startLba, UINT_64 bufferOffset, ULONG sectorCount, UINT_32 seed, PDMC_CONTROL_MEMORY pDMC);
    WDMSERVICEPUBLIC void                generateSglBufferContents(VolumeIdentifier uniqueDiskIdentifier, UINT_32 startLba, PDMC_CONTROL_MEMORY pDMC, IOTaskOp_t op, UINT_16 setSglElementCount);

    WDMSERVICEPUBLIC BOOL                compare(VolumeIdentifier uniqueDiskIdentifier, UINT_64 startLba, IOReference *other, char *msgPrefix = "", BOOL dbgBreakOnError = FALSE, Runnable *ActionOnError = NULL);
    WDMSERVICEPUBLIC BOOL                identify(VolumeIdentifier uniqueDiskIdentifier, PDMC_CONTROL_MEMORY pDMC, UINT_64 startLba, char *msgPrefix = "", BOOL dbgBreakOnError = FALSE, Runnable *ActionOnError = NULL);

    WDMSERVICEPUBLIC void                poisonBuffer();

    WDMSERVICEPUBLIC void                zeroSectorSequenceNumber(UINT_32 startLba, PDMC_CONTROL_MEMORY pDMC);
    /*
     * methods required of Runnable
     */
    const char *getName();
    void run();

private:
    void handleDataMiscompare(Runnable *ActionOnError);
    IOSectorIdentity_ID getIDFromBuffer(UINT_8 *buffer) const;
    VolumeIdentifier getDiskFromBuffer(UINT_8* buffer) const;
    
    void releaseFieldMemory();

    UINT_32 getSeedFromBuffer(UINT_8 *buffer) const;

    PIOTaskId           mTaskID;
    UINT_64             mKey;
    IOSectorPattern_t   mPattern;
    UINT_32             mSeed;
    UINT_64             mNoSectors;
    PIOSectorIdentity   *mSectorList;
    IOBuffer            *mIOBuffer;
    IOBuffer            *mSGLBuffer;
    IOReference_State_t mState;
    IOReference         *mComparedReference;
    UINT_64             mCurrentSectorIndex;
    bool                mDeallocateIOB;
    bool                mDeallocateSGL;
};

typedef IOReference *PIOReference;

#endif
