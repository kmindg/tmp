/*******************************************************************************
 * Copyright (C) EMC Corporation, 1998 - 2015.
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/

/*******************************************************************************
 * flare_ioctls.h
 *
 * This header file defines constants and structures associated with IOCTLs
 * defined by CLARiiON.
 *
 ******************************************************************************/

#if !defined (FLARE_IOCTLS_H)
#define FLARE_IOCTLS_H

#if !defined (FLARE_EXPORT_IOCTLS_H)
#include "flare_export_ioctls.h"
#endif

#if !defined (FLARE_SGL_H)
#include "flare_sgl.h"
#endif

#if !defined (K10_DEFS_H)
#include "k10defs.h"
#endif

# include "InterlockedFIFOList.h"
# include "SinglyLinkedList.h"

//#include <ntddk.h>
#include "EmcPAL.h"
#include "DisparateWriteIoctl.h"

/*
 * The following are the values returned in the IoStatus.Information field of
 * the IRP when the I/O operation was unsuccessful. (In case of success, that
 * field contains the number of bytes transfered.)
 * Each error value is the bitwise OR of an error code and an error information.
 * The former provides a broad category and the latter a refinement. Consequently,
 * not all combinations make sense.
 *
 * Error Code:
 */

#define IRP_TARGET_NOT_READY    0x100
#define IRP_MEDIUM_ERROR        0x200
#define IRP_HARDWARE_ERROR      0x300
#define IRP_ILLEGAL_COMMAND     0x400
#define IRP_ABORTED_COMMAND     0x500

/*
 * Error Information:
 */

#define IRP_LUN_BECOMING_ASSIGNED   0x00
#define IRP_LUN_DOWN                0x01
#define IRP_PARITY_ERROR            0x00
#define IRP_VERIFY_PARITY_ERROR     0x01
#define IRP_HARD_ERROR              0x00
#define IRP_SEL_TIMEOUT             0x01
#define IRP_LUN_DOESNT_EXIST        0x00
#define IRP_LUN_IS_HOT_SPARE        0x01
#define IRP_LUN_NOT_ACCESSIBLE      0x02
#define IRP_INVALID_OPCODE          0x10
#define IRP_INVALID_LENGTH          0x11
#define IRP_LBA_OUT_OF_RANGE        0x12
#define IRP_INVALID_FIELD           0x13
#define IRP_TRY_LATER               0x00
#define IRP_NO_RETRY                0x01

/****************************/
/* CLARiiON defined IOCTLs. */
/****************************/


/* #define IOCTLS moved to export_ioctls for userspace access */


/**********************************************/
/* CLARiiON defined IRP Minor Function codes. */
/**********************************************/

#define FLARE_DCA_READ              0x10
#define FLARE_DCA_WRITE             0x11
#define FLARE_INVALIDATE_CRC        0x12
#define FLARE_SFE_GENERIC_REQUEST   0x13
#define FLARE_FIRMWARE_UPDATE       0x14
#define FLARE_DM_SOURCE             0x15
#define FLARE_DM_DESTINATION        0x16
#define FLARE_SGL_READ              0x17
#define FLARE_SGL_WRITE             0x18
#define FLARE_NOTIFY_EXTENT_CHANGE  0x19

// Standard WDM IO_STACK_LOCATION::Flags for IRP_MJ_READ/WRITE
// #define SL_KEY_SPECIFIED                0x01
// #define SL_OVERRIDE_VERIFY_VOLUME       0x02
// #define SL_WRITE_THROUGH                0x04  // FUA Force unit access
// #define SL_FT_SEQUENTIAL_WRITE          0x08  // DPO unimportant to cache

/*
 * Setting this flag indicate that the request wants to run a low priority.
 */
#define SL_K10_LOW_PRIORITY             0x01

/*
 * COOPERATING VIRTUALIZER IO_STACK_LOCATION::Flags for IRP_MJ_{READ,WRITE}
 * and  IRP_MJ_INTERNAL_DEVICE_CONTROL/FLARE_DCA_{READ_WRITE}
 */
#define SL_K10_COP_VIRTUALIZER       0x10

