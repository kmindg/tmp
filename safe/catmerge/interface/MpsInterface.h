//++
// Copyright (C) Data General Corporation, 1998
// All rights reserved.
// Licensed material -- property of Data General Corporation
//--

//++
// File:            MpsInterface.h
//
// Description:     This file contains data structure definitions used during
//                  MPS operations between clients and MPS.
//
// History:         06-Nov-98       ALT     Created
//                  23-Nov-98       ALT     Changed order of presentation to
//                                          a logical flow
//--

#ifndef __MPS_INTERFACE__
#define __MPS_INTERFACE__

#include "CmiUpperInterface.h"
#include "EmcPAL_List.h"

//
// The definition of the size limitations in characters on mailbox and domain
// names.
//
#define MPS_MAX_MAILBOX_NAME_LENGTH  64

//
// Since MPS exposes a user-mode interface, it must lock out user-mode
// applications from opening a filament in a kernel-mode layered driver's
// name space.  All user-mode applications share the same namespace and
// by allowing this shared name space, user-mode applications cannot
// hijack a filament before a kernel-mode component can open the filament.
// Kernel mode components that require delivery to user mode clients must use
// this domain.
//
#define MPS_USER_MODE_DOMAIN_NAME                   'User'

//
// All user-mode filament operations should be on the same underlying CMI
// conduit.  Place these requests on the default MPS conduit.  Kernel mode
// components that require delivery to user mode clients must use this
// conduit.
//
#define MPS_USER_MODE_CONDUIT                       Cmi_Filament_Conduit

//
// All user-mode filament operations send a synchronous message by using
// the same sequence tag.  This ensures a strict producer/consumer model.
// Kernel mode components that require delivery to user mode clients must
// use this sequence tag to ensure correct delivery.
//
#define MPS_USER_SEQUENCE_TAG                       0xA5A5A5A5

//++
// Type:            MPS_HANDLE
//
// Description:     A unique opaque handle used for filament operations.
//--

typedef PVOID MPS_HANDLE, *PMPS_HANDLE;

//++
// Type:            UMPS_HANDLE
//
// Description:     A unique opaque handle used for user space filament operations.
//                  This is defined as ULONGLONG so that 64 bit MPS can support
//                  32 bit user space clients.
//--

typedef ULONGLONG UMPS_HANDLE, *PUMPS_HANDLE;
#define NULL_UMPS_HANDLE 0


//++
// Type:            MPS_CONTEXT
//
// Description:     A unique value to identify received message.
//                  This is defined as ULONGLONG so that 64 bit MPS can support
//                  32 bit user space clients.
//--

typedef ULONGLONG MPS_CONTEXT, *PMPS_CONTEXT;
#define NULL_MPS_CONTEXT 0

//++
// Type:            MPS_SEND_MESSAGE_INFO
//
// Description:     The data supplied by the client to MPS that indicates
//                  the properties of a transmission request.  The delivery
//                  information should be non-paged memory.  After a request,
//                  the client can release the memory for this structure itself.
//                  There is no need to wait for transmission confirmation.
//
// Members:         ContextHandle - An opaque pointer belonging to the client.
//                      The handle is not part of the sent message. Use is
//                      optional.
//                  Destination - The intended SP message recipient.  The id
//                      must not be the same as the sending id.  Therefore a
//                      client cannot send a message to itself.
//                  Ptr - The MPS allocated, non-paged message buffer. The
//                      client is responsible for eventually freeing the
//                      memory associated with the message through the MPS
//                      free routine.
//                  Size - The length of the transmission in bytes.  The size
//                      cannot exceed the space allocated through the MPS
//                      allocate routine.
//                  SequenceTag - An id, supplied by the client, that MPS
//                      uses to distinguish intended synchronous receive
//                      recipients.  A value of zero specifies asynchronous
//                      delivery.  A non-zero value indicates that the message
//                      should be delivered to a synchronous receive
//                      operation.  The message is queued for delivery if
//                      there are no pending synchronous receive requests.
//--
typedef struct _MPS_SEND_MESSAGE_INFO
{
    PVOID       ContextHandle;
    CMI_SP_ID   Destination;
    PVOID       Ptr;
    ULONG       Size;
    ULONG       SequenceTag;
} MPS_SEND_MESSAGE_INFO, *PMPS_SEND_MESSAGE_INFO;

