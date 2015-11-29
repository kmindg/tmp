#ifndef IDM_H
#define IDM_H 0x00000001  /* from common dir */
#define FLARE__STAR__H
/**********************************************************************
 * Copyright (C) EMC Corporation 2003
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 **********************************************************************/

/**********************************************************************
 * idm.h
 **********************************************************************
 *
 * DESCRIPTION:
 *   This header file contains structures and definitions needed by
 *   the Flare IDM driver interface
 *
 * NOTES:
 *
 * HISTORY:
 *   22-Jan-03 BJP -- Created.
 *
 **********************************************************************/

/* Other include files
 */

#include "IdmInterface.h"
#include "LIFOSinglyLinkedList.h"
#include "InterlockedFIFOList.h"

/* Enumeration that describes possible buffer types
 */
typedef enum _BufferType
{
    BufferTypeIn,
    BufferTypeOut
} BufferType;

/* Enumeration that describes possible modes of operation
 */
typedef enum _RequestMode
{
    RequestModeSynchronous,
    RequestModeAsynchronous
} RequestMode;

/* Structure containing request data to be issued to the IDM driver
 */
typedef struct _IdmRequest IdmRequest;
struct _IdmRequest
{
    /* Idm request data
     */
    IdmRequest *nextPtr;
    OPAQUE_PTR   bufferPtr;
    UINT_32     inBufferSize;
    UINT_32     outBufferSize;
    UINT_32     ctlCode;
    
    /* Caller's notification information
     */
    EMCPAL_STATUS    requestStatus;
    OPAQUE_PTR   contextPtr;
    UINT_32     msgCode;
    INT_32      pipeId;
};

IFIFOListDefineListType(IdmRequest, InterlockedListofIdmReq);
IFIFOListDefineInlineListTypeFunctions(IdmRequest, InterlockedListofIdmReq, nextPtr);

/* Interface Function prototypes
 */
BOOL idmOpen(void);

VOID idmClose(void);

IdmRequest *idmBuildIdmRequest(UINT_32 requestCode,
                               OPAQUE_PTR dataBuffer,
                               UINT_32 dataBufferSize,
                               BufferType dataBufferType,
                               OPAQUE_32 contextPtr,
                               UINT_32 msgCode,
                               INT_32 pipeId);
                               
EMCPAL_STATUS idmControl(IdmRequest *idmRequestPtr,
                    RequestMode requestMode);

/* Not an interface function but needs to be declared
   at this level so that the process can be registered
 */
VOID idmAsyncThread(UINT_32 unused);

#endif
/*
 * End idm.h
 */
