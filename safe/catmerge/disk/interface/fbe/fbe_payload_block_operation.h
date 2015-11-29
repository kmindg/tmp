#ifndef FBE_PAYLOAD_BLOCK_OPERATION_H
#define FBE_PAYLOAD_BLOCK_OPERATION_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!**************************************************************************
 * @file fbe_payload_block_operation.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the definitions of the fbe_payload_block_operation_t
 *  structure and the related structures, enums and definitions.
 * 
 *  The @ref fbe_payload_block_operation_t is the payload for the logical
 *  storage extent protocol.  This protocol is used for all logical block
 *  operations.
 *
 * @version
 *   8/2008:  Created. Peter Puhov.
 *
 ***************************************************************************/
 
/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_time.h"
#include "fbe/fbe_payload_sg_descriptor.h"
#include "fbe/fbe_payload_pre_read_descriptor.h"

#include "fbe/fbe_payload_operation_header.h"
#include "fbe/fbe_scsi_interface.h"

/*!*******************************************************************
 * @enum fbe_block_sizes_t
 *********************************************************************
 * @brief This simply enumerates the various block sizes we expect.
 *
 *********************************************************************/
typedef enum fbe_block_sizes_e
{
    FBE_BLOCK_SIZES_INVALID,
    FBE_BLOCK_SIZES_512 = 512,
    FBE_BLOCK_SIZES_520 = 520,
    FBE_BLOCK_SIZES_4096 = 4096,
    FBE_BLOCK_SIZES_4160 = 4160,
    FBE_BLOCK_SIZES_LAST = FBE_U32_MAX,
}
fbe_block_sizes_t;

/* First number is "physical" block size, second value is "logical" block size */
typedef enum fbe_block_edge_geometry_e{
	FBE_BLOCK_EDGE_GEOMETRY_INVALID,
	FBE_BLOCK_EDGE_GEOMETRY_512_512, /* Used by SATA PDO */
	FBE_BLOCK_EDGE_GEOMETRY_512_520,
	FBE_BLOCK_EDGE_GEOMETRY_520_520,
    FBE_BLOCK_EDGE_GEOMETRY_4096_520,
    FBE_BLOCK_EDGE_GEOMETRY_4096_4096,
    FBE_BLOCK_EDGE_GEOMETRY_4160_520,
    FBE_BLOCK_EDGE_GEOMETRY_4160_4160,

	FBE_BLOCK_EDGE_GEOMETRY_LAST
}fbe_block_edge_geometry_t;

typedef fbe_u32_t fbe_optimum_block_size_t;

/*! @defgroup fbe_payload_block_operation Block Transport Payload
 *  
 *  This is the set of definitions that make up the block transport payload.
 *  
 *  @ingroup fbe_block_transport_interfaces
 * @{
 */ 

/*! @enum fbe_payload_block_operation_opcode_t 
 *  
 *  @brief This enum describes the type of block operation.  
 */
typedef enum fbe_payload_block_operation_opcode_e {
    /*! The block operation opcode is not valid.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID,

    /*! A standard read command.  The sg in the fbe_payload_ex_t contains 
     *  the sg list to store this data in. 
     */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,

    /*! A standard write command.  The sg in the fbe_payload_ex_t contains 
     *  the sg list to store this data in.
     *  If this write is not aligned to the optimum block size, then the
     *  pre-read descriptor in the block payload will be used to contain
     *  a reference to the read data needed to perform this write.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, 

    /*! A zero operation will simply perform a write of zeros with a 
     *  valid checksum, this might go through just metadata update before
     *  completion. 
     */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO,

    /*! Check to see if this piece of the object is zeroed. 
     *  If this operation is successful, then the status is set to
     *  @ref FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS and
     *  one of these qualifiers is set:
     *   @ref FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_ZEROED
     *   @ref FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NOT_ZEROED
     */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CHECK_ZEROED,

    /*! A force zero request will simply issues the write same request without 
     *  going through the metadata.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_FORCE_ZERO,

    /*! A background zeroing operation will always use map to decide for the 
     *  zeroing operation.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_BACKGROUND_ZERO,

    /*! The semantics of a write same operation are to take a single block  
     *  of data and replicate it across a range.  In all cases we expect this
     *  operation to be aligned to the optimum block size.
     *  In some cases where the exported block size does not match the imported
     *  block size (like 520 exported and 512 imported), the minimum size we will
     *  be able to write might be the optimum block size. In this case the
     *  minimum we will be able to write is a single optimum block.  In this
     *  case we will use the sg descriptor in the block payload to change the
     *  repeat count to reference a single optimum block's worth of data.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME,

    /*! The semantics of a verify command are to make sure the data in the range
     *  is readable.  Typically the drive will execute this as a SCSI verify
     *  command.  There is no sg list passed in with this operation since no
     *  data needs to be given back to the client, just status.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY,

    /*! This operation will first verify an area and then perform a write  
     * operation.  The semantics are otherwise the same as for write. 
     */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE,

    /*! Write out the given range and then validate that it was written.
     */
	FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY,

    /*! For stacked raid groups like RAID-10 this is a special verify
     *  request to populate the passed buffers with data that was
     *  recovered using the mirror verify algorithm.
     */
	FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WITH_BUFFER,
    
    /*! Rebuild the given area on the given redundant raid group.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_REBUILD,

    /*! This operation returns the identity of the server.  This typically 
     *  includes a model number serial number combination that is guaranteed to
     *  be unique.  The structure that is input and output is the
     *  @ref fbe_block_transport_identify_t.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_IDENTIFY,

    /*! This allows the client to request use of a block size that is different 
     *  from the physical block size.  Currently only the logical drive allows
     *  for negotiation.  The structure that is input and output is the
     *  @ref fbe_block_transport_negotiate_t.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_NEGOTIATE_BLOCK_SIZE,

    /*! This only does a verify with no corrections.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY,

    /*! This does a verify with corrections.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ERROR_VERIFY,

    /*! This does a verify with corrections.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INCOMPLETE_WRITE_VERIFY,

    /*! This does a verify with corrections.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_SYSTEM_VERIFY,

    /*! Opcode for marking rebuild.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_MARK_FOR_REBUILD,

    /*! This does a verify with corrections.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_SPECIFIC_AREA,

    /*! This is a write that is not cached.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED,

    /*! This is a write that is a corrupt data operation.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CORRUPT_DATA,

    /*! This code is used to read in write log slot headers
     */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_HDR_RD,

    /*! This code is used to flush a valid write log slot
     */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_FLUSH,

	/*! This code is used to set the Consume bits in the PVD metadata
	 */
	FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_MARK_CONSUMED,

    /*! This code is used to for user disk zeroing request
	 */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_MARK_DISK_ZERO,

    /*! This code is used to clear NZ and UZ bits and set CD bit
     */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_UNMARK_ZERO,

    /*! Code used to verify if region is valid and if not 
     * invalidate it by writing invalid pattern
     */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_INVALIDATE,

    /*! opcodes to initiate verify for a LUN
     */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_RO_VERIFY,
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_USER_VERIFY,
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_SYSTEM_VERIFY,
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_ERROR_VERIFY,
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_INCOMPLETE_WRITE_VERIFY,

    /* ! opcode to initialize the Metadata information */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INIT_EXTENT_METADATA,

    /*! Opcode is used mark PVD consumed area zero. */ 
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_MARK_CONSUMED_ZERO,


    /* ! Opcode to read with the old key and write with the new key. */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_READ_PAGED,

    /* ! Opcode to zeros with the new key and update the paged. */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE_ZEROS,

    /* ! Opcode to write with the new key and update the paged. */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE,

    /* ! Opcode to zero with the new key and update the paged. */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_ZERO,

    /*! Opcode for striper to tell mirrors to set chkpt. */ 
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_SET_CHKPT,

    /* Instead of performing a zero operation, do a write of zeros. */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_ZEROS,

    /*! This does a verify of an lba and block count unconditionally 
     *  and does not correct any errors it finds. 
     */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY_SPECIFIC_AREA,

    /*! The semantics of an unmap operation are to take a single block  
     *  of data and unmap it across a range.  In all cases we expect this
     *  operation to be aligned to the optimum block size.
     *  In some cases where the exported block size does not match the imported
     *  block size (like 520 exported and 512 imported), the minimum size we will
     *  be able to write might be the optimum block size. In this case the
     *  minimum we will be able to write is a single optimum block.  In this
     *  case we will use the sg descriptor in the block payload to change the
     *  repeat count to reference a single optimum block's worth of data.
     */
    //FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_UNMAP,

    /*! This must always be the last operation. 
     */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_LAST
}
fbe_payload_block_operation_opcode_t;

