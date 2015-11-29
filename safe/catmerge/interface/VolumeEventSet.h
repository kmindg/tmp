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


#ifndef __VolumeEventSet_h__
#define __VolumeEventSet_h__

//++
// File Name: VolumeEventSet.h
//    
//
// Contents:
//      Defines possible events which might be notified to registeted drivers.
//

// Revision History:

// The possible events which might be notified.
enum VolumeEvent { 

    // The volume size may have changed.
    VolumeEventSizeChanged = 2,

    // The logical unit is in the process of being reset.
    VolumeEventLogicalUnitReset = 4,

    // The logical unit has been told to execute a Clear Task Set operation.
    VolumeEventClearTaskSet = 8,

    // The state of Optimal/Non-optimal paths may have changed.
    VolumeEventOwnerChange = 0x10,

    // The state of reservations may have changed.
    VolumeEventReservationChange = 0x20,

    // The some volume parameter may have changed.
    VolumeEventParameterChange = 0x40,

    // An SP just booted.
    VolumeEventSpUp = 0x80,

    // Transition of Ready
    VolumeEventReadyStateChanged = 0x100,

    // Transistion of TrespassDisable.  TrespassDisables do not 
    // generate notifications from the drivers, so 
    // there should not be a dependency on async notification of 
    // this event.
    VolumeEventTrespassDisableChanged = 0x200,

    // Transistion of volume attributes
    VolumeEventAttributesChanged = 0x400,

    //Make sure that this volume gets at least one notification
    //to do whatever initialization it has to do.
    VolumeEventAddVolume = 0x800,

    //Notification to indicate that Erase information changed
    VolumeEventEraseInfoChanged = 0x1000
};

// Some volume events only require Upper Redirector ownership while others require
// ownership at the layered driver level also.
const ULONG VolumeEventsRequiringLayeredDriverOwnership = (
    VolumeEventSizeChanged|VolumeEventReadyStateChanged|
    VolumeEventTrespassDisableChanged|VolumeEventAttributesChanged);

// Sets of volume events are implemented as a bitmap.
typedef ULONG VolumeEventSet;

#endif  // __VolumeEventSet_h__
