/************************************************************************
 *
 *  Copyright (c) 2007 - 2013, EMC Corporation.
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

#ifndef __CrashCoordinator_h__
#define __CrashCoordinator_h__

//++
// File Name: CrashCoordinator.h
//    
// Contents:
//      Exports the methods of the CrashCoordinator driver.
//
// Revision History:
//      Michal Dobosz - Created.
//--

# include "generics.h"
# include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT
# include "EmcPAL_Irp.h"

/*
 * Define the DLL declaration.
 */

// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL or driver simpler.  All files within this driver are compiled with the
// CRASH_COORDINATOR_EXPORTS symbol defined.  This symbol should not be defined in any
// project that uses this driver.  This way any other project whose source files include
// this file see CRASH_COORDINATOR_DLL_API functions as being imported from a driver,
// whereas this driver sees symbols defined with this macro as being exported.

#if !defined(UMODE_ENV) && !defined(SIMMODE_ENV)
#ifdef  CRASH_COORDINATOR_EXPORTS
#define CRASH_COORDINATOR_DLL_API        CSX_MOD_EXPORT
#else
#define CRASH_COORDINATOR_DLL_API        CSX_MOD_IMPORT
#endif 
#else
#define CRASH_COORDINATOR_DLL_API
#endif

/*
 * Constants.
 */

// These crash dispositions tell the CrashCoordinator driver what to do in the event of an
// impending crash. Registered drivers tell the CrashCoordinator what they think of the
// impending crash based on the NotificationInformation that they receive. Refer to the
// comments of CrashCoordNotifyOfImpendingCrash() for more information.
typedef enum {

    FirstCrashDispositionValue = 0, // Keep this set to the first value.
    NoInformation = 1,
    IrpIsLocal = 2,
    IrpIsWaitingForPeer = 3,
    DoNotCrashNow = 4,
    LastCrashDispositionValue = 5 // Keep this set to the last value.

} CrashDisposition;

/*
 * Data types.
 */

// A NotificationInformation object contains pointers to information that the notifying
// driver wishes to pass all registered drivers.
typedef struct _NotificationInformation
{
    // A pointer to an I/O Request Packet (IRP may be NULL).
    PEMCPAL_IRP      pIRP;
    
    //pIRP_Assoc is IRP subordinated to  pIRP, e.g. a driver receive pIRP,generate pIRP_Assoc to downstream driver
    //each CrashCoordinatorCallback() will optionally set pIRP_Assoc based on pIRP depend on itself nature.
    PEMCPAL_IRP      pIRP_Assoc;

} NotificationInformation, *PNotificationInformation;

// used to control if Current Timestamp - LastCrashAttemp Timestamp fall into this max lagging duration, 
// Crash request will be abode by CC via returning DoNotCrashNow, unit is minutes
# define CRASH_COORDINATOR_MAX_LAGGING_TIME_IN_MINUTES (5) 

// used to start a new epoch to do lagging treatment, unit is minute
# define CRASH_COORDINATOR_LAST_ATTEMP_EPOCH_IN_MINUTES (120) 

/*
 * Function types.
 */

typedef CrashDisposition (*CrashDispositionCallbackRoutine)(PNotificationInformation);

/*
 * Function interfaces.
 */

// Registers a driver to receive crash notifications through callbacks.  NOTE: The
// callback function will be called with IRQL set to DISPATCH_LEVEL.
//
// @param CallbackRoutine - The callback routine to call when a crash is impending.
//
// Returns a unique RegisteredDriver HANDLE, or NULL on failure.
EXTERN_C CRASH_COORDINATOR_DLL_API BOOLEAN 
CrashCoordRegisterForNotification(CrashDispositionCallbackRoutine CallbackRoutine);

// De-registers a driver from receiving crash notifications through callbacks.
//
// @param CallbackRoutine - The callback routine specified when registering.
//
// Returns STATUS_SUCCESS on success, other bad NT Status on failure.
EXTERN_C CRASH_COORDINATOR_DLL_API VOID
CrashCoordDeRegisterFromNotification(CrashDispositionCallbackRoutine CallbackRoutine);

// Notifies all registered drivers that a crash is about to occur. A driver does not need
// to register in order to notify all other registered drivers of an impending crash.
// NOTE: The callback functions will be called with IRQL set at DISPATCH_LEVEL.  
//
// @param CallbackRoutine - The callback routine specified when registering.
//
// @param pNotificationInfo - The NotificationInformation will help the CrashCoordinator
//                            determine if a crash is warranted at this time.
//
// Returns the CrashDisposition:
//      NoInformation - Returned when all of the drivers registered with the
//                      CrashCoordinator did not know anything about the supplied
//                      NotificationInformation. The calling driver may crash.
//      IrpIsLocal - Returned when all of the registered drivers agree that the IRP
//                   supplied in the NotificationInformation is local to this SP. The
//                   calling driver may crash.
//      IrpIsWaitingForPeer - Returned when at least one registered driver knows that the
//                            IRP is waiting for the peer SP. The calling driver should
//                            recall this function if the trigger continues to exist.
//      DoNotCrashNow - Returned when a registered driver needs this SP to stay up. The
//                      calling driver should recall this function if the trigger coninues
//                      to exist.
EXTERN_C CRASH_COORDINATOR_DLL_API CrashDisposition 
CrashCoordNotifyOfImpendingCrash(CrashDispositionCallbackRoutine CallbackRoutine, PNotificationInformation pNotificationInfo);

/*
 * Helper functions.
 */

//
// For users who only want to know whether it's safe to crash. The logic appears backwards,
// but this macro is replacing multiple clients' (CrashDisposition == DoNotCrashNow) checks 
//
// @param _CrashDisposition_ -  the Disposition being queried 
// 
// Returns  TRUE - it's not safe to crash now or FALSE - it's safe to crash now
#define CC_DO_NOT_CRASH_NOW( _CrashDisposition_) \
((DoNotCrashNow == _CrashDisposition_ ) || (IrpIsWaitingForPeer  == _CrashDisposition_ ))

#endif // __CrashCoordinator_h__
