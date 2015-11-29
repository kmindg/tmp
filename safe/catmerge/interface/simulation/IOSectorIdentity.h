/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               IOSectorIdentity.h
 ***************************************************************************
 *
 * DESCRIPTION:  Definitions for the BVDSim IO Sector Identity
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    01/27/2010  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef IOSECTORIDENTITY
#define IOSECTORIDENTITY

# include "generic_types.h"
# include "simulation/Runnable.h"
# include "simulation/IOSectorIdentity_ID.h"
# include "BasicVolumeDriver/BasicVolumeParams.h"

#if defined(WDMSERVICES_EXPORT)
#define WDMSERVICEPUBLIC CSX_MOD_EXPORT
#else
#define WDMSERVICEPUBLIC CSX_MOD_IMPORT
#endif

class IOSector_DM;

class IOSectorIdentity;
typedef IOSectorIdentity *PIOSectorIdentity;

# include "IOTaskId.h"

class IOSectorIdentity {
private:
    friend class  IOSector_DM;
    WDMSERVICEPUBLIC IOSectorIdentity(IOSectorIdentity_ID id, PIOSectorIdentity Parent = NULL);

public:
    WDMSERVICEPUBLIC ~IOSectorIdentity();

    WDMSERVICEPUBLIC const IOTaskId *        getTaskID() const;
    WDMSERVICEPUBLIC IOSectorIdentity_ID     getID() const;
    WDMSERVICEPUBLIC IOSectorPattern_t       getPattern() const;
    WDMSERVICEPUBLIC UINT_32                 getSeed() const;
    WDMSERVICEPUBLIC UINT_32                 getNextSeed(UINT_32 seed) const;
    WDMSERVICEPUBLIC UINT_32                 getPayloadLength() const;
    WDMSERVICEPUBLIC UINT_32                 getLength() const;
    WDMSERVICEPUBLIC IOSectorIdentity        *getParent() const;

    WDMSERVICEPUBLIC UINT_8              *populate(VolumeIdentifier uniqueDiskIdentifier, UINT_32 seqNo, UINT_8 *buffer, IOTaskId * pTaskId = NULL);

    WDMSERVICEPUBLIC BOOL                compare(VolumeIdentifier uniqueDIskIdentifier, UINT_64 lba, UINT_8 *buffer, char *msgPrefix = "", BOOL dbgBreakOnError = FALSE, Runnable *ActionOnError = NULL);
    WDMSERVICEPUBLIC BOOL                comparePattern(VolumeIdentifier uniqueDIskIdentifier, UINT_64 lba, UINT_8 *buffer, char *msgPrefix = "", BOOL dbgBreakOnError = FALSE, Runnable *ActionOnError = NULL);

    WDMSERVICEPUBLIC void                KtraceReport(UINT_8 *ptr);

    static WDMSERVICEPUBLIC UINT_64      constructID(IOTaskId * taskId, IOSectorPattern_t Pattern, UINT_32 Seed);
    static WDMSERVICEPUBLIC UINT_64      constructID(UINT_64 taskId, IOSectorPattern_t Pattern, UINT_32 Seed);
    static WDMSERVICEPUBLIC UINT_64      getTaskID(IOSectorIdentity *ios);

    static WDMSERVICEPUBLIC IOSectorPattern_t    getPatternFromSectorID(UINT_64 ID);
    static WDMSERVICEPUBLIC UINT_32              getSeedFromSectorID(UINT_64 ID);
    static WDMSERVICEPUBLIC UINT_64              getLBAFromSectorID(UINT_64 ID);
    static WDMSERVICEPUBLIC UINT_64              getTaskIDFromSectorID(UINT_64 ID);
    static WDMSERVICEPUBLIC void                 validate(UINT_64 fileOffset, PIOSectorIdentity ios);

    static const UINT_64          ID_PIOTaskID_Mask      = 0x0fffffff;
    static const UINT_32          ID_PIOTaskId_Shift     = 32;

    static const UINT_64          ID_Seed_Mask           = 0xffffffff;
    static const UINT_32          ID_Seed_Shift          = 0;
    static const UINT_64          ID_SeedType_Mask       = 0xf;
    static const UINT_64          ID_SeedType_Shift      = 60;

private:
    void    handleDataMiscompare(Runnable *ActionOnError);


    
    IOSectorIdentity_ID     mID;
    IOSectorIdentity        *mParent;
};

#endif
