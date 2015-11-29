//***************************************************************************
// Copyright (C) Data General Corporation 1989-1999
// All rights reserved.
// Licensed material -- property of Data General Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

#ifndef _DLU_FLAGS_H_
#define _DLU_FLAGS_H_
//++
// File Name:
//      DlsFlags.h
//
// Contents:
//      DLU_FLAG
//      Macros that define DLS flags
//
//
// Exported:
//
// Revision History:
//      4/8/1999    MWagner   Created
//
//--

#ifdef __cplusplus
extern "C" {
#endif

//++
// Include Files
//--

//++
// Interface
//--

#include "K10LayeredDefs.h"

//++
// Implementation
//--

//++
// End Includes
//--

//++
// Macro:
//      DLU_FLAG_SUB_SEMAPHORE
//
// Description:
//      The "base" value for a DLS flag
//
// Revision History:
//      4/8/1999   MWagner    Created.
//
//--
#define DLU_FLAG_SUB_SEMAPHORE       0x00000000
// .End DLU_FLAG_SUB_SEMAPHORE

//++
// Macro:
//      DLU_FLAG_SUB_MISC
//
// Description:
//      This flag is "consumed" by the DLS Valet
//      Subssytem- 5 is for V for Valet.
//
// Revision History:
//      4/8/1999   MWagner    Created.
//
//--
#define DLU_FLAG_SUB_MISC      0x10000000
// .End DLU_FLAG_SUB_MISC

//++
// Macro:
//      DLU_FLAG
//
// Description:
//      Creates a DLU Flag from SUB, and
//      a value.
//      The top two byte is used to encode the
//      fact that this is in fact a DLS flag,
//      and which subsystem is intended to process
//      the flag.
//
//      For example, a Flag of 0x2000001 is a
//      DLS Flag understood by the DLS Clerk
//      Subsystem.
//
// Arguments:
//                      DLU_FLAG_SUB_SEMAPHORE
//                      DLU_FLAG_SUB_MISC
//
// Revision History:
//      4/8/1999   MWagner    Created.
//
//--
#define DLU_FLAG(SUB, VALUE)      ((K10_DLU_FACILITY | SUB | VALUE))
// .End DLU_FLAG

//++
// Semaphore Flags
//

#define DLU_SEMAPHORE_PEER_NO_WAIT \
                    DLU_FLAG(DLU_FLAG_SUB_SEMAPHORE, 0x00000001)

#define DLU_SEMAPHORE_LOCAL_NO_WAIT \
                    DLU_FLAG(DLU_FLAG_SUB_SEMAPHORE, 0x00000010)
//++
// Misc Flags
//

#ifdef __cplusplus
};
#endif

// End DLU_FLAGS_H

#endif
