/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               IOSectorIdentity_ID.h
 ***************************************************************************
 *
 * DESCRIPTION:  Definitions of class IOSectorIdentity_ID
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    06/14/2013  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef __IOSECTORIDENTITY_ID__
#define __IOSECTORIDENTITY_ID__

# include "generic_types.h"


#if defined(WDMSERVICES_EXPORT)
#define WDMSERVICEPUBLIC CSX_MOD_EXPORT
#else
#define WDMSERVICEPUBLIC CSX_MOD_IMPORT
#endif


typedef struct IOSectorIdentity_ID_s {
    UINT_64 mID;
    UINT_64 mGeneration;
} IOSectorIdentity_ID_t, *PIOSectorIdentity_ID_t;


class IOSectorIdentity_ID: public IOSectorIdentity_ID_t {
public:
    WDMSERVICEPUBLIC IOSectorIdentity_ID(UINT_64 id = 0, UINT_64 generation = 0);

    WDMSERVICEPUBLIC bool operator==(IOSectorIdentity_ID rhs ) const {
        return mGeneration == rhs.mGeneration && mID == rhs.mID;
    }
    WDMSERVICEPUBLIC bool operator!=(IOSectorIdentity_ID_t rhs) const {
        return mGeneration != rhs.mGeneration || mID != rhs.mID;
    }
    WDMSERVICEPUBLIC UINT_64 Hash() const;
    
    WDMSERVICEPUBLIC UINT_64 getID() const;

    WDMSERVICEPUBLIC UINT_64 getGeneration() const;

private:
    friend class IOSector_DM;
};

#endif
