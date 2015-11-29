/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               AbstractDiskContainer.h
 ***************************************************************************
 *
 * DESCRIPTION: Root abstraction for accessing a Disks contents
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    02/01/2010  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef _ABSTRACTDISKCONTAINER_
#define _ABSTRACTDISKCONTAINER_

# include "generic_types.h"
# include "IOSectorIdentity.h"


class AbstractDiskContainer {
public:
    virtual ~AbstractDiskContainer() {}
    virtual BOOL IdentityExists(UINT_32 Lba) = 0;
    virtual IOSectorIdentity *getIdentity(UINT_32 Lba) = 0;
    virtual void setIdentity(UINT_32 Lba, IOSectorIdentity *identity) = 0;
};


typedef AbstractDiskContainer *PAbstractDiskContainer;

#endif // _ABSTRACTDISKCONTAINER_
