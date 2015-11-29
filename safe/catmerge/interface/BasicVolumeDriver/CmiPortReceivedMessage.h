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

#ifndef __CmiPortReceivedMessage_h__
#define __CmiPortReceivedMessage_h__

//++
// File Name: CmiPortReceivedMessage.h
//    
//
// Contents:
//      Defines the CmiPortReceivedMessage class.
//
// Revision History:
//--

# include "CmiUpperInterface.h"
# include "InboundMessage.h"

class CmiPort;
typedef CmiPort * PCmiPort;

//  A CmiPortReceivedMessage describes a message that has just been received.  The message
//  is held in the receive ring buffer of a CMI pipe, potentially flow-controlling the
//  sender, during the lifetime of this object.
//
//  The external interface to a message is defined by the abstract base class
//  InboundMessage.
//
//  A message is a virtually contiguous byte stream in memory.  The beginning of this byte
//  stream may be "consumed", leaving only the unconsumed data accessible.
//
// Notes:
//  The CmiPortReceivedMessage is often instantiated within some portion of the message it
//  describes, e.g. in
//      CMI_MESSAGE_RECEPTION_EVENT_DATA::extra_data
//  this allows an arbitrary number of messages to be received without any explicit memory
//  allocations (and therefore no out of memory conditions).
class CmiPortReceivedMessage : public InboundMessage {
public:

    // The constructor wraps the object around a received message.
    //
    // @param msg - the message being recieved (see CmiUpperInterface.h).
    // @param port - the port (CMI CONDUIT) that this message was received on.
    CmiPortReceivedMessage(PCMI_MESSAGE_RECEPTION_EVENT_DATA msg, PCmiPort port);


    // Pure virtual in the base class InboundMessage, this subclass returns the total
    // number of bytes of floating data that have not been consumed.
    ULONG       GetDataLength() const;

    // Pure virtual in the base class InboundMessage, this subclass returns a pointer to
    // the next byte of unconsumed data. The pointer is valid until ReceiveComplete() is
    // called or the equivalent.
    PUCHAR      GetData() const;

    // Pure virtual in the base class InboundMessage, get a pointer to the first byte of
    // unconsumed data, and consume that data.
    // 
    // @param numBytes - the number of bytes to consume from the message. Should be <=
    //                   GetDataLength() or this method will fail.
    //
    // Returns NULL if the message does not contain at least numBytes, and consumes
    // nothing, otherwise return a pointer to the first unconsumed (at the time of the
    // call) byte of data.  The pointer is valid until ReceiveComplete() is called or the
    // equivalent.
    PUCHAR      ConsumeData(ULONG numBytes);


    // This function must be called when the receiver is done with the message, unless the
    // release of the message is implicit upon return from the function that received the
    // message.
    VOID ReceiveComplete();


    // Returns the ID of the SP that originated this message.
    const CMI_SP_ID & SourceSpId() const { return mMsg->originating_sp; }

    // Returns the port that this message was received on.
    PCmiPort SourcePort() const { return mPort; }


    // Returns a description of the underlying CMID message.  See CmiUpperInterface.h
    PCMI_MESSAGE_RECEPTION_EVENT_DATA GetReceptionData() const { return mMsg; }

protected:

    // What real message does this object wrap?
    PCMI_MESSAGE_RECEPTION_EVENT_DATA   mMsg;

    // The number of bytes of data that are currently consumed.
    ULONG                               mBytesConsumed;

    // The CmiPort this message was received on.
    PCmiPort                            mPort; 
};

typedef CmiPortReceivedMessage * PCmiPortReceivedMessage;

#endif // __CmiPortReceivedMessage_h__

