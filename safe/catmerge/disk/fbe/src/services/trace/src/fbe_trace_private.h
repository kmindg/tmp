#ifndef __FBE_TRACE_PRIVATE_H__
#define __FBE_TRACE_PRIVATE_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_trace_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains local definitions for the trace service.
 *
 * @version
 *   7/15/2010:  Created. Rob Foley
 *
 ***************************************************************************/

fbe_status_t fbe_trace_stop_system(void);
void CallInt3(void);

/*************************
 * end file fbe_trace_private.h
 *************************/

#endif /* end __FBE_TRACE_PRIVATE_H__ */

