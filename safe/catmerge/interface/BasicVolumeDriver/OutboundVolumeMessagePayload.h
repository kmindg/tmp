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

#ifndef __OutboundVolumeMessagePayload_h__
#define __OutboundVolumeMessagePayload_h__

//++
// File Name: OutboundVolumeMessagePayload.h
//    
//
// Contents:
//      Defines the OutboundVolumeMessagePayload abstract base class. 
//
// Revision History:
//--

#include "BasicVolumeDriver/OutboundMessagePayload.h"


// Defines where an inter-SP message should be routed, either to the driver object or a
// volume object.
enum VolumeMessageDestinationType {
    // Message is directed at the driver itself.
    VolumeMessageDestinationDriver, 

    // The message is directed to a volume.
    VolumeMessageDestinationVolume 
};

// OutboundVolumeMessagePayload defines an interface that is used to describe the data
// that is to be sent in an outbound message that is targeted at a particular volume.
class OutboundVolumeMessagePayload : public OutboundMessagePayload {
public:

    // Returns what type of object this message is being sent to.
    virtual VolumeMessageDestinationType DestinationObject() const = 0;

    // Identifies the volume that this message is to be sent to.
    virtual const VolumeIdentifier & VolumeId() const = 0;

    // Allows query as to the virtual address of a range of the payload to be sent as
    // "floating" data. The floating data payload is assumed to be a bytestream made up of
    // one or more disjoint virtual address ranges.  This byte stream is logically
    // contiguous, starting at offset 0.
    //
    // @param offset - what offset is this query for?  (0 => first byte).
    // @param address - returns the virtual address of the range starting at offset.
    //
    // Return Value:
    //   0 - 'offset' is past the end of the byte stream.
    //   other - the number of bytes in the range.
    virtual ULONG GetPayloadVirtualFloatingRange(ULONG offset, PVOID & address) const = 0;

    // Used only by ListOfOutboundMessagePayload or IFIFOListOfOutboundMessagePayload
    OutboundVolumeMessagePayload * mLink;
};

SLListDefineListType(OutboundVolumeMessagePayload, ListOfOutboundVolumeMessagePayload);

SLListDefineInlineCppListTypeFunctions(OutboundVolumeMessagePayload, 
                                       ListOfOutboundVolumeMessagePayload, mLink);

#endif  // __OutboundVolumeMessagePayload_h__

