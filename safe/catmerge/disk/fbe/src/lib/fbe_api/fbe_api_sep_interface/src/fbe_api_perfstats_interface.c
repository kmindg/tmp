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
#include "fbe/fbe_perfstats_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_trace_interface.h"
#include "spid_types.h" 
#include "spid_kernel_interface.h" 
#include "fbe/fbe_queue.h"

/**************************************
                Local variables
**************************************/

/*******************************************
                Local functions
********************************************/

/*!***************************************************************
 * @fn fbe_api_perfstats_get_statistics_container_for_package(fbe_u64_t *ptr, 
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
 * NOTE: In simulation this will not return a valid pointer, but rather 64-bit value which, when converted
 * to a char[], will be the identifying tag for that package's specific instance of perfstats counter memory.
 * The function fbe_api_perfstats_sim_get_statistics_container_for_package() handles this logic, and is what
 * should be used to interact with the perfstats container in simulation. 
 *
 * @param ptr - pointer to the variable that will hold the address
 * @param package_id - which package to get the container from
 *
 * @return
 *  FBE_STATUS_GENERIC_FAILURE - Mapping failed or statistics container unallocated
 *  FBE_STATUS_OK - returned pointer is valid
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_perfstats_get_statistics_container_for_package(fbe_u64_t *ptr, 
                                                                                 fbe_package_id_t package_id)
{
    
    fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_u64_t                                   get_va = 0;

    status = fbe_api_common_send_control_packet_to_service (FBE_PERFSTATS_CONTROL_CODE_GET_STATS_CONTAINER,
                                                            &get_va, 
                                                            sizeof(fbe_u64_t),
                                                            FBE_SERVICE_ID_PERFSTATS,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            package_id);
    if (get_va) 
    {
        *ptr = get_va;
    }
    else //we might actually be using CSX native shim, try that before giving up
    {
       csx_status_e shm_status;
       csx_p_native_shm_handle_t shm_handle;
       shm_status = csx_p_native_shm_open(&shm_handle,
                             package_id == FBE_PACKAGE_ID_SEP_0 ? PERFSTATS_SHARED_MEMORY_NAME_SEP : PERFSTATS_SHARED_MEMORY_NAME_PHYSICAL_PACKAGE);
       if (CSX_SUCCESS(shm_status)) {
          *ptr = CSX_CAST_PTR_TO_PTRMAX(csx_p_native_shm_get_base(shm_handle));
          if (!*ptr) 
          {
             return FBE_STATUS_GENERIC_FAILURE;
          }
       }
       else
       {
          return FBE_STATUS_GENERIC_FAILURE;
       }
    }
    return status;
}

/*!***************************************************************
 * @fn fbe_api_perfstats_zero_statistics_for_package(fbe_package_id_t package_id)
 ****************************************************************
 * @brief
 *  This function zeroes all the data in the performance statistics container for
 *  the provided package. This will always be called whenever statistics are enabled.
 *
 * @param package_id - which package to zero the container for
 *
 * @return
 *  FBE_STATUS_GENERIC_FAILURE - statistics container unallocated
 *  FBE_STATUS_OK - zeroing successful
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_perfstats_zero_statistics_for_package(fbe_package_id_t package_id)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t     status_info;

    status = fbe_api_common_send_control_packet_to_service (FBE_PERFSTATS_CONTROL_CODE_ZERO_STATS,
                                                            NULL, 
                                                            0,
                                                            FBE_SERVICE_ID_PERFSTATS,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            package_id);

    return status;
}

/*!***************************************************************
 * @fn fbe_api_perfstats_enable_statistics_for_package(fbe_package_id_t package_id)
 ****************************************************************
 * @brief
 * This function zeroes the performance struct and enables statistics for every object
 * for which we can collect stats for in the provided package.
 *
 * @param package_id - which package to enable statistics for
 *
 * @return
 *  FBE_STATUS_GENERIC_FAILURE - statistics enabling failed
 *  FBE_STATUS_OK - every collectable object in the package has collection enabled
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_perfstats_enable_statistics_for_package(fbe_package_id_t package_id){
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_bool_t                                  enable_b;
    fbe_api_control_operation_status_info_t     status_info;

    //ALWAYS zero before we enable stats
    status = fbe_api_perfstats_zero_statistics_for_package(package_id);
    if (package_id == FBE_PACKAGE_ID_SEP_0) 
    {
        enable_b = FBE_TRUE;
        status = fbe_api_common_send_control_packet_to_service (FBE_PERFSTATS_CONTROL_CODE_SET_ENABLED,
                                                                &enable_b, 
                                                                sizeof(fbe_bool_t),
                                                                FBE_SERVICE_ID_PERFSTATS,
                                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                                &status_info,
                                                                package_id);
        if (status != FBE_STATUS_OK) 
        {
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Couldn't set statistics logging at the package level.\n", __FUNCTION__);
            return status;
        }
        status = fbe_api_common_send_control_packet_to_class(FBE_LUN_CONTROL_CODE_CLASS_PREFSTATS_SET_ENABLED,
                                                               NULL,
                                                               0,
                                                               FBE_CLASS_ID_LUN,
                                                               FBE_PACKET_FLAG_NO_ATTRIB,
                                                               &status_info,
                                                               package_id);
        if (status != FBE_STATUS_OK) 
        {
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: 'perfstats set enabled' from the kernel for SEP falied .\n", __FUNCTION__);
            return status;
        }


        return FBE_STATUS_OK;
    }
    else if (package_id == FBE_PACKAGE_ID_PHYSICAL) {
        fbe_class_id_t  filter_class_id;

        enable_b = FBE_TRUE;
        status = fbe_api_common_send_control_packet_to_service (FBE_PERFSTATS_CONTROL_CODE_SET_ENABLED,
                                                                &enable_b, 
                                                                sizeof(fbe_bool_t),
                                                                FBE_SERVICE_ID_PERFSTATS,
                                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                                &status_info,
                                                                package_id);
        if (status != FBE_STATUS_OK) 
        {
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Couldn't set statistics logging at the package level.\n", __FUNCTION__);
            return status;
        }

        for (filter_class_id = FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_FIRST +1; filter_class_id < FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_LAST; filter_class_id++)
        {
            
            status = fbe_api_common_send_control_packet_to_class(FBE_PHYSICAL_DRIVE_CONTROL_CODE_CLASS_PREFSTATS_SET_ENABLED,
                                                               &filter_class_id,
                                                               sizeof(fbe_class_id_t),
                                                               FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                                               FBE_PACKET_FLAG_NO_ATTRIB,
                                                               &status_info,
                                                               package_id);
             if (status != FBE_STATUS_OK) {
                fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: 'perfstats set enabled' from the kernel for PP falied .\n", __FUNCTION__);
                return status;
            }
        }
    }
    else{
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Invalid package", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_api_perfstats_disable_statistics_for_package(fbe_package_id_t package_id)
 ****************************************************************
 * @brief
 * This function disables statistics for every object
 * for which we can collect stats for in the provided package.
 *
 * @param package_id - which package to disable statistics for
 *
 * @return
 *  FBE_STATUS_GENERIC_FAILURE - statistics disabling failed
 *  FBE_STATUS_OK - every collectable object in the package has collection disabled
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_perfstats_disable_statistics_for_package(fbe_package_id_t package_id){
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_bool_t                                  enable_b;
    fbe_api_control_operation_status_info_t     status_info;
    
    if (package_id == FBE_PACKAGE_ID_SEP_0) {
        enable_b = FBE_FALSE;
        status = fbe_api_common_send_control_packet_to_service (FBE_PERFSTATS_CONTROL_CODE_SET_ENABLED,
                                                                &enable_b, 
                                                                sizeof(fbe_bool_t),
                                                                FBE_SERVICE_ID_PERFSTATS,
                                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                                &status_info,
                                                                package_id);
        if (status != FBE_STATUS_OK) 
        {
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Couldn't set statistics logging at the package level.\n", __FUNCTION__);
            return status;
        }

        status = fbe_api_common_send_control_packet_to_class(FBE_LUN_CONTROL_CODE_CLASS_PREFSTATS_SET_DISABLED,
                                                               NULL,
                                                               0,
                                                               FBE_CLASS_ID_LUN,
                                                               FBE_PACKET_FLAG_NO_ATTRIB,
                                                               &status_info,
                                                               package_id);
        if (status != FBE_STATUS_OK) 
        {
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: 'perfstats set enabled' from the kernel for SEP falied .\n", __FUNCTION__);
            return status;
        }

        return FBE_STATUS_OK;
    }
    else if (package_id == FBE_PACKAGE_ID_PHYSICAL) {
        fbe_class_id_t  filter_class_id;
        
        enable_b = FBE_FALSE;
        status = fbe_api_common_send_control_packet_to_service (FBE_PERFSTATS_CONTROL_CODE_SET_ENABLED,
                                                                &enable_b, 
                                                                sizeof(fbe_bool_t),
                                                                FBE_SERVICE_ID_PERFSTATS,
                                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                                &status_info,
                                                                package_id);
        if (status != FBE_STATUS_OK) 
        {
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Couldn't set statistics logging at the package level.\n", __FUNCTION__);
            return status;
        }

        for (filter_class_id = FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_FIRST +1; filter_class_id < FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_LAST; filter_class_id++)
        {
            status = fbe_api_common_send_control_packet_to_class(FBE_PHYSICAL_DRIVE_CONTROL_CODE_CLASS_PREFSTATS_SET_DISABLED,
                                                               &filter_class_id,
                                                               sizeof(fbe_class_id_t),
                                                               FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                                               FBE_PACKET_FLAG_NO_ATTRIB,
                                                               &status_info,
                                                               package_id);
            if (status != FBE_STATUS_OK) {
                fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: 'perfstats set disabled' from the kernel for PP falied .\n", __FUNCTION__);
                return status;
            }
        }
    }
    else{
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Invalid package", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_status_t FBE_API_CALL fbe_api_perfstats_get_offset_for_object(fbe_object_id_t object_id,
                                                                  fbe_package_id_t package_id,
                                                                  fbe_u32_t *offset)
 ****************************************************************
 * @brief
 * Within the performance struct there is an array that holds the per-object stats for that package.
 *
 * This function returns the offset within the performance statistics container for
 * a given package the per-object statistics for the object with the provided object_id.
 *
 * Offsets are persistant. Even if an object is destroyed and another object is created with
 * the same object ID, it will use the same offset within the container.
 *
 * @param object_id - which object we're trying to find the offset for.
 * @param package_id - which package to look in
 * @param *offset - returned offset
 *
 * @return
 *  FBE_STATUS_OK - packet set to perfstats service okay. Offset will be PERFSTATS_INVALID_OBJECT_OFFSET
 * if the object was not in the Perfstats object map for this package.
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_perfstats_get_offset_for_object(fbe_object_id_t object_id,
                                                                  fbe_package_id_t package_id,
                                                                  fbe_u32_t *offset)
{
    fbe_status_t                                        status;
    fbe_perfstats_service_get_offset_t                  get_offset;
    fbe_api_control_operation_status_info_t             status_info;

    get_offset.object_id = object_id;

    status = fbe_api_common_send_control_packet_to_service (FBE_PERFSTATS_CONTROL_CODE_GET_OFFSET,
                                                            &get_offset, 
                                                            sizeof(fbe_perfstats_service_get_offset_t),
                                                            FBE_SERVICE_ID_PERFSTATS,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            package_id);
    *offset = get_offset.offset;
    if (package_id == FBE_PACKAGE_ID_SEP_0 && *offset >= PERFSTATS_MAX_SEP_OBJECTS) 
    {
        *offset = PERFSTATS_INVALID_OBJECT_OFFSET;
    }
    else if (package_id == FBE_PACKAGE_ID_PHYSICAL && *offset >= PERFSTATS_MAX_PHYSICAL_OBJECTS) 
    {
        *offset = PERFSTATS_INVALID_OBJECT_OFFSET;
    }
    return status;
}

/*!***************************************************************
 * @fn fbe_status_t FBE_API_CALL fbe_api_perfstats_is_statistics_collection_enabled_for_package(fbe_package_id_t package_id,
                                                                                                fbe_bool_t *is_enabled
 ****************************************************************
 * @brief
 * Checks to see if package-wide statistics enabling has been toggled.
 *
 * @param package_id - which package to look in
 * @param *is_enabled - whether or not collection is enabled
 *
 * @return
 *  FBE_STATUS_GENERIC_FAILURE - Something failed sending the packet to the service, or there's an unsupported package_id
 *  FBE_STATUS_OK - check successful
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_perfstats_is_statistics_collection_enabled_for_package(fbe_package_id_t package_id,
                                                                                         fbe_bool_t *is_enabled)
{
    fbe_status_t                                        status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t             status_info;
    fbe_bool_t                                          enabled = FBE_FALSE;
    status = fbe_api_common_send_control_packet_to_service (FBE_PERFSTATS_CONTROL_CODE_IS_COLLECTION_ENABLED,
                                                            &enabled, 
                                                            sizeof(fbe_bool_t),
                                                            FBE_SERVICE_ID_PERFSTATS,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            package_id);
    if (status == FBE_STATUS_OK) {
        *is_enabled = enabled;
    }
    return status;
}

/*!***************************************************************
 * @fn fbe_status_t FBE_API_CALL fbe_api_perfstats_get_summed_lun_stats(fbe_perfstats_lun_sum_t *summed_stats,
                                                                 fbe_lun_performance_counters_t *lun_stats)
 ****************************************************************
 * @brief
 * This is a function that sums all of our per-core counters for a given LUN and
 * puts them in an aggregate struct.
 *
 * @param *summed stats - pointer to the aggregate stats constainer for a single object.
 * @param *lun_stats - pointer to the lun stats we wish to sum
 *
 * @return
 *  FBE_STATUS_GENERIC_FAILURE - one of the input pointers was null
 *  FBE_STATUS_OK - counters successfully summed
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_perfstats_get_summed_lun_stats(fbe_perfstats_lun_sum_t *summed_stats,
                                                                 fbe_lun_performance_counters_t *lun_stats)
{
    fbe_u32_t core_i;
    fbe_u32_t pos_i;

    if (!summed_stats || !lun_stats){
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //zero summed stat struct
    fbe_zero_memory(summed_stats, sizeof(fbe_perfstats_lun_sum_t));

    //timestamp
    summed_stats->timestamp = lun_stats->timestamp;
    summed_stats->object_id = lun_stats->object_id;
    //sum everything
    for (core_i = 0; core_i < PERFSTATS_CORES_SUPPORTED; core_i++) {
        summed_stats->cumulative_read_response_time     += lun_stats->cumulative_read_response_time[core_i];
        summed_stats->cumulative_write_response_time    += lun_stats->cumulative_write_response_time[core_i];
        summed_stats->lun_blocks_read                   += lun_stats->lun_blocks_read[core_i];
    	summed_stats->lun_blocks_written                += lun_stats->lun_blocks_written[core_i];
        summed_stats->lun_read_requests                 += lun_stats->lun_read_requests[core_i];
        summed_stats->lun_write_requests                += lun_stats->lun_write_requests[core_i];
        summed_stats->stripe_crossings                  += lun_stats->stripe_crossings[core_i];
        summed_stats->stripe_writes                     += lun_stats->stripe_writes[core_i];
        summed_stats->non_zero_queue_arrivals           += lun_stats->non_zero_queue_arrivals[core_i];
        summed_stats->sum_arrival_queue_length          += lun_stats->sum_arrival_queue_length[core_i];

        //RG counters
        for (pos_i = 0; pos_i < FBE_RAID_MAX_DISK_ARRAY_WIDTH; pos_i++) {
            summed_stats->disk_blocks_read[pos_i]       += lun_stats->disk_blocks_read[pos_i][core_i];
            summed_stats->disk_blocks_written[pos_i]    += lun_stats->disk_blocks_written[pos_i][core_i];
            summed_stats->disk_reads[pos_i]             += lun_stats->disk_reads[pos_i][core_i];
            summed_stats->disk_writes[pos_i]            += lun_stats->disk_writes[pos_i][core_i];
        }

        //io size histograms
        for(pos_i = 0; pos_i < FBE_PERFSTATS_HISTOGRAM_BUCKET_LAST; pos_i++) {
            summed_stats->lun_io_size_read_histogram[pos_i]     += lun_stats->lun_io_size_read_histogram[pos_i][core_i];
            summed_stats->lun_io_size_write_histogram[pos_i]    += lun_stats->lun_io_size_write_histogram[pos_i][core_i];
        }
    }
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_status_t FBE_API_CALL fbe_api_perfstats_get_summed_pdo_stats(fbe_perfstats_pdo_sum_t *summed_stats,
                                                                 fbe_pdo_performance_counters_t *pdo_stats)
 ****************************************************************
 * @brief
 * This is a function that sums all of our per-core counters for a given PDO and
 * puts them in an aggregate struct.
 *
 * @param *summed stats - pointer to the aggregate stats constainer for a single object.
 * @param *pdo_stats - pointer to the pdo stats we wish to sum
 *
 * @return
 *  FBE_STATUS_GENERIC_FAILURE - one of the input pointers was null
 *  FBE_STATUS_OK - counters successfully summed.
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_perfstats_get_summed_pdo_stats(fbe_perfstats_pdo_sum_t *summed_stats,
                                                                 fbe_pdo_performance_counters_t *pdo_stats){
    fbe_u32_t core_i;
    fbe_u32_t bucket_i;
    fbe_u64_t diff;
    fbe_time_t last_monitor_timestamp;
    fbe_time_t last_transition_timestamp;

    if (!summed_stats || !pdo_stats){
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //zero summed stat struct
    fbe_zero_memory(summed_stats, sizeof(fbe_perfstats_pdo_sum_t));

    last_monitor_timestamp = pdo_stats->last_monitor_timestamp;
    last_transition_timestamp = pdo_stats->timestamp;
    summed_stats->object_id = pdo_stats->object_id;
    summed_stats->busy_ticks                =  pdo_stats->busy_ticks[0];
    summed_stats->idle_ticks                =  pdo_stats->idle_ticks[0];        

    //sum everything, for busy/idle ticks we only use CPU 0.        
    for (core_i = 0; core_i < PERFSTATS_CORES_SUPPORTED; core_i++) {
        summed_stats->non_zero_queue_arrivals       +=  pdo_stats->non_zero_queue_arrivals[core_i];
        summed_stats->sum_arrival_queue_length      +=  pdo_stats->sum_arrival_queue_length[core_i];
        summed_stats->disk_blocks_read              +=  pdo_stats->disk_blocks_read [core_i];
        summed_stats->disk_blocks_written           +=  pdo_stats->disk_blocks_written[core_i];
        summed_stats->disk_reads                    +=  pdo_stats->disk_reads[core_i];
        summed_stats->disk_writes                   +=  pdo_stats->disk_writes[core_i];
        summed_stats->sum_blocks_seeked             +=  pdo_stats->sum_blocks_seeked[core_i];
        for (bucket_i = 0; bucket_i < FBE_PERFSTATS_SRV_TIME_HISTOGRAM_BUCKET_LAST; bucket_i++)
        {
            summed_stats->disk_srv_time_histogram[bucket_i] += pdo_stats->disk_srv_time_histogram[bucket_i][core_i];
        }
    }

    summed_stats->timestamp = last_transition_timestamp;

    //we need to adjust ticks to handle the edge case of a drive that is 100% busy or 100% idle between polls
    //use PDO monitor timestamp to adjust ticks accordingly
    if (last_transition_timestamp < last_monitor_timestamp) { //monitor timestamp is younger than update, must report tick change
        diff = ((last_monitor_timestamp | FBE_PERFSTATS_BUSY_STATE) - (last_transition_timestamp | FBE_PERFSTATS_BUSY_STATE)); //difference is number of ticks, 1 tick = 1 microsecond.
        if (last_monitor_timestamp & FBE_PERFSTATS_BUSY_STATE) { //LSB of last monitor timestamp encodes the busy/idle state of the drive, this means it's busy
            summed_stats->busy_ticks += diff;
        }
        else{ //idle
            summed_stats->idle_ticks += diff;
        }
    } //if the object timestamp is younger there's no need to adjust the ticks.


    return FBE_STATUS_OK;
}

/*************************
 * end file fbe_api_perfstats_interface.c
 *************************/