/*! fbe_payload_block_operation_opcode_is_media_modify  
 *  @brief Returns true if this operation will be changing the media. 
 */
static __inline fbe_bool_t 
fbe_payload_block_operation_opcode_is_media_modify(fbe_payload_block_operation_opcode_t op)
{
    fbe_bool_t b_status = FBE_FALSE;

    switch (op)
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_ZEROS:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE_ZEROS:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_UNMARK_ZERO:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CORRUPT_DATA:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_ZERO:
            b_status = FBE_TRUE;
            break;
        default:
            b_status = FBE_FALSE;
            break;
    };
    return b_status;
}
/*! fbe_payload_block_operation_opcode_requires_sg  
 *  @brief Returns true if this operation will require an sg list be passed in.
 */
static __inline fbe_bool_t 
fbe_payload_block_operation_opcode_requires_sg(fbe_payload_block_operation_opcode_t op)
{
    fbe_bool_t b_status = FBE_FALSE;

    switch (op)
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CORRUPT_DATA:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE:
            b_status = FBE_TRUE;
            break;
        default:
            b_status = FBE_FALSE;
            break;
    };
    return b_status;
}

/*! fbe_payload_block_operation_opcode_is_media_read  
 *  @brief Returns true if this operation will be reading the media. 
 */
static __inline fbe_bool_t
fbe_payload_block_operation_opcode_is_media_read(fbe_payload_block_operation_opcode_t op)
{
    fbe_bool_t b_status = FBE_FALSE;

    switch(op) {
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:
            b_status = FBE_TRUE;
            break;
        default:
            b_status = FBE_FALSE;
    }
    return b_status;
}
/*! fbe_payload_block_is_monitor_opcode  
 *   @brief Returns true if this operation is a monitor operation.
 */ 
static __inline fbe_bool_t
fbe_payload_block_is_monitor_opcode(fbe_payload_block_operation_opcode_t op)
{
    fbe_bool_t b_status = FBE_FALSE;

    switch(op) {
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_BACKGROUND_ZERO:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CHECK_ZEROED:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ERROR_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INCOMPLETE_WRITE_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_SYSTEM_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_FLUSH:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_HDR_RD:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_REBUILD:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_READ_PAGED:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_SPECIFIC_AREA:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY_SPECIFIC_AREA:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_INVALIDATE:
            b_status = FBE_TRUE;
            break;
        default:
            b_status = FBE_FALSE;
    }
    return b_status;
}

/*! @enum fbe_payload_block_operation_flags_t 
 *  
 *  @brief This enum describes the flags that we can specify on a block operation.  
 */
enum fbe_payload_block_operation_flags_e {
    /*! The block operation flags is not valid.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_INVALID,

    /*! Set the FUA - Force unit Access when sending the command to the device. 
     * This causes the device to bypass any onboard cache when performing the command. 
     */
    FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_FORCE_UNIT_ACCESS     = 0x00000001,

    /*! This flag is set by a client to indicate that the checksum check
     *  should be executed for this request.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_CHECK_CHECKSUM        = 0x00000002,

    /*! This flag is set by a client to indicate that the buffer contains
     *  invalidated blocks to be written.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_CORRUPT_CRC           = 0x00000004,

    /*! This flag is set when the request is generated as part of metadata operation.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_METADATA_OP           = 0x00000008,

    /*! This flag is set only for write request to indicate that the write is 
     *  `forced'.  This is used by downstream objects to determine how to
     *  handle metadata.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_FORCED_WRITE          = 0x00000010,

    /*! Tells us that we can actually fail back an I/O if we are congested.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_ALLOW_FAIL_CONGESTION = 0x00000020,

    /*! This is a special I/O that we should not hold up with throttles.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_DO_NOT_QUEUE          = 0x00000040,

    /*! Indicates we are allowed to fail the request for SLF        
     */
    FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_ALLOW_FAIL_NOT_PREFERRED = 0x00000080,

    /*! Indicates we should use the new encryption key.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_USE_NEW_KEY = 0x00000100,

    /*! Indicates we should not stripe lock at the first level.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_NO_SL_THIS_LEVEL = 0x00000200,

    /*! Indicates we should do unmap instead of write same.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_UNMAP = 0x00000400,

    /*! Indicates we should allow read of unmapped regions for zod
     */
    FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_ALLOW_UNMAP_READ = 0x00000800,

