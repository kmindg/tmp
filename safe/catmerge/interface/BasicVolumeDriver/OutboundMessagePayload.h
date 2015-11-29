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

#ifndef __OutboundMessagePayload_h__
#define __OutboundMessagePayload_h__

//++
// File Name: OutboundMessagePayload.h
//    
//
// Contents:
//      Defines the OutboundMessagePayload abstract base class. 
//
// Revision History:
//--

#include "BasicVolumeDriver/BasicVolume.h"
#include "flare_sgl.h"
#include "SinglyLinkedList.h"
#include "InterlockedFIFOList.h"


// OutboundMessagePayload defines an interface that is used to describe the data that is
// to be sent in an outbound message, and the callback function that will be executed when
// the send is complete.
class OutboundMessagePayload {
public:
    // Callback for when the payload is confirmed as sent.
    //
    // @param status - indicates whether the send was confirmed as sent, or whether the
    //                 send failed.  The send can fail if the target of the send is
    //                 offline, the message was cancelled, or the message is too large.
    virtual VOID SendComplete(EMCPAL_STATUS status) = 0;

    // Returns the SP ID of where the message should be sent.
    virtual const CMI_SP_ID & GetTargetSpId() const = 0;


    // Allows query as to the virtual address of a range of the payload to be sent as
    // "floating" data.  The floating payload source data is assumed to be a bytestream
    // made up of one or more disjoint virtual address ranges followed by zero or more
    // disjoint physical address ranges.  The set of virtual ranges starts at offset ==
    // 0.
    //
    // @param offset - what offset is this query for?  (0 => first byte).
    // @param address - returns the virtual address of the range starting at offset.
    //
    // Return Value:
    //   0 - 'offset' is past the end of the byte stream
    //   other - the number of bytes in the range.
    virtual ULONG GetPayloadVirtualFloatingRange(ULONG offset, PVOID & address) const = 0;

    // Allows query as to the physical address of a range of the payload to be sent as
    // "floating" data.  The floating payload source data is assumed to be a bytestream
    // made up of one or more disjoint virtual address ranges followed by zero or more
    // disjoint physical address ranges.  The set of physical ranges starts at offset ==
    // 0.
    //
    // @param offset - what offset is this query for?  (0 => first byte).
    // @param address - returns the physical address of the range starting at offset.
    //
    // Return Value:
    //   0 - 'offset' is past the end of the byte stream
    //   other - the number of bytes in the range.
    virtual ULONG GetPayloadPhysicalFloatingRange(ULONG offset, SGL_PADDR & address) const 
    {
        // Physical floating is rare, so provide a default...
        return 0;
    }


    // Returns an SGL describing the fixed data.
    //
    // @param maxSglEntries - the maximum number of entries that can be placed in the sgl
    //                        array.
    // @param sgl - an array to be filled in with SGL entries to describe the fixed data,
    //              up to maxSglEntries in length.  Entries should contain physical
    //              addresses to send from.
    // @param blob - the fixed data description blob that tells the receiver where to put
    //               the fixed data.
    // @param blobLen - the number of bytes in blob.
    //
    // Return Value:
    //   0 - there is no 'fixed' data, sets sgl = NULL, blob = NULL, blobLen = 0.
    //   other - the number of bytes of fixed data described by 'sgl'.
    virtual ULONG GetPayloadFixedSGL(ULONG maxSglEntries, PSGL sgl, PVOID & blob, ULONG & blobLen) const 
    {
        return 0;
    }
};

#endif  // __OutboundMessagePayload_h__

