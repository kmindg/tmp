#ifndef _DLU_STATUSCODES_H_
#define _DLU_STATUSCODES_H_

//***************************************************************************
// Copyright (C) Data General Corporation 1989-1998
// All rights reserved.
// Licensed material -- property of Data General Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      DluStatusCodes.h
//
// Contents:
//      Status codes for the DLU subsystem
//
// Revision History:
//      
//      11-Jan-00   Login    Created.
//
//--

#ifdef __cplusplus
extern "C" {
#endif

//#include <ntdef.h>

//++
// Macro:
//      DLU_COMPONENT_CODE
//
// Description:
//      The Dlu Componet code, as viewed by K10LogService
//
// Revision History:
//      08-Mar-2000   MWagner  Created.
//
//--
#define DLU_COMPONENT_CODE                    (0x71120000)
// .End DLU_COMPONENT_CODE


//++
// Macro:
//      DLU_FACILITY_CODE
//
// Description:
//      A base value for the DLU Status codes in drive space
//
// Revision History:
//      11-Jan-99   MWagner  Created.
//
//--
#define DLU_FACILITY_CODE                    (0x0FFF0000 & DLU_COMPONENT_CODE) 
// .End DLU_FACILITY_CODE

//++
// K10DistributedLockUtilitesMessages
//
// All curent DLU Status codes are generated using MC, which
// creates the "K10DistributedLockUtilitesMessasages.h" file.
//--
#include "k10distributedlockutilitiesmessages.h"


//++
// Macro:
//      DLU_STATUS
// Description:
//      this macro creates a DLU status
//
// Arguments:
//      ULONG     Severity:  ERROR_SEVERITY_WARNING
//                           ERROR_SEVERITY_SUCCESS
//                           ERROR_SEVERITY_INFORMATIONAL
//                           ERROR_SEVERITY_ERROR
//      ULONG     Error:     The error to be encoded
//
// Return Value
//      XXX       a DLU status
//
// Revision History:
//      11-Jan-99   MWagner  Created.
//
//--
#define DLU_STATUS(Severity, Error)                    (\
    (ULONG) (Severity |                                 \
     APPLICATION_ERROR_MASK |                           \
     DLU_STATUS_FACILITY |                              \
     Error)                                             \
                                                        )
// .End DLU_STATUS

#ifdef __cplusplus
};
#endif

#endif // __DLU_STATUSCODES__