    /*! This must always be the last flag. 
     */
    FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_LAST                  = 0x00000800
};
typedef fbe_u32_t fbe_payload_block_operation_flags_t;

/*! @enum fbe_payload_block_operation_status_t 
 *  
 *  @brief These are the supported status values for the block operation.
 *         Positive values indicate a good status and negative values indicate
 *         bad status.
 */
typedef enum fbe_payload_block_operation_status_e {
    /*! This is an invalid status.  We intentionally make the value 0 so that 0 
     * is never a valid status.  
     */
    FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID = 0,

    /*!  Operation completed successfully. 
     * Qualifiers: 
     *   @ref FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_COMPLETE_WITH_REMAP 
     *   @ref FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_REMAP_REQUIRED
     *  If opcode is FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CHECK_ZEROED valid qualifiers are:
     *   @ref FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_ZEROED
     *   @ref FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NOT_ZEROED
     */
    FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS = 1,

    /*! The timeout specified by the user in the packet was exceeded. 
     * This could occur if a device is taking a long time to respond.  Possible 
     * actions could be to fail the command back to the client. 
     */
    FBE_PAYLOAD_BLOCK_OPERATION_STATUS_TIMEOUT = -1, /* 0xff */

    /*! A media error occurred during the operation. 
     * The additional status indicates the status of the media and what type of 
     * action should be taken. 
     *  
     * Qualifiers: 
     *   @ref FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_BAD_BLOCK  
     *   @ref FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST 
     *   @ref FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_NO_REMAP 
     */
    FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR = -2, /* 0xfe */

    /*! The operation to the device failed and could not be retried immediately. 
     * One example of returning this error could be if the device is taking 
     * hardware errors that are not recoverable.  The device is not usable and 
     * the expectation is that the client will not continue using this device. 
     * A Qualifier of CRC ERROR indicates the data provided contained an unexpected
     * CRC error.
     *  
     * Qualifiers: 
     *   @ref FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE 
     *   @ref FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE  
     *   @ref FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CRC_ERROR
     *   @ref FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_WRITE_LOG_ABORTED
     */
    FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED = -3, /* 0xfd */

    /*! The request failed because it was aborted. 
     * The qualifier determines the reason for the abort and what further action 
     * is needed. 
     *  
     * Qualifiers: 
     *   @ref FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED 
     *   @ref FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_OPTIONAL_ABORTED_LEGACY
     *   @ref FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RAIDLIB_ABORTED 
     */
    FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED = -4, /* 0xfc */

    /*!  The request failed because the device is not available. 
     *   This could occur for example if the device is in the middle of a
     *   firmware download. 
     */ 
    FBE_PAYLOAD_BLOCK_OPERATION_STATUS_NOT_READY = -5, /* 0xfb */

    /*! The request failed because the block operation was not formed correctly. 
     * This should be returned in any case where the block operation or anything 
     * it refers to such as the sg list, appears to be invalid. 
     * The qualifier contains a hint of what was wrong with the request. 
     *  
     * Qualifiers: 
     *   @ref FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NULL_SG
     *   @ref FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_SG
     *   @ref FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CAPACITY_EXCEEDED
     *   @ref FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_SIZE
     *   @ref FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNSUPPORTED_OPCODE
     *   @ref FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INSUFFICIENT_RESOURCES
     *   @ref FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR
     *   @ref FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_TRANSFER_SIZE_EXCEEDED
     *   @ref FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MISMATCH
     *   @ref FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_TOO_MANY_DEAD_POSITIONS
     *   @ref FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNALIGNED_REQUEST
     */
    FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST = -6, /* 0xfa */
    
    FBE_PAYLOAD_BLOCK_OPERATION_STATUS_LAST
}
fbe_payload_block_operation_status_t;

/*! @enum fbe_payload_block_operation_qualifier_t 
 *  
 *  @brief These are the supported qualifier values for the block operation.
 *         Positive values are associated with the status
 *         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, and negative values are
 *         associated with negative status values which have a bad status.
 */
