#ifndef EMCPAL_DRIVER_SHELL_H
#define EMCPAL_DRIVER_SHELL_H
/*!
 * @file EmcPAL_DriverShell.h
 **************************************************************************
 Copyright (C) EMC Corporation 2008-2011
 All rights reserved.
 Licensed material -- property of EMC Corporation
 *************************************************************************
 */


 /*!
 * File:           EmcPAL_DriverShell.h
 *
 * @brief     This file contains types and interfaces exported to PAL-enabled
 *                  drivers.  By linking with the PAL driver shell a driver is
 *                  automatically registered as a PAL client at startup time and
 *                  may access the core PAL interfaces.  PAL drivers are similar
 *                  to Windows drivers in that they define a driver entry-point
 *                  and optional unload routine that are called by the PAL infrastructure
 *                  during driver initialization and teardown.
 *                  
 * Types:
 *      PEMCPAL_NATIVE_DRIVER_OBJECT (defined in EmcPAL.h)
 *      PEMCPAL_NATIVE_REGISTRY_PATH
 *      EMCPAL_DRIVER_UNLOAD_CALLBACK
 *      EMCPAL_DRIVER
 *
 * External Variables:
 *      EmcpalCurrentDriver
 *
 * Imported Function Prototypes (user defined):
 *      EmcpalDriverEntry
 *
 * Interface Functions:
 *      EmcpalDriverSetUnloadCallback()
 *      EmcpalDriverGetUnloadCallback()
 *      EmcpalDriverGetDriverName()
 *      EmcpalDriverGetRegistryPath()
 *      EmcpalDriverSetRegistryPath()
 *      EmcpalDriverGetClientObject()
 *      EmcpalDriverGetCurrentClientObject()
 *      EmcpalDriverGetNativeDriverObject()
 *      EmcpalDriverGetNativeRegistryPath()
 *      EmcpalGetCurrentDriver()
 *
 * Internal Functions:
 *
 * @version
 *      12-08		CGould		Created.<br />
 *      03/09       CGould      Added routines to store/access driver's native registry path
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

//++
// Include files
//--

#include "EmcPAL.h"
#include "EmcPAL_Irp.h"

//++
// .End Include files
//--

//++
// Forward Declarations
//--

struct _EMCPAL_DRIVER;


//++
// .End Forward Declarations
//--


/*!
 *      PEMCPAL_NATIVE_REGISTRY_PATH
 *
 * @brief
 *      Native platform registry path.  On CLARiiON this is a pointer
 *      to a EMCPAL_UNICODE_STRING containing the driver registry path.  On
 *      SSPG there is no native registry path so this is unused.
 *
 */
typedef EMCPAL_PUNICODE_STRING  PEMCPAL_NATIVE_REGISTRY_PATH;
//.End PEMCPAL_NATIVE_REGISTRY_PATH


/*!
 * Function Type:
 *      EMCPAL_DRIVER_UNLOAD_CALLBACK 
 *
 * @brief
 *      This is the driver unload callback handler type for user-defined driver
 *      unload handlers.  A driver can get/set an optional unload callback handler
 *      using the EmcpalDriverGetUnloadCallback and EmcpalDriverSetUnloadCallback
 *      interface functions described below.
 *
 */
typedef EMCPAL_STATUS (*EMCPAL_DRIVER_UNLOAD_CALLBACK)(struct _EMCPAL_DRIVER *pDriverObject);
//.End


/*!
 * Type:
 *      EMCPAL_DRIVER
 *
 * @brief
 *      This structure contains the state information associated
 *      with a PAL-enabled driver.  A driver object is created
 *      for each PAL driver as it is loaded and passed to the driver's
 *      EmcpalDriverEntry routine.  None of the fields of this structure
 *      should be accessed directly by PAL drivers.  Drivers can access
 *      information specific to their driver using the EmcpalDriver*
 *      interface functions.
 *
 */
