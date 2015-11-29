/************************************************************************
*
*  Copyright (c) 2002, 2005-2006 EMC Corporation.
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

#ifndef __VolumeMultiplexor_H__
#define __VolumeMultiplexor_H__


//++
// File Name:
//    
//
// Contents:
//      Defines the VolumeMultiplexor base class. 
//
//
// Revision History:
//--

class BasicVolume;
class OutboundVolumeMessagePayload;

// VolumeMultiplexor is an interface through which messages can be sent to another SP. A
// VolumeMultiplexor knows how to send and receive messages that are targeted at
// particular volumes, and it routes incoming messages to those volumes.
//
//
// BasicVolumeDriver::CreateVolumeMultiplexor() is used to create an object which supports
// the VolumeMultiplexor interface. Arriving messages call
// BasicVolume::PeerMessageReceived() or BasicVolume::MessageReceived() on the associated
// volume.
class  VolumeMultiplexor {
public:

    // Send a message to the instance of the specified volume on the other SP.
    // 
    //   Note: payload->SendComplete() will be called when the data is safe on the peer SP
    //   or the Peer SP has failed or the message was cancelled or had some other error.
    //
    // @param payload - defines the data to be sent to the volume. Also defines the target
    //                  volume and the target SP.
    //
    virtual VOID SendMessage(OutboundVolumeMessagePayload * payload) = 0;

    // Get a message previously sent via SendMessage() to call SendComplete() quickly,
    // perhaps having the send fail with STATUS_CANCELLED.
    // 
    // @param payload - the prior parameter to SendMessage.  This is used as a key to find
    //                  the message, and if the message is no longer being sent, then this
    //                  function will not dereference this pointer.
    //
    // WARNING: The caller must ensure that if the message happens to complete during this
    // call, that the message will not be reused/resent until this call completes, since
    // this could cause the new instance of the message to be cancelled.
    //
    virtual VOID CancelSendMessage(OutboundVolumeMessagePayload * payload) = 0; 

    virtual ~VolumeMultiplexor() {};
};



#endif  // __VolumeMultiplexor_H__

