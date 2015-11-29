//++
// Copyright (C) EMC Corporation, 1998, 2009
// All rights reserved.
// Licensed material -- property of EMC Corporation
//--

//++
// File:            Mps.h
//
// Description:     This file contains the prototypes for the MPS interface.  A
//                  potential client (another layered driver or other kernel
//                  component) must include this file for MPS support.  At
//                  this time there is no user space support.
//
//                  The MPS layered driver and MPS kernel mode DLL must be
//                  loaded for proper functionality.
//
//                  Clients must include <ntddk.h> for MPS support.
//
// History:         06-Nov-98       ALT     Created
//                  23-Nov-98       ALT     - Changed order of presentation to
//                                            a logical flow
//                                          - Changed MpsAllocate() and MpsFree()
//                                            to MpsAllocateMessage() and
//                                            MpsFreeMessage()
//                                          - Changed "TransactionId" variable in
//                                            MpsSynchronousReceive() to
//                                            "SequenceTag"
//                                          - Changed the timeout variable type in
//                                            MpsSynchronousReceive() to LONG.
//                  27-May-99       ALT     Added CMI return values.
//--

#ifndef __MPS__
#define __MPS__

#include "MpsInterface.h"
#include "cmm.h"

#ifdef _I_AM_MPS_

// if we are MPS then we want to export the variable,
// otherwise we want to import it.  CSX doesn't currently support
// exporting variables on every platform so only do so if we're using Windows.
// To make this truly portable we're going to need to rework MPS Dll to
// export functions instead of function pointers or try to rework CSX.

#define MPS_EXPORTED_VAR CSX_MOD_VAR_EXPORT

#else /* _I_AM_MPS_ */

#define MPS_EXPORTED_VAR extern CSX_MOD_VAR_IMPORT

#endif /* _I_AM_MPS_ */

//++
// Function:        MpsInitialize()
//
// Description:     This routine initializes the MPS kernel mode DLL.  All MPS
//                  clients must call this routine before calling any other MPS
//                  routine.
//
// IRQL:            IRQL == PASSIVE_LEVEL
//
// Parameters:      NONE
//
// Return Values:   STATUS_SUCCESS - If the initialization succeeded.
//                  STATUS_RETRY - If the initialization failed.
//--
MPS_EXPORTED_VAR
EMCPAL_STATUS
MpsInitialize(
    VOID
    );

//++
// Function:        MpsOpen()
//
// Description:     Open a filament.
//
// IRQL:            IRQL == PASSIVE_LEVEL
//
// Parameters:      CallbackRoutine - A pointer to a client callback routine.
//                      The routine must be capable of executing at
//                      DISPATCH_LEVEL.  All asynchronous filament events are
//                      communicated through the callback routine.  This
//                      routine accepts two parameters.  "Event" is the
//                      description of the action on the filament.  "Data" is
//                      the event specific data.  The callback routine must not
//                      block, and should release the received message buffer
//                      before returning.
//                  Conduit - The transporting CMI conduit.
//                  DomainName - Is a string, delimited by single quote marks,
//                      with up to four characters.  The domain name provides
//                      clients with a mailbox name space.
//                  pMailboxName - A pointer to a wide character string
//                      representation of the mailbox name.  The mailbox
//                      name is a named memory location.  It is the method of
//                      identifying recipients.
//                  Context - An opaque pointer belonging to the client.
//                      The pointer is supplied to the client during execution
//                      of the callback routine.  Use is optional.
//                  pFilamentHandle - A pointer to the unique opaque handle
//                      for filament operations.  The filament handle is
//                      invalid if the open request fails.
//
// Return Values:   STATUS_SUCCESS - The filament opened successfully.
//                  STATUS_INSUFFICIENT_RESOURCES - The request failed due to
//                      unavailable system resources.
//                  STATUS_DUPLICATE_NAME - A filament by that name already
//                      exists.
//--
MPS_EXPORTED_VAR
EMCPAL_STATUS
(*MpsOpen)(
    IN MPS_CALLBACK_ROUTINE         CallbackRoutine,
    IN CMI_CONDUIT_ID               Conduit,
    IN ULONG                        DomainName,
    IN csx_pchar_t                  pMailboxName,
    IN PVOID                        Context,
    OUT PMPS_HANDLE                 pFilamentHandle
    );

//++
// Function:        MpsClose()
//
// Description:     Close a filament.
//
// IRQL:            IRQL == PASSIVE_LEVEL
//
// Parameters:      FilamentHandle - The unique opaque handle for filament
//                      operations.
//
// Return Values:   STATUS_SUCCESS - The filament was closed.  There will be no
//                      asynchronous notification.
//                  STATUS_PENDING - There were transactions outstanding on the
//                      filament.  MPS will notify the client of the closure
//                      asynchronously.  The client should be prepared to
//                      accept the callback by checking the return value.
//--
MPS_EXPORTED_VAR
EMCPAL_STATUS
(*MpsClose)(
    IN MPS_HANDLE                   FilamentHandle
    );

