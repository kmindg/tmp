/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_trace_panic.c
 ***************************************************************************
 *
 * @brief
 *  This file contains trace service code for panicing the system.
 * 
 *  We intentionally isolate this code here since it contains #ifdefs.
 * 
 *  The architecture plan is for there to be only one place in the system
 *  to panic, and that is inside the trace service.
 *
 * @version
 *  7/15/2010:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe_trace_private.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/* We define this here since we only want one place in the system for the system to panic.
 * At present only the trace service will be allowed to panic in fbe.
 */

fbe_status_t fbe_trace_stop_system(void)
{
#if !(defined(UMODE_ENV) || defined(SIMMODE_ENV)) 
	CSX_BREAK(); 
	/* blue screen at this point. Suppress Prefast warning 311 which is a 
	 * suggestion to not use KeBugCheckEx() which is proper here.
	 */
#pragma warning(disable: 4068)
#pragma prefast(disable: 311)
/*! @todo need to add a proper panic code here.
 */
	EmcpalBugCheck(0, 0, 0, /* panic code */ __LINE__, 0);
#pragma prefast(default: 311)
#pragma warning(default: 4068)
#else
    fbe_DbgBreakPoint();
#endif
    return FBE_STATUS_OK;
}

/*************************
 * end file fbe_trace_panic.c
 *************************/
