/************************************************************************
 *
 *  Copyright (c) 2005,2007 EMC Corporation.
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

//++
// File Name: BvdSimDeviceManagement.h
//     
//
// Contents: 
//    Declaration of the DeviceManagement class.
//--
#ifndef __DeviceManagement__
#define __DeviceManagement__
# include "k10ntddk.h"
#include "EmcPAL_DriverShell.h"
# include "K10AnsiString.h"
# include "generic_types.h"
# include "SinglyLinkedList.h"
# include "K10HashTable.h"
# include "simulation/HashTable.h"

# include "simulation/BvdSimFileIo.h"
# include "simulation/AbstractDriverReference.h"
# include "simulation/ResetManager.h"


#if defined(WDMSERVICES_EXPORT)
#define WDMSERVICEPUBLIC CSX_MOD_EXPORT 
#else
#define WDMSERVICEPUBLIC CSX_MOD_IMPORT 
#endif

// Type of volume. It may be either user volume or private volume like CDR, Vault etc
enum VolumeType {
    UserVolume = 0,
    PrivateVolume = 1
};

// Node part of the K10_WWID for user volume.
#define USER_VOLUME_NODE  1

// Node part of the K10_WWID for system/private volume like CDR & Vault.
#define SYSTEM_VOLUME_NODE  2

class DeviceManagement {
public:
    static WDMSERVICEPUBLIC void AddDevice(PEMCPAL_DEVICE_OBJECT DeviceObject, char *deviceName);
    static WDMSERVICEPUBLIC void RemoveDevice(char *deviceName);
    static WDMSERVICEPUBLIC void RemoveDevice(PEMCPAL_DEVICE_OBJECT deviceObject);
                          
    static WDMSERVICEPUBLIC PEMCPAL_DEVICE_OBJECT FindDeviceObject(char *deviceName);
    static WDMSERVICEPUBLIC K10_WWID findDeviceWWID(char *deviceName);
    static WDMSERVICEPUBLIC K10_WWID getWWIDFromDeviceObject(PEMCPAL_DEVICE_OBJECT deviceObject);

    static WDMSERVICEPUBLIC K10_WWID registerLunWwn(const char *path, K10_WWID Wwn = K10_WWID_Zero());


    static WDMSERVICEPUBLIC void Add_Object_Reference(PEMCPAL_DEVICE_OBJECT deviceObject);
    static WDMSERVICEPUBLIC void Add_Object_Reference(PEMCPAL_FILE_OBJECT fileObject);
    static WDMSERVICEPUBLIC void Dereference_Object(UINT_64 reference);

	static WDMSERVICEPUBLIC void clear();

    static WDMSERVICEPUBLIC void registerAddress(void * addr);
    static WDMSERVICEPUBLIC void unRegisterAddress(void *addr);
    static WDMSERVICEPUBLIC void checkAddress(void *addr);

    static WDMSERVICEPUBLIC void RegisterDriver(AbstractDriverReference *driver);
    static WDMSERVICEPUBLIC void BootSystem();
    static WDMSERVICEPUBLIC void ShutdownSystem();
    static WDMSERVICEPUBLIC void registerResetManager(ResetManager *resetManager);
    static WDMSERVICEPUBLIC K10_WWID ConvertIndexToWWID(ULONG index, VolumeType type);
    static WDMSERVICEPUBLIC ULONG ConvertWWIDToIndex(K10_WWID key);
    static WDMSERVICEPUBLIC K10_WWID ConvertStringToWWID(const char * stringWWID);

};

#if DMREG
#define WDMI_Registered_Irp(_Irp) csx_p_dcall_verify(_Irp)
#else
#define WDMI_Registered_Irp(_Irp) do { } while(0)
#endif

#endif // __DeviceManagement__
