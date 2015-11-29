/*****************************************************************************
 * Copyright (C) EMC Corp. 2004
 * All rights reserved.
 * Licensed material - Property of EMC Corporation.
 *****************************************************************************/

/*****************************************************************************
 * EmcHalDmaApi.h 
 *****************************************************************************
 *
 * DESCRIPTION:
 *    This module defines the public APIs, along with public constants,
 *    structures, needed to call these APIs
 *
 * NOTES:
 *
 * HISTORY:
 *      9/9/2004    Maher Salha         Created
 *     10/6/2004    Maher Salha         Modified per code review    
 *
 *****************************************************************************/
#ifndef EMCHALDMAAPI_H
#define EMCHALDMAAPI_H

#include "k10ntddk.h"
#include "EmcPAL_DriverShell.h"
#include "CmiCommonInterface.h"     // Required for SGL definitions

// The maximum number of entries the each EmcHalDma client can request
#define EMCHALDMA_MAX_SG_ENTRIES	256

/*****************************************************************************
 * enum EMCHALDMA_STATUS 
 *    EmcHalDma status code enumeration.
 *   
 *****************************************************************************/
typedef enum _EMCHALDMA_STATUS
{
   EMCHALDMA_STATUS_SUCCESS   = 0,  // DMA transfer completed successfully

   EMCHALDMA_STATUS_ERROR     = 1,  // An error occurred 

   EMCHALDMA_STATUS_PENDING   = 2,  // The DMA transfer is in progess

   EMCHALDMA_STATUS_INVALID_PARAMETER  = 3,  // Request specified
                                             // is not currently pending

   EMCHALDMA_STATUS_INSUFFICIENT_RESOURCES   = 4,  // The client driver 
                          //requested a DMA transfer while one is pending or
                          // The SGL entries are not enough to satisfy the request


   EMCHALDMA_STATUS_ABORTED   = 5,  // The current DMA transfer is aborted
                                    // on behalf of the client driver request
   
} EMCHALDMA_STATUS;


/*****************************************************************************
 * const EMCHAL_CTL_CODE  - A helper macro used to define EMCHALDMA ioctls.
 *****************************************************************************/
#define EMCHALDMA_CTL_CODE(code, method) (            \
    EMCPAL_IOCTL_CTL_CODE(EMCPAL_IOCTL_FILE_DEVICE_UNKNOWN, 0xC00 + (code),  \
(method), EMCPAL_IOCTL_FILE_ANY_ACCESS) )

/*****************************************************************************
 * const IOCTL_EMCHALDMA_PERFORM_HANDSHAKE - I/O control code to exchange 
 *    function call table with the client driver 
 *****************************************************************************/
#define IOCTL_EMCHALDMA_PERFORM_HANDSHAKE          \
EMCHALDMA_CTL_CODE(2, EMCPAL_IOCTL_METHOD_BUFFERED)

/*****************************************************************************
 * struct EMCHALDMA_SGL_TRACKER 
 *    This data structure is used to pass all information required to form
 *    a combined SGL from two independent (source and destination) SGLs.
 *    
 *****************************************************************************/
typedef struct _EMCHALDMA_SGL_TRACKER
{
   
   PCMI_PHYSICAL_SG_ELEMENT   pSourceSgl; // Pointer to the SGL that 
                           // describes the location and length of source data.

   PCMI_PHYSICAL_SG_ELEMENT   pDestinationSgl; // Pointer to the SGL that
                        // describes the location and length of destination data.
   
   ULONG                      TotalBytes; // This is set to the the number
                                          // of bytes to transfer
} EMCHALDMA_SGL_TRACKER, *PEMCHALDMA_SGL_TRACKER;

/*****************************************************************************
 * struct EMCHALDMA_REQUEST_HANDLE 
 *    Structure to track a DMA request
 *****************************************************************************/
typedef struct _EMCHALDMA_REQUEST_HANDLE
{
   ULONG             GID;     // Generation ID of this request
   PVOID             pDmaRequest;   // Pointer to the DMA request 
} EMCHALDMA_REQUEST_HANDLE, *PEMCHALDMA_REQUEST_HANDLE;

/*****************************************************************************
 * typedef PPCIHALDMA_DO_DMA  
 *    A PPCIHALDMA_DO_DMA-typed routine performs the DMA transfer
 *
 * Parameters:
 *
 *  pDevObj      
 *    Pointer to the device object associated with the DMA channel
 *
 *  pSglInfo 
 *    Pointer to Scatter/Gather information used as both an input and an output. 
 *
 *  pCallback 
 *    Pointer to a caller-specified function that is called to determine whether
 *    the given DMA transfer is competed or aborted. 
 *    If this is NULL, no callback the client driver could not determine the 
 *    result of the DMA transfer.
 *
 *  pContext 
 *    Context that is passed directly back to the callback routine when called.  
 *
 *  pDmaRequestHandle 
 *    This should point to a location that will receive a pointer to this 
 *    DMA request.  This can then be used to identify this specific request in
 *    a subsequent call to PciHalAbortDMARequest. A NULL pointer can be passed
 *    in.
 *
 * Return Value:
 *    If successful, a PPCIHALDMA_DO_DMA-typed routine returns 
 *    EMCHALDMA_STATUS_PENDING code; otherwise, it returns the appropriate
 *    EMCHADMA  error code.
 *
 * Comments:
 *    The DMA transfer operation is an Asynchronous operation.
 *    It is the client driver's responsiblity to supply a DMA completion 
 *    callback routine to be called when the DMA transfer is complete.
 *****************************************************************************/