//++
// Type:            MPS_EVENT
//
// Description:     An enumeration of the events that trigger MPS to execute a
//                  client's asynchronous callback routine.  These events
//                  indicate a specific action on a given filament.  Each
//                  event is accompanied by associated data.  It is the
//                  client's responsibility to take appropriate action for
//                  each event using the supplied data.
//
// Members:         MpsEventCloseCompleted - An asynchronous event that
//                      indicates a filament has closed.  MPS supplies a
//                      MPS_CLOSE_EVENT_DATA data structure to the client when
//                      this event is triggered.
//                  MpsEventMessageReceived - An asynchronous event that
//                      indicates an asynchronous message has been received.
//                      MPS supplies a MPS_RECEIVED_EVENT_DATA data structure
//                      to the client when this event is triggered.
//                  MpsEventMessageSent - An asynchronous event that indicates
//                      a message was successfully sent.  MPS supplies a
//                      MPS_SEND_EVENT_DATA data structure to the client when
//                      this event is triggered.
//                  MpsEventSpContactLost - Contact with a SP has been reported
//                      by CMI to be lost.
//                  MpsEventInvalid - The enumeration terminator.
//--
typedef enum _MPS_EVENT
{
    MpsEventCloseCompleted  = 0,
    MpsEventMessageReceived = 1,
    MpsEventMessageSent     = 2,
    MpsEventSpContactLost   = 3,
    MpsEventInvalid         = 4
} MPS_EVENT, *PMPS_EVENT;

//++
// Type:            MPS_CLOSE_EVENT_DATA
//
// Description:     The data supplied asynchronously to a client when a
//                  filament closes.  The data accompanies a
//                  "MpsEventCloseCompleted" event.  Clients will receive
//                  asynchronous notification of closure only if there are
//                  outstanding transmissions or synchronous receive requests.
//                  If there is no activity on a filament, it will close
//                  immediately.
//
// Members:         FilamentHandle - The unique opaque handle that represents
//                      the closed filament.
//                  CloseStatus - The status of the closed filament.
//                      STATUS_SUCCESS - The conduit closed successfully.
//--
typedef struct _MPS_CLOSE_EVENT_DATA
{
    MPS_HANDLE      FilamentHandle;
    EMCPAL_STATUS        CloseStatus;
} MPS_CLOSE_EVENT_DATA, *PMPS_CLOSE_EVENT_DATA;

//++
// Type:            MPS_RECEIVED_EVENT_DATA
//
// Description:     The data supplied to a client when a message is received.
//                  The data can be supplied to the client in two ways.
//                  First, MPS can supply this data through an asynchronous
//                  callback.  The data, an asynchronous message, accompanies a
//                  "MpsEventMessageReceived"  event.  Second, the client can
//                  post a synchronous receive operation and wait for the
//                  available synchronous message.
//
//                  In the case of an asynchronous callback:
//                  1) MPS can implicitly release the received message if the
//                      client returns STATUS_SUCCESS from the callback routine.
//                  2) The client can defer releasing the received message for
//                      sometime in the future by setting
//                      "IsDelayedReleaseRequested" to TRUE.
//                  3) The client can explicitly release the received message
//                      in the callback routine by returning STATUS_FILE_DELETED
//                      from the callback routine.
//
// Members:         Ptr - A pointer to the message.  Once the received
//                      message has been released, the message pointer is
//                      invalid.
//                  Sender - The originating SP of the message.
//                  Size - The length of the transmission in bytes.
//                  IsDelayedReleaseRequested - This field is writeable by the
//                      client.  If the client is going to delay releasing the
//                      received message, it should set this field to TRUE.
//                      Otherwise, this field should remain set to FALSE.
//                      This field is not used in a synchronous receive
//                      operation because there is no context for MPS to read
//                      the value set by the client.
//--
typedef struct _MPS_RECEIVED_EVENT_DATA
{
    PVOID           Ptr;
    CMI_SP_ID       Sender;
    ULONG           Size;
    BOOLEAN         IsDelayedReleaseRequested;
} MPS_RECEIVED_EVENT_DATA, *PMPS_RECEIVED_EVENT_DATA;

