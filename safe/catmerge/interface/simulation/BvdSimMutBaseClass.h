/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               BvdSimMutBaseClass.cpp
 ***************************************************************************
 *
 * DESCRIPTION:  Declarations of the BvdSimMutMaseClass API definition
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    06/02/2009  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef __BVDSIMMUTBASECLASS__
#define __BVDSIMMUTBASECLASS__
# include "simulation/DiskIdentity.h"
# include "simulation/BvdSimFileIo.h"
# include "simulation/VirtualFlareDriver.h"
# include "simulation/BvdSim_Registry.h"
# include "simulation/BvdSimIoTestConfig.h"
# include "simulation/AbstractDriverReference.h"
# include "simulation/ArrayDirector.h"
# include "simulation/ConcurrentIoGenerator.h"
# include "simulation/DefaultDriverConfiguration.h"
# include "AutoInsertFactoryStackEntry.h"

# include "BasicLib/ConfigDatabase.h"
# include "BasicLib/MemoryBasedConfigDatabase.h"
# include "simulation/managed_malloc.h"
# include "simulation/DeviceConstructor.h"
# include "simulation/AbstractSystemConfigurator.h"
# include "simulation/BvdSimIOUtilityBaseClass.h"
# include "simulation/DefaultSPControl.h"
# include "simulation/SpId_option.h"
# include "simulation/arguments.h"
# include "BvdSimDeviceManagement.h"

# include "EmcPAL.h"
# include "generic_types.h"
# include "Program_Option.h"

# include "mut_cpp.h"

//#include "EmcPAL_Misc.h"

#define CDRFILENAME             "CDR"
#define PSMFILENAME             "PSM"
#define VAULTLUNFILENAME        "VaultLUN"

/*
 * Test programs use these CLIare initialized in BvdSimUnitTestSetup
 * These global pointers are located in BvdSimMutBaseClass.cpp
 */
extern Bool_option *useExistingSystemDevices;
extern Bool_option *leaveSystemDevices;
extern Bool_option *useNVCM;
extern Bool_option *SkipSyncEvent;
/*
 * Combine all BvdSim class capabilities here
 */
enum BvdSimCapability_e {
    BvdSim_Capability_Simulation        = 0, // When not set unit testing, when set simulating
    BvdSim_Capability_BootSystem        = 1, // when set this process configures and boots a System/integration environment
    BvdSim_Capability_LaunchPeer        = 2, // when set this process is responsible for managing the Peer SP process (Mirror/2Legged Stool)
    BvdSim_Capability_PeerHandshake     = 3, // when set this process is subordinate to another process. Handshake during Startup/TearDown & termination
    BvdSim_Capability_SkipTearDown      = 4, // when set this process does not perform Teardown (except BvdSim_Capability_PeerHandshake)
    BvdSim_Capability_PersistentConfiguration = 5,    // include the drivers that the SynchTransaction library require in system configurations
    BvdSim_Capability_PersistentDeviceCreation= 6,    // When set, BVD drivers create devices with IOCTL_BVD_CREATE_VOLUME 
    BvdSim_Capability_AutoLaunchSPA           = 7,    // when set the 3legged control process launches the SPA process during StartUp()
    BvdSim_Capability_AutoLaunchSPB           = 8,    // when set the 3legged control process launches the SPB process during StartUp()
    BvdSim_Capability_Delete_FS_On_Startup    = 9,    // default action is to delete FS devices, during StartUp()
    BvdSim_Capability_Delete_FS_On_TearDown   = 10,   // default action is to delete FS devices, during TearDown() (normally fails, because Devices are open)
    BvdSim_Capability_SkipSyncDelay           = 11,   // when set, skip IOSectorManagement sync delays
    BvdSim_Capability_HostAffinity            = 12,   // when set, Concurrent IO(Host IO) threads will be affined to a particular thread 

    // The following 2 bits are for Special test configurations (not 3 legged where the control leg and SPA run in the same process

    BvdSim_Capability_SPA_consumeALLCMMMemory = 13,   // Single SP Performance testing, SPA consumes all available CMM memory
    BvdSim_Capability_SPA_LargeCMMMemory      = 14,   // When set, use 1GB of CMM memory, when not set, use 64MB of CMM memory

