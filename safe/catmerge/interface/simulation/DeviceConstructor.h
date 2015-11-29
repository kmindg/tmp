/***************************************************************************
 * Copyright (C) EMC Corporation 2009 - 2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                        DeviceConstructor.h
 ***************************************************************************
 *
 * DESCRIPTION:  definition of the DeviceConstructor class
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    08/10/2009  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef _DEVICESTACKCONSTRUCTOR_
#define _DEVICESTACKCONSTRUCTOR_

# include "generic_types.h"
# include "simulation/AbstractStackEntry.h"
# include "simulation/VirtualFlareStackEntry.h"
# include "simulation/DiskIdentity.h"
# include "simulation/IOTaskId.h"

class ListOfStackEntries;

class DeviceConstructionListener {
public:
    /*
     * \fn void requestDeviceCreation()
     * \param index - index of CLARiiON disk/LUN
     * \param identity - disk identity describing the disk
     * \details
     *  postRequestDeviceCreation is called when requestDeviceCreation completes.
     *  This notification is provided for tests that need to perform additional
     *  configuration as LUNS are created
     */
    virtual void postRequestDeviceCreation(int index, PDiskIdentity identity) = 0;    
    virtual ~DeviceConstructionListener() {};
};

typedef DeviceConstructionListener *PDeviceConstructionListener;

/*
 * A default device construction listener that does nothing
 */
class DefaultDeviceConstructorListener: public DeviceConstructionListener {
public:
    virtual void postRequestDeviceCreation(int index, PDiskIdentity identity) { return; }    
};

/*
 * Class DeviceConstructor is a factory responsible for creating LUN volumes 
 * used within simulation.
 *
 * LUN volumes are decorated with a set of drivers(the driver Stack) that
 * filter IO request for that volume.  
 *
 * This class is responsible for managing configuration of the driver Stack and
 * forwards configuration requests to appropriate drivers as volumes are
 * created and/or deleted.
 * 
 */
class DeviceConstructor {
public:
    DeviceConstructor(PDeviceConstructionListener listener = NULL,
                        AbstractStackEntry *StackEntryConstructor = NULL,
                        UINT_64 defaultLunSize   = DEFAULT_DISK_SIZE_IN_SECTORS,
                        UINT_32 defaultBlockSize = FIBRE_SECTORSIZE);
    virtual ~DeviceConstructor();

    void extendStack(AbstractStackEntry *StackEntryConstructor);
    /*
     * returns the device name of the Flare device associated with volume index
     */
    void genericFlareDeviceName(int index, char deviceName[K10_DEVICE_ID_LEN]);

    enum ConstructedDeviceType {
        CONSTRUCT_BVD_DEVICE,
        CONSTRUCT_VFD_DEVICE,
    };
    
    /*
     * \fn void genericDeviceName()
     * \param index - The index of the volume to be named
     * \param deviceName - the returned name of volume index
     * \param DriverName - The driver that exports the device to be named.in the device stack which 
     * \details
     * returns the device name associated with a particular device that contributes to servicing volume index IO
     *
     * All user volumes are services by a stack of drivers.  This function is used to 
     * obtain the device path to the volume which a particular driver exports to service volume index.
     */
    void genericDeviceName(int index, char deviceName[K10_DEVICE_ID_LEN], const char *DriverName = NULL);

    /*
     * \fn void genericDeviceName()
     * \param index - CLARiiONDisk index for which a device extension is needed
     * \param DriverName - driver name used to select driver within device stack
     * \detail
     *
     * BVD devices are capable of controlling/managing multiple drivers within the device stack.
     * As such, their top device (as returned by genericDeviceName) is not the device that directly
     * interfaces with this driver.  Test that need/want to access a driver device using the
     * Drivers Volume class logic(extending from BasicVolume), need to call getDeviceExtension()
     * to obtain the correct device extension
     */
    virtual void *getDeviceExtension(int index, const char *DriverName = NULL);

