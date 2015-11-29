#ifndef    _DLSKERNELINTERFACE_H_
#define    _DLSKERNELINTERFACE_H_

//***************************************************************************
// Copyright (C) Data General Corporation 1989-2000
// All rights reserved.
// Licensed material -- property of Data General Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      DlsKernelInterface.h
//
// Contents:
//
// Functions:
//
//      DLSInitialize()
//      DistributedLockServiceOpenLock()
//      DistributedLockServiceCloseLock()
//      DistributedLockServiceConvertLock()
//      DistributedLockServiceQueryLock() - *** FUNCTION NOT IMPLEMENTED ***
//      DistributedLockServiceForceMailBox()
//      DistributedLockServiceReplaceMailBox()
//      DistributedLockServiceUpRootMailBox()
//      DistributedLockServiceFindHandle()
//      DistributedLockServiceAdminShutdown()
//
// Revision History:
//      02-Jan-03   CVaidya  QueryLock() now returns "not implemented".
//                           See DIMS 81417.
//      03-Jul-01   CVaidya  Added code to find the lock
//                           handle given its name.
//      16-Dec-98   MWagner  Created.
//
//--

//++
// Include files
//--

//++
// Interface
//--

#ifdef __cplusplus
extern "C" {
#endif

#include "k10ntddk.h"

#include "K10DistributedLockServiceMessages.h"

#include "DlsTypes.h"
#include "DlsFlags.h"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

//++
// End Includes
//--
//++
// Function:
//      DLSInitialize()
//
// Description:
//      DLSInitialize() initializes the Distributed Lock
//          Service, using an EmcpalInterlockedExchange to prevent a race
//          condition.
//
// Arguments:
//      NONE.
//
// Return Value:
//      STATUS_SUCCESS:  the  Distributed Lock Service was initialized
//      STATUS_RETRY:    the  Distributed Lock Service was not initialized
//      Others:   the program will not halt
//
// Revision History:
//      11/24/1999   MWagner    Created.
//
//--
EMCPAL_STATUS 
CSX_MOD_EXPORT
DLSInitialize(void);

// .End DLSInitialize

//++
// Function:
//      DistributedLockServiceOpenLock()
//
// Description:
//      DistributedLockServiceOpenLock() creates an association between a
//      lock name and a lock handle. If the DLS can complete the
//      operation without communicating with another SP, this
//      function will return STATUS_SUCCESS, and the lock handle
//      will be set in handle. If the DLS needs to communicate
//      with another SP to complete this request, then this function
//      will return STATUS_PENDING, and the lock handle will be
//      sent back via the callback function.
//
//      Callers must be careful to ensure that a mailbox, if specified, is
//      in a consistent state prior to passing it as an argument.
//
// Arguments:
//      pHandle      if return status == SUCCESS, the lock handle used
//                       for all subsequent operations on the lock
//
//      name             the name of the lock  
//
//      callbackFunction the function to be called when the DLS wants to 
//                       communicate with the client 
//
//      mailbox          the mailbox, associated with this lock
//
//      fastFailDelayInSeconds  
//                        nnn        - number of minutes to wait before
//                                     Fast Failing an SP processing 
//                                     Request for this lock.
//                        DLS_DEFAULT_FAIL_FAST_DELAY  - accept the default
//                        DLS_INVALID_FAIL_FAST_DELAY  - don't participate 
//                                     in Fail Fast
//                          
//
// Return Value:
//      STATUS_SUCCESS:  the  lock has been opened, the lock handle
//                       is in *handle
//      STATUS_PENDING:  the DLS is processing the request, the
//                       lock handle will be returned in an event
//                       in the callbackFunction.
//      Others:   
//
// Revision History:
//      16-Dec-98   MWagner  Created.
//
//--
EMCPAL_STATUS 
CSX_MOD_EXPORT
DistributedLockServiceOpenLock(
                       OUT PDLS_LOCK_HANDLE        pHandle,
                       IN  PDLS_LOCK_NAME          pName,  
                       IN  DLS_LOCK_CALLBACKFUNC   callbackFunction,
                       IN  PVOID                   context,
                       IN  PDLS_LOCK_MAILBOX       pMailBox,
                       IN  LOGICAL                 flags,    
                       IN  ULONG                   failFastDelayInMinutes
                       );

// .End DistributedLockServiceOpenLock


//++
// Function:
//      DistributedLockServiceCloseLock()
//
// Description:
//      DistributedLockServiceCloseLock() destroys an association between a
//      lock name and a lock handle.
//
// Arguments:
//      handle      the Lock Handle returned from DistributedLockServiceOpenLock()   
//      flags            UNUSED   
//
// Return Value:
//      STATUS_SUCCESS:  the  lock was closed
//      STATUS_PENDING:  the callbackFunction will be
//                               called when the lock is closed
//      Others:   
//
// Revision History:
//      16-Dec-98   MWagner  Created.
//
//--
EMCPAL_STATUS
CSX_MOD_EXPORT
DistributedLockServiceCloseLock(
                        IN  DLS_LOCK_HANDLE     handle,
                        IN  LOGICAL             flags    
                        );

// .End DistributedLockServiceCloseLock


//++
// Function:
//      DistributedLockServiceConvertLock()
//
// Description:
//      DistributedLockServiceConvertLock() makes a request to obtain a lock
//      in a particular mode If the DLS can complete the
//      operation without communicating with another SP, this
//      function will return STATUS_SUCCESS. If the DLS needs 
//      to communicate with another SP to complete this request, 
//      then this function will return STATUS_PENDING, and 
//      the callback function will be called with an OpenLock
//      Event.
//
// Arguments:
//      handle      the Lock Handle returned from DistributedLockServiceOpenLock()   
//      mode:            the requested mode
//      flags    :       flags for the convert,
//                                              
//
// Return Value:
//      STATUS_SUCCESS:  the  lock was converted
//      STATUS_PENDING:  the callbackFunction will be
//                       called when the lock is converted
//
// Revision History:
//      16-Dec-98   MWagner  Created.
//
//--
EMCPAL_STATUS
CSX_MOD_EXPORT
DistributedLockServiceConvertLock(
                          IN  DLS_LOCK_HANDLE          handle,
                          IN  DLS_LOCK_MODE            mode,
                          IN  LOGICAL                  flags    
                          );

// .End DistributedLockServiceConvertLock

//++
// Function:
//      DistributedLockServiceQueryLock()
//
// Description:
//      DistributedLockServiceQueryLock() returns the current lock mode
//      in pMode
//      **** NEW NOTE: ****
//      **** THIS FUNCTION IS NOT IMPLEMENTED ****
//
// Arguments:
//      handle      the Lock Handle returned from DistributedLockServiceOpenLock()   
//      mode:            the current mode
//      flags    :       flags for the query
//
// Return Value:
//      STATUS_SUCCESS:  the mode was returned
//      **** NEW NOTE: ****
//      STATUS_NOT_IMPLEMENTED: **** ALWAYS RETURNS THIS STATUS ****
//      Others:
//
// Revision History:
//      02-Jan-03   CVaidya  QueryLock() now returns "not implemented".
//                           See DIMS 81417.
//      16-Dec-98   MWagner  Created.
//
//--
EMCPAL_STATUS
CSX_MOD_EXPORT
DistributedLockServiceQueryLock(
                        IN  DLS_LOCK_HANDLE     handle,
                        OUT PDLS_LOCK_MODE      pMode,
                        IN  LOGICAL             flags    
                        );

// .End DistributedLockServiceQueryLock


//++
// Function:
//      DistributedLockServiceForceMailBox()
//
// Description:
//      DistributedLockServiceForceMailBox() sends the caller's mailbox
//      to the other members of the Cabal. If there are
//      no other members of the Cabal, then this function
//      will return STATUS_SUCCESS. Else it will return
//      STATUS_PENDING, and the callbackFunction will
//      be called with ForceMailBox event when the force
//      has been complete
//
// Arguments:
//      handle      the Lock Handle returned from DistributedLockServiceOpenLock()   
//      flags    :       flags for the force
//                                      
//
// Return Value:
//      STATUS_SUCCESS:  there  were no other members of the Cabal
//      STATUS_PENDING:  the DLS is processing the force, the
//                       calbackFunction will be executed with a
//                       ForceMailBox event when the force completes
//      Others:   
//
// Revision History:
//      16-Dec-98   MWagner  Created.
//
//--
EMCPAL_STATUS
CSX_MOD_EXPORT
DistributedLockServiceForceMailBox(
                           IN  DLS_LOCK_HANDLE          handle,
                           IN  LOGICAL                  flags    
                           );

// .End DistributedLockServiceForceMailBox


//++
// Function:
//      DistributedLockServiceReplaceMailBox()
//
// Description:
//      DistributedLockServiceReplaceMailBox() replaces the current mailbox
//      in the lock with a new mailbox
//
//      Callers must be careful to ensure that the new mailbox is
//      in a consistent state prior to passing it as an argument.
//
// Arguments:
//      handle      the Lock Handle returned from DistributedLockServiceOpenLock()   
//      pNewMailBox      the new mailbox
//      flags            flags for the replace
//                                      
//
// Return Value:
//      STATUS_SUCCESS:  the  mailbox was replaced 
//      Others:   
//
// Revision History:
//      16-Dec-98   MWagner  Created.
//
//--
EMCPAL_STATUS
CSX_MOD_EXPORT
DistributedLockServiceReplaceMailBox(
                             IN  DLS_LOCK_HANDLE     pLockrHandle,
                             IN  PDLS_LOCK_MAILBOX   pNewMailBox,
                             IN  LOGICAL             flags
                             );

// .End DistributedLockServiceReplaceMailBox


//++
// Function:
//      DistributedLockServiceUpRootMailBox()
//
// Description:
//      DistributedLockServiceUpRootMailBox() removes a mailbox from
//      a lock
//
// Arguments:
//      handle      the Lock Handle returned from DistributedLockServiceOpenLock()   
//      flags            flags for the uprooting
//                                          
//
// Return Value:
//      STATUS_SUCCESS:     the  mailbox was uprooted, it may be
//                          safely de-allocated
//      Others:   
//
// Revision History:
//      16-Dec-98   MWagner  Created.
//
//--
EMCPAL_STATUS
CSX_MOD_EXPORT
DistributedLockServiceUpRootMailBox(
                            IN  DLS_LOCK_HANDLE          handle,
                            IN  LOGICAL                  flags    
                            );

// .End DistributedLockServiceUpRootMailBox


//++
// Function:
//      DistributedLockServiceFindHandle()
//
// Description:
//      Find the lock's handle given its name.
//
// Arguments:
//      pHandle          the lock handle discovered
//
//      name             the name of the lock  
//
// Return Value:
//      STATUS_SUCCESS:                  handle found, the lock handle
//                                       is in *handle
//      DLS_WARNING_LOCKNAME_NOT_FOUND:  no matching handle
//      Others:   
//
// Revision History:
//      03-Jul-01   CVaidya  Created.
//
//--
EMCPAL_STATUS 
CSX_MOD_EXPORT
DistributedLockServiceFindHandle(
                       OUT PDLS_LOCK_HANDLE        pHandle,
                       IN  PDLS_LOCK_NAME          pName,
					   IN  LOGICAL                 flags
                       );

// .End DistributedLockServiceFindHandle

//++
// Function:
//      DistributedLockServiceAdminShutdown()
//
// Description:
//      Cancel all the executioner timers.
//
// Arguments:
//      None
//
// Return Value:
//      STATUS_SUCCESS:                  Cancel of all timer
//                                       successful
// Revision History:
//      03-Jul-01   CVaidya  Created.
//
//--
EMCPAL_STATUS 
CSX_MOD_EXPORT
DistributedLockServiceAdminShutdown(void);

// .End DistributedLockServiceAdminShutdown

//++
// Function:
//      DistributedLockServiceGetLocalInstantiation()
//
// Description:
//      Returns the local SP's Cabal Registration instantiation.
//
//      The Local Instantiation can be used to detect "stale" messages from
//      from an rebooted peer SP - messages that were sent from a previous
//      reboot of that SP.
//
//      The logic is a bit wacky, but as long as an SP's peer is
//      up, subsequent boots of that SP will have unique instantiations.
//      For example, if SP A is up, every time SP B reboots, it will
//      have a unique instantiation.  As long as SP B is up, ever time SP A
//      reboots, it will have a unique instantiation.
// 
//      If both SP's go down, the instantiations of both will be reset, but 
//      we don't care about stale messages if both SPs go down.
//
//      This function must be called after the local SP's attempt to join the
//      DLS Cabal has completed. That's not a big deal to most prospective
//      clients - the join Cabal request is completed in DLSInitialize() which
//      happens very early. 
//  
//
// Arguments:
//      pLocalInstantiation -    if the call is successful, this is the local SP's
//                          instantiation, else it is unchanged.
//
//      flags          -    not used
//
// Return Value:
//      STATUS_SUCCESS:                  
//      STATUS_DEVICE_BUSY: the function was called before the SP request to
//                          join the DLS Cabal had completed.
// Revision History:
//      09-Jul-12   CVaidya  Created.
//
//--
EMCPAL_STATUS
CSX_MOD_EXPORT
DistributedLockServiceGetLocalInstantiation(
                               OUT PULONG     pLocalInstantiation,
                               IN  LOGICAL    flags
                               );

// .End DistributedLockServiceGetLocalInstantiation
                               
//++
// Function:
//      DistributedLockServiceGetPeerInstantiation()
//
// Description:
//      Returns the peer SP's Cabal Registration instantiation.
//
//      The Local/Peer Instantiation can be used to detect "stale" messages from
//      from a rebooted peer SP - messages that were sent from a previous
//      reboot of that SP.  See TDX an example implementation.
//
//      The logic is a bit wacky, but as long as an SP's peer is
//      up, subsequent boots of that SP will have unique instantiations.
//      For example, if SP A is up, every time SP B reboots, it will
//      have a unique instantiation.  As long as SP B is up, every time SP A
//      reboots, it will have a unique instantiation.
// 
//      If both SP's go down, the instantiations of both will be reset, but 
//      we don't care about stale messages if both SPs go down.
//
//      This function must be called after the local SP's attempt to join the
//      DLS Cabal has completed. That's not a big deal to most prospective
//      clients - the join Cabal request is completed in DLSInitialize() which
//      happens very early. 
//  
//
// Arguments:
//      pPeerInstantiation -    if the call is successful, this is the peer SP's
//                              instantiation, else it is unchanged.
//
//      flags          -    not used
//
// Return Value:
//      STATUS_SUCCESS:                  
//      STATUS_DEVICE_BUSY: the function was called before the SP request to
//                          join the DLS Cabal had completed.
// Revision History:
//      06-Jun-15   emenhs Created.
//
//--
EMCPAL_STATUS
CSX_MOD_EXPORT
DistributedLockServiceGetPeerInstantiation(
                               OUT PULONG     pPeerInstantiation,
                               IN  LOGICAL    flags
                               );
                               
// .End DistributedLockServiceGetPeerInstantiation

#ifdef __cplusplus
};
#endif

#endif //  _DLSKERNELINTERFACE_H_