//++
// Function:        MpsAllocateMessage()
//
// Description:     Allocates a client message.  Clients must allocate memory
//                  for a message with this routine.  Clients are responsible
//                  for freeing the memory after confirmation of transmission
//                  with the MPS free routine.  The memory is allocated from
//                  non-paged memory.
//
// IRQL:            IRQL <= DISPATCH_LEVEL
//
// Parameters:      Size - The requested amount of memory in bytes.
//                  Ptr - A pointer to the pointer to the allocated memory.
//
// Return Values:   STATUS_SUCCESS - The memory was allocated successfully.
//                  STATUS_INSUFFICIENT_RESOURCES - The request failed due to
//                      unavailable system resources.
//--
MPS_EXPORTED_VAR
EMCPAL_STATUS
(*MpsAllocateMessage)(
    IN ULONG                        Size,
    OUT PVOID                      *Ptr
    );

//++
// Function:        MpsFreeMessage()
//
// Description:     Free a block of memory previously allocated with the MPS
//                  allocate routine.
//
// IRQL:            IRQL <= DISPATCH_LEVEL
//
// Parameters:      Ptr - A pointer to a non-paged block of memory to free.
//                      The memory must be allocated with the MPS allocate
//                      routine.
//
// Return Values:   STATUS_SUCCESS- The memory was freed successfully.
//--
MPS_EXPORTED_VAR
EMCPAL_STATUS
(*MpsFreeMessage)(
    IN PVOID                        Ptr
    );

//++
// Function:        MpsAllocateCmmMessage()
//
// Description:     Allocate a segment of memory using CMM. This memory should
//                  be used for an MPS transmission.  It is the caller's
//                  responsibility to free the returned memory with the MPS
//                  CMM free operation. MPS maintains transmission information
//                  with the allocated memory.  It is mandatory that a client
//                  use memory allocated from the MPS allocation routine.  The
//                  caller can "allocate" memory of size zero.
//
// IRQL:            IRQL <= DISPATCH_LEVEL
//
// Parameters:      ClientId - The caller's CMM Client ID.
//                  Size - The requested amount of memory in bytes.
//                  Ptr  - A pointer to the pointer to the allocated memory.
//
// Return Values:   STATUS_SUCCESS - The memory was allocated successfully.
//                  STATUS_INSUFFICIENT_RESOURCES - The request failed due to
//                      unavailable system resources.
//--
MPS_EXPORTED_VAR
EMCPAL_STATUS
(*MpsAllocateCmmMessage)(
    IN CMM_CLIENT_ID                ClientId,
    IN ULONG                        Size,
    OUT PVOID                      *Ptr
    );

//++
// Function:        MpsFreeCmmMessage()
//
// Description:     Free a segment of memory using CMM.  The memory must
//                  be previously allocated by the MPS CMM allocation
//                  routine.
//
// IRQL:            IRQL <= DISPATCH_LEVEL
//
// Parameters:      ClientId - The caller's CMM Client ID.
//                  Ptr - A pointer to the memory to free. The memory must
//                        have been allocated with the MPS CMM allocate
//                        routine.
//
// Return Values:   STATUS_SUCCESS - The memory was freed successfully.
//--
MPS_EXPORTED_VAR
EMCPAL_STATUS
(*MpsFreeCmmMessage)(
    IN CMM_CLIENT_ID                ClientId,
    IN PVOID                        Ptr
    );

//++
// Function:        MpsAsynchronousSend()
//
// Description:     Send a message asynchronously.  Confirmation of
//                  transmission is delivered asynchronously.
//
// IRQL:            IRQL <= DISPATCH_LEVEL
//
// Parameters:      FilamentHandle - The unique opaque handle for filament
//                      operations.
//                  pSendMessageInfo - A pointer to the message delivery
//                      information.  The caller must supply the non-paged
//                      memory.  The memory may be freed once the call returns.
//                      pSendMessageInfo->Ptr (the message) must be allocated
//                      with the MPS allocate routine.
//
// Return Values:   STATUS_SUCCESS - The message was successfully queued for
//                      asynchronous delivery.
//                  From CMI:
//                      STATUS_PORT_DISCONNECTED - No connection to the
//                          specified destination SP was found.
//--
MPS_EXPORTED_VAR
EMCPAL_STATUS
(*MpsAsynchronousSend)(
    IN MPS_HANDLE                   FilamentHandle,
    IN PMPS_SEND_MESSAGE_INFO       pSendMessageInfo
    );