    /*
     * \fn PBvdSimFileIo_File getDriverFactory()
     * \Param   DriverName - The name of the driver
     * \details 
     * returns the File handle to the device factory so
     * that tests can issue IOCTLs directly to a specific driver
     *
     */
    PBvdSimFileIo_File getDriverFactory(const char *DriverName = NULL);

    /*
     * \fn PDiskIdentity constructDevice()
     * \param index - CLARiiONDisk index of device that needs to be created
     * \param rg - Raid group to which this volume is associated.
     * \param diskType - disk type of the raid group, default diskType will be SSD.
     * \details
     *
     * constructDevice  creates a Flare LUN with appropriate characterists
     *  after which a Device Stack is constructed on that Flare LUN
     *
     */
    void constructDevice(UINT_32 index, UINT_32 rg = 0, AbstractDisk_type_t diskType = DiskTypeSSD);
    void constructDevice(UINT_32 index, DiskIdentity *diskConfig);

    /*
     * \fn void destructDevice()
     * \details
     *
     * destructDevice  walks the Device Stack sending destroyDevice requests
     *  after which the Flare LUN is destroyed
     *
     */
    void destructDevice(UINT_32 index);

    /*
     * Provide control for specifying usage of Persistent or Fast CLARiiONDisk
     */
    void setDefaultCLARiiONDiskType(AbstractDisk_type_t type);

    /*
     * \fn void setMaximumDiskVolumes()
     * \param max - specifies the maximum number of disk volumes allowed during simulation
     * \details
     *
     * The test can configure the number of Disk Volumes to be used during testing
     * When NOT specified, the requestDeviceCreation configures simple incremeting volumes
     * When specified, requestDeviceCreation configures volumes with the specified geometries
     */
    void setMaximumDiskVolumes(UINT_32 max);


    /*
     * \fn void setDefaultLunSize()
     * \param noOfSectors - the default number of sectors for new volumes
     * \details
     *
     * used to configure the default volume size
     */
    void setDefaultLunSize(UINT_64 noOfSectors);

     /*
      * \fn void registerDiskConfigration()
      * \param index - CLARiiONDisk index of volume to be configured
      * \param diskConfig - disk configuration to be applied to CLARiiONDisk index
      * \details
      *
      * use registerDiskConfiguration to configure a disk a disk
      */
    void registerDiskConfiguration(UINT_32 volumeIndex, DiskIdentity *diskConfig);

    UINT_64 getDeviceSize(ULONG index);

     /*
      * \fn void getStackEntry()
      * \param index - desired stack index
      * 
      * \details      *
      * Returns stack entry for specified index.
      */
    AbstractStackEntry* getStackEntry(ULONG index);

    /*
     * \fn void openDevice(char *path, PIOTaskID taskID = NULL)
     *  \param path     - path to device/file to open
     *  \param taskID   - optional argument used to specify buffer management capabilities
     *                      primarily used to specify sector length and to track sectors
     *                      through the environment
     *  \details
     *
     * Opens Device "path" using BvdSimFileIo_CreateFile
     *
     * Note, the BasicVolume requires that a volume be trespassed/assigned to an SP
     * before IO can be performed.  As such, openDevice, issues IOCTL_FLARE_TRESPASS_EXECUTE
     * on the device, after opening it. 
     *
     * This function also saves the BvdSimFileIO_File instance pointer to mFile,
     * Field mFile is provided as a convience for tracking the most recently opened file
     */
    BvdSimFileIo_File *openDevice(char *path, PIOTaskId taskID = NULL, ConstructedDeviceType dt = CONSTRUCT_BVD_DEVICE);

    /*
     * \fn void openDevice(char *path, PIOTaskID taskID = NULL)
     *  \param index    - CLARiiONDisk index to be opened
     *  \param taskID   - optional argument used to specify buffer management capabilities
     *                      primarily used to specify sector length and to track sectors
     *                      through the environment
     *  \param DriverName - open volume index device associated with "DriverName". DriverName
     *                      must be participating in the device stack
     *  \details
     *
     * openDevice constructs file path "\\device\upper{index}" which is opened
     * with BvdSimFileIo_CreateFile.
     *
     * Note, the BasicVolume requires that a volume be trespassed/assigned to an SP
     * before IO can be performed.  As such, openDevice, issues IOCTL_FLARE_TRESPASS_EXECUTE
     * on the device, after opening it. 
     *
     * This function also saves the BvdSimFileIO_File instance pointer to mFile,
     * Field mFile is provided as a convience for tracking the most recently opened file
     */
    BvdSimFileIo_File *openDevice(int index, PIOTaskId taskID = NULL, const char *DriverName = NULL, ConstructedDeviceType dt = CONSTRUCT_BVD_DEVICE);