    // End of special bits

    BvdSim_Capability_ActivePassive           = 15,   // Simulate Flare Active/Passive instead of Rockies Active/Active
    BvdSim_Capability_MCC_Loaded              = 16,   // Set when the MCC/SPCache is loaded
    BvdSim_Capability_Use_LRedirector         = 17,   // include LR in system configuration
    BvdSim_Capability_Use_URedirector         = 18,   // include LR in system configuration
    BvdSim_Capability_Use_FlashCache          = 19,   // include flash in system configuration
    BvdSim_Capability_PersistentMemory        = 20,   // Manage share mem in 3Legged Control process
    BvdSim_Capability_Enable_ZeroFill         = 21,   // ZeroFill control
    BvdSim_Capability_Smaller_Vault           = 22,   // vault size would be lesser than cache size.
    BvdSim_Capability_Do_Not_Commit           = 23,   // Would not commit automatically, test has to explicitly commit the cache driver.
    BvdSim_Capability_Latch_SI_Status         = 24,   // Enable Rockies vaulting behavior
    BvdSim_Capability_MLU_Loaded              = 25,   // Enable MLU Driver
    BvdSim_Capability_Advanced_DMC_Checking   = 26,   // Use advanced DMC checking abilities
    BvdSim_Capability_1GB_CMMMemory           = 27,   // Allocate 1GB of cache for each SP
    BvdSim_Capability_use_MCC                 = 28,   // include MCC in system configuration
    BvdSim_Capability_NEXT                    = 29,   // other test classes capabilities start here



    BvdSim_Capability_MAX                     = 63,   // Maximum Capability currently supported
};

/*
 * This is the Root class for the BvdSimulation Test Environment
 */
//class BvdSimMutBaseClass: public CAbstractTest, public BvdSimIOUtilityBaseClass {
class BvdSimMutBaseClass: public CAbstractTest, public BvdSimIOUtilityBaseClass {
public:
    /*
     * \fn void BvdSimMutBaseClass()
     * \details
     *
     *   Constructor for Class BvdSimMutBaseClass
     */
    BvdSimMutBaseClass(PAbstractDriverReference driverConfiguration = NULL);

    /*
     * \fn void ~BvdSimMutBaseClass()
     * \details
     *
     *   Destructor for Class BvdSimMutBaseClass
     */
    virtual ~BvdSimMutBaseClass();

    /*
     * \fn void StartUp()
     * \details
     *
     *   Virtual initialization routine
     *   Operations required to initialize a test belong in a StartUp method
     */
    void StartUp();

    /*
     * \fn void TearDown()
     * \details
     *
     *   Virtual cleanup routine
     *   Operations required to cleanup a test belong in a TearDown method
     */
    void TearDown();

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
     * \fn void registerDiskConfigration()
     * \param index - CLARiiONDisk index of volume to be configured
     * \param diskConfig - disk configuration to be applied to CLARiiONDisk index
     * \details
     *
      * use registerDiskConfiguration to configure a disk a disk
      */
     void registerDiskConfiguration(UINT_32 volumeIndex, DiskIdentity *diskConfig);

    /*
     * \fn void requestDeviceCreation()
     * \param index - CLARiiONDisk index which requiring construction of a device stack
     * \param rg - Raid group to which this volume is associated.
     * \param diskType - disk type of the raid group, default diskType will be SSD.
     * \details
     *
     * requestDeviceCreation issues IOCTL_AIDVD_INSERT_DISK_VOLUME_PARAMS ioctl to 
     * an open drivers device Factory.  This ioctl creates an association between
     *   \\device\upper{index} and \\device\CLARiiONDisk{index}.
     *
     * This function also creates device \\device\CLARiiONDisk{index}. 
     *
     * The ensures that device \\device\CLARiiONDisk{index} is registered in the BvdSimDeviceManagement registry,
     * before \\device\\upper{index} can be opened.
     */
    void requestDeviceCreation(int index, int rg = 0, AbstractDisk_type_t diskType = DiskTypeSSD);
    void requestDeviceCreation(int index, DiskIdentity *diskConfig);