//++
// Function:        MpsSynchronousSend()
//
// Description:     Send a message synchronously.  The timeout will be
//                  generated from CMI.
//
// IRQL:            IRQL == PASSIVE_LEVEL
//
// Parameters:      FilamentHandle - The unique opaque handle for filament
//                      operations.
//                  pSendMessageInfo - A pointer to the message delivery
//                      information.  The caller must supply the non-paged
//                      memory.  The memory may be freed once the call returns.
//                      pSendMessageInfo->Ptr (the message) must be allocated
//                      with the MPS allocate routine.
//
// Return Values:   STATUS_SUCCESS - The message was successfully sent.
//                  STATUS_OBJECT_NAME_NOT_FOUND - A filament with the
//                      corresponding mailbox name was not found on the
//                      destination.
//                  STATUS_INSUFFICIENT_RESOURCES - Not enough resources exist
//                      on the destination SP to deliver the message to a
//                      synchronous receive operation.  This status is can not
//                      be returned for a message delivered asynchronously.
//                  From CMI:
//                      STATUS_PORT_DISCONNECTED - No connection to the
//                          specified destination SP was found.
//                      STATUS_UNSUCCESSFUL:  the destination SP was too busy
//                          or had too few free resources to allow it to accept
//                          the message.
//--
MPS_EXPORTED_VAR
EMCPAL_STATUS
(*MpsSynchronousSend)(
    IN MPS_HANDLE                   FilamentHandle,
    IN PMPS_SEND_MESSAGE_INFO       pSendMessageInfo
    );

//++
// Function:        MpsSynchronousReceive()
//
// Description:     Receive a message synchronously.
//
// IRQL:            IRQL <= DISPATCH_LEVEL if the message is waiting on the
//                          synchronous received message queue.
//                  IRQL == PASSIVE_LEVEL if the message has not been
//                          delivered.
//
// Parameters:      FilamentHandle - The unique opaque handle for filament
//                      operations.
//                  Timeout - The number of seconds to wait before the request
//                      terminates. If a negative timeout is specified, the
//                      wait is indefinite.  A timeout of zero "tests" to
//                      see if the message is on the queue.
//                  SequenceTag - A non-zero id that MPS uses to distinguish
//                      intended synchronous receive recipients.  The id is
//                      user generated.
//                  pReceivedEventData - A pointer to the receiving message
//                      data.  The message should be released with the MPS
//                      release received message routine as soon as possible.
//
// Return Values:   STATUS_SUCCESS - The message was received synchronously.
//                  STATUS_NO_SUCH_MEMBER - The client specified a timeout of
//                      zero, and the message was not on the synchronously
//                      received message queue.
//                  STATUS_TIMEOUT - The request timed out without receiving
//                      the message.
//
//--
MPS_EXPORTED_VAR
EMCPAL_STATUS
(*MpsSynchronousReceive)(
    IN MPS_HANDLE                       FilamentHandle,
    IN LONG                             Timeout,
    IN ULONG                            SequenceTag,
    IN PMPS_CANCEL_SYNCHRONOUS_RECEIVE  pCancelSyncRecv,
    OUT PMPS_RECEIVED_EVENT_DATA        *pReceivedEventData
    );

//++
// Function:        MpsReleaseReceivedMessage()
//
// Description:     Release a received message.  A synchronously received
//                  message should be released as soon as possible.  The
//                  limitation is to avoid deadlock.
//
// IRQL:            IRQL <= DISPATCH_LEVEL
//
// Parameters:      Ptr - A pointer to the received message.
//
// Return Values:   STATUS_SUCCESS - The received message was released
//                      successfully.
//--
MPS_EXPORTED_VAR
EMCPAL_STATUS
(*MpsReleaseReceivedMessage)(
    IN PVOID                        Ptr
    );

//++
// Function:        MpsGetSpId()
//
// Description:     This routine provides a mechanism for obtaining
//                  the CMI_SP_ID for both SPs in a K10.
//
// IRQL:            IRQL <= DISPATCH_LEVEL
//
// Parameters:      pLocalCmiSpId - A pointer to the local CMI_SP_ID.
//                  pPeerCmiSpId - A pointer to the peer CMI_SP_ID.
//
// Return Values:   STATUS_SUCCESS - The local and peer CMI_SP_ID were
//                      obtained successfully.
//--
MPS_EXPORTED_VAR
EMCPAL_STATUS
(*MpsGetSpId)(
    IN OUT PCMI_SP_ID               pLocalCmiSpId,
    IN OUT PCMI_SP_ID               pPeerCmiSpId
    );

//++
//Function:         MpsCancelSynchronousReceiveRequest()
//
// Description:    This function is used to Cancel Synchronous Receive messages in the
//                 event of receiving lost contact from sp. This helps in preventing  
//                 indefinite wait if the lost contact is recived in between synchronous
//                 send and synchronous receive
//
// Parameters:     pCancelSyncRecv- pointer to the Cancel Synchronous Receive to Cancel
//
// Return Values:  VOID
//

MPS_EXPORTED_VAR
VOID
(*MpsCancelSynchronousReceiveRequest)(
    IN PMPS_CANCEL_SYNCHRONOUS_RECEIVE  pSyncReceive
    );

#endif // __MPS__

//
// End of Mps.h
//