typedef struct _EMCPAL_DRIVER
{
    PEMCPAL_NATIVE_DRIVER_OBJECT    pNativeDriverObject; /*!< Driver object passed in from native driver entry-point */

    PEMCPAL_NATIVE_REGISTRY_PATH    pNativeRegistryPath; /*!< Registry path passed in from native driver entry-point */

    EMCPAL_CLIENT                   palClient; /*!< Client object used to register a particular PAL-enabled driver with the PAL. */
                                                         
    EMCPAL_DRIVER_UNLOAD_CALLBACK   DriverUnloadCallback; /*!< Optional unload routine that is called when the driver is unloaded. If an unload routine is desired this field must be populated in the user's EmcpalDriverEntry function. */

} EMCPAL_DRIVER, *PEMCPAL_DRIVER;	/*!< ptr to EMCPAL_DRIVER */
//.End


//++
// Type:
//      EMCPAL_IRP_HANDLER
//
// Description:
//      CX_PREP_TODO
//
//--
/*! @brief EmcPal IRP handler */
typedef EMCPAL_STATUS (*EMCPAL_IRP_HANDLER)(PEMCPAL_DEVICE_OBJECT, PEMCPAL_IRP);
//.End
 

//++
// Type:
//      EMCPAL_IRP_HANDLER_SET
//
// Description:
//      CX_PREP_TODO
//
//--
/*! @brief EmcPAL IRP handler set
 */
typedef struct
{
    EMCPAL_IRP_HANDLER OpenHandler;						/*!< Open handler */
    EMCPAL_IRP_HANDLER CloseHandler;					/*!< Close handler */
    EMCPAL_IRP_HANDLER ReadHandler;						/*!< Read handler */
    EMCPAL_IRP_HANDLER WriteHandler;					/*!< Write handler */
    EMCPAL_IRP_HANDLER IoctlHandler;					/*!< Ioctl handler */
    EMCPAL_IRP_HANDLER InternalHandler;					/*!< Internal handler */
} EMCPAL_IRP_HANDLER_SET, *PEMCPAL_IRP_HANDLER_SET;		/*!< ptr to EmcPAL IRP handler set */
//.End 

#if defined(EMCPAL_PAL_BUILD) || defined(EMCPAL_MODULE_LINKAGE) || defined(EMCPAL_OPT_DRIVERSHELL_INLINED)
//++
//Variable:
//     EmcpalCurrentDriver
//
// Description:
//      Pointer to the driver object for the current driver.  This
//      can be accessed by a PAL-enabled driver via EmcpalGetCurrentDriver.
//
// Revision History:
//      12/2008   CGould Created.
//
//--

extern EMCPAL_DRIVER EmcpalCurrentDriver;	/*!< Current driver */
//.End EmcpalCurrentDriver
#endif


//**********************************************************************
//
//  IMPORTED INTERFACE FUNCTION PROTOTYPES 
//  (user-defined functions called by the PAL Driver Shell)
//
//**********************************************************************

//++
// Function:
//      EmcpalDriverEntry()
//
// Description:
//      This is the driver entry-point that must be defined by
//      all PAL-enabled drivers.  The PAL driver infrastructure
//      will call the user's EmcpalDriverEntry function when the
//      driver is loaded.  The EmcpalDriverEntry routine should
//      perform any initialization necessary for the driver and
//      set the driver unload callback if one is desired.  If an
//      error status is returned then the PAL will reject the driver
//      load.
//
// Arguments:
//      pDriverObject           PAL driver object
//
// Return Value:
//      STATUS_SUCCESS:         the driver successfully completed initialization
//      otherwise error code bubbled up
//       
// Revision History:
//      12/08  CGould Created.
//
//--
/*! @brief EmcpalDriverEntry
 *         This is the driver entry-point that must be defined by
 *         all PAL-enabled drivers.  The PAL driver infrastructure
 *         will call the user's EmcpalDriverEntry function when the
 *         driver is loaded.  The EmcpalDriverEntry routine should
 *         perform any initialization necessary for the driver and
 *         set the driver unload callback if one is desired.  If an
 *         error status is returned then the PAL will reject the driver load.
 *  @param pDriverObject PAL driver object
 *  @return STATUS_SUCCESS: the driver successfully completed initialization, 
 *      otherwise error code bubbled up
 */
