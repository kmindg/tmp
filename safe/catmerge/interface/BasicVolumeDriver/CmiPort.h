/************************************************************************
*
*  Copyright (c) 1998-2006 EMC Corporation.
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

#ifndef _CMI_PORT_H_
#define _CMI_PORT_H_

//++
// File Name: CmiPort.h
//    
//
// Contents:
//      Defines a class that allows CmiPortMessage's to be transmitted between SPs.
//
// Revision History:
//--

# include "CmiUpperInterface.h"
# include "BasicVolumeDriver/CmiPortMessage.h"
# include "BasicVolumeDriver/CmiPortReceivedMessage.h"
# include "BasicLib/BasicConsumedDevice.h"

// The code assumes that a CmiPortReceivedMessage will fit into the ExtraData field of a
// CMI message.  This will prevent the contrary from compiling.
extern int ASSERTER[CMI_MESSAGE_NUM_BYTES_EXTRA_DATA >= sizeof(CmiPortReceivedMessage)];

//   CmiPort defines an abstract base class that allows CmiPortMessage's to be transmitted
//   between SPs.
//
//   A CmiPort is a full duplex message passing channel between SPs.  
//
//   Multiple CmiPort's may exist between a pair of SPs.
//
//   Each direction has independent flow control. Incoming messages are delivered by a
//   callback mechanism.  The callback may either handle the incoming message immediately,
//   or it may hold the message indefinitely, potentially causing subsequent messages to
//   be held on the sender.  In the latter case, there is significant deadlock potential
//   which the user of this class must avoid.
//
//   A CmiPort must be "Open"ed before use.  
class CmiPort: public BasicConsumedDevice {
public:
    CmiPort();

    ~CmiPort();


    // Enable communication via this port to all SPs.  CloseCmiPort() is required before
    // destruction.  A particular CmiPort can be opened at most once. A CmiPort can be
    // opened regardless of whether there is currently another SP to communicate with.
    //
    // Note: Messages can arrive on this port *prior* to this function returning.
    // 
    // @param conduit - identifies the port number that should be used for all incoming
    //                  and outgoing messages.  Port numbers must be allocated in
    //                  CmiUpperInterface.h
    // @param CallerProvidedMemory - caller provided memory for the ring buffer.  Default:
    //                               CMID decides size/allocates memory.
    // @param CallerProvidedSize - size of provided memory
    // 
    // Returns a success status if the port is open and ready for use, an error status if
    // the port could not be opened (e.g., it is already open).
    EMCPAL_STATUS    OpenCmiPort(CMI_CONDUIT_ID conduit, PVOID CallerProvidedMemory = NULL, ULONG CallerProvidedSize = 0, BOOLEAN WaitForClientCredit = false);

    // Close the port opened by OpenCmiPort.
    VOID        CloseCmiPort();


    // Send a message to some SP.  MessageTransmitted() will be called when the message is
    // sent, or when the destination goes offline, if a success status is returned. The
    // default implementation of MessageTransmitted() will call
    // CmiPortReceivedMessage::SendComplete().
    // Note: MessageTransmitted() is called asynchronously, and therefore might be called
    // before this function completes returning.
    // 
    // @param   message - the message to send.  The message knows the destination and the
    //                    contents.
    //
    // Return Value:
    //    !NT_SUCCESS() - SP down, or pipe not open
    EMCPAL_STATUS SendMessage(PCmiPortMessage message);

    // Send an IOCTL to the CMID to panic the desired SP. It will
    // be synchronous operation.
    //
    // @param spId - Identifies the SP.
    //
    // Returns:
    //  NT_SUCCESS() - Successfully panicked the SP.
    //  !NT_SUCCESS() - Unable to panic the SP. 
    EMCPAL_STATUS PanicSpSynchronous(CMI_SP_ID spId);
    
    // Send an IOCTL to the CMID to panic the desired SP. It will
    // be asynchronous operation and therefore doesn't wait for
    // notification that the IOCTL was received by the specified SP.
    //
    // @param spId - Identifies the SP.
    //
    // Returns:
    //  NT_SUCCESS() - Successfully panicked the SP.
    //  !NT_SUCCESS() - Unable to panic the SP. 
    EMCPAL_STATUS PanicSpAsynchronous(CMI_SP_ID spId);
    
    // Send a response message to the SP that sent the original message.
    // 
    // @param message - the message to send.
    // @param origMessage - the message to respond to.
    //
    // Return Value:
    //    !NT_SUCCESS() - SP down, or pipe not open.
    EMCPAL_STATUS SendResponse(PCmiPortMessage message, CmiPortReceivedMessage * origMessage);

    // Called when the receiver has accepted (but not necessarily read/serviced) the
    // message. If the sender fails, the receiver will still receive the message.
    // CmiPortMessage::SendComplete() will be called by this function, if not overridden
    // by the subclass.
    //
    // @param message - the message that was just transmitted. 
    virtual VOID MessageTransmitted(CmiPortMessage * message);

    // Called back when the connection to another SP fails.  All connections to that SP
    // will fail simultaneously.
    //
    // @param delayedReleaseHandle - used later if "false" is returned.
    // @param spId - the SP that failed or became unreachable.
    //
    // Return Value:
    //    false - delayed Release; don't allow reconnection by the failed SP until
    //            DelayedContactLostComplete() is called.
    //    true - ContactLost processing is complete.
    virtual bool ContactLost(PCMI_SP_CONTACT_LOST_EVENT_DATA delayedReleaseHandle, CMI_SP_ID spId) = 0;

    // Acknowledges a ContactLost event that was previously delayed.  Does not need
    // specific information about the CmiPort to do this.
    //
    // @param delayedReleaseHandle - the value that was previously passed in to
    //                               ContactLost().
    static void DelayedContactLostComplete(PVOID delayedReleaseHandle);

    // Called back when a message arrives from some SP, including the peer SP (except if
    // the sub-class requested peer messages separately). Message may arrive prior to the
    // open completing.
    //
    // @param message - the message that just arrived.
    // 
    // Return Value:
    //    false - delayed Release; message is held waiting further service and is not
    //            freed until DelayedMessageReceivedComplete().  The subclass is
    //            responsible for deadlock avoidance.
    //    true - Message processing is complete.
    virtual bool MessageReceived(CmiPortReceivedMessage * message) = 0;

    // Called back when the connection to the peer SP fails.  All connections to that SP
    // will fail simultaneously.
    //
    // @param delayedReleaseHandle - used later if "false" is returned to identify what to
    //                               release.
    //
    // Return Value:
    //    false - delayed Release; don't allow reconnection until
    //            DelayedContactLostComplete() is called.
    //    true - ContactLost processing is complete.
    virtual bool PeerContactLost(PCMI_SP_CONTACT_LOST_EVENT_DATA delayedReleaseHandle) 
    {
        return ContactLost(delayedReleaseHandle, PeerSP_ID());
    }

    // Called back when a message arrives from the Peer SP. Defaults to calling the non-SP
    // specific MessageReceived() function.
    //
    // @param message - the message that was received
    //
    // Return Value:
    //    true - delayed Release; message is held waiting further service.
    //    false - Message processing is complete.
    virtual bool PeerMessageReceived(CmiPortReceivedMessage * message) {
        return MessageReceived(message);
    }

    // Called back at DISPATCH_LEVEL when a message arrives from some SP that is
    // attempting to deliver fixed data.
    //
    // @param pDMAAddrEventData - describes the data that is arriving, and provides space
    //                            for the subclass to indicate where the fixed data should
    //                            be DMA'd to.  See CmiUpperInterface.h for details.
    virtual VOID DMAAddrNeeded(
        PCMI_FIXED_DATA_DMA_NEEDED_EVENT_DATA pDMAAddrEventData) const = 0;

    // Acknowledges a message receive event that was previously delayed. A prior call to
    // PeerMessageReceived() or MessageReceived() functions in the subclass was returned
    // with true, indicating that the message service was not complete.  This call signals
    // that the message is no longer needed.
    //
    // @param message - identifies the message to be released.
    void DelayedMessageReceivedComplete(CmiPortReceivedMessage * message);

    // Returns the SP ID of the Peer SP.
    const CMI_SP_ID & PeerSP_ID() const { return mPeerSpId; }

protected:
    // Tells CMID to complete quickly any messages blocked on send flow control that have
    // been marked as cancelled for this SP.  Such messages always complete the message
    // send (calling MessageTransmitted()), possibly with a status of STATUS_CANCELLED.
    // Those messages should complete shortly.
    //
    // @param spid - the SP ID to which there might be messages to be sent, where the
    //               "cancelled" flag may be set on some messages.
    void PurgeCancelledMessages(CMI_SP_ID spid);

private:
    // Create a CmiPortReceivedMessage message object from an arriving message.
    //
    // @param mred - the raw CMID form of the received message.
    // @param memory - memory to be used for the CmiPortReceivedMessage structure.
    // @param size - the number of bytes allocated to "memory".
    //
    // Returns the message (cannot fail).
    CmiPortReceivedMessage * ConstructReceivedMessage(
        PCMI_MESSAGE_RECEPTION_EVENT_DATA mred,
        PVOID memory, ULONG size);

    // Send an asynchronous IRP_MJ_DEVICE_CONTROL to the CMID driver.
    // CmiPort::IoctlIrpCompletion() will be called when the IRP is done.
    //
    // @param ioctlCode - the IOCTL code to use in the IO_STACK_LOCATION.
    // @param pInputBuffer - the data to send down the device stack.
    // @param inputBufferSize - the number of bytes in pInputBuffer.
    // @param pOutputBuffer - a buffer to receive any output data in.
    // @param outputBufferSize - the number of bytes in pOutputBuffer.
    // @param waiter - It will be signaled when IRP completes.
    //
    // Returns: 
    //   NT_SUCCESS()  - IRP was sent
    //   otherwise - IRP failed.
    EMCPAL_STATUS SendIoctl(IN ULONG ioctlCode,
        IN PVOID pInputBuffer,
        IN ULONG inputBufferSize,
        IN PVOID pOutputBuffer,
        IN ULONG outputBufferSize,
        BasicWaiter * waiter = NULL);

    // Function that CMID calls if an event occurs on some conduit.
    // 
    // @param Event - the event type, see CmiUpperInterface.h for details.
    // @param Data - depends on Event, see CmiUpperInterface.h for details.
    //
    // Returns status appropriate to a handler of CMID events, see CmiUpperInterface.h for
    // details.
    static EMCPAL_STATUS CmiEventCallback(IN CMI_CONDUIT_EVENT Event, IN PVOID Data);

    // Called when an Ioctl IRP completes in the driver below.
    //
    // @param pDeviceObject - the device that just completed the Ioctl 
    // @param pIrp - the IRP that just completed.
    // @param pContext 
    //
    // Returns STATUS_MORE_PROCESSING_REQUIRED;
    static EMCPAL_STATUS IoctlIrpCompletion(IN PVOID Unused,
        IN PBasicIoRequest pIrp,
        IN PVOID pContext);


    // Called when a Cmi_Conduit_Event_Message_Transmitted event is received.
    //
    // message - the message whose delivery to another SP has completed.
    //           message->GetStatus() can be used to determine if the send was successful.
    VOID HandleEventMessageTransmitted(CmiPortMessage * message);

    // Called when a Cmi_Conduit_Event_Message_Received event is received.
    //
    // pReceptionEventData - describes the message that just arrived, see
    //                       CmiUpperInterface.h for details.
    EMCPAL_STATUS HandleEventMessageReceived(PCMI_MESSAGE_RECEPTION_EVENT_DATA pReceptionEventData);

    // Called when a Cmi_Conduit_Event_Close_Completed event is received.
    //
    // pCloseConduitIoctlInfo - describes the conduit close that just completed, see
    //                          CmiUpperInterface.h for details.
    VOID HandleEventCloseComplete(PCMI_CLOSE_CONDUIT_IOCTL_INFO pCloseConduitIoctlInfo);

    // Called when a Cmi_Conduit_Event_Sp_Contact_Lost event is received.
    //
    // pContactLostInfo - describes the SP that just failed, see CmiUpperInterface.h for
    //                    details.
    VOID HandleEventContactLost(PCMI_SP_CONTACT_LOST_EVENT_DATA pContactLostInfo) ;

    //  Which CMI conduit was opened?  Only valid while open.
    CMI_CONDUIT_ID mConduit;

    // Temporary memory to avoid a large structure on the stack and to avoid memory
    // allocation failures.
    CMI_GET_SP_IDS_IOCTL_INFO   mSpIds;

    // The SP ID of the partner/peer SP.
    CMI_SP_ID   mPeerSpId;


    // Each CONDUIT might have at most one CmiPort.  This table allows events presented on
    // CONDUITs to be translated to the CmiPort used to open that conduit.
    static CmiPort * CmiPortConduitToPortTable[CMI_MAX_CONDUITS];
};


inline  CmiPortReceivedMessage * CmiPort::ConstructReceivedMessage(
    PCMI_MESSAGE_RECEPTION_EVENT_DATA mred,
    PVOID memory, ULONG size) 
{
    CmiPortReceivedMessage * msg = (CmiPortReceivedMessage *)memory;
    DBG_ASSERT(size >= sizeof(*msg));

#ifdef CSX_AD_COMPILER_TYPE_VS
    msg->CmiPortReceivedMessage::CmiPortReceivedMessage(mred, this);
#else
    new (msg) CmiPortReceivedMessage(mred, this);
#endif

    return msg;
}


#endif 
