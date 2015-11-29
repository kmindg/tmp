/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               VirtualFlareStackEntry.h
 ***************************************************************************
 *
 * DESCRIPTION:  class definitions used to manage the VirtualFlare DeviceStack Entry
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    08/10/2009  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef _VIRTUALFLARESTACKENTRYCONSTRUCTOR_
#define _VIRTUALFLARESTACKENTRYCONSTRUCTOR_

#if defined(WDMSERVICES_EXPORT)
#define WDMSERVICEPUBLIC CSX_MOD_EXPORT
#else
#define WDMSERVICEPUBLIC CSX_MOD_IMPORT
#endif


# include "generic_types.h"
# include "simulation/AutoInsertFactoryStackEntry.h"


/*
 * This class implements the logic needed to report the
 * Flare LUN device paths back to the DeviceConstructor, responsible
 * for building device stacks.
 *
 * Since the real controls for managing Flare LUNS are part of the VirtualFlareDriver,
 * methods createDevice() and destroyDevice() are effectively noops.
 */
#ifdef ALAMOSA_WINDOWS_ENV
class VirtualFlareStackEntry: public AutoInsertFactoryStackEntry {
#else
class CSX_CLASS_EXPORT VirtualFlareStackEntry: public AutoInsertFactoryStackEntry {
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - C++ export mess */
public:
    /*
     * \fn void VirtualFlareStackEntry()
     * \details
     *
     * This constructor generates:
     *             - the name of the AutoInsertFactory path from driverName and
     *               opens that device in preparation for issueing device creations
     *             - device paths from the driverName
     */
    WDMSERVICEPUBLIC VirtualFlareStackEntry(const char *driverName);

    WDMSERVICEPUBLIC VirtualFlareStackEntry(const char *factoryDevicePath, const char *deviceNamePrefix);

    /*
     * \fn void ~VirtualFlareStackEntry()
     * \details
     *
     * Destructor for class AutoInsertFactory
     */
    WDMSERVICEPUBLIC ~VirtualFlareStackEntry();

    /*
     * \fn void createDevice()
     * \param index          - index of device
     * \param consumedDevice - buffer initialy contains the path of the consumed Volume
     *                         on return, buffer contains the path of the volume created
     * \details
     *
     * Given a device index, createDevice will creates a new device that consumes the
     * volume referenced in buffer consumedDevice.  After creating the new device, buffer
     * consumedDevice is updated to contain the path to the new device.
     */
    WDMSERVICEPUBLIC void createDevice(UINT_32 index,  char consumedDevicePath[K10_DEVICE_ID_LEN], DiskIdentity * diskId);
};

typedef VirtualFlareStackEntry *PVirtualFlareStackEntry;
 
#endif // _VIRTUALFLARESTACKENTRYCONSTRUCTOR_