EMCPAL_GLOBAL
EMCPAL_STATUS EmcpalDriverEntry(
    PEMCPAL_DRIVER pDriverObject);
//.End EmcpalDriverEntry


//**********************************************************************
//
//  EXPORTED INTERFACE FUNCTION PROTOTYPES 
//
//**********************************************************************
/*!
 * @addtogroup emcpal_driver_utilities
 * @{
 */
#if !defined(EMCPAL_PAL_BUILD) && !defined(EMCPAL_MODULE_LINKAGE) && defined(EMCPAL_OPT_DRIVERSHELL_LIBRARY)

//++
// Function:
//      EmcpalDriverGetUnloadCallback()
//
// Description:
//      This function returns a pointer to the unload callback handler
//      for the given driver.  
//
// Arguments:
//      pDriverObject       pointer to PAL driver object
//
// Return Value:
//      pointer to unload callback or NULL if none is set
//       
// Revision History:
//      12/08  CGould Created.
//
//--
EMCPAL_GLOBAL
EMCPAL_DRIVER_UNLOAD_CALLBACK
EmcpalDriverGetUnloadCallback(
    PEMCPAL_DRIVER pDriverObject);

//.End EmcpalDriverGetUnloadCallback

//++
// Function:
//      EmcpalDriverSetUnloadCallback()
//
// Description:
//      This function allows a PAL driver to set a callback
//      that is issued when the driver is unloaded.  If an
//      unsuccessful status is returned from the callback then
//      the PAL will prevent the driver from unloading if possible.
//      Subsequent attempts to unload the driver will result in
//      further invocations of the driver unload callback handler.
//      Currently this behavior is only supported on platforms other
//      than Windows.  This routine should only be called from
//      a driver's EmcpalDriverEntry function.  The one exception to
//      this is when a driver wants to clear its unload callback handler,
//      by passing in a NULL pointer as the callback argument.  A driver
//      can clear its unload callback at any time.
//
// Arguments:
//      pDriverObject       pointer to PAL driver object
//      callback            unload callback handler
//
// Return Value:
//      None
//       
// Revision History:
//      12/08  CGould Created.
//
//--
EMCPAL_GLOBAL
VOID
EmcpalDriverSetUnloadCallback(
    PEMCPAL_DRIVER					pDriverObject,
    EMCPAL_DRIVER_UNLOAD_CALLBACK 	callback);

//.End EmcpalDriverSetUnloadCallback

//++
// Function:
//      EmcpalDriverGetDriverName()
//
// Description:
//      This function returns a pointer to the driver name
//      for the given PAL driver.  
//
// Arguments:
//      pDriverObject       pointer to PAL driver object
//
// Return Value:
//      driver name string
//       
// Revision History:
//      12/08  CGould Created.
//
//--
EMCPAL_GLOBAL
TEXT *
EmcpalDriverGetDriverName(
    PEMCPAL_DRIVER pDriverObject);


//++
// Inline Function:
//      EmcpalGetCurrentDriver()
//
// Description:
//      This function returns a pointer to the PAL driver object associated
//      with the calling driver.
//
// Arguments:
//      None
//
// Return Value:
//      Pointer to PAL driver object
// 
// Revision History:
//      12/08  CGould Created.
//
//--
EMCPAL_GLOBAL
PEMCPAL_DRIVER
EmcpalGetCurrentDriver(
    VOID);


//++
// Function:
//      EmcpalDriverGetRegistryPath()
//
// Description:
//      This function returns a pointer to the registry path
//      for the given PAL driver.  This is registry path that
//      contains the keys for a PAL-enabled driver.  This is
//      the path that a PAL client will typically provide when
//      calling the PAL registry APIs.
//
// Arguments:
//      pDriverObject       pointer to PAL driver object
//
// Return Value:
//      registry path string
//       
// Revision History:
//      12/08  CGould Created.
//
//--
EMCPAL_GLOBAL
TEXT *
EmcpalDriverGetRegistryPath(
    PEMCPAL_DRIVER   pDriverObject);