/*
 * Passed in IO_STACK_LOCATION::Flags for IRP_MJ_{READ,WRITE} to
 * indicate that the Thin LU should look for holes in the LBA
 * region specified; 0x800 was unused.
 */

#define SL_K10_ZERO_DETECT           0x20

/*
 * Passed in IO_STACK_LOCATION::Flags for IRP_MJ_{READ,WRITE}
 * and IRP_MJ_INTERNAL_DEVICE_CONTROL/FLARE_DCA_{READ_WRITE),IOCTL_FLARE_ZERO_FILL.
 * Set by hostside if command is from non active/active behavior initiator.
 * Upper redirector returns an error if this flag is set and the command
 * arrives on the non owner. Needed to implement peer quiesce during trespass.
 */

#define SL_K10_DISALLOW_REDIRECTION  0x40    /* see SL_K10_HIGH_PRIORITY overload of bit*/

// Usable only below the MiddleRedirector, because it shares a bit used by
// the top of the stack.  Setting this flag indicate that the request
// wants better response time than requests that do not have  this bit set.
#define SL_K10_HIGH_PRIORITY         0x40    /* see SL_K10_DISALLOW_REDIRECTION overload of bit */


/*
 * Hint passed in IO_STACK_LOCATION::Flags for
 * IRP_MJ_{READ,WRITE} and  IRP_MJ_INTERNAL_DEVICE_CONTROL/FLARE_DCA_{READ_WRITE}
 * indicating that there are likely to be more accesses to the blocks being transferred.
 */

#define SL_K10_HIGH_REUSE_METADATA   0x80

/*
 * Hint passed into csx_dcall_level_t::level_flags for 
 * IRP_MJ_WRITE and  IRP_MJ_INTERNAL_DEVICE_CONTROL/FLARE_DCA_WRITE
 * indicating that write bandwidth is preferred over cache protection.
 */
#define SL_K10_MAXIMIZE_WRITE_BANDWIDTH_VS_PROTECTION 0x100


/*
 * Hint passed into csx_dcall_level_t::level_flags for 
 * IRP_MJ_{READ,WRITE} and  IRP_MJ_INTERNAL_DEVICE_CONTROL/FLARE_DCA_{READ_WRITE}
 * indicating that the data is unlikely to be accessed again soon and should
 * therefore not be cached by the FAST cache.
 */
#define SL_K10_MCF_UNCACHED          0x200

/*
 * Hint passed into csx_dcall_level_t::level_flags for 
 * IRP_MJ_WRITE and  IRP_MJ_INTERNAL_DEVICE_CONTROL/FLARE_DCA_WRITE
 * indicating that the data will be accessed soon and should remain in
 * the DRAM cache, if possible, until that happens.
 */
#define SL_K10_ACTION_PENDING        0x400

/*****************************/
/* DCA specific definitions. */
/*****************************/
#define MAX_RESOURCE_POOLS          256     //to use in Resource Pool Number for the DCA_TABLE


