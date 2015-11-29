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

#ifndef _CmiPortMessage_h_
#define _CmiPortMessage_h_

//++
// File Name: CmiPortMessage.h
//    
//
// Contents:
//      A CmiPort message hold an OutboundMessagePayload while that payload is being
//      transmitted to another SP.
//
// Revision History:
//--

# include "CmiUpperInterface.h"
# include "BasicVolumeDriver/OutboundMessagePayload.h"

class CmiPort;
class CmiPortMultiplexor;

// A CmiPortMessage holds an OutboundMessagePayload while that payload is being
// transmitted to another SP.
// 
// This structure contains all the resources necessary to queue a payload to an outgoing
// CmiPort.
class CmiPortMessage : public CMI_TRANSMIT_MESSAGE_IOCTL_INFO {
public:
    virtual ~CmiPortMessage();

    CmiPortMessage();

    // Establish the contents of the message, to be used when the message is sent.
    // 
    // @param header       - The starting virtual address of a range of virtual memory to
    //                       be sent as the beginning of the floating data for this
    //                       message.
    // @param headerLength - The number of bytes of data in the header.
    // @param payload      - Describes the destination, the data that will follow the
    //                       header (with the source addresses specified as virtual or
    //                       physical addresses), plus an optional description of data to
    //                       be DMAed between the source and destination SPs.
    //
    // Returns the number of floating bytes in the payload.
    ULONG SetupPayload(PVOID header, ULONG headerLength, OutboundMessagePayload * payload);
    
    // Send this message.  GetTargetSpId() specifies the destination.
    //
    // @param port - the CmiPort to send this message to.
    //
    // Return Value:
    //   success - the send has been initiated, payload->SendComplete() will be called
    //             later.
    //   error - the send has not been initiated, no callbacks will occur.
    EMCPAL_STATUS SendMessage(CmiPort * port);

    // The message send is complete, with the message either delivered or failed.
    //
    // @param Status - success indicates that the message was successfully sent, failure
    //                 indicates that the destination is down, or the message is too
    //                 large.
    virtual VOID SendComplete(EMCPAL_STATUS Status) = 0;

    // What SP should this message be sent to.
    virtual const CMI_SP_ID & GetTargetSpId() const = 0;

    // Returns the resulting transfer status.  Valid only after SendComplete() is called.
    EMCPAL_STATUS GetStatus() const { return transmission_status; }

protected:

    // Adds a source virtual address range to the current end of the floating data.
    //
    // @param addr - the source virtual address range
    // @param len - the number of bytes
    VOID AppendVirtualFloatingData(PVOID addr, ULONG len);

    // Adds a source virtual address range to the current end of the floating data.
    //
    // @param addr - the source physical address range
    // @param len - the number of bytes
    VOID AppendPhysicalFloatingData(SGL_PADDR addr, ULONG len);


    // CmiUpperInterface expects the sender to pass in SGLs, requiring the sender to
    // allocate memory for the SGLs, rather than the more flexible interface used by
    // OutboundMessagePayload, where the sender provides a means to query for the source
    // addresses.  To bridge these two worlds, we allocate SGLs here.  A better approach
    // would be to have CMID, which creates its own SGLs anyway, provide a similar type of
    // callback interface.
    enum FIXED_MAXIMUMS {

        // How many SGLs for transfers to pre-allocate for fixed and floating transfers?
        // 2 (for header) + 1 (NULL terminate) + 257 (4K pages, unaligned, 1MB) + 1 NULL
        // terminate
    # if defined(SIMMODE_ENV)
        // support 128-element SG
        MAX_SGLS = 132
    # else
        MAX_SGLS = 261 
    # endif
    };

    // Memory for virtual and physical SGLs.  
    CMI_PHYSICAL_SG_ELEMENT mSgl[MAX_SGLS];


    // The # of entries used in the SGL.
    ULONG                   mSglIndex;

    // The # of bytes represented by mFloatSgl.
    ULONG                   mFloatLen;
};




typedef CmiPortMessage * PCmiPortMessage;


#endif
