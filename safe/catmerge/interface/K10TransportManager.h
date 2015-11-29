/***************************************************************************
 * Copyright (C) EMC Corporation 2007
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *
 * DESCRIPTION:
 *   This header file contains API definitions for the Generic Transport Manager library
 *
 * NOTES:
 *
 * HISTORY:
 *       *
 ***************************************************************************/

#ifndef  _K10TRANSPORTMANAGER_H_
#define  _K10TRANSPORTMANAGER_H_

//
//  INCLUDE FILES
//
//

//
// K10 Transport Manager Callback Functions
//

//
//Peer callback routine
//
typedef VOID (*K10TM_DELIVERY_CALLBACK_ROUTINE)(
                                               IN ULONG32 PeerMessageId,
                                               IN PVOID   pTMDescriptor,
                                               IN PVOID   MessagePtr,
                                               IN ULONG32 MessageSize
                                                );
//
// Lost contact callback 
//
typedef VOID (*K10TM_LOST_CONTACT_CALLBACK_ROUTINE)(VOID);

//
//Peer Online callback
//
typedef VOID (*K10TM_PEER_ONLINE_CALLBACK_ROUTINE)(VOID);  


//
//Enumerations for Transport Manager Library
//
//
//  Type:
//      K10TM_SPID
//
//  Description:
//      .
//
//  Members:
//      
//

typedef enum
{
    InvalidSPId = -1,
    SPA,
    SPB,
    MaxSPId
} K10TM_SPID, *PK10TM_SPID;





///////////////////////////////////////////////////////////////////////////////////////////
//
//  K10 Transport Manager Library
//
//////////////////////////////////////////////////////////////////////////////////////////

// K10TransportManagerPreInitialize
//
//  Description:
//      This function begins the 3 phase initialization of the Transport Manager Library. 
//      This function will initialize and allocate various resources required for the internal 
//      components of Transport Manger. The routine is synchronous in nature and once this is 
//      successfully completed then the clients of Transport Manager can start registering their 
//      peer transport info with the TM. This function will be called during DriverEntry() and must 
//      be called at passive level. If the clients of the TM start using any of the services after 
//      this routine but before the initialization is complete than they will get a 
//      K10_TRANSPORT_MANAGER_ NOT_READY status back.
//
//  Arguments:
//      
//
//  Returns:
//    
//
EMCPAL_STATUS 
K10TransportManagerPreInitialize(
                                IN PCHAR              ClientName ,
                                IN ULONG32            DriverVersion ,
                                IN ULONG32            ProtocolVersion,
                                IN ULONG32            RequestCMIConduit ,
                                IN ULONG32            ResponseCMIConduit ,
                                IN ULONG32            MpsDomain,
                                IN ULONG32            MaxPeerMessages,
                                PEMCPAL_ALLOC_FUNCTION    MemAllocFunc,
                                PEMCPAL_FREE_FUNCTION        MemFreeFunc,
                                OUT                   PVOID * TMHandle  
                                );

// K10TransportManagerAddRegistration
//
//  Description:
//      This function will be called by the clients of TM to register various PeerMessageIds and 
//      their associated response callback routine with the Transport Manager Library. This routine 
//      can be called multiple times by various components within the client's driver to register 
//      their own TM_ClientMessageIdDeliverList, response handler routine and the lost contact callback 
//      routine if required. The routine is synchronous in nature. This function will be called during 
//      clients DriverEntry ().and must be called at passive level..
//
//  Arguments:
//      
//
//  Returns:
//    
//

EMCPAL_STATUS 
K10TransportManagerAddRegistration (
                                   IN PVOID        TMHandle,
                                   IN PCHAR        PeerMessageList[],
                                   IN K10TM_DELIVERY_CALLBACK_ROUTINE  PeerMessageCallback,
                                   IN K10TM_LOST_CONTACT_CALLBACK_ROUTINE LostContactCallback,
                                   IN K10TM_PEER_ONLINE_CALLBACK_ROUTINE  PeerOnlineCallback,
                                   OUT PVOID * TMRegToken
                                   );

