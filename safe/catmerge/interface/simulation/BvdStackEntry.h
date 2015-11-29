/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                        BvdStackEntry.h
 ***************************************************************************
 *
 * DESCRIPTION:  definition of the BvdStackEntry class
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    09/29/2012  Martin Buckley Initial Version
 *
 **************************************************************************/

#ifndef _BVDSTACKENTRY_
#define _BVDSTACKENTRY_

# include "simulation/AutoInsertFactoryStackEntry.h"

class BvdStackEntry: public AutoInsertFactoryStackEntry {
public:
    BvdStackEntry(const char *driverName);

    /*
     * \fn void createDevice()
     * \param index          - index of device
     * \param consumedDevice - buffer initialy contains the path of the consumed Volume
     *                         on return, buffer contains the path of the volume created
     * \param diskId         - Identity of Disk.     
     * \details
     *
     * Given a device index, createDevice will creates a new device that consumes the
     * volume referenced in buffer consumedDevice.  After creating the new device, buffer
     * consumedDevice is updated to contain the path to the new device.
     */
    virtual void createDevice(UINT_32 index, char consumedDevicePath[K10_DEVICE_ID_LEN], DiskIdentity * diskId) = 0;

    /*
     * \fn void destroyDevice()
     * \param index          - index of device to be destroyed
     *
     * \details
     *
     * Given a device index, destroyDevice will destroyes the appropriate device
     */
    virtual void destroyDevice(UINT_32 index);

};
#endif
