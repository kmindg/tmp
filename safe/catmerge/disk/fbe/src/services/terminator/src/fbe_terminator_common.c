/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_terminator_common.c
 ***************************************************************************
 *
 *  Description
 *      This file contains general common utilities for all parts of the 
 *  Termiantor to use
 *
 *
 *  History:
 *      01/05/10    guov - Created
 *      11/16/10    miaot  Updated for fbe_terminator_allocate_memory()
 *                          and fbe_terminator_free_memory()
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe_base.h"
#include "fbe_trace.h"

#define FBE_TERM_POOL_TAG   'mret'

/*********************************************************************
 *            fbe_terminator_allocate_memory ()
 *********************************************************************
 *
 *  Description: memory allocation, ready to be cahnged for csx if needed
 *
 *  Arguments:
 *      NumberOfBytes
 *          number of bytes of the memory to be allocated
 *
 *  Return value:
 *      allocated memory pointer
 *
 *  History:
 *      10/10/08    sharel Created
 *      01/05/10    guov   Copy from fbe_api
 *      11/16/10    miaot  Use fbe_allocate_nonpaged_pool_with_tag() instead of
 *                          fbe_terminator_allocate_memory().
 *********************************************************************/
void * fbe_terminator_allocate_memory(fbe_u32_t NumberOfBytes)
{
    void *mem_ptr = fbe_allocate_nonpaged_pool_with_tag( NumberOfBytes, FBE_TERM_POOL_TAG);

#if 0 /* This is performance problem */
    if (mem_ptr != NULL) {
        fbe_zero_memory(mem_ptr, NumberOfBytes);
    }
#endif

    return mem_ptr;
}

/*********************************************************************
 *            fbe_terminator_free_memory()
 *********************************************************************
 *
 *  Description: memory freeing, ready to be changed for csx if needed
 *
 *  Arguments:
 *      mem_ptr
 *          number of bytes of the memory to be freed
 *
 *  Return value:
 *      N/A
 *
 *  History:
 *      10/10/08    sharel Created
 *      01/05/10    guov   Copy from fbe_api
 *      11/16/10    miaot  Use fbe_release_nonpaged_pool_with_tag() instead of
 *                          fbe_terminator_free_memory().
 *********************************************************************/
void fbe_terminator_free_memory(void * mem_ptr)
{
    fbe_release_nonpaged_pool_with_tag(mem_ptr, FBE_TERM_POOL_TAG);
}

void fbe_terminator_sleep (fbe_u32_t msec)
{
	fbe_thread_delay(msec);
}

void terminator_trace(fbe_trace_level_t trace_level,
                 fbe_trace_message_id_t message_id,
                 const char * fmt, ...)
{
    fbe_trace_level_t default_trace_level;
    fbe_trace_level_t service_trace_level;
    va_list args;

    service_trace_level = default_trace_level = fbe_trace_get_default_trace_level();
    if (trace_level > service_trace_level) {
        return;
    }
    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_SERVICE,
                     FBE_SERVICE_ID_TERMINATOR,
                     trace_level,
                     message_id,
                     fmt, 
                     args);
    va_end(args);
}

void terminator_trace_report(fbe_trace_level_t trace_level,
                             fbe_u32_t message_id,
                             const fbe_u8_t * fmt, 
                             va_list args)
{
    fbe_trace_report(FBE_COMPONENT_TYPE_SERVICE,
                     FBE_SERVICE_ID_TERMINATOR,
                     trace_level,
                     message_id,
                     fmt, 
                     args);
}
