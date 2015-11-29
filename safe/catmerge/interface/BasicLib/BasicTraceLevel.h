/* $CCRev: ./array_sw/catmerge/FabricArray/interface/BasicTrace.h@@/main/daytona_mr1/1 */
/************************************************************************
 *
 *  Copyright (c) 2005 EMC Corporation.
 *  All rights reserved.
 *
 *  ALL RIGHTS ARE RESERVED BY EMC CORPORATION.  ACCESS TO THIS
 *  SOURCE CODE IS STRICTLY RESTRICTED UNDER CONTRACT. THIS CODE IS TO
 *  BE KEPT STRICTLY CONFIDENTIAL.
 *
 *  UNAUTHORIZED MODIFICATIONS OF THIS FILE WILL VOID YOUR SUPPORT
 *  CONTRACT WITH EMC CORPORATION.  IF SUCH MODIFICATIONS ARE FOR
 *  THE PURPOSE OF CIRCUMVENTING LICENSING LIMITATIONS, LEGAL ACTION
 *  MAY RESULT.
 *
 ************************************************************************
 *
 *  2005-11-10 08:47pm  fabx-r5036       jahern    .../daytona_mr1/1
 *
 ************************************************************************/

#ifndef __BasicTraceLevel__
#define __BasicTraceLevel__

# include "ktrace.h"
# include <stdarg.h>
//++
// File Name:
//      BasicTrace.h
//
// Contents:
//      This file contains the BasicTraceLevel type.
//

// The BasicTraceLevel enum classifies Trace statements, so that they can be filtered and
// routed to trace buffers by that classification.
enum BasicTraceLevel {
    BTRC_BUG        = 1, // Indicates a software failure that requires engineering analysis. 
    BTRC_ERROR        = 2, // Indicates a serious issue
    BTRC_WARN         = 3, // Indicates a noteworthy event
    BTRC_START_INFO   = 5, // Indicates a startup event. The total number of these should be small.
    BTRC_INFO         = 6, // Indicates an informational message.
    BTRC_DEBUG_TERSE  = 12, // Should be logged when doing general testing and debugging
    BTRC_DEBUG_VERBOSE = 14, // Should only be logged when doing detailed debugging.
    BTRC_MAX_LEVEL = 15
};



#endif // __BasicTraceLevel__
