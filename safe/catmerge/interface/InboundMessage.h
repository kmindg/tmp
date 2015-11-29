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


#ifndef __InboundMessage_h__
#define __InboundMessage_h__

//++
// File Name: InboundMessage.h
//    
//
// Contents:
//      Defines the InboundMessage abstract base class. 
//

// Revision History:
//--
# include "SinglyLinkedList.h"

class InboundMessage;

SLListDefineListType(InboundMessage, ListOfInboundMessage);


// The InboundMessage abstract base class defines an interface for manipulating a message
// that has just arrived.
//
// A messages is virtually contiguous.  The body of the message may simply be read (using
// GetData() and GetDataLength()), or the beginning of the message may be "consumed"
// (logically removed from the message), so that subsequent views of the message do not
// contain that data. This is useful for stripping headers as the message is passed though
// layers of protocols.
//
// An InboundMessage is assumed to be "owned" by one context at a time, i.e., the
// interfaces are not thread safe.  The protocol for passing ownership is outside the
// scope of this definition.
//
// The owner of the message has the rights to put this message on one ListOfInboundMessage
// at a time.
//
// When the message is completely serviced, the last owner of the message must ensure that
//  the message is released, either by a call to ReceiveComplete() or by some other means
// that is specific to the sub-class.
//
class InboundMessage {
public:
    // This function must be called when the receiver is done with the message, unless the
    // sub-class provides an alternative method (e.g., implicit release of the message
    // upon return from the function that received the message).
    virtual VOID ReceiveComplete() = 0;

    // Return a pointer to the first byte of data that has not been consumed. The pointer
    // is valid until ReceiveComplete() is called or the equivalent.
    virtual PUCHAR GetData() const = 0;

    // Returns the number of bytes of data that have not been consumed. 
    virtual ULONG GetDataLength() const = 0;

    // Get a pointer to the first byte of unconsumed data, and consume that data.
    // 
    // @param numBytes - the number of bytes to consume from the message. Should be <=
    //                   GetDataLength() or this method will fail.
    //
    // Returns NULL if the message does not contain at least numBytes, and consumes
    // nothing, otherwise return a pointer to the first unconsumed (at the time of the
    // call) byte of data.  The pointer is valid until ReceiveComplete() is called or the
    // equivalent.
    virtual PUCHAR ConsumeData(ULONG numBytes) = 0;

    // Return the SP ID of the SP that sent this message.
    virtual const CMI_SP_ID & SourceSpId() const = 0;

    // Call ReceiveComplete for each message on the list.
    // @param msgList - the list of messages to complete.  Empty on return.
    static VOID ReceiveComplete(ListOfInboundMessage & msgList);


    // Message owner can queue these.....
    InboundMessage *                    mLink;

};


SLListDefineInlineCppListTypeFunctions(InboundMessage, ListOfInboundMessage, mLink);


inline VOID InboundMessage::ReceiveComplete(ListOfInboundMessage & msgList)
{

    // Release any messages from the failed peer that were waiting for trackers.
    for(;;) {
        InboundMessage * msg = msgList.RemoveHead();
        if (!msg) { break; }
        msg->ReceiveComplete();
    }
}
#endif  // __InboundMessage_h__

