/************************************************************************
 *
 *  Copyright (c) 2011 EMC Corporation.
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
 *
 ************************************************************************/

#ifndef __TestControlTctlDefs__h__
#define __TestControlTctlDefs__h__


//++
// File Name:
//      TestControlTctlDefs.h
//
// Contents:
//      This file defines the test control operations for the "Tctl" feature
//      itself.  Several operations are defined, partly as examples and partly
//      to assist in diagnosis and debugging of the test control mechanism.
//
//      See services/TestControl/TestControlOverview.txt for an overview of
//      the Test Control mechanism.
//--


#include "TestControlUserDefs.h"
#include "StaticAssert.h"


// The following opcodes must be distinct, greater than 0, and less than 2^16.
#define Tctl_Mark_Op            1
#define Tctl_Tally_Op           2
#define Tctl_Ticks_Op           3


// Tctl_Mark_Op
//
// This operation posts a test control "mark" requests with the given subcode.
// If a mark is exclusive (`(Flags & Tctl_Mark_Exclusive) != 0`), it
// cannot coexist with other marks with the same subcode.

typedef struct {
    TctlHeader                          Header;
    csx_u64_t                              Flags;
    csx_u64_t                              Subcode;
} Tctl_Mark_Buffer;

#define Tctl_Mark_Exclusive     (1 << 0)  // whether mark subcode must be unique


// Tctl_Tally_Op
//
// This operation tallies active test controls in the targeted locus.
//
// An active test control is selected if its four identifiers (driver,
// feature, opcode, and instance) match those of `Selector`.  A pair of
// identifiers matches if they have the same value or if either is zero.
//
// If the `Tctl_Tally_Dump` flag is present, the contents of selected test
// control buffers are appended to the output buffer as long as they fit.
//
// If the `Tctl_Tally_Complete` flag is present, the selected test controls
// are completed with status `Selector.CompletionStatus`.  If the
// `Tctl_Tally_UseInfo` flag is present, the selected test controls are
// completed with information `Selector.CompletionInformation`; otherwise,
// the current information is used.  (The current information depends on
// the test control.  Usually, it is the number of meaningful bytes of
// the output buffer.)

typedef struct {
    TctlHeader                          Header;
    TctlHeader                          Selector;
    csx_u64_t                              Flags;
    csx_u64_t                              Reserved;
    csx_u64_t                              FoundItems;
    csx_u64_t                              FoundBytes;
    csx_u64_t                              SelectedItems;
    csx_u64_t                              SelectedBytes;
    csx_u64_t                              DumpedItems;
    csx_u64_t                              DumpedBytes;
    // Concatenation of dumps of individual test control buffers may follow.
} Tctl_Tally_Buffer;

STATIC_ASSERT(size_of_Tctl_Tally_Buffer_is_a_multiple_of_TctlBufferAlignment,
              sizeof(Tctl_Tally_Buffer) % TctlBufferAlignment == 0)

#define Tctl_Tally_Dump         (1 << 0)  // Dump selected IOCTL buffers
#define Tctl_Tally_Complete     (1 << 1)  // Complete selected IOCTLs
#define Tctl_Tally_UseInfo      (1 << 2)  // Use CompletionInformation when completing

// Each dump of an individual test control IOCTL buffer is preceded by a
// `TctlBufferDumpHeader` and followed by `PaddedLength - ActualLength`
// zero bytes.
//
typedef struct {
    csx_u64_t              PaddedLength;
    csx_u64_t              ActualLength;
} TctlBufferDumpHeader;

STATIC_ASSERT(size_of_TctlBufferDumpHeader_is_a_multiple_of_TctlBufferAlignment,
              sizeof(TctlBufferDumpHeader) % TctlBufferAlignment == 0)


// The minimum buffer size for `Tctl_Tally_Op` is `sizeof(Tctl_Tally_Buffer)`.
// When dumping is requested and more space is provided, as many whole test
// control IOCTL buffers are dumped as will fit.


// Tctl_Ticks_Op
//
// This operation returns the results of the `csx_p_get_timestamp_frequency` and
// `csx_p_get_timestamp_value` routines.

typedef struct {
    TctlHeader                          Header;
    csx_u64_t                              Flags;
    csx_u64_t                              Frequency;
    csx_u64_t                              Current;
} Tctl_Ticks_Buffer;

#define Tctl_Ticks_NoFrequency  (1 << 0)  // Skip `csx_p_get_timestamp_frequency`.
#define Tctl_Ticks_NoCurrent    (1 << 1)  // Skip `csx_p_get_timestamp_value`.
// When a call is skipped, its result is returned as 0.


#endif // __TestControlTctlDefs__h__