// K10TransportManagerIntialize
//
//  Description:
//      This routine completes the three phase initialization of the Transport Manager Library.  
//      The routine opens the MPS filaments and necessary TM internal data structures are initialized 
//      at this time.  This routine is called on the DriverEntry path and must be executing at 
//       passive level..
//
//  Arguments:
//      
//
//  Returns:
//    
//
EMCPAL_STATUS
K10TransportManagerIntialize (
                             IN PVOID TMHandle
                             );

// K10TransportManagerShutdown
//
//  Description:
//      This function wills un-initialize the various internal components of Transport Manger and 
//      free the allocated resources .This function will be called during Driver Shutdown and when 
//      the driver is unloaded from the SP. The un-initialization routine is synchronous in nature 
//      and once the Transport Manager is un-initialized the clients of Transport Manager can no longer 
//      use any of its services. If the Transport Manager is un-initialized without first initializing
//      it then a success status will be returned back to the client.Additionally in this function 
//      Transport Manager will wait for all its outstanding messages (request /response pairs) to drain 
//      before it starts closing all the MPS filaments. This routine must be called at passive level.
//
//  Arguments:
//      
//
//  Returns:
//    
//

EMCPAL_STATUS
K10TransportManagerShutdown (
                            IN PVOID TMHandle
                            );

// K10TransportManagerGetLocalSPId
//
//  Description:
//      This function will return the local SP Id. As the local SP Id will be populated only during 
//      K10TransportPreInitialize() before any of the clients start using the Transport Manager service , 
//      its value will not change as long as the SP is online. This routine can be called at passive or 
//      dispatch level.
//
//  Arguments:
//      
//
//  Returns:
//    
//

K10TM_SPID
K10TransportManagerGetLocalSPId (void);

// K10TransportManagerGetPeerSPVersion
//
//  Description:
//      This function will return   the Driver Version running on the peer SP. Clients of the 
//      Transport Manager can use this value to decide whether a particular message needs to be send 
//      to the peer during NDU scenarios when SPs are running mismatch driver versions. This routine 
//      can be called at passive or dispatch level.
//
//  Arguments:
//      
//
//  Returns:
//    
//
ULONG32
K10TransportManagerGetPeerSPVersion (void);


// K10TransportManagerAllocateRequestBuffer
//
//  Description:
//      This function will allocate a request MPS buffer on behalf of clients on the local SP and 
//      return the buffer pointer back to the clients. Clients can use this buffer to fill in the 
//      required message contents that will be used in the subsequent K10TransportManagerSend() call. 
//      This routine must be called at passive level.
//
//  Arguments:
//      
//
//  Returns:
//    
//
PVOID
K10TransportManagerAllocateRequestBuffer (
                                         IN  PVOID   pTMToken,
                                         IN  ULONG32 RequestSize 
                                         );

// K10TransportManagerReleaseRequestBuffer
//
//  Description:
//      This function will release the received MPS buffer on the Peer SP .Once the Clients are completely 
//      done with the Messagebuffer passed as part of peer callback routine they should call this function
//      for releasing the MPS buffer before allocating any new MPS buffer in order to avoid ring buffer 
//      deadlock. This routine must be called at passive level.
//
//  Arguments:
//      
//
//  Returns:
//    
//
VOID
K10TransportManagerReleaseRequestBuffer (
                                        IN  PVOID   pK10TMDescriptor
                                        );

                       
// K10TransportManagerAllocateResponseBuffer
//
//  Description:
//      This function will allocate a response MPS buffer on behalf of clients on the peer SP and 
//      return the buffer pointer back to the client. Clients can use this buffer to fill in the 
//      required response that will be use in the subsequent K10TransportManagerResponse () call.
//      This routine must be called at passive level.
//
//  Arguments:
//      
//
//  Returns:
//    
//
PVOID
K10TransportManagerAllocateResponseBuffer (
                                         IN  PVOID   pTMToken,
                                         IN  ULONG32 ResponseSize 
                                         ) ;


// K10TransportManagerReleaseResponseBuffer
//
//  Description:
//      This function will release the response MPS buffer on the local SP .Once the Clients are 
//      completely done with the response buffer returned as part K10TransportManagerSend() then 
//      they should call this function for releasing the MPS buffer before allocating any new MPS 
//      buffer in order to avoid ring buffer deadlock. This routine must be called at passive level.
//
//  Arguments:
//      
//
//  Returns:
//    
//
VOID
K10TransportManagerReleaseResponseBuffer(
                                        IN  PVOID   ResponsePtr
                                        );