typedef EMCHALDMA_STATUS (*PEMCHALDMA_DO_DMA)
(
   PEMCPAL_DEVICE_OBJECT    pDevObj,
   PEMCHALDMA_SGL_TRACKER  pSglInfo,
   VOID              (*pCallback)
   (
      PVOID             pContext,
      EMCHALDMA_STATUS      Status
   ),
   PVOID             pContext,
   PEMCHALDMA_REQUEST_HANDLE pDmaRequestHandle
);

/****************************************************************************
 * typedef PEMCHALDMA_ABORT_REQUEST  
 *    A PEMCHALDMA_ABORT_REQUEST-typed routine aborts an outstanding DMA
 *    transfer associated with the passed in DMA handle that contains the DMA
 *    request to abort
 *
 * Parameters:
 *
 *   pDmaRequestHandle 
 *    Points to the request handle (containing the DMA request) to abort
 *
 * Return Value:
 *
 *    EMCHALDMA_STATUS_SUCCESS - successfully aborted
 *    EMCHALDMA_STATUS_INVALID_PARAMETER - If request specified is not
 *       currently pending
 *
 * Comments:
 *    If the request is currently being processed by the hardware, then an
 *    attempt will be made to abort the request. No guarantee is made that the
 *    request might not complete successfully first, as the callback routine 
 *    might or might not be called before the call to EmcHalDmabortRequest
 *    returns.
 *    
 *    In addition, the status of the callback might or might not indicate 
 *    EMCHALDMA_STATUS_ABORTED depending on the timing of the abort with
 *    respect to the DMA in progress.
 *    
 ***************************************************************************/
typedef EMCHALDMA_STATUS (*PEMCHALDMA_ABORT_REQUEST)
(
   PEMCHALDMA_REQUEST_HANDLE pDmaRequest
);

/*****************************************************************************
 * struct EMCHALDMA_HANDSHAKE_DATA_OUT 
 *
 *    The EMCHALDMA_HANDSHAKE_DATA_OUT interface structure enables 
 *    client drivers to make direct calls to EmcHalDma routines
 *
 *****************************************************************************/
typedef struct _EMCHALDMA_HANDSHAKE_DATA_OUT
{
   PEMCPAL_DEVICE_OBJECT          pDevObj; // Pointer to the device object that
                                    // owns the DMA channel            
   
   ULONG                   MaximumSglEntries;// The maximum number of 
                                    // scatter/gather entries that are
                                    // allocated by EmcHalDma device

   ULONG                   MaximumTransferLengthPerSglEntry;// The 
                                       // maximum number of bytes per each
                                       // scatter/gather entry
   
   PEMCHALDMA_DO_DMA       pEmcHalDmaDoDma;// Pointer to a routine of
                                       //type PEMCHALDMA_DO_DMA that 
                                       //configures and starts a DMA transfer

   PEMCHALDMA_ABORT_REQUEST    pEmcHalDmaAbortRequest;// Pointer to a
                                       // routine of type PPCIHALDMA_ABORT_REQUEST
                                       // that aborts the DMA transfer
} EMCHALDMA_HANDSHAKE_DATA_OUT, *PEMCHALDMA_HANDSHAKE_DATA_OUT;


/*****************************************************************************
 * struct EMCHALDMA_HANDSHAKE_DATA_IN 
 *
 *    The EMCHALDMA_HANDSHAKE_DATA_IN interface structure is used to send 
 *    client device drivers data to EmcHalDma device object.
 *
 * Comments:
 *
 *    Based on the client driver buffer size, they set MaximumSglEntries
 *    field to notify the handshake routine of their required number of 
 *    SGL entries. EmcHalDma, in turns, tries to allocate memory to satisfy
 *    the requested number of SGL entries. If EmcHalDma is unable to satisfy
 *    the client's request, it will return STATUS_INSUFFICIENT_RESOURCES.
 *
 *    Important Note:
 *    SGL entries are setup to allow of transfer of scattered memory
 *    regions. If the client driver wishes to transfer buffers that are
 *    set in contiguous memory regions, SGL can have as little as one entry.
 *    The less the number of SGL elementes the faster the DMA transfer. 
 *****************************************************************************/
typedef struct _EMCHALDMA_HANDSHAKE_DATA_IN
{
   ULONG    MaximumSglEntries; // The maximum number of 
                               // SGL entries that the client driver
                               // requires for DMA transfer
   VOID     (*pEventCallback)(PVOID context,
                              ULONG event,
                              PVOID data);// Pointer to a client
                                          // callback function to be called upon
                                         // an event

   PVOID    Context;          // Points to an event
                              // context to pass to the EventCallback
                              // routine.
} EMCHALDMA_HANDSHAKE_DATA_IN, *PEMCHALDMA_HANDSHAKE_DATA_IN;


#endif // EMCHALDMAAPI_H
