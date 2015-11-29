/*++

Copyright (C) 1997  Data General Corporation

Module Name:

    icdmini.h

Abstract:
    "Initiator Class Driver"/"MiniPort Driver" shared header file.

Author:

    Hugh Nhan 

Environment:

    Kernel mode only:

    This file is shared by back-end drivers ONLY.  Neither the TCD, TxDs,
    nor the host-side-only MiniPort drivers should include this file.

--*/
#ifndef _ICDMINI_H_
#define _ICDMINI_H_

#include "generics.h" 
#include "minishare.h"

#define CIM_REGISTER_CALLBACK           0x90000081L
#define CIM_GET_EXTENSIONS              0x90000082L
#define CIM_RECOVERY_CONFIG             0x90000083L
#define CIM_DEREGISTER_CALLBACK         0x90000084L
#define CIM_CM_LOOP_COMMAND             0x90000085L
#define CIM_RESET_DEVICE                0x90000086L
#define CIM_BUS_RESET                   0x900000F0L     // Test code.

#define TFCB_VER_MAJOR          1
#define TFCB_VER_MINOR          0

#define MAX_DISK_DEVICES        120

//
//  The miniport and enclosure-services use these values to
//  exchange loop maps for discovery requests and responses.
//  The "suspected" state allows the upper-layer-driver to
//  request that the miniport ping away at slots zero and
//  two to detect the presence of a new enclosure.  The miniport
//  uses this information to control which slots it probes
//  and how often it retries.  It always reports the actual
//  status of the loop.
//

typedef enum CM_SLOT_E
{
    CM_SLOT_UNKNOWN,        // unknown condition (power-up)
    CM_SLOT_EMPTY,          // not present (selection timeout)
    CM_SLOT_FAILED,         // present but login failed
    CM_SLOT_NORMAL,         // present and login succeeded
    CM_SLOT_SUSPECTED,      // suspected (discover DAE)
} CM_SLOT_T;

#define CM_SLOT_SHOULD_RETRY(status)    ((CM_SLOT_NORMAL==status) ||    \
                                         (CM_SLOT_SUSPECTED==status))

typedef struct _CM_LOOP_COMMAND
{
    ULONG           CommandType;
    ULONG           IoCtlStatus;
    ULONG           DiscoverLoopStatus;
    ULONG           LoopFailStatus;
    ULONG           Port;
    CM_SLOT_T       LoopMap[MAX_DISK_DEVICES];
} CM_LOOP_COMMAND, *PCM_LOOP_COMMAND;


typedef struct _RESET_DEVICE_CMD
{
    UCHAR           Mode;       // Must be 0...For future expansion.
    UCHAR           PathId;
    UCHAR           TargetId;
    UCHAR           Lun;
} RESET_DEVICE_CMD, *PRESET_DEVICE_CMD;


typedef struct _SET_ASYNC_CALLBACK 
{ 
    PVOID           Context;                    // Cw,Mr
    PASYNC_CALLBACK AsyncCallback;              // Cw,Mr
    PVOID           CallbackHandle;             // Cr,Mw
} SET_ASYNC_CALLBACK, *PSET_ASYNC_CALLBACK; 


typedef struct _UNSET_ASYNC_CALLBACK 
{ 
    PVOID           CallbackHandle;             // Cw,Mr
} UNSET_ASYNC_CALLBACK, *PUNSET_ASYNC_CALLBACK; 

// Definitions of CLAR_GET_EXTENSION.SupportBits
//
#define DRV_SUPPORTS_MIXED_SG               0x00000001L
#define DRV_SUPPORTS_RECOVERY_CONFIG        0x00000002L
#define DRV_SUPPORTS_DETAILED_LOGGING       0x00000004L

typedef struct _CLAR_GET_EXTENSIONS
{
    ULONG           SupportBits;                // Cr,Mw
    ULONG           MaxTransferLength;          // Cr,Mw
    USHORT          MaxSGEntries;               // Cr,Mw
    USHORT          SGLength;                   // Cr,Mw

} CLAR_GET_EXTENSIONS, *PCLAR_GET_EXTENSIONS;

typedef struct _CLAR_RECOVERY_CONFIG
{
    USHORT          RetriesBeforeAbort;         // Cw,Mr
    USHORT          AbortsBeforeReset;          // Cw,Mr
    USHORT          RecoveryBits;               // Cw,Mr

} CLAR_RECOVERY_CONFIG, *PCLAR_RECOVERY_CONFIG;

// SRB SrbFlags definitions
#define SRB_FLAGS_FLARE_CMD         0x40000000  // SrbFlags bit.

// NTBE_SRB SrbFlags definitions
#define SRB_FLAGS_PHYS_SGL          0x20000000  // sgl has physical addresses.
#define SRB_FLAGS_USE_EXTENDED_LUN  0x10000000  // use NTBE_SRB AccessMode
                                                // and Lun fields, not SRB's
#define SRB_FLAGS_SKIP_SECTOR_OVERHEAD_BYTES    0x08000000
                                        // miniport skips metadata in SGL


#pragma pack(4)
typedef struct _NTBE_SGL
{
    ULONG        count;
    PHYS_ADDR    address;
} NTBE_SGL, *PNTBE_SGL;
#pragma pack()

#endif  //_ICDMINI_H_
