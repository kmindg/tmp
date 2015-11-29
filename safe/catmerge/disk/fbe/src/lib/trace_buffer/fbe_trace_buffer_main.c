/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_trace_buffer_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the functions of a simple trace buffer.
 *
 * @version
 *   11/17/2011:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_trace_buffer.h"

/*!**************************************************************
 * fbe_trace_buffer_log()
 ****************************************************************
 * @brief
 *  This is a simple logging mechanism that is designed
 *  to have low impact on the timing of the system.
 *
 * @param arg0
 * @param arg1
 * @param arg2
 * @param arg3
 * @param arg4           
 *
 * @return fbe_status_t
 *
 * @author
 *  11/4/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_trace_buffer_log(fbe_trace_buffer_t *trace_buffer_p,
                                  fbe_trace_buffer_data_type_t arg0, 
                                  fbe_trace_buffer_data_type_t arg1, 
                                  fbe_trace_buffer_data_type_t arg2, 
                                  fbe_trace_buffer_data_type_t arg3, 
                                  fbe_trace_buffer_data_type_t arg4)
{
    if (trace_buffer_p->b_is_initialized == FBE_FALSE)
    {
        trace_buffer_p->b_is_initialized = FBE_TRUE;
        fbe_spinlock_init(&trace_buffer_p->spinlock);
    }

    fbe_spinlock_lock(&trace_buffer_p->spinlock);
    trace_buffer_p->ring_array[trace_buffer_p->index].arg0 = arg0;
    trace_buffer_p->ring_array[trace_buffer_p->index].arg1 = arg1;
    trace_buffer_p->ring_array[trace_buffer_p->index].arg2 = arg2;
    trace_buffer_p->ring_array[trace_buffer_p->index].arg3 = arg3;
    trace_buffer_p->ring_array[trace_buffer_p->index].arg4 = arg4;

    trace_buffer_p->index++;
    if (trace_buffer_p->index >= FBE_TRACE_BUFFER_MAX_ENTRIES)
    {
        trace_buffer_p->index = 0;
    }
    fbe_spinlock_unlock(&trace_buffer_p->spinlock);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_trace_buffer_log()
 ******************************************/
/*!**************************************************************
 * fbe_trace_buffer_destroy()
 ****************************************************************
 * @brief
 *  Destroy our trace buffer.
 *
 * @param trace_buffer_p - pointer to our buffer.               
 *
 * @return None.   
 *
 * @author
 *  11/21/2011 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_trace_buffer_destroy(fbe_trace_buffer_t *trace_buffer_p)
{
    fbe_spinlock_destroy(&trace_buffer_p->spinlock);
}
/******************************************
 * end fbe_trace_buffer_destroy()
 ******************************************/

/*!**************************************************************
 * fbe_trace_buffer_init()
 ****************************************************************
 * @brief
 *  Initialize our trace buffer.
 *
 * @param trace_buffer_p - pointer to our buffer.               
 *
 * @return None.   
 *
 * @author
 *  11/21/2011 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_trace_buffer_init(fbe_trace_buffer_t *trace_buffer_p)
{
    fbe_spinlock_init(&trace_buffer_p->spinlock);
}
/**************************************
 * end fbe_trace_buffer_init()
 **************************************/
/*************************
 * end file fbe_trace_buffer_main.c
 *************************/
