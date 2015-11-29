// fileheader
//
//   File:          MluSupport.h
//
//   Description:   Contains all the MLU Support Library functions.
//
//
//  Copyright (c) 2007-2012, EMC Corporation.  All rights reserved.
//  Licensed Material -- Property of EMC Corporation.
//
// 

#pragma once

#include "K10MluMessages.h"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT
#include "MluGeneralTypes.h"
#include "EmcPAL.h"

#if defined(__cplusplus)
extern "C"
{
#endif


#if defined(UMODE_ENV)

#define RETURN_STATUS       CSX_MOD_EXPORT HRESULT

#else

#define RETURN_STATUS       CSX_MOD_EXPORT EMCPAL_STATUS

#endif

// We're doing it only to ensure identical structure alignment between 32-bit and 64-bit executables,
//  so that ioctls sent by 32-bit user space programs to the 64-bit MLU driver will work correctly.
#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack(4)
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches - better resolution? */


//=============================================================================
// Name:
//    POOL_LU_ATTRIBUTES
//
// Description:
//    This structure is a collection of Boolean values that can be combined to 
//    describe the attributes of a Pool LU.  Note that not all combinations are valid.
//
// Members:
//    IsSparselyProvisioned: True if and only if the specified LU is thinly provisioned, 
//        meaning that it is possible for writes to the LU to fail due to storage space 
//        being unavailable. 
//
//    IsSnapshot: True if and only if the specified LU is a Snapshot LU of a Base LU from 
//        an MLU storage pool. 
//
//    IsRollingBack: True if and only if the specified LU is a Base LU from an MLU storage 
//        pool and is currently rolling back a Snap.
//
//    IsCompressionEnabled: True if and only if the specified LU has had compression turned 
//        on. Note that this flag will be set as soon as a user requests compression, even 
//        if the actual compression process has not yet begun.
//
//    IsCompressed: True if and only if the specified LU is compressed.  Note that this flag 
//        will remain on even if a user requests decompression, until the decompression 
//        process completes.
//
//    Reserved: Must be zero.
//
//
//=============================================================================
  
typedef struct _POOL_LU_ATTRIBUTES
{
    unsigned int	IsSparselyProvisioned   : 1;
    unsigned int	IsSnapshot              : 1;
    unsigned int	IsRollingBack           : 1;
    unsigned int	IsCompressionTurnedOn   : 1;
    unsigned int	IsCompressed            : 1;
    unsigned int	Reserved                : 27;

} POOL_LU_ATTRIBUTES, *PPOOL_LU_ATTRIBUTES;

//=============================================================================
// Name:
//    POOL_LU_POOL_PROPERTIES
//
// Description:
//    This structure describes various properties of the Pool to which a 
//    Pool LU belongs.
//
// Members:
//    PoolId: The user-visible ID of the Pool to which the LU belongs.  
//        Note that Pool ID numbers can be reused within a given array.
//
//    PoolSizeInSectors: The user-visible size of the Pool in 512-byte 
//        sectors.
//
//    PoolFreeSpaceInSectors: The number of 512-byte sectors in the 
//        Pool that are not already allocated or reserved.
// 
//    PoolConsumedStorageInSectors: The number of 512-byte sectors in the
//        pool that are currently in use (discounting reservations).
//=============================================================================

typedef struct _POOL_LU_POOL_PROPERTIES
{
    ULONG64		PoolId;
    ULONG64		PoolSizeInSectors;
    ULONG64		PoolFreeSpaceInSectors;
    ULONG64     PoolConsumedStorageInSectors;
}
POOL_LU_POOL_PROPERTIES, *PPOOL_LU_POOL_PROPERTIES;

//=============================================================================
// Name:
//    POOL_LU_EXTENDED_PROPERTIES
//
// Description:
//    This structure is used as an input and output buffer by some of the 
//    MLU Support Library functional interfaces.
//
// Members:
//    LuAttributes: The attributes of the LU being queried.
//
//    LuSizeInSectors: The size of the LU in 512-byte sectors. 
//
//    LuConsumedStorageInSectors:  The number of 512-byte sectors consumed by the LU. 
//
//    LuAllocationSP: The SP (0 for SPA, 1 for SPB) on which space allocation 
//        requests for the specified LU will be made by default. 
//
//    LuDataConsumedStorageInSectors:  The actual number of 512-byte data sectors consumed by the LU.
//         this count excludes the metadata storage which is already reported through LuConsumedStorageInSectors. 
//
//    LuPoolProperties: The properties of the storage pool to which the LU belongs
//
//    LuCurrentSP: The SP (0 for SPA, 1 for SPB) on which space allocation 
//        requests for the specified LU is made currently. 
//
//=============================================================================

typedef struct _POOL_LU_EXTENDED_PROPERTIES
{
    POOL_LU_ATTRIBUTES      LuAttributes;
    ULONG64                 LuSizeInSectors;
    ULONG64                 LuConsumedStorageInSectors;
    UCHAR                   LuAllocationSP;
    ULONG64                 LuDataConsumedStorageInSectors;
    POOL_LU_POOL_PROPERTIES LuPoolProperties;
    UCHAR                   LuCurrentSP;
}
POOL_LU_EXTENDED_PROPERTIES, *PPOOL_LU_EXTENDED_PROPERTIES;
    
#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack()
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches - better resolution? */

//=============================================================================
//  Function:
//      MluSLGetLuAttributes
//
//  Description:
//      This function retrieves the MLU attributes of a single Pool LU.
//      The LU is specified by its WWN.
//
//  Arguments:
//      LuId:  WWN of the LU to be examined.
//
//      pAttributes:  Address of an attributes structure that will be 
//          filled in with the MLU attributes of the specified LU.  The 
//          contents of the structure are valid only when the function 
//          returns STATUS_SUCCESS, and are undefined otherwise.
//
//  Return Value:
//      STATUS_SUCCESS:       The attributes were successfully retrieved.
//      MLU_NOT_A_MAPPED_LUN: The specified LU does not belong to a Pool.
//      Any errors returned by the driver
//
//=============================================================================

RETURN_STATUS
MluSLGetLuAttributes(IN K10_WWID LuId, 
                     OUT PPOOL_LU_ATTRIBUTES pAttributes
                     );

//=============================================================================
//  Function:
//      MluSLGetLuAttributesByName
//
//  Description:
//      This function retrieves the MLU attributes of a single Pool LU.
//      The LU is specified by its unique NiceName.
//
//  Arguments:
//      NiceName:  NiceName of the LU to be examined.
//
//      pAttributes:  Address of an attributes structure that will be 
//          filled in with the MLU attributes of the specified LU.  The 
//          contents of the structure are valid only when the function 
//          returns STATUS_SUCCESS, and are undefined otherwise.
//
//  Return Value:
//      STATUS_SUCCESS:       The attributes were successfully retrieved.
//      MLU_NOT_A_MAPPED_LUN: The specified LU does not belong to a Pool.
//      Any errors returned by the driver
//
//=============================================================================

RETURN_STATUS
MluSLGetLuAttributesByName(IN MLU_THIN_LU_NICE_NAME NiceName, 
                           OUT PPOOL_LU_ATTRIBUTES pAttributes
                           );

//=============================================================================
//  Function:
//      MluSLGetLuExtendedProperties
//
//  Description:
//      This function retrives the extended MLU prperties of a single Pool LU.
//
//  Arguments:
//      LuId:  WWN of the LU whose properties are to be retrieved.
//
//      pProperties:  Address of a buffer that will be filled in 
//          with the extended properties of the specified LU. The 
//          contents of the structure are valid only when the function 
//          returns STATUS_SUCCESS, and are undefined otherwise.
//
//  Return Value:
//      STATUS_SUCCESS: The properties were successfully retrieved.
//      MLU_NOT_A_MAPPED_LUN: The specified LU does not belong to a Pool.
// 
//=============================================================================

RETURN_STATUS
MluSLGetLuExtendedProperties ( IN K10_WWID LuId, 
                               OUT PPOOL_LU_EXTENDED_PROPERTIES pProperties
                               );


//=============================================================================
//  Function:
//      MluSLGetLuWorldWideNameByVUOid
//
//  Description:
//      This function retrives the MLU VU WWN of a single Pool LU.
//      The LU is specified by its VU Object Id.
//
//  Arguments:
//      VUOid:  The VU Object ID of the LU to be examined.
//
//      pWorldWideName:  Address of WWN to be filled out
//
//  Return Value:
//      STATUS_SUCCESS: The properties were successfully retrieved.
//      MLU_NOT_A_MAPPED_LUN: The specified LU does not belong to a Pool.
// 
//=============================================================================

RETURN_STATUS 
MluSLGetLuWorldWideNameByVUOid(IN ULONG64 VUOid, 
                               OUT K10_WWID  *pWorldWideName
                               );

//=============================================================================
//  Function:
//      MluSLReserveLUName()
//
//  Description:
//      This function reserves a new NiceName for the specified LU
//      A "reserved" NiceName can ONLY be used on the LU that is reserving it.
//      Any other LU that attempts to set its NiceName to a string that is 
//      "reserved" by any other LU will receive the error "nice name already in use".
//      To "unreserve" a "reserved" NiceName, simply set the "reserved" NiceName
//      to a NULL string.
//
//  Arguments:
//      LuId:  WWN of the LU that will "reserve" the new NiceName for future use.
//
//      NiceNameToReserve:
//          The NiceName that is to be reserved for future use by this LU.
//
//  Return Value:
//      STATUS_SUCCESS: The NiceName was reserved for this LU.
//
//=============================================================================

RETURN_STATUS
MluSLReserveLUName(IN K10_WWID          LuId, 
                   IN K10_LOGICAL_ID    NiceNameToReserve
                   );

//=============================================================================
//  Function:
//      MluSLModifyTDXSessionCountForFS()
//
//  Description:
//      This function modifies the TDX session count for the LD FS
//      associated with the LU.
//
//  Arguments:
//      LuId: WWN of the LU whose FS TDX session count is being modified.
//
//      IsIncrement:
//          This flag determines whether to increment or decrement the
//          count. If the value is set to TRUE then the count will be
//          incremented and otherwise, it will be decremented.
//
//  Return Value:
//      EMCPAL_STATUS_SUCCESS: The TDX session count will be modified
//      appropriately.
//      In case of error an appropriate ERROR code will be returned.
//=============================================================================

RETURN_STATUS
MluSLModifyTDXSessionCountForFS( IN K10_WWID         LuId,
                                 IN BOOLEAN          IsIncrement
                                 );

//=============================================================================

//**************************************************
// Configuration Change Notification (CCN) Interface
//**************************************************

//=============================================================================
// Name:
//    MLUSL_CCN_REVISION
//
// Description:
//    Value passed to MluSLCCNInitialize().  This should be updated as filters
//    are added/removed/shuffled to ensure that the client and MLUSupport have
//    consistent definitions.  The MLU_CCN_FILTERS enum in mluTypes.h defines
//    the filters.
//=============================================================================

#define MLUSL_CCN_REVISION      1


//=============================================================================
//  Function:
//      MluSLCCNInitialize
//
//  Description:
//      This function performs any initialization required in advance of 
//      the client calling other CCN interface functions.  It must be called
//      by the client once before other CCN functions are used.
//
//  Arguments:
//      RevisionID
//          This is the MLUSL_CCN_REVISION value.
//
//  Return Value:
//      S_OK
//          Successfully performed initialization.
//      E_INVALIDARG
//          Client has mismatched MLUSL_CCN_REVISION value.
//      E_NOINTERFACE
//          MLU admin device could not be opened.
//      E_FAIL
//          The function was called more than once.
//=============================================================================

RETURN_STATUS
MluSLCCNInitialize( IN ULONG32 RevisionID );


//=============================================================================
//  Function:
//      MluSLCCNGetID
//
//  Description:
//      This function returns a CCN ID value that can be passed to one
//      or more calls to MluSLCCNRegister().  This value can be used by the
//      client to cancel any outstanding registration requests that were
//      started with it.
//
//      If MluSLCCNInitialize() hasn't been called, this will return an
//      invalid value (MLU_CCN_INVALID_ID).
//
//      The contents of the MLU_CCN_ID should be treated as opaque data by
//      the caller.
//
//  Arguments:
//      None.
//
//  Return Value:
//      MLU_CCN_ID
//=============================================================================

CSX_MOD_EXPORT
MLU_CCN_ID
MluSLCCNGetID(VOID);

//=============================================================================
//  Function:
//      MluSLCCNRegister
//
//  Description:
//      This function registers the client to receive a notification when one
//      or more of the specified filters have been updated.  This is a
//      synchronous operation.  It will block in MluSLCCNRegister() until the
//      register request has been completed (either due to a filter update,
//      request cancellation, or error).  The client can cancel the request
//      by calling MluSLCCNCancel() and specifying the same CCNID.
//
//  Arguments:
//      CCNID
//          A MLU_CCN_ID value returned from MluSLCCNGetID().  This
//          value does not need to be unique - other requests (outstanding
//          or not) can use the same value.  The CCNID gives the client
//          a way of canceling all outstanding requests using the same value.
//      filterBitmap
//          The set of filters the client want to register to receive
//          update notifications for.  This bitmap can't be empty.
//      pFilterDeltaTokens
//          A filter DeltaToken array obtained from a prior poll that
//          indicates the last known DeltaToken values.
//      pTriggeredFilterBitmap
//          Indicates which filters (a subset of filterBitmap) caused the
//          request to be completed.  If an error or cancellation caused the
//          request to be completed, *pTriggeredFilterBitmap will be zero.
//
//  Return Value:
//      S_OK
//          Request was completed due to one or more filter updates.  
//          pTriggeredFilterBitmap indicates what subset of filterBitmap was
//          updated.
//      E_ABORT
//          Request was canceled.
//      E_UNEXPECTED
//          Request failed due to MLU driver shutdown or initialization failure.
//      E_INVALIDARG
//          Invalid (or empty) filterBitmap specified, or invalid CCNID.
//      E_NOINTERFACE
//          The system is not committed to R31 level compatability.
//      E_FAIL
//          Other error.
//=============================================================================

RETURN_STATUS
MluSLCCNRegister(IN  MLU_CCN_ID              CCNID,
                 IN  MLU_CCN_FILTER_BITMAP   filterBitmap,
                 IN  PMLU_DELTA_TOKEN        pFilterDeltaTokens,
                 OUT PMLU_CCN_FILTER_BITMAP  pTriggeredFilterBitmap
                 );

//=============================================================================
//  Function:
//      MluSLCCNCancel
//
//  Description:
//      This function cancels one or more outstanding MluSLCCNRegister()
//      requests.  All outstanding registration requests that were started
//      with a MLU_CCN_ID value that matches CCNID are canceled.
//
//      Due to race conditions, a matching outstanding registration may
//      be completed for reasons other than the cancelation.  The client
//      should expect this.
//
//  Arguments:
//      CCNID
//          A MLU_CCN_ID value returned from MluSLCCNGetID().
//
//  Return Value:
//      S_OK
//          The cancel request was sent to the driver.
//      E_UNEXPECTED
//          Request failed due to MLU driver shutdown or initialization failure.
//      E_FAIL
//          The driver failed the cancel request.
//=============================================================================

RETURN_STATUS
MluSLCCNCancel(IN MLU_CCN_ID  CCNID );

//=============================================================================
//  Function:
//      MluSLCCNGetFilterName
//
//  Description:
//      This is a helper function to convert a filter index value into
//      a string representation of the filter name.  This is just a
//      convenience function provided mostly for tools (e.g. CCNClient)
//      and log messages.
//
//      The caller does not own the memory pointed to by the return value
//      and must not modify or free it.
//
//  Arguments:
//      FilterIndex
//          The defined value for the filter of interest.
//
//  Return Value:
//      A pointer to the name string.
//=============================================================================

CSX_MOD_EXPORT
const CHAR *
MluSLCCNGetFilterName(IN MLU_CCN_FILTERS FilterIndex );

//******************************************************
// End Configuration Change Notification (CCN) Interface
//******************************************************

//=============================================================================
//=============================================================================

//**************************************************
// Fixture Communication Channel (FCC) Interface
//**************************************************

//=============================================================================
//  Function:
//      MluSLFCCOpenChannel
//
//  Description:
//      This function is used to open an FCC channel.
//
//  Arguments:
//      FCCChannelName
//          The FCC channel name to open.
// 
//      pHandle
//          A pointer to a location to store the FCC handle.
//
//  Return Value:
//      S_OK
//          The open was successful and the handle is valid.
//      Other
//          The open was not successful and the handle is not valid.
//=============================================================================

RETURN_STATUS
MluSLFCCOpenChannel(IN MLU_FCC_CHANNEL_NAME     FCCChannelName,
                    OUT PMLU_FCC_HANDLE         pHandle
                    );

//=============================================================================
//  Function:
//      MluSLFCCIssueControl
//
//  Description:
//      This function is used to issue a control request to a previously
//      opened FCC.
//
//  Arguments:
//      Handle
//          A previously open FCC handle.
//      OpCode
//          The Fixture defined OpCode for the request.
//      pInputBuffer
//          A pointer to the input buffer.
//      InputBufferLength
//          The size (in bytes) of the input buffer.
//      pOutputBuffer
//          A pointer to the output buffer.
//      OutputBufferLength
//          The size (in bytes) of the output buffer.
//
//  Return Value:
//      S_OK
//          The close was successful and the handle is no longer valid.
//      Other
//          The close was not successful.
//=============================================================================

RETURN_STATUS
MluSLFCCIssueControl(IN MLU_FCC_HANDLE           Handle,
                     IN ULONG64                  OpCode,
                     IN PVOID                    pInputBuffer,
                     IN ULONG                    InputBufferLength,
                     IN PVOID                    pOutputBuffer,
                     IN PULONG                   pOutputBufferLength
                     );

//=============================================================================
//  Function:
//      MluSLFCCCloseChannel
//
//  Description:
//      This function is used to close an FCC channel.
//
//  Arguments:
//      pHandle
//          A previously open FCC handle.
//
//  Return Value:
//      S_OK
//          The close was successful and the handle is no longer valid.
//      Other
//          The close was not successful.
//=============================================================================

RETURN_STATUS
MluSLFCCCloseChannel(IN MLU_FCC_HANDLE           Handle);

//******************************************************
// End Fixture Communication Channel (FCC) Interface
//******************************************************

//=============================================================================
//=============================================================================
// Name:
//    MLU_SAT_SLICE_INFO
//
// Description:
//    This structure defines what attribute returned for each slice entry in SAT.
//
// Members:
//    FileSystemOid: File system OID that is associated with the Slice, 
//         value (MLU_SLICE_INFO_SLICE_IS_NOT_ALLOCATED) is used if slice is not allicated.
//    
//=============================================================================

typedef struct _MLU_SAT_SLICE_INFO
{

    ULONG64                         FileSystemOid;
    ULONG64                         Position;
    ULONG64                         SATFlags;

} MLU_SAT_SLICE_INFO, *PMLU_SAT_SLICE_INFO;

//=============================================================================
// Name:
//    MLU_SAT_INFO_FOR_FLU
//
// Description:
//    This structure defines what SAT info returned in function MluSLServiceGetFLUSlices.
//
// Members:
//    SliceCount: the number of slices contained in the FLU, 
//                also means how many entry returned in SliceInfo field.
//    
//    BaseOffset: is the offset in bytes from the beginning of the FLU to
//                the header of the first slice.
//
//    HeaderSize: is the size in bytes of each slice's header portion, the
//                beginning portion of the slice that is not user-accessible.
//
//    DataSize: is the size in bytes of each slice's data portion, the
//              following portion of the slice that is user-accessible.
//
//    SPAffinity: SP affinity.
//
//    Padding: explicitly pad with SPAffinity field of 64 bits.
//
//    SliceInfo: entry array for all slices, index of each entry is also slice index.
//    
//=============================================================================

typedef struct _MLU_SAT_INFO_FOR_FLU
{
    ULONG64                         SliceCount;
    ULONG64                         BaseOffset;
    ULONG64                         HeaderSize;
    ULONG64                         DataSize;

    LONG                            SPAffinity;     // see MLU_SP_OWNERSHIP
    LONG                            Padding;        // explicity pad with above field of 64 bits
 
    ULONG32                         DiskType;
    ULONG64                         DeviceTierDescriptor;
    
    MLU_SAT_SLICE_INFO              SliceInfo[1];

} MLU_SAT_INFO_FOR_FLU, *PMLU_SAT_INFO_FOR_FLU;

//=============================================================================
//  Function:
//      MluSLServiceGetFLUSlices
//
//  Description:
//      Obtail slice allocation table of a given FLU.
//
//  Arguments:
//      ThinPoolID
//          Pool Object ID.
//          Can be MLU_INVALID_OID, means caller does not know which pool the FLU is in.
// 
//      FluOID
//          FLU Object ID.
//
//      SliceEntryCount
//          means how much slice entry pre-allocated in SatInfo->SliceInfo[],
//          caller should allocate enough memory to accommodate all Slice entry
//          that will return in SatInfo.
//          If this number is less than actual Slice count in the FLU, the call
//          will fail with E_NOT_SUFFICIENT_BUFFER, and actual Slice count will be
//          returned in SatInfo->SliceCount field.
//
//      SatInfo
//          Pre-allocated buffer to save SAT info.
//
//  Return Value:
//
//      S_OK
//          Successful get SAT info of the FLU.
//
//      E_NOT_SUFFICIENT_BUFFER
//          Slice count is more than SliceEntryCount, output buffer is too small.
//
//      E_OUTOFMEMORY
//          Allocate memory failed, system is out of memory.
//
//      Other
//          Not successful, detailed error code is returned.
//
//=============================================================================

RETURN_STATUS
MluSLServiceGetFLUSlices(IN  ULONG64                   ThinPoolID,
                         IN  ULONG64                   FluOID,
                         IN  ULONG                     SliceEntryCount,
                         OUT PMLU_SAT_INFO_FOR_FLU     SatInfo
                         );


// We're doing it only to ensure identical structure alignment between 32-bit and 64-bit executables,
//  so that ioctls sent by 32-bit user space programs to the 64-bit MLU driver will work correctly.
#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack(4)
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches - better resolution? */

typedef enum
{
    GET_CHUNK_INVALID_OP_TYPE = -1,
    GET_CHUNK_SIZE = 1, 
    GET_OPTIMAL_ALLOCATED_CHUNK_SIZE, 
    GET_OPTIMAL_UNSHARED_CHUNK_SIZE, 
    GET_NUMBER_OF_UNSHARED_CHUNKS,  
    DESCRIBE_ALLOCATED_CHUNKS_MAP,
    DESCRIBE_UNSHARED_CHUNKS_MAP,
    GET_CHUNK_MAX_OP_TYPE

} MLU_SL_DESCRIBE_CHUNKS_OP_TYPE, *PMLU_SL_DESCRIBE_CHUNKS_OP_TYPE;

#define MLU_CBFS_FILESYSTEM_BLOCK_IN_SECTORS 0x10                    // CBFS specifies 8k FSB
#define MLU_SLICE_SIZE          0x80000                              // Currently is defined as 256Meg
#define CHUNK_SIZE_DEFAULT	    0x80                                 // Default to 0x80 sectors, 65536 bytes (64KB)
#define CHUNK_SIZE_MIN	        0x1                                  // This cannot be zero.
#define CHUNK_SIZE_MAX	        MLU_SLICE_SIZE                       // CBFS specifies 8k FSB
#define CHUNK_BITMAP_IN_BYTES	0x1000                               // The default bitmap size 4KB bytes
#define CHUNK_BITMAP_IN_BITS	0x8000                               // The default bitmap size 32K bits
#define END_OF_LU_LENGTH (ULONGLONG) (-1)                            // Describe chunks for remainder
#define MLU_SL_DESCRIBE_CHUNK_IOCTL_TIME_OUT_VALUE 5                 // Describe Chunk IOCTL Timeout default
#define GET_CHUNK_INFO_MAX_TIME_OUT_VALUE 60                         // Get Chunk Info Support Library function Timeout default
#define SECTOR_BYTE_SHIFT 9                                          // Conversion factor between sector and bytes

//=============================================================================
// Name:
//    MLU_SL_DESCRIBE_CHUNKS_REQUEST
//
// Description:
//    This structure is used to describe various input parameters.
//
// Members:
// Oid1:   The MLU (VVol, Snap, VU) Object Id of the resource for which the segment 
//         address space belongs to.
//
// Oid2:   If specified, the MLU (VVol, Snap, VU) Object Id of the resource that 
//         should be compared with the Oid1.
//
// StartOffsetInSectors: The byte offset onto the MLU Logical Unit's media.
//         It must be a multiple of sector size.
// 
// SegmentLengthInSectorss: The maximum length of the segment of the LU starting at 
//         SegmentStartByteOffset for which chunk information may be returned. 
//         A value of CHUNK_END_OF_LU_LENGTH indicates the remainder of the LU. 
//         Otherwise, the value must be nonzero and a multiple of sector size.
//
// ChunkSizeInSectors: The chunk size used for OpType: DESCRIBE_ALLOCATED_CHUNKS_MAP
//         and DESCRIBE_UNSHARED_CHUNKS_MAP. This value if specified, unless it is 
//         CHUNK_END_OF_LU_LENGTH, must be equal to or greater than 
//         SegmentLenghInBytes / ( CHUNK_BITMAP_IN_BYTES * 8). 
//         This value must be a multiple of sector size. If it is not specified, 
//         it is default to CHUNK_SIZE_DEFAULT.
//
// TimeoutValueInSeconds: Client can specify a time out value. When the time
//         expires, this IOCTL should be returned with valid data (bitmap) up 
//         to expiration time.
//
// options: Option for the request enable specific request handling.
//
//=============================================================================
#pragma pack(push, 1)
typedef struct _MLU_SL_DESCRIBE_CHUNKS_REQUEST
{ 
    MLU_SL_DESCRIBE_CHUNKS_OP_TYPE  OpType;
    ULONGLONG                       Oid1;
    ULONGLONG                       Oid2;
    ULONGLONG                       StartOffsetInSectors;
    ULONGLONG                       LengthInSectors;
    ULONGLONG                       ChunkSizeInSectors;
    ULONG   				        TimeoutValueInSeconds;
    ULONG                           Options;

} MLU_SL_DESCRIBE_CHUNKS_REQUEST, *PMLU_SL_DESCRIBE_CHUNKS_REQUEST;
#pragma pack (pop)
//=============================================================================
// Name:
//    MLU_SL_DESCRIBE_CHUNKS_BITMAP_INFO
//
// Description:
//    This structure is used to describe various input parameters.
//
// Members:
// ChunkSizeInSectors: The chunk size used pertaining for the operation. If the 
//       option VARIABLE_CHUNK_SIZE is set, this may be larger or smaller than
//       the value specified the input parameter.
//
// NumberOfScannedChunks: The number of chunks the operation has processed.
//
// BitMap[CHUNK_BITMAP_IN_BYTES]: Bitmap with bits set for chunks that are allocated or 
//       unshared. The number of valid bits is indicated by the 
//       NumberOfScannedChunks.
//
//=============================================================================
  
typedef struct _MLU_SL_DESCRIBE_CHUNKS_BITMAP_INFO
{ 
    ULONGLONG        ChunkSizeInSectors;
    ULONGLONG        NumberOfScannedChunks;
    ULONGLONG        NumberOfUnsharedChunks;
    UCHAR		     BitMap[CHUNK_BITMAP_IN_BYTES];

} MLU_SL_DESCRIBE_CHUNKS_BITMAP_INFO, *PMLU_SL_DESCRIBE_CHUNKS_BITMAP_INFO;


//=============================================================================
//  Function:
//      MluSLGetResourceUnsharedChunkBitMap
//
//  Description:
//      This function gets the unshared chunk bitmap information from MLU.
//
//  Arguments:
//  ChunkRequest: Specify the request attributes.
//  pChunkInfo: Address of a buffer that will be filled in with the chunk 
//  information of the specified resource. The contents of the structure are valid 
//  only when the function returns STATUS_SUCCESS, 
//  otherwise, they are undefined. 
//  Caller must provide the memory storage of the output data structure.
//  
//  Return Value:
//      STATUS_SUCCESS - Successfully returned the attribute of the given lun
//      MLU_DEVICE_ERROR - The function is not able to complete the operation at this time.
//      MLU_DEVICE_NOT_FOUND - The specified Oid is not supported.
//      MLU_INCOMPATIBLE_COMPARSION_RESOURCES - The specifiied Oids (Oid1 and Oid2) are
//                                          not compatible and cannot be compared for
//                                          shared/unshared chunks.
//      MLU_INVALID_TIMEOUT_VALUE - An invalid timeout value was specified.
//      MLU_INVALID_CHUNK_SIZE - An invalid chunk size value was specified.
//      MLU_INVALID_GET_CHUNK_OPTION - An invalid or unsupported option was specified.
//      MLU_INVALID_ADDRESS_RANGE - An invalid range of the resource's address space was
//                              specified.
//
//=============================================================================

RETURN_STATUS 
MluSLGetResourceUnsharedChunkBitMap( 
    MLU_SL_DESCRIBE_CHUNKS_REQUEST              ChunkRequest, 
    OUT PMLU_SL_DESCRIBE_CHUNKS_BITMAP_INFO     pChunkInfo 
);

typedef VOID
(*MLU_SL_GET_RESOURCE_BITMAP_CALLBACK) (
    EMCPAL_STATUS                               Status,
    PVOID                                       pContext,
    PMLU_SL_DESCRIBE_CHUNKS_BITMAP_INFO         pChunkInfo
    );

//=============================================================================
//  Function:
//      MluSLGetResourceUnsharedChunkBitMapAsync()
//
//  Description:
//      This function gets the unshared chunk bitmap information from MLU
//      asynchronously. Caller needs to provide a callback function and context
//      which is called when the scan is complete.
//
//      This method is streamlined so that no MLU "lookups" are performed. The caller
//      provides the pointer to the device object that the IOCTL will be sent to.
//
//  NOTE:
//      The memory used for the pChunkInfo output buffer must come from a location that
//      is acceptable for use in an IO operation that is redirected (i.e. the memory should
//      come from CMM).
//
//  NOTE:
//      The values contained in the input buffer (pChunkRequest) may be modified as part
//      of the operation and should be cleared / reset for any subsequent calls using the
//      same buffer.
//
//  Arguments:
//      pChunkRequest:  Specify the request attributes. Setting the TimeoutValueInSeconds
//                      field to zero indicates that the request should use the maximum
//                      allowable time to complete the operation.
//      pChunkInfo:     Address of a buffer that will be filled in with the chunk
//                      information of the specified resource. The contents of the structure
//                      are valid only when the function returns STATUS_SUCCESS,
//                      otherwise, they are undefined.
//                      Caller must provide the memory storage of the output data structure.
//      pDeviceObject:  caller provided device object pointer to which the IOCTL will be sent.
//      Callback:       callback function to be invoked when scan finishes.
//      pContext:       pointer to caller context that will be used in Callback.
//
//  Return Value:
//      STATUS_PENDING - Successfully issued the IOCTL.
//      MLU_DEVICE_ERROR - The function is not able to complete the operation at this time.
//      MLU_DEVICE_NOT_FOUND - The specified Oid is not supported.
//      MLU_INVALID_TIMEOUT_VALUE - An invalid timeout value was specified.
//      MLU_INVALID_CHUNK_SIZE - An invalid chunk size value was specified.
//      MLU_INVALID_GET_CHUNK_OPTION - An invalid or unsupported option was specified.
//      MLU_INVALID_ADDRESS_RANGE - An invalid range of the resource's address space was specified.
//      MLU_COMMON_ERROR_OUT_OF_MEMORY - An internal memory allocation has failed.
//
//=============================================================================

RETURN_STATUS 
MluSLGetResourceUnsharedChunkBitMapAsync( 
    PMLU_SL_DESCRIBE_CHUNKS_REQUEST             pChunkRequest, 
    PMLU_SL_DESCRIBE_CHUNKS_BITMAP_INFO         pChunkInfo,
    PEMCPAL_DEVICE_OBJECT                       pDeviceObject,
    MLU_SL_GET_RESOURCE_BITMAP_CALLBACK         Callback,
    PVOID                                       pContext 
);


//=============================================================================
//  Function:
//      MluSLGetResourceAllocatededChunkBitMap
//
//  Description:
//      This function gets the allocated chunk bitmap information from MLU.
//
//  Arguments:
//  ChunkRequest: Specify the request attributes.
//  pChunkInfo: Address of a buffer that will be filled in with the chunk 
//  information of the specified resource. The contents of the structure are valid 
//  only when the function returns STATUS_SUCCESS, 
//  otherwise, they are undefined. 
//  Caller must provide the memory storage of the output data structure.
//
//  Return Value:
//      STATUS_SUCCESS - Successfully returned the attribute of the given lun
//      MLU_DEVICE_ERROR - The function is not able to complete the operation at this time.
//      MLU_DEVICE_NOT_FOUND - The specified Oid is not supported.
//      MLU_INCOMPATIBLE_COMPARSION_RESOURCES - The specifiied Oids (Oid1 and Oid2) are
//                                          not compatible and cannot be compared for
//                                          shared/unshared chunks.
//      MLU_INVALID_TIMEOUT_VALUE - An invalid timeout value was specified.
//      MLU_INVALID_CHUNK_SIZE - An invalid chunk size value was specified.
//      MLU_INVALID_GET_CHUNK_OPTION - An invalid or unsupported option was specified.
//      MLU_INVALID_ADDRESS_RANGE - An invalid range of the resource's address space was
//                              specified.
//
//=============================================================================

RETURN_STATUS 
MluSLGetResourceAllocatedChunkBitMap( MLU_SL_DESCRIBE_CHUNKS_REQUEST ChunkRequest, 
                                             OUT PMLU_SL_DESCRIBE_CHUNKS_BITMAP_INFO pChunkInfo );

//=============================================================================
//  Function:
//      MluSLGetResourceNumberOfUnsharedChunks
//
//  Description:
//      This function gets the number of unshared chunks information from MLU.
//
//  Arguments:
//  Oid1: The object Id of the resource to be compared with Oid2 resource
//  Oid2: The object Id of the resource to be compared with Oid1 resource
//  StartOffsetInSectors: The starting address of the range to be compared
//  LengthInSectors: The length of the address space for this operation
//  ChunkSizeInSectors: This value defines the chunk size to be used
//                      by this API when determining the number of unshared chunks within
//                      the specified address range
//  TimeoutValueInSeconds: Time allotted for this operation.
//  pChunkSizeInSectors:  Pointer to a buffer for this API to return the chunk size.  
//                        It is the responsibility of the caller to allocate this buffer.
//  pNumberOfUnsharedChunks: Pointer to the memory address of which the number
//                             of unshared chunks is returned.
//  pNumberOfScannedChunks: Pointer to the memory address of which the number 
//                             of scanned chunks is returned.
//
//  Return Value:
//      STATUS_SUCCESS - Successfully returned the attribute of the given lun
//      MLU_DEVICE_ERROR - The function is not able to complete the operation at this time.
//      MLU_DEVICE_NOT_FOUND - The specified Oid is not supported.
//      MLU_INCOMPATIBLE_COMPARSION_RESOURCES - The specifiied Oids (Oid1 and Oid2) are
//                                          not compatible and cannot be compared for
//                                          shared/unshared chunks.
//      MLU_INVALID_TIMEOUT_VALUE - An invalid timeout value was specified.
//      MLU_INVALID_CHUNK_SIZE - An invalid chunk size value was specified.
//      MLU_INVALID_ADDRESS_RANGE - An invalid range of the resource's address space was
//                              specified.
//
//=============================================================================

RETURN_STATUS 
MluSLGetResourceNumberOfUnsharedChunks( IN ULONGLONG    Oid1,
                                        IN ULONGLONG    Oid2,
                                        IN ULONGLONG    StartOffsetInSectors,
                                        IN ULONGLONG    LengthInSectors,
                                        IN ULONGLONG    ChunkSizeInSectors,
                                        IN ULONG32      TimeoutValueInSeconds,
                                        OUT PULONGLONG  pChunkSizeInSectors,
                                        OUT PULONGLONG  pNumberOfUnsharedChunks,
                                        OUT PULONGLONG  pNumberOfScannedChunks );

//=============================================================================
//  Function:
//      MluSLGetResourceDefaultChunkSize
//
//  Description:
//      This function gets the default native chunk Size from MLU.
//
//  Arguments:
//  pDefaultChunkSizeInSectors: Pointer to the memory address of which the chunk size is returned.
//
//  Return Value:
//      STATUS_SUCCESS - The chunk information pointed by pDefaultChunkSizeInSectors is valid.
//      MLU_DEVICE_ERROR - The function is not able to complete the operation at this time.
//
//=============================================================================

RETURN_STATUS 
MluSLGetResourceDefaultChunkSize( OUT PULONGLONG pDefaultChunkSizeInSectors );

//=============================================================================
//  Function:
//      MluSLGetResourceOptimalAllocatedChunkSize
//
//  Description:
//      This function gets the optimal allocated chunk Size of a region from MLU.
//
//  Arguments:
//  Oid: The object Id of the resource
//  pNativeChunkSize: Pointer to the memory address of which the chunk size is returned.
//
//  Return Value:
//      STATUS_SUCCESS - The chunk information pointed by pNativeChunkSize is valid.
//      MLU_DEVICE_ERROR - The function is not able to complete the operation at this time.
//
//=============================================================================

RETURN_STATUS 
MluSLGetResourceOptimalAllocatedChunkSize( IN ULONGLONG Oid,
                                           IN ULONGLONG StartOffsetInSectors,
                                           IN ULONGLONG LengthInSectors,
                                           IN ULONG32	 TimeOutValueInSeconds,
                                           OUT PULONGLONG pNumberOfScannedSectors,
                                           OUT PULONGLONG pOptimalChunkSizeInSectors);

//=============================================================================
//  Function:
//      MluSLGetResourceOptimalUnsharedChunkSize
//
//  Description:
//      This function gets the optimal unshared chunk Size of a region from MLU.
//
//  Arguments:
//  Oid1: The object Id of the resource to be compared with Oid2 resource
//  Oid2: The object Id of the resource to be compared with Oid1 resource
//  pOptimalUnsharedChunkSizeInSectors: Pointer to the memory address of which the chunk size is returned.
//
//  Return Value:
//      STATUS_SUCCESS - The chunk information pointed by pNativeChunkSize is valid.
//      MLU_DEVICE_ERROR - The function is not able to complete the operation at this time.
//
//=============================================================================

RETURN_STATUS 
MluSLGetResourceOptimalUnsharedChunkSize( IN ULONGLONG Oid1,
                                           IN ULONGLONG Oid2,
                                           IN ULONGLONG StartOffsetInSectors,
                                           IN ULONGLONG LengthInSectors,
                                           IN ULONG32	 TimeOutValueInSeconds,
                                           OUT PULONGLONG pNumberOfScannedSectors,
                                           OUT PULONGLONG pOptimalUnsharedChunkSizeInSectors);

//=============================================================================
//  Function:
//      MluSLGetLuOidByWwn
//
//  Description:
//      This function gets MLU Object ID of a Lun given its WWN.
//
//  Arguments:
//  ChunkRequest: Specify the request type and its attributes.
//
//  pChunkInfo: Address of a buffer that will be filled in with the chunk 
//  information of the specified LU. The contents of the structure are valid 
//  only when the function returns STATUS_SUCCESS, 
//  otherwise, they are undefined. 
//  Caller must provide the memory storage of the output data structure.
//
//  Return Value:
//  STATUS_SUCCESS <96> The chunk information pointed by pChunkInfo is valid.
//  STATUS_IO_DEVICE_ERROR <96> The function is not able to complete the operation
//                           at this time. 
//  STATUS_CANCELLED <96> The request was aborted prior to completion.
//  STATUS_NOT_FOUND <96> The LU_ID(s) was (were) not recognized. 
//  STATUS_NOT_SUPPORTED <96> This request was issued to a FLARE LU.
//  STATUS_NOT_IMPLEMENTED <96> This request was issued to a LU that doesn<92>t 
//                           understand this request.
//  STATUS_VOLUME_DISMOUNTED - This request requires one of the Lu's or their
//                             primary Lu be exported, but none of them are
//                             mounted.
//  STATUS_INVALID_PARAMETER <96> An invalid range of the Lu<92>s address space was 
//                             specified.
//
//=============================================================================

RETURN_STATUS
MluSLGetLuOidByWwn (
                IN K10_WWID LuWwn,
                OUT ULONG64 *pLuOid
                );



#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack()
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches - better resolution? */

#if defined(__cplusplus)
}
#endif