//.End EmcpalDriverGetRegistryPath

//++
// Function:
//      EmcpalDriverSetRegistryPath()
//
// Description:
//      This function sets the registry path for the given PAL driver.
//      This is registry path that contains the keys for a PAL-enabled
//      driver.  This is the path that a PAL client will typically 
//      provide when calling the PAL registry APIs.
//
// Arguments:
//      pDriverObject       pointer to PAL driver object
//
// Return Value:
//      registry path string
//       
// Revision History:
//      12/11  Manjit Singh Created.
//
//--
EMCPAL_GLOBAL
VOID
EmcpalDriverSetRegistryPath(
    PEMCPAL_DRIVER   pDriverObject,
    TEXT* registrypath);

//.End EmcpalDriverSetRegistryPath

//++
// Function:
//      EmcpalDriverGetClientObject()
//
// Description:
//      This function returns a pointer to the PAL client object for the
//      given PAL driver.
//
// Arguments:
//      pDriverObject       pointer to PAL driver object
//
// Return Value:
//      pointer to PAL client
//       
// Revision History:
//      12/08  CGould Created.
//
//--
EMCPAL_GLOBAL
PEMCPAL_CLIENT
EmcpalDriverGetClientObject(
    PEMCPAL_DRIVER   pDriverObject);

//.End EmcpalDriverGetClientObject



//++
// Function:
//      EmcpalDriverGetCurrentClientObject()
//
// Description:
//      This function returns a pointer to the PAL client object for the
//      current PAL driver.
//
// Arguments:
//      None
//
// Return Value:
//      pointer to PAL client
//       
// Revision History:
//      3/09  CGould Created.
//
//--
EMCPAL_GLOBAL
PEMCPAL_CLIENT
EmcpalDriverGetCurrentClientObject(VOID);


//.End EmcpalDriverGetCurrentClientObject


//++
// Function:
//      EmcpalDriverGetNativeDriverObject()
//
// Description:
//      This function returns a pointer to the native driver object
//      associated with a given PAL driver.  The native driver object
//      is the NT driver in the CLARiiON case, and a CSX driver
//      in the SSPG case.
//
// Arguments:
//      pDriverObject       pointer to PAL driver object
//
// Return Value:
//      pointer to native driver object
//       
// Revision History:
//      12/08  CGould Created.
//
//--
EMCPAL_GLOBAL
PEMCPAL_NATIVE_DRIVER_OBJECT
EmcpalDriverGetNativeDriverObject(
    PEMCPAL_DRIVER  pDriverObject);


//.End EmcpalDriverGetNativeDriverObject


//++
// Function:
//      EmcpalDriverGetNativeRegistryPath()
//
// Description:
//      This function returns a pointer to the native registry path 
//      associated with a given PAL driver.  The native registry path
//      is a copy to the NT driver's unicode registry path in the CLARiiON
//      case, and is unused in the SSPG case.
//
// Arguments:
//      pDriverObject       pointer to PAL driver object
//
// Return Value:
//      pointer to native registry path
//       
// Revision History:
//      3/09  CGould Created.
//
//--
EMCPAL_GLOBAL
PEMCPAL_NATIVE_REGISTRY_PATH
EmcpalDriverGetNativeRegistryPath(
    PEMCPAL_DRIVER  pDriverObject);

//.End EmcpalDriverGetNativeRegistryPath

#endif

//**********************************************************************
//
//  INLINE INTERFACE FUNCTION DEFINITIONS
//
//**********************************************************************
#if defined(EMCPAL_PAL_BUILD) || defined(EMCPAL_MODULE_LINKAGE) || defined(EMCPAL_OPT_DRIVERSHELL_INLINED)

//++
// Function:
//      EmcpalDriverGetUnloadCallback()
//
// Description:
//      This function returns a pointer to the unload callback handler
//      for the given driver.  
//
// Arguments:
//      pDriverObject       pointer to PAL driver object
//
// Return Value:
//      pointer to unload callback or NULL if none is set
//       
// Revision History:
//      12/08  CGould Created.
//
//--
/*! @brief EmcpalDriverGetUnloadCallback - This function returns a pointer to the unload callback handler
 *         for the given driver. 
 *  @param pDriverObject pointer to PAL driver object
 *  @return Pointer to unload callback or NULL if none is set
 */
