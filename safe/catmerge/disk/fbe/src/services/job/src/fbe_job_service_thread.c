/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_job_service_thread.c
 ***************************************************************************
 *
 * @brief
 *  This file contains job service functions that process a queued element
 *  from either the Recovery queue or the Creation queue.
 *  
 *  The main goal of this file is to dequeue a request and run the request
 *  thru the job registration functions
 *
 *
 * @version
 *  01/05/2010 - Created. Jesus Medina
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe_base_object.h"
#include "fbe_transport_memory.h"
#include "fbe_base_service.h"
#include "fbe_topology.h"
#include "fbe_job_service_private.h"
#include "fbe_job_service.h"
#include "fbe_spare.h"
#include "fbe_job_service_operations.h"
#include "../test/fbe_job_service_test_private.h"

/*************************
 *    INLINE FUNCTIONS 
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/* Accessor methods for the flags field.
*/
fbe_bool_t fbe_job_service_thread_is_flag_set(fbe_job_service_thread_flags_t flag)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    return ((job_service_p->thread_flags & flag) == flag);
}

void fbe_job_service_thread_init_flags()
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 

    /* Set the thread to RUN only if active. For Passive, it is comming up,
     * so set it to SYNC mode to synchronize the job queue. Once done, it
     * will set it to RUN mode. 
     */ 
    job_service_p->thread_flags = (fbe_job_service_cmi_is_active()) ? 
        FBE_JOB_SERVICE_THREAD_FLAGS_RUN : FBE_JOB_SERVICE_THREAD_FLAGS_SYNC;

    return;
}

void fbe_job_service_thread_set_flag(fbe_job_service_thread_flags_t flag)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    job_service_p->thread_flags = flag;
    return;
}

void fbe_job_service_thread_clear_flag(fbe_job_service_thread_flags_t flag)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    job_service_p->thread_flags  &= ~flag;
    return;
}

/*!**************************************************************
 * job_service_thread_func()
 ****************************************************************
 * @brief
 * runs the job control thread
 *
 * @param context - unreferenced value             
 *
 * @return fbe_status_t - status of call to start job service thread
 *
 * @author
 * 10/01/2009 - Created. Jesus Medina
 *
 ****************************************************************/

void job_service_thread_func(void * context)
{
    EMCPAL_STATUS nt_status;
	fbe_database_state_t	db_state = FBE_DATABASE_STATE_INVALID;

    FBE_UNREFERENCED_PARAMETER(context);

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s job_service_thread_func\n", __FUNCTION__);

	fbe_thread_set_affinity(&fbe_job_service.thread_handle, 0x1);

	fbe_transport_set_cpu_id(fbe_job_service.job_packet_p, 0);

    while(1)    
    {
        if ((!fbe_job_service_thread_is_flag_set(FBE_JOB_SERVICE_THREAD_FLAGS_RUN)) &&
            (!fbe_job_service_thread_is_flag_set(FBE_JOB_SERVICE_THREAD_FLAGS_SYNC)))
        {
            break;
        }

        /* If the state of the DB service is not READY, do not process the job queues.
         * This is required, coz the DB service comes up after job service, and the
         * PASSIVE should wait for the DB to be READY before starting to process 
         * the jobs - where ACTIVE goes down after sync'ing up all the jobs to PASSIVE and 
         * the DB is not updated on PASSIVE side... PASSIVE should know the PVD's and 
         * any object's that are created and stored in DB/disk by ACTIVE before actually 
         * processing the jobs.. When ACTIVE goes down, PASSIVE picks up all the info 
         * from the disk, loads it on its own and transitions to READY.  
         */ 
        if(fbe_job_service_get_database_state() != FBE_DATABASE_STATE_READY && fbe_job_service_get_database_state() != FBE_DATABASE_STATE_CORRUPT)
        {
            fbe_thread_delay(1000);
            continue;
        }

        nt_status = fbe_semaphore_wait(&fbe_job_service.job_control_queue_semaphore, NULL);

        /* To prevent a corner case that job can also sneak in before the database CMI
          callback function has time to set DB service state as not ready, we should 
          check if the special sp state value is changed in job service CMI callback
          function. Until the state is set as Active, we can release the job thread 
          to continue...
        */
        if(fbe_job_service.sp_state_for_peer_lost_handle != FBE_CMI_STATE_ACTIVE)
        {
            continue;
        }

		/*when we get here some long time could have passed since we last made the check above.
		we could get here as the peer just died which is a bit tricky becuse the DB service may be just setting up 
		ther persit lun and doing other things. If we immedietly check if the DB state is ready, we can sneak in before 
		it had the time to set it's state to a not ready state while it's setting up everything.
		to prevent that, we'll check if the peer just died, and if so, we'll give the db some time to change it's state.
		we may have been on the active side all along but this would do no harm*/
		if (FBE_IS_TRUE(fbe_job_service.peer_just_died) && fbe_job_service_thread_is_flag_set(FBE_JOB_SERVICE_THREAD_FLAGS_RUN)) {

			fbe_job_service.peer_just_died = FBE_FALSE;/*no need to interlock, we get this only once*/

			job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "%s peer died, waiting for DB to be ready\n", __FUNCTION__);

            fbe_thread_delay(3000);/*let's wait before we bother DB...*/
			do{
				fbe_thread_delay(1000);
				db_state = fbe_job_service_get_database_state();
			}while (fbe_job_service_thread_is_flag_set(FBE_JOB_SERVICE_THREAD_FLAGS_RUN) && db_state != FBE_DATABASE_STATE_READY && db_state!= FBE_DATABASE_STATE_CORRUPT);

            /*recheck database state here because the rolling check above may quit due to job flag changes from
             *RUN TO SYNC. This happens when peer SP boots and asks us to sync jobs to it. 
             *Our database may not be READY here. So check it again*/
            db_state = fbe_job_service_get_database_state();
            while(db_state != FBE_DATABASE_STATE_READY && db_state != FBE_DATABASE_STATE_CORRUPT)
            {
                fbe_thread_delay(1000);
                db_state = fbe_job_service_get_database_state();
            }
            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "%s peer died, DB is ready, going on\n", __FUNCTION__);
		}

        while((!fbe_queue_is_empty(&fbe_job_service.recovery_q_head)) ||
                ((!fbe_queue_is_empty(&fbe_job_service.creation_q_head))))
        {
            /* Process items on the recovery queue first
            */
            while ((!fbe_queue_is_empty(&fbe_job_service.recovery_q_head))  &&
                    (fbe_job_service_get_recovery_queue_access() == FBE_TRUE))
            {
                fbe_job_service_process_recovery_queue();
            }

            /* then process an item on the creation queue next
            */
            if ((!fbe_queue_is_empty(&fbe_job_service.creation_q_head))  &&
                    (fbe_job_service_get_creation_queue_access() == FBE_TRUE))
            {
                fbe_job_service_process_creation_queue();
            }
        }
    }

    fbe_job_service_thread_set_flag(FBE_JOB_SERVICE_THREAD_FLAGS_DONE); 
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}
/*******************************
 * end job_service_thread_func()
 *******************************/





