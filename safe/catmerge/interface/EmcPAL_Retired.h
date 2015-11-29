#ifndef _EMCPAL_RETIRED_H_
#define _EMCPAL_RETIRED_H_
//***************************************************************************
// Copyright (C) EMC Corporation 2012
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//++
// File Name:
//      EmcPAL_Retired.c
//
// Contents:
//      This file contains PAL functions that have been retired.  They are no
//      longer intended for general use, and have been replaced by CSX or other
//      platform layer implementations.  The definitions here exist purely for
//      backward compatibility reasons
//
// Retired Functions:
//      EmcpalDebugBreakPoint
//
//
// Revision History:
//     02/03/2012  Chris Gould Initial creation
//
//--

//++
// Include files
//--

#include "csx_ext.h"

//++
// End Includes
//--
//

//++
// Function:
//      EmcpalDebugBreakPoint()
//
// Description:
//      This function is a retired Emcpal abstraction for triggering a breakpoint
//      It is provided here for backward compatibility reasons.  Any future
//      code should use the CSX APIs.
//
// Arguments:
//       Nothing.
//
// Return Value:
//       Nothing.
//      
// Revision History:
//     04/27/2011  Manjit Singh Initial creation
//     02/03/2012 Chris Gould - retired API
//--
#define EmcpalDebugBreakPoint() CSX_BREAKPOINT_HANDLER(__FILE__, __LINE__)

//++
//.End File EmcPAL_Retired.h
//--

#endif /* _EMCPAL_RETIRED_H_ */