EMCPAL_LOCAL EMCPAL_INLINE
EMCPAL_DRIVER_UNLOAD_CALLBACK
EmcpalDriverGetUnloadCallback(
    PEMCPAL_DRIVER pDriverObject)
{
   return pDriverObject->DriverUnloadCallback;
}
//.End EmcpalDriverGetUnloadCallback

//++
// Function:
//      EmcpalDriverSetUnloadCallback()
//
// Description:
//      This function allows a PAL driver to set a callback
//      that is issued when the driver is unloaded.  If an
//      unsuccessful status is returned from the callback then
//      the PAL will prevent the driver from unloading if possible.
//      Subsequent attempts to unload the driver will result in
//      further invocations of the driver unload callback handler.
//      Currently this behavior is only supported on platforms other
//      than Windows.  This routine should only be called from
//      a driver's EmcpalDriverEntry function.  The one exception to
//      this is when a driver wants to clear its unload callback handler,
//      by passing in a NULL pointer as the callback argument.  A driver
//      can clear its unload callback at any time.
//
// Arguments:
//      pDriverObject       pointer to PAL driver object
//      callback            unload callback handler
//
// Return Value:
//      None
//       
// Revision History:
//      12/08  CGould Created.
//
//--
/*! @brief EmcpalDriverSetUnloadCallback - This function allows a PAL driver to set a callback
 *      that is issued when the driver is unloaded.  If an
 *      unsuccessful status is returned from the callback then
 *      the PAL will prevent the driver from unloading if possible.
 *      Subsequent attempts to unload the driver will result in
 *      further invocations of the driver unload callback handler.
 *      Currently this behavior is only supported on platforms other
 *      than Windows.  This routine should only be called from
 *      a driver's EmcpalDriverEntry function.  The one exception to
 *      this is when a driver wants to clear its unload callback handler,
 *      by passing in a NULL pointer as the callback argument.  A driver
 *      can clear its unload callback at any time.
 *  @param pDriverObject Pointer to PAL driver object
 *  @param  callback Unload callback handler
 *  @return none
 */
EMCPAL_LOCAL EMCPAL_INLINE
VOID
EmcpalDriverSetUnloadCallback(
    PEMCPAL_DRIVER					pDriverObject,
    EMCPAL_DRIVER_UNLOAD_CALLBACK 	callback)
{
   pDriverObject->DriverUnloadCallback = callback;
}
//.End EmcpalDriverSetUnloadCallback


//++
// Function:
//      EmcpalDriverGetDriverName()
//
// Description:
//      This function returns a pointer to the driver name
//      for the given PAL driver.  
//
// Arguments:
//      pDriverObject       pointer to PAL driver object
//
// Return Value:
//      driver name string
//       
// Revision History:
//      12/08  CGould Created.
//
//--
/*! @brief EmcpalDriverGetDriverName - This function returns a pointer to the driver name
 *      for the given PAL driver.  
 *  @param pDriverObject Pointer to PAL driver object
 *  @return Driver name string
 */
EMCPAL_LOCAL EMCPAL_INLINE
TEXT *
EmcpalDriverGetDriverName(
    PEMCPAL_DRIVER pDriverObject)
{
    return EmcpalClientGetName(&pDriverObject->palClient);
}
// Inline Function:
//      EmcpalGetCurrentDriver()
//
// Description:
//      This function returns a pointer to the PAL driver object associated
//      with the calling driver.
//
// Arguments:
//      None
//
// Return Value:
//      Pointer to PAL driver object
// 
// Revision History:
//      12/08  CGould Created.
//
//--
/*! @brief EmcpalGetCurrentDriver - This function returns a pointer to the PAL driver object associated
 *      with the calling driver.
 *  @return Pointer to PAL driver object
 */
