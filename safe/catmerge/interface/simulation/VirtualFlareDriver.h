/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               VirtualFlareDriver.h
 ***************************************************************************
 *
 * DESCRIPTION: VirtualFlare Driver API definitions
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    07/13/2009  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef __VIRTUALFLAREDRIVER
#define __VIRTUALFLAREDRIVER

# include "k10ntddk.h"
#include "EmcPAL_DriverShell.h"
# include "generic_types.h"
# include "BasicVolumeDriver/BasicNotificationVolume.h"
# include "BasicVolumeDriver/BasicVolumeDriver.h"
# include "core_config_runtime.h"
# include "simulation/AbstractDriverReference.h"
# include "simulation/DiskGeometry.h"
# include "simulation/BvdSimFileIo.h"
# include "BasicLib/BasicIoRequest.h"
# include "BasicLib/BasicThread.h"
# include "simulation/VirtualFlareLogicalTable.h"
# include "simulation/BvdSimDeviceManagement.h"
# include "BasicLib/BasicCPUGroupThreadPriorityMonitorInterface.h"

#if defined(VFDRIVER_EXPORT)
#define VFDRIVERPUBLIC CSX_MOD_EXPORT 
#else
#define VFDRIVERPUBLIC CSX_MOD_IMPORT 
#endif

#define VIRTUAL_FLARE_DEVICE_PREFIX "\\Device\\VirtualFlare"

// Below defines used while creating volumes from the registry
// Default volume size if it is not specified in the registry
#define DEFAULT_VOLUME_SIZE_IN_SECTORS  ((100 * 1024 * 1024) / 512) // 100 MB

// Defines smaller vault size which is lesser than cache size i.e. 64 MB
#define SMALLER_VAULT_SIZE_IN_SECTORS  ((30 * 1024 * 1024) / 512) // 30 MB

// Default sector size if it is not specified in the registry.
#define DEFAULT_SECTOR_SIZE     520

// Default RG for the vault and PSM
#define PSM_RG_ID   1000
#define VAULT_RG_ID 1003

#define DEFAULT_RAID_NUMBER_OF_DRIVES (4+1)

// FIXME: After including BasicVolumeDriver.h I was getting deprecated errors for
//      sprintf.
#pragma warning(push)
#pragma warning(disable: 4995)

// Forward declaration
class VirtualFlareVolumeParams;
class VirtualFlareVolume;

class VirtualFlareDriver : public BasicVolumeDriver, public Runnable,
    // BasicPerCpuThreadPriorityMonitor is in a separate component, but this class makes
    // it behave as if VFD inherits from BasicPerCpuThreadPriorityMonitor
    public BasicPerCpuThreadPriorityMonitorProxy
{

public:

    VirtualFlareDriver();
    
    EMCPAL_STATUS DriverEntryAfterConstructingDevices(DatabaseKey * parameters) {
        SPID spId = MySpId();

        TraceStartup("VFD::DriverEntryAfterConstructingDevices started");

        if(!BasicPerCpuThreadPriorityMonitorProxy::Attach()) {
            TraceStartup("VFD::DriverEntryAfterConstructingDevices FAILED");
            return EMCPAL_STATUS_INSUFFICIENT_RESOURCES;
        }

        // If it is SPA then we pass true to SetSPID otherwise false. Logical table has its own
        // enumeration to define SPA & SPB.
        mLogicalTable->SetSPID((spId.engine == 0) ? SPA : SPB /* Is SPA? */);

        mLVDTNotifierService = mLogicalTable->GetNotifierService();
        mNotificationProcessThread->start();
        TraceStartup("VFD::DriverEntryAfterConstructingDevices finished");
        return EMCPAL_STATUS_SUCCESS;
    }

    // Override of the base class function.  Initializes the driver, allocates memory, etc.
    // @param parameters - an open registry key starting at
    //                      ...\CurrentControlSet\<DriverName>\Parameters
    // Returns: success if driver should continue loading, failure otherwise
    EMCPAL_STATUS DriverEntryBeforeConstructiongDevices(DatabaseKey * parameters)
    {
        TraceStartup("VFD::DriverEntryBeforeConstructiongDevices started");

        TraceStartup("VFD::DriverEntryBeforeConstructiongDevices finished");

        return EMCPAL_STATUS_SUCCESS;
    }

    ~VirtualFlareDriver() {        
        delete mNotificationProcessThread;
    }
    // Overridden from Runnable.
    void run()
         {
              while(true) {
                 if(mLVDTNotifierService != NULL) {
                     mLVDTNotifierService->wait(1000);
                     UINT_32 volHashIndex = (UINT_32)mLVDTNotifierService->get_flags();  

                     //KvPrint("Notified (%s) flags = %d",(MySpId().engine == 0) ? "SPA" : "SPB", volHashIndex);
                     
                     // If volume table index table is set to greather then max volume supported
                     // then it means virtual flare driver is about to unload.
                     if(volHashIndex == (SERVICE_INITIALIZED)) {
                         continue;
                     }
                     else if(volHashIndex == (SERVICE__STOPPED)) {
                         break;
                     }
                     else {
                         VolumeDataTableNotificationHandler(volHashIndex);
                     }
                  }
                 else {
                     break;// Just exit if no service
                  }
              }
          }


    void AboutToUnload()  {
        // Set flags to max volume supported + 2 which cause to notifier thread to stop.
        mLVDTNotifierService->set_flags(SERVICE__STOPPED);
        mLVDTNotifierService->notify();
        mNotificationProcessThread->stop();
    }
    
    VirtualFlareLogicalTable * GetLogicalTable() {
        return mLogicalTable;
    }
    
    void VolumeDataTableNotificationHandler(UINT_32 volTableIndex);
    
    // Overridden from Runnable.
    const char *getName() {
        return "VirtualFlareVolumeData";
    }

    ULONG SizeOfVolume() const;

    ULONG SizeOfVolumeParams() const;
       
    BasicVolume *  DeviceConstructor(BasicVolumeParams * volumeParams, 
        ULONG volumeParamsLen, VOID * memory);

    // Overridden from BasicVolumeDriver.
    // It fills volume parameter by reading the registry. This routine only gets
    // called when we creates volume from the registry.
    bool FillInVolumeParametersFromRegistry(DatabaseKey * deviceParams, 
                BasicVolumeParams * params) OVERRIDE;    

private:
    // Logical table which keeps track of the volume state for both the SPs.
    VirtualFlareLogicalTable * mLogicalTable;

    // Notifier service which triggers when any changes made to the volume logical data table.
    shmem_service * mLVDTNotifierService;

    // Seperate thread to process the notification from the volume logical table.
    Thread * mNotificationProcessThread;    
};