    /*
     * \fn void requestDeviceDestruction()
     * \param index - CLARiiONDisk index which requiring destruction of a device stack
     * \details
     *
     * requestDeviceDestruction issues IOCTL_AIDVD_DESTROY_DISK_VOLUME ioctl
     * to the autoinsert factory.  This ioctl removes the device stack from
     *   \\device\uper{index} and \\device\CLARiiONDisk{index}
     */
    void requestDeviceDestruction(UINT_32 index);

    /*
     * \fn PBvdSimFileIo_File getDriverFactory()
     * \param   DriverName - The name of the driver
     * \details returns the File handle that can be used to issue IOCTLs to a specific driver
     *
     */
    PBvdSimFileIo_File getDriverFactory(const char *DriverName = NULL);

    /*
     * \fn void genericFlareDeviceName()
     * \param index - The index of the flare volume to be named
     * \param deviceName - The returned name of Flare volume inddex
     * \detail
     * returns the device name of the Flare device associated with volume index
     */
     void genericFlareDeviceName(int index, char deviceName[K10_DEVICE_ID_LEN]);

    /*
     * \fn void genericDeviceName()
     * \param index - The index of the volume to be named
     * \param deviceName - the returned name of volume index
     * \param DriverName - The driver that exports the device to be named.in the device stack which 
     * \details
     * returns the device name associated with a particular device that contributes to servicing volume index IO
     *
     * All user volumes are services by a stack of drivers.  This function is used to 
     * obtain the device path to the volume which a particular driver exports to service 
     */
     void genericDeviceName(int index, char deviceName[K10_DEVICE_ID_LEN], const char *DriverName = NULL);

    void ownershipLoss(BvdSimFileIo_File *file);
    void ownershipGain(BvdSimFileIo_File *file);

    /*
     * \fn void deleteDeviceFromFS(char *path)
     * \param path     - path to device/file to delete
     * \details
     *
     * Remove device file from the local file system
     */
    BOOL deleteDeviceFromFS(char *path);
    

    /*
     * \fn void resetSimulationEnvironment()
     * \details
     *
     * resetSimulationEnvironment() performs all needed cleanup to 
     * allow further tests to execute without side effects from prior tests
     * Currenty, this involves clearing the BvdSimDeviceManagement Registry
     */
    void resetSimulationEnvironment();

    /*
     * Provide control for specifying usage of Persistent or Fast CLARiiONDisk
     */
    void setDefaultCLARiiONDiskType(AbstractDisk_type_t type);

    /*
     * \fn PIOTaskId getDefaultIOTaskId(PIOTaskId taskID)
     * \details
     *
     * returns the default PIoTaskId used for all IO generation
     */
    PIOTaskId getDefaultIOTaskId();


    /*
     * \fn void getDeviceFactory()
     * \details
     *
     * Method returns a pointer to the LUN factory
     */
    PDeviceConstructor getDeviceFactory();

    /*
     * \fn void closeDeviceFactory()
     *
     * \details 
     *
     * Method closeDeviceFactory() closes the device factory.  The device factory has opened
     * all of the autoinsert factory devices that have been programmed into the device stack.
     * BVD Drivers can not be closed until after their autoinsert factories have been closed
     *
     * Once this method has been called, devices can no longer be created
     *
     */
     void closeDeviceFactory();

    /*
     * \fn void setDeviceFactory()
     * \param taskID    - the taskID to be used to manage packet/sector tracking
     *
     * \details
     *
     * register a device factory used to create devices used during testing
     */
     void setDeviceFactory(PDeviceConstructor deviceFactory);

    /*
     * \fn PAbstractDriverReference getDriverConfiguration()
     *
     * \details
     *
     * return the Driver Configuration data
     */
    PAbstractDriverReference getDriverConfiguration();
  
    /*
     * \fn void Test()
     * \details
     *
     * Test writers are required to create an implementation for method Test
     */
    virtual void Test() = 0;


    /*
     * \fn UINT_64 getDeviceSize()
     * \Param index     - the index to a CLARiiONDisk device
     * \details
     *
     * returns the size of the CLARiiONDisk device
     */
    UINT_64 getDeviceSize(ULONG index) {
        return mDeviceFactory->getDeviceSize(index);
    }