EMCPAL_LOCAL EMCPAL_INLINE
PEMCPAL_DRIVER
EmcpalGetCurrentDriver(
    VOID)
{
    return &EmcpalCurrentDriver;
}

//++
// Function:
//      EmcpalDriverGetRegistryPath()
//
// Description:
//      This function returns a pointer to the registry path
//      for the given PAL driver.  This is registry path that
//      contains the keys for a PAL-enabled driver.  This is
//      the path that a PAL client will typically provide when
//      calling the PAL registry APIs.
//
// Arguments:
//      pDriverObject       pointer to PAL driver object
//
// Return Value:
//      registry path string
//       
// Revision History:
//      12/08  CGould Created.
//
//--
/*! @brief EmcpalDriverGetRegistryPath - This function returns a pointer to the registry path
 *      for the given PAL driver.  This is registry path that
 *      contains the keys for a PAL-enabled driver.  This is
 *      the path that a PAL client will typically provide when
 *      calling the PAL registry APIs.
 *  @param pDriverObject Pointer to PAL driver object
 *  @return Registry path string
 */
EMCPAL_LOCAL EMCPAL_INLINE
TEXT *
EmcpalDriverGetRegistryPath(
    PEMCPAL_DRIVER   pDriverObject)
{
    return EmcpalClientGetRegistryPath(&pDriverObject->palClient);
}
//.End EmcpalDriverGetRegistryPath


//++
// Function:
//      EmcpalDriverGetClientObject()
//
// Description:
//      This function returns a pointer to the PAL client object for the
//      given PAL driver.
//
// Arguments:
//      pDriverObject       pointer to PAL driver object
//
// Return Value:
//      pointer to PAL client
//       
// Revision History:
//      12/08  CGould Created.
//
//--
/*! @brief EmcpalDriverGetClientObject - This function returns a pointer to the PAL client object for the
 *      given PAL driver.
 *  @param pDriverObject Pointer to PAL driver object
 *  @return Pointer to PAL client
 */
EMCPAL_LOCAL EMCPAL_INLINE
PEMCPAL_CLIENT
EmcpalDriverGetClientObject(
    PEMCPAL_DRIVER   pDriverObject)
{
    return &pDriverObject->palClient;
}
//.End EmcpalDriverGetClientObject



//++
// Function:
//      EmcpalDriverGetCurrentClientObject()
//
// Description:
//      This function returns a pointer to the PAL client object for the
//      current PAL driver.
//
// Arguments:
//      None
//
// Return Value:
//      pointer to PAL client
//       
// Revision History:
//      3/09  CGould Created.
//
//--
/*! @brief EmcpalDriverGetCurrentClientObject - This function returns a pointer to the PAL client object for the
 *      current PAL driver.
 *  @return Pointer to PAL client
 */
EMCPAL_LOCAL EMCPAL_INLINE
PEMCPAL_CLIENT
EmcpalDriverGetCurrentClientObject(VOID)
{
    PEMCPAL_DRIVER pDriverObject = EmcpalGetCurrentDriver();
    return &pDriverObject->palClient;
}

//.End EmcpalDriverGetCurrentClientObject


//++
// Function:
//      EmcpalDriverGetNativeDriverObject()
//
// Description:
//      This function returns a pointer to the native driver object
//      associated with a given PAL driver.  The native driver object
//      is the NT driver in the CLARiiON case, and a CSX driver
//      in the SSPG case.
//
// Arguments:
//      pDriverObject       pointer to PAL driver object
//
// Return Value:
//      pointer to native driver object
//       
// Revision History:
//      12/08  CGould Created.
//
//--
/*! @brief EmcpalDriverGetNativeDriverObject - This function returns a pointer to the native driver object
 *      associated with a given PAL driver.  The native driver object
 *      is the NT driver in the CLARiiON case, and a CSX driver
 *      in the SSPG case.
 *  @param pDriverObject Pointer to PAL driver object
 *  @return Pointer to native driver object
 */
