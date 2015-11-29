
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_perfstats_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the APIs used to communicate with the Perfstats service
 *
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * @ingroup fbe_api_perfstats_interface
 *
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_perfstats_sim.h"
#include "fbe/fbe_perfstats_interface.h"
#include "fbe/fbe_api_perfstats_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_trace_interface.h"
#include "spid_types.h" 
#include "spid_kernel_interface.h" 
#include "fbe/fbe_queue.h"
#include <stdlib.h>

/**************************************
                Local variables
**************************************/

/*******************************************
                Local functions
********************************************/
/*!***************************************************************
 * @fn fbe_api_perfstats_sim_get_statistics_container_for_package(fbe_u64_t *ptr, 
                                                              fbe_package_id_t package_id)
 ****************************************************************
 * @brief
 *  This function returns a user-space pointer to the performance statistics container
 * for the specified package (FBE_PACKAGE_ID_PHYSICAL or FBE_PACKAGE_ID_SEP_0). 
 *
 * The pointer is returned as an unsigned 64 bit integer, and has to be cast as a
 * pointer to an fbe_performance_container_t. This is to get around pointer incompatibilities with
 * the 32-bit libraries that this function is designed to be called by.
 *
 * NOTE: This function is only usable in simulation. For hardware,
 * use fbe_api_perfstats_get_statistics_container_for_package(). 
 *
 * @param ptr - pointer to the variable that will hold the address
 * @param package_id - which package to get the container from
 *
 * @return
 *  FBE_STATUS_INSUFFICIENT_RESOURCES - Mapping failed or statistics container unallocated
 *  FBE_STATUS_OK - returned pointer is valid
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_perfstats_sim_get_statistics_container_for_package(fbe_u64_t *ptr, fbe_package_id_t package_id)
{
    csx_p_native_shm_handle_t perfstats_handle;
    csx_bytes_t va_to_return;
    csx_size_t size;
    csx_bool_t existed_already;
    csx_status_e csx_status;
    fbe_status_t status;
    char smem_name[256];
    char smem_tag[8];

    status = fbe_api_perfstats_get_statistics_container_for_package((fbe_u64_t *) smem_tag, package_id);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "Could not get shared memory tag for perfstats sim with package %d", package_id);
        return status;
    }
    if (package_id == FBE_PACKAGE_ID_PHYSICAL){
        size = sizeof(fbe_perfstats_physical_package_container_t);
        sprintf(smem_name, "%s%s", PERFSTATS_SHARED_MEMORY_NAME_SEP, smem_tag);
        csx_status = csx_p_native_shm_create_if_necessary(&perfstats_handle,
                                                          size, smem_name,
                                                          &existed_already);
    }
    else if (package_id == FBE_PACKAGE_ID_SEP_0) {
        size = sizeof(fbe_perfstats_sep_container_t);
        sprintf(smem_name, "%s%s", PERFSTATS_SHARED_MEMORY_NAME_SEP, smem_tag);
        csx_status = csx_p_native_shm_create_if_necessary(&perfstats_handle,
                                                          size, smem_name,
                                                          &existed_already);
    }
    else {
        return FBE_STATUS_GENERIC_FAILURE; //invalid package
    }
    if(!CSX_SUCCESS(csx_status) || !perfstats_handle) { //no allocation occured
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    va_to_return = csx_p_native_shm_get_base(perfstats_handle);

    if (!va_to_return) {
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    *ptr = (fbe_u64_t) va_to_return;
   
    return FBE_STATUS_OK;
}

/*************************
 * end file fbe_api_perfstats_interface.c
 *************************/
