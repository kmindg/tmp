/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_sep_shim_main_kernel.c
 ***************************************************************************
 *
 *  Description
 *      Kernel implementation for the BAM
 *      
 *
 *  History:
 *      09/29/10    sharel - Created
 *    
 ***************************************************************************/
#include <ntddk.h>
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_trace_interface.h"
#include "fbe/fbe_library_interface.h"
#include "fbe_service_manager.h"
#include "fbe_topology.h"
#include "fbe/fbe_queue.h"
#include "fbe_sep_shim_private_interface.h"
#include "fbe_sep_shim_private_ioctl.h"
#include "fbe/fbe_sep.h"
#include "fbe/fbe_memory.h"
#include "flare_ioctls.h"
#include "activity_mgr_interface.h"

/**************************************
                Local variables
**************************************/
static activity_mgr_job_block_t		bam_job_block;/*we need only one*/
static activity_mgr_client_api_t	sep_client_api;

/*******************************************
                Local functions
********************************************/
static void sep_shim_background_activity_mgr_callback(activity_mgr_job_block_t *job_block_p,
                                                      activity_mgr_credit_scale_t job_rate_scale,
                                                      activity_mgr_job_io_tag_t job_io_tag);

/*********************************************************************
 *            fbe_sep_shim_init_background_activity_manager_interface ()
 *********************************************************************
 *
 *  Description: Initialize the sep shim BAM interface
 *
 *	Inputs: 
 *
 *  Return Value: 
 *
 *    
 *********************************************************************/
void fbe_sep_shim_init_background_activity_manager_interface (void)
{
    activity_mgr_status_e 			status = ACTIVITY_MGR_STATUS_ERROR_GENERIC;

    /*this is the callback we get when the BAM decides to change our credits*/
    sep_client_api.credits_available_for_job = sep_shim_background_activity_mgr_callback;

    bam_job_block.client_api = sep_client_api;
    bam_job_block.job_type = ACTIVITY_MGR_JOB_ID_RAID;

    /*this job will actually run forever and BAM will from some time to time change it's credits via the callback*/
    status = activity_mgr_sched_a_job_and_update_job_block(&bam_job_block,
                                                           ACTIVITY_MGR_JOB_PRIORITY_SYSTEM);

    if (status != ACTIVITY_MGR_STATUS_SUCCESS) {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_WARNING, "Failed to initialize connection to Background Activity Manager\n");
        return;
    }

    fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "Connected to Background Activity Manager\n");
    
}

static void sep_shim_background_activity_mgr_callback(activity_mgr_job_block_t *job_block_p,
                                                      activity_mgr_credit_scale_t job_rate_scale,
                                                      activity_mgr_job_io_tag_t job_io_tag)
{
    fbe_scheduler_set_scale(job_rate_scale);
    fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "%s: Background Activity Manager set scale: %d\n", 
        __FUNCTION__, job_rate_scale);
}

void fbe_sep_shim_destroy_background_activity_manager_interface (void)
{
    activity_mgr_status_e 			status = ACTIVITY_MGR_STATUS_ERROR_GENERIC;

    status = activity_mgr_unschedule_a_job(&bam_job_block);
    if (status != ACTIVITY_MGR_STATUS_SUCCESS) {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_WARNING, "Failed to disconnect from Background Activity Manager\n");
        return;
    }
}
