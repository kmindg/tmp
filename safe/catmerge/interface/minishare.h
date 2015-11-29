/*++

Copyright (C) 1998  Data General Corporation

Module Name:

    minishare.h

Abstract:

    This file is shared by the TCD, CLARiiON MiniPort Drivers, CMI, and NTBE.

Author:

    Thomas B. Westbom

Environment:

    Kernel mode only - Shared with MiniPort Drivers.

--*/
#ifndef _MINIPORT_H_
#define _MINIPORT_H_

#include "minipath.h"

typedef struct _EVENT_INFO  *PEVENT_INFO;

//
//  Used by host\Inc\tcdmini.h and miniport\Inc\icdmini.h.  It defines
//  the Asynchronous Event Callback function type, which is called by
//  the MiniPort driver at IRQL Interrupt Level.
//
typedef BOOLEAN     (*PASYNC_CALLBACK)(PVOID Context, PEVENT_INFO Info);

//
//  These are all of the Inquiry DeviceType codes that we accept to date.
//
#define SCSITARG_DEVICE     0x1F        // TCD Target device.
#define FLAREDISK_DEVICE    0x1E        // NTBE Disk device.
#define FLAREXPANDER_DEVICE 0x1C		// NTBE Expander device


//
//  Careful how these are manipulated.  They are used directly in Inquiry Data.
//
#define VendorIdCLARiiON                                                \
    'C','L','A','R','i','i','O','N'

#define ProductIdItp5526Tfcb                                            \
    'I','t','p','5','5','2','6',' ','K','1','0',' ','D','i','s','k'

//
//  These are the async events which are reported by the miniport.
//  When an Async event is reported, the miniport freezes its queues
//  and expects a subsequent CTM_ASYNC_EVENT_DECREMENT to acknowledge each
//  async event reported.  When all async events are ack'd, the queues
//  are unfrozen.
//
//  Codes used by Miniport Drivers are segregated into two groups: those
//  which require actions to be taken, typically aborts, and those which are
//  purely informational and are meant to either be logged or ignored.
//
//  There are also TCD-only codes, which are part of this enumeration
//  for convenience.  Codes used only by the TCD should be inserted at the end.
//  Please add any future codes at the end of each group and don't forget to
//  add the little "= number" just in case the compiler forgets to do it.  8-0
//
typedef enum _SCSI_EVENT_TYPE
{
    //  Abort events which require outstanding operations to be aborted.
    //  Define what each event means.  The TCD is required to set the
    //  policy on what to abort.
    //

    //  These codes are defined in the SCSI specs.  Please don't attempt to
    //  redefine them here.  Thanks.
    //
    ET_BUS_RESET = 1,                       // Abort all CCBs for this bus.
    ET_BUS_DEVICE_RESET_MSG,                // EVENT_INFO.u.Path.Initiator
    ET_ABORT_INITIATOR_LUN_MSG,             // EVENT_INFO.u.Path
                                            // {Initiator, Lun} valid
    ET_ABORT_TAG_MSG,                       // EVENT_INFO.u.Path valid
    ET_ABORT_LUN_ALL_INITIATORS_MSG,        // EVENT_INFO.u.Path
                                            // {Initiator, Lun} valid
    ET_CLEAR_AUTO_CONTIGENT_ALLEGIANCE,     // EVENT_INFO.u.Path
                                            // {Initiator, Lun} valid
    ET_ABORT_INITIATOR_MSG,                 // EVENT_INFO.u.Path.Bitl
                                            // {Initiator} valid
	ET_LOGICAL_UNIT_RESET,

    //  Fibre Channel-specific codes.
    //
    //  These codes are defined in the Fibre Channel specs. and/or the
    //  Tachyon specs.  Please don't attempt to redefine them here.  Thanks.
    //
    ET_LINK_DOWN = 20,                      // EVENT_INFO.u.LinkState valid
    ET_LINK_UP,                             // EVENT_INFO.u.LinkState valid
    ET_LOOP_TO,                             // Link time-out.
    ET_TACHYON_OFFLINE,
    ET_LIP_TIMEOUT,
    ET_LINK_UP_TIMEOUT,
    ET_PORT_LOGIN,                          // EVENT_INFO.u.PortLogin valid
    ET_PORT_LOGOUT,                         // EVENT_INFO.u.PortLogin valid
    ET_LOOP_HUNG,
    ET_LOOP_NEEDS_DISCOVERY,                // EVENT_INFO.u.Boolean valid
    ET_TEST_LOOP,
    ET_TEST_TOPOLOGY,
    ET_HUNG,
    ET_SUSPEND,
    ET_RESUME,

    ET_GET_AFFECTED_INITIATORS,
    ET_TPRLO_RESET,


    // Define events for new dual-mode asynchronous callbacks

    FC_EVENT_FABRIC_STATE_CHANGE = 40,
    FC_EVENT_LOGIN_FAILED,                  // EVENT_INFO.u.PortLogin valid
    FC_EVENT_DEVICE_FAILED,                 // EVENT_INFO.u.PortLogin valid
    FC_EVENT_LOGIN_REQUESTED              // EVENT_INFO.u.LoginRequested valid

} SCSI_EVENT_TYPE;

// Bit field values for current_login_type and requested_login_type fields
// of FC_LOGIN_REQUESTED structure

#define FC_LOGIN_REQUESTED_INITIATOR    0x00000001  // May initiate commands
#define FC_LOGIN_REQUESTED_TARGET       0x00000002  // Accept inbound commands



#endif  //_MINIPORT_H_
