/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               AbstractDriverReference.h
 ***************************************************************************
 *
 * DESCRIPTION: Abstract Driver Reference class definition
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    10/16/2009  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef __ABSTRACTDRIVERREFERENCE__
#define  __ABSTRACTDRIVERREFERENCE__
#include "k10ntddk.h"
#include "EmcPAL_DriverShell.h"
# include "generic_types.h"
# include "k10ntstatus.h"
# include "simulation/AbstractSystemConfigurator.h"
# include "simulation/AbstractDriverConfiguration.h"

class BvdSimMutBaseClass;
class BvdSimMutSystemTestBaseClass;

class DriverGetRegistryListener {
public:
    /*
     * \fn PBvdSim_Registry_Data postGetRegistryData()
     * \param PBvdSim_Registry_Data data - registry data for driver.
     * \details
     *  postGetRegistryData is called when getRegistryData completes.
     *  This notification is provided for tests that need to perform additional
     *  configuration 
     */
    virtual void postGetRegistryData(PBvdSim_Registry_Data data) = 0;
    virtual ~DriverGetRegistryListener() {};
};

typedef DriverGetRegistryListener *PDriverGetRegistryListener;

/*
 * A default device construction listener that does nothing
 */
class DefaultDriverGetRegistryListener: public DriverGetRegistryListener {
public:
    virtual void postGetRegistryData(PBvdSim_Registry_Data data) {}    
};



typedef EMCPAL_STATUS (*CmmInit_t)(const char *RegistryPath);

typedef EMCPAL_STATUS (*DriverEntry)(PEMCPAL_NATIVE_DRIVER_OBJECT pDriverObject, const char *RegistryPath);

class AbstractDriverReference: public AbstractSystemConfigurator, public AbstractDriverConfiguration {
public:
    AbstractDriverReference(CmmInit_t MemoryInit = NULL, const char **RequiredDevices = NULL, const char **ExportedDevices = NULL);
    virtual ~AbstractDriverReference() {}

    /*
     *********************************************************************
     * These methods are required for interface AbstractDriverConfiguration
     *********************************************************************
     */
    virtual const char              *getDriverName() = 0;
    virtual PDeviceConstructor       getDeviceFactory() = 0;
    virtual PBvdSim_Registry_Data    getRegistryData() = 0;
    virtual const PCHAR              getDriverRegistryPath();
    virtual PIOTaskId                getDefaultIOTaskId() = 0;
    virtual void                     setCurrentTest(BvdSimMutBaseClass *test);
    virtual BvdSimMutBaseClass      *getCurrentTest();
    virtual BvdSimMutBaseClass      *getSystem(); // alias for getCurrentTest()


    /*
     *********************************************************************
     * These methods are required for interface AbstractDriverReference
     *********************************************************************
     */
    virtual void                     InitRegistry(PBvdSim_Registry_Data registry_data = NULL);
    virtual void                     LoadDriver(PEMCPAL_NATIVE_DRIVER_OBJECT DriverObject, const PCHAR DriverRegistryPath) = 0;
    virtual void                     UnloadDriver(PEMCPAL_NATIVE_DRIVER_OBJECT DriverObject) = 0;

    virtual void                     PreLoadDriverSetup();      // implement as needed to perform additional setup
    virtual void                     PostLoadDriverSetup();     // implement as needed to perform additional setup
    virtual void                     PreUnloadDriverCleanup();  // implement as neeeded to perform additional cleanup
    virtual void                     PostUnloadDriverCleanup(); // implement as neeeded to perform additional cleanup

    virtual const char               **getRequiredDevices();
    virtual const char               **getExportsDevices();

    virtual EMCPAL_STATUS            MemoryInit(const PCHAR RegistryPath);

    virtual void                    Commit();

    PEMCPAL_NATIVE_DRIVER_OBJECT                   getDriverObject();

    void Start_Driver();
    void Stop_Driver();

