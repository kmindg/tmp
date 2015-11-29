/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                        AutoInsertFactoryStackEntry.h
 ***************************************************************************
 *
 * DESCRIPTION:  definition of the AutoInsertFactoryStackEntry class
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    08/10/2009  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef _AUTOINSERTFACTORYSTACKENTRYCONSTRUCTOR_
#define _AUTOINSERTFACTORYSTACKENTRYCONSTRUCTOR_

# include "simulation/AbstractStackEntry.h"



/*
 * Class AutoInsertFactoryStackEntry is an adapter that can issue ioctls
 * IOCTL_AIDVD_INSERT_DISK_VOLUME & IOCTL_AIDVD_DESTROY_DISK_VOLUME to
 * BasicVolumeDriver AutoInsertFactory devices.
 *
 * This class and classes that inherit from it, can be used by Class DeviceConstructor. 
 * to manage Device creation in a  multiple drivers are stacked environment
 */
class AutoInsertFactoryStackEntry: public AbstractStackEntry {
public:
    /*
     * \fn void AutoInsertFactoryStackEntry()
     * \param driverName -  The name of the Driver for which this StackEntry
     *                      interacts with to perform autoInsert operations
     * \details
     *
     * This constructor generates:
     *             - the name of the AutoInsertFactory path from driverName and
     *               opens that device in preparation for issueing device creations
     *             - device paths from the driverName
     */
    AutoInsertFactoryStackEntry(const char *driverName);

    /*
     * \fn void AutoInsertFactoryStackEntry()
     * \param factoryDevicePath - The path to the AutoInsert Factory
     * \param deviceNamePrefix  - All created devices will have path deviceNamePrefix{index}
     * \details
     *
     * This constructor opens the specified device and saves the pefix for generated device paths
     *
     */
    AutoInsertFactoryStackEntry(const char *factoryDevicePath, const char *deviceNamePrefix);

    /*
     * \fn void ~AutoInsertFactoryStackEntry()
     * \details
     *
     * Destructor for class AutoInsertFactory
     */
    ~AutoInsertFactoryStackEntry();

    /*
     * \fn const char *getDriverName()
     *
     * \details
     *
     *  Return the name of the Driver responsible for this StackEntry
     */
    virtual const char *getDriverName();

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
    void getLinkPath(UINT_32 index, char devicePath[K10_DEVICE_ID_LEN]);

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
    void getDevicePath(UINT_32 index, char devicePath[K10_DEVICE_ID_LEN]);

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
    virtual void createDevice(UINT_32 index, char consumedDevicePath[K10_DEVICE_ID_LEN], DiskIdentity * diskId);

    /*
     * \fn void destroyDevice()
     * \param index          - index of device to be destroyed
     *
     * \details
     *
     * Given a device index, destroyDevice will destroyes the appropriate device
     */
    virtual void destroyDevice(UINT_32 index);

    /*
     * \fn void getFactory()
     *
     * \details
     *
     * Returns factory object associated with this Stack entries Driver
     */
    virtual PBvdSimFileIo_File getFactory();

    /*
     * \fn void getFactory()
     *
     * \details
     *
     * Lazy open of Device Factory
     */
    void openAutoInsertFactory();

protected:

    PBvdSimFileIo_File mFactory;

private:
    const char *mFactoryDevicePath;
    const char *mDeviceNamePrefix;
};


#endif // _AUTOINSERTFACTORYSTACKENTRYCONSTRUCTOR_