typedef enum fbe_payload_block_operation_qualifier_e{

    /*! This is an invalid qualifier.  We intentionally make the value 0 so that
     * 0 is never a valid qualifier. 
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID = 0,

    /* Non error qualifiers have positive values.
     */

    /*! The qualifier is valid, but there is no qualifier.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE = 1,


    /*! This operation was successful, but some blocks were remapped as part of 
     *  this command.  No additional action is needed by the client.
     */ 
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_COMPLETE_WITH_REMAP = 2,

    /*! The operation is successful, but a block required remap. 
     * The caller should schedule a verify of this area (optimum block size) on 
     * the device to correct errors.  This corresponds in Flare terms to a soft 
     * media error, which is a vd status of VD_REQUEST_COMPLETE and a dh_code of 
     * DH_EXT_RECOVERED_REMAP_REQUIRED. 
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_REMAP_REQUIRED = 3, 

    /*! The operation is successful, and the area is already zeroed. 
     *  Used with the FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CHECK_ZEROED. 
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_ZEROED = 4, 

    /*! The operation is successful, and the area is not zeroed. 
     *  Used with the FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CHECK_ZEROED. 
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NOT_ZEROED = 5, 

    /*! The operation is successful and the rekey should continue. 
     *  Used with the FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_READ_PAGED. 
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CONTINUE_REKEY = 6,  

    /*! We found nothing to rekey here, but media_error_lba contains the 
     * next lba to rekey. 
     *  Used with the FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_READ_PAGED. 
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NOTHING_TO_REKEY = 7, 

    /*! The chunk was found to have incomplete write during rekey. 
     * Used with the FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_READ_PAGED. 
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INCOMPLETE_WRITE_FOUND = 8, 

    /* Error qualifier have negative values.
     */

    /*! A media error occurred during the operation and the data 
     * for all or part of this area is lost.  This corresponds in Flare terms to 
     * a hard media error or VD Status VD_REQUEST_MEDIA_ERROR.  The client 
     * should not expect to receive data if this is retried. 
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST = -3,/* 0xfd */

    /*! A media error occurred during the operation and the data 
     * for all or part of this area is lost.  This type of media error cannot be 
     * remapped.  This corresponds in Flare terms to a hard media error no remap 
     * OR VD Status VD_REQUEST_MEDIA_ERROR_NO_REMAP.  We would typically see 
     * this type of error during or before proactive sparing where a drive is 
     * starting to fail. 
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_NO_REMAP = -4, /* 0xfc */

    /*! Indicates that it may be possible to retry this command at a later time. 
     *  The retry_wait_msecs field of the block operation payload contains
     *  the number of milliseconds we should wait before retry.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE = -5, /* 0xfb */

    /*! Indicates that a retry of this command to this device is not possible.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE = -6, /* 0xfa */

    /*! The request was aborted because the client aborted the request. 
     * Since the client aborted the request, the client needs to determine what 
     * to do next.  Typically the client will return an error on this command. 
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED  = -7, /* 0xf9 */

    /*! The request of optional priority was aborted because it 
     * was aged by the PDO's request aging mechanism.  This occurs when N 
     * requests of higher priority are started ahead of a given optional 
     * request, where N is the age threshold Packet options parameter of the 
     * request. 
     */ 
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_OPTIONAL_ABORTED_LEGACY  = -8, /* 0xf8 */

    /*! Indicates that a NULL sg list was received.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NULL_SG = -9, /* 0xf7 */

    /*! The sg list does not appear to be formed correctly for this command.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_SG = -10, /* 0xf6 */

    /*! One or more of the blocks referenced in the command is beyond the capacity of the device.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CAPACITY_EXCEEDED = -11, /* 0xf5 */

    /*! The operation requested an invalid block size for this device.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_SIZE = -12, /*0xf4 */

    /*! The operation contained an invalid opcode for this device.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNSUPPORTED_OPCODE = -13, /* 0xf3 */

    /*! The device does not have sufficient resources to process this operation.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INSUFFICIENT_RESOURCES = -14, /* 0xf2 */

    /*! An unexpected error was encountered while processing the operation.  
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR = -15, /* 0xf1 */

    /*! The pre-read descriptor was not valid.  Normally this occurs when 
     * the pre-read descriptor lba, blocks do not cover enough area. 
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_PRE_READ_DESCRIPTOR = -16, /* 0xf0 */

    /*! The pre-read descriptor scatter gather list was not valid.  Normally 
     * this occurs when the scatter gather for the pre-read descriptor is too 
     * small. 
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_PRE_READ_DESCRIPTOR_SG = -17, /* 0xef */

    /*! this qualifier is used with FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED to 
     * specify that the miniport aborted this request (the IO was on the miniport driver 
     * queue but we could not send it in the terminator world it translates to a user 
     * initiating a logout on a port/encl/drive while there is still IO pending for this 
     * drive 
     */
	FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MINIPORT_ABORTED_IO = -18, /* 0xee */

    /*! This status indicates that we failed to get a lock.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_LOCK_FAILED = -19, /* 0xed */

    /*! Indicates that the caller provided data that contained an unexpected CRC
     * error.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CRC_ERROR = -20, /* 0xec */

    /*! Indicates that pvd object gets unaligned zero request from upstream object. 
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNALIGNED_ZERO_REQUEST = -21, /* 0xeb */

    /*! The transfer size is larger than the device supports.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_TRANSFER_SIZE_EXCEEDED = -22,  /* 0xea */

    /*! Indicates that the attempt to get a slot for the journal write log was aborted.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_WRITE_LOG_ABORTED = -23,  /* 0xe9 */

	/*! No more entries in the journal log 
	 */
	FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_WRITE_LOG_EMPTY = -24, /* 0xe8 */

	/*! No valid write log entry 
	 */
	FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_ENTRY_NOT_VALID = -25, /* 0xe7 */

	/*! Entry mismatch 
	 */
	FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MISMATCH = -26, /* 0xe6 */

    /*! Raw mirror mismatch 
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RAW_MIRROR_MISMATCH = -27, /* 0xe5 */

    /*! The request was aborted because the client aborted the request. 
     * In this case the server did not even start I/Os to the disk 
     * prior to the abort getting seen. 
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_WR_NOT_STARTED_CLIENT_ABORTED  = -28, /* 0xe4 */

    /*! A media error occurred during the operation and we will continue to read 
      * regardless the bad block.
      */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_BAD_BLOCK = -29,/* 0xe3*/

    /*! Raid library aborted the request and RG object can retry the Iots. 
      */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RAIDLIB_ABORTED = -30,/* 0xe2*/

    /*! Unexpected number of blocks for this type of operation.
      */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_COUNT = -31,/* 0xe1*/

    /*! The request is not allowed since full stripe writes are preferred.
      */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NOT_FULL_STRIPE = -32,/* 0xe0*/

    /*! The request is not allowed since this is not the preferred path.
      */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NOT_PREFERRED = -33,/* 0xdf*/

    /*! Informs client with a failure that we are not able to process I/O 
     * due to a congested condition. 
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CONGESTED = -34,/* 0xde*/

    /*! There are too many dead positions to execute the request.  This is 
     *  unexpected since the request should not have been sent to the library.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_TOO_MANY_DEAD_POSITIONS = -35,/* 0xdd*/

    /*! Informs client on a successful completion that we are still congested.
      */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_STILL_CONGESTED = -36,/* 0xdc*/

    /*! Indicates that the request was not aligned to the physical block size. 
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNALIGNED_REQUEST = -37, /* 0xdb */

    /*! Miniport found that encryption is not enabled. 
     * Can occur either at init time or during I/O. 
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_ENCRYPTION_NOT_ENABLED = -38, /* 0xda */

    /*! Handle is not valid due to bogus, stale handle or incorrect bus handle.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_BAD_KEY_HANDLE = -39, /* 0xd9*/

    /*! Error in unwrapping the DEK.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_KEY_WRAP_ERROR = -40, /* 0xd8 */

    /*! This should always be the last qualifier.
     */
    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_LAST  /* 0xe1 */
}
fbe_payload_block_operation_qualifier_t;

/*! @struct fbe_payload_block_operation_t 
 *
 *  @brief This is the payload which is used for the logical block transport.
 */
