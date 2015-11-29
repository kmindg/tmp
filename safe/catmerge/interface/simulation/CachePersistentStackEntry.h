/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               CachePeristentStackEntry.h
 ***************************************************************************
 *
 * DESCRIPTION: Definition of the CachePersistentStackEntry class
 *
 *    This class issues IOCTL IOCTL_BVD_CREATE_VOLUME to request a LUN
 *      creation
 *
 * FUNCTIONS: 
 *
 * NOTES:
 *
 * HISTORY:
 *    06/23/2011  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef __CACHEPERSITENTSTACK__
#define __CACHEPERSITENTSTACK__

#include "simulation/AutoInsertFactoryStackEntry.h"

class CachePersistentStackEntry: public AutoInsertFactoryStackEntry {

public:
    CachePersistentStackEntry(const char *driverName, bool zeroFill, bool useLR);

    virtual void createDevice(UINT_32 index,
                                char consumedDevicePath[K10_DEVICE_ID_LEN],
                                DiskIdentity * diskId);

    /*
     * \fn void getDriverDevicePath()
     * \param index      - index of device
     * \param devicePath - return buffer will contain the path to device index
     * \details
     *
     * Given a device index, getDevicePath will generate the path that can be used
     * to communicate with that device, using the BvdSimFIleIo library.
     *
     * Note, BVD drivers can manage multiple devices within the device stack.
     *       Method getDriverDevicePath() returns the correct path that can
     *       be used to directly interface with the driver Volume (extends BasicVolume)
     */
    void getDriverDevicePath(UINT_32 index, char devicePath[K10_DEVICE_ID_LEN]);

    void destroyDevice(UINT_32 index);
private:
    bool mZeroFill;
    bool mUseUR;
};
#endif
