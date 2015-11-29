

/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation 
 *  
 * Author - Ryan Gadsby 4/3/2012 
 ***************************************************************************/

/***************************************************************************
 *  fbe_perfstats_service_main_sim.c
 ***************************************************************************
 *
 *  Description
 *      This file contains the sim implementation for the FBE Performance Stats code
 *      
 *    
 ***************************************************************************/
//figure out what I actually need
#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_perfstats_interface.h"
#include "fbe_perfstats_private.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_trace_interface.h"
#include "spid_types.h" 
#include "spid_kernel_interface.h" 
#include "fbe/fbe_queue.h"
#include "fbe_perfstats.h"
#include "fbe/fbe_terminator_api.h"
#include <stdlib.h>


/* Local definitions */


/* Local members*/
static csx_p_native_shm_handle_t perfstats_sim_handle;
static char smem_name[256];
static char smem_tag[8];
/* Forward declerations */


/*********************************************************************************************************/
fbe_status_t perfstats_allocate_perfstats_memory(fbe_package_id_t package_id, void **container_p){
    csx_status_e status;
    csx_bool_t existed_already;
    csx_size_t size = 0;
    fbe_system_time_t time; //unique identifier for perfstats memory


    fbe_get_system_time(&time);
    sprintf(smem_tag,"%d%d%d", time.minute, time.second, time.milliseconds);    
    if (package_id == FBE_PACKAGE_ID_SEP_0) {
        size = sizeof(fbe_perfstats_sep_container_t);
        
        sprintf(smem_name, "%s%s", PERFSTATS_SHARED_MEMORY_NAME_SEP, smem_tag);
    }
    else if (package_id == FBE_PACKAGE_ID_PHYSICAL) {
        size = sizeof(fbe_perfstats_physical_package_container_t);
        sprintf(smem_name, "%s%s", PERFSTATS_SHARED_MEMORY_NAME_PHYSICAL_PACKAGE, smem_tag);
    }
    
    fbe_perfstats_trace(FBE_TRACE_LEVEL_INFO, "Sim perfstats name: %s\n", smem_name);
    status = csx_p_native_shm_create_if_necessary(&perfstats_sim_handle, size,
                                                  smem_name, &existed_already);

    if(!CSX_SUCCESS(status) || !perfstats_sim_handle) { //no allocation occured
        fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "Perfstats service: Allocation failed for package id: %d status: 0x%x\n", package_id, (int)status);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    *container_p = (void *)csx_p_native_shm_get_base(perfstats_sim_handle);

    if (!*container_p) { //we can check failure on either field
            fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "Perfstats service: Mapping failed for package id: %d\n", package_id);
            return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    return FBE_STATUS_OK;
}

fbe_status_t perfstats_deallocate_perfstats_memory(fbe_package_id_t package_id, void **container_p){
    if (container_p && perfstats_sim_handle) {
        csx_p_native_shm_close(perfstats_sim_handle);
    }
    return FBE_STATUS_OK;
}

fbe_status_t perfstats_get_mapped_stats_container(fbe_u64_t *user_va){
    //in simulation this simply returns the tag needed to correctly identify the right instance of perf memory.
    fbe_copy_memory(user_va, smem_tag, sizeof(fbe_u64_t));
    //fbe_perfstats_trace(FBE_TRACE_LEVEL_WARNING, "Use fbe_api_perfstats_sim_get_statistics_container_for_package() instead\n");
    return FBE_STATUS_GENERIC_FAILURE;
}