    /*
     * /fn void setTestCapability()
     * /param index - index of Cabability flag to set
     * /param b - value to set Capability flag to
     * /detail
     *
     * Set the boolean status for test capability "index"
     *
     *  Return the boolean status for test capability "index"
     *
     *  There are 64 Capability flags available to a test suite
     *  for use in characterizing test behavior.
     *
     *  Usage of these flags is optional.
     *
     *  Capability flags are provided for so that a test can 
     *  provide an indication of its needs when its DriverReference
     *  constructs the test environment.
     *
     */
    void setTestCapability(UINT_32 index, bool b);

    /*
     * \fn bool getTestCapability()
     * \param index - index of Capability flag to return
     * \detail
     *
     *  Return the boolean status for test capability "index"
     *
     *  There are 64 Capability flags available to a test suite
     *  for use in characterizing test behavior.
     *
     *  Usage of these flags is optional.
     *
     *  Capability flags are provided for so that a test can 
     *  provide an indication of its needs when its DriverReference
     *  consructs the test environment.
     *
     */
    bool getTestCapability(UINT_32 index);

    /*
     * \fn UINT_64 getTestCacheSizeBytes()
     * \detail
     *
     *  Return the cache size in bytes to be used for the test if specified
     *  by the user.  If the value is 0, then the test defaults to
     *  the cache size defaults for a test.
     *
     *  Usage of this is optional
     *
     */
    UINT_64 getTestCacheSizeBytes();

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
    BvdSimFileIo_File *openDevice(char *path, PIOTaskId taskID = NULL);

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
    BvdSimFileIo_File *openDevice(int index, PIOTaskId taskID = NULL, const char *DriverName = NULL);

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
    BvdSimFileIo_File *openDeviceNonOwner(char *path, PIOTaskId taskID = NULL);

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
    BvdSimFileIo_File *openDeviceNonOwner(int index, PIOTaskId taskID = NULL);

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
     * The most recently opened BvdSimFileIo_File instance pointer
     * This is public to simplify writing IO tests
     */
    BvdSimFileIo_File *mFile;
    
     void RegisterDriver(AbstractDriverReference *driver) {
        DeviceManagement::RegisterDriver(driver);
    }

    /*
     * \fn BOOL cleanUpSystemLuns()
     * \param none
     * \detail
     *
     * clenaupSystemLuns is responsible for deleting
     * all Volume data contained on the local File System
     */
    virtual void cleanupSystemLuns();

protected:
    PAbstractDriverReference        mDriverConfiguration;
    
private:
    PDeviceConstructor              mDeviceFactory;
    UINT_64                         mTestCapabilities;
};






class TestIndexOption: public Int_option {
public:
    TestIndexOption(UINT_32 value = 0) : Int_option("-test", "N", "Selected test") {

        //EmcpalDebugBreakPoint();
        char **optionValue = (char**)malloc(3 * sizeof(char*));

        Arguments::getInstance()->getCli("-test",optionValue);

        if(*optionValue != NULL){
            //traverse the "-" argument
            optionValue++;
            
            int v = atoi(*optionValue);
            Int_option::_set(v);
            Program_Option::set();      
        }else{
            Int_option::_set(value);
        }
    }
};

class BvdSimMutConcurrentIoLoad {
public:
    BvdSimMutConcurrentIoLoad(PAbstractDriverReference config): mConfig(config) {}
    void Test() {
        ConcurrentIoGenerator *ioGenerator = new ConcurrentIoGenerator(mConfig->getDeviceFactory(),
                                                                       mConfig->getThreadConfigurator(),
                                                                       getTestConfiguration());
        
        ioGenerator->PerformIO();
        delete ioGenerator;
    }

    virtual BvdSimIoTestConfig *getTestConfiguration() = 0;
    
    BvdSimFileIo_File *openDevice(int index, PIOTaskId taskID = NULL);

private:
    PAbstractDriverReference mConfig;
};

/*
 * \Class BvdSimMutSystemTestBaseClass 
 * 
 * \details 
 *
 * Class Contains common utility methods for use
 * in creating SingleSP and DualSP simulation environments.
 *
 * Tests should NOT extend BvdSimMutSystemTestBaseClass, they should
 * extend BvdSimMutMasterSystemBaseClass or BvdSimMutPeerSystemBaseClass
 * Depending on thier role as the Master/Peer process
 */