typedef struct fbe_payload_block_operation_s{
    /*! This must be first in the payload since the functions which manipulate 
     * the payload will cast it to a payload operation header. 
     */
    fbe_payload_operation_header_t			operation_header;

    /*! This is the operation that this payload represents, e.g. read, write, etc. */
	fbe_payload_block_operation_opcode_t	block_opcode;

    /*! A set of flags that allow us to specify things like FUA, etc. */
	fbe_payload_block_operation_flags_t	    block_flags;

    /*! This is our storage to remember edge on which we started this block operation. */
    struct fbe_block_edge_s *               block_edge_p;

    /*! This is the logical block address of this operation. */
	fbe_lba_t								lba;

    /*! This is the number of blocks represented by this operation. */
    fbe_block_count_t						block_count;

    /*! This is the size in bytes of the block size the client expects to be operating on. */
    fbe_block_size_t						block_size;

    /*! This is the size in blocks which are required in order to perform  
     * "edgeless" I/Os.  Note that the client fetches this information when it 
     * peforms a @ref FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_NEGOTIATE_BLOCK_SIZE 
     * operation. 
     */
    fbe_block_size_t						optimum_block_size;

    /*! This is a pointer to a descriptor of pre-read data. 
     * This data is required for all write operations which are not 
     * aligned to the optimum block size. 
     * Note that only write operations use this descriptor.
     */
	fbe_payload_pre_read_descriptor_t		* pre_read_descriptor;

    /*! This is an output field that is expected to be filled in 
     *  by the server.  The client will examine this field to determine if the
     *  operation was successful or not.
     */
    fbe_payload_block_operation_status_t	status;

    /*! This provides additional information about the status of this operation. 
     *  If there is no additional status, then this field will be set to
     *  FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE by the server.
     */
    fbe_payload_block_operation_qualifier_t status_qualifier;

    /*! This describes the sg list that was passed in in the fbe_payload 
     *  structure, which this fbe_payload_block_operation_t is a child of.  
     */
    fbe_payload_sg_descriptor_t				payload_sg_descriptor;
    /*! This is the amount we decreased the throttle by. */ 
    fbe_u32_t						        throttle_count;
    /*! This is the number of disk operations needed for this operation. */ 
    fbe_u32_t						        io_credits;


}fbe_payload_block_operation_t;


/*! @def FBE_BLOCK_TRANSPORT_CONTROL_IDENTIFY_LENGTH 
 *  
 *  @brief The length of the identify information.
 *         The identify information is a string that uniquely identifies this
 *         device.
 *         The length is 30 since it consists of:
 *  
 *         vendor id: 8 bytes + 1 byte NULL
 *         serial number: 20 bytes + 1 byte NULL
 *  
 *  @see fbe_block_transport_identify_t
 */
#define FBE_BLOCK_TRANSPORT_CONTROL_IDENTIFY_LENGTH 30

/*! @struct fbe_block_transport_identify_t 
 *  
 *  @brief The identity consists of a string that is used to uniquely identify
 *         this device.
 *         This is the output of the block transport opcode of
 *         @ref FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_IDENTIFY
 */
typedef struct fbe_block_transport_identify_s
{
    /*! The information used to identify the device is a simple  
     *  character string with two parts, a vendor identifier and
     *  a serial number.
     *  @see FBE_BLOCK_TRANSPORT_CONTROL_IDENTIFY_LENGTH
     */
	char identify_info[FBE_BLOCK_TRANSPORT_CONTROL_IDENTIFY_LENGTH];
}
fbe_block_transport_identify_t;

/*! @struct fbe_block_transport_negotiate_t
 * 
 *  @brief This structure defines the input and output for the block transport
 *         opcode of
 *         @ref FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_NEGOTIATE_BLOCK_SIZE
 */

typedef struct fbe_block_transport_negotiate_s
{
    /*! The capacity in blocks of the device. 
     *  capacity * block_size = total bytes in the device.
     */
    fbe_lba_t block_count;
    /*! The block size of the device in bytes.
     *  This will be the same as the block size that was negotiated for in most
     *  cases.
     */
    fbe_block_size_t block_size;

    /*! @todo remove this.
     */
    fbe_block_size_t optimum_block_size;

    /*! The size of an I/O in blocks to perform to get edgeless I/Os. 
     *  The block size units in bytes are the above "block size" field. 
     */
    fbe_block_size_t optimum_block_alignment;

    /*! Lowest level block size. Typically the size of blocks on the device.
     */
    fbe_block_size_t physical_block_size;

    /*! Lowest level optimal block size. 
     *  This is the number of physical blocks per exported optimal block size. 
     */
    fbe_block_size_t physical_optimum_block_size;

    /*! The requested exported block size.  This is the size in bytes the client 
     *  is negotiating for.
     */
    fbe_block_size_t requested_block_size;

    /*! This is the requested number of blocks per optimum block.  The optimum 
     *  block size is the number of blocks required in order to perform edgeless
     *  I/Os.
     */
    fbe_block_size_t requested_optimum_block_size;
} 
fbe_block_transport_negotiate_t;

/* These are the api functions of the block payload.
 */
fbe_status_t 
fbe_payload_block_build_operation(fbe_payload_block_operation_t * payload_block_operation,
                                  fbe_payload_block_operation_opcode_t block_opcode,
                                  fbe_lba_t lba,
                                  fbe_block_count_t block_count,
                                  fbe_block_size_t block_size,
                                  fbe_block_size_t optimum_block_size,
                                  fbe_payload_pre_read_descriptor_t	* pre_read_descriptor);
fbe_status_t 
fbe_payload_block_build_identify(fbe_payload_block_operation_t * payload_block_operation_p);
fbe_status_t 
fbe_payload_block_build_negotiate_block_size(fbe_payload_block_operation_t * payload_block_operation_p);

/*!**************************************************************
 * @fn fbe_payload_block_init_flags(
 *          fbe_payload_block_operation_t *payload_block_operation,
 *          fbe_payload_block_operation_flags_t flags)
 ****************************************************************
 * @brief This will initialize the block flags to the value specified
 *
 * @param payload_block_operation - Ptr to the block payload struct.
 * @param flag - flag to set
 *
 * @return Always FBE_STATUS_OK
 * 
 * @see fbe_payload_block_operation_t
 * @see fbe_payload_block_operation_t::block_flags
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_payload_block_init_flags(fbe_payload_block_operation_t * payload_block_operation,
                             fbe_payload_block_operation_flags_t flags)
{
	payload_block_operation->block_flags = flags;
	return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_payload_block_set_flag(
 *          fbe_payload_block_operation_t *payload_block_operation,
 *          fbe_payload_block_operation_flags_t flag)
 ****************************************************************
 * @brief This will set a flag in the block flags
 *
 * @param payload_block_operation - Ptr to the block payload struct.
 * @param flag - flag to set
 *
 * @return Always FBE_STATUS_OK
 * 
 * @see fbe_payload_block_operation_t
 * @see fbe_payload_block_operation_t::block_flags
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_payload_block_set_flag(fbe_payload_block_operation_t * payload_block_operation,
						   fbe_payload_block_operation_flags_t flag)
{
	payload_block_operation->block_flags |= flag;
	return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_payload_block_is_flag_set(
 *          fbe_payload_block_operation_t *payload_block_operation,
 *			fbe_payload_block_operation_flags_t flag)
 ****************************************************************
 * @brief This will tell us if a flag is set or not
 *
 * @param payload_block_operation - Ptr to the block payload struct.
 * @param flag - flag to get
 * @return fbe_bool_t - flag is set or not
 * 
 * @see fbe_payload_block_operation_t
 * @see fbe_payload_block_operation_t::block_flags
 *
 ****************************************************************/