// K10TransportManagerSend
//
//  Description:
//      This function will be called when clients of the Transport Manager need to send a message to 
//      the peer SP synchronously. If the message is sent synchronously then the status of the delivery 
//      is available immediately This function will also include the processing associated with 
//      request / response pair counters so that these counters can be correctly accounted during 
//      quiescing of Transport Manager  in K10TransportManagerShutdown().This routine must be called 
//      at passive level.
//
//  Arguments:
//      
//
//  Returns:
//    
//
EMCPAL_STATUS
K10TransportManagerSend (
                        IN PVOID    TMRegToken,
                        IN ULONG32  PeerMessageId,
                        IN PVOID    MessagePtr,
                        IN ULONG32  MessageSize,
                        OUT PVOID   *ResponsePtr,
                        OUT ULONG32 *ResponseSize
                        );

// K10TransportManagerResponse
//
//  Description:
//      This function will be called when clients of the Transport Manager need to send a response
//      to the peer SP. This function will also include the processing associated with request / response 
//      pair counters so that these counters can be correctly accounted during quiescing of Transport Manager
//     in K10TransportManagerShutdown().This routine must be called at passive level
//
//  Arguments:
//      
//
//  Returns:
//    
//
EMCPAL_STATUS
K10TransportManagerResponse (
                            IN PVOID    TMRegToken,
                            IN ULONG32  PeerMessageId,
                            IN PVOID    MessagePtr,
                            IN ULONG32  MessageSize,
                            IN PVOID    pTMDescriptor,
                            IN BOOLEAN  ReleaseInputBuffer
                            );

// K10TransportManagerGetStats
//
//  Description:
//      This function is called to return various statistics info maintained by the 
//      Transport Manager Library
//
//  Arguments:
//      
//
//  Returns:
//    
//
EMCPAL_STATUS
K10TransportManagerGetStats(
                           IN PVOID    TMHandle,
                           OUT PVOID * TMStats
                           );

// K10TransportManagerIsPeerAlive
//
//  Description:
//      This function is called to check if peer is online. This routine must be called at passive level.
//
//  Arguments:
//      
//
//  Returns:
//    
//
BOOLEAN
K10TransportManagerIsPeerAlive (void);

// K10TransportManagerLocalProtocolVersion
//
//  Description:
//      This function will return the communication version of the local SP.This functions should be
//      called whenever clients needs to populate the version assoicated with any message it is
//      sending or receiving from the peer.This routine must be called at passive or dispatch level.
//      bootup.
//
//  Arguments:
//      
//
//  Returns:
//    
//
ULONG32
K10TransportManagerLocalProtocolVersion(void);

// K10TransportManagerPeerProtocolVersion
//
//  Description:
//      This function will return the communication version of the Peer SP.Clients should call this routine
//      to populate the version assoicated with the message which is changed from release to release.
//      This routine must be called at passive or dispatch level.
//
//  Arguments:
//      
//
//  Returns:
//    
//
ULONG32
K10TransportManagerPeerProtocolVersion(void);


VOID
K10TransportUnload(void);

VOID
K10TransportManagerSetDelayedProcessingRequired (
                                        IN  PVOID   pK10TMDescriptor
                                        );

VOID
K10TransportManagerReleaseTMDescriptorAfterDelayedProcessing (
                                                         IN  PVOID   pK10TMDescriptor
                                                         );

// K10TransportManagerSeverCommunicationsLater
//
//  Description:
//     This function will sets a flag to indicate that all communication with the peer needs to be terminated
//     after the next incoming message is processed. This function will be called when the
//     client no longer wishes to receive any incoming messages, after its shutdown timer
//     has fired but before it gets a chance to call K10TransportManagerShutdown(). Any
//     calls made K10TransportManagerShutdown() after this will return successfully without
//     doing anything.This routine can be called at dispatch level.
//
//  Arguments:
//      
//
//  Returns:
//    
//
VOID
K10TransportManagerSeverCommunicationsLater( 
                           IN PVOID pK10TMHandle
                           );

#endif