EMCPAL_LOCAL EMCPAL_INLINE
PEMCPAL_NATIVE_DRIVER_OBJECT
EmcpalDriverGetNativeDriverObject(
    PEMCPAL_DRIVER  pDriverObject)
{
	return pDriverObject->pNativeDriverObject;
}

//.End EmcpalDriverGetNativeDriverObject


//++
// Function:
//      EmcpalDriverGetNativeRegistryPath()
//
// Description:
//      This function returns a pointer to the native registry path 
//      associated with a given PAL driver.  The native registry path
//      is a copy to the NT driver's unicode registry path in the CLARiiON
//      case, and is unused in the SSPG case.
//
// Arguments:
//      pDriverObject       pointer to PAL driver object
//
// Return Value:
//      pointer to native registry path
//       
// Revision History:
//      3/09  CGould Created.
//
//--
/*! @brief EmcpalDriverGetNativeRegistryPath - This function returns a pointer to the native registry path 
 *      associated with a given PAL driver.  The native registry path
 *      is a copy to the NT driver's unicode registry path in the CLARiiON
 *      case, and is unused in the SSPG case.
 *  @param pDriverObject Pointer to PAL driver object
 *  @return Pointer to native registry path
 */
EMCPAL_LOCAL EMCPAL_INLINE
PEMCPAL_NATIVE_REGISTRY_PATH
EmcpalDriverGetNativeRegistryPath(
    PEMCPAL_DRIVER  pDriverObject)
{
    return pDriverObject->pNativeRegistryPath;
}
//.End EmcpalDriverGetNativeRegistryPath

#endif

/* @} End driver utilities interfaces */

//**********************************************************************
//
//  INLINE INTERFACE FUNCTION DEFINITIONS
//
//**********************************************************************


#ifndef EMCPAL_USE_CSX_DCALL
/*! @brief EmcPAL int device extension */
typedef struct
{
    PWSTR linkName;			/*!< Pointer to link name (Unicode) */
    PVOID pad;				/*!<  PAD so device extensions are 16 byte aligned on x64 to */ 
} EMCPAL_INT_DEVICE_EXTENSION, *PEMCPAL_INT_DEVICE_EXTENSION;	/*!< Pointer to  EmcPAL int device extension */
#endif

/*! @brief Device IO type
 *  @remark CSX always uses buffered I/O between containers.
 *          Otherwise all buffers are assumed to be accessible.
 */
typedef enum
{
    EMCPAL_DEVICE_IO_TYPE_BUFFERED,
    EMCPAL_DEVICE_IO_TYPE_DIRECT,
    EMCPAL_DEVICE_IO_TYPE_NEITHER
} EMCPAL_DEVICE_IO_TYPE;


#define EMCPAL_DEVICE_FLAG_EXCLUSIVE 0x1	/*!< EmcPAL exclusive device flag */

/*! @brief EmcPAL device flags */
typedef ULONG EMCPAL_DEVICE_FLAGS;

/*! @brief EmcPAL device walk context descriptor */
typedef struct {
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_devices_find_context_t csx_devices_find_context;
#else
    struct _DEVICE_OBJECT              *pNextDeviceObject;	/*!< Ptr to next device object */
#endif
} EMCPAL_DEVICE_WALK_CONTEXT, *PEMCPAL_DEVICE_WALK_CONTEXT;	/*!< Pointer to EmcPAL device walk context descriptor */

/*! @brief EmcpalDriverBeginDeviceWalk - Driver begin device walk
 *  @param pPalDriver Pointer to driver object
 *  @param pDeviceWalkContext Pointer to device walk context
 *  @return none
 */
EMCPAL_GLOBAL
VOID
EmcpalDriverBeginDeviceWalk(
    PEMCPAL_DRIVER pPalDriver,
    PEMCPAL_DEVICE_WALK_CONTEXT pDeviceWalkContext);

/*! @brief EmcpalDriverSetIrpHandlers - Driver set IRP handlers
 *  @param pDriverObject Pointer to driver object
 *  @param irpHandlers IRP handler set
 *  @return STATUS_SUCCESS if success, appropriate error-code otherwise
 */
