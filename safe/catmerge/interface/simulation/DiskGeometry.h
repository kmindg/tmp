/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               DiskGeometry.h
 ***************************************************************************
 *
 * DESCRIPTION: Definition of Disk class capable of managing an extent of
 *              of sectors using a Logical Block Address
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    02/01/2010  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef DISKGEOMETRY
#define DISKGEOMETRY

# include "generic_types.h"
# include "DiskIdentity.h"
# include "AbstractDiskContainer.h"


class DiskGeometry: public DiskIdentity, public AbstractDiskContainer {
public:
    DiskGeometry(AbstractDisk *clone, PIOTaskId TaskId = NULL);
    virtual ~DiskGeometry();
    DiskGeometry(AbstractDisk_type_t type, UINT_64 SizeInSectors, UINT_32 SectorSize = 520, UINT_32 RaidGroupID = 0, UINT_32 TracksPerCylinder = 1, UINT_32 SectorsPerTrack = 1, PIOTaskId mTaskId = NULL);

    BOOL IdentityExists(UINT_32 Lba);
    IOSectorIdentity *getIdentity(UINT_32 Lba);
    void setIdentity(UINT_32 Lba, IOSectorIdentity *identity);
private:
    AbstractDiskContainer *mStorage;
};


typedef DiskGeometry *PDiskGeometry;

#endif // DISKGEOMETRY
