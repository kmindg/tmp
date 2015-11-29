/***************************************************************************
 * Copyright (C) EMC Corporation 2011-2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               VirtualFlareDriverReference.h
 ***************************************************************************
 *
 * DESCRIPTION: VirtualFlareDriverReference definitions
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    07/26/2011  Bhavesh Patel Initial Version
 *
 **************************************************************************/
#ifndef __VIRTUALFLAREDRIVERREFERENCE
#define __VIRTUALFLAREDRIVERREFERENCE

#include "simulation/BasicDriverEntryPoint_DriverReferenceAdapter.h"
#include "simulation/BvdSim_Admin.h"
#include "simulation/BvdSimSyncTransaction.h"

#define PSM_RG_ID   1000

static AbstractDriverReference *VirtualFlare_DriverReference = NULL;

static VirtualFlareDriverEntryPoint myDriver("VirtualFlare");
static const char *VirtualFlareExports[]   = {"\\Device\\CLARiiONDisk"};
static const char *VirtualFlareImports[]   = {"\\Device\\Cmid"};

BasicDriverEntryPoint *VirtualFlareDriverEntryPoint = &myDriver;

CSX_MOD_EXPORT BasicDriverEntryPoint *get_VirtualFlareDriverEntryPoint() {
    return VirtualFlareDriverEntryPoint;
}

class VirtualFlareDriverReference: public BasicDriverEntryPoint_DriverReferenceAdapter {
public:
    VirtualFlareDriverReference(ULONG vaultSize)
    : mRegistryData(NULL), mVaultSize(vaultSize), 
      BasicDriverEntryPoint_DriverReferenceAdapter(get_VirtualFlareDriverEntryPoint(), VirtualFlareImports, VirtualFlareExports)
    {}
    

    void LoadDriver(PEMCPAL_NATIVE_DRIVER_OBJECT DriverObject) {
        /*
         * Perform normal driver initialization
         */
        NTSTATUS Status;

        Status = get_VirtualFlareDriverEntryPoint()->CppDriverEntry(getDriverObject(), (const PCHAR)getDriverName());            
                
        FF_ASSERT(NT_SUCCESS(Status));
    }

    /*
     * These methods are required for interface AbstractDriverConfiguration
     */
    const char              *getDriverName() {
        return "VirtualFlare";
    }

    PDeviceConstructor       getDeviceFactory() {
        return NULL;
    }

    void PreLoadDriverSetup() {
        // VFD still requires the capability flag set to enable usage of syn transaction library.
        // We can't make it enable by default as some of the test like pass thru tests are
        // not using PSM.
        if(getSystem() && getSystem()->getTestCapability(BvdSim_Capability_PersistentConfiguration)
          && getSystem()->getTestCapability(BvdSim_Capability_PersistentDeviceCreation)) {
            /*
             * Configure the BvdSimSyncTransation switch to use the real SyncTransaction library
             */
            BvdSim_set_SyncTransaction_enable(true);
        }
    }
    
    PBvdSim_Registry_Data    getRegistryData()  {
        if(mRegistryData == NULL) {
            PBvdSimRegistry_Values BvdData = new BvdSimRegistry_Values(getDriverName());
            mRegistryData = new BvdSim_Registry_Data(getDriverName(), getDriverName(), BvdData);

            // Add registry keys to create volumes while loading the driver.
            // CDR volume
            mRegistryData->addParameter(new BvdSimRegistry_Value("DeviceName", "\\Device\\CDR", "CDR"));
            mRegistryData->addParameter(new BvdSimRegistry_Value("DeviceNameLink", "\\DosDevices\\CDR", "CDR"));
            mRegistryData->addParameter(new BvdSimRegistry_Value("WWN", CDR_KEY, "CDR"));

            // VAULT volume
            mRegistryData->addParameter(new BvdSimRegistry_Value("DeviceName", "\\Device\\VaultLUN", "VAULT"));
            mRegistryData->addParameter(new BvdSimRegistry_Value("DeviceNameLink", "\\DosDevices\\VaultLUN", "VAULT"));
            mRegistryData->addParameter(new BvdSimRegistry_Value("WWN", VAULT_KEY, "VAULT"));
            mRegistryData->addParameter(new BvdSimRegistry_Value("SizeInSectors", mVaultSize, "VAULT"));
            
            // PSM Volume
            mRegistryData->addParameter(new BvdSimRegistry_Value("DeviceName", "\\Device\\PSM", "PSM"));
            mRegistryData->addParameter(new BvdSimRegistry_Value("DeviceNameLink", "\\DosDevices\\PSM", "PSM"));
            mRegistryData->addParameter(new BvdSimRegistry_Value("RaidGroupID", PSM_RG_ID, "PSM"));            
            mRegistryData->addParameter(new BvdSimRegistry_Value("WWN", PSM_KEY, "PSM"));            

            // FDR Volume
            mRegistryData->addParameter(new BvdSimRegistry_Value("DeviceName", "\\Device\\FDR", "FDR"));
            mRegistryData->addParameter(new BvdSimRegistry_Value("DeviceNameLink", "\\DosDevices\\FDR", "FDR"));
            mRegistryData->addParameter(new BvdSimRegistry_Value("RaidGroupID", PSM_RG_ID, "FDR"));            
            mRegistryData->addParameter(new BvdSimRegistry_Value("WWN", FDR_KEY, "FDR"));            

            mRegistryData->addParameter(new BvdSimRegistry_Value("MaxVolumes", 0x4000));
            mRegistryData->addParameter(new BvdSimRegistry_Value("MaxEdges", 0x1));
            mRegistryData->addParameter(new BvdSimRegistry_Value("DatabaseName", "VFD_Config", "Factory"));
            /*
             * LoadOnIOCTL defers loading Persistent DBMS until after the PSM has been loaded
             * See BasicVolumeDriver.cpp/BasicVolumeDriver::ConsiderCreatePersistenceFactory for more details
             * Note, The PSM DriverReference will issue the appropriate IOCTL after the PSM is loaded
             */
            mRegistryData->addParameter(new BvdSimRegistry_Value("LoadOnIOCTL", 0x1, "Factory"));

        }

        return mRegistryData;
    }

    PIOTaskId                getDefaultIOTaskId() {
        return NULL;
    }

    const char               **getRequiredDevices() {
        return NULL;
    }

    const char               **getExportsDevices() {
        return VirtualFlareExports;
    }

private:

    PBvdSim_Registry_Data   mRegistryData;
    ULONG mVaultSize;
};

// routine provided so that test code can load th VirtualFlare driver
AbstractDriverReference *VirtualFlare_get_DriverReference(ULONG vaultSize = DEFAULT_VOLUME_SIZE_IN_SECTORS)
{
    if(VirtualFlare_DriverReference == NULL) {
        
        KvPrint("VirtualFlare_get_DriverReference() allocating Virtual Flare driver reference. Vault Size: %d (%lldMB)\n",
            vaultSize, (UINT_64) ((UINT_64) vaultSize * (UINT_64) BYTES_PER_SECTOR)/(UINT_64) (1024*1024));

        VirtualFlare_DriverReference = new VirtualFlareDriverReference(vaultSize);
    }

    return VirtualFlare_DriverReference;
}


#endif // __VIRTUALFLAREDRIVERREFERENCE


