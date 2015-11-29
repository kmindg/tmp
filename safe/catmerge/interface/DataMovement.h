#ifndef Data_Movement_H
#define Data_Movement_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2005,2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *
 * DESCRIPTION:
 *   This header file contains API definitions for the Data Movement library
 *
 * NOTES:
 *
 * HISTORY:
 *      27-January-2005 A. Spang Created
 *
 ***************************************************************************/



/*************************
 * INCLUDE FILES
 *************************/
#include "k10ntddk.h"
#include "EmcPAL_DriverShell.h"
#include "EmcPAL_List.h"
#include "flare_ioctls.h"
#include "DataMovementStats.h"
#include "ZeroFillLibrary.h"

#ifdef __cplusplus
extern "C" {
#endif
#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
#include "simulation/BvdSim_cmm.h"
#define DM_IRP_START_DEBUG 1
#else
#include "cmm.h"
#endif
/*******************************
 * LITERAL DEFINITIONS
 *******************************/
/*******************************
 * DM IRP State Definitions
 *******************************/
typedef enum {
    DM_IRP_STATE_UNKNOWN = 0,
    DM_IRP_STATE_IDLE,
    DM_IRP_STATE_WAIT_CALLBACK,
    DM_IRP_STATE_START_HANDSHAKE,
    DM_IRP_STATE_WAIT_COPY,
    DM_IRP_STATE_WAIT_DCA_COPY,
    DM_IRP_STATE_CANCELED,
    DM_IRP_STATE_TIMEDOUT,
    DM_IRP_STATE_WAIT_COMPLETE,
    DM_IRP_STATE_IRP_DONE,
    DM_MAX_IRP_STATE            /* This must be the last entry */
} DM_IRP_STATE;

/*******************************
 * DM State Event Definitions
 *******************************/
typedef enum {
    DM_UNKNOWN_EVENT = 0,
    DM_DM_START_EVENT,
    DM_DCA_START_EVENT,
    DM_READ_WRITE_START_EVENT,
    DM_DM_NOT_SUPPORTED_EVENT,
    DM_DCA_CALLBACK_EVENT,
    DM_DM_CALLBACK_EVENT,
    DM_TIMEOUT_EVENT,
    DM_HANDSHAKE_STARTED_EVENT,
    DM_DM_IRP_COMPLETE_EVENT,
    DM_DM_ZERO_DETECT_EVENT,
    DM_DCA_IRP_COMPLETE_EVENT,
    DM_READ_WRITE_COMPLETE_EVENT,
    DM_CANCELED_IRP_COMPLETE_EVENT,
    DM_CANCEL_EVENT,
    DM_COPY_COMPLETE_EVENT,
    DM_REQUEST_STATE_TRANSITION,
    DM_MAX_STATE_EVENT            /* This must be the last entry */
} DM_STATE_EVENT;


/*******************************
 * DM Request State Definitions
 *******************************/
typedef enum {
    DM_REQUEST_STATE_UNKNOWN = 0,
    DM_REQUEST_STATE_INITIALIZED,
    DM_REQUEST_STATE_WAIT_CALLBACK,
    DM_REQUEST_STATE_WAIT_ZERO_COMPLETE,
    DM_REQUEST_STATE_WAIT_DMIRP_COMPLETE,
    DM_REQUEST_STATE_REQUEST_COMPLETE,
    DM_REQUEST_STATE_DCA_START,
    DM_REQUEST_STATE_WAIT_DCA_CALLBACK,
    DM_REQUEST_STATE_WAIT_DCA_COPY,
    DM_REQUEST_STATE_WAIT_DCA_COMPLETE,
    DM_REQUEST_STATE_BUFFER_COPY_START,
    DM_REQUEST_STATE_WAIT_READ,
    DM_REQUEST_STATE_WAIT_WRITE,
    DM_REQUEST_STATE_BUFFER_COPY_COMPLETE,
    DM_REQUEST_STATE_DM_TIMEOUT,
    DM_REQUEST_STATE_CANCELED,
    DM_MAX_REQUEST_STATE            /* This must be the last entry */
} DM_REQUEST_STATE;

/**********************************************
 * Used by debug macros to parse req_state_hist
 **********************************************/
#define DM_EXT_REQ_STATE_MASK 0xF   // Covers full range of DM_REQUEST_STATE
#define DM_EXT_REQ_STATE_BITS 4     // Number of bits in mask

/**********************************************
 * Used by Intermediate Sink-capable drivers to
 * check if they are allowed to hold the IRP.
 **********************************************/
#define DM_IS_SINK_OPERATION_ALLOWED(_irp) \
    !(((DM_request *) _irp->GetDmIoctlContext())->flags & DM_DISALLOW_INTERMEDIATE_SINK)

#define DM_IS_FASTCOPY_OPERATION_ALLOWED(_irp) \
    ((((DM_request *) _irp->GetDmIoctlContext())->flags & DM_SRC_DST_SPARSE_ALLOCATION_SUPPORTED) && \
     !(((DM_request *) _irp->GetDmIoctlContext())->flags & DM_DISALLOW_INTERMEDIATE_SINK))

/********************************************************************
 * The following flags and masks are used to specify various optional
 * capabilities to the DM Library along with some internally used 
 * flags. 
 * The following flags are public and are used by clients to
 * specify optional capabilities:
 *    DM_PREFETCH_ENABLE - Enable prefetching of source device data
 *    DM_ENABLE_BAD_BLK_HANDLING - Enable bad block handling
 *    DM_DISABLE_BUFFERED_COPY - Prevents fallback to Buffered Copy method
 *    DM_DISABLE_DM_IOCTL - Blocks use of DM IOCTLs for move operation
 *    DM_IOBUFFER_ATTACHED - Driver provided a buffer so no need to use
 *                           internal pool for Buffered Copy.
 *    DM_DISABLE_DM_TIMEOUT_RETRY - Disable retrying request on DM
 *                             timeout using buffered copies and instead
 *                             fail the request with STATUS_IO_TIMEOUT.
 *    DM_FORCE_WRITE_THROUGH - Set SL_WRITE_THROUGH on IRPs.
 *    DM_ZERO_FILL_DESTINATION - Zero Fill the destination region (bulk only)
 *
 * The following flags are used internal to the DM library.
 *    DM_TAGID_VALID_FLAG - The tag id in the request is valid.
 *    DM_BAD_BLK_PROCESSING - Bad block handling is in process.
 *    DM_INVALIDATE_BLK - This block is invalid.
 *    DM_BUFFER_COPY_WAIT_HEAD_FLAG - This request was at the head
 *                                    of the buffer wait queue.
 *    DM_DCA_XFER_ABORTED_FLAG - The source and/or destination has
 *                               aborted this request or this DCA
 *                               request as been canceled.
 *    DM_DCA_TIMEOUT_FLAG - This request has been timed out during
 *                          a DCA transfer.
 *    DM_COMPLETION_DONE - This request is about to complete. Ignore 
 *                          any further STATE_TRANSITIONs issued due
 *                          race conditions.
 ********************************************************************/
enum DM_FLAGS {
    DM_PREFETCH_ENABLE = 0x1,
    DM_ENABLE_BAD_BLK_HANDLING = 0x2,
    DM_DISABLE_BUFFERED_COPY = 0x4,
    DM_DISABLE_DM_IOCTL = 0x8,

    DM_IOBUFFER_ATTACHED = 0x10,
    DM_DISABLE_DM_TIMEOUT_RETRY = 0x20,
    DM_FORCE_WRITE_THROUGH = 0x40,
    DM_ZERO_FILL_DESTINATION = 0x80,

    DM_XCOPY_IO = 0x100, // Set by DML user TDD and Snapback to inform UR that Xcopy IO's should not be counted agaist load balancing counter.
    DM_RENAME_OP = 0x200, // OBSOLETE - formerly used by PFDC. Should be reverted bck to Reserved_2 and usage removed from code-base
    DM_IO_FLAG_REUSE_METADATA = 0x400, //Used by passing PriorityUrgentRetain mark in copy path.

    DM_ORIGINATOR_FLAGS_MASK = 0x000007FF,

    DM_TRUNCATED_FOR_ALIGNMENT = 0x800,
    
    DM_BUFFER_COPY_WAIT_HEAD_FLAG = 0x1000,
    DM_DCA_XFER_ABORTED_FLAG = 0x2000,
    DM_DCA_TIMEOUT_FLAG = 0x4000,
    DM_DCA_RESTART_XFER_FLAG = 0x8000,

    DM_COMPLETION_PROCESSING_NEEDED = 0x10000,
    DM_BUFFER_COPY_ZERO_FILL_FLAG = 0x20000,
    DM_DCA_SRC_SKIP_OVERHEAD_BYTES = 0x40000,
    DM_DCA_DST_SKIP_OVERHEAD_BYTES = 0x80000,

    DM_SINGLE_GOOD_BLOCK_COPIED = 0x100000,
    DM_DM_ZERO_FILL_PENDING = 0x200000,
    DM_SRC_DST_SINK_MISMATCH = 0x400000,
    DM_SRC_DST_SPARSE_ALLOCATION_SUPPORTED = 0x800000,

    DM_USING_BUFFERED_COPY = 0x1000000,
    DM_TAGID_VALID_FLAG = 0x2000000,
    DM_BAD_BLK_PROCESSING = 0x4000000,
    DM_INVALIDATE_BLK = 0x8000000,

    DM_DISALLOW_INTERMEDIATE_SINK = 0x10000000,
    DM_CANCEL_LOCK = 0x20000000,
    DM_TIMEOUT_LOCK = 0x40000000,
    DM_COMPLETION_DONE = 0x80000000,

    DM_LIBRARY_INTERNAL_FLAG_MASK = 0xFFFFF800,
};

enum DM_DEBUG_FLAGS {
    DM_DBG_TEST_FORCE_TIMEOUT = 0x01000000,
    DM_DBG_TEST_FORCE_SRC_IO_DEV_ERROR = 0x02000000,
    DM_DBG_TEST_FORCE_DST_IO_DEV_ERROR = 0x04000000,
    DM_DBG_TEST_FORCE_SRC_DISK_CORRUPT_ERROR = 0x08000000,
};

/*********************************
 * STRUCTURE DEFINITIONS
 *********************************/
/********************************************************************
 * Define list and the functions to manipulate the lists of 
 * FLARE DCA structures. 
 ********************************************************************/
SLListDefineListType(FLARE_DCA_ARG, ListOfDCA_ARG);
SLListDefineInlineListTypeFunctions(FLARE_DCA_ARG, ListOfDCA_ARG, Link);

typedef struct DM_request_s DM_request, *PDM_request;
struct DM_request_s {
    EMCPAL_LIST_ENTRY Links;   // Private list for originator
    ULONG tag_id;              // Unique identifier used to hash DM IRPs
    DM_request *flink;         // Doubly linked list link for library
    DM_request *blink;         // Doubly linked list link for library
    PVOID Handle;              // Pointer to DM structure
    PEMCPAL_DEVICE_OBJECT src_dev;    // Pointer to the Source device
    PBasicIoRequest src_irp;   // Source IRP
    PEMCPAL_DEVICE_OBJECT dst_dev;    // Pointer to the destination  device
    PBasicIoRequest dst_irp;   // Destination IRP
    ULONGLONG src_offset;      // Source starting offset
    ULONGLONG curr_src_offset; // Current source offset
    ULONGLONG curr_dst_offset; // Current destination offset
    ULONG xfer_length;         // Source length
    ULONG curr_length;         // Current xfer length
    ULONG dca_src_length;      // DCA source bytes copied
    ULONG dca_dst_length;      // DCA destination bytes copied
    DM_IRP_STATE src_state;    // Source request state
    ULONG src_dca_query;       // Source DCA Query status
    ULONGLONG dst_offset;      // Destination starting offset
    DM_IRP_STATE dst_state;    // Dest. Request state
    ULONG dst_dca_query;       // Dest. DCA Query status
    ListOfDCA_ARG src_args;    // Pointer to Source DM/DCA ARG structures
    ListOfDCA_ARG dst_args;    // Pointer to Dest. DM/DCA ARG structures
    DM_REQUEST_STATE req_state;// Request state
    ULONG flags;               // Bitmap of DM_FLAGS
    ULONG bytes_xfer;          // Actual number of bytes xfered.
    EMCPAL_LARGE_INTEGER time; // System time when request started - EMCPAL_TIME_100NSECS.
    ULONGLONG req_state_hist;  // Tracks the request state transition history.
    VOID (*callback)(VOID *context, DM_request *dm_request);
                               // Originator callback function.
    PVOID context;             // Originator callback context.
    EMCPAL_STATUS status;      // Request status
    PVOID iobuffer;            // Pointer to buffer memory passed by hostside
    ULONG buf_size_in_bytes;   // Buffer size given by hostside
    DM_LAYER_ID sourceLayer;   // Layer ID that is processing source IRP
    ULONG bad_blocks_found;    // Count of bad blocks copied to destination
#ifdef DM_IRP_START_DEBUG
    UINT_16 src_irp_start_cnt;
    UINT_16 dst_irp_start_cnt;
#endif
    ULONG debug_flags;         // Bitmap of DM_DEBUG_FLAGS  
};


typedef struct DM_sgl_s DM_sgl, *PDM_sgl;
struct DM_sgl_s {
    ULONGLONG offset;               // SGL element offset in bytes
    ULONG length;                   // SGL element length in bytes
};

typedef struct DM_bulk_cursor_s DM_bulk_cursor, *PDM_bulk_cursor;
struct DM_bulk_cursor_s {
    ULONG sgl_element;  // Index into the SGL array
    ULONG length;       // Current cursor position within the SGL
};

typedef struct DM_bulk_request_s DM_bulk_request, *PDM_bulk_request;

typedef VOID
(*PDM_bulk_request_callback) (
    IN VOID *context,
    IN DM_bulk_request *dm_bulk_request
    );

typedef VOID
(*PDM_bulk_request_cancel_callback) (
    IN VOID *context,
    IN DM_bulk_request *dm_bulk_request
    );

// The maximum number of bytes in a single DM request
#define DM_MAX_REQUEST_PIECE_TRANSFER_SIZE_IN_BYTES      (512*1024)

// The number of DM requests that are embedded in the DM bulk structure itself.
// These DM requests belong to the DM bulk request to prevent starvation, and
// allow forward progress.
#define DM_NUM_EMBEDDED_BULK_COPY_DM_REQUESTS               4

// The maximum number of DM request slots in a DM bulk request.  We will try 
// to send this many requests if there are sufficient resources.
#define DM_MAX_BULK_COPY_DM_REQUESTS                        128

// The number of Zero Fill requests that are embedded in the DM bulk structure
// itself.
#define DM_NUM_EMBEDDED_BULK_COPY_ZF_REQUESTS               2

typedef struct DM_request_piece_s DM_request_piece, *PDM_request_piece;
struct DM_request_piece_s {
    PDM_request request;                          // Individual DM requests
    ULONG transfer_size_in_bytes;                 // Size of DM request
    ULONG sequence_number;                        // Sequence # of the bulk copy
    UINT_64E time_stamp;                          // Start time of the request
};

typedef struct DM_zf_piece_s DM_zf_piece, *PDM_zf_piece;
struct DM_zf_piece_s {
    ZERO_FILL_CONTEXT request;                  // Individual ZF context requests
    FLARE_ZERO_FILL_INFO flareZeroFillInfo;     // Payload to describe the ZF request
    PBasicIoRequest pZFIrp;                     // ZF IRP
    ULONG zf_size_in_blocks;                    // Size of ZF request
    ULONG sequence_number;                      // Sequence # of the bulk copy
    BOOLEAN inUse;                              // Indicates whether or not this
                                                // piece is used
    UINT_64E time_stamp;                        // Start time of the request
};

struct DM_bulk_request_s {
    PVOID Handle;                                 // Pointer to DM structure
    EMCPAL_SPINLOCK lock;                         // Lock protecting cursors
                                                  // and DM request state.
    PDM_request_piece requests;					  // Individual DM requests
    DM_zf_piece zfRequests[DM_NUM_EMBEDDED_BULK_COPY_ZF_REQUESTS]; // Individual ZF requests
    ULONG next_sequence_number_to_assign;         // Seq # for next req
    ULONG in_order_sequence_number;               // Current in-order completed
                                                  // sequence number.
    ULONGLONG in_order_bytes_complete;            // Current in-order bytes
                                                  // successfully completed.
    ULONG newest_out_of_order_sequence_number;    // Newest completed sequence
                                                  // number
    ULONGLONG out_of_order_bytes_complete;        // Accumulation of bytes
                                                  // completed out-of-order
    
    PEMCPAL_DEVICE_OBJECT src_device;             // Ptr to the source device for this request
    ULONG src_dca_query_status;                   // DCA query results for the source device
    DM_sgl src_sgl[DM_MAX_BULK_SGL_ELEMENTS];     // The array of src SGL
                                                  // elements for the request
    ULONG num_src_sgl_elements;                   // Number of src SGL elements.
    DM_bulk_cursor src_cursor;                    // The current location of the
                                                  // operation within the src SGL

    PEMCPAL_DEVICE_OBJECT dst_device;             // Ptr to the dest device for this request
    ULONG dst_dca_query_status;                   // DCA query results for the dest device
    DM_sgl dst_sgl[DM_MAX_BULK_SGL_ELEMENTS];     // The array of dst SGL
                                                  // elements for the request
    ULONG num_dst_sgl_elements;                   // Number of dst SGL elements.
    DM_bulk_cursor dst_cursor;                    // The current location of the
                                                  // operation within the dst SGL

    LONG reference_count;                         // Number of async refs to this
                                                  // request.
    PDM_bulk_request_callback callback;           // Originator callback function.
    PVOID context;                                // Originator callback context.
    EMCPAL_STATUS status;                              // Status of the bulk copy
    ULONG flags;                                  // Bitmap of DM_FLAGS
    BOOLEAN cancelled;                            // TRUE iff client cancelled this req
    PVOID cancel_context;                         // Context for cancellation
                                                  // callback
    PDM_bulk_request_cancel_callback cancel_callback; // Cancellation callback
    UINT_64E time_stamp;                          // Start time of the request
};


/*********************************
 * Macros
 *********************************/

#define DM_REQ_SET_BUFFER(_request, _buffer, _buf_size_in_bytes) \
    ((_request)->iobuffer = (PVOID) _buffer, \
     (_request)->buf_size_in_bytes = _buf_size_in_bytes)

#define DM_REQ_GET_BUFFER(_request) ((_request)->iobuffer)

/*********************************
 * EXTERNAL REFERENCE DEFINITIONS
 *********************************/

/************************
 * PROTOTYPE DEFINITIONS
 ************************/
/**************************************************************************
 *                       DM_initialize ()
 **************************************************************************
 *
 * DESCRIPTION:
 *   DM_initialize - This function is used to intialize the Data Movement
 *   library. It is called once during startup.
 *
 * PARAMETERS:
 *  IN UCHAR * block - Pointer to a block of memory for buffers.
 *  IN ULONG blk_length - Length of the block of memory.
 *  IN DM_LAYER_ID layer_id - layer ID.
 *  IN ULONG timeout - Timeout value in seconds.
 *  IN CMM_CLIENT_ID cmm_id - CMM memory pool ID.
 *  OUT PVOID handle - Returned handle to global library data.
 *
 * RETURN VALUES/ERRORS:
 *   STATUS_SUCCESS - Initialization was successful.
 *   STATUS_FAILURE - DM Library failed to initailize.
 *   STATUS_NO_MEMORY - DM Library was unable to allocate memory for buffer pool.
 *   STATUS_INVALID_PARAMETER - One or more of the parameters was invalid.
 *
 * NOTES:
 *   This routine must only be called once by each client at intialization time.
 *
 * HISTORY:
 *   27-Jan-05: A. Spang -- Created
 **************************************************************************/
EMCPAL_STATUS DM_initialize( 
    IN UCHAR * block,
    IN ULONG blk_length, 
    IN DM_LAYER_ID layer_id,
    IN ULONG timeout,
    IN CMM_CLIENT_ID cmm_id,
    OUT PVOID *handle );

/**************************************************************************
 *                          DM_create_request ()
 **************************************************************************
 *
 * DESCRIPTION:
 *   DM_create_request - Function used to allocate and initialize a Data 
 *   Movement request structures.
 *
 * PARAMETERS:
 *   IN VOID * handle - Handle to the DM library global data.
 *
 * RETURN VALUES/ERRORS:
 *   Pointer to a DM_request structure or NULL if allocation failed.
 *
 * NOTES:
 *
 * HISTORY:
 *   27-Jan-05: A. Spang -- Created
 **************************************************************************/

PDM_request DM_create_request(
    IN VOID * handle );

/**************************************************************************
 *                          DM_create_request_pool ()
 **************************************************************************
 *
 * DESCRIPTION:
 *   DM_create_request_pool - Function used to allocate and initialize a  
 *   pool of Data Movement request structures. This function will allocate
 *   the requests from the CMM pool setup at initialization time of the
 *   DM library.
 *
 * PARAMETERS:
 *   IN VOID * handle - Handle to the DM library global data.
 *   IN ULONG num_requests - Number of DM requests to allocate.
 *   OUT EMCPAL_LIST_ENTRY list - List of DM requests returned.
 *   OUT VOID ** pool_handle - Handle to the pool of requests.
 *
 * RETURN VALUES/ERRORS:
 *   STATUS_SUCCESS - The DM request pool was created.
 *   STATUS_INSUFFICIENT_RESOURCES - Unable to allocate memory.
 *   STATUS_INVALID_PARAMETER - Invalid parameters entered.
 *
 * NOTES:
 *
 * HISTORY:
 *   21-Feb-06: A. Spang -- Created
 **************************************************************************/

EMCPAL_STATUS DM_create_request_pool(
    IN VOID * handle,
    IN ULONG num_requests,
    OUT EMCPAL_LIST_ENTRY *list,
    OUT VOID ** pool_handle );


/**************************************************************************
 *                          DM_create_bulk_request ()
 **************************************************************************
 *
 * DESCRIPTION:
 *   DM_create_bulk_request - Function used to allocate and initialize a Data 
 *   Movement request structures for a bulk copy operation.
 *
 * PARAMETERS:
 *   IN VOID * handle - Handle to the DM library global data.
 *
 * RETURN VALUES/ERRORS:
 *   Pointer to a DM_bulk_request structure or NULL if allocation failed.
 *
 * NOTES:
 *
 * HISTORY:
 *   30-Mar-12: A. Taylor -- Created
 **************************************************************************/

PDM_bulk_request DM_create_bulk_request(
    IN VOID * handle );

/**************************************************************************
 *                          DM_request_size ()
 **************************************************************************
 *
 * DESCRIPTION:
 *   DM_request_size - Function is used to get the size of the DM request
 *   including the size of the IRPs and IRP stacks.
 *
 * PARAMETERS:
 *   IN VOID * handle - Handle to the DM library global data.
 *
 * RETURN VALUES/ERRORS:
 *   Size of a DM request in bytes
 *
 * NOTES:
 *
 * HISTORY:
 *   16-Feb-05: A. Spang -- Created
 **************************************************************************/

ULONG DM_request_size(
    IN VOID * handle );

/**************************************************************************
 *                          DM_start_initiator_request ()
 **************************************************************************
 *
 * DESCRIPTION:
 *   DM_start_initiator_request - Function used to start a request 
 *   from an originating driver.
 *
 * PARAMETERS:
 *   IN DM_request * dm_request - Pointer to a DM request structure
 *   IN PEMCPAL_DEVICE_OBJECT src_device - Pointer to the source device object.
 *   IN ULONGLONG src_offset - Source starting byte offset
 *   IN ULONG length - Number of bytes to transfer.
 *   IN ULONG src_dca_query_status - Source DCA Query status.
 *   IN PEMCPAL_DEVICE_OBJECT dst_device - Pointer to the dest. device object.
 *   IN ULONGLONG dst_offset - Destination starting byte offset.
 *   IN ULONG dst_dca_query_status - Destination DCA query status
 *   IN ULONG flags - DM flag values
 *   IN VOID (*callback)(VOID *context, DM_request *dm_request ),
 *                      - Request complete callback.
 *   IN VOID * context - Request complete callback context.
 *
 * RETURN VALUES/ERRORS:
 *   STATUS_SUCCESS - Request has been start successfully.
 *   STATUS_INVALID_PARAMETER - One or more parameters are invalid.
 *   STATUS_FAILURE - The DM library has not been initialized correctly.
 *
 * NOTES:
 *
 * HISTORY:
 *   27-Jan-05: A. Spang -- Created
 **************************************************************************/

EMCPAL_STATUS DM_start_initiator_request(
    IN DM_request * dm_request,
    IN PEMCPAL_DEVICE_OBJECT src_device,
    IN ULONGLONG src_offset,
    IN ULONG length,
    IN ULONG src_dca_query_status,
    IN PEMCPAL_DEVICE_OBJECT dst_device,
    IN ULONGLONG dst_offset,
    IN ULONG dst_dca_query_status,
    IN ULONG flags, 
    IN VOID (*callback)(VOID *context, DM_request *dm_request ),
    IN VOID * context);
        

/**************************************************************************
 *                    DM_start_intermediate_request ()
 **************************************************************************
 *
 * DESCRIPTION:
 *   DM_start_intermediate_request - This function is used by intermediate 
 *   drivers that receive a DM IRP to forward the IRP on to a different 
 *   device.
 *
 * PARAMETERS:
 *   IN PIRP Irp - Originator's DM IRP.
 *   IN PEMCPAL_DEVICE_OBJECT Device - Pointer to the new device object.
 *   IN ULONGLONG Offset - Starting byte offset into the LU.
 *   IN ULONG Length - transfer length.
 *   IN PIO_COMPLETION_ROUTINE CompletionRoutine) - IRP complete callback.
 *   IN VOID * Context - IRP complete callback context.
 *
 * RETURN VALUES/ERRORS:
 *   See IoCallDriver.
 *
 * NOTES:
 *
 * HISTORY:
 *   27-Jan-05: A. Spang -- Created
 **************************************************************************/
EMCPAL_STATUS DM_start_intermediate_request(
    IN PBasicIoRequest Irp,
    IN PEMCPAL_DEVICE_OBJECT Device,
    IN ULONGLONG Offset,
    IN ULONG Length,
    IN PEMCPAL_IRP_COMPLETION_ROUTINE CompletionRoutine,
    IN VOID * Context,
    IN DM_LAYER_ID layerId);


/**************************************************************************
 *                          DM_start_initiator_bulk_request ()
 **************************************************************************
 *
 * DESCRIPTION:
 *   DM_start_initiator_bulk_request - Function used to start a bulk request 
 *   from an originating driver.
 *
 * PARAMETERS:
 *   IN DM_bulk_request * dm_bulk_request - Pointer to a DM bulk request struct
 *   IN PDEVICE_OBJECT src_device - Pointer to the source device object.
 *   IN DM_sgl *src_sgl - Pointer to the source SGL
 *   IN ULONG num_src_sgl_elements - Number of elements in the source SGL
 *   IN ULONGLONG src_sgl_starting_offset - The starting offset into the source SGL.
 *   IN ULONG src_dca_query_status - Source DCA Query status.
 *   IN PDEVICE_OBJECT dst_device - Pointer to the dest. device object.
 *   IN DM_sgl *dst_sgl - Pointer to the destination SGL
 *   IN ULONG num_dst_sgl_elements - Number of elements in the source SGL
 *   IN ULONG dst_dca_query_status - Destination DCA query status
 *   IN ULONG flags - DM flag values
 *   IN PDM_bulk_request_callback callback - Request complete callback.
 *   IN VOID * context - Request complete callback context.
 *
 * RETURN VALUES/ERRORS:
 *   STATUS_SUCCESS - Bulk request has been start successfully.
 *   STATUS_INVALID_PARAMETER - One or more parameters are invalid.
 *   STATUS_FAILURE - The DM library has not been initialized correctly.
 *
 * NOTES:
 *
 * HISTORY:
 *   30-Mar-12: A. Taylor -- Created
 **************************************************************************/

EMCPAL_STATUS DM_start_initiator_bulk_request(
    IN DM_bulk_request *dm_bulk_request,
    IN PEMCPAL_DEVICE_OBJECT src_device,
    IN DM_sgl *src_sgl,
    IN ULONG num_src_sgl_elements,
    IN ULONGLONG src_sgl_starting_offset,
    IN ULONG src_dca_query_status,
    IN PEMCPAL_DEVICE_OBJECT dst_device,
    IN DM_sgl *dst_sgl,
    IN ULONG num_dst_sgl_elements,
    IN ULONG dst_dca_query_status,
    IN ULONG flags,
    IN PDM_bulk_request_callback callback,
    IN VOID * context);

/**************************************************************************
 *                    DM_intermediate_request_done ()
 **************************************************************************
 *
 * DESCRIPTION:
 *   DM_intermediate_request_done - This function should be called by
 *   Intermediate drivers after their own callback routine is invoked.
 *
 * PARAMETERS:
 *   IN PIRP Irp - Originator's DM IRP.
 *
 * RETURN VALUES/ERRORS:
 *   None.
 *
 * NOTES:
 *
 * HISTORY:
 *   04-Nov-10: T. Rivera -- Created
 **************************************************************************/
void DM_intermediate_request_done( IN PBasicIoRequest Irp );

/**************************************************************************
 *                       DM_get_bytes_transfered ()
 **************************************************************************
 *
 * DESCRIPTION:
 *   DM_get_bytes_transfered - This function returns the actual number of
 *   bytes that this request has transferred.
 *
 * PARAMETERS:
 *   IN DM_request * dm_request - Pointer to DM request.
 *
 * RETURN VALUES/ERRORS:
 *   Number of bytes actually transferred or zero if called before request
 *   has completed.
 *
 * NOTES:
 *
 * HISTORY:
 *   27-Jan-05: A. Spang -- Created
 **************************************************************************/

ULONG DM_get_bytes_transfered(
    IN DM_request * dm_request );

/**************************************************************************
 *                       DM_get_request_status ()
 **************************************************************************
 *
 * DESCRIPTION:
 *   DM_get_request_status - This function returns the status of a DM 
 *   request. 
 *
 * PARAMETERS:
 *   IN DM_request * dm_request - Pointer to a DM request.
 *
 * RETURN VALUES/ERRORS:
 *   STATUS_SUCCESS if the DM request completed successfully.
 *   STATUS_PENDING if the DM request is in progress.
 *   STATUS_REQUEST_ABORTED or STATUS_NOT_SUPPORTED may be returned for 
 *       sub-requests when a lower level driver doesn't support DM 
 *       requests or the driver indicators are different.
 *   STATUS_NOT_ENOUGH_RESOURCES may be returned if no buffers were 
 *       allocated at initialization time.
 *   Other NT Status codes for various failures returned by lower drivers.
 *
 * NOTES:
 *
 * HISTORY:
 *   27-Jan-05: A. Spang -- Created
 **************************************************************************/

EMCPAL_STATUS DM_get_request_status(
    IN DM_request * dm_request );

/**************************************************************************
 *                       DM_get_source_status ()
 **************************************************************************
 *
 * DESCRIPTION:
 *   DM_get_source_status - This function returns the status of a source 
 *   IRP. 
 *
 * PARAMETERS:
 *   IN DM_request * dm_request - Pointer to a DM request.
 *
 * RETURN VALUES/ERRORS:
 *   STATUS_SUCCESS if the DM request completed successfully.
 *   STATUS_PENDING if the DM request is in progress.
 *   STATUS_CANCELLED if the DM request has been cancelled.
 *   Various other NT status codes as defined in the K10 Disk Device IRP
 *   specification
 *
 * NOTES:
 *
 * HISTORY:
 *   14-Feb-05: A. Spang -- Created
 **************************************************************************/

EMCPAL_STATUS DM_get_source_status(
    IN DM_request * dm_request );

/**************************************************************************
 *                       DM_get_destination_status ()
 **************************************************************************
 *
 * DESCRIPTION:
 *   DM_get_source_status - This function returns the status of a 
 *   destination IRP. 
 *
 * PARAMETERS:
 *   IN DM_request * dm_request - Pointer to a DM request.
 *
 * RETURN VALUES/ERRORS:
 *   STATUS_SUCCESS if the DM request completed successfully.
 *   STATUS_PENDING if the DM request is in progress.
 *   STATUS_CANCELLED if the DM request has been cancelled.
 *   Various other NT status codes as defined in the K10 Disk Device IRP
 *   specification
 *
 * NOTES:
 *
 * HISTORY:
 *   14-Feb-05: A. Spang -- Created
 **************************************************************************/

EMCPAL_STATUS DM_get_destination_status(
    IN DM_request * dm_request );


/**************************************************************************
 *                       DM_get_bulk_request_status ()
 **************************************************************************
 *
 * DESCRIPTION:
 *   DM_get_bulk_request_status - This function returns the status of a DM 
 *   bulk request. 
 *
 * PARAMETERS:
 *   IN DM_bulk_request * dm_bulk_request - Pointer to a DM bulk request.
 *
 * RETURN VALUES/ERRORS:
 *   STATUS_SUCCESS if the DM bulk request completed successfully.
 *   STATUS_PENDING if the DM bulk request is in progress. 
 *   Other NT Status codes for various failures returned by lower drivers.
 *
 * NOTES:
 *
 * HISTORY:
 *   30-Mar-12: A. Taylor -- Created
 **************************************************************************/
EMCPAL_STATUS DM_get_bulk_request_status(
    IN DM_bulk_request * dm_bulk_request );


/**************************************************************************
 *                       DM_get_bytes_transferred_bulk ()
 **************************************************************************
 *
 * DESCRIPTION:
 *   DM_get_bytes_transferred_bulk - This function returns the actual number of
 *   bytes that this bulk request has transferred.
 *
 * PARAMETERS:
 *   IN DM_bulk_request * dm_bulk_request - Pointer to DM bulk request.
 *
 * RETURN VALUES/ERRORS:
 *   Number of bytes actually transferred.  For in-progress bulk transfers,
 *   returns a snapshot of the current progress.
 *
 * NOTES:
 *
 * HISTORY:
 *   30-Mar-12: A. Taylor -- Created
 **************************************************************************/

ULONGLONG DM_get_bytes_transferred_bulk(
    IN DM_bulk_request * dm_bulk_request );

/**************************************************************************
 *                          DM_cancel_request ()
 **************************************************************************
 *
 * DESCRIPTION:
 *   DM_cancel_request - This function is used to cancel a DM request
 *
 * PARAMETERS:
 *   IN DM_request * dm_request - Pointer to a DM request.
 *
 * RETURN VALUES/ERRORS:
 *   None
 *
 * NOTES:
 *   The request complete callback function will be called when the 
 *   DM request is completely canceled.
 *
 * HISTORY:
 *   27-Jan-05: A. Spang -- Created
 **************************************************************************/

VOID DM_cancel_request(
    IN DM_request * dm_request );

/**************************************************************************
 *                          DM_cancel_bulk_request ()
 **************************************************************************
 *
 * DESCRIPTION:
 *   DM_cancel_bulk_request - This function is used to cancel a DM bulk request
 *
 * PARAMETERS:
 *   IN DM_bulk_request * dm_bulk_request - Pointer to a DM bulk request.
 *   IN PDM_bulk_request_cancel_callback callback - Cancellation callback
 *       (optional, DM bulk request callback executed regardless of presence
 *       of this callback)
 *   IN VOID * context - Cancellation complete callback context (optional).
 *
 * RETURN VALUES/ERRORS:
 *   STATUS_SUCCESS if the DM bulk request was successfully cancelled (no
 *       future callback)
 *   Other if an error occurred while starting the cancellation process (no
 *       future callback)
 *
 * NOTES:
 *   The request complete callback function will be called when the 
 *   DM bulk request is completely canceled.  After the request callback
 *   executes, the cancel logic will execute the cancel callback (if valid).
 *   If the DM bulk request is not outstanding only the cancel callback will
 *   be executed.
 *
 * HISTORY:
 *   30-Mar-12: A. Taylor -- Created
 **************************************************************************/

EMCPAL_STATUS DM_cancel_bulk_request(
    IN DM_bulk_request * dm_bulk_request,
    IN PDM_bulk_request_cancel_callback callback,
    IN VOID * context );

/**************************************************************************
 *                          DM_reinit_request ()
 **************************************************************************
 *
 * DESCRIPTION:
 *   DM_reinit_request - This function is used to re-initialize a DM 
 *   request before it is to be reused.
 *
 * PARAMETERS:
 *   IN VOID * handle - Pointer to the library global data.
 *   IN DM_request * dm_request - Pointer to the DM request to be re-init.
 *
 * RETURN VALUES/ERRORS:
 *   STATUS_SUCCESS - The request has been reinitialized.
 *   STATUS_FAILURE - The request is still being used.
 *
 * NOTES:
 *
 * HISTORY:
 *   27-Jan-05: A. Spang -- Created
 **************************************************************************/

EMCPAL_STATUS DM_reinit_request(
    IN VOID * handle,
    IN DM_request * dm_request );


/**************************************************************************
 *                          DM_reinit_bulk_request ()
 **************************************************************************
 *
 * DESCRIPTION:
 *   DM_reinit_bulk_request - This function is used to re-initialize a DM 
 *   bulk request before it is to be reused.
 *
 * PARAMETERS:
 *   IN DM_bulk_request * dm_bulk_request - Pointer to the DM bulk request to
 *        be re-init.
 *
 * RETURN VALUES/ERRORS:
 *   STATUS_SUCCESS - The request has been reinitialized.
 *   STATUS_FAILURE - The request is still being used.
 *
 * NOTES:
 *
 * HISTORY:
 *   07-May-12: A. Taylor -- Created
 **************************************************************************/

EMCPAL_STATUS DM_reinit_bulk_request(
    IN DM_bulk_request * dm_bulk_request );
    
/**************************************************************************
 *                        DM_delete_request ()
 **************************************************************************
 *
 * DESCRIPTION:
 *   DM_delete_request - This function is used to destroy a request. This
 *   function will deallocate all resources associated with this request
 *   and the request itself.
 *
 * PARAMETERS:
 *   IN DM_request * dm_request - Pointer to the request to be deleted.
 *
 * RETURN VALUES/ERRORS:
 *   STATUS_SUCCESS - The DM request has been successfully deallocated.
 *   STATUS_FAILURE - The DM request has not completed.
 *
 * NOTES:
 *
 * HISTORY:
 *   27-Jan-05: A. Spang -- Created
 **************************************************************************/

EMCPAL_STATUS DM_delete_request(
    IN DM_request * dm_request );

/**************************************************************************
 *                        DM_delete_request_pool ()
 **************************************************************************
 *
 * DESCRIPTION:
 *   DM_delete_request_pool - This function is used to destroy a pool of 
 *   requests. This function will deallocate all resources associated with 
 *   these requests and the request themselves.
 *
 * PARAMETERS:
 *   IN VOID * handle - Opaque handle to the DM global structure.
 *   IN VOID * pool_handle - Opaque handle to the request pool.
 *
 * RETURN VALUES/ERRORS:
 *   STATUS_SUCCESS - The DM requests have been successfully deallocated.
 *   STATUS_UNSUCCESSFUL - One or more DM request have not completed.
 *   STATUS_INVALID_PARAMETER - Invalid pool handle.
 *
 * NOTES:
 *
 * HISTORY:
 *   21-Feb-06: A. Spang -- Created
 **************************************************************************/

EMCPAL_STATUS DM_delete_request_pool(
    IN VOID * handle,
    IN VOID * pool_handle );

/**************************************************************************
 *                        DM_delete_bulk_request ()
 **************************************************************************
 *
 * DESCRIPTION:
 *   DM_delete_bulk_request - This function is used to destroy a bulk copy
 *   request. This function will deallocate all resources associated with
 *   this request and the request itself.
 *
 * PARAMETERS:
 *   IN DM_bulk_request * request - Pointer to the request to be deleted.
 *
 * RETURN VALUES/ERRORS:
 *   STATUS_SUCCESS - The DM bulk request has been successfully deallocated.
 *   STATUS_UNSUCCESSFUL - The DM bulk request has not completed.
 *
 * NOTES:
 *
 * HISTORY:
 *   30-Mar-12: A. Taylor -- Created
 **************************************************************************/

EMCPAL_STATUS DM_delete_bulk_request(
    IN DM_bulk_request * request );

/**************************************************************************
 *                          DM_get_stats ()
 **************************************************************************
 *
 * DESCRIPTION:
 *   DM_get_stats - This function is used to get a copy of the current
 *   DM statistics structure for this instance of the library.
 *
 * PARAMETERS:
 *   IN VOID * handle - Pointer to the DM global data.
 *
 * RETURN VALUES/ERRORS:
 *   A DM_stats structure will be returned.
 *
 * NOTES:
 *
 * HISTORY:
 *   27-Jan-05: A. Spang -- Created
 **************************************************************************/

DM_stats DM_get_stats( 
    IN VOID * handle );

/**************************************************************************
 *                          DM_clear_stats ()
 **************************************************************************
 *
 * DESCRIPTION:
 *   DM_clear_stats - This function is used to clear the DM statistics 
 *   structure.
 *
 * PARAMETERS:
 *   IN VOID * handle - Pointer to the DM global data.
 *
 * RETURN VALUES/ERRORS:
 *   None
 *
 * NOTES:
 *   If this function is called while DM requests are outstanding, the 
 *   statistics counters may be incorrect.
 *
 * HISTORY:
 *   27-Jan-05: A. Spang -- Created
 **************************************************************************/

VOID DM_clear_stats( 
    IN VOID * handle );

/**************************************************************************
 *                          DM_shutdown ()
 **************************************************************************
 *
 * DESCRIPTION:
 *   DM_shutdown - This function is used to shutdown the DM library. All 
 *   global resources are freed.
 *
 * PARAMETERS:
 *   IN VOID * handle - Pointer to the DM global data.
 *
 * RETURN VALUES/ERRORS:
 *   STATUS_SUCCESS - The DM library shutdown correctly.
 *   STATUS_UNSUCCESSFUL - There are requests still in use.
 *
 * NOTES:
 *   After this function is called, DM requests will not be processed.
 *
 * HISTORY:
 *   27-Jan-05: A. Spang -- Created
 **************************************************************************/
EMCPAL_STATUS DM_shutdown( 
    IN VOID * handle );

ULONG DM_extract_LU_number(
    IN PVOID deviceExtension);

char * DM_get_DM_request_flag_string(IN UINT_32 flag);

/**************************************************************************
 *                       DM_get_bulk_request_size ()
 **************************************************************************
 *
 * DESCRIPTION:
 *   DM_get_bulk_request_size - This function returns the size of a DM 
 *   bulk request. 
 *
 * PARAMETERS:
 *   IN VOID * handle - Handle to the DM library global data.
 *   IN DM_LAYER_ID - ID of the layered driver.
 *
 * RETURN VALUES/ERRORS:
 *   The size of a DM bulk request.
 *
 * NOTES:
 *
 * HISTORY:
 *   27-Sep-12: D. Wang -- Created
 **************************************************************************/
ULONG DM_get_bulk_request_size(
    IN VOID * handle,
    IN DM_LAYER_ID layer_id);


/**************************************************************************
 *                       DM_get_overhead_size ()
 **************************************************************************
 *
 * DESCRIPTION:
 *   DM_get_overhead_size - This function allows a particular client to find out how much
 *   memory the serivce might have to allocate on behalf of that client.
 *
 * PARAMETERS:
 *   IN VOID * handle - Handle to the DM library global data.
 *   IN DM_LAYER_ID - ID of the layered driver.
 *
 * RETURN VALUES/ERRORS:
 *   The size of the memory.
 *
 * NOTES:
 *
 * HISTORY:
 *   12-Otc-12: D. Wang -- Created
 **************************************************************************/
ULONG DM_get_overhead_size(
    IN VOID * handle,
    IN DM_LAYER_ID layer_id);


#ifdef __cplusplus
}
#endif

/*
 * End $Id:$
 */

#endif
