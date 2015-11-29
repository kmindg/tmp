/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2014
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
#ifndef IOBUFFER
#define IOBUFFER

# include "generic_types.h"
# include "csx_ext.h"

#if defined(WDMSERVICES_EXPORT)
#define WDMSERVICEPUBLIC CSX_MOD_EXPORT
#else
#define WDMSERVICEPUBLIC CSX_MOD_IMPORT
#endif


class  IOSector_DM;
class IOBuffer;

typedef IOBuffer *PIOBuffer;

class IOBuffer
{
public:
    WDMSERVICEPUBLIC ~IOBuffer();

    WDMSERVICEPUBLIC UINT_8 *getBuffer() const;
    WDMSERVICEPUBLIC UINT_64 getLen() const;
    WDMSERVICEPUBLIC void poison();

private:
    friend class  IOSector_DM;
    WDMSERVICEPUBLIC IOBuffer(UINT_64 Len, UINT_8 *buffer = NULL);

    // Points to 512 byte aligned memory offset in mBaseBuffer.
    UINT_8  *mBuffer;

    // Pointer to base memory address. Only set if buffer was allocated by
    // IOBuffer. Must free memory using this address.
    void    *mBaseBuffer;

    // Size of mBuffer.
    UINT_64 mLen;

    // If true, the buffer was allocated by IOBuffer and needs to be freed
    // in destructor.
    BOOL    mManagedBuffer;
};


#endif
