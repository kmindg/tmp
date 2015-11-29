
/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation 
 * Author - Ryan Gadsby 4/3/2012  
 ***************************************************************************/

/***************************************************************************
 *  fbe_perfstats_service_main_kernel.c
 ***************************************************************************
 *
 *  Description
 *      This file contains the Kernel implementation for the FBE Performance Stats code
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
#include "fbe/fbe_platform.h"
#include "fbe_perfstats.h"


/* Local definitions */

/* Local members*/
#if defined(ALAMOSA_WINDOWS_ENV)
static PMDL perfstats_mdl; //pointer to the kernel-virtual memory we allocate to hold all of the perf data.
#else
static csx_p_native_shm_handle_t shm_handle = 0;
static fbe_bool_t is_sep_container = FBE_FALSE;
#endif

/* Forward declerations */


/*********************************************************************************************************/
fbe_status_t perfstats_allocate_perfstats_memory(fbe_package_id_t package_id, void **container_p){
#if defined(ALAMOSA_WINDOWS_ENV)
    fbe_perfstats_trace(FBE_TRACE_LEVEL_INFO, "Perfstats service: Initializing for package_ID:%d using Windows memory\n", package_id);
    if (package_id == FBE_PACKAGE_ID_PHYSICAL){
        *container_p = ExAllocatePoolWithTag(NonPagedPool,
                                             sizeof(fbe_perfstats_physical_package_container_t),
                                             'PTSP');
        if(!*container_p) {//no allocation occured
            fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "Perfstats service: Allocation failed for package id: %d\n", package_id);
            return FBE_STATUS_INSUFFICIENT_RESOURCES;
        }
        perfstats_mdl = IoAllocateMdl(*container_p,
                                      sizeof(fbe_perfstats_physical_package_container_t),
                                      FALSE,
                                      FALSE,
                                      NULL);
    } else if (package_id == FBE_PACKAGE_ID_SEP_0){
       *container_p = ExAllocatePoolWithTag(NonPagedPool,
                                             sizeof(fbe_perfstats_sep_container_t),
                                             'SSFP');
       if(!*container_p) {//no allocation occured
            fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "Perfstats service: Allocation failed for package id: %d\n", package_id);
            return FBE_STATUS_INSUFFICIENT_RESOURCES;
        }
        perfstats_mdl = IoAllocateMdl(*container_p,
                                      sizeof(fbe_perfstats_sep_container_t),
                                      FALSE,
                                      FALSE,
                                      NULL);
    }

    else //unsupported package
    {
        fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "Perfstats service unsupported for package id: %d\n", package_id);
    }

    if(!perfstats_mdl) { //MDL creation failed
        fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "Perfstats service: MDL creation failed for package id: %d\n", package_id);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    MmBuildMdlForNonPagedPool(perfstats_mdl);
#else
    csx_status_e status = CSX_STATUS_GENERAL_FAILURE;
    fbe_perfstats_trace(FBE_TRACE_LEVEL_INFO, "Perfstats service: Initializing for package_ID:%d using CSX memory\n", package_id);
     if (package_id == FBE_PACKAGE_ID_SEP_0){
            is_sep_container = FBE_TRUE;
     }
     else if (package_id != FBE_PACKAGE_ID_PHYSICAL) //unsupported package
     {
       fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "Perfstats service unsupported for package id: %d\n", package_id);
     }
     status = csx_p_native_shm_create(&shm_handle,
                                      is_sep_container ? sizeof(fbe_perfstats_sep_container_t) : sizeof(fbe_perfstats_physical_package_container_t),
                                      is_sep_container ? PERFSTATS_SHARED_MEMORY_NAME_SEP : PERFSTATS_SHARED_MEMORY_NAME_PHYSICAL_PACKAGE);

     if(!CSX_SUCCESS(status)) {//should never hit this but sanity check anyway
         fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "Perfstats service: Allocation failed for package id: %d\n", package_id);
         return FBE_STATUS_INSUFFICIENT_RESOURCES;
     }
     *container_p = (void *)csx_p_native_shm_get_base(shm_handle);

#endif
    return FBE_STATUS_OK;
}

fbe_status_t perfstats_deallocate_perfstats_memory(fbe_package_id_t package_id, void **container_p){
#if defined(ALAMOSA_WINDOWS_ENV)
    fbe_perfstats_trace(FBE_TRACE_LEVEL_INFO, "Perfstats service: Destroying for package_ID:%d", package_id);

    if (perfstats_mdl)
    {
        IoFreeMdl(perfstats_mdl);
        if (package_id == FBE_PACKAGE_ID_PHYSICAL && *container_p)
        {
            ExFreePoolWithTag(*container_p,
                              'PTSP');
        } else if (package_id == FBE_PACKAGE_ID_SEP_0 && *container_p)
        {
          ExFreePoolWithTag(*container_p,
                              'SSFP');
        }
    }
#else
    fbe_perfstats_trace(FBE_TRACE_LEVEL_INFO, "Perfstats service: Destroying for package_ID:%d", package_id);

    if(*container_p)
    {
        csx_p_native_shm_close(shm_handle);
    }
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - system */
    return FBE_STATUS_OK;
}

fbe_status_t perfstats_get_mapped_stats_container(fbe_u64_t *user_va){
#if defined(ALAMOSA_WINDOWS_ENV)
    PVOID user_va_to_return;
    if (!perfstats_mdl) {
        return FBE_STATUS_GENERIC_FAILURE; //somehow that got freed, something is horribly wrong!
    }

    user_va_to_return = MmMapLockedPagesSpecifyCache(perfstats_mdl, UserMode, MmCached, NULL, FALSE, NormalPagePriority);
    if (!user_va_to_return) {//mapping failed
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    *user_va = (fbe_u64_t) user_va_to_return;
#else
    *user_va = 0;
    if (shm_handle) // make sure we had successfully opened it 
    {
       //If we return a pointer from here, it's not going to be in the right context.
       //So instead we will return 0x0 and an OK status
       //This signals FBE_API to make a csx_shm call and return a valid pointer in the context of the caller.
       fbe_perfstats_trace(FBE_TRACE_LEVEL_INFO, "Instructing fbe_api to use CSX SHM");
       return FBE_STATUS_OK;
    }
    //something bad happened otherwise
    fbe_perfstats_trace(FBE_TRACE_LEVEL_ERROR, "CSX SHM for Perfstats sice not set up!");
    return FBE_STATUS_GENERIC_FAILURE;
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - system */
    return FBE_STATUS_OK;
}

