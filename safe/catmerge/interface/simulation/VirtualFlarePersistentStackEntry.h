/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               VirtualFlarePeristentStackEntry.h
 ***************************************************************************
 *
 * DESCRIPTION: Definition of the CachePersistentStackEntry class
 *
 *    This class issues IOCTL IOCTL_BVD_CREATE_VOLUME to request a 
 *    VirualFlare LUN creation
 *
 * FUNCTIONS: 
 *
 * NOTES:
 *
 * HISTORY:
 *    08/03/2011  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef __VFDPERSITENTSTACK__
#define __VFDPERSITENTSTACK__

#include "simulation/VirtualFlareStackEntry.h"

class VirtualFlarePersistentStackEntry: public VirtualFlareStackEntry {
public:
    VirtualFlarePersistentStackEntry();

    virtual void createDevice(UINT_32 index,
                                char consumedDevicePath[K10_DEVICE_ID_LEN],
                                DiskIdentity * diskId);

    void destroyDevice(UINT_32 index);
};
#endif  // __VFDPERSITENTSTACK__
