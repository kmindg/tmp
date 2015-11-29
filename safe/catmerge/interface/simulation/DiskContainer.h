/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               DiskContainer.h
 ***************************************************************************
 *
 * DESCRIPTION: Definition of a class that contains a Disks contents
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    02/01/2010  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef _DISKCONTAINER_
#define _DISKCONTAINER_

# include "generic_types.h"
# include "AbstractDisk.h"
# include "AbstractDiskContainer.h"
# include "IOSectorManagement.h"

#if defined(WDMSERVICES_EXPORT)
#define WDMSERVICEPUBLIC CSX_MOD_EXPORT
#else
#define WDMSERVICEPUBLIC CSX_MOD_IMPORT
#endif


class DiskContainer: public AbstractDiskContainer {
public:
    WDMSERVICEPUBLIC DiskContainer(AbstractDisk *Disk, PIOTaskId TaskId = NULL);
    WDMSERVICEPUBLIC ~DiskContainer();

    WDMSERVICEPUBLIC BOOL IdentityExists(UINT_32 Lba);
    WDMSERVICEPUBLIC IOSectorIdentity *getIdentity(UINT_32 Lba);
    WDMSERVICEPUBLIC void setIdentity(UINT_32 Lba, IOSectorIdentity *identity);

private:
    PIOTaskId    mTaskID;
    VOID         *mSectors;
    AbstractDisk *mDisk;
};


typedef DiskContainer *PDiskContainer;

#endif // _DISKCONTAINER_
