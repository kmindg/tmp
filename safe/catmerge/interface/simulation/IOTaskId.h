/***************************************************************************
 * Copyright (C) EMC Corporation 2009-13
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               IOTaskId.h
 ***************************************************************************
 *
 * DESCRIPTION:  Description of the BVDSim IOTaskId class 
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    01/27/2010  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef _IOTASKID_
#define _IOTASKID_
# include "generic_types.h"
# include "core_config.h"

#if defined(WDMSERVICES_EXPORT)
#define WDMSERVICEPUBLIC CSX_MOD_EXPORT
#else
#define WDMSERVICEPUBLIC CSX_MOD_IMPORT
#endif


# include "simulation/IOSectorIdentity_ID.h"
# include "simulation/IOSectorManagement.h"
# include "simulation/IOSectorIdentity.h"

# define IOTASKIDNAMELEN 64



class IOSector_DM;
class IOTaskId;
typedef IOTaskId *PIOTaskId;

class IOTaskId {
public:
    WDMSERVICEPUBLIC UINT_64         getID() const;

    WDMSERVICEPUBLIC const void      *getThreadId() const;
    WDMSERVICEPUBLIC const char      *getName() const;
    WDMSERVICEPUBLIC IOTaskType_e    getTaskType() const;
    WDMSERVICEPUBLIC IOTaskSP_e      getSP() const;

    WDMSERVICEPUBLIC UINT_16         getSectorLength() const;
    WDMSERVICEPUBLIC UINT_16         getSectorPayloadLength() const;


    /*
     * The following 6 methods have properly meaningful names 
     */
    WDMSERVICEPUBLIC UINT_64   convertFlareAddressToLBA(UINT_64 flareAddress) const;
    WDMSERVICEPUBLIC UINT_64   convertLBAToFlareAddress(UINT_64 lba) const;

    WDMSERVICEPUBLIC UINT_64   convertDeviceAddressToLBA(UINT_64 deviceAddress) const;
    WDMSERVICEPUBLIC UINT_64   convertLBAToDeviceAddress(UINT_64 lba) const;

    WDMSERVICEPUBLIC UINT_64   convertFlareAddressToDeviceAddress(UINT_64 flareAddress) const;
    WDMSERVICEPUBLIC UINT_64   convertDeviceAddressToFlareAddress(UINT_64 deviceAddress) const;

    WDMSERVICEPUBLIC UINT_64   convertDeviceLengthToFlareLength(UINT_64 deviceLength) const;
    WDMSERVICEPUBLIC UINT_64   convertFlareLengthToDeviceLength(UINT_64 deviceLength) const;

    /*
     * These two methods are deprecated
     */
    WDMSERVICEPUBLIC UINT_64   convertLBAtoSectorAddress(UINT_64 lba) const;
    WDMSERVICEPUBLIC UINT_64   convertSectorAddressToLBA(UINT_64 address) const;

    /*
     * The following 6 methods have properly meaningful names
     */
    WDMSERVICEPUBLIC UINT_64   convertBufferLengthToNoSectors(UINT_64 length) const;
    WDMSERVICEPUBLIC UINT_64   convertDeviceLengthToNoSectors(UINT_64 length) const;
    WDMSERVICEPUBLIC UINT_64   convertFlareLengthToNoSectors(UINT_64 length) const;

    WDMSERVICEPUBLIC UINT_64   convertNoSectorsToFlareLength(UINT_64 sectors) const;
    WDMSERVICEPUBLIC UINT_64   convertNoSectorsToBufferLength(UINT_64 sectors) const;
    WDMSERVICEPUBLIC UINT_64   convertNoSectorsToDeviceLength(UINT_64 sectors) const;

    /*
     * These two methods have properly meaningful names
     */
    WDMSERVICEPUBLIC UINT_64   convertFlareLengthToBufferLength(UINT_64 length) const;
    WDMSERVICEPUBLIC UINT_64   convertBufferLengthToFlareLength(UINT_64 length) const;

    WDMSERVICEPUBLIC IOBuffer *allocateBuffer(UINT_64 noSectors);


    WDMSERVICEPUBLIC IOSectorIdentity *selectIdentity(IOSectorIdentity_ID sectorId, UINT_32 seed);
    WDMSERVICEPUBLIC IOSectorIdentity *selectIdentity(IOSectorIdentity *identity);

    WDMSERVICEPUBLIC bool getNeedsChecksum();
    WDMSERVICEPUBLIC void setNeedsChecksum();

    static WDMSERVICEPUBLIC IOTaskSP_e getSPFromID(UINT_64 ID);
    static WDMSERVICEPUBLIC IOTaskType_e getTaskTypeFromID(UINT_64 ID);
    static WDMSERVICEPUBLIC UINT_32 getSectorLengthFromID(UINT_64 ID);
    static WDMSERVICEPUBLIC UINT_32 getSectorPayloadLengthFromID(UINT_64 ID);
    static WDMSERVICEPUBLIC const char *decodeIOTaskSPType(IOTaskSP_e);
    static WDMSERVICEPUBLIC const char *decodeIOTaskType(IOTaskType_e);
    static WDMSERVICEPUBLIC const char *decodeIOTaskID(UINT_64 ID);
    static WDMSERVICEPUBLIC bool checksumSector(UINT_8 * buffer);
    static WDMSERVICEPUBLIC void setChecksum(UINT_8 * buffer, UINT_16E checksum);
    static WDMSERVICEPUBLIC UINT_16E getChecksum(UINT_8 * buffer);
    static WDMSERVICEPUBLIC void corruptChecksum(UINT_8 * buffer);
    static WDMSERVICEPUBLIC void MakeSectorKnownBad(UINT_8 * sectorData, UINT_32 SPIndex, UINT_64 lba);

private:
    friend class  IOSector_DM;
    WDMSERVICEPUBLIC IOTaskId(const void *threadId, 
                              IOTaskType_e taskType,
                              const char *name,
                              IOTASKSP_T SP = IOSTASK_SP_UNKNOWN,
                              bool chkSumNeeded = FALSE);

    static const UINT_32 IOTASKID_MASK          = 0xfffff;
    static const UINT_32 IOTASKID_MAX_ID        = 65536;

    static const UINT_32 IOTASKID_type_mask     = 0x7;
    static const UINT_32 IOTASKID_type_shift    = 16;

    static const UINT_32 IOTASKID_SP_mask       = 0x1;
    static const UINT_32 IOTASKID_SP_shift      = 19;

    UINT_64         mKey;
    const void      *mThreadId;
    char            mName[IOTASKIDNAMELEN];
    const UINT_16   mSectorLength;
    bool            mChkSumNeeded;
};


#endif   // _IOTASKID_
