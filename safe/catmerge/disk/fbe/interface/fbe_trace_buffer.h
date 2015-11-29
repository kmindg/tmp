#ifndef __FBE_TRACE_BUFFER_H__
#define __FBE_TRACE_BUFFER_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_trace_buffer.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions for the simple ring buffer.
 *
 * @version
 *   11/17/2011:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"

/* This is a simple ring mechanism that is designed to have low impact
 * on the timing of the system. 
 */
#define fbe_trace_buffer_data_type_t fbe_u64_t

/*!*******************************************************************
 * @def FBE_TRACE_BUFFER_MAX_ENTRIES
 *********************************************************************
 * @brief Trace buffer maximum size.
 *
 *********************************************************************/
#define FBE_TRACE_BUFFER_MAX_ENTRIES 50000

/*!*******************************************************************
 * @struct fbe_trace_buffer_data_t
 *********************************************************************
 * @brief This is a simple ring structure appropriate for logging where
 *        we do not want to affect the timing.
 *
 *********************************************************************/
typedef struct fbe_trace_buffer_data_s
{
    fbe_trace_buffer_data_type_t arg0;
    fbe_trace_buffer_data_type_t arg1;
    fbe_trace_buffer_data_type_t arg2;
    fbe_trace_buffer_data_type_t arg3;
    fbe_trace_buffer_data_type_t arg4;
}
fbe_trace_buffer_data_t;

/*!*******************************************************************
 * @struct fbe_trace_buffer_t
 *********************************************************************
 * @brief This is a simple ring structure appropriate for logging where
 *        we do not want to affect the timing.
 *
 *********************************************************************/
typedef struct fbe_trace_buffer_s
{
    fbe_bool_t b_is_initialized;
    fbe_u32_t index;
    fbe_spinlock_t spinlock;
    fbe_trace_buffer_data_t ring_array[FBE_TRACE_BUFFER_MAX_ENTRIES];
}
fbe_trace_buffer_t;

/* Trace buffer library API
 */
void fbe_trace_buffer_destroy(fbe_trace_buffer_t *trace_buffer_p);
void fbe_trace_buffer_init(fbe_trace_buffer_t *trace_buffer_p);
fbe_status_t fbe_trace_buffer_log(fbe_trace_buffer_t *trace_buffer_p,
                                  fbe_trace_buffer_data_type_t arg0, 
                                  fbe_trace_buffer_data_type_t arg1, 
                                  fbe_trace_buffer_data_type_t arg2, 
                                  fbe_trace_buffer_data_type_t arg3, 
                                  fbe_trace_buffer_data_type_t arg4);
/*************************
 * end file fbe_trace_buffer.h
 *************************/

#endif /* end __FBE_TRACE_BUFFER_H__ */