class VirtualFlareDriverEntryPoint : public BasicDriverEntryPoint 
{
public:

    // Constructor. It will register ourself with the BasicLib.
    VirtualFlareDriverEntryPoint() : BasicDriverEntryPoint("VirtualFlare") {}

    VirtualFlareDriverEntryPoint(char* name) : BasicDriverEntryPoint(name) {}

    //++
    // Function:
    //      CreateDriver
    //
    // Description:
    //      This function allocates memory for the driver.
    //
    // Return Value:
    //      returns pointer of the BasicDriver.
    //      returns NULL it it is not able to allocate memory for the driver.
    //  
    BasicDriver* CreateDriver(const PCHAR RegistryPath);

    // generic routine used to initializememory 
    // this routine can/should perform CMM initialization
    EMCPAL_STATUS MemoryInit(const PCHAR RegistryPath) {
         ccrInit();

        return EMCPAL_STATUS_SUCCESS;
    }
};

class VirtualFlareIOMonitor {
public:
    virtual ~VirtualFlareIOMonitor() {} 
    virtual void VFD_IO_Starting() = 0;
    virtual void VFD_IO_Completed() = 0;
    virtual void VFD_Read_Sector(UINT_64 LBA, UINT_8 *buffer) = 0;
    virtual void VFD_Write_Sector(UINT_64 LBA, UINT_8 *buffer) = 0;

    virtual void VFD_Read_Sectors(UINT_64 LBA, UINT_64 noLba, UINT_8 *buffer) = 0;
    virtual void VFD_Write_Sectors(UINT_64 LBA, UINT_64 noLba, UINT_8 *buffer) = 0;
};

class VirtualFlareIODummyMonitor: public VirtualFlareIOMonitor {
    virtual void VFD_IO_Starting() {}
    virtual void VFD_IO_Completed() {}
    virtual void VFD_Read_Sector(UINT_64 LBA, UINT_8 *buffer) {}
    virtual void VFD_Write_Sector(UINT_64 LBA, UINT_8 *buffer) {}

    virtual void VFD_Read_Sectors(UINT_64 LBA, UINT_64 noLba, UINT_8 *buffer) {}
    virtual void VFD_Write_Sectors(UINT_64 LBA, UINT_64 noLba, UINT_8 *buffer) {}
};

PAbstractDriverReference get_VirtualFlare_DriverReference(ULONG vaultSize = DEFAULT_VOLUME_SIZE_IN_SECTORS);

#ifdef __cplusplus
/*
 * all of these functions need standard C linkage.  Otherwise, they can not be referenced from C.
 * Note, the implementation of these functions is within a cpp file, which allows these functions to
 * access C++ instances, instance methods and class methods
 */
extern "C" {
#endif

typedef VOID (*Register_Io_Notification_Callback)(const EMCPAL_IRP *irp, EMCPAL_STATUS status, void *context);
typedef bool (*Register_Is_Device_Ready_Callback)(void* Context);

extern VFDRIVERPUBLIC PEMCPAL_NATIVE_DRIVER_OBJECT VirtualFlare_getDriverObject();
    
extern VFDRIVERPUBLIC VirtualFlareVolume* VirtualFlare_create_FSVolume(BasicVolumeDriver *driver, VOID * memory, VirtualFlareVolumeParams *volumeParams);
extern VFDRIVERPUBLIC ULONG VirtualFlare_Sizeof_FSVolume();
extern VFDRIVERPUBLIC VirtualFlareVolume* VirtualFlare_create_MCRVolume(BasicVolumeDriver *driver, VOID * memory, VirtualFlareVolumeParams * volumeParams);
extern VFDRIVERPUBLIC ULONG VirtualFlare_Sizeof_MCRVolume();
extern VFDRIVERPUBLIC VirtualFlareVolume* VirtualFlare_create_PatternedVolume(BasicVolumeDriver *driver, VOID * memory, VirtualFlareVolumeParams * volumeParams);
extern VFDRIVERPUBLIC ULONG VirtualFlare_Sizeof_PatternedVolume();
extern void VFDRIVERPUBLIC VirtuaFlareDriver_register_for_IO_notification(BvdSimFileIo_File *device, Register_Io_Notification_Callback callback);
extern void VFDRIVERPUBLIC VirtuaFlareDriver_register_IO_Monitor(BvdSimFileIo_File *device, VirtualFlareIOMonitor *monitor);

#ifdef __cplusplus
};
#endif

#endif // __VIRTUALFLAREDRIVER