class BvdSimMutSystemTestBaseClass: public BvdSimMutBaseClass {
public:
    BvdSimMutSystemTestBaseClass(PAbstractDriverReference config);
    virtual ~BvdSimMutSystemTestBaseClass();

    PAbstractSystemConfigurator getSystemConfigurator();

    void RegisterDriver(AbstractDriverReference *driver);

    virtual void BootSystem();
    virtual void ShutdownSystem();
    virtual void Configure_This_SP();
    virtual void Configure_Other_SP();
    virtual void Boot_This_SP();
    virtual void Boot_Other_SP();
    virtual void Shutdown_Other_SP();
    virtual void Halt_Other_SP();

    // enable/delay the master Timeout default is 10 seconds
    void StartInactivityTimer(EMCPAL_TIMEOUT_MSECS timeout = 10*1000);

    void StartPeerShutdownThread();
    virtual void StartUp();
    virtual void TearDown();

    /*
     * Test writers are required to create an implementation for method Test
     */
    virtual void Test() = 0;

    virtual Options *getCommandLineOptions();
    virtual void addCommandLineOptions(Options *options);

    virtual char *getPeerSPProgram() {
        return mut_get_program_name();
    }
protected:
    char                        *mCommandLine;

private:

    virtual char * construct_command();
    static void ShutdownPeerSPCallback(IN PVOID StartContext);
    static void MasterTimeoutCallback(EMCPAL_TIMER_CONTEXT context);

    EMCPAL_MONOSTABLE_TIMER     mInactivityTimer;
    PEMCPAL_THREAD              mpPeerShutdownThread;
    EMCPAL_THREAD               mPeerShutdownThread;
    ArrayDirector               *mSimDirector;

    Options                     *mCommandLineOptions;
};

/*
 * Class BvdSimMutIntegrationTestBaseClass is provided to control 
 * integration level test validation. 
 * This class extends from BvdSimMutSystemTestBaseClass, which contains
 * all of the methods required to configure and manage a system comprised 
 * of multiple drivers and start that system in a 2 Legged stool dual
 * SP environment. Note, by design, Integration level Testing does not
 * launch the Peer SP process.
 */
class BvdSimMutIntegrationTestBaseClass: public BvdSimMutSystemTestBaseClass {
public:
    BvdSimMutIntegrationTestBaseClass(PAbstractDriverReference config);
};

/*
 * Class BvdSimMutMasterSystemBaseClass & BvdSimMutPeerSystemBaseClass are
 * provided to control dual SP simulations.
 *
 * Each SP in a Dual SP simulation is controlled by a MUT test suite
 * that contains a single test.  Each SP process is loosely coupled to
 * the other SP. 
 *
 * The "Peer SP" is started by the "Master SP" during * test StartUP().
 *
 * Both SPs communicate using the ArrayDirector.
 *
 * During test TearDown():
 *      - the Master SP signals to the Peer SP that testing is completed
 *          using the ArrayDirector->ShutdownSystem(). 
 *      - the Peer SP uses the ArrayDirector->MonitorSystem() to 
 *          to be notified that the Master has completed testing
 */
class BvdSimMutMasterSystemBaseClass: public BvdSimMutSystemTestBaseClass {
public:
    BvdSimMutMasterSystemBaseClass(PAbstractDriverReference config);
};

/*
 * Class BvdSimMutSPAOnlyMasterSystemBaseClass is provided for tests 
 * that need to boot a single SP.
 *
 * Also, test that extend from this class can directly invoke
 *          Configure_Other_SP
 *          Boot_Other_SP
 *          Shutdown_other_SP
 * within the test method or in a thread context.
 *
 */
class BvdSimMutSPAOnlyMasterSystemBaseClass:  public BvdSimMutSystemTestBaseClass {
public:
    BvdSimMutSPAOnlyMasterSystemBaseClass(PAbstractDriverReference config)
    : BvdSimMutSystemTestBaseClass(config) { }
};