// Place holder for IO TAGS
// Note that this is a bitmask.
typedef enum io_tag
{
    ZERO_DETECT_CANDIDATE = 0x01,

    //
    // Both flags are produced and consumed inside MLU driver (by Compression)
    // Can be moved out if slots are needed
    //
    IOTAG_MLU_BYPASS_REQUEST = 1 << 1,
    IOTAG_MLU_ACCESS_CREGION = 1 << 2,
    IOTAG_SCHED = 1 << 3,

    // ACT_MGR_TODO: We need to decide whether this tag is for OOB only, and whether we need suffix the name
    // Also we need to decide how many bits we actually need to reserve here.
   IOTAG_ACTIVITY_MGR = 1 << 4,

    // Layer sets this flag to indicate write operation to this LUN (mapped
    // LUN with Compression on) needs to perform inline compression
    IOTAG_INLINE_COMPRESSION_CANDIDATE = 1 << 5,

    // When DM_XCOPY_IO flag is set, DataMover sets this flag to indicate that IO is due to XCopy operation.
    // When this flag is set, IoRedirector does not count that IO against the load balancing count.
    IOTAG_XCOPY_OPERATION = 1 << 6,

    // When this bit is set, MCC will return good status only when all the data is in cache.
    IOTAG_CACHE_READ_HIT_ONLY = 0x1 << 27,

    // Three bits for caching priority hints.  These are only hints to any caching layer
    // in a stack.  These may be used internally or on host I/Os (as a translation
    // of other CDB flags perhaps).

    // Leaving the bits zeroed corresponds to a "normal" priority
    IOTAG_CACHE_PRIORITY_NO_TAG = 0,

    // Mask for macro usage
    IOTAG_CACHE_PRIORITY_HINTS = 0x7 << 28,

    // All hints that need to be explicitly set are non-zero
    IOTAG_CACHE_PRIORITY_NOCACHE = 0x1 << 28,       // Don't bother....
    IOTAG_CACHE_PRIORITY_LOW =  0x2 << 28,          // Hold onto only if you've got room and time
    IOTAG_CACHE_PRIORITY_HIGH = 0x3 << 28,          // Make an extra effort to keep this in cache
    IOTAG_CACHE_PRIORITY_URGENT = 0x4 << 28,        // Really, really try to hold this as long as you can

    // The highest bit in the lower 32 bits is reserved for host I/Os.
    // Client will check this bit to decide it is host I/O.
    // TDD driver will set this flag to all host initiated I/O requests.
    IOTAG_HOST_IO = 1 << 31,
} io_tag;


/* Following line moved to generic_types.h */
//typedef ULONG IO_TAG_TYPE;

/* This macro is to make the PDCA_TABLE pointer to be stored
*  in the Parameters.Others.Argument4 member instead of the current
*  PDCA_TABLE pointer stored in the Parameters.Others.Argument2 member.
*  Parameters.Others.Argument2 member corresponds to the key member of write/read structure.
* The key member will be used to store a tag */

#if defined(_WIN64)
/* SAFEBUG - these need to be fixed.  we shouldn't assume the binary layout of the IRP stucture -- this
 * code requires the Read parameters and Others parameters to line up perfectly */

// 64-bit PDCA in Argument4, io-tag in Argument2(Write.Key)
#define SET_PDCA_TABLE_IN_IRP(p_irp_stack, pdca_table) \
    EmcpalExtIrpStackParamArg4Set(p_irp_stack, (PVOID)(pdca_table))

/* This macro gets the PDCA_TABLE pointer value from the Parameters.Others.Argument4*/

#define GET_PDCA_TABLE_FROM_IRP(p_irp_stack) \
    (EmcpalExtIrpStackParamArg4Get(p_irp_stack))

#define SET_PSGL_INFO_IN_IRP(p_irp_stack, psgl_info) \
    EmcpalExtIrpStackParamArg4Set(p_irp_stack, (PVOID)(psgl_info))

#define GET_PSGL_INFO_FROM_IRP(p_irp_stack) \
    (EmcpalExtIrpStackParamArg4Get(p_irp_stack))

// Macros for setting and getting the IO tags
#define SET_IO_TAG_BITS(p_irp_stack, io_tag) \
    (EmcpalExtIrpStackParamArg2Set(p_irp_stack, (PVOID)(csx_ptrhld_t)(io_tag & 0xffffffff)))

#define GET_IO_TAG(p_irp_stack) \
    ((IO_TAG_TYPE)(((ULONGLONG)(EmcpalExtIrpStackParamArg2Get(p_irp_stack))) & 0xffffffff))

#define RESET_IO_TAG(p_irp_stack) \
    (EmcpalExtIrpStackParamArg2Set(p_irp_stack, 0))

#define IS_HOST_IO(p_irp_stack) \
    ((BOOLEAN)(((ULONGLONG)(EmcpalExtIrpStackParamArg2Get(p_irp_stack))) & IOTAG_HOST_IO))

#define SET_IO_TAG_CACHING_HINT(p_irp_stack, io_tag) \
    (EmcpalExtIrpStackParamArg2Set(p_irp_stack, EmcpalExtIrpStackParamArg2Get(p_irp_stack) | (PVOID)((io_tag & 0xffffffff) & IOTAG_CACHE_PRIORITY_HINTS)))

