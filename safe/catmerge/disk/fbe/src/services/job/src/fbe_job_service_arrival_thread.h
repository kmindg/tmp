#ifndef FBE_JOB_SERVICE_ARRIVAL_THREAD_H
#define FBE_JOB_SERVICE_ARRIVAL_THREAD_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_job_service_arrival_thread.h
 ***************************************************************************
 *
 * @brief
 *  This file contains local definitions for the job service thread
 *
 * @version
 *   08/24/2010:  Created. Jesus Medina
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_winddk.h"
#include "fbe_base_object.h"
#include "fbe_base_service.h"
#include "fbe_service_manager.h"
#include "fbe_block_transport.h"
#include "fbe_job_service.h"
#include "fbe/fbe_sector.h"
#include "fbe/fbe_random.h"

//void job_service_arrival_thread_func(void * context);

/*!*******************************************************************
 * @enum job_service_arrival_thread_flag_e
 *********************************************************************
 * @brief
 *  This is the set of flags used in the thread.
 *
 *********************************************************************/
typedef enum job_service_arrival_thread_flag_e
{
    FBE_JOB_SERVICE_ARRIVAL_THREAD_FLAGS_INVALID  =  0x00,  /*!< No flags are set. */
    FBE_JOB_SERVICE_ARRIVAL_THREAD_FLAGS_RUN      =  0x01,  /*!< running thread.   */
    FBE_JOB_SERVICE_ARRIVAL_THREAD_FLAGS_STOPPED  =  0x02,  /*!< Thread is stopped. */
    FBE_JOB_SERVICE_ARRIVAL_THREAD_FLAGS_DONE     =  0x04,  /*!< Thread is done. */
    FBE_JOB_SERVICE_ARRIVAL_THREAD_FLAGS_SYNC     =  0x08,  /*!< Thread is synchronizing with peer. */
    FBE_JOB_SERVICE_ARRIVAL_THREAD_FLAGS_LAST
}
fbe_job_service_arrival_thread_flags_t;

void fbe_job_service_arrival_thread_copy_queue_element(fbe_job_queue_element_t *out_job_queue_element_p,  
                                                       fbe_job_queue_element_t *in_job_queue_element);

fbe_status_t fbe_job_service_arrival_thread_validate_queue_element(fbe_packet_t * packet_p,
                                                                   fbe_job_type_t *job_type, 
                                                                   fbe_job_control_queue_type_t *queue_type);

void fbe_job_service_arrival_thread_add_element_to_queue(fbe_job_queue_element_t *job_queue_element);

void fbe_job_service_arrival_thread_increment_sync_element(void);

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/* Accessor methods for the flags field.
*/
extern fbe_bool_t fbe_job_service_arrival_thread_is_flag_set(fbe_job_service_arrival_thread_flags_t flag);
extern void fbe_job_service_arrival_thread_init_flags(void);
extern void fbe_job_service_arrival_thread_set_flag(fbe_job_service_arrival_thread_flags_t flag);
extern void fbe_job_service_arrival_thread_clear_flag(fbe_job_service_arrival_thread_flags_t flag);

/*******************************************
 * end file fbe_job_service_arrival_thread.h
 ******************************************/

#endif /* end FBE_JOB_SERVICE_ARRIVAL_THREAD_H */