class BvdSimMutSPAOnlyMasterSPConcurrentIoBaseClass: public BvdSimMutSystemTestBaseClass, public BvdSimMutConcurrentIoLoad {
public:
    BvdSimMutSPAOnlyMasterSPConcurrentIoBaseClass(PAbstractDriverReference config)
    : BvdSimMutSystemTestBaseClass(config), BvdSimMutConcurrentIoLoad(config) { }

    void Test() {
        BvdSimMutConcurrentIoLoad::Test();
    }
};

class BvdSimMutPeerSystemBaseClass: public BvdSimMutSystemTestBaseClass {
public:
    BvdSimMutPeerSystemBaseClass(PAbstractDriverReference config)
    : BvdSimMutSystemTestBaseClass(config) { 
        setTestCapability(BvdSim_Capability_PeerHandshake, TRUE);

        // peer SP processes should never attempt to cleanup FS devices
        setTestCapability(BvdSim_Capability_Delete_FS_On_Startup, false);
        setTestCapability(BvdSim_Capability_Delete_FS_On_TearDown, false);
    }
};

class BvdSimMutMasterSPConcurrentIoBaseClass: public BvdSimMutSystemTestBaseClass, public BvdSimMutConcurrentIoLoad {
public:
    BvdSimMutMasterSPConcurrentIoBaseClass(PAbstractDriverReference config)
    : BvdSimMutSystemTestBaseClass(config), BvdSimMutConcurrentIoLoad(config) { 
        setTestCapability(BvdSim_Capability_LaunchPeer, TRUE);
    }

    void Test() {
        BvdSimMutConcurrentIoLoad::Test();
    }
};

class BvdSimMutPeerSPConcurrentIoBaseClass: public BvdSimMutSystemTestBaseClass, public BvdSimMutConcurrentIoLoad {
public:
    BvdSimMutPeerSPConcurrentIoBaseClass(PAbstractDriverReference config)
    : BvdSimMutSystemTestBaseClass(config), BvdSimMutConcurrentIoLoad(config) {
        setTestCapability(BvdSim_Capability_PeerHandshake, TRUE);
    }

    void Test() {
        BvdSimMutConcurrentIoLoad::Test();
    }
};

//
// NOTE: This class should not be used as a basis for attempting to create 3 legged stool tests
//   All new test development, whether or not it is a multi-sp test should  be done with
//  the classes found in BvdSim3LMutBaseClass.h. Check out the PassThru DualSP3L example
//
class BvdSimMutRemoteSystemTestBaseClass: public BvdSimMutBaseClass, public DefaultSPControl {
public:
    BvdSimMutRemoteSystemTestBaseClass(PAbstractDriverReference driverReference = NULL)
    : BvdSimMutBaseClass(driverReference) {
        setTestCapability(BvdSim_Capability_Simulation, TRUE); // TODO: What does this control
        setTestCapability(BvdSim_Capability_AutoLaunchSPA, TRUE);
        setTestCapability(BvdSim_Capability_AutoLaunchSPB, TRUE);
        /*
         * The user can specify how System LUNS are to be managed
         * All 3legged stool Control processes are responsible for FS cleanup
         */
        setTestCapability(BvdSim_Capability_Delete_FS_On_Startup, useExistingSystemDevices == NULL || !useExistingSystemDevices->isSet());
        setTestCapability(BvdSim_Capability_Delete_FS_On_TearDown, leaveSystemDevices == NULL || !leaveSystemDevices->isSet());
    }

    virtual void        StartUp();
    virtual void        TearDown();
    virtual void        Test() = 0;
    virtual void setTestIndex(UINT_32 testIndex);
    virtual UINT_32 getTestIndex();

    /*
     * Methods provided via DefaultSPControl, that need to be sub-classed
     */
    virtual void        addCommandLineOptions(Options *options, SPIdentity_t sp);

private:
    UINT_32 mTestIndex;
};



/*
 * tests should inherit from one of the BvdSimSimple*MutBaseClasses listed below.
 *
 * The classes above (named BvdSimMut*BaseClass) exists so that templates can be applied to
 * to the class hierarchy, without requiring changes to test code.
 */