#define GET_IO_TAG_CACHING_HINT(p_irp_stack) \
    ((IO_TAG_TYPE)(((ULONGLONG)(EmcpalExtIrpStackParamArg2Get(p_irp_stack))) & 0xffffffff) & IOTAG_CACHE_PRIORITY_HINTS)

#else

//32-BIT PDCA in Argument2, cannot use io-tag

#define SET_PDCA_TABLE_IN_IRP(p_irp_stack, pdca_table) \
    EmcpalExtIrpStackParamArg2Set(p_irp_stack, (PVOID)(pdca_table))

#define GET_PDCA_TABLE_FROM_IRP(p_irp_stack) \
    (EmcpalExtIrpStackParamArg2Get(p_irp_stack))

#define SET_PSGL_INFO_IN_IRP(p_irp_stack, psgl_info) \
    EmcpalExtIrpStackParamArg2Set(p_irp_stack, (PVOID)(psgl_info))

#define GET_PSGL_INFO_FROM_IRP(p_irp_stack) \
    (EmcpalExtIrpStackParamArg2Get(p_irp_stack))

//Macros in the following lines are defines as such for 32bit, where we don't support iotags.
#define SET_IO_TAG_BITS(p_irp_stack, io_tag)
#define GET_IO_TAG(p_irp_stack)  (0)
#define RESET_IO_TAG(p_irp_stack)
#define IS_HOST_IO(p_irp_stack) (TRUE)
#define SET_IO_TAG_CACHING_HINT(p_irp_stack, io_tag)
#define GET_IO_TAG_CACHING_HINT(p_irp_stack) (0)

#endif

/*
 *  The FLARE_CAPABILITIES_QUERY structure is used in conjunction with
 *  IOCTL_FLARE_CAPABILITIES_QUERY.  It replaces the legacy FLARE_DCA_NEGOTIATION structure.
 */
typedef struct _FLARE_CAPABILITIES_QUERY
{

    ULONG       support;

} FLARE_CAPABILITIES_QUERY, *PFLARE_CAPABILITIES_QUERY;

/*
 *The FLARE_DCA_NEGOTIATION structure is deprecated.  Use FLARE_CAPABILITIES_QUERY instead.
 */
typedef FLARE_CAPABILITIES_QUERY FLARE_DCA_NEGOTIATION, *PFLARE_DCA_NEGOTIATION;

/*
 * The following values are returned in the support
 * field of a FLARE_CAPABILITIES_QUERY structure.
 */

#define FLARE_DCA_NEITHER_SUPPORTED 0x0
#define FLARE_DCA_READ_SUPPORTED    0x1
#define FLARE_DCA_WRITE_SUPPORTED   0x2
#define FLARE_DCA_BOTH_SUPPORTED    (FLARE_DCA_READ_SUPPORTED | FLARE_DCA_WRITE_SUPPORTED)
#define FLARE_COV_FLAGS_SUPPORTED   0x4
#define FLARE_CAPABILITY_SPARSE_ALLOCATION_SUPPORTED 0x08
#define FLARE_CAPABILITY_ZERO_FILL_SUPPORTED         0x10
#define FLARE_SGL_READ_SUPPORTED    0x20
#define FLARE_SGL_WRITE_SUPPORTED   0x40
#define FLARE_SGL_BOTH_SUPPORTED    (FLARE_SGL_READ_SUPPORTED | FLARE_SGL_WRITE_SUPPORTED)


/*
 * CLARiiON/NT Status codes.
 */

#define STATUS_HOSTSIDE_ABORTED         ((EMCPAL_STATUS)0xE000F001L)
#define STATUS_DISKSIDE_ABORTED         ((EMCPAL_STATUS)0xE000F002L)
#define STATUS_MISCOMPARE               ((EMCPAL_STATUS)0xE000F003L)
#define STATUS_RESERVATION_CONFLICT     ((EMCPAL_STATUS)0xE000F004L)

#define STATUS_PARTIAL_READ_COMPLETE    ((EMCPAL_STATUS)0x6000F005L)

/*
 * Prototype for the DiskSide CallBack function.  The first argument is a
 * status value.  The second and third arguments are copied from the
 * FLARE_DCA_ARG stucture.
 */

typedef VOID (*PDSCB)(EMCPAL_STATUS, PVOID);

