//***************************************************************************
// Copyright (C) Data General Corporation 1989-1999
// All rights reserved.
// Licensed material -- property of Data General Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

#ifndef _DLS_FLAGS_H_
#define _DLS_FLAGS_H_
//++
// File Name:
//      DlsFlags.h
//
// Contents:
//      DLS_FLAG
//      Macros that define DLS flags
//
//
// Exported:
//
// Revision History:
//      6/30/2001   CVaidya   Added a flag to the Valet
//                            Subsystem for a close by name
//                            operation
//      4/8/1999    MWagner   Created
//
//--

//++
// Include Files
//--

//++
// Interface
//--

//++
// Implementation
//--

//++
// End Includes
//--

//++
// Macro:
//      DLS_FLAG_SUB_CLIENT
//
// Description:
//      The "base" value for a DLS flag
//
// Revision History:
//      4/8/1999   MWagner    Created.
//
//--
#define DLS_FLAG_SUB_CLIENT       0x00000000
// .End DLS_FLAG_SUB_CLIENT

//++
// Macro:
//      DLS_FLAG_SUB_BUTLER
//
// Description:
//      This flag is "consumed" by the DLS Butler
//      Subsystem- "B" is for Butler
//
// Revision History:
//      4/8/1999   MWagner    Created.
//
//--
#define DLS_FLAG_SUB_BUTLER     0x10000000
// .End DLS_FLAG_SUB_BUTLER


//++
// Macro:
//      DLS_FLAG_SUB_CLERK
//
// Description:
//      This flag is "consumed" by the DLS Clerk
//      Subsystem- "C" is for Clerk
//
// Revision History:
//      4/8/1999   MWagner    Created.
//
//--
#define DLS_FLAG_SUB_CLERK      0x20000000
// .End DLS_FLAG_SUB_CLERK

//++
// Macro:
//      DLS_FLAG_SUB_VALET
//
// Description:
//      This flag is "consumed" by the DLS Valet
//      Subssytem- 5 is for V for Valet.
//
// Revision History:
//      4/8/1999   MWagner    Created.
//
//--
#define DLS_FLAG_SUB_VALET      0x40000000
// .End DLS_FLAG_SUB_VALET


//++
// Macro:
//      DLS_FLAG_SUB_MISC
//
// Description:
//      This flag is "consumed" by the DLS Valet
//      Subssytem- 5 is for V for Valet.
//
// Revision History:
//      4/8/1999   MWagner    Created.
//
//--
#define DLS_FLAG_SUB_MISC      0x80000000
// .End DLS_FLAG_SUB_MISC

//++
// Macro:
//      DLS_FLAG
//
// Description:
//      Creates a DLS Flag from SUB, and
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
//      LOGICAL SUB     DLS_FLAG_SUB_BUTLER
//                      DLS_FLAG_SUB_CLERK
//                      DLS_FLAG_SUB_VALET
//                      DLS_FLAG_SUB_CLIENT
//                      DLS_FLAG_SUB_MISC
//
// Revision History:
//      4/8/1999   MWagner    Created.
//
//--
#define DLS_FLAG(SUB, VALUE)      ((SUB | VALUE))
// .End DLS_FLAG

//++
// Client Flags
//

#define DLS_CLIENT_CONVERT_LOCK_NO_WAIT \
                    DLS_FLAG(DLS_FLAG_SUB_CLIENT, 0x00000001)

//++
// Butler Flags
//

#define DLS_BUTLER_CREATE_LOCK_WITH_CABAL_ID \
                    DLS_FLAG(DLS_FLAG_SUB_BUTLER, 0x00000001)

#define DLS_BUTLER_ERSATZ_OPERATION \
                    DLS_FLAG(DLS_FLAG_SUB_BUTLER, 0x00000010)
//++
// Clerk Flags
//--


//++
// Valet Flags
//--

#define DLS_VALET_RETURN_ZOMBIE_FROM_FIND  \
                    DLS_FLAG(DLS_FLAG_SUB_VALET, 0x00000001)

#define DLS_VALET_CLOSE_LOCK_BY_NAME \
                    DLS_FLAG(DLS_FLAG_SUB_VALET, 0x00000010)

// End DLS_FLAGS_H

#endif