    /*
     * \fn void openDevice(char *path, PIOTaskID taskID = NULL)
     *  \param path     - path to device/file to open
     *  \param taskID   - optional argument used to specify buffer management capabilities
     *                      primarily used to specify sector length and to track sectors
     *                      through the environment
     *  \details
     *
     * Opens Device "path" usign BvdSimFileIo_CreateFile
     *
     * Note: This function trespasses the volume and then issues an ownershiploss.
     *
     * This function also saves the BvdSimFileIO_File instance pointer to mFile,
     * Field mFile is provided as a convience for tracking the most recently opened file
     */
    BvdSimFileIo_File *openDeviceNonOwner(char *path, PIOTaskId taskID = NULL, ConstructedDeviceType dt = CONSTRUCT_BVD_DEVICE);

    /*
     * \fn void openDevice(char *path, PIOTaskID taskID = NULL)
     * \param index    - CLARiiONDisk index to be opened
     * \param taskID   - optional argument used to specify buffer management capabilities
     *                      primarily used to specify sector length and to track sectors
     *                      through the environment
     * \details
     *
     * openDeviceNonOwner constructs file path "\\device\upper{index}" which is opened
     * with BvdSimFileIo_CreateFile.
     *
     * Note: This function trespasses the volume and then issues an ownershiploss.
     *
     * This function also saves the BvdSimFileIO_File instance pointer to mFile,
     * Field mFile is provided as a convience for tracking the most recently opened file
     */
    BvdSimFileIo_File *openDeviceNonOwner(int index, PIOTaskId taskID = NULL, ConstructedDeviceType dt = CONSTRUCT_BVD_DEVICE);

    /*
     * \fn void openFlareDevice(char *path, PIOTaskID taskID = NULL)
     *  \param index    - CLARiiONDisk index to be opened
     *  \param taskID   - optional argument used to specify buffer management capabilities
     *                      primarily used to specify sector length and to track sectors
     *                      through the environment
     *  \details
     *
     * openDevice constructs file path "\\device\VirtualFlare{index}" which is opened
     * with BvdSimFileIo_CreateFile.
     *
     *
     * This function also saves the BvdSimFileIO_File instance pointer to mFile,
     * Field mFile is provided as a convience for tracking the most recently opened file
     */
    BvdSimFileIo_File *openFlareDevice(int index, PIOTaskId taskID = NULL);

    /*
     * \fn void closeDevice()
     * \details
     *
     * Close the most recently opened device
     */
    BOOL closeDevice(BvdSimFileIo_File *file = NULL);

     /*
     * \fn PBvdSimFileIo_File getIoDeviceFile()
     * \Param   index - volume number.
     * \Param   DriverName - The name of the driver
     * \details 
     * returns the File handle to the device factory so
     * that tests can issue IOCTLs directly to this device.
     *
     */
    PBvdSimFileIo_File getIoDeviceFile(int index,  const char *DriverName);


private:
    void growStack();

    AbstractStackEntry *getDriverStackEntry(const char *DriverName);

    PDeviceConstructionListener     mListener;
    UINT_64                         mDefaultLunSize;
    UINT_32                         mDefaultBlockSize;
    ListOfStackEntries              *mDeviceStack;

    AbstractDisk_type_t             mDefaultDriveType;
    UINT_32                         mMaxDiskVolumes;
    PDiskIdentity                   *mDiskConfigs;

    static IOTaskId*                mPTaskId;
};

typedef DeviceConstructor *PDeviceConstructor;
#endif // _DEVICESTACKCONSTRUCTOR_