/*
 * The FLARE_DCA_ARG structure is passed from the Diskside to the Hostside
 * when part (or all) of a DCA operation is to occur.  This structure
 * specifies which part of the overall operation is to occur (Offset), how
 * large this part of the operation is (TransferBytes), where the data buffer
 * is located (SGList), a DiskSide callback function to call when this part
 * of the operation is complete (DSCAllBack), and an argument to
 * pass to the DiskSide callback function (DSContext1).
 */

typedef struct FLARE_DCA_ARG FLARE_DCA_ARG, *PFLARE_DCA_ARG;

struct FLARE_DCA_ARG
{
    FLARE_DCA_ARG * Link;       // Type-safe Link

    ULONG       Offset;         // Byte offset into the overall request.
    ULONG       TransferBytes;  // Number of data bytes to transfer.
    PSGL        SGList;         // Scatter Gather list.
    PDSCB       DSCallback;     // Called when this part of op. is complete.
    PVOID       DSContext1;     // Second argument to DSCallback().
    UCHAR       SglType;        // bitmap: OR the following bits
#   define SGL_NORMAL                        0
#   define SGL_SKIP_SECTOR_OVERHEAD_BYTES    1      // The SGL contains extra bytes per secctor that should be ignored
#   define SGL_ALL_ZERO                      2      // The data referenced is all zeros.
#   define SGL_VIRTUAL_ADDRESSES             4      // The addresses in each entry are virtual not physical.
#   define SGL_CHECK_PIM_CRC                 8      // XFER to host only
#   define SGL_GEN_PIM_CRC               8      // XFER from host only
#   define SGL_FORMAT_MASK                 0x30      // The format is specified in these bits
#   define SGL_FORMAT_ADJACENT_XOR_512_8   0x00      // The data is laid out so that CRC is contiguous. The SGLE includes the CRC
#   define SGL_FORMAT_SEPARATE_XOR_512_8   0x10      // The data is laid out so that CRC is not contiguous. The SGLE excludes the CRC
#   define SGL_FORMAT_NO_XOR_512           0x20      // The data is laid out so that there is no CRC. The SGLE excludes the CRC

    // The following fields are needed for SGL_FORMAT_SEPARATE_XOR_...
    //
    UCHAR               PageSizeInBlocks;    // The CRC field for offset 0 is this*512 bytes from the begining of the page.
    UCHAR               FirstPageOffset;     // The very first SGL address is for the nth block in a page.
    UCHAR               Spare[1];
};

SLListDefineListType(FLARE_DCA_ARG, ListOfFLARE_DCA_ARG);

# if __cplusplus
SLListDefineInlineCppListTypeFunctions(FLARE_DCA_ARG, ListOfFLARE_DCA_ARG, Link);
#else
SLListDefineInlineListTypeFunctions(FLARE_DCA_ARG, ListOfFLARE_DCA_ARG, Link);
# endif

/*
* The following changes are made to establish a channel for DCA Reads and
* Writes for iScsi Arrays that will get Scatter Gatther pointers that are
* packaged such that they improve iScsi performance. This design also
* ensures that FC Arrays still function with the same performance.
*/

/*
 * Prototype for the HostSide CallBack function.  The first argument is the IRP
 * which was sent to the Diskside driver to initiate the DCA operation.  The
 * second argument is a pointer to a FLARE_DCA_ARG structure (defined below).
 */

typedef VOID (*PDCA_XFER_FUNCTION)(PEMCPAL_IRP, PFLARE_DCA_ARG);

enum AVOID_SKIP_OVERHEAD_BYTES_e {
    ASOB_FALSE = 0,
    ASOB_TRUE  = 1,
    ASOB_DONT_CARE = 2,
};

typedef struct  DCA_TABLE DCA_TABLE, *PDCA_TABLE;

struct DCA_TABLE
{
    PDCA_XFER_FUNCTION      XferFunction;               // function to call to transfer data.
    ULONG                   AvoidSkipOverheadBytes;     // TRUE if overhead bytes need to be skipped in the SGL. DONT_CARE if MCC should decide which layout to return.
    ULONG                   ResourcePoolNumber;         // Pool Resources for AvoidSkipOverheadBytes == TRUE.
    BOOLEAN                 ReportVirtualAddressesInSGL;// If TRUE, generate an SGL containing virtual, rather than physical addresses.
};