static __forceinline fbe_bool_t
fbe_payload_block_is_flag_set(fbe_payload_block_operation_t * payload_block_operation,
						   fbe_payload_block_operation_flags_t flag)
{
    return((payload_block_operation->block_flags & flag) != 0);
}

/*!**************************************************************
 * @fn fbe_payload_block_clear_flag(
 *          fbe_payload_block_operation_t *payload_block_operation,
 *			fbe_payload_block_operation_flags_t flag)
 ****************************************************************
 * @brief This will return the flags of the block
 *
 * @param payload_block_operation - Ptr to the block payload struct.
 * @param flag - flag to clear
 * @return Always FBE_STATUS_OK
 * 
 * @see fbe_payload_block_operation_t
 * @see fbe_payload_block_operation_t::block_flags
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_payload_block_clear_flag(fbe_payload_block_operation_t * payload_block_operation,
						   fbe_payload_block_operation_flags_t flag)
{
    payload_block_operation->block_flags &= ~flag;
	return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_payload_block_set_block_edge(
 *          fbe_payload_block_operation_t * payload_block_operation,
 *			fbe_block_edge_t *  block_edge_p)
 ****************************************************************
 * @brief This will return the flags of the block
 *
 * @param payload_block_operation - Ptr to the block payload struct.
 * @param flag - flag to clear
 * @return Always FBE_STATUS_OK
 * 
 * @see fbe_payload_block_operation_t
 * @see fbe_payload_block_operation_t::block_flags
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_payload_block_set_block_edge (fbe_payload_block_operation_t * payload_block_operation,
                                  struct fbe_block_edge_s * block_edge_p)
{
    payload_block_operation->block_edge_p = block_edge_p;
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_payload_block_clear_flag(
 *          fbe_payload_block_operation_t *payload_block_operation,
 *			ffbe_block_edge_t ** block_edge_p)
 ****************************************************************
 * @brief This will return the flags of the block
 *
 * @param payload_block_operation - Ptr to the block payload struct.
 * @param block_edge_p - pointer to the block edge pointer.
 * @return Always FBE_STATUS_OK
 * 
 * @see fbe_payload_block_operation_t
 * @see fbe_payload_block_operation_t::block_edge_p
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_payload_block_get_block_edge (fbe_payload_block_operation_t * payload_block_operation,
                                  struct fbe_block_edge_s ** block_edge_p)
{
    *block_edge_p = payload_block_operation->block_edge_p;
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_payload_block_get_qualifier(
 *          fbe_payload_block_operation_t *payload_block_operation,
 *          fbe_payload_block_operation_qualifier_t
 *          * payload_block_operation_qualifier)
 ****************************************************************
 * @brief This will return the block operation's qualifier.
 *
 * @param payload_block_operation - Ptr to the block payload struct.
 * @param payload_block_operation_qualifier - Ptr to qualifier to return.
 *
 * @return Always FBE_STATUS_OK
 * 
 * @see fbe_payload_block_operation_t
 * @see fbe_payload_block_operation_t::status_qualifier
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_payload_block_get_qualifier(fbe_payload_block_operation_t * payload_block_operation,
								fbe_payload_block_operation_qualifier_t * payload_block_operation_qualifier)
{
	* payload_block_operation_qualifier = payload_block_operation->status_qualifier;
	return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_payload_block_set_status(
 *          fbe_payload_block_operation_t *payload_block_operation,
            fbe_payload_block_operation_status_t status,
 *          fbe_payload_block_operation_qualifier_t status_qualifier)
 ****************************************************************
 * @brief This sets the block operation payload's status and qualifier.
 *
 * @param payload_block_operation - Ptr to the block payload struct.
 * @param status - Status to set
 * @param status_qualifier - Qualifier to set.
 *
 * @return Always FBE_STATUS_OK
 * 
 * @see fbe_payload_block_operation_t
 * @see fbe_payload_block_operation_t::status
 * @see fbe_payload_block_operation_t::status_qualifier
 *
 ****************************************************************/
