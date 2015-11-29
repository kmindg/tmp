//
// Copyright (C) Data General Corporation, 1999
// All rights reserved.
// Licensed material -- property of Data General Corporation
//

//
// File:            UMps.h
//
// Description:     This file contains the prototypes for the UMps interface.  A
//                  potential user-mode client must include this file for UMps
//                  support.
//
//                  The MPS layered driver must be loaded for proper
//                  functionality.
//
//                  Clients must include "windows.h" for UMps support.
//
// History:         02-Mar-99       ALT     Created
//

#ifndef __UMPS__
#define __UMPS__

#include "K10MpsMessages.h"
#include "csx_ext.h"

//
// User space programs can not interpret a NTSTATUS value.  Since
// "CmiUpperInterface.h" make use of NTSTATUS, typedef the value
// before including K10 specific header files.
//

#include "MpsInterface.h"

#ifdef UMPS_EXPORT
#define UMPS_API CSX_MOD_EXPORT
#else
#define UMPS_API CSX_MOD_IMPORT
#endif

//
// The following defines default value of user space MPS handle
//

#define     UMPS_INVALID_HANDLE_VALUE               0

//
// All UMps interface routines return BOOL and set the last error to an
// appropriate value.  If the return value is TRUE, the last error is
// K10_UMPS_ERROR_SUCCESS.  If the return value is FALSE, the last error is
// either a value in the UMps error enumeration (below) or it is a value set by
// Win32.  The NTSTATUS type is not used by UMps or UMps clients
//

#ifdef __cplusplus
extern "C" {
#endif

UMPS_API
ULONG
UMpsGetLastError(VOID);

UMPS_API
VOID
UMpsSetLastError(ULONG error);

UMPS_API
BOOL
UMpsOpen(
    IN  csx_cstring_t                         pMailboxName,
    OUT PUMPS_HANDLE                    pFilamentHandle
    );

UMPS_API
BOOL
UMpsClose(
    IN  UMPS_HANDLE                     FilamentHandle
    );

UMPS_API
BOOL
UMpsPoke(
    IN  UMPS_HANDLE                     FilamentHandle
    );

UMPS_API
BOOL
UMpsSynchronousSend(
    IN UMPS_HANDLE                      FilamentHandle,
    IN PCMI_SP_ID                       pDestination,
    IN LPVOID                           Ptr,
    IN DWORD                            Size
    );

UMPS_API
BOOL
UMpsReceive(
    IN  UMPS_HANDLE                     FilamentHandle,
    IN  LONG                            Timeout,
    OUT LPVOID                         *Ptr,
    OUT PDWORD                          pSize,
    OUT PCMI_SP_ID                      pSender,
    OUT MPS_CONTEXT                    *Context
    );

UMPS_API
BOOL
UMpsReceiveWait(
    IN  UMPS_HANDLE                     FilamentHandle,
    IN  LONG                            Timeout,
    OUT LPVOID                         *Ptr,
    OUT PDWORD                          pSize,
    OUT PCMI_SP_ID                      pSender,
    OUT MPS_CONTEXT                     *Context
    );

UMPS_API
BOOL
UMpsReleaseReceivedMessage(
    IN  UMPS_HANDLE                    FilamentHandle,
    IN  MPS_CONTEXT                    Context
    );

UMPS_API
BOOL
UMpsGetSpId(
    IN OUT PCMI_SP_ID                   pLocalCmiSpId,
    IN OUT PCMI_SP_ID                   pPeerCmiSpId
    );

UMPS_API
BOOL
UMpsSynchronousOperation(
    IN UMPS_HANDLE                  FilamentHandle,
    IN PCMI_SP_ID                   pDestination,
    IN ULONG                        SequenceTag,
    IN LPVOID                       InputPtr,
    IN DWORD                        InputSize,
    OUT LPVOID                      *OutputBuffer,
    OUT PULONG                      OutputBufferSize,
    OUT MPS_CONTEXT                 *Context
    );

UMPS_API
BOOL
UMpsSynchronousResponse(
    IN UMPS_HANDLE                  FilamentHandle,
    IN PCMI_SP_ID                   pDestination,
    IN ULONG                        SequenceTag,
    IN LPVOID                       Ptr,
    IN DWORD                        Size
    );

#ifdef __cplusplus
};
#endif

#endif // __UMPS__

//
// End of UMps.h
//