typedef  PDCA_TABLE PHSCB;  // Temporary to reduce required edits.

/* Struct defined to reduce the typecasts for DCA operations associated
with the Buffering Changes */
typedef struct DCA_PARAMS DCA_PARAMS, *PDCA_PARAMS;

struct DCA_PARAMS
{
    ULONG       Length;         //Similar to Irp's Parameters.Read.Length
    PHSCB       TransferDesc;   //Similar to Irp's Parameters.Read.Key
    ULONGLONG   ByteOffset;     //Similar to Irp's Parameters.Read.ByteOffset
};

/*
 * Prototype for Data Source's or Sink's completion CallBack function.
 * The first argument is a status value.  The second and third arguments
 * are copied from the FLARE_DM_ARG stucture.
 */
typedef VOID (*PDMHCCB)(EMCPAL_STATUS, PVOID, PVOID);


/* The FLARE_DM_ARG structure is used to pass data between an originator
 * and data sources and sinks to negotiate and initiate an internal data
 * movement.
 */

typedef struct FLARE_DM_ARG FLARE_DM_ARG, *PFLARE_DM_ARG;
struct FLARE_DM_ARG {
    FLARE_DM_ARG * Link;   // Type-safe Link
    ULONG Tag;             // Unique DM Tag
    ULONG Flags;           // Bitmap of DM Flags (see below).
    ULONG Driver;          // Driver indicator (see below).
    PDMHCCB DMHCCallback;  // Handshake completion callback
    PVOID DMContext1;      // 2nd argument to handshake completion callback
    PVOID DMContext2;      // 3rd argument to handshake completion callback
};


// Prototype for an Originator's DM start callback function
typedef VOID (*PDMHSCB)(PEMCPAL_IRP, ULONGLONG, ULONG);

/********************************************************************
 * Layer IDs
 * These IDs are used by client layers to indicate which driver a
 * DM request is coming from as part of the DM tag ID. The driver
 * order specified in the following enum has no basis on the actual
 * driver load order or stacking.
 ********************************************************************/
enum DM_LAYER_ID_e {
    DM_UNKNOWN_ID = 0,
    DM_FLARE_ID = 1,
    DM_AGGREGATE_ID = 2,
    DM_MIGRATION_ID = 3,
    DM_SNAPVIEW_ID = 4,
    DM_CLONES_ID = 5,
    DM_MIRRORVIEW_A_ID = 6,
    DM_MIRRORVIEW_S_ID = 7,
    DM_SANCOPY_ID = 8,
    DM_ROLLBACK_ID = 9,
    DM_MLU_ID = 10,
    DM_FCT_ID = 11,
    DM_HOSTSIDE_ID = 12,
    DM_CPM_ID = 13,
    DM_CBFS_INTERNAL_ID = 14,
    DM_SPCACHE_ID = 15,
    DM_SNAPBACK_ID = 16,
    DM_DVL_ID = 17,
    DM_TDX_ID = 18,
    DM_MAX_ID = 255 
};
typedef enum DM_LAYER_ID_e DM_LAYER_ID;

// KEY_PRIORITY stored in (Key & 0x7)
enum KEY_PRIORITY {
    KEY_PRIORITY_DEFAULT  = 0,
    KEY_PRIORITY_OPTIONAL = 1,
    KEY_PRIORITY_LOW      = 2,
    KEY_PRIORITY_LOW1     = 3,
    KEY_PRIORITY_NORMAL   = 4,
    KEY_PRIORITY_NORMAL1  = 5,
    KEY_PRIORITY_HIGH     = 6,
    KEY_PRIORITY_URGENT   = 7
};

// FLARE_SGL_INFO to describe backend Scatter Gather request
typedef struct flare_sgl_info_s
{
    PSGL  SGList;  // Backend Native Scatter Gather list.
    UCHAR SglType; // bitmap (see FLARE_DCA_ARG above)
    UCHAR FromPeer; // 0 if locally initiated I/O
    UINT_16 Flags; // Flags indicates some action for RAID.
    UCHAR Spare[4];
} FLARE_SGL_INFO, *PFLARE_SGL_INFO;