//++
// Type:            MPS_SEND_EVENT_DATA
//
// Description:     The data supplied asynchronously to a client for
//                  confirmation of transmission delivery.  The data
//                  accompanies a "MpsEventMessageSent" event.
//
//                  Synchronous send operations do not receive confirmation.
//                  The synchronous send routine returns a status variable that
//                  indicates the condition of delivery.
//
// Members:         ContextHandle - An opaque pointer belonging to the client.
//                      The handle is not part of the message.  Use is
//                      optional.
//                  Status - The condition of delivery.
//                      STATUS_SUCCESS - The message was delivered successfully.
//                      STATUS_OBJECT_NAME_NOT_FOUND - The mailbox name was not
//                          registered on the destination SP.
//                      STATUS_INSUFFICIENT_RESOURCES - Not enough resources exist
//                          on the destination SP to deliver the message to a
//                          synchronous receive operation.  This status is can not
//                          be returned for a message delivered asynchronously.
//                      From CMI:
//                          STATUS_PORT_DISCONNECTED - No connection to the
//                              specified destination SP was found.
//                          STATUS_UNSUCCESSFUL:  the destination SP was too
//                              busy or had too few free resources to allow it
//                              to accept the message.
//--
typedef struct _MPS_SEND_EVENT_DATA
{
    PVOID           ContextHandle;
    EMCPAL_STATUS        Status;
} MPS_SEND_EVENT_DATA, *PMPS_SEND_EVENT_DATA;

//++
// Type:            MPS_SP_CONTACT_LOST_EVENT_DATA
//
// Description:     This is the data that is passed to a filament owner when
//                  MPS is informed by CMI that contact with a remote SP has
//                  been lost.
//
// Members:         pFilament - A pointer to the filament associated with the
//                      event.
//                  RemoteSp - The address of the remote SP for which contact
//                      has been lost.
//--
typedef struct _MPS_SP_CONTACT_LOST_EVENT_DATA
{
    PMPS_HANDLE     pFilament;
    CMI_SP_ID       RemoteSp;
} MPS_SP_CONTACT_LOST_EVENT_DATA, *PMPS_SP_CONTACT_LOST_EVENT_DATA;

//++
//Type:             MPS_CANCEL_SYNCHRONOUS_RECEIVE
//
//Description: 
//
//Members:      ClientList -  Used to group the Cancel Sync Receive Requests in a Linked List
//                            This field is set by the client
//              Cancelled -   used to indicate whether to put the synchrnous receive request
//                            in the mps Synchronous receive queue or not. if True, the request
//                            is not put in the queue. This field is private to MPS.it is set
//                            in MpsCancelSynchronousRequest() and is verified in MpsFrontEnd
//                            SynchronousReceive()
//              Filament -    Handle to the mps filament. This field is set by the client
//                            and verified by MPS
//              SequenceTag - used to match valid sync recv requests. This field is set by
//                               Client and read by MPS
//                               
//              SpId -        SpId of the destination also used in matching valid sync recv requests
//                            This field is set by Client and verified by MPS
//
//

typedef struct _MPS_CANCEL_SYNCHRONOUS_RECEIVE
{
    EMCPAL_LIST_ENTRY      ClientList;
    BOOLEAN         Cancelled;
    MPS_HANDLE      Filament;
    ULONG           SequenceTag;
    CMI_SP_ID       SpId;
} MPS_CANCEL_SYNCHRONOUS_RECEIVE, *PMPS_CANCEL_SYNCHRONOUS_RECEIVE;

//++
// Type:            MPS_CALLBACK_ROUTINE
//
// Description:     A routine that MPS will execute when an event occurs on a
//                  filament.
//--
typedef
EMCPAL_STATUS
(* MPS_CALLBACK_ROUTINE)(
    IN      MPS_EVENT                   Event,
    IN      PVOID                       Data,
    IN      PVOID                       Context
    );

#endif // __MPS_INTERFACE__

//
// End of MpsInterface.h
//



