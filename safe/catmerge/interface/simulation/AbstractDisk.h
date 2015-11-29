/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               AbstractDisk.h
 ***************************************************************************
 *
 * DESCRIPTION: Root abstraction for a Disk
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    02/01/2010  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef _ABSTRACTDISK_
#define _ABSTRACTDISK_

# include "generic_types.h"

typedef enum AbstractDisk_type_e
{
    DiskTypeSSD     = 0,
    DiskTypeFibre   = 1,
    DiskTypeSAS     = 2,
    DiskTypeATA     = 3,
    PersistentDisks_DiskTypeSSD = 4,
    PersistentDisks_DiskTypeFibre   = 5,
    PersistentDisks_DiskTypeSAS     = 6,
    PersistentDisks_DiskTypeATA     = 7,
    DiskTypeLAST    = PersistentDisks_DiskTypeATA + 1,
    DiskTypeInvalid = DiskTypeLAST + 1,
} AbstractDisk_type_t;


# define DEFAULT_DISK_SIZE_IN_SECTORS 204800 //define a 100MB default volume
# define DISK_META_DATA_SIZE 2000  // When using persistent disks size extended 2000 sectors to represent disk
                                   // metadata.   ReadAndPin allows write/read past end of normal volume size

class AbstractDisk {
public:
    virtual ~AbstractDisk() {};
    virtual AbstractDisk_type_t getDiskType() const = 0;
    virtual bool isPatterned() const { return getDiskType() <= DiskTypeATA;}
    virtual UINT_32 getRaidGroupID() const = 0;
    virtual UINT_64 getSizeInSectors() const = 0;
    virtual UINT_32 getTracksPerCylinder() const = 0;
    virtual UINT_32 getSectorsPerTrack() const = 0;
    virtual UINT_32 getSectorPayloadSize() const = 0;
    virtual UINT_32 getSectorSize() const = 0;
    virtual UINT_32 getSectorsPerStripe() const = 0;
    virtual UINT_32 getElementsPerParityStripe() const = 0;
    virtual UINT_32 getFlags() const = 0;
};



typedef AbstractDisk *PAbstractDisk;

#endif // _ABSTRACTDISK_
