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

#ifndef __TestControlUserDefs__h__
#define __TestControlUserDefs__h__

#include "EmcPAL.h"

//++
// File Name:
//      TestControlUserDefs.h
//
// Contents:
//      This file provides the definitions needed to use Test Control
//      operations, that is, to send IOCTL_TEST_CONTROL requests from user
//      space and to interpret the results.
//
//      See services/TestControl/TestControlOverview.txt for an overview of
//      the Test Control mechanism.
//
//      All definitions herein are usable from both C and C++.
//--



#include "StaticAssert.h"
    // STATIC_ASSERT

#include "TestControlMessages.h"
    // error codes



typedef          short      SINT16;
typedef          int        SINT32;
typedef          long long  SINT64;

typedef unsigned short      UINT16;
typedef unsigned int        UINT32;
typedef unsigned long long  csx_u64_t;


// A test control request is conveyed as an IOCTL with code IOCTL_TEST_CONTROL.

#define FILE_DEVICE_TEST_CONTROL     0x8203
    // We hope this is not otherwise used in the CLARiiON context!
    // Set the high bit (of 16) according to Microsoft convention.

#define IOCTL_TEST_CONTROL_FUNCTION   0x800
    // Set the high bit (of 12).  This shouldn't be necessary to avoid conflict
    // with Microsoft-defined values, because our DeviceType is already marked
    // as customer-defined, but doing so seems to be the custom nevertheless.

#define IOCTL_TEST_CONTROL                                              \
    EMCPAL_IOCTL_CTL_CODE( FILE_DEVICE_TEST_CONTROL, IOCTL_TEST_CONTROL_FUNCTION,    \
              EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS )                        \


// Flags governing the behavior of the operation
//
// These flags apply generally to all test control operations.
// (Flags specific to a particular test control operation are accommodated
// with the data specific to that operation.)
//
typedef UINT16          TctlFlags;

#define TEST_CONTROL_USE_WWID           (1 << 0)
    // Use provided WWID to target specified volume or driver locus.


// Driver Identification
//
// See TestControlRegistry.h for the registry of drivers using the
// Test Control mechanism.
// Zero means no specific driver is identified.
//
typedef UINT16          TctlDriverId;


// Feature Identification
//
// See TestControlRegistry.h for the registry of features using the
// Test Control mechanism.
//
typedef UINT16          TctlFeatureId;


// Opcode Identification
//
// Definition of the opcodes for a feature is under control of that feature.
//
typedef UINT16          TctlOpcodeId;


// Identifier for the specific test control IOCTL request instance
//
// Zero means no identifier is specified.
//
typedef csx_u64_t          TctlInstanceId;


// Identification of a storage device
//
// Comprises World Wide Names for the two parts of a K10_WWID
//
#ifdef __cplusplus
struct TctlWWID {
#else
typedef struct _TctlWWID {
#endif
    csx_u64_t      nPort;
    csx_u64_t      node;
#ifdef __cplusplus
    TctlWWID () {
        nPort = 0;
        node  = 0;
    };
    TctlWWID (csx_u64_t p, csx_u64_t n) {
        nPort = p;
        node  = n;
    };
};
#else
} TctlWWID;
#endif


// Status code indicating result of an IRP
//
typedef EMCPAL_STATUS        TctlStatus;


// An operation ID combines a feature ID and an opcode ID.
//
typedef UINT32          TctlOperationId;

#define MakeTctlOperationId(_featureId, _opcodeId) \
    (((TctlOperationId)(_featureId) << 16) | (TctlOperationId)(_opcodeId))


// The combined input/output buffer for a test control operation has a header
// described by this structure, which is followed immediately in memory by data
// specific to the designated operation.
//
typedef struct {

    // This field gives flags for the operation.
    TctlFlags                   Flags;

    // This field specifies the driver that should handle the operation.
    // Zero means any driver will do.
    TctlDriverId                DriverId;

    // The feature and opcode fields together specify the operation to be performed.
    // Each must be nonzero.
    TctlFeatureId               FeatureId;
    TctlOpcodeId                OpcodeId;

    // This field identifies the specific test control IOCTL request instance.
    // When a test control request is made, if this field is zero, a fresh identifier
    // is assigned.
    TctlInstanceId              InstanceId;

    // This field identifies the locus for the operation.
    // Zero means the driver's control device.  Nonzero means the volume with that WWID.
    //
    // If flag TEST_CONTROL_USE_WWID is present, this field is an input field: The
    // contents are used to (help) determine the locus.  If the flag is not present,
    // this field is an output field: The selected locus's WWID is stored here.
    TctlWWID                    Wwid;

    // These two fields are for private use by the test control mechanism.  Hands off!
    csx_u64_t                      _private1;
    csx_u64_t                      _private2;

    // This field returns the completion status for the request.
    TctlStatus                  CompletionStatus;

    // (This field makes padding explicit.)
    UINT32                      _padding1;

    // This field returns the completion information for the request.
    csx_u64_t                      CompletionInformation;

    // Operation-specific information follows immediately.

} TctlHeader;


// The following alias for the above structure is used when we wish to connote
// that it is followed by the rest of the IOCTL input/output buffer (which is
// nearly all the time).
//
typedef TctlHeader TctlBuffer;


// The size of the TctlHeader structure is guaranteed to be a multiple of
// TctlBufferAlignment.  Hence, if a test control IOCTL buffer is allocated
// such that it begins at an address aligned TctlBufferAlignment, the end
// of the header will be similarly aligned.
//
#define TctlBufferAlignment 16

STATIC_ASSERT(size_of_TctlHeader_is_a_multiple_of_TctlBufferAlignment,
              sizeof(TctlHeader) % TctlBufferAlignment == 0)



#endif // __TestControlUserDefs__h__
