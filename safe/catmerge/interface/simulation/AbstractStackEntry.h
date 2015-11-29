/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               AbstractStackEntry.h
 ***************************************************************************
 *
 * DESCRIPTION:  Abstract Class definitions used to construct a DeviceStack Entry
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    08/10/2009  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef _ABSTRACTSTACKENTRYCONSTRUCTOR_
#define _ABSTRACTSTACKENTRYCONSTRUCTOR_

# include "generic_types.h"
# include "simulation/BvdSimFileIo.h"
# include "simulation/DiskIdentity.h"


class AbstractStackEntry {
public:
    /*
     * \fn void ~AbstractStackEntry()
     * \details
     *
     * This is the destructor for class AbstractStackEntry, declared virtual
     * so that delete instance_of_AbstractStackEntry can select the correct
     * destructor when AbstractStackEntry has been subclassed.
     */
    virtual ~AbstractStackEntry() {}

    /*
     * \fn const char *getDriverName()
     *
     * \details
     *
     *  Return the name of the Driver responsible for this StackEntry
     */
    virtual const char *getDriverName() = 0;

    /*
     * \fn void getLinkPath()
     * \param index      - index of device
     * \param devicePath - return buffer will contain the path to device index
     * \details
     *
     * Given a device index, getLinkPath will generate the path that can be used
     * to communicate with that device, using the BvdSimFIleIo library.   On
     * Microsoft Windows sytems the link path is "\\DosDevices\\Global\\DRIVERNAME{index}" 
     */
    virtual void getLinkPath(UINT_32 index, char devicePath[K10_DEVICE_ID_LEN]) = 0;

    /*
     * \fn void getDevicePath()
     * \param index      - index of device
     * \param devicePath - return buffer will contain the path to device index
     * \details
     *
     * Given a device index, getDevicePath will generate the path that can be used
     * to communicate with that device, using the BvdSimFIleIo library.   On
     * Microsoft Windows sytems the device path is "\\Device\\\\DRIVERNAME{index}" 
     */
    virtual void getDevicePath(UINT_32 index, char devicePath[K10_DEVICE_ID_LEN]) = 0;

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
    virtual void getDriverDevicePath(UINT_32 index, char devicePath[K10_DEVICE_ID_LEN]) {
        getDevicePath(index, devicePath);
    }

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
     * \param index          - index of device
     *
     * \details
     *
     * Given a device index, destroyDevice will destroyes the appropriate device
     */
    virtual void destroyDevice(UINT_32 index) = 0;

    /*
     * \fn void getFactory()
     *
     * \details
     *
     * Returns factory object associated with this Stack entries Driver
     */
    virtual PBvdSimFileIo_File getFactory() = 0;

};

typedef AbstractStackEntry *PAbstractStackEntry;
#endif // _ABSTRACTSTACKENTRYCONSTRUCTOR_