EMCPAL_GLOBAL
EMCPAL_STATUS
EmcpalDriverSetIrpHandlers(
    PEMCPAL_DRIVER pDriverObject,
    PEMCPAL_IRP_HANDLER_SET irpHandlers);

/*! @brief EmcpalDriverSetIoHandler - Driver set IO handler
 *  @param pDriverObject Pointer to driver object
 *  @param irp_code IRP code
 *  @param pHandler ptr to handler
 *  @return STATUS_SUCCESS if success, appropriate error-code otherwise
 */
EMCPAL_GLOBAL
EMCPAL_STATUS
EmcpalDriverSetIoHandler(
    PEMCPAL_DRIVER pDriverObject,
    char irp_code,
    EMCPAL_IRP_HANDLER pHandler);

/*! @brief EmcpalDriverContinueDeviceWalk - Driver continue device walk
 *  @param pDeviceWalkContext Pointer to device walk context
 *  @return STATUS_SUCCESS if success, appropriate error-code otherwise
 */
EMCPAL_GLOBAL
PEMCPAL_DEVICE_OBJECT
EmcpalDriverContinueDeviceWalk(
    PEMCPAL_DEVICE_WALK_CONTEXT pDeviceWalkContext);

/*! @brief EmcpalDeviceMarkEnabled - Device mark enabled
 *  @param pPalDeviceObject - ptr to device object
 *  @return none
 */
EMCPAL_GLOBAL
VOID
EmcpalDeviceMarkEnabled(
    PEMCPAL_DEVICE_OBJECT pPalDeviceObject);

/*! @brief EmcpalDeviceIsEnabled - Get enabled status
 *  @param pPalDeviceObject - ptr to device object
 *  @return True if device enabled, false otherwise
 */
EMCPAL_GLOBAL
BOOL
EmcpalDeviceIsEnabled(
    PEMCPAL_DEVICE_OBJECT pPalDeviceObject);

/*! @brief EmcpalDeviceCreate - Create new device
 *  @param pPalDriver Pointer to driver object
 *  @param deviceName Optional device name
 *  @param linkName Optional link ame
 *  @param deviceExtensionSize Size of device extension
 *  @param deviceType Device type
 *  @param deviceCharacteristics Device characteristics
 *  @param deviceIoType Device IO type
 *  @param deviceFlags Device flags
 *  @param pPalDeviceObject Pointer to new device object
 *  @return STATUS_SUCCESS if success, appropriate error-code otherwise
 */
EMCPAL_GLOBAL
EMCPAL_STATUS
EmcpalDeviceCreate(
    PEMCPAL_DRIVER pPalDriver,
    TEXT *deviceName,
    TEXT *linkName,
    ULONG deviceExtensionSize,
    ULONG deviceType,
    ULONG deviceCharacteristics,
    EMCPAL_DEVICE_IO_TYPE deviceIoType,
	EMCPAL_DEVICE_FLAGS deviceFlags,
    PEMCPAL_DEVICE_OBJECT *pPalDeviceObject);

/*! @brief EmcpalDeviceUnlink - Unlink the device
 *  @param pPalDeviceObject Pointer to device object
 *  @return STATUS_SUCCESS if success, appropriate error-code otherwise
 */
EMCPAL_GLOBAL
EMCPAL_STATUS
EmcpalDeviceUnlink(
    PEMCPAL_DEVICE_OBJECT pPalDeviceObject);

/*! @brief EmcpalDeviceDelete 0 Delete the device
 *  @param pPalDeviceObject Pointer to device object
 *  @return STATUS_SUCCESS if success, appropriate error-code otherwise
 */
EMCPAL_GLOBAL
EMCPAL_STATUS
EmcpalDeviceDelete(
    PEMCPAL_DEVICE_OBJECT pPalDeviceObject);

#ifdef PAL_FULL_IRP_AVAIL
#include "EmcPAL_DriverShell_Extensions.h"
#endif

#ifdef __cplusplus
}
#endif

// End File EmcPAL_DriverShell.h


#endif /* EMCPAL_DRIVER_SHELL_H */