class BvdSimSimpleDriverMutBaseClass: public BvdSimMutBaseClass {
public:
    BvdSimSimpleDriverMutBaseClass(PAbstractDriverReference config): BvdSimMutBaseClass(config) {}

    virtual void        Test() = 0;

    void StartUp() {
        BvdSimMutBaseClass::StartUp();
        mut_printf(MUT_LOG_HIGH, "Starting Driverload %s\n", getDriverConfiguration()->getDriverName());
        getDriverConfiguration()->Start_Driver();
        mut_printf(MUT_LOG_HIGH, "Finished Driverload %s\n", getDriverConfiguration()->getDriverName());
    }

    void TearDown() {
        closeDeviceFactory();

        mut_printf(MUT_LOG_HIGH, "Starting DriverUnload %s\n", getDriverConfiguration()->getDriverName());
        getDriverConfiguration()->Stop_Driver();
        mut_printf(MUT_LOG_HIGH, "Finished DriverUnload %s\n", getDriverConfiguration()->getDriverName());

        BvdSimMutBaseClass::TearDown();
    }
};

class BvdSimSimpleIntegrationTestMutBaseClass: BvdSimMutIntegrationTestBaseClass {
public:
     BvdSimSimpleIntegrationTestMutBaseClass(PAbstractDriverReference config)
    : BvdSimMutIntegrationTestBaseClass(config) { }
};

class BvdSimSimpleSystemTestMutBaseClass: public BvdSimMutSystemTestBaseClass {
public:
    BvdSimSimpleSystemTestMutBaseClass(PAbstractDriverReference config)
    : BvdSimMutSystemTestBaseClass(config) { }
};

class BvdSimSimpleMasterSystemMutBaseClass: public BvdSimMutMasterSystemBaseClass {
public:
    BvdSimSimpleMasterSystemMutBaseClass(PAbstractDriverReference config)
    : BvdSimMutMasterSystemBaseClass(config) { }
};

class BvdSimSimpleSPAOnlyMasterSystemMutBaseClass: public BvdSimMutSPAOnlyMasterSystemBaseClass {
public:
    BvdSimSimpleSPAOnlyMasterSystemMutBaseClass(PAbstractDriverReference config)
    : BvdSimMutSPAOnlyMasterSystemBaseClass(config) { }
};

class BvdSimSimpleSPAOnlyMasterSPConcurrentIoBaseClass: public BvdSimMutSPAOnlyMasterSPConcurrentIoBaseClass {
public:
    BvdSimSimpleSPAOnlyMasterSPConcurrentIoBaseClass(PAbstractDriverReference config)
    : BvdSimMutSPAOnlyMasterSPConcurrentIoBaseClass(config) { }
};

//
// NOTE: This class should not be used as a basis for attempting to create 3 legged stool tests
//   All new test development, whether or not it is a multi-sp test should  be done with
//  the classes found in BvdSim3LMutBaseClass.h.  Check out the PassThru DualSP3L example
//
class BvdSimSimplePeerSystemMutBaseClass: public BvdSimMutPeerSystemBaseClass {
public:
    BvdSimSimplePeerSystemMutBaseClass(PAbstractDriverReference config)
    : BvdSimMutPeerSystemBaseClass(config) { }

};

class BvdSimSimpleMaserSPConcurrentIoMutBaseClass: public BvdSimMutMasterSPConcurrentIoBaseClass {
public:
    BvdSimSimpleMaserSPConcurrentIoMutBaseClass(PAbstractDriverReference config)
    : BvdSimMutMasterSPConcurrentIoBaseClass(config) { }
};

class BvdSimSimplePeerSPConcurrentIoMutBaseClass: public BvdSimMutPeerSPConcurrentIoBaseClass {
public:
    BvdSimSimplePeerSPConcurrentIoMutBaseClass(PAbstractDriverReference config)
    : BvdSimMutPeerSPConcurrentIoBaseClass(config) { }
};

template <typename TestClass, typename RunnableClass>
class BvdSimRunnableTest  : public TestClass {
public:

    void Test() {
        RunnableClass *spControl = new RunnableClass(this);
        spControl->run();
        delete spControl;
    }
};



#endif // __BVDSIMMUTBASECLASS__