static __forceinline fbe_status_t 
fbe_payload_block_set_status(fbe_payload_block_operation_t * payload_block_operation, 
                             fbe_payload_block_operation_status_t status,
                             fbe_payload_block_operation_qualifier_t status_qualifier)
{
	payload_block_operation->status = status;
	payload_block_operation->status_qualifier = status_qualifier;
	return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_payload_block_get_status(
 *          fbe_payload_block_operation_t *payload_block_operation,
 *          fbe_payload_block_operation_status_t * payload_block_operation_status)
 ****************************************************************
 * @brief Gets the status of the block operation payload.
 *
 * @param payload_block_operation - Ptr to the block payload struct.
 * @param payload_block_operation_status - Ptr to status to return.
 *
 * @return Always FBE_STATUS_OK
 * 
 * @see fbe_payload_block_operation_t
 * @see fbe_payload_block_operation_t::status
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_payload_block_get_status(fbe_payload_block_operation_t * payload_block_operation,
							 fbe_payload_block_operation_status_t * payload_block_operation_status)
{
	* payload_block_operation_status = payload_block_operation->status;
	return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_payload_block_copy_flags(
 *          fbe_payload_block_operation_t *source_block_operation,
 *          fbe_payload_block_operation_t *destination_block_operation)
 ****************************************************************
 * @brief Copies the flags of one block operation payload to a second
 *
 * @param source_block_operation - Ptr to a block payload struct.
 * @param destination_block_operation - Ptr to a block payload struct.
 *
 * @return Always FBE_STATUS_OK
 * 
 * @see fbe_payload_block_operation_t
 * @see fbe_payload_block_operation_t::status
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_payload_block_copy_flags(fbe_payload_block_operation_t * source_block_operation,
							 fbe_payload_block_operation_t * destination_block_operation)
{
	destination_block_operation->block_flags = source_block_operation->block_flags;
	return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_payload_block_get_flags(
 *          fbe_payload_block_operation_t *payload_block_operation,
 *          fbe_payload_block_operation_flags_t *payload_block_operation_flags)
 ****************************************************************
 * @brief Gets the flags of the block operation payload.
 *
 * @param payload_block_operation - Ptr to the block payload struct.
 * @param payload_block_operation_status - Ptr to status to return.
 *
 * @return Always FBE_STATUS_OK
 * 
 * @see fbe_payload_block_operation_t
 * @see fbe_payload_block_operation_t::status
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_payload_block_get_flags(fbe_payload_block_operation_t * payload_block_operation,
							 fbe_payload_block_operation_flags_t * payload_block_operation_flags)
{
	* payload_block_operation_flags = payload_block_operation->block_flags;
	return FBE_STATUS_OK;
}


/*!**************************************************************
 * @fn fbe_payload_block_set_pre_read_descriptor(
 *          fbe_payload_block_operation_t *payload_block_operation,
 *          fbe_payload_pre_read_descriptor_t *pre_read_descriptor)
 ****************************************************************
 * @brief Sets the pre-read descriptor ptr of the block operation payload.
 * 
 * @param payload_block_operation - Ptr to the block payload struct.
 * @param pre_read_descriptor - Ptr to the pre_read_descriptor to set.
 *
 * @return Always FBE_STATUS_OK
 * 
 * @see fbe_payload_block_operation_t
 * @see fbe_payload_block_operation_t::pre_read_descriptor
 *
 ****************************************************************/
static __forceinline fbe_status_t 
fbe_payload_block_set_pre_read_descriptor(fbe_payload_block_operation_t * payload_block_operation, 
                                          fbe_payload_pre_read_descriptor_t * pre_read_descriptor)
{
	payload_block_operation->pre_read_descriptor = pre_read_descriptor;
	return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_payload_block_get_pre_read_descriptor(
 *          fbe_payload_block_operation_t *payload_block_operation,
 *          fbe_payload_pre_read_descriptor_t **pre_read_descriptor)
 ****************************************************************
 * @brief Returns the ptr to the pre-read descriptor of the block operation
 * payload.
 *
 * @param payload_block_operation - Ptr to the block payload struct.
 * @param pre_read_descriptor - Ptr to the pre_read_descriptor ptr to return.
 *
 * @return Always FBE_STATUS_OK
 * 
 * @see fbe_payload_block_operation_t 
 * @see fbe_payload_block_operation_t::pre_read_descriptor
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_payload_block_get_pre_read_descriptor(fbe_payload_block_operation_t * payload_block_operation,
										  fbe_payload_pre_read_descriptor_t ** pre_read_descriptor)
{
	* pre_read_descriptor = payload_block_operation->pre_read_descriptor;
	return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_payload_block_get_sg_descriptor(
 *          fbe_payload_block_operation_t *payload_block_operation,
 *          fbe_payload_sg_descriptor_t ** sg_descriptor)
 ****************************************************************
 * @brief Returns the ptr to the sg descriptor ptr of the block operation
 * payload.
 *
 * @param payload_block_operation - Ptr to the block payload struct.
 * @param sg_descriptor - Ptr to the sg descriptor ptr to return.
 *
 * @return Always FBE_STATUS_OK
 * 
 * @see fbe_payload_block_operation_t 
 * @see fbe_payload_block_operation_t::sg_descriptor
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_payload_block_get_sg_descriptor(fbe_payload_block_operation_t * payload_block_operation,
									fbe_payload_sg_descriptor_t ** sg_descriptor)
{
	* sg_descriptor = &payload_block_operation->payload_sg_descriptor;
	return FBE_STATUS_OK;
}


/*!**************************************************************
 * @fn fbe_payload_block_set_optimum_block_size(
 *          fbe_payload_block_operation_t *payload_block_operation,
 *          fbe_block_size_t optimum_block_size)
 ****************************************************************
 * @brief Sets the optimum_block_size of the block operation payload.
 *
 * @param payload_block_operation - Ptr to the block payload struct.
 * @param optimum_block_size - The optimum block size to set in the payload.
 *
 * @return Always FBE_STATUS_OK
 * 
 * @see fbe_payload_block_operation_t 
 * @see fbe_payload_block_operation_t::optimum_block_size
 *
 ****************************************************************/
static __forceinline fbe_status_t 
fbe_payload_block_set_optimum_block_size(fbe_payload_block_operation_t * payload_block_operation, 
                                         fbe_block_size_t optimum_block_size)
{
	payload_block_operation->optimum_block_size = optimum_block_size;
	return FBE_STATUS_OK;
}



/*!**************************************************************
 * @fn fbe_payload_block_get_optimum_block_size(
 *          fbe_payload_block_operation_t *payload_block_operation,
 *          fbe_block_size_t * optimum_block_size)
 ****************************************************************
 * @brief Returns the optimum_block_size of the block
 *        operation payload.
 *
 * @param payload_block_operation - Ptr to the block payload struct.
 * @param optimum_block_size - Ptr to the optimum block size to return.
 *
 * @return Always FBE_STATUS_OK
 * 
 * @see fbe_payload_block_operation_t
 * @see fbe_payload_block_operation_t::optimum_block_size
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_payload_block_get_optimum_block_size(fbe_payload_block_operation_t * payload_block_operation, 
                                         fbe_block_size_t * optimum_block_size)
{
	* optimum_block_size = payload_block_operation->optimum_block_size;
	return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_payload_block_get_opcode(
 *          fbe_payload_block_operation_t *payload_block_operation,
 *          fbe_payload_block_operation_opcode_t	* block_opcode)
 ****************************************************************
 * @brief Returns the opcode of the block operation payload.
 *
 * @param payload_block_operation - Ptr to the block payload struct.
 * @param block_opcode - Ptr to the block opcode to return.
 *
 * @return Always FBE_STATUS_OK
 * 
 * @see fbe_payload_block_operation_t 
 * @see fbe_payload_block_operation_t::opcode
 *
 ****************************************************************/
static __forceinline fbe_status_t 
fbe_payload_block_get_opcode(fbe_payload_block_operation_t * payload_block_operation,
                             fbe_payload_block_operation_opcode_t	* block_opcode)
{
	* block_opcode = payload_block_operation->block_opcode;
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_payload_block_set_opcode(
 *          fbe_payload_block_operation_t *payload_block_operation,
 *          fbe_payload_block_operation_opcode_t block_opcode)
 ****************************************************************
 * @brief Sets the opcode of the block operation payload.
 *
 * @param payload_block_operation - Ptr to the block payload struct.
 * @param block_opcode - The block opcode to set.
 *
 * @return Always FBE_STATUS_OK
 * 
 * @see fbe_payload_block_operation_t 
 * @see fbe_payload_block_operation_t::opcode
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_payload_block_set_opcode(fbe_payload_block_operation_t * payload_block_operation,
                             fbe_payload_block_operation_opcode_t block_opcode)
{
    payload_block_operation->block_opcode = block_opcode;
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_payload_block_get_lba(
 *          fbe_payload_block_operation_t *payload_block_operation,
 *          fbe_lba_t * lba)
 ****************************************************************
 * @brief Returns the lba of the block operation payload.
 *
 * @param payload_block_operation - Ptr to the block payload struct.
 * @param lba - Ptr to the logical block address to return.
 *
 * @return Always FBE_STATUS_OK
 * 
 * @see fbe_payload_block_operation_t 
 * @see fbe_payload_block_operation_t::lba
 *
 ****************************************************************/
static __forceinline fbe_lba_t
fbe_payload_block_get_lba(fbe_payload_block_operation_t * payload_block_operation, fbe_lba_t * lba)
{
	 *lba = payload_block_operation->lba;
	 return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_payload_block_set_lba(
 *          fbe_payload_block_operation_t *payload_block_operation,
 *          fbe_lba_t lba)
 ****************************************************************
 * @brief Sets the lba of the block operation payload.
 *
 * @param payload_block_operation - Ptr to the block payload struct.
 * @param lba - The logical block address to set in the payload.
 *
 * @return Always FBE_STATUS_OK
 * 
 * @see fbe_payload_block_operation_t 
 * @see fbe_payload_block_operation_t::lba
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_payload_block_set_lba(fbe_payload_block_operation_t * payload_block_operation, fbe_lba_t lba)
{
	payload_block_operation->lba = lba;
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_payload_block_get_block_count(
 *          fbe_payload_block_operation_t *payload_block_operation,
 *          fbe_block_count_t * block_count)
 ****************************************************************
 * @brief Returns the block_count of the block operation payload.
 *
 * @param payload_block_operation - Ptr to the block payload struct.
 * @param block_count - Ptr to the block count to return.
 *
 * @return Always FBE_STATUS_OK
 * 
 * @see fbe_payload_block_operation_t 
 * @see fbe_payload_block_operation_t::block_count
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_payload_block_get_block_count(fbe_payload_block_operation_t * payload_block_operation, fbe_block_count_t * block_count)
{
	*block_count = payload_block_operation->block_count;
	return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_payload_block_set_block_count(
 *          fbe_payload_block_operation_t *payload_block_operation,
 *          fbe_block_count_t block_count)
 ****************************************************************
 * @brief Sets the block_count of the block operation payload.
 *
 * @param payload_block_operation - Ptr to the block payload struct.
 * @param block_count - The block count to set in the payload.
 *
 * @return Always FBE_STATUS_OK
 * 
 * @see fbe_payload_block_operation_t 
 * @see fbe_payload_block_operation_t::block_count
 *
 ****************************************************************/
static __forceinline fbe_status_t 
fbe_payload_block_set_block_count(fbe_payload_block_operation_t * payload_block_operation, 
                                       fbe_block_count_t block_count)
{
	payload_block_operation->block_count = block_count;
	return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_payload_block_get_block_size(
 *          fbe_payload_block_operation_t *payload_block_operation,
 *          fbe_block_size_t * block_size)
 ****************************************************************
 * @brief Returns the block_size of the block operation payload.
 *
 * @param payload_block_operation - Ptr to the block payload struct.
 * @param block_size - Ptr to the block size to return.
 *
 * @return Always FBE_STATUS_OK
 * 
 * @see fbe_payload_block_operation_t 
 * @see fbe_payload_block_operation_t::block_size
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_payload_block_get_block_size(fbe_payload_block_operation_t * payload_block_operation, fbe_block_size_t * block_size)
{
	* block_size = payload_block_operation->block_size;
	return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_payload_block_set_block_size(
 *          fbe_payload_block_operation_t *payload_block_operation,
 *          fbe_block_size_t block_size)
 ****************************************************************
 * @brief Sets the block_size of the block operation payload.
 *
 * @param payload_block_operation - Ptr to the block payload struct.
 * @param block_size - The block size to set in the payload.
 *
 * @return Always FBE_STATUS_OK
 * 
 * @see fbe_payload_block_operation_t 
 * @see fbe_payload_block_operation_t::block_size
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_payload_block_set_block_size(fbe_payload_block_operation_t * payload_block_operation,
                                           fbe_block_size_t block_size)
{
	payload_block_operation->block_size = block_size;
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_payload_block_set_throttle_count()
 ****************************************************************
 * @brief Set the cost of this I/O into the playload.
 *        We will need this once the operation completes.
 *
 * @param payload_block_operation - Ptr to the block payload struct.
 * @param throttle_count - The block count to set in the payload.
 *
 * @return Always FBE_STATUS_OK
 * 
 * @see fbe_payload_block_operation_t 
 * @see fbe_payload_block_operation_t::throttle_count
 *
 ****************************************************************/
static __forceinline fbe_status_t 
fbe_payload_block_set_throttle_count(fbe_payload_block_operation_t * payload_block_operation, 
                                     fbe_u32_t throttle_count)
{
	payload_block_operation->throttle_count = throttle_count;
	return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_payload_block_set_io_credits()
 ****************************************************************
 * @brief Set the weight of this I/O into the playload.
 *        We will need this once the operation completes.
 *
 * @param payload_block_operation - Ptr to the block payload struct.
 * @param throttle_count - The block count to set in the payload.
 *
 * @return Always FBE_STATUS_OK
 * 
 * @see fbe_payload_block_operation_t 
 * @see fbe_payload_block_operation_t::throttle_count
 *
 ****************************************************************/
static __forceinline fbe_status_t 
fbe_payload_block_set_io_credits(fbe_payload_block_operation_t * payload_block_operation, 
                                fbe_u32_t io_weight)
{
	payload_block_operation->io_credits = io_weight;
	return FBE_STATUS_OK;
}
static __forceinline fbe_status_t
fbe_payload_block_operation_copy_status(fbe_payload_block_operation_t * source_payload_block_operation,
                                        fbe_payload_block_operation_t * master_payload_block_operation)
{
	master_payload_block_operation->status = source_payload_block_operation->status;
    master_payload_block_operation->status_qualifier = source_payload_block_operation->status_qualifier;
    return FBE_STATUS_OK;
}
#endif /* FBE_PAYLOAD_BLOCK_OPERATION_H */