/*
 * FLARE_SGL_INFO.Flags definition
 */
/*
 * Indicates that checksum should be checked in the data.
 */
#define SGL_FLAG_VALIDATE_DATA          0x1

/*
 * Indicates a write verify opcode should be created.
 */
#define SGL_FLAG_VERIFY_BEFORE_WRITE 0x2

/*
 *FUA Force unit access
 */
#define SGL_FLAG_WRITE_THROUGH                0x04

/*
 * Indicates that it is non cached write and RAID driver should take care of marking
 * LUN as dirty or clean.
 */
#define SGL_FLAG_NON_CACHED_WRITE 0x8

/*
 * Indicates that this I/O is allowed to fail due to redirection
 * with the status EMCPAL_STATUS_LOCAL_DISCONNECT.
 * EMCPAL_STATUS_LOCAL_DISCONNECT status indicates that the I/O was not started, i.e.,
 * that this failure does not require a write VERIFY operation on retry.
 */
#define SGL_FLAG_ALLOW_FAIL_FOR_REDIRECTION 0x10

/*
 * Indicates that this I/O is allowed to fail due to congestion or
 * other transient resource reasons, with the status EMCPAL_STATUS_QUOTA_EXCEEDED.
 * EMCPAL_STATUS_QUOTA_EXCEEDED status indicates that the I/O was not started, i.e.,
 * that this failure does not require a write VERIFY operation on retry.
 */
#define SGL_FLAG_ALLOW_FAIL_FOR_CONGESTION 0x20

/* 
 * Indicates that if I/O fails with one or more invalidated blocks, and the client 
 * has requested to be notified, mark the I/O as failed. 
 */
#define SGL_FLAG_REPORT_VALIDATION_ERROR    0x40

/*
 * Flare callback notification
 */

typedef enum _FLARE_CB_OP
{
    FLARE_CB_GLOBAL_UPDATE_OP,
    FLARE_CB_GLOBAL_REMOVE_OP,
    FLARE_CB_REMOVE_OP,
    FLARE_CB_UPDATE_OP
} FLARE_CB_OP;

typedef enum _FLARE_CB_SUBOP
{
    FLARE_CB_ADD_SUBOP,
    FLARE_CB_REMOVE_SUBOP,
    FLARE_CB_REPLACE_SUBOP
} FLARE_CB_SUBOP;

// Really should be BITS_32 but there's religious disagreement about making generics.h visible
// outside of Flare and I don't want to have that fight.

//typedef BITS_32   FLARE_CB_BITFIELD;
typedef unsigned int    FLARE_CB_BITFIELD;

typedef  void (*FLARE_CALLBACK) (unsigned int       logicalUnit,
                                 FLARE_CB_BITFIELD  eventBitfield);

typedef struct FLARE_ATTACH_CALLBACK
{
    unsigned int        revision;
    FLARE_CB_OP         operation;
    FLARE_CB_SUBOP      subOperation;
    FLARE_CALLBACK      callbackFunction;
    FLARE_CB_BITFIELD   callbackBitfield;
} FLARE_ATTACH_CALLBACK;

#define FLARE_CB_UNBIND 0x00000001

#define FLARE_CB_ALL_EVENTS     FLARE_CB_UNBIND

/*
 * Supported revisions
 */

#define FLARE_ATTACH_CB_REV     FLARE_ATTACH_CB_REV0

#define FLARE_ATTACH_CB_REV0    19990326

//
//  MCC Read And Pin operation types
//  These operations are below the MCC zero-fill bitmap level and MUST not be used above that level.
//
typedef enum  {
    PIN_ACTION_INVALID = 0, // Invalid pin action
    PIN_READ_ONLY = 1,      // Valid in LBA Range, load missing data first.  Data already in MCC retains C/D state
    PIN_AND_MAKE_DIRTY,     // Valid and Dirty in LBA range, load missing data first.
}PIN_ACTION;

