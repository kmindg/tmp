#ifndef _IPMI_STATIC_LIB_H_
#define _IPMI_STATIC_LIB_H_

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Copyright (C) EMC Corporation, 2012
// All rights reserved.
// Licensed material -- property of EMC
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//++
// File Name:
//      ipmiStaticLib.h
//
// Contents:
//      This interface is specifically meant for providing NvRam access on
//      the dump path.  All other cases should use the IPMI ioctl interface.
//       
// Exported:
//      ipmiLocalRequestProcess
//
// Revision History:
//      06-29-2012  Sri             Created. 
//
// Notes:
//
//--

CSX_CDECLS_BEGIN

//++
// Includes
//--
#include "environment.h"

//++
//.End Includes
//--

#if 0
//++
// Data Types
//--

//++
// Name:
//      IPMI_INTERFACE_STATE
//
// Description:
//      Enumeration for the states of the KCS interface.
//
//--
enum IPMI_INTERFACE_STATE
{
    IPMI_INTERFACE_STATE_UNINITIALIZED      = 0,
    IPMI_INTERFACE_STATE_OK                 = 1,
    IPMI_INTERFACE_STATE_DEAD               = 2,

    // Add more state above here.
    IPMI_INTERFACE_STATE_INVALID            = 0xFF
};

//++
//.End Data Structures
//--

//++
// Globals
//--
extern IPMI_INTERFACE_STATE ipmiInterfaceState;

//--
//.End Globals
//++

//++
// Exported Functions
//--

extern BOOLEAN ipmiHasRecvMsg (VOID);
extern EMCPAL_STATUS ipmiLocalInterfaceInit (VOID);
extern struct _IPMI_BMC_REQUEST* ipmiRequestAllocateBmcRequest (VOID);
extern struct _IPMI_BMC_RESPONSE* ipmiRequestAllocateBmcResponse (VOID);
extern VOID ipmiRequestFreeBmcRequest (struct _IPMI_BMC_REQUEST* PBmcRequest);
extern VOID ipmiRequestFreeBmcResponse (struct _IPMI_BMC_RESPONSE* PBmcResponse);
#endif // 0

//++
// Function:
//      ipmiLocalRequestProcess
//
// Description:
//      This function process the incoming IPMI request.  It writes the IPMI command
//      (request) to the BMC and reads the response from the BMC, both through the KCS
//      interface.  If there's an error returned by the KCS interface, it attempts to
//      abort the request (with no retries) and returns a failure status to the caller.
//      A different status is returned if the abort fails.
//
// Input:
//      PIpmiRequest        - Buffer for the incoming IPMI request.
//      InputLength         - Length of the request buffer in bytes.
//      PIpmiResponse       - Buffer for the IPMI response that is returned to the caller.
//      OutputLength        - Length of the response buffer in bytes.
//
// Output:
//      PInfo               - Number of bytes returned in the response buffer.
//
// Returns:
//      EMCPAL_STATUS_SUCCESS, if the request/response completes successfully.
//      EMCPAL_STATUS_INVALID_PARAMETER, if any of the arguments are invalid.
//      IPMI_ERROR_INTERFACE, if the KCS interface returned an error, but the interface
//                            is funcitoning properly.
//      IPMI_ERROR_INTERFACE_DEAD, if the KCS interface to the BMC is dead (hardware error).
//                                 In this case, the BMC would have to be reset.
//
//--
extern
EMCPAL_STATUS 
ipmiLocalRequestProcess
(
    IN  struct _IPMI_REQUEST*   PIpmiRequest,
    IN  ULONG32                 InputLength,
    IN  struct _IPMI_RESPONSE*  PIpmiResponse,
    IN  ULONG32                 OutputLength,
    OUT PULONG32                PInfo
);

//++
//.End Exported Functions
//--


CSX_CDECLS_END

#endif // _IPMI_STATIC_LIB_H_
