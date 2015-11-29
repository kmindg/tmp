/*******************************************************************************
 * Copyright (C) EMC Corporation, 1998 - 2012
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/

/*******************************************************************************
 * XclExportIoctls.h
 *
 * This header file defines constants and structures associated with IOCTLs
 * specific to the XCL (XCopy Lite) component.
 *
 ******************************************************************************/

#if !defined (XCL_EXPORT_IOCTLS_H)
#define XCL_EXPORT_IOCTLS_H

#if !defined (K10_DEFS_H)
#include "k10defs.h"
#endif


//
//  XCL-defined IOCTLs
//

#define XCL_IOCTL_BASE     0x5B00  // Why this value? "5B" looks a little like "SB", and XCL lives in SnapBack

#define XCL_DEFINE_IOCTL(_Index) \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, XCL_IOCTL_BASE + (_Index), EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)
    
//
//  IOCTLs exported to Hostside
//
#define IOCTL_XCL_POPULATE_TOKEN           XCL_DEFINE_IOCTL(0x00)

#define IOCTL_XCL_WRITE_USING_TOKEN        XCL_DEFINE_IOCTL(0x01)

    

/////////////////////////////////////////////////////////////////////////////////
//
//  Types used in SCSI XCopy Lite token-based copying operations
//  (IOCTL_XCL_POPULATE_TOKEN and IOCTL_XCL_WRITE_USING_TOKEN)
//
/////////////////////////////////////////////////////////////////////////////////


//
//  The SGL entry type used in XCopy Lite token-based copying
//
typedef struct _XCL_TOKEN_BLOCK_DESCRIPTOR
{
    ULONGLONG    OffsetInSectors;

    ULONG        LengthInSectors;

} XCL_TOKEN_BLOCK_DESCRIPTOR, *PXCL_TOKEN_BLOCK_DESCRIPTOR;


//
//  The type of offloaded data copying which the token to be created
//  will implement. These values come from the T10/1731-D Revision 35
//  working draft and T10/11-489 r2 proposal, and are expressed in 
//  Little-Endian byte order.
//
typedef enum _XCL_TOKEN_TYPE
{
    //  No source preservation semantics
    XclTokenTypeAccessUponReference    = 0x00000100,

    //  Vendor-specific behavior
    XclTokenTypeDefault                = 0x00008000,

    //  Prefer fewest resources over PIT preservation 
    XclTokenTypeChangeVulnerable       = 0x01008000,

    //  PIT preservation
    XclTokenTypePersistent             = 0x02008000,

    //  Well Known Token
    XclTokenTypeZeroROD                = 0x0100FFFF,

} XCL_TOKEN_TYPE, *PXCL_TOKEN_TYPE;


//
//  An XCopy Lite Token, which is an opaque representation of
//  data to be copied within the array. A token is returned as
//  part of the output from a successful IOCTL_XCL_POPULATE_TOKEN.
//
#define XCL_TOKEN_SIZE_IN_BYTES     512
 
typedef struct _XCL_TOKEN
{
    XCL_TOKEN_TYPE TokenType;

    UCHAR OtherTokenData[XCL_TOKEN_SIZE_IN_BYTES - sizeof(XCL_TOKEN_TYPE)];

} XCL_TOKEN, *PXCL_TOKEN; 


//
//  The input buffer for IOCTL_XCL_POPULATE_TOKEN
//
typedef struct _XCL_POPULATE_TOKEN_IN_BUF
{
    //
    //  A token that has not been accessed in this amount of time can be
    //  invalidated by the array. A value of 0 means that the default value
    //  will be used.
    //
    ULONG InactivityTimeoutInSeconds;

    //
    //  The type of token to be created.
    //
    XCL_TOKEN_TYPE TokenType;

    //
    //  The number of entries in the SGL that follows.
    //
    ULONG NumBlockDescriptors;

    //
    //  The anchor of the SGL that describes the data to be copied.
    //  The caller must allocate enough additional memory beyond
    //  the nominal end of this structure to incorporate all entries
    //  in the SGL.
    //
    XCL_TOKEN_BLOCK_DESCRIPTOR SourceBlockDescriptors[1];

} XCL_POPULATE_TOKEN_IN_BUF, *PXCL_POPULATE_TOKEN_IN_BUF; 


//
// A macro that computes the length of the input buffer for IOCTL_XCL_POPULATE_TOKEN
//
#define XCL_POPULATE_TOKEN_IN_BUF_LENGTH(_NumExtents) \
    (sizeof(XCL_POPULATE_TOKEN_IN_BUF) + \
    (sizeof(XCL_TOKEN_BLOCK_DESCRIPTOR) * ((_NumExtents) - 1)))


//
//  The output buffer for IOCTL_XCL_POPULATE_TOKEN
//
typedef struct _XCL_POPULATE_TOKEN_OUT_BUF
{
    //   
    //  A token representing the data to be copied.
    //
    XCL_TOKEN Token;

} XCL_POPULATE_TOKEN_OUT_BUF, *PXCL_POPULATE_TOKEN_OUT_BUF; 


//
//  The input buffer for IOCTL_XCL_WRITE_USING_TOKEN
//
typedef struct _XCL_WRITE_USING_TOKEN_IN_BUF
{
    //
    //  A representation of the data to be copied, constructed by a 
    //  previous call to IOCTL_XCL_POPULATE_TOKEN.
    //
    XCL_TOKEN Token;

    //
    //  The number of sectors in the source data to be copied (represented
    //  by the token) to be skipped over for this write.
    //
    ULONGLONG OffsetInSectorsIntoSourceData;

    //
    //  The number of entries in the SGL that follows.
    //
    ULONG NumBlockDescriptors;

    //
    //  The anchor of the SGL that describes the destination of the
    //  data to be copied. The caller must allocate enough additional memory
    //  beyond the nominal end of this structure to incorporate all entries
    //  in the SGL.
    //
    XCL_TOKEN_BLOCK_DESCRIPTOR DestinationBlockDescriptors[1];

} XCL_WRITE_USING_TOKEN_IN_BUF, *PXCL_WRITE_USING_TOKEN_IN_BUF;

// A macro that computes the length of the input buffer for write using token

#define XCL_WRITE_USING_TOKEN_IN_BUF_LENGTH(_NumExtents) \
    (sizeof(XCL_WRITE_USING_TOKEN_IN_BUF) + \
    (sizeof(XCL_TOKEN_BLOCK_DESCRIPTOR) * ((_NumExtents) - 1)))

//
//  The output buffer for IOCTL_XCL_WRITE_USING_TOKEN
//
typedef struct _XCL_WRITE_USING_TOKEN_OUT_BUF
{
    //
    //  The number of sectors that were successfully copied (may be 0)
    //  during this call to IOCTL_XCL_WRITE_USING_TOKEN.
    //
    ULONGLONG NumSectorsWritten;

} XCL_WRITE_USING_TOKEN_OUT_BUF, *PXCL_WRITE_USING_TOKEN_OUT_BUF; 


#endif // #if !defined (XCL_EXPORT_IOCTLS_H)