//
// MCC Read and Pin Extended operation flags used by CBE to extend Read and Pin functionality.
//
typedef enum  {
    PIN_EXTENDED_NO_OPERATION = 0,            // No extended actions requested
    PIN_EXTENDED_DISABLE_CAPACITY_CHECK = 1,  // Disable LUN capacity check because operation may extend past MCC exported volume size
    PIN_EXTENDED_READ_HIT_ONLY = 2,           // Return good status only when the content is already in cache.
}PIN_EXTENDED_ACTION;

typedef struct {
    OPAQUE_32                     Index32;
    OPAQUE_64                     Index64;
}LUN_INDEX;


typedef struct  {

    LUN_INDEX                        LunIndex;             // Index of requested LUN as previously provided by servicing driver
    LBA_T                            StartLba;              // Starting block to act on.
    ULONG                            BlockCnt;            // Block Length to act on.
    PIN_ACTION                       Action;
    SGL*                             ClientProvidedSgl;             // Null if MCC provided SGL to be used
    ULONG                            ClientProvidedSglMaxEntries;   // 0 if MCC provided SGL to be used, otherwise number of SGL entries in
                                                                    //   in ClientProvidedSgl
    PIN_EXTENDED_ACTION              ExtendedAction;                // Extended Action Flags, 0 is a valid option.
    ULONG                            spare[1];
}IoctlCacheReadAndPinReq;

typedef struct  {

    PVOID                            Opaque0;            // Opaque context information filled in by Cache
    BOOL                             wasDirty;           // Some or all page data was valid and dirty in MCC prior to PIN operation
    PFLARE_SGL_INFO                  pSgl;               // Returned pointer to SGL structure provided by Cache
    PIN_ACTION                       Result;             // Pin action that MCC was able to effect.
    ULONG                            spare[2];

}IoctlCacheReadAndPinResp;

typedef union {
    IoctlCacheReadAndPinReq          Request;
    IoctlCacheReadAndPinResp         Response;
}IoctlCacheReadAndPinInfo ;


typedef enum {
    UNPIN_ACTION_INVALID = 0,          // Invalid unpin action
    UNPIN_HAS_BEEN_WRITTEN = 1,        // Data transfer complete.  Cached data may be marked clean
    UNPIN_AND_FLUSH,                   // MCC persists data.  Data remains marked dirty.  Flush need not be immediate
    UNPIN_FLUSH_VERIFY                 // Data transfer failed such that a write-verify is required.  If the SGL_INFO structure returned
                                       // on the corresponding PIN request did not have the SGL_FLAG_NON_CACHED_WRITE flag set
                                       // the flush by MCC must specify Write-Verify and it must be the next backend access to
                                       // the range from either SP.  If the flag was set process as an UNPIN_AND_FLUSH.
}UNPIN_ACTION ;

struct IoctlCacheUnpinInfo {
    PVOID                      Opaque0;                // Opaque context echoed from corresponding Read and Pin response
    UNPIN_ACTION               Action;                 // Indicator of other actions that may be taken upon unpin
    ULONG                      spare[2];
};


//
// SCN recipients from MLU Driver.
//
typedef enum  {
    STATE_CHANGE_NOTIFICATION_RECIPIENT_MIDDLE_REDIRECTOR = 0,            
    STATE_CHANGE_NOTIFICATION_RECIPIENT_DVL = 1,  
    STATE_CHANGE_NOTIFICATION_RECIPIENT_MAX,           
}STATE_CHANGE_NOTIFICATION_RECIPIENT;



// Used in conjunction with IOCTL_ACKNOWLEDGE_STATE_CHANGE_NOTIFICATION
// dvlSizeInSectors is the new size of the volume in sectors
// startTime is the time when this confirmation was sent
// scnRecipient is the driver which is acknowledging the SCN
typedef struct _ACKNOWLEDGE_SCN_INFO
{
    ULONGLONG dvlSizeInSectors;
    EMCUTIL_SYSTEM_TIME startTime;
    STATE_CHANGE_NOTIFICATION_RECIPIENT scnRecipient;
} ACKNOWLEDGE_SCN_INFO, *PACKNOWLEDGE_SCN_INFO;

#endif /* !defined (FLARE_IOCTLS_H) */