    /*
     *********************************************************************
     * These methods are required for interface AbstractSystemConfigurator
     * Test suites that extend from BvdSimMutSystemTestBaseClass need to 
     * to use these methods, particularly configureSystemDrivers()
     *********************************************************************
     */

    /*
     * Method configureSystemDrivers() is responsible for
     * registering all of the Drivers that are required to be 
     * running in an SP test process.
     */
    virtual void configureSystemDrivers();


    virtual AbstractThreadConfigurator *getThreadConfigurator();


    AbstractDriverReference *mLink;


    void SetGetRegistryListener(DriverGetRegistryListener* listener) {
        mDriverGetRegistryListener = listener;
    }

    DriverGetRegistryListener* getGetRegistryListener() {
        return mDriverGetRegistryListener;
    }

private:

    BvdSimMutBaseClass *mTest;
    CmmInit_t       mMemoryInit;
    const char      **mRequiredDevices;
    const char      **mExportedDevices;

    PEMCPAL_NATIVE_DRIVER_OBJECT   mDriverObjectPtr;

    PDriverGetRegistryListener     mDriverGetRegistryListener;
};

typedef AbstractDriverReference *PAbstractDriverReference;

class DelegateDriverReference: public AbstractDriverReference {
public:
    DelegateDriverReference(PAbstractDriverReference dr): mDelegateDR(dr), mTest(NULL) {}

    virtual const char              *getDriverName() {
        return getDelegate()->getDriverName();

    }

    virtual PDeviceConstructor       getDeviceFactory() {
        return getDelegate()->getDeviceFactory();
    }

    virtual PBvdSim_Registry_Data    getRegistryData() {
        return getDelegate()->getRegistryData();
    }
    virtual const PCHAR    getDriverRegistryPath() {
        return getDelegate()->getDriverRegistryPath();
    }

    virtual PIOTaskId                getDefaultIOTaskId() {
        return getDelegate()->getDefaultIOTaskId();
    }

    virtual void                     setCurrentTest(BvdSimMutBaseClass *test) {
        mTest = test;
        getDelegate()->setCurrentTest(test);
    }

    BvdSimMutBaseClass *getSystem() {
        return getDelegate()->getSystem();
    }


    /*
     * These methods are required for interface AbstractDriverReference
     */
    virtual void                     InitRegistry() {
        getDelegate()->InitRegistry(getRegistryData());
    }

    virtual void                     LoadDriver(PEMCPAL_NATIVE_DRIVER_OBJECT DriverObject, const PCHAR registryPath) {
        getDelegate()->LoadDriver(DriverObject, registryPath);
    }

    virtual void                     UnloadDriver(PEMCPAL_NATIVE_DRIVER_OBJECT DriverObject) {
        getDelegate()->UnloadDriver(DriverObject);
    }

    virtual void                     PreLoadDriverSetup() {
        getDelegate()->PreLoadDriverSetup();
    }

    virtual const char               **getRequiredDevices() {
        return getDelegate()->getRequiredDevices();
    }

    virtual const char               **getExportsDevices() {
        return getDelegate()->getExportsDevices();
    }

    virtual EMCPAL_STATUS            MemoryInit(const PCHAR RegistryPath) {
        return getDelegate()->MemoryInit(RegistryPath);
    }

    virtual void configureSystemDrivers() {
        getDelegate()->configureSystemDrivers();
    }

    virtual AbstractThreadConfigurator *getThreadConfigurator() {
        return getDelegate()->getThreadConfigurator();
    }

    PAbstractDriverReference getDelegate() {
        return mDelegateDR;
    }

    void Commit() {
        getDelegate()->Commit();
    }

private:
    PAbstractDriverReference    mDelegateDR;
    BvdSimMutBaseClass          *mTest;
};


PVOID getSharedSymbol(const char *sharedLibrary, const char *symbol);
PAbstractDriverReference get_SharedDriverReference(const char *sharedLibrary, const char *symbol = "get_DriverReference");

#endif //__ABSTRACTDRIVERREFERENCE__
