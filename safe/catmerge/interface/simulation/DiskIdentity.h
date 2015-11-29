/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               DiskIdentity.h
 ***************************************************************************
 *
 * DESCRIPTION: Definitions of class that describes a disks geometry
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    02/01/2010  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef _DISKIDENTITY_
#define _DISKIDENTITY_

# include "generic_types.h"
# include "AbstractDisk.h"
# include "simulation/IOSectorManagement.h"
# include "k10ntddk.h"
# include "flare_export_ioctls.h"

// Tells the code that no data checking will take place. I.E no data validation
// so this should not be used it looking to find DMCs.
#define DISK_IDENTITY_FLAG_NO_DATA_CHECK 0x00000001

typedef struct DiskIdentity_s {
    AbstractDisk_type_t mDiskType;
    UINT_64             mSizeInSectors;
    UINT_32             mRaidGroupID;
    UINT_32             mTracksPerCylinder;
    UINT_32             mSectorsPerTrack;
    UINT_32             mSectorSize;
    UINT_32             mSectorsPerStripe;
    UINT_32             mElementsPerParityStripe;
    UINT_32             mFlags;
} DiskIdentity_t;

class DiskIdentity;

typedef DiskIdentity *PDiskIdentity;

#define DISKIDENTITY_TRACKSPERCYLINDER_DEFAULT 1
#define DISKIDENTITY_SECTORSPERTRACK_DEFAULT 1
#define DISKIDENTITY_SECTORSPERSTRIPE_DEFAULT (128 * 4)
#define DISKIDENTITY_ELEMENTSPERPARITY_STRIPE_DEFAULT 8
#define DISKIDENTITY_FLAGS_DEFAULT 0


class DiskIdentity: public AbstractDisk, public DiskIdentity_t {
public:
    DiskIdentity(PDiskIdentity data);
    DiskIdentity(AbstractDisk *clone);
    DiskIdentity(AbstractDisk_type_t type,
            UINT_64 SizeInSectors,
            UINT_32 SectorSize = FIBRE_SECTORSIZE,
            UINT_32 RaidGroupID = 0,
            UINT_32 TracksPerCylinder = DISKIDENTITY_TRACKSPERCYLINDER_DEFAULT,
            UINT_32 SectorsPerTrack = DISKIDENTITY_SECTORSPERTRACK_DEFAULT,
            UINT_32 SectorsPerStripe = DISKIDENTITY_SECTORSPERSTRIPE_DEFAULT,
            UINT_32 ElementsPerParityStripe = DISKIDENTITY_ELEMENTSPERPARITY_STRIPE_DEFAULT,
            UINT_32 Flags = DISKIDENTITY_FLAGS_DEFAULT);

    AbstractDisk_type_t getDiskType() const;
    UINT_32 getRaidGroupID() const;
    UINT_64 getSizeInSectors() const;
    UINT_32 getTracksPerCylinder() const;
    UINT_32 getSectorsPerTrack() const;
    UINT_32 getSectorPayloadSize() const;
    UINT_32 getSectorSize() const;
    UINT_32 getSectorsPerStripe() const;
    UINT_32 getElementsPerParityStripe() const;
    UINT_32 getFlags() const;
    bool getNoDataCheckFlag() const { return ((getFlags() & DISK_IDENTITY_FLAG_NO_DATA_CHECK) ? TRUE : FALSE);}
};



#endif // _DISKIDENTITY_
