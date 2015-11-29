
#ifndef FBE_DEST_SERVICE_PRIVATE_H
#define FBE_DEST_SERVICE_PRIVATE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_dest_service.h
 ***************************************************************************
 *
 * @brief
 *  This file contains local definitions for the dest service.
 *
 * History:
*   03/06/2012  Created. kothal
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h" 
#include "fbe_base_service.h" 
#include "fbe/fbe_dest.h" 

/*!*******************************************************************
 * @struct fbe_dest_service_t
 *********************************************************************
 * @brief
 *  This is the definition of the dest service, which is used to
 *  inject error for physical objects.
 *
 *********************************************************************/
typedef struct fbe_dest_service_s
{
    fbe_base_service_t base_service;

    /*! Lock to protect our queues.
     */
    fbe_spinlock_t   dest_error_queue_lock;

    /*! This is the list of error record, 
     * which track all error to be injected. 
     */
    fbe_queue_head_t dest_error_queue;
}fbe_dest_service_t;

void fbe_dest_service_trace(fbe_trace_level_t trace_level,
                            fbe_trace_message_id_t message_id,
                            const char * fmt, ...)  __attribute__ ((format(__printf_func__, 3, 4)));

/*************************
 * end file fbe_dest_service.h
 *************************/
#endif /* FBE_DEST_SERVICE_PRIVATE_H */
