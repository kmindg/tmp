//***************************************************************************
// Copyright (C) EMC Corporation 2013
// Licensed material -- property of EMC Corporation
//**************************************************************************/

#ifndef __NvcmPortReceivedMessage_h__
#define __NvcmPortReceivedMessage_h__

//++
// File Name: NvcmPortReceivedMessage.h
//    
//
// Contents:
//      Defines the NvcmPortReceivedMessage class.
//
// Revision History:
//--

#include "CmiUpperInterface.h"
#include "InboundMessage.h"
#include "Messages/CacheMessages.h"
#include "References/NvcmReference.h"

//  A NvcmPortReceivedMessage describes a message that should be sent back to 
//  MCC sender right after mirroring the reference data to eHornet.  
//  The message is held in the private memory of NvcmReferenceProcessor
//
//  The external interface to a message is defined by the abstract base class
//  InboundMessage.
//
//  Unlike CmiPortReceivedMessage where a message is just a part of Cmi buffer, which contains Cmi specific
//  header, the NvcmPortReceivedMessage holds only CachePageReferenceSetMessage and there is no need to
//  consume the Cmi specific header to get the pointer to CachePageReferenceSetMessage 
//  Refer to CmiPortReferenceMessage.h for details
// 
//  !!!Note:
//  The NvcmPortReceivedMessage panics when ConsumeData(ULONG numBytes) and SourceSpId() methods are called.
class NvcmReferenceProcessor;

class NvcmPortReceivedMessage : public InboundMessage 
{
    public:

        // This method returns the total number of bytes of floating data 
        ULONG       GetDataLength() const;

        // Returns a pointer to the next byte of unconsumed data. 
        // The pointer is valid until ReceiveComplete() is called or the equivalent.
        PUCHAR      GetData() const;

        // This method shouldn't be called for NvcmPortReceivedMessage
        PUCHAR      ConsumeData(ULONG numBytes);

        // This function must be called when the receiver is done with the message, unless the
        // release of the message is implicit upon return from the function that received the
        // message.
        VOID ReceiveComplete();

        // This method shouln't be called for NvcmPortReceivedMessage
        // Returns the ID of the SP that originated this message.
        virtual const CMI_SP_ID & SourceSpId() const 
        {
            FF_ASSERT(false); 
            return originating_sp; 
        }

        // This method just duplicates the reference set to its local reference set message
        void CopyReferenceForResponse(CacheReferenceMessage* msg,  NvcmReference* ref);
        void CopyReferenceSetHead(CachePageReferenceSetMessage* refSet);
        void InitMessage(USHORT core);
       
        bool IsFull()
        {
            return mMsg.mNumPageMessages == CachePageReferenceSetMessage::MaxLogicalPagesPerReferenceSetMessage; 
        }

        bool NeedToSend()
        {
            return mMsg.mNumPageMessages > 0;
        }
        
        // This method binds the message to corresponding NvcmReferenceProcessor
        void SetReferenceProcessor(NvcmReferenceProcessor * RefProcessor);

    protected:
        CMI_SP_ID originating_sp;
        CachePageReferenceSetMessage mMsg;
        NvcmReferenceProcessor * mRefProcessor;
};

typedef NvcmPortReceivedMessage *PNvcmPortReceivedMessage;

#endif // __NvcmPortReceivedMessage_h__

