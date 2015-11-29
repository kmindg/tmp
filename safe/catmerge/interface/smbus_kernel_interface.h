#ifndef __SMBUS_INTERFACE__
#define __SMBUS_INTERFACE__

//***************************************************************************
// Copyright (C) EMC Corporation 2004-2005
// All rights reserved.
// Licensed material -- property of Data General Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

/*******************************************************************************
 * File Name:
 *      smbus_kernel_interface.h
 *
 * Contents:
 *      Definitions of the exported data structures 
 *      that form the interface between the SMBus driver and its higher-
 *      level clients.
 *
 * Revision History:
 *  25-Apr-2005   Naizhong Qiu    Created.
 *
 ******************************************************************************/


//
// Header files
//
#include "smbus_driver.h"

// define read/write operations, for opCode in callerRequest struct
#define SMBUS_READ_OP  IOCTL_SMBUS_READ
#define SMBUS_WRITE_OP IOCTL_SMBUS_WRITE

/* Facility panic codes for WHO */
/* The Flare panic.h file has panic bases up to 0x04xxx00000,
 *  so we start our offsets at 0x05810000 so our values are distinct.
 * (We use our own panic facility so this is not critical)
 */
#define SMB_BASE_MASK    (0xFFFF0000)
#define SMB_ENTRY_BASE   (0x05910000)

/*******************************************
 *
 *         INTERFACE DATA STRUCTURES
 *
 ******************************************/


typedef struct _SMBUS_CALLER_CONTEXT
{
    PEMCPAL_IRP_COMPLETION_ROUTINE	completionCallback;    // IO completion routine
    PVOID                   pCallerParm1;           // 
    ULONG                   callerParm2;           // 
} SMBUS_CALLER_CONTEXT_STRUCT, *PSMBUS_CALLER_CONTEXT_STRUCT;

#define SMBUS_INTERFACE_REQ_SIGNATURE    0x900DBEEF

typedef struct _SMBUS_CALLER_REQUEST
{
    ULONG                          signature;      // to make sure it's a valid request
    ULONG                          opCode;         // stores ioctl code
    PSMBUS_CALLER_CONTEXT_STRUCT   pCallerContext; // link to caller context
    EMCPAL_STATUS                  requestStatus;  // IO completion status

    /********************************************
     **** Maintain as last element of struct ****
     ********************************************/
    struct _SMB_REQUEST            smbRequest;     // SMBUS request (chain)
} SMBUS_CALLER_REQUEST_STRUCT, *PSMBUS_CALLER_REQUEST_STRUCT;

#define MEMBER_OF_SMBUS_CALLER_REQUEST(request)   \
    (SMBUS_CALLER_REQUEST_STRUCT *)((char *)request - (ULONG_PTR)(&(((SMBUS_CALLER_REQUEST_STRUCT *)(0))->smbRequest)))

/*******************************************
 *
 *         INTERFACE FUNCTION PROTOTYPES
 *
 ******************************************/

// This function initializes smbus interface library.
CSX_EXTERN_C EMCPAL_STATUS 
smbusInterfaceInit(void);

// This function cleans up smbus interface library.
CSX_EXTERN_C void 
smbusInterfaceFini(void);

// This is the interface for smbus read/write access.
CSX_EXTERN_C EMCPAL_STATUS 
smbusReadWrite(PSMBUS_CALLER_REQUEST_STRUCT pSmbusRequest);

// fill in caller context structure
CSX_EXTERN_C void
smbusInitCallerContext(PSMBUS_CALLER_CONTEXT_STRUCT pContext,
                       PEMCPAL_IRP_COMPLETION_ROUTINE       callbackRoutine,
                       PVOID                        pParm1,
                       ULONG                        parm2);

// fill in caller request structure
CSX_EXTERN_C void
smbusFillCallerRequest(PSMBUS_CALLER_REQUEST_STRUCT pRequest,
                       ULONG                        opCode,
                       SMB_DEVICE                   m_device,
                       ULONG                        m_offset,
                       ULONG                        m_count,
                       PUCHAR                       m_buff_p,
                       ULONG                        m_timeout,
                       PSMBUS_CALLER_CONTEXT_STRUCT pContext);

// chain smb request together
CSX_EXTERN_C void 
smbusChainRequests (PSMBUS_CALLER_REQUEST_STRUCT m_head_p, 
                    PSMBUS_CALLER_REQUEST_STRUCT m_new_p);


#endif //__SMBUS_INTERFACE__
