/************************************************************************
 *
 *  Copyright (c) 2002, 2005-2009 EMC Corporation.
 *  All rights reserved.
 *
 *  ALL RIGHTS ARE RESERVED BY EMC CORPORATION.  ACCESS TO THIS
 *  SOURCE CODE IS STRICTLY RESTRICTED UNDER CONTRACT. THIS CODE IS TO
 *  BE KEPT STRICTLY CONFIDENTIAL.
 *
 *  UNAUTHORIZED MODIFICATIONS OF THIS FILE WILL VOID YOUR SUPPORT
 *  CONTRACT WITH EMC CORPORATION.  IF SUCH MODIFICATIONS ARE FOR
 *  THE PURPOSE OF CIRCUMVENTING LICENSING LIMITATIONS, LEGAL ACTION
 *  MAY RESULT.
 *
 ************************************************************************/

#ifndef __BasicDriver__
#define __BasicDriver__

#include "k10ntddk.h"
#include "EmcPAL_DriverShell.h"
# include "BasicLib/ConfigDatabase.h"
# include "CmiUpperInterface.h"
# include "SinglyLinkedList.h"

//++
// File Name:
//      BasicDriver.h
//
// Contents:
//  Defines the BasicDriver class.
//  Defines the BasicDriverEntryPoint class.
//
//
// Revision History:
//--

class BasicDriverEntryPoint;
class BasicDriver;

// List of drivers.
// We use this list to store the drivers which has started within the single binary.
SLListDefineListType(BasicDriver,ListOfBasicDriver);

class BasicDriver {
public:
    //   Called from GenericDriverEntry routine to start up the driver.
    //
    //   @param rootKey - Parameter to ::DriverEntry.
    //   @param spid - SPID of this SP.
    //   @param driverName: Display name of the Driver.
    //
    // Return Value:
    //   STATUS_SUCCESS - Driver was started. 
    //   Error - Driver cannot be started.
    virtual EMCPAL_STATUS DriverEntry(DatabaseKey * rootKey, CMI_SP_ID spid, char* driverName) = 0;
//	virtual EMCPAL_STATUS DriverEntry(DatabaseKey * rootKey) = 0;

    //   Called from BasicLib::cppDriverEntry routine.
    //   It performs some common operation, like set dispatch rotines, open 
    //   parameters registry key etc, which is applicable for the drivers 
    //   derived from BasicDriver/BasicVolumeDriver. After that it calls 
    //   driver specific DriverEntry routine.
    //
    //   @param DriverObject - Driver Object.
    //   @param RegistryPath - Path of the driver specific registry.
    //   @param driverName: Display name of the Driver.
    //
    // Return Value:
    //   STATUS_SUCCESS - Driver was started. 
    //   Error - Driver cannot be started.
    EMCPAL_STATUS GenericDriverEntry(PEMCPAL_NATIVE_DRIVER_OBJECT DriverObject, const PCHAR  RegistryPath, char* driverName);

    // Constructor.
    // We maintains list of driver i.e. sRegisteredDriver. So, wheenver our constructor
    // gets called we store this pointer in the list.
    BasicDriver();

    // Desturctor.    
    virtual ~BasicDriver();
    
    // This is the driver specific unload routine. Each driver has to implement its own
    // DriverUnload routine.
    virtual EMCPAL_STATUS  DriverUnload() = 0;

    // It stores driver entry point to the global list so whenever cppDriverEntry
    // calls we can remove the each driver entry point from the global list and call
    // CreateDriver() routine.
    static VOID RegisterDriver(BasicDriverEntryPoint* driver);      

    //
    // Returns DrvierEntryPoint from the list of Registered Driver Entrypoints
    static BasicDriverEntryPoint *FindDriverEntryPoint(char *name);    

    static ListOfBasicDriver sRegisteredDriver;

    BasicDriver* mNext;

    // Write entry to NT Event Log
    //
    // @param MessageNumber:   the logging code to be used in the new entry.
    // @param ArgFormatString: an optional wide format string.
    // @param...:  optional arguments for the insertion strings,
    //	whose presentation formats are described in <ArgFormatString>.
    virtual VOID WriteLogEntry(int MessageNumber, csx_pchar_t ArgFormatString = CSX_NULL, ... ) __attribute__((format(__printf_func__,3,4)));

private:

    PEMCPAL_NATIVE_DRIVER_OBJECT mDriverObject;
};

// Driver unload routine which we register as a driver unload rotine during GenericDriverEntry.
// This routine gets called while unloading the drivers. It removes drivers from sRegisteredDriver
// list one by one and calls DriverUnload() for each driver.
// Note: 3 overloaded function for diver unload. one each for PAL and pre-PAL style unload 
//          handler signature. One actual implementation - invoked from above two
EMCPAL_STATUS BasicDriverGenericDriverUnload(PEMCPAL_DRIVER DriverObject);
//Pre-PAL style unload handler - required form simulation mode handling
VOID BasicDriverGenericDriverUnload(IN PEMCPAL_NATIVE_DRIVER_OBJECT pDriverObject);
//Unload handler impletenmatio for both - pre-PAL and PAL style unload handler
EMCPAL_STATUS BasicDriverGenericDriverUnload();


SLListDefineInlineCppListTypeFunctions(BasicDriver, ListOfBasicDriver, mNext);

// This class is a base calss for DriverEntryPoint classes. All the drivers
// which is derived from BVD needs to create one class which has to be derived
// from this (BasicDriverEntryPoint) class.
// Derived class has to implement CreateDriver() routine as it is pur virtual
// function. CreateDriver() routine should have code related to DriverEntry().
class BasicDriverEntryPoint{
public:
    BasicDriverEntryPoint(BOOT_SW_COMPONENT_CODE component = static_cast<BOOT_SW_COMPONENT_CODE>(SW_COMPONENT_LIMIT+1));
    BasicDriverEntryPoint(char* name, BOOT_SW_COMPONENT_CODE component = static_cast<BOOT_SW_COMPONENT_CODE>(SW_COMPONENT_LIMIT+1));

    /*
     * This method is only used in Simulation. The simulator does not make use of BasicDriver::CppDriverEntry,
     * opting for loading individual drivers, as needed
     */
    EMCPAL_STATUS CppDriverEntry( IN PEMCPAL_NATIVE_DRIVER_OBJECT DriverObject, const PCHAR RegistryPath);


    // It will allocate memory for the driver and return driver's pointer.
    virtual BasicDriver* CreateDriver(const PCHAR RegistryPath) = 0;
    

    // generic routine used to initialize memory 
    // this routine can/should perform CMM initialization
    virtual EMCPAL_STATUS MemoryInit(const PCHAR RegistryPath);

    // Returns driver name.
    char* GetDriverName() {
        return mDriverName;
    }    
    
    BOOT_SW_COMPONENT_CODE GetComponent() const { return mComponent; }

    BasicDriverEntryPoint*   mNext;

protected:
    // Stores driver's name. It will be either read from registry "DisplayName" or
    // user specified.
    char* mDriverName;
    // sw component
    BOOT_SW_COMPONENT_CODE mComponent;
};

// List of BasicDriverEntryPoint. This list maintains the list of driver entry point
// and we have to create & call the DriverEntry for each registered driver entry point.
SLListDefineListType(BasicDriverEntryPoint,ListOfBasicDriverEntryPoint);
SLListDefineInlineCppListTypeFunctions(BasicDriverEntryPoint, ListOfBasicDriverEntryPoint, mNext);

#endif  // __BasicDriver__
