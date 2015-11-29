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


#ifndef __Reservation_I_T_nexus_h__
#define __Reservation_I_T_nexus_h__

//++
// File Name: Reservation_I_T_nexus.h
//    
//
// Contents:
//      Defines the class  Reservation_I_T_nexus.  
//

// Revision History:
//--
# include "SinglyLinkedList.h"
# include "ScsiTargGlobals.h"
# include "scsitarghostports.h"
# include "ScsiTargInitiators.h"



// Contains a unique key for an IT_NEXUS, which allows that IT_NEXUS to be distinguished
// from all others. Note that this is large, because of iSCSI name.
class Reservation_I_T_nexus {
public:
    bool operator == (const Reservation_I_T_nexus & other) const
    {
        return mRelativeTargetPortIdentifier == other.mRelativeTargetPortIdentifier
            &&  INITIATOR_KEY_EQUAL(this->mInitiatorKey, other.mInitiatorKey);
    }

    bool operator != (const Reservation_I_T_nexus & other) const
    {
        return mRelativeTargetPortIdentifier == other.mRelativeTargetPortIdentifier
            &&  INITIATOR_KEY_EQUAL(this->mInitiatorKey, other.mInitiatorKey);
    }

    // Initialize the object.
    //
    // @param RelativeTargetPortIdentifier -  [SPC-3]3.1.90 relative target port
    //                                        identifier:
    //                                        A relative port identifier (see 3.1.88) for
    //                                        a SCSI target port (see 3.1.101). [SPC-3]
    //                                        Table
    //                                        311 — RELATIVE TARGET PORT IDENTIFIER field
    //    - Code Description
    //    -   0h Reserved
    //    -   1h Relative port 1, historically known as port A
    //    -   2h Relative port 2, historically known as port B
    //    -   3h - FFFFh Relative port 3 through 65535
    // @param InitiatorKey - Uniquely identifies the initator port.
    void Set(ULONG RelativeTargetPortIdentifier, const INITIATOR_KEY & InitiatorKey)
    {
        mInitiatorKey = InitiatorKey;
        mRelativeTargetPortIdentifier = RelativeTargetPortIdentifier;
    }

    // The SCSI spec requires each target port to have a unique identifier, which is good
    // enough for us to use to identify each port uniquely.
    ULONG           mRelativeTargetPortIdentifier:16;

    // Uniquely identifies the initiator.  
    INITIATOR_KEY   mInitiatorKey;
};

#endif  // __Reservation_I_T_nexus_h__

