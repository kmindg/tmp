/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!*************************************************************************
 * @file fbe_api_raid_group_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the fbe_raid_group interface for the storage extent package.
 *
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * @ingroup fbe_api_raid_group_interface
 *
 * @version
 *   11/05/2009:  Created. guov
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

#include "fbe/fbe_parity.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_raid_group.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_cmi_interface.h"


/*************************
 *   FORWARD DECLARATIONS
 *************************/
static void fbe_api_raid_group_get_min_rebuild_checkpoint(fbe_api_raid_group_get_info_t rg_info,
                                                      fbe_lba_t *min_chkpt);

static fbe_status_t fbe_raid_group_get_verify_checkpoint(fbe_api_raid_group_get_info_t rg_info,
                                                         fbe_lun_verify_type_t verify_type, 
                                                         fbe_lba_t *rg_checkpoint);

/**************************************
                Local variables
**************************************/

typedef struct rg_job_wait_s{
    fbe_semaphore_t create_semaphore;
    fbe_object_id_t created_rg_id;
    fbe_job_service_error_type_t job_status;
    fbe_raid_group_type_t raid_type;
}rg_job_wait_t;

#define FBE_API_RG_WAIT_EXPANSION 60000

/*******************************************
                Local functions
********************************************/

/*!***************************************************************
* @fn fbe_api_create_rg()
****************************************************************
* @brief
*  This function creates a RG. It is synchronous and will return once the RG is created.
*  If the user passes FBE_RAID_ID_INVALID in rg_create_strcut->raid_group_id MCR will 
*  generte the ID and return it in rg_create_strcut->raid_group_id
*
* @param rg_create_strcut - The data needed for RG creation
* @param wait_ready - does the user want to wait for the RG to become available
* @param ready_timeout_msec - how long to wait for the RG to become available.
* @param rg_object_id - pointer to object id
* @param job_error_type - pointer to job error type which will be used by Admin.
* @return
*  fbe_status_t
*
* 11/15/2012  Modified by Vera Wang
****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_create_rg(fbe_api_raid_group_create_t *rg_create_strcut,
                                            fbe_bool_t wait_ready, 
                                            fbe_u32_t ready_timeout_msec,
                                            fbe_object_id_t *rg_object_id,
                                            fbe_job_service_error_type_t *job_error_type)
{
    fbe_status_t                             status , job_status;
    fbe_api_job_service_raid_group_create_t  rg_create_job = {0};
    fbe_object_id_t                          raid_object_id = FBE_OBJECT_ID_INVALID;

    if (job_error_type == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: failed to pass job_error_type pointer. \n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (rg_create_strcut->is_system_rg)
    {
        rg_create_job.is_system_rg = FBE_TRUE;
        rg_create_job.rg_id = rg_create_strcut->rg_id;
        rg_create_job.raid_group_id = rg_create_strcut->raid_group_id;
    } else 
    {
        /*fill in all the information we need*/
        rg_create_job.capacity = rg_create_strcut->capacity;
        rg_create_job.drive_count = rg_create_strcut->drive_count;
        rg_create_job.is_private = rg_create_strcut->is_private;
        rg_create_job.user_private = rg_create_strcut->user_private;
        rg_create_job.max_raid_latency_time_is_sec = rg_create_strcut->max_raid_latency_time_is_sec;
    

        rg_create_job.power_saving_idle_time_in_seconds = rg_create_strcut->power_saving_idle_time_in_seconds;
        rg_create_job.b_bandwidth = rg_create_strcut->b_bandwidth;
        rg_create_job.raid_type = rg_create_strcut->raid_type;
        rg_create_job.raid_group_id = rg_create_strcut->raid_group_id;
        fbe_copy_memory(rg_create_job.disk_array,rg_create_strcut->disk_array, FBE_RAID_MAX_DISK_ARRAY_WIDTH * sizeof(fbe_job_service_bes_t));
    	rg_create_job.wait_ready = wait_ready;
    	rg_create_job.ready_timeout_msec = ready_timeout_msec;
    }

    /*start the job*/
    status = fbe_api_job_service_raid_group_create(&rg_create_job);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: failed to start RG creation job\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    /*wait for it*/
    status = fbe_api_common_wait_for_job(rg_create_job.job_number,
                                             ready_timeout_msec,
                                             job_error_type,
                                             &job_status,
                                             &raid_object_id);

    if ((status != FBE_STATUS_OK) || (job_status != FBE_STATUS_OK) || (*job_error_type != FBE_JOB_SERVICE_ERROR_NO_ERROR)) {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s:job errors: status:%d, job_code:%d\n",__FUNCTION__, job_status, *job_error_type);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    if (rg_object_id != NULL) {
        *rg_object_id = raid_object_id;
    }

    /*let's populate the RG number the DB assigned to us*/
    if (rg_create_strcut->raid_group_id == FBE_RAID_ID_INVALID) {
        status = fbe_api_database_lookup_raid_group_by_object_id(raid_object_id, &rg_create_strcut->raid_group_id);
        if (status != FBE_STATUS_OK) {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: can't get assigned RG number for id:0x%x\n",
                      __FUNCTION__, raid_object_id);

            return status;
        }
    }

    return status;
}
/**************************************
 * end fbe_api_create_rg()
 **************************************/
/*!***************************************************************
 *  fbe_api_raid_group_get_info()
 ****************************************************************
 * @brief
 *  This function get info with the given raid group.
 *
 * @param rg_object_id - RG object id
 * @param raid_group_info_p - pointer to RG info
 * @param attribute - attribute
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  11/05/09 - Created. guov
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_raid_group_get_info(fbe_object_id_t rg_object_id, 
                            fbe_api_raid_group_get_info_t * raid_group_info_p,
                            fbe_packet_attr_t attribute)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_raid_group_get_info_t                       raid_group_info;
    fbe_u32_t                                       index;

    if (raid_group_info_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(&raid_group_info, sizeof(fbe_raid_group_get_info_t));

    status = fbe_api_common_send_control_packet (FBE_RAID_GROUP_CONTROL_CODE_GET_INFO,
                                                 &raid_group_info,
                                                 sizeof(fbe_raid_group_get_info_t),
                                                 rg_object_id,
                                                 attribute,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        if (status == FBE_STATUS_OK)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        return status;
    }

    raid_group_info_p->capacity = raid_group_info.capacity;
    raid_group_info_p->raid_capacity = raid_group_info.raid_capacity;
    raid_group_info_p->imported_blocks_per_disk = raid_group_info.imported_blocks_per_disk;
    raid_group_info_p->paged_metadata_start_lba = raid_group_info.paged_metadata_start_lba;
    raid_group_info_p->paged_metadata_capacity = raid_group_info.paged_metadata_capacity;
    raid_group_info_p->write_log_start_pba = raid_group_info.write_log_start_pba;
    raid_group_info_p->write_log_physical_capacity = raid_group_info.write_log_physical_capacity;
    raid_group_info_p->physical_offset = raid_group_info.physical_offset;
    raid_group_info_p->num_data_disk = raid_group_info.num_data_disk;

    raid_group_info_p->debug_flags = raid_group_info.debug_flags;
    raid_group_info_p->library_debug_flags = raid_group_info.library_debug_flags;
    raid_group_info_p->element_size = raid_group_info.element_size;
    raid_group_info_p->sectors_per_stripe = raid_group_info.sectors_per_stripe;
    raid_group_info_p->width = raid_group_info.width;
    raid_group_info_p->physical_width = raid_group_info.physical_width;
    raid_group_info_p->exported_block_size = raid_group_info.exported_block_size;
    raid_group_info_p->imported_block_size = raid_group_info.imported_block_size;
    raid_group_info_p->optimal_block_size = raid_group_info.optimal_block_size;
    raid_group_info_p->flags = raid_group_info.flags;
    raid_group_info_p->base_config_flags = raid_group_info.base_config_flags;
    raid_group_info_p->base_config_clustered_flags = raid_group_info.base_config_clustered_flags;
    raid_group_info_p->base_config_peer_clustered_flags = raid_group_info.base_config_peer_clustered_flags;
    raid_group_info_p->raid_type = raid_group_info.raid_type;
    raid_group_info_p->stripe_count = raid_group_info.stripe_count;
    raid_group_info_p->bitmask_4k = raid_group_info.bitmask_4k;
    raid_group_info_p->ro_verify_checkpoint = raid_group_info.ro_verify_checkpoint;
    raid_group_info_p->rw_verify_checkpoint = raid_group_info.rw_verify_checkpoint;
    raid_group_info_p->error_verify_checkpoint = raid_group_info.error_verify_checkpoint;  
    raid_group_info_p->journal_verify_checkpoint = raid_group_info.journal_verify_checkpoint;    
    raid_group_info_p->incomplete_write_verify_checkpoint = raid_group_info.incomplete_write_verify_checkpoint;
    raid_group_info_p->system_verify_checkpoint = raid_group_info.system_verify_checkpoint; 
    raid_group_info_p->raid_group_np_md_flags = raid_group_info.raid_group_np_md_flags;
    raid_group_info_p->raid_group_np_md_extended_flags = raid_group_info.raid_group_np_md_extended_flags;
    raid_group_info_p->local_state = raid_group_info.local_state;
    raid_group_info_p->peer_state = raid_group_info.peer_state;
    raid_group_info_p->metadata_element_state = raid_group_info.metadata_element_state;
    raid_group_info_p->paged_metadata_verify_pass_count = raid_group_info.paged_metadata_verify_pass_count;
    raid_group_info_p->total_chunks = raid_group_info.total_chunks;
    raid_group_info_p->lun_align_size = raid_group_info.lun_align_size;
    raid_group_info_p->user_private = raid_group_info.user_private;
    raid_group_info_p->b_is_event_q_empty = raid_group_info.b_is_event_q_empty;
    raid_group_info_p->background_op_seconds = raid_group_info.background_op_seconds;

    for ( index = 0; index < FBE_RAID_MAX_DISK_ARRAY_WIDTH; index++)
    {
        raid_group_info_p->b_rb_logging[index] = raid_group_info.b_rb_logging[index];
        raid_group_info_p->rebuild_checkpoint[index] = raid_group_info.rebuild_checkpoint[index];
    }

    raid_group_info_p->elements_per_parity_stripe = raid_group_info.elements_per_parity_stripe;
    raid_group_info_p->rekey_checkpoint = raid_group_info.rekey_checkpoint;
    return status;
}
/**************************************
 * end fbe_api_raid_group_get_info()
 **************************************/

/*!***************************************************************
 *  fbe_api_raid_group_get_write_log_info()
 ****************************************************************
 * @brief
 *  This function get info with the given raid group.
 *
 * @param rg_object_id - RG object id
 * @param raid_group_info_p - pointer to RG info
 * @param attribute - attribute
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  11/05/09 - Created. guov
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_raid_group_get_write_log_info(fbe_object_id_t                  rg_object_id,
                                                                fbe_parity_get_write_log_info_t *write_log_info_p,
                                                                fbe_packet_attr_t                attribute)

{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_parity_get_write_log_info_t             write_log_info;

    if (write_log_info_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(&write_log_info, sizeof(fbe_parity_get_write_log_info_t));

    status = fbe_api_common_send_control_packet (FBE_RAID_GROUP_CONTROL_CODE_GET_WRITE_LOG_INFO,
                                                 &write_log_info,
                                                 sizeof(fbe_parity_get_write_log_info_t),
                                                 rg_object_id,
                                                 attribute,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        if (status == FBE_STATUS_OK)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        return status;
    }

    *write_log_info_p = write_log_info;

    return status;
}
/**************************************
 * end fbe_api_raid_group_get_write_log_info()
 **************************************/



/*!***************************************************************
   @fn fbe_api_raid_group_initiate_verify(fbe_object_id_t       in_lun_object_id ,
                                         fbe_packet_attr_t     in_attribute,
                                         fbe_lun_verify_type_t in_verify_type)
 ****************************************************************
 * @brief
 *  This function initiates a verify operation on a Raid group.
 *
 * @param in_raid_group_object_id      - The Raid group object ID
 * @param in_attribute                 - packet attribute flag
 * @param in_verify_type               - type of verify (read-only, normal)
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  06/28/10 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_raid_group_initiate_verify(
                            fbe_object_id_t             in_raid_group_object_id, 
                            fbe_packet_attr_t           in_attribute,
                            fbe_lun_verify_type_t       in_verify_type)
{
    fbe_status_t                                status;
    fbe_api_base_config_upstream_object_list_t  upstream_object_list;
    fbe_u32_t                                   index;
    fbe_class_id_t                              class_id;
    fbe_u32_t                                   raid_group_number;
    fbe_u32_t                                   lun_number;


    
    /* Get the upstream object list    
     */
    status = fbe_api_base_config_get_upstream_object_list(in_raid_group_object_id, &upstream_object_list);

    if(status != FBE_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Go through the list and initiate verify for each.
       As a sanity check make sure that the upstream edge is indeed a lun.
       Right now there is no way to get all the luns that belong to the raid group
       we just have to assume above the raid object is the lun object    
    */
    for (index = 0; index < upstream_object_list.number_of_upstream_objects; index++)
    {
        /* Get the class id of the upstream objects*/
        status = fbe_api_get_object_class_id(upstream_object_list.upstream_object_list[index], &class_id, FBE_PACKAGE_ID_SEP_0);

        if(status != FBE_STATUS_OK)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, " %s Get class id failed\n", __FUNCTION__);
            continue;           
        }

        if(class_id != FBE_CLASS_ID_LUN)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, " %s Upstream edge is not a lun\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        status = fbe_api_lun_initiate_verify(upstream_object_list.upstream_object_list[index],
                                             in_attribute,
                                             in_verify_type,
                                             FBE_TRUE, /* Verify the entire lun*/ 
                                             FBE_LUN_VERIFY_START_LBA_LUN_START,  
                                             FBE_LUN_VERIFY_BLOCKS_ENTIRE_LUN);    

        if(status != FBE_STATUS_OK)
        {
            status = fbe_api_database_lookup_raid_group_by_object_id(in_raid_group_object_id, &raid_group_number);
            if (status != FBE_STATUS_OK) {
                fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: can't get assigned RG number for RG id:0x%x\n",
                              __FUNCTION__, in_raid_group_object_id);
            }

            status = fbe_api_database_lookup_lun_by_object_id(upstream_object_list.upstream_object_list[index], &lun_number);
            if (status != FBE_STATUS_OK) {
                fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: Group based verify for RG %d failed for lun id: %d",
                              __FUNCTION__, raid_group_number, upstream_object_list.upstream_object_list[index]);
            }
        }
    }
        
    return status;
}   // end fbe_api_raid_group_initiate_verify()

/*!***************************************************************
 * @fn fbe_api_raid_group_get_flush_error_counters(
                              fbe_object_id_t                 in_raid_group_object_id,
                              fbe_packet_attr_t               in_attribute,
                              fbe_raid_flush_error_counts_t*  out_verify_report_p);
 ****************************************************************
 * @brief
 *  This function gets the raid group write log flush error counters.
 *
 * @param in_raid_group_object_id      - the raid group object ID
 * @param in_attribute                 - packet attribute flag
 * @param out_verify_report_p          - pointer to the verify report data
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  5/11/2012 - Created. Dave Agans
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_raid_group_get_flush_error_counters(
                                     fbe_object_id_t                 in_raid_group_object_id,
                                     fbe_packet_attr_t               in_attribute,
                                     fbe_raid_flush_error_counts_t*  out_verify_report_p)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    if (out_verify_report_p == NULL) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: NULL buffer pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_RAID_GROUP_CONTROL_CODE_GET_FLUSH_ERROR_COUNTERS,
                                                 out_verify_report_p,
                                                 sizeof(fbe_raid_flush_error_counts_t),
                                                 in_raid_group_object_id,
                                                 in_attribute,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}   // end fbe_api_raid_group_get_flush_error_counters()


/*!***************************************************************
 * @fn fbe_api_raid_group_clear_flush_error_counters(
                                       fbe_object_id_t         in_raid_group_object_id, 
                                       fbe_packet_attr_t       in_attribute)
 ****************************************************************
 * @brief
 *  This function clears the write log flush error counters on a raid group.
 *
 * @param in_raid_group_object_id  - pointer to the raid group object
 * @param in_attribute             - attributes
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  5/11/2012 - Created. Dave Agans
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_raid_group_clear_flush_error_counters(
                            fbe_object_id_t             in_raid_group_object_id, 
                            fbe_packet_attr_t           in_attribute)
{
    fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;


    status = fbe_api_common_send_control_packet (FBE_RAID_GROUP_CONTROL_CODE_CLEAR_FLUSH_ERROR_COUNTERS,
                                                 NULL, 
                                                 0,
                                                 in_raid_group_object_id,
                                                 in_attribute,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;

}   // end fbe_api_raid_group_clear_flush_error_counters()

/*!***************************************************************
 * @fn fbe_api_raid_group_get_io_info(fbe_object_id_t   in_raid_group_object_id,
                                      fbe_packet_attr_t           in_attribute,
                                      fbe_api_raid_group_get_io_info_t *raid_group_io_info_p)
 ****************************************************************
 * @brief
 *  This function gets the raid group I/O information.
 *
 * @param in_raid_group_object_id - Raid group object id
 * @param raid_group_io_info_p  - Pointer raid group I/O information structure to
 *                                populate
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  05/09/2013  Ron Proulx  - Updated.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_raid_group_get_io_info(
                            fbe_object_id_t             in_raid_group_object_id,
                            fbe_api_raid_group_get_io_info_t *raid_group_io_info_p)
{
    fbe_status_t                            status;
    fbe_api_control_operation_status_info_t status_info;
    fbe_raid_group_get_io_info_t            get_rg_io_info;

    if (raid_group_io_info_p == NULL) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: NULL input pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(&get_rg_io_info, sizeof(fbe_raid_group_get_io_info_t));
    status = fbe_api_common_send_control_packet (FBE_RAID_GROUP_CONTROL_CODE_GET_IO_INFO,
                                                 &get_rg_io_info,
                                                 sizeof(fbe_raid_group_get_io_info_t),
                                                 in_raid_group_object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    raid_group_io_info_p->outstanding_io_count = get_rg_io_info.outstanding_io_count;
    raid_group_io_info_p->b_is_quiesced = get_rg_io_info.b_is_quiesced;
    raid_group_io_info_p->quiesced_io_count = get_rg_io_info.quiesced_io_count;

    return status;
}   // end fbe_api_raid_group_get_io_info()


/*!***************************************************************
 * fbe_api_raid_group_quiesce()
 ****************************************************************
 * @brief
 *  This function quiesce the given raid group.
 *
 * @param rg_object_id - RG object ID
 * @param attribute - attribute
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  11/05/09 - Created. guov
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_raid_group_quiesce(fbe_object_id_t rg_object_id, 
                                fbe_packet_attr_t attribute)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    status = fbe_api_common_send_control_packet (FBE_RAID_GROUP_CONTROL_CODE_QUIESCE,
                                                 NULL, /* no struct*/
                                                 0, /* no size */
                                                 rg_object_id,
                                                 attribute,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_api_raid_group_quiesce()
 **************************************/

/*!***************************************************************
 *  fbe_api_raid_group_unquiesce()
 ****************************************************************
 * @brief
 *  This function unquiesce the given raid group.
 *
 * @param rg_object_id - RG object ID
 * @param attribute - attribute package
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  11/05/09 - Created. guov
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_raid_group_unquiesce(fbe_object_id_t rg_object_id, 
                                fbe_packet_attr_t attribute)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    status = fbe_api_common_send_control_packet (FBE_RAID_GROUP_CONTROL_CODE_UNQUIESCE,
                                                 NULL, /* no struct*/
                                                 0, /* no size */
                                                 rg_object_id,
                                                 attribute,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_api_raid_group_unquiesce()
 **************************************/

/*!***************************************************************
 *  fbe_api_raid_group_calculate_capacity()
 ****************************************************************
 * @brief
 *  This function calculates capacity information based on input values.
 *
 * @param capacity_info_p - pointer to RG calculated capacity
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  01/10/10 - Created. Jesus Medina
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_raid_group_calculate_capacity(fbe_api_raid_group_calculate_capacity_info_t * capacity_info_p)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_parity_control_class_calculate_capacity_t   calculate_capacity;

    calculate_capacity.imported_capacity = capacity_info_p->imported_capacity;
    calculate_capacity.block_edge_geometry = capacity_info_p->block_edge_geometry;
    calculate_capacity.exported_capacity = 0;

    status = fbe_api_common_send_control_packet_to_class (FBE_PARITY_CONTROL_CODE_CLASS_CALCULATE_CAPACITY,
                                                          &calculate_capacity,
                                                          sizeof(fbe_parity_control_class_calculate_capacity_t),
                                                          FBE_CLASS_ID_PARITY,
                                                          FBE_PACKET_FLAG_NO_ATTRIB,
                                                          &status_info,
                                                          FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    capacity_info_p->exported_capacity = calculate_capacity.exported_capacity;
    return status;
}
/*********************************************
 * end fbe_api_raid_group_calculate_capacity()
 ********************************************/

/*!***************************************************************
 *  fbe_api_raid_group_class_get_info()
 ****************************************************************
 * @brief
 *  This function determines information about a raid gorup
 *  given the input values.
 *
 * @param get_info_p - pointer to get RG class info
 * @param class_id - class ID
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  1/28/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_raid_group_class_get_info(fbe_api_raid_group_class_get_info_t * get_info_p,
                                  fbe_class_id_t class_id)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_raid_group_class_get_info_t   get_info;

    get_info.raid_type = get_info_p->raid_type;
    get_info.width = get_info_p->width;
    get_info.b_bandwidth = get_info_p->b_bandwidth;
    get_info.exported_capacity = get_info_p->exported_capacity;
    get_info.single_drive_capacity = get_info_p->single_drive_capacity;

    status = fbe_api_common_send_control_packet_to_class(FBE_RAID_GROUP_CONTROL_CODE_CLASS_GET_INFO,
                                                         &get_info,
                                                         sizeof(fbe_raid_group_class_get_info_t),
                                                         class_id,
                                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                                         &status_info,
                                                         FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    get_info_p->exported_capacity = get_info.exported_capacity;
    get_info_p->imported_capacity = get_info.imported_capacity;
    get_info_p->single_drive_capacity = get_info.single_drive_capacity;
    get_info_p->element_size = get_info.element_size;
    get_info_p->elements_per_parity = get_info.elements_per_parity;
    get_info_p->data_disks = get_info.data_disks;
    return status;
}
/*********************************************
 * end fbe_api_raid_group_class_get_info()
 ********************************************/

fbe_status_t FBE_API_CALL
fbe_api_raid_group_class_init_options(fbe_api_raid_group_class_set_options_t * set_options_p)
{
    fbe_zero_memory(set_options_p, sizeof(fbe_api_raid_group_class_set_options_t));
    return FBE_STATUS_OK;
}
/*!***************************************************************
 *  fbe_api_raid_group_class_set_options()
 ****************************************************************
 * @brief
 *  This function determines information about a raid gorup
 *  given the input values.
 *
 * @param get_info_p - pointer to get RG class info
 * @param class_id - class ID
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  8/4/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_raid_group_class_set_options(fbe_api_raid_group_class_set_options_t * set_options_p,
                                     fbe_class_id_t class_id)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_raid_group_class_set_options_t   set_options;

    set_options.paged_metadata_io_expiration_time = set_options_p->paged_metadata_io_expiration_time;
    set_options.user_io_expiration_time = set_options_p->user_io_expiration_time;
    set_options.encrypt_vault_wait_time = set_options_p->encrypt_vault_wait_time;
    set_options.encryption_blob_blocks = set_options_p->encryption_blob_blocks;

    status = fbe_api_common_send_control_packet_to_class(FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_OPTIONS,
                                                         &set_options,
                                                         sizeof(fbe_raid_group_class_set_options_t),
                                                         class_id,
                                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                                         &status_info,
                                                         FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/*********************************************
 * end fbe_api_raid_group_class_set_options()
 ********************************************/

/*!***************************************************************************
 * fbe_api_raid_group_send_set_debug_to_raid_group_class()
 *****************************************************************************
 *
 * @brief   This method sends the control_code passed to ALL the raid groups
 *          for the class specificed. Currently the classes that inherit the 
 *          raid group class:
 *              o FBE_CLASS_ID_MIRROR
 *              o FBE_CLASS_ID_STRIPER
 *              o FBE_CLASS_ID_PARITY
 *              o FBE_CLASS_ID_VIRTUAL_DRIVE
 *
 * @param   control_code - raid group control code to send
 * @param   raid_group_class_id - Class id to send to
 * @param   in_debug_flags - Debug flags to set
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  03/15/2010  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t FBE_API_CALL
fbe_api_raid_group_send_set_debug_to_raid_group_class(fbe_raid_group_control_code_t control_code, 
                                                      fbe_class_id_t raid_group_class_id,
                                                      fbe_u32_t in_debug_flags)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    fbe_u32_t                               set_debug_flags = in_debug_flags;
    fbe_raid_group_control_code_t           class_control_code = FBE_RAID_GROUP_CONTROL_CODE_INVALID;

    /* Validate that control structures are a single enum.
     */
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_raid_group_raid_library_debug_payload_t) == sizeof(fbe_u32_t));
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_raid_group_raid_group_debug_payload_t) == sizeof(fbe_u32_t));

    /* Validate that the control code is supported.
     */
    switch (control_code)
    {
        case FBE_RAID_GROUP_CONTROL_CODE_SET_RAID_LIBRARY_DEBUG_FLAGS:
            class_control_code = FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_RAID_LIBRARY_DEBUG_FLAGS;
            break;

        case FBE_RAID_GROUP_CONTROL_CODE_SET_RAID_GROUP_DEBUG_FLAGS:
            class_control_code = FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_RAID_GROUP_DEBUG_FLAGS;
            break;

        default:
            /* Unsupported control code.
             */
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                         "%s: Unsupported control code: 0x%x \n",
                          __FUNCTION__, control_code);
            return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Validate the riad group class id.
     */
    switch(raid_group_class_id)
    {
        case FBE_CLASS_ID_MIRROR:
        case FBE_CLASS_ID_STRIPER:
        case FBE_CLASS_ID_PARITY:
        case FBE_CLASS_ID_VIRTUAL_DRIVE:
            break;

        default:
            /* Unsupported control code.
             */
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                         "%s: Unsupported raid group class id: %d\n",
                          __FUNCTION__, raid_group_class_id);
            return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Send the class control code to the raid group class.  This changes
     * the default debug flags for any new raid groups created.
     */
    status = fbe_api_common_send_control_packet_to_class(class_control_code,
                                                         &set_debug_flags,
                                                         sizeof(fbe_u32_t),
                                                         /* There is no rg class
                                                          * instances so we need to send 
                                                          * it to one of the leaf 
                                                          * classes. 
                                                          */
                                                         raid_group_class_id,
                                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                                         &status_info,
                                                         FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
                       "%s: line: %d packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__, __LINE__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Always return the execution status.
     */
    return(status);
}
/***************************************************************
 * fbe_api_raid_group_send_set_debug_to_raid_group_class()
 ***************************************************************/

/*!***************************************************************************
 * fbe_api_raid_group_send_control_to_all_raid_group_classes()
 *****************************************************************************
 *
 * @brief   This method sends the control_code passed to ALL the raid group
 *          classes.  Currently the classes that inherit the raid group class:
 *              o FBE_CLASS_ID_MIRROR
 *              o FBE_CLASS_ID_STRIPER
 *              o FBE_CLASS_ID_PARITY
 *
 * @param   control_code - raid group control code to send
 * @param   in_debug_flags - Debug flags to set
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  03/15/2010  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t FBE_API_CALL
fbe_api_raid_group_send_set_debug_to_all_raid_group_classes(fbe_raid_group_control_code_t control_code,
                                                            fbe_u32_t in_debug_flags)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    fbe_u32_t                               set_debug_flags = in_debug_flags;
    fbe_class_id_t                          class_id;
    fbe_raid_group_control_code_t           class_control_code = FBE_RAID_GROUP_CONTROL_CODE_INVALID;
    fbe_u32_t                               retry_count = 0;

    /* Validate that control structures are a single enum.
     */
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_raid_group_raid_library_debug_payload_t) == sizeof(fbe_u32_t));
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_raid_group_raid_group_debug_payload_t) == sizeof(fbe_u32_t));

    /* Validate that the control code is supported.
     */
    switch (control_code)
    {
        case FBE_RAID_GROUP_CONTROL_CODE_SET_RAID_LIBRARY_DEBUG_FLAGS:
            class_control_code = FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_RAID_LIBRARY_DEBUG_FLAGS;
            break;

        case FBE_RAID_GROUP_CONTROL_CODE_SET_RAID_GROUP_DEBUG_FLAGS:
            class_control_code = FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_RAID_GROUP_DEBUG_FLAGS;
            break;

        default:
            /* Unsupported control code.
             */
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                         "%s: Unsupported control code: 0x%x \n",
                          __FUNCTION__, control_code);
            return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* First send the class control code to the raid group class.  This changes
     * the default debug flags for any new raid groups created.
     */
    status = fbe_api_common_send_control_packet_to_class(class_control_code,
                                                         &set_debug_flags,
                                                         sizeof(fbe_u32_t),
                                                         /* There is no rg class
                                                          * instances so we need to send 
                                                          * it to one of the leaf 
                                                          * classes. 
                                                          */
                                                         FBE_CLASS_ID_STRIPER,
                                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                                         &status_info,
                                                         FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
                       "%s: line: %d packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__, __LINE__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Now walk thru all the raid group class ids and and send the object 
     * control code to each object of that class.
     */
    for (class_id = FBE_CLASS_ID_MIRROR; class_id < FBE_CLASS_ID_RAID_LAST; class_id++)
    {
        fbe_u32_t index;
        fbe_u32_t num_objects = 0;
        fbe_object_id_t *obj_list_p = NULL;

        /* First get all members of this class.
         */
        status = fbe_api_enumerate_class(class_id, FBE_PACKAGE_ID_SEP_0, &obj_list_p, &num_objects );

        /* It is perfectly acceptable that there may not be any raid group of a certain class.
         */
        if (status != FBE_STATUS_OK)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                          "%s: enumerate class: 0x%x status: 0x%x obj_list_p: 0x%p\n", 
                          __FUNCTION__, class_id, status, obj_list_p);
            if (obj_list_p != NULL)
            {
                fbe_api_free_memory(obj_list_p);
            }
            return(status);
        }

        /*! @note It is perfectly acceptable that there may not be any raid 
         *        group of a certain class.  But this means that the debug
         *        flags will not be set for that raid group type.
         */
        if ((obj_list_p  == NULL) ||
            (num_objects == 0)       )
        {
            if (obj_list_p != NULL)
            {
                fbe_api_free_memory(obj_list_p);
            }
            continue;
        }

        /* Now send control packet for each object id of this class.
         */
        for ( index = 0; index < num_objects; index++)
        {
            fbe_api_wait_for_object_lifecycle_state(obj_list_p[index], FBE_LIFECYCLE_STATE_READY, 30000, FBE_PACKAGE_ID_SEP_0);

            retry_count = 0;

            do {
                /* Limit the retry for atleast 20 times if the status stays BUSY. 
                 * Otherwise we will end up sitting here forever if the object is in
                 * SPECIALIZE state.. topology return BUSY in that case...
                 */
                if (retry_count != 0) {
                    fbe_api_sleep(50);
                }

                status = fbe_api_common_send_control_packet (control_code,
                                                             &set_debug_flags,
                                                             sizeof(fbe_u32_t),
                                                             obj_list_p[index],
                                                             FBE_PACKET_FLAG_TRAVERSE,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_SEP_0);

                retry_count++;

            } while ((status == FBE_STATUS_BUSY) && (retry_count < 20));

            if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
            {
                if(obj_list_p != NULL){
                    fbe_api_free_memory(obj_list_p);
                }
                fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
                               "%s: line: %d packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                               __FUNCTION__, __LINE__,
                               status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
                return(FBE_STATUS_GENERIC_FAILURE);
            }
        
        } /* end for all objects of this class */

        /* Now free the allocated memory. */
        if(obj_list_p != NULL){
            fbe_api_free_memory(obj_list_p);
        }

    } /* end for all raid group class_ids */

    /* Always return the execution status.
     */
    return(status);
}
/***************************************************************
 * fbe_api_raid_group_send_set_debug_to_all_raid_group_classes()
 ***************************************************************/

/*!***************************************************************************
 * fbe_api_raid_group_set_library_debug_flags()       
 *****************************************************************************
 *
 * @brief   This method set the raid geometry debug flags (which are included
 *          in the raid group object) to the value specified.  If the raid group
 *          object id is FBE_OBJECT_ID_INVALID, it indicates that the flags
 *          should be set for ALL classes that inherit the raid group object.
 *
 * @param   rg_object_id - FBE_OBJECT_ID_INVALID - Set the raid library debug
 *                                  flags for all raid group objects.
 *                         Other - Set the raid library debug flags for the
 *                                  raid group specified.
 * @param   raid_library_debug_flags - Value to set raid library debug flags
 *                                  to. 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  03/15/2010  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_raid_group_set_library_debug_flags(fbe_object_id_t rg_object_id, 
                                           fbe_raid_library_debug_flags_t raid_library_debug_flags)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    fbe_raid_group_raid_library_debug_payload_t  set_debug_flags; 

    /* Populate the control structure.
     */
    set_debug_flags.raid_library_debug_flags = raid_library_debug_flags;

    /* If the object id is invalid it indicates that we should set default and
     * the current raid library debug flags for ALL raid groups to the value
     * indicated.
     */
    if (rg_object_id == FBE_OBJECT_ID_INVALID)
    {
        /* Change the raid library debug flags for ALL raid groups.  In addition any new
         * raid groups created will also use the new raid library debug flags.
         */
        status = fbe_api_raid_group_send_set_debug_to_all_raid_group_classes(FBE_RAID_GROUP_CONTROL_CODE_SET_RAID_LIBRARY_DEBUG_FLAGS,
                                                                             (fbe_u32_t)raid_library_debug_flags);
    }
    else
    {
        /* Else set the raid library debug flags for the raid group specified.
         */
        status = fbe_api_common_send_control_packet (FBE_RAID_GROUP_CONTROL_CODE_SET_RAID_LIBRARY_DEBUG_FLAGS,
                                                     &set_debug_flags,
                                                     sizeof(fbe_raid_group_raid_library_debug_payload_t),
                                                     rg_object_id,
                                                     FBE_PACKET_FLAG_TRAVERSE,
                                                     &status_info,
                                                     FBE_PACKAGE_ID_SEP_0);
        if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
        {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                          "%s: line: %d packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                          __FUNCTION__, __LINE__,
                          status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
            status = FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* Always return the execution status.
     */
    return(status);
}
/**************************************************
 * end fbe_api_raid_group_set_library_debug_flags()
 **************************************************/

/*!***************************************************************************
 * fbe_api_raid_group_set_group_debug_flags()       
 *****************************************************************************
 *
 * @brief   This method set the raid group debug flags (which are included
 *          in the raid group object) to the value specified.  If the raid group
 *          object id is FBE_OBJECT_ID_INVALID, it indicates that the flags
 *          should be set for ALL classes that inherit the raid group object.
 *
 * @param   rg_object_id - FBE_OBJECT_ID_INVALID - Set the raid group debug
 *                                  flags for all raid group objects.
 *                         Other - Set the raid group debug flags for the
 *                                  raid group specified.
 * @param   raid_group_debug_flags - Value to set raid group debug flags to. 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  03/15/2010  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_raid_group_set_group_debug_flags(fbe_object_id_t rg_object_id, 
                                         fbe_raid_group_debug_flags_t raid_group_debug_flags)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    fbe_raid_group_raid_group_debug_payload_t  set_debug_flags; 

    /* Populate the control structure.
     */
    set_debug_flags.raid_group_debug_flags = raid_group_debug_flags;

    /* If the object id is invalid it indicates that we should set default and
     * the current raid library debug flags for ALL raid groups to the value
     * indicated.
     */
    if (rg_object_id == FBE_OBJECT_ID_INVALID)
    {
        /* Change the raid library debug flags for ALL raid groups.  In addition any new
         * raid groups created will also use the new raid library debug flags.
         */
        status = fbe_api_raid_group_send_set_debug_to_all_raid_group_classes(FBE_RAID_GROUP_CONTROL_CODE_SET_RAID_GROUP_DEBUG_FLAGS,
                                                                             (fbe_u32_t)raid_group_debug_flags);
    }
    else
    {
        /* Else set the raid library debug flags for the raid group specified.
         */
        status = fbe_api_common_send_control_packet (FBE_RAID_GROUP_CONTROL_CODE_SET_RAID_GROUP_DEBUG_FLAGS,
                                                     &set_debug_flags,
                                                     sizeof(fbe_raid_group_raid_group_debug_payload_t),
                                                     rg_object_id,
                                                     FBE_PACKET_FLAG_TRAVERSE,
                                                     &status_info,
                                                     FBE_PACKAGE_ID_SEP_0);
        if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
        {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                          "%s: line: %d packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                          __FUNCTION__, __LINE__,
                          status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
            status = FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* Always return the execution status.
     */
    return(status);
}
/************************************************
 * end fbe_api_raid_group_set_group_debug_flags()
 ************************************************/

/*!***************************************************************************
 * fbe_api_raid_group_get_library_debug_flags()       
 *****************************************************************************
 *
 * @brief   This method gets the raid library debug flags (which are included
 *          in the raid group object).  If the raid group object id is 
 *          FBE_OBJECT_ID_INVALID, it indicates that the class default value
 *          for the raid library debug flags should be retrieved.
 *
 * @param   rg_object_id - FBE_OBJECT_ID_INVALID - Get the default raid library 
 *                                  debug flags for the raid group class
 *                         Other -  Get the raid library debug flags for the
 *                                  raid group specified.
 * @param   raid_library_debug_flags_p - Pointer raid library debug flags to populate 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  03/17/2010  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_raid_group_get_library_debug_flags(fbe_object_id_t rg_object_id, 
                                           fbe_raid_library_debug_flags_t *raid_library_debug_flags_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    fbe_raid_group_raid_library_debug_payload_t  get_debug_flags; 

    /* If the object id is invalid it indicates that we should get the default
     * raid library debug flags for all raid group classes.
     */
    if (rg_object_id == FBE_OBJECT_ID_INVALID)
    {
        /* Get the default setting for the raid group class.
         */
        status = fbe_api_common_send_control_packet_to_class(FBE_RAID_GROUP_CONTROL_CODE_CLASS_GET_RAID_LIBRARY_DEBUG_FLAGS,
                                                             &get_debug_flags,
                                                             sizeof(fbe_raid_group_raid_library_debug_payload_t),
                                                             /* There is no rg class
                                                              * instances so we need to send 
                                                              * it to one of the leaf 
                                                              * classes. 
                                                              */
                                                             FBE_CLASS_ID_PARITY,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_SEP_0);
        if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
        {
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
                           "%s: line: %d packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                           __FUNCTION__, __LINE__,
                           status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
            status = FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else
    {
        /* Else get the raid library debug flags for the raid group specified.
         */
        status = fbe_api_common_send_control_packet (FBE_RAID_GROUP_CONTROL_CODE_GET_RAID_LIBRARY_DEBUG_FLAGS,
                                                     &get_debug_flags,
                                                     sizeof(fbe_raid_group_raid_library_debug_payload_t),
                                                     rg_object_id,
                                                     FBE_PACKET_FLAG_TRAVERSE,
                                                     &status_info,
                                                     FBE_PACKAGE_ID_SEP_0);
        if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
        {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                          "%s: line: %d packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                          __FUNCTION__, __LINE__,
                          status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
            status = FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* If success populate the passed value.
     */
    if (status == FBE_STATUS_OK)
    {
        *raid_library_debug_flags_p = get_debug_flags.raid_library_debug_flags;
    }

    /* Always return the execution status.
     */
    return(status);
}
/**************************************************
 * end fbe_api_raid_group_get_library_debug_flags()
 **************************************************/

/*!***************************************************************************
 * fbe_api_raid_group_get_group_debug_flags()       
 *****************************************************************************
 *
 * @brief   This method gets the raid group debug flags (which are included
 *          in the raid group object).  If the raid group object id is 
 *          FBE_OBJECT_ID_INVALID, it indicates that the class default value
 *          for the raid group debug flags should be retrieved.
 *
 * @param   rg_object_id - FBE_OBJECT_ID_INVALID - Get the default raid group 
 *                                  debug flags for the raid group class
 *                         Other -  Get the raid group debug flags for the
 *                                  raid group specified.
 * @param   raid_group_debug_flags_p - Pointer raid group debug flags to populate 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  03/17/2010  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_raid_group_get_group_debug_flags(fbe_object_id_t rg_object_id, 
                                         fbe_raid_group_debug_flags_t *raid_group_debug_flags_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    fbe_raid_group_raid_group_debug_payload_t  get_debug_flags;

    /* If the object id is invalid it indicates that we should get the default
     * raid group debug flags for all raid group classes.
     */
    if (rg_object_id == FBE_OBJECT_ID_INVALID)
    {
        /* Get the default setting for the raid group class.
         */
        status = fbe_api_common_send_control_packet_to_class(FBE_RAID_GROUP_CONTROL_CODE_CLASS_GET_RAID_GROUP_DEBUG_FLAGS,
                                                             &get_debug_flags,
                                                             sizeof(fbe_raid_group_raid_group_debug_payload_t),
                                                             /* There is no rg class
                                                              * instances so we need to send 
                                                              * it to one of the leaf 
                                                              * classes. 
                                                              */
                                                             FBE_CLASS_ID_PARITY,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_SEP_0);
        if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
        {
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
                           "%s: line: %d packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                           __FUNCTION__, __LINE__,
                           status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
            status = FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else
    {
        /* Else get the raid library debug flags for the raid group specified.
         */
        status = fbe_api_common_send_control_packet (FBE_RAID_GROUP_CONTROL_CODE_GET_RAID_GROUP_DEBUG_FLAGS,
                                                     &get_debug_flags,
                                                     sizeof(fbe_raid_group_raid_group_debug_payload_t),
                                                     rg_object_id,
                                                     FBE_PACKET_FLAG_TRAVERSE,
                                                     &status_info,
                                                     FBE_PACKAGE_ID_SEP_0);
        if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
        {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                          "%s: line: %d packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                          __FUNCTION__, __LINE__,
                          status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
            status = FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* If success populate the passed value.
     */
    if (status == FBE_STATUS_OK)
    {
        *raid_group_debug_flags_p = get_debug_flags.raid_group_debug_flags;
    }

    /* Always return the execution status.
     */
    return(status);
}
/************************************************
 * end fbe_api_raid_group_get_group_debug_flags()
 ************************************************/

/*!***************************************************************************
 * fbe_api_raid_group_set_class_library_debug_flags()       
 *****************************************************************************
 *
 * @brief   This method set the raid geometry debug flags (which are included
 *          in the raid group object) to the value specified.  If the raid group
 *          object id is FBE_OBJECT_ID_INVALID, it indicates that the flags
 *          should be set for ALL classes that inherit the raid group object.
 *
 * @param   raid_group_class_id - Set the raid library flag for this raid
 *                                group class id.
 *                
 * @param   raid_library_debug_flags - Value to set raid library debug flags
 *                                  to. 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  03/15/2010  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_raid_group_set_class_library_debug_flags(fbe_class_id_t raid_group_class_id,
                                                 fbe_raid_library_debug_flags_t raid_library_debug_flags)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_raid_group_raid_library_debug_payload_t  set_debug_flags; 

    /* Populate the control structure.
     */
    set_debug_flags.raid_library_debug_flags = raid_library_debug_flags;

    /* Change the raid library debug flags for ALL raid groups of this class.
     */
    status = fbe_api_raid_group_send_set_debug_to_raid_group_class(FBE_RAID_GROUP_CONTROL_CODE_SET_RAID_LIBRARY_DEBUG_FLAGS,
                                                                   raid_group_class_id,
                                                                   (fbe_u32_t)raid_library_debug_flags);

    /* Always return the execution status.
     */
    return(status);
}
/********************************************************
 * end fbe_api_raid_group_set_class_library_debug_flags()
 ********************************************************/

/*!***************************************************************************
 * fbe_api_raid_group_set_class_group_debug_flags()       
 *****************************************************************************
 *
 * @brief   This method set the raid group debug flags (which are included
 *          in the raid group object) to the value specified.  If the raid group
 *          object id is FBE_OBJECT_ID_INVALID, it indicates that the flags
 *          should be set for ALL classes that inherit the raid group object.
 *
 * @param   raid_group_class_id - Set the raid group debug flag for this raid
 *                                group class id.
 *                               
 * @param   raid_group_debug_flags - Value to set raid group debug flags to. 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  03/15/2010  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_raid_group_set_class_group_debug_flags(fbe_class_id_t raid_group_class_id,
                                               fbe_raid_group_debug_flags_t raid_group_debug_flags)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_raid_group_raid_group_debug_payload_t  set_debug_flags; 

    /* Populate the control structure.
     */
    set_debug_flags.raid_group_debug_flags = raid_group_debug_flags;

    /* Change the raid group debug flags for ALL raid groups of this class.
     */
    status = fbe_api_raid_group_send_set_debug_to_raid_group_class(FBE_RAID_GROUP_CONTROL_CODE_SET_RAID_GROUP_DEBUG_FLAGS,
                                                                   raid_group_class_id,
                                                                   (fbe_u32_t)raid_group_debug_flags);

    /* Always return the execution status.
     */
    return(status);
}
/*****************************************************
 * end fbe_api_raid_group_set_class_group_debug_flags()
 *****************************************************/

/*!***************************************************************************
 * fbe_api_raid_group_set_default_debug_flags()       
 *****************************************************************************
 * @brief 
 *   This sets the default debug flags for user and system RGs.
 *
 * @param raid_group_class_id - Set the raid group debug flag for this raid
 *                                group class id.
 * @param user_debug_flags - Value to set raid group debug flags to for
 *                           user raid groups.
 * @param system_debug_flags - Value to set raid group debug flags to for
 *                             system raid groups
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  5/6/2013 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_raid_group_set_default_debug_flags(fbe_class_id_t raid_group_class_id,
                                           fbe_raid_group_debug_flags_t user_debug_flags,
                                           fbe_raid_group_debug_flags_t system_debug_flags)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_group_default_debug_flag_payload_t set_debug_flags; 
    fbe_api_control_operation_status_info_t status_info;

    set_debug_flags.user_debug_flags = user_debug_flags;
    set_debug_flags.system_debug_flags = system_debug_flags;

    /* Send the class control code to the raid group class.  This changes
     * the default debug flags for any new raid groups created.
     */
    status = fbe_api_common_send_control_packet_to_class(FBE_RAID_GROUP_CONTROL_CODE_SET_DEFAULT_DEBUG_FLAGS,
                                                         &set_debug_flags,
                                                         sizeof(fbe_raid_group_default_debug_flag_payload_t),
                                                         /* There is no rg class
                                                          * instances so we need to send 
                                                          * it to one of the leaf 
                                                          * classes. 
                                                          */
                                                         raid_group_class_id,
                                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                                         &status_info,
                                                         FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
                       "%s: line: %d packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__, __LINE__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Always return the execution status.
     */
    return(status);
}
/*****************************************************
 * end fbe_api_raid_group_set_default_debug_flags()
 *****************************************************/

/*!***************************************************************************
 * fbe_api_raid_group_set_default_library_flags()       
 *****************************************************************************
 * @brief 
 *   This sets the default library flags for user and system RGs.
 *
 * @param raid_group_class_id - Set the raid group debug flag for this raid
 *                                group class id.
 * @param user_debug_flags - Value to set raid group debug flags to for
 *                           user raid groups.
 * @param system_debug_flags - Value to set raid group debug flags to for
 *                             system raid groups
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  5/6/2013 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_raid_group_set_default_library_flags(fbe_class_id_t raid_group_class_id,
                                             fbe_raid_group_debug_flags_t user_debug_flags,
                                             fbe_raid_group_debug_flags_t system_debug_flags)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_group_default_library_flag_payload_t set_library_flags; 
    fbe_api_control_operation_status_info_t status_info;

    set_library_flags.user_debug_flags = user_debug_flags;
    set_library_flags.system_debug_flags = system_debug_flags;

    /* Send the class control code to the raid group class.  This changes
     * the default debug flags for any new raid groups created.
     */
    status = fbe_api_common_send_control_packet_to_class(FBE_RAID_GROUP_CONTROL_CODE_SET_DEFAULT_LIBRARY_FLAGS,
                                                         &set_library_flags,
                                                         sizeof(fbe_raid_group_default_library_flag_payload_t),
                                                         /* There is no rg class
                                                          * instances so we need to send 
                                                          * it to one of the leaf 
                                                          * classes. 
                                                          */
                                                         raid_group_class_id,
                                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                                         &status_info,
                                                         FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
                       "%s: line: %d packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__, __LINE__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Always return the execution status.
     */
    return(status);
}
/*****************************************************
 * end fbe_api_raid_group_set_default_library_flags()
 *****************************************************/
/*!***************************************************************************
 * fbe_api_raid_group_get_default_debug_flags()       
 *****************************************************************************
 * @brief 
 *   This gets the default debug flags for user and system RGs
 *
 * @param raid_group_class_id - get the raid group debug flag for this raid
 *                                group class id.
 * @param debug_flags_p - output value of debug flags. 
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  5/6/2013 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_raid_group_get_default_debug_flags(fbe_class_id_t raid_group_class_id,
                                           fbe_raid_group_default_debug_flag_payload_t *debug_flags_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;

    /* Send the class control code to the raid group class.  This changes
     * the default debug flags for any new raid groups created.
     */
    status = fbe_api_common_send_control_packet_to_class(FBE_RAID_GROUP_CONTROL_CODE_GET_DEFAULT_DEBUG_FLAGS,
                                                         debug_flags_p,
                                                         sizeof(fbe_raid_group_default_debug_flag_payload_t),
                                                         /* There is no rg class
                                                          * instances so we need to send 
                                                          * it to one of the leaf 
                                                          * classes. 
                                                          */
                                                         raid_group_class_id,
                                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                                         &status_info,
                                                         FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
                       "%s: line: %d packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__, __LINE__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Always return the execution status.
     */
    return(status);
}
/*****************************************************
 * end fbe_api_raid_group_get_default_debug_flags()
 *****************************************************/

/*!***************************************************************************
 * fbe_api_raid_group_get_default_library_flags()       
 *****************************************************************************
 * @brief 
 *   This gets the default library flags for user and system RGs.
 *
 * @param raid_group_class_id - get the raid group debug flag for this raid
 *                                group class id.
 * @param debug_flags_p - Output values of debug flags.
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  5/6/2013 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_raid_group_get_default_library_flags(fbe_class_id_t raid_group_class_id,
                                             fbe_raid_group_default_library_flag_payload_t *debug_flags_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;

    /* Send the class control code to the raid group class.  This changes
     * the default debug flags for any new raid groups created.
     */
    status = fbe_api_common_send_control_packet_to_class(FBE_RAID_GROUP_CONTROL_CODE_GET_DEFAULT_LIBRARY_FLAGS,
                                                         debug_flags_p,
                                                         sizeof(fbe_raid_group_default_library_flag_payload_t),
                                                         /* There is no rg class
                                                          * instances so we need to send 
                                                          * it to one of the leaf 
                                                          * classes. 
                                                          */
                                                         raid_group_class_id,
                                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                                         &status_info,
                                                         FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
                       "%s: line: %d packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__, __LINE__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Always return the execution status.
     */
    return(status);
}
/*****************************************************
 * end fbe_api_raid_group_get_default_library_flags()
 *****************************************************/
/*!***************************************************************************
 * fbe_api_raid_group_get_raid_lib_error_testing()
 *****************************************************************************
 *
 * @brief   This method enables the raid library error testing functionality.
 *
 * @param   None
 *
 * @return  fbe_status_t
 *
 * @version
 *  07/09/2010  Jyoti Ranjan  - Created.
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_raid_group_set_raid_lib_error_testing(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t  status_info;

    /* Now, send the control code to class. 
     */
    status = fbe_api_common_send_control_packet_to_class(FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_RAID_LIBRARY_ERROR_TESTING,
                                                         NULL, /* no struct */
                                                         0, /* no size */
                                                         FBE_CLASS_ID_PARITY,
                                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                                         &status_info,
                                                         FBE_PACKAGE_ID_SEP_0);


    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                      "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n",
                      __FUNCTION__,
                      status, 
                      status_info.packet_qualifier, 
                      status_info.control_operation_status,
                      status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/************************************************
 * end fbe_api_raid_group_set_raid_lib_error_testing()
 ************************************************/

/*!***************************************************************************
 * fbe_api_raid_group_set_raid_lib_error_testing()
 *****************************************************************************
 *
 * @brief This method enables the random raid library error testing functionality.
 *
 * @param   percentage - percentage of time 0..100 to inject errors.
 * @param   b_only_user_rgs - FBE_TRUE to only inject on user rgs
 *                            FBE_FALSE to inject on system and user rgs.
 * @return  fbe_status_t
 *
 * @version
 *  6/27/2012 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_raid_group_enable_raid_lib_random_errors(fbe_u32_t percentage,
                                                                           fbe_bool_t b_only_user_rgs)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t  status_info;
    fbe_raid_group_class_set_library_errors_t set_errors;

    set_errors.inject_percentage = percentage;
    set_errors.b_only_inject_user_rgs = b_only_user_rgs;

    /* Now, send the control code to class. 
     */
    status = fbe_api_common_send_control_packet_to_class(FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_RAID_LIBRARY_RANDOM_ERRORS,
                                                         &set_errors, /* no struct */
                                                         sizeof(fbe_raid_group_class_set_library_errors_t),
                                                         FBE_CLASS_ID_PARITY,
                                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                                         &status_info,
                                                         FBE_PACKAGE_ID_SEP_0);


    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                      "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n",
                      __FUNCTION__,
                      status, 
                      status_info.packet_qualifier, 
                      status_info.control_operation_status,
                      status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/************************************************
 * end fbe_api_raid_group_set_raid_lib_error_testing()
 ************************************************/

/*!***************************************************************************
 * fbe_api_raid_lib_get_error_testing_stats()       
 *****************************************************************************
 *
 * @brief   This functions gets the statistics of error testing
 *
 * @param   raid_lib_error_stats_p - pointer to return error stats information
 *
 * @return  fbe_status_t
 *
 * @version
 *  07/09/2010  Jyoti Ranjan  - Created.
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_raid_group_get_raid_library_error_testing_stats(fbe_api_raid_library_error_testing_stats_t *error_testing_stats_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t  status_info;
    fbe_raid_group_raid_library_error_testing_stats_payload_t error_stats_payload;

    /* Send the control packet to retrieve error-testing stats
     */
    status = fbe_api_common_send_control_packet_to_class(FBE_RAID_GROUP_CONTROL_CODE_CLASS_GET_RAID_LIBRARY_ERROR_TESTING_STATS,
                                                         &error_stats_payload,
                                                         sizeof(fbe_raid_group_raid_library_error_testing_stats_payload_t),
                                                         FBE_CLASS_ID_PARITY,
                                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                                         &status_info,
                                                         FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                      "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n",
                      __FUNCTION__,
                      status, 
                      status_info.packet_qualifier, 
                      status_info.control_operation_status,
                      status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    error_testing_stats_p->error_count = error_stats_payload.error_stats.error_count;
    return status;
}
/************************************************
 * end fbe_api_raid_group_get_raid_library_error_testing_stats()
 ************************************************/

/*!***************************************************************************
 * fbe_api_raid_group_get_raid_memory_stats()       
 *****************************************************************************
 *
 * @brief   This functions gets the statistics of raid memory
 *
 * @param   memory_stats_p - pointer to return raid memory stats information
 *
 * @return  fbe_status_t
 *
 * @version
 *  03/07/2011  Swati Fursule  - Created.
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_raid_group_get_raid_memory_stats(fbe_api_raid_memory_stats_t *memory_stats_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t  status_info;
    fbe_api_raid_memory_stats_t memory_stats_payload;

    /* Send the control packet to retrieve error-testing stats
     */
    status = fbe_api_common_send_control_packet_to_class(FBE_RAID_GROUP_CONTROL_CODE_CLASS_GET_RAID_MEMORY_STATS,
                                                         &memory_stats_payload,
                                                         sizeof(fbe_api_raid_memory_stats_t),
                                                         FBE_CLASS_ID_PARITY,
                                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                                         &status_info,
                                                         FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                      "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n",
                      __FUNCTION__,
                      status, 
                      status_info.packet_qualifier, 
                      status_info.control_operation_status,
                      status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* copy the statistics
     */
    memory_stats_p->allocations = memory_stats_payload.allocations;
    memory_stats_p->frees = memory_stats_payload.frees;
    memory_stats_p->allocated_bytes = memory_stats_payload.allocated_bytes;
    memory_stats_p->freed_bytes = memory_stats_payload.freed_bytes;
    memory_stats_p->deferred_allocations = memory_stats_payload.deferred_allocations;
    memory_stats_p->pending_allocations = memory_stats_payload.pending_allocations;
    memory_stats_p->aborted_allocations = memory_stats_payload.aborted_allocations;
    memory_stats_p->allocation_errors = memory_stats_payload.allocation_errors;
    memory_stats_p->free_errors = memory_stats_payload.free_errors;
    return status;
}
/************************************************
 * end fbe_api_raid_group_get_raid_memory_stats()
 ************************************************/

/*!***************************************************************************
 * fbe_api_raid_group_reset_raid_memory_stats()       
 *****************************************************************************
 *
 * @brief   This functions resets the statistics of raid memory
 *
 * @param   none
 *
 * @return  fbe_status_t
 *
 * @note    It is assumed that there is no active I/O.
 *
 * @version
 *  02/10/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_raid_group_reset_raid_memory_stats(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t  status_info;

    /* Reset (to zero) the raid group memory statistics.  There is no
     * guarantee they will be zero.
     */
    status = fbe_api_common_send_control_packet_to_class(FBE_RAID_GROUP_CONTROL_CODE_CLASS_RESET_RAID_MEMORY_STATS,
                                                         NULL, /* no struct */
                                                         0, /* no size */
                                                         FBE_CLASS_ID_PARITY,
                                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                                         &status_info,
                                                         FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
                       "%s: line: %d packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__, __LINE__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/************************************************
 * end fbe_api_raid_group_reset_raid_memory_stats()
 ************************************************/

/*!***************************************************************************
 * fbe_api_raid_group_reset_raid_lib_error_testing_stats()       
 *****************************************************************************
 *
 * @brief   This method resets the statistics of error-testing. Also, it disables
 *          the error testing functionality.
 *
 * @param   None
 *
 * @return  fbe_status_t
 *
 * @version
 *  07/08/2010  Jyoti Ranjan  - Created.
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_raid_group_reset_raid_lib_error_testing_stats(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t  status_info;

    /* Erase all information stored in queue used to maintain traced RAID_COND() 
     * information. It also disables error testing path.
     */
    status = fbe_api_common_send_control_packet_to_class(FBE_RAID_GROUP_CONTROL_CODE_CLASS_RESET_RAID_LIBRARY_ERROR_TESTING_STATS,
                                                         NULL, /* no struct */
                                                         0, /* no size */
                                                         FBE_CLASS_ID_PARITY,
                                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                                         &status_info,
                                                         FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
                       "%s: line: %d packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__, __LINE__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    return status;
}
/************************************************
 * end fbe_api_raid_group_reset_raid_lib_error_testing_stats()
 ************************************************/

/*!***************************************************************
 * @fn fbe_api_raid_group_get_power_saving_policy(fbe_object_id_t                 in_rg_object_id,
                                            fbe_raid_group_get_power_saving_info_t*    in_policy_p)
 ****************************************************************
 * @brief
 *  This function gets the power saving policy of the RG
 *
 * @param in_rg_object_id      - The RG object ID
 * @param in_policy_p   - pointer to the power saving policy of the LU
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_raid_group_get_power_saving_policy(
                            fbe_object_id_t                 in_rg_object_id,
                            fbe_raid_group_get_power_saving_info_t*    in_policy_p)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    if (in_policy_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: NULL buffer pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_RAID_GROUP_CONTROL_CODE_GET_POWER_SAVING_PARAMETERS,
                                                 in_policy_p,
                                                 sizeof(fbe_raid_group_get_power_saving_info_t),
                                                 in_rg_object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}   // end fbe_api_raid_group_get_power_saving_policy()

/*!***************************************************************
 * @fn fbe_api_destroy_rg(fbe_api_destroy_rg_t *destroy_rg_struct_p, fbe_bool_t wait_destroy_b, 
 *                        fbe_u32_t ready_timeout_msec, fbe_job_service_error_type_t *job_error_type)
****************************************************************
* @brief
*  This function destroys a RG. It is synchronous and will return once
*  the RG is destroyed.
*
* @param destroy_rg_struct_p - The data needed for destroying a RG
* @param wait_destroy_b - does the user want to wait for the RG to be gone completely
* @param ready_timeout_msec - how long to wait for the RG to become available
* @param job_error_type - pointer to job error type which will be used by Admin.
* @return
*  fbe_status_t
*
****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_destroy_rg(fbe_api_destroy_rg_t *destroy_rg_struct_p, 
                                             fbe_bool_t wait_destroy_b, 
                                             fbe_u32_t destroy_timeout_msec,
                                             fbe_job_service_error_type_t *job_error_type)
{
    fbe_status_t                              status = FBE_STATUS_OK;
    fbe_api_job_service_raid_group_destroy_t  rg_destroy_job = {0};
    fbe_status_t                              job_status = FBE_STATUS_OK;

    if (job_error_type == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: failed to pass job_error_type pointer. \n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Fill in the needed information*/
    rg_destroy_job.raid_group_id = destroy_rg_struct_p->raid_group_id;
    rg_destroy_job.wait_destroy = wait_destroy_b;
    rg_destroy_job.destroy_timeout_msec = destroy_timeout_msec;
    /* Start the job*/
    status = fbe_api_job_service_raid_group_destroy(&rg_destroy_job);
    if (status != FBE_STATUS_OK) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: failed to start RG destruction job\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* wait for it*/
    status = fbe_api_common_wait_for_job(rg_destroy_job.job_number, destroy_timeout_msec, job_error_type, &job_status, NULL);
    if (status != FBE_STATUS_OK || job_status != FBE_STATUS_OK) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: wait for RG job failed. Status: 0x%X, job status:0x%X, job error:0x%x\n",
                                     __FUNCTION__, status, job_status, *job_error_type);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* JOB service returned error and so cannot destroy RG.. */
    if(*job_error_type != FBE_JOB_SERVICE_ERROR_NO_ERROR)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                      "RG cannot be deleted.. job_error_type: 0x%x (%s)\n",
                      *job_error_type, 
                      fbe_api_job_service_notification_error_code_to_string(*job_error_type));

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

} // end fbe_api_destroy_rg()
/*!***************************************************************
 *  @fn fbe_api_raid_group_get_paged_bits(fbe_object_id_t rg_object_id,fbe_api_raid_group_get_paged_info_t *paged_info_p)
 ****************************************************************
 * @brief
 *  This function returns the summation of the paged bits
 *  from the raid group's paged metadata.
 *
 * @param rg_object_id - RG object id
 * @param paged_info_p - Ptr to paged summation structure.
 * @param b_force_unit_access - FBE_TRUE - Get the paged data from disk (not from cache)
 *                              FBE_FALSE - Allow the data to come from cache   
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  4/26/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_raid_group_get_paged_bits(fbe_object_id_t rg_object_id,
                                  fbe_api_raid_group_get_paged_info_t *paged_info_p,
                                  fbe_bool_t b_force_unit_access)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_u32_t index;
    fbe_chunk_index_t total_chunks;
    fbe_chunk_index_t chunks_remaining;
    fbe_u64_t metadata_offset = 0;
    fbe_chunk_index_t current_chunks;
    fbe_chunk_index_t chunks_per_request;
    fbe_chunk_index_t chunk_index;
    fbe_api_base_config_metadata_paged_get_bits_t get_bits;

    if (paged_info_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Clear this out since we will be adding to it.
     */
    fbe_zero_memory(paged_info_p, sizeof(fbe_api_raid_group_get_paged_info_t));
    
    /* Get the information about the capacity so we can calculate number of chunks.
     */
    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK) 
    { 
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: rg get info status: 0x%x\n", __FUNCTION__, status);
        return status; 
    }

    total_chunks = (rg_info.capacity / rg_info.num_data_disk) / FBE_RAID_DEFAULT_CHUNK_SIZE;
    chunks_remaining = total_chunks;
    chunks_per_request = FBE_PAYLOAD_METADATA_MAX_DATA_SIZE / sizeof(fbe_raid_group_paged_metadata_t);

    /* If the data must come from disk flag that fact.
     */
    if (b_force_unit_access == FBE_TRUE)
    {
        get_bits.get_bits_flags = FBE_API_BASE_CONFIG_METADATA_PAGED_GET_BITS_FLAGS_FUA_READ;  
    }
    else
    {
        get_bits.get_bits_flags = 0;
    }
    paged_info_p->first_rekey_chunk = FBE_U32_MAX;
    paged_info_p->last_rekey_chunk = FBE_U32_MAX;

    /* Loop over all the chunks, and sum up the bits across all chunks. 
     * We will fetch as many chunks as we can per request (chunks_per_request).
     */
    chunk_index = 0;
    while (chunks_remaining > 0)
    {
        fbe_u32_t wait_msecs = 0;
        fbe_raid_group_paged_metadata_t *paged_p = (fbe_raid_group_paged_metadata_t *)&get_bits.metadata_record_data[0];

        current_chunks = FBE_MIN(chunks_per_request, chunks_remaining);
        if (current_chunks > FBE_U32_MAX)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: current_chunks: %d > FBE_U32_MAX\n", __FUNCTION__, (int)current_chunks);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        get_bits.metadata_offset = metadata_offset;
        get_bits.metadata_record_data_size = (fbe_u32_t)(current_chunks * sizeof(fbe_raid_group_paged_metadata_t));

        /* Fetch the bits for this set of chunks.  It is possible to get errors back
         * if the object is quiescing, so just retry briefly. 
         */
        status = fbe_api_base_config_metadata_paged_get_bits(rg_object_id, &get_bits);
        while (status != FBE_STATUS_OK)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s: get bits status: 0x%x\n", __FUNCTION__, status);
            if (wait_msecs > 30000) 
            { 
                fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: status: 0x%x timed out\n", __FUNCTION__, status);
                return FBE_STATUS_TIMEOUT; 
            }
            fbe_api_sleep(500);
            wait_msecs += 500;

            status = fbe_api_base_config_metadata_paged_get_bits(rg_object_id, &get_bits);
        }
        /* For each chunk in this request keep track of how many were marked NR.
         */
        for ( index = 0; index < current_chunks; index++)
        {
            fbe_u32_t pos;
            fbe_raid_position_bitmask_t bits_remaining;
            paged_info_p->chunk_count ++;
            if (paged_p->needs_rebuild_bits)
            {
                paged_info_p->needs_rebuild_bitmask |= paged_p->needs_rebuild_bits;
                paged_info_p->num_nr_chunks++;
                bits_remaining = paged_info_p->needs_rebuild_bitmask;

                /* Loop over all the bits, and keep track of how many chunks were
                 * marked per position. 
                 */
                for ( pos = 0; 
                      bits_remaining && (pos < rg_info.width); 
                      pos++)
                {
                    if (bits_remaining & (1 << pos))
                    {
                        paged_info_p->pos_nr_chunks[pos]++;
                        bits_remaining &= ~(1 << pos);
                    }
                }
                //fbe_api_trace(FBE_TRACE_LEVEL_INFO, "rgid: 0x%x Chunk: %d nr_bits: 0x%x",
                //              rg_object_id, index, paged_p->needs_rebuild_bits);
            }
            if (paged_p->verify_bits)
            {
                if (paged_p->verify_bits & FBE_RAID_BITMAP_VERIFY_USER_READ_WRITE)
                {
                    paged_info_p->num_user_vr_chunks++;
                }
                if (paged_p->verify_bits & FBE_RAID_BITMAP_VERIFY_ERROR)
                {
                    paged_info_p->num_error_vr_chunks++;
                }
                if (paged_p->verify_bits & FBE_RAID_BITMAP_VERIFY_USER_READ_ONLY)
                {
                    paged_info_p->num_read_only_vr_chunks++;
                }
                if (paged_p->verify_bits & FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE)
                {
                    paged_info_p->num_incomplete_write_vr_chunks++;
                }
                if (paged_p->verify_bits & FBE_RAID_BITMAP_VERIFY_SYSTEM)
                {
                    paged_info_p->num_system_vr_chunks++;
                }              
            }
            if (paged_p->rekey) 
            {
                paged_info_p->num_rekey_chunks++;
                if (paged_info_p->first_rekey_chunk == FBE_U32_MAX) {
                    paged_info_p->first_rekey_chunk = chunk_index;
                }
                if ((paged_info_p->last_rekey_chunk == FBE_U32_MAX) ||
                    (chunk_index > paged_info_p->last_rekey_chunk)) {
                    paged_info_p->last_rekey_chunk = chunk_index;
                }
            }
            chunk_index++;
            paged_p++;
        }
        metadata_offset += current_chunks * sizeof(fbe_raid_group_paged_metadata_t);
        chunks_remaining -= current_chunks;
    }
    return status;
}
/**************************************
 * end fbe_api_raid_group_get_paged_bits()
 **************************************/

/*!***************************************************************
 *  @fn fbe_api_raid_group_get_rebuild_status(fbe_object_id_t rg_object_id,
 *                                            fbe_api_rg_get_status_t *rebuild_status)
 ****************************************************************
 * @brief
 *  This function returns the rebuild status for RG
 *
 * @param rg_object_id - RG object id
 * @param rebuild_status - ptr to data structure that will hold
 *                          rebuild status
 *
 * @return
 *  fbe_status_t - status
 *
 * @version
 *  7/09/2011 - Created. Sonali K
 *  6/18/2012 - Modified. Vera Wang
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_raid_group_get_rebuild_status(fbe_object_id_t rg_object_id, 
                                                                fbe_api_rg_get_status_t *rebuild_status_p)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_raid_group_rebuild_status_t                 rg_rebuild_status;

    if (rebuild_status_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: NULL buffer pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_RAID_GROUP_CONTROL_CODE_GET_REBUILD_STATUS,
                                                 &rg_rebuild_status,
                                                 sizeof(fbe_raid_group_rebuild_status_t),
                                                 rg_object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    rebuild_status_p->checkpoint = rg_rebuild_status.rebuild_checkpoint;
    rebuild_status_p->percentage_completed = rg_rebuild_status.rebuild_percentage;

    return status;
}

/*!***************************************************************
 *  @fn fbe_api_raid_group_get_paged_metadata_verify_pass_count(fbe_object_id_t rg_object_id,
 *                                                              fbe_u32_t *pass_count)
 ****************************************************************
 * @brief
 *  This function returns verify count of the paged metadata region
 *
 * @param rg_object_id - RG object id
 * @param pass_count - ptr to store the metadata verify count
 *
 * @return
 *  fbe_status_t - status
 *
 * @version
 *  01/12/2012 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_raid_group_get_paged_metadata_verify_pass_count(fbe_object_id_t rg_object_id, 
                                                                                  fbe_u32_t *pass_count)
{
    fbe_api_base_config_downstream_object_list_t downstream_object_list; 
    fbe_api_raid_group_get_info_t rg_info;
    fbe_status_t  status = FBE_STATUS_OK;   
    fbe_api_raid_group_get_info_t rg_mirror_info;
    fbe_u16_t index;
     
    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    if(status != FBE_STATUS_OK)
    {          
        return status;
    }
       
    /*Additional calculations to handle Raid 10*/
    if(rg_info.raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        /* There could be multiple RG objects and some of them might NOT have completed the verify
         * pass and so just return the lowest pass count of all possible RG objects
         */
         status = fbe_api_base_config_get_downstream_object_list(rg_object_id, &downstream_object_list);
         if(status != FBE_STATUS_OK)
         {          
             return status;
         }
         status = fbe_api_raid_group_get_info(downstream_object_list.downstream_object_list[0], 
                                              &rg_mirror_info, 
                                              FBE_PACKET_FLAG_NO_ATTRIB);

         if(status != FBE_STATUS_OK)
         {          
             return status;
         }
         *pass_count = rg_mirror_info.paged_metadata_verify_pass_count;

         for(index = 1;index < downstream_object_list.number_of_downstream_objects; index++)
         {
             status = fbe_api_raid_group_get_info(downstream_object_list.downstream_object_list[index], 
                                                  &rg_mirror_info, 
                                                  FBE_PACKET_FLAG_NO_ATTRIB);

             if(status != FBE_STATUS_OK)
             {          
                 return status;
             }
             if(rg_info.paged_metadata_verify_pass_count < *pass_count)
             {
                 *pass_count = rg_mirror_info.paged_metadata_verify_pass_count;
             }
             
         }
    }
    else 
    {
        *pass_count = rg_info.paged_metadata_verify_pass_count;
    }
    return status;
}

/*!***************************************************************
 *  @fn fbe_api_raid_group_get_bgverify_status(fbe_object_id_t rg_object_id,
 *                                            fbe_api_rg_get_status_t *bgverify_status,
 *                                            fbe_lun_verify_type_t verify_type)
 ****************************************************************
 * @brief
 *  This function returns the BG verify status for RG
 *
 * @param rg_object_id - RG object id
 * @param bgverify_status - ptr to data structure that will hold
 *                          bg verify status
 * @param verify_type - type of verify RO/RW.
 *
 * @return
 *  fbe_status_t - status
 *
 * @version
 *  7/09/2011 - Created. Sonali K
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_raid_group_get_bgverify_status(fbe_object_id_t rg_object_id, 
                                                                 fbe_api_rg_get_status_t *bgverify_status,
                                                                 fbe_lun_verify_type_t verify_type)
{
    fbe_api_base_config_downstream_object_list_t downstream_object_list;          
    fbe_api_raid_group_get_info_t rg_info,rg_mirror_info; 
    fbe_status_t  status = FBE_STATUS_OK;
    fbe_lba_t min_chkpt = FBE_LBA_INVALID; 
    fbe_lba_t capacity,rg_checkpoint; 
    fbe_u16_t index,num_data_disk;  
    
    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    if(status != FBE_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: rg get info failed, obj 0x%x, status: 0x%x\n", 
                      __FUNCTION__, rg_object_id, status);
        return status;
    }
     
    /*Additional calculations for Raid 10*/
    if(rg_info.raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        status = fbe_api_base_config_get_downstream_object_list(rg_object_id, &downstream_object_list);
        if(status != FBE_STATUS_OK)
        {          
            return status;
        }
        for(index = 0;index < downstream_object_list.number_of_downstream_objects; index++)
        {
            status = fbe_api_raid_group_get_info(downstream_object_list.downstream_object_list[index], 
                                                 &rg_mirror_info, 
                                                 FBE_PACKET_FLAG_NO_ATTRIB);
            if(status != FBE_STATUS_OK)
            {          
                return status;
            }
            status = fbe_raid_group_get_verify_checkpoint(rg_mirror_info,verify_type,&rg_checkpoint);     
                                        
            if(status != FBE_STATUS_OK)
            {
                fbe_api_trace(FBE_TRACE_LEVEL_WARNING,                   
                              "%s:Failed to get BG verify status for RG!!\n",
                              __FUNCTION__);

                return status;
            }
            if(rg_checkpoint <= min_chkpt)
            {
                min_chkpt = rg_checkpoint;
                capacity = rg_mirror_info.capacity;
                num_data_disk = rg_mirror_info.num_data_disk;                     
            }                          
        }        
        bgverify_status->checkpoint = min_chkpt;
         

    }else
    {
        status = fbe_raid_group_get_verify_checkpoint(rg_info,verify_type,&rg_checkpoint);
        if(status != FBE_STATUS_OK)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING,                   
                          "%s:Failed to get BG verify status for RG!!\n",
                          __FUNCTION__);

            return status;
        }
        capacity = rg_info.capacity;
        bgverify_status->checkpoint = rg_checkpoint;
        num_data_disk = rg_info.num_data_disk;
    }
      /* calculating the rebuild percentage value for RG*/ 
    if(bgverify_status->checkpoint == FBE_LBA_INVALID)
    {
        bgverify_status->percentage_completed = 100;
       
    }else
    {         
        bgverify_status->percentage_completed = (fbe_u16_t)((bgverify_status->checkpoint * 100 * num_data_disk)/capacity);   
    }    
    fbe_api_trace(FBE_TRACE_LEVEL_INFO,                   
                  "BG Verify checkpoint for RG 0x%x is 0x%llx and BGV percentage completed is %d\n",
                  rg_object_id,
		  (unsigned long long)bgverify_status->checkpoint,
		  bgverify_status->percentage_completed );

    return status;
}

/*!***************************************************************
 *  @fn fbe_api_raid_group_get_physical_from_logical_lba()
 ****************************************************************
 * @brief
 *  This function calculates the PBA for the given LBA
 *
 * @param rg_object_id - RG object id
 * @param lba - the LBA to translate to PBA
 * @param pba_p - pointer to the PBA.
 *
 * @return
 *  fbe_status_t - status
 *
 * @version
 *  10/18/2011 - Created. Jason White
 *  6/4/2012 - Rewrote to use map_lba interface. Rob Foley
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_raid_group_get_physical_from_logical_lba(fbe_object_id_t rg_object_id,
                                                                           fbe_lba_t lba,
                                                                           fbe_u64_t *pba_p)
{
    fbe_status_t                   status = FBE_STATUS_OK;
    fbe_raid_group_map_info_t      rg_map_info;

    rg_map_info.lba = lba;
    status = fbe_api_raid_group_map_lba(rg_object_id, &rg_map_info);
    *pba_p = rg_map_info.pba;
    return status;
}
/**************************************
 * end fbe_api_raid_group_get_physical_from_logical_lba()
 **************************************/
/*!****************************************************************************
 * fbe_raid_group_get_verify_checkpoint(fbe_api_raid_group_get_info_t rg_info,
                                        fbe_lun_verify_type_t verify_type,
                                        fbe_lba_t *rg_checkpoint)
 ******************************************************************************
 *
 * @brief
 *    This function is used to get verify_checkpoint from RG info.
 *
 * @param   rg_info                -  rg info .
 * @param   verify_type            -  lun verify type.
 * @param   rg_checkpoint          -  pointer to rg verify checkpoint value
 * 
 * 
 * @return fbe_status_t status
 * 
 * @version
 *    09/2011 - Created. Sonali K
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_get_verify_checkpoint(fbe_api_raid_group_get_info_t rg_info,
                                      fbe_lun_verify_type_t verify_type, fbe_lba_t *rg_checkpoint)
{
     fbe_status_t  status = FBE_STATUS_OK;  

     if(verify_type == FBE_LUN_VERIFY_TYPE_USER_RW)
     {
         *rg_checkpoint = rg_info.rw_verify_checkpoint;
    
     }else if (verify_type == FBE_LUN_VERIFY_TYPE_USER_RO)
     {
         *rg_checkpoint = rg_info.ro_verify_checkpoint;
    
     }else if (verify_type == FBE_LUN_VERIFY_TYPE_ERROR)
     {
         *rg_checkpoint = rg_info.error_verify_checkpoint;
    
     }else if (verify_type == FBE_LUN_VERIFY_TYPE_INCOMPLETE_WRITE)
     {
         *rg_checkpoint = rg_info.incomplete_write_verify_checkpoint;
    
     }else if (verify_type == FBE_LUN_VERIFY_TYPE_SYSTEM)     
     {
         *rg_checkpoint = rg_info.system_verify_checkpoint;
    
     }else
     {   
         fbe_api_trace(FBE_TRACE_LEVEL_WARNING,                      
                      "%s:Illegal Verify type for lun specified!!x\n",__FUNCTION__);                     

         status =  FBE_STATUS_GENERIC_FAILURE;
     }

     return status;
} 

/*!****************************************************************************
 * fbe_api_raid_group_get_min_rebuild_checkpoint(fbe_api_raid_group_get_info_t rg_info,
                                             fbe_lba_t *min_chkpt)
 ******************************************************************************
 *
 * @brief
 *    This function is used to get min rebuild checkpoint from RG info.
 *
 * @param   rg_info  - rg info
 * @param  min_chkpt - pointer to min rebuild checkpoint value
 * 
 * @return void
 * 
 * @version
 *    09/2011 - Created. Sonali K
 *
 ******************************************************************************/
static void fbe_api_raid_group_get_min_rebuild_checkpoint(fbe_api_raid_group_get_info_t rg_info,
                                                                    fbe_lba_t *min_chkpt)
{     
     fbe_u16_t index;

     /* Finding the minimum checkpoint value from the disks in the RG
        */        
     for (index = 0;index < rg_info.width; index++)
     {
         fbe_api_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM,                      
                       "Checkpoint for drive at index %d : 0x%llx\n",index,
                       (unsigned long long)rg_info.rebuild_checkpoint[index]);
    
         *min_chkpt = FBE_MIN(*min_chkpt,rg_info.rebuild_checkpoint[index]);               
     }
}

/*!***************************************************************
 * @fn fbe_api_raid_group_get_max_unused_extent_size(fbe_object_id_t  in_rg_object_id, 
 *          fbe_block_transport_control_get_max_unused_extent_size_t* out_extent_size_p)
 ****************************************************************
 * @brief
 *  This function is used to send a request to get the extent size from the block transport server.
 *
 * @param in_rg_object_id     - RG object ID
 * @param out_extent_size_p   - pointer to the extent size
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_raid_group_get_max_unused_extent_size(fbe_object_id_t  in_rg_object_id, 
                                              fbe_block_transport_control_get_max_unused_extent_size_t* out_extent_size_p)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;


    status = fbe_api_common_send_control_packet(FBE_BLOCK_TRANSPORT_CONTROL_CODE_GET_MAX_UNUSED_EXTENT_SIZE,
                                                out_extent_size_p,
                                                sizeof(fbe_block_transport_control_get_max_unused_extent_size_t),
                                                in_rg_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "_get_max_unused_extent_size: pkt_err:%d, pkt_qual:%d, pyld_err:%d, pyld_qual:%d\n", 
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) 
        {
            return status;
        }
        else
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
        
    return status;

} /* end of fbe_api_raid_group_get_max_unused_extent_size() */


/*!***************************************************************
 * @fn fbe_api_raid_group_set_rebuild_checkpoint(fbe_object_id_t  in_rg_object_id, 
 *                                               fbe_raid_group_set_rb_checkpoint_t in_rg_set_checkpoint)
 ****************************************************************
 * @brief
 *  This function is used to set the rebuild checkpoint.
 *
 * @param in_rg_object_id           - RG object ID
 * @param in_rg_set_checkpoint      - rebuild checkpoint to set
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_raid_group_set_rebuild_checkpoint(fbe_object_id_t  in_rg_object_id, 
                                          fbe_raid_group_set_rb_checkpoint_t in_rg_set_checkpoint)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;


    status = fbe_api_common_send_control_packet(FBE_RAID_GROUP_CONTROL_CODE_SET_RB_CHECKPOINT,
                                                &in_rg_set_checkpoint,
                                                sizeof(fbe_raid_group_set_rb_checkpoint_t),
                                                in_rg_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "_set_rb_checkpoint: pkt_err:%d, pkt_qual:%d, pyld_err:%d, pyld_qual:%d\n", 
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) 
        {
            return status;
        }
        else
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
        
    return status;

} /* end of fbe_api_raid_group_set_rebuild_checkpoint() */

/*!***************************************************************
 * @fn fbe_api_raid_group_set_verify_checkpoint(fbe_object_id_t  in_rg_object_id, 
 *                                              fbe_raid_group_set_verify_checkpoint_t in_rg_set_checkpoint)
 ****************************************************************
 * @brief
 *  This function is used to set the verify checkpoint.
 *
 * @param in_rg_object_id           - RG object ID
 * @param in_rg_set_checkpoint      - checkpoints to set
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_raid_group_set_verify_checkpoint(fbe_object_id_t  in_rg_object_id, 
                                         fbe_raid_group_set_verify_checkpoint_t in_rg_set_checkpoint)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;


    status = fbe_api_common_send_control_packet(FBE_RAID_GROUP_CONTROL_CODE_SET_VERIFY_CHECKPOINT,
                                                &in_rg_set_checkpoint,
                                                sizeof(fbe_raid_group_set_verify_checkpoint_t),
                                                in_rg_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "_set_verify_checkpoint: pkt_err:%d, pkt_qual:%d, pyld_err:%d, pyld_qual:%d\n", 
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) 
        {
            return status;
        }
        else
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
        
    return status;

} /* end of fbe_api_raid_group_set_verify_checkpoint() */

/*!**************************************************************
 * fbe_api_raid_group_info_is_degraded()
 ****************************************************************
 * @brief
 *  Determine if rg is degraded using the rg info that was
 *  fetched from a raid group.
 *
 * @param rg_info_p               
 *
 * @return FBE_TRUE yes degraded FBE_FALSE otherwise
 *
 * @author
 *  3/27/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_api_raid_group_info_is_degraded(fbe_api_raid_group_get_info_t *rg_info_p)
{
    fbe_u32_t index;
    for (index = 0; index < rg_info_p->width; index++)
    {
        if ((rg_info_p->b_rb_logging[index] == FBE_TRUE) ||
            (rg_info_p->rebuild_checkpoint[index] != FBE_LBA_INVALID))
        {
            return FBE_TRUE;
        }
    }
    return FBE_FALSE;
}
/******************************************
 * end fbe_api_raid_group_info_is_degraded()
 ******************************************/

/*!**************************************************************
 * fbe_api_raid_group_get_verify_checkpoint()
 ****************************************************************
 * @brief
 *  Fetch the correct verify checkpoint from the input rg info.
 *
 * @param rg_info_p - Info we fetched from the rg.
 * @param verify_type - Type of verify chkpt to return.
 * @param chkpt_p - Chkpt to return.
 *
 * @return fbe_status_t
 *
 * @author
 *  4/6/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_raid_group_get_verify_checkpoint(fbe_api_raid_group_get_info_t *rg_info_p,
                                                      fbe_lun_verify_type_t verify_type,
                                                      fbe_lba_t *chkpt_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    *chkpt_p = 1; /* not 0 and not lba invalid */
    switch (verify_type)
    {
        case FBE_LUN_VERIFY_TYPE_USER_RO:
            *chkpt_p = rg_info_p->ro_verify_checkpoint;
            break;
        case FBE_LUN_VERIFY_TYPE_USER_RW:
            *chkpt_p = rg_info_p->rw_verify_checkpoint;
            break;
        case FBE_LUN_VERIFY_TYPE_ERROR:
            *chkpt_p = rg_info_p->error_verify_checkpoint;
            break;
        case FBE_LUN_VERIFY_TYPE_INCOMPLETE_WRITE:
            *chkpt_p = rg_info_p->incomplete_write_verify_checkpoint;
            break;
        case FBE_LUN_VERIFY_TYPE_SYSTEM:
            *chkpt_p = rg_info_p->system_verify_checkpoint;
            break;            
        default:
            fbe_api_trace(FBE_TRACE_LEVEL_INFO, "verify type %d unknown\n", verify_type);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    };
    return status;
}
/******************************************
 * end fbe_api_raid_group_get_verify_checkpoint()
 ******************************************/

/*!***************************************************************
 *  fbe_api_raid_group_map_lba()
 ****************************************************************
 * @brief
 *  This function maps an lba onto the raid group.
 *
 * @param rg_object_id - RG object id
 * @param rg_map_info_p - pointer to RG info
 * @param attribute - attribute
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  5/29/2012 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_raid_group_map_lba(fbe_object_id_t rg_object_id, 
                           fbe_raid_group_map_info_t * rg_map_info_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    if (rg_map_info_p == NULL) {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet(FBE_RAID_GROUP_CONTROL_CODE_MAP_LBA,
                                                rg_map_info_p,
                                                sizeof(fbe_raid_group_map_info_t),
                                                rg_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    return status;
}
/**************************************
 * end fbe_api_raid_group_map_lba()
 **************************************/

/*!***************************************************************
 *  fbe_api_raid_group_map_pba()
 ****************************************************************
 * @brief
 *  This function maps an lba onto the raid group.
 *
 * @param rg_object_id - RG object id
 * @param rg_map_info_p - pointer to RG info
 * @param attribute - attribute
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  11/1/2012 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_raid_group_map_pba(fbe_object_id_t rg_object_id, 
                           fbe_raid_group_map_info_t * rg_map_info_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    if (rg_map_info_p == NULL) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet(FBE_RAID_GROUP_CONTROL_CODE_MAP_PBA,
                                                rg_map_info_p,
                                                sizeof(fbe_raid_group_map_info_t),
                                                rg_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    return status;
}
/**************************************
 * end fbe_api_raid_group_map_pba()
 **************************************/

/*!**************************************************************
 * fbe_api_raid_group_max_get_paged_chunks()
 ****************************************************************
 * @brief
 *  Determine the max number of chunks we can get at a time.
 *
 * @param num_chunks_p - number of chunks returned.               
 *
 * @return fbe_status_t   
 *
 * @author
 *  5/31/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_raid_group_max_get_paged_chunks(fbe_u32_t *num_chunks_p)
{
    *num_chunks_p = FBE_PAYLOAD_METADATA_MAX_DATA_SIZE / sizeof(fbe_raid_group_paged_metadata_t);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_api_raid_group_max_get_paged_chunks()
 ******************************************/
/*!**************************************************************
 * fbe_api_raid_group_get_paged_metadata()
 ****************************************************************
 * @brief
 *  Get a set of paged metadata.
 *
 *  The chunk count should not exceed that returned by
 *  fbe_api_raid_group_max_get_paged_chunks()
 *
 * @param rg_object_id - object ID to get chunks from.
 * @param chunk_offset - Chunk number to start read.
 * @param chunk_count - Number of chunks to read and number of
 *                      chunks of memory in paged_md_p
 * @param paged_md_p - Ptr to array of chunk_count number of chunks.
 * @param b_force_unit_access - FBE_TRUE - Get the paged data from disk (not from cache)
 *                              FBE_FALSE - Allow the data to come from cache              
 *
 * @return fbe_status_t
 *
 * @author
 *  5/31/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_raid_group_get_paged_metadata(fbe_object_id_t rg_object_id,
                                                   fbe_chunk_index_t chunk_offset,
                                                   fbe_chunk_index_t chunk_count,
                                                   fbe_raid_group_paged_metadata_t *paged_md_p,
                                                   fbe_bool_t b_force_unit_access)
{
    fbe_api_raid_group_get_info_t rg_info;
    fbe_status_t  status = FBE_STATUS_OK;
    fbe_chunk_index_t total_chunks;
    fbe_u64_t metadata_offset = 0;
    fbe_u32_t max_chunks_per_request;
    fbe_api_base_config_metadata_paged_get_bits_t get_bits;
    fbe_u32_t wait_msecs = 0;

    /* Get the information about the capacity so we can calculate number of chunks.
     */
    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK) 
    { 
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: rg get info failed, obj 0x%x, status: 0x%x\n", 
                        __FUNCTION__, rg_object_id, status);
        return status; 
    }

    total_chunks = rg_info.total_chunks;

    if (chunk_offset > total_chunks)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: input chunk offset 0x%llx is greater than total chunks 0x%llx\n", 
                        __FUNCTION__, (unsigned long long)chunk_offset, (unsigned long long)total_chunks);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_api_raid_group_max_get_paged_chunks(&max_chunks_per_request);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: get max paged failed, obj 0x%x, status: 0x%x\n", 
                        __FUNCTION__, rg_object_id, status);
        return status; 
    }
    if (chunk_count > max_chunks_per_request)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: input chunk count 0x%llx > max allowed 0x%x\n", 
                        __FUNCTION__, (unsigned long long)chunk_count, max_chunks_per_request);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    metadata_offset = chunk_offset * sizeof(fbe_raid_group_paged_metadata_t);

    get_bits.metadata_offset = metadata_offset;
    get_bits.metadata_record_data_size = (fbe_u32_t)(chunk_count * sizeof(fbe_raid_group_paged_metadata_t));

    /* If the data must come from disk flag that fact.
     */
    if (b_force_unit_access == FBE_TRUE)
    {
        get_bits.get_bits_flags = FBE_API_BASE_CONFIG_METADATA_PAGED_GET_BITS_FLAGS_FUA_READ;  
    }
    else
    {
        get_bits.get_bits_flags = 0;
    }

    /* Fetch the bits for this set of chunks.  It is possible to get errors back
     * if the object is quiescing, so just retry briefly. 
     */
    status = fbe_api_base_config_metadata_paged_get_bits(rg_object_id, &get_bits);
    while (status != FBE_STATUS_OK)
    {
        if (wait_msecs > 30000)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: get bits failed, obj 0x%x offset 0x%llx status: 0x%x\n", 
                          __FUNCTION__, rg_object_id, (unsigned long long)metadata_offset, status);
            return FBE_STATUS_TIMEOUT;
        }
        fbe_api_sleep(500);
        wait_msecs += 500;

        status = fbe_api_base_config_metadata_paged_get_bits(rg_object_id, &get_bits);
    }
    if (status == FBE_STATUS_OK)
    {
        fbe_u32_t chunk_index;
        fbe_raid_group_paged_metadata_t *paged_p = (fbe_raid_group_paged_metadata_t *)&get_bits.metadata_record_data[0];

        for (chunk_index = 0; chunk_index < chunk_count; chunk_index++)
        {
            paged_md_p[chunk_index] = paged_p[chunk_index];
        }
    }
    return status;
}
/******************************************
 * end fbe_api_raid_group_get_paged_metadata()
 ******************************************/
/*!**************************************************************
 * fbe_api_raid_group_set_paged_metadata()
 ****************************************************************
 * @brief
 *  Set of paged metadata.
 *
 *  The chunk count should not exceed that returned by
 *  fbe_api_raid_group_max_get_paged_chunks()
 *
 * @param rg_object_id - object ID to get chunks from.
 * @param chunk_offset - Chunk number to start read.
 * @param chunk_count - Number of chunks to read and number of
 *                      chunks of memory in paged_md_p
 * @param paged_md_p - Ptr to array of chunk_count number of chunks.           
 *
 * @return fbe_status_t
 *
 * @author
 *  03/12/2013  Ron Proulx  - Created.
 *
 ****************************************************************/

fbe_status_t fbe_api_raid_group_set_paged_metadata(fbe_object_id_t rg_object_id,
                                                   fbe_chunk_index_t chunk_offset,
                                                   fbe_chunk_index_t chunk_count,
                                                   fbe_raid_group_paged_metadata_t *paged_md_p)
{
    fbe_api_raid_group_get_info_t rg_info;
    fbe_status_t  status = FBE_STATUS_OK;
    fbe_chunk_index_t total_chunks;
    fbe_u64_t metadata_offset = 0;
    fbe_u32_t metadata_record_data_size = 0;
    fbe_u32_t max_chunks_per_request;
    fbe_api_base_config_metadata_paged_change_bits_t set_bits;
    fbe_u32_t wait_msecs = 0;

    /* Get the information about the capacity so we can calculate number of chunks.
     */
    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK) 
    { 
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: rg get info failed, obj 0x%x, status: 0x%x\n", 
                        __FUNCTION__, rg_object_id, status);
        return status; 
    }

    total_chunks = rg_info.total_chunks;

    if (chunk_offset > total_chunks)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: input chunk offset 0x%llx is greater than total chunks 0x%llx\n", 
                        __FUNCTION__, (unsigned long long)chunk_offset, (unsigned long long)total_chunks);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_api_raid_group_max_get_paged_chunks(&max_chunks_per_request);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: get max paged failed, obj 0x%x, status: 0x%x\n", 
                        __FUNCTION__, rg_object_id, status);
        return status; 
    }
    if (chunk_count > max_chunks_per_request)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: input chunk count 0x%llx > max allowed 0x%x\n", 
                        __FUNCTION__, (unsigned long long)chunk_count, max_chunks_per_request);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    metadata_offset = chunk_offset * sizeof(fbe_raid_group_paged_metadata_t);

    fbe_zero_memory(&set_bits, sizeof(fbe_api_base_config_metadata_paged_change_bits_t));
    set_bits.metadata_offset = metadata_offset;
    metadata_record_data_size = sizeof(fbe_raid_group_paged_metadata_t) * (fbe_u32_t)chunk_count;
    fbe_copy_memory(&set_bits.metadata_record_data[0], paged_md_p, metadata_record_data_size);
    set_bits.metadata_record_data_size = metadata_record_data_size;
    set_bits.metadata_repeat_count = 1;

    /* Set the bits for this set of chunks.  It is possible to get errors back
     * if the object is quiescing, so just retry briefly. 
     */
    status = fbe_api_base_config_metadata_paged_set_bits(rg_object_id, &set_bits);
    while (status != FBE_STATUS_OK)
    {
        if (wait_msecs > 30000)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: set bits failed, obj 0x%x offset 0x%llx status: 0x%x\n", 
                          __FUNCTION__, rg_object_id, (unsigned long long)metadata_offset, status);
            return FBE_STATUS_TIMEOUT;
        }
        fbe_api_sleep(500);
        wait_msecs += 500;

        status = fbe_api_base_config_metadata_paged_set_bits(rg_object_id, &set_bits);
    }
    return status;
}
/*********************************************
 * end fbe_api_raid_group_set_paged_metadata()
 *********************************************/
/*!**************************************************************
 * fbe_api_raid_group_force_mark_nr()
 ****************************************************************
 * @brief
 *  This function marks the entire drive on the specified RG for NR. 
 * This API needs to be used very carefully only for engineering/debug 
 * and testing purposes only
 *
 * @param rg_object_id - RG Object ID that needs to be marked.
 * @param port - Port Number
 * @param encl - Enclosure Number
 * @param slot - Slot number
 *
 * @return fbe_status_t
 *
 * @notes: ENGINEERING/DEBUG PURPOSES ONLY 
 * 
 * @author
 *  06/15/2012 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_raid_group_force_mark_nr(fbe_object_id_t rg_object_id,
                                                           fbe_u32_t port,
                                                           fbe_u32_t encl,
                                                           fbe_u32_t slot)
{
    fbe_status_t status;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_object_id_t pvd_object_id;
    fbe_api_base_config_upstream_object_list_t upstream_object_list;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_base_config_drive_to_mark_nr_t drive_to_mark_nr;

    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK) 
    { 
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: rg get info failed, obj 0x%x, status: 0x%x\n", 
                        __FUNCTION__, rg_object_id, status);
        return status; 
    }

    if(rg_info.raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: Need the object ID for the Mirror objects \n", 
                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_provision_drive_get_obj_id_by_location(port,encl,slot, &pvd_object_id);

    if (status != FBE_STATUS_OK) 
    { 
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: pvd get location failed for port:%d, encl:%d, slot:%d, status: 0x%x\n", 
                        __FUNCTION__, port, encl, slot, status);
        return status; 
    }

    status = fbe_api_base_config_get_upstream_object_list(pvd_object_id,
                                                          &upstream_object_list);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: unable to get upstream for PVD:0x%x status: 0x%x\n", 
                        __FUNCTION__, pvd_object_id, status);
        return status; 
    }

    if(upstream_object_list.number_of_upstream_objects == 0)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: No upstream objects for PVD:0x%x \n", 
                        __FUNCTION__, (unsigned int)pvd_object_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    drive_to_mark_nr.force_nr = FBE_TRUE;
    drive_to_mark_nr.vd_object_id = upstream_object_list.upstream_object_list[0];
    drive_to_mark_nr.pvd_object_id = pvd_object_id;
    status = fbe_api_common_send_control_packet(FBE_RAID_GROUP_CONTROL_CODE_MARK_NR,
                                                &drive_to_mark_nr,
                                                sizeof(fbe_base_config_drive_to_mark_nr_t),
                                                rg_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__, status, status_info.packet_qualifier, 
                       status_info.control_operation_status, 
                       status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(!drive_to_mark_nr.nr_status)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:Failed to mark NR for RG:0x%x \n", 
                       __FUNCTION__, rg_object_id);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/******************************************
 * end fbe_api_raid_group_force_mark_nr()
 ******************************************/

/*!***************************************************************
 *  @fn fbe_api_raid_group_set_mirror_prefered_position
 ****************************************************************
 * @brief
 *  This function sets mirror prefered position for RG
 *
 * @param rg_object_id - RG object id
 * @param prefered_position - prefered_position
 *
 * @return
 *  fbe_status_t - status
 *
 * @version
 *  6/22/2011 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_raid_group_set_mirror_prefered_position(fbe_object_id_t rg_object_id, 
                                                                          fbe_u32_t prefered_position)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_u32_t                                       position = prefered_position;

    status = fbe_api_common_send_control_packet (FBE_RAID_GROUP_CONTROL_CODE_SET_PREFERED_POSITION,
                                                 &position,
                                                 sizeof(fbe_u32_t),
                                                 rg_object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/******************************************
 * end fbe_api_raid_group_set_mirror_prefered_position()
 ******************************************/

/*!***************************************************************
 * @fn fbe_api_raid_group_set_background_operation_speed(
 *          fbe_provision_drive_background_op_t bg_op, 
 *          fbe_u32_t bg_op_speed)
 ****************************************************************
 * @brief
 *  This API sends request to raid group to set background operation speed.
 *  It is only used for engineering purpose.
 *
 * @param bg_op       - background operation type
 * @param bg_op_speed - background operation speed
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  08/06/2012 - Created. Amit Dhaduk
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_raid_group_set_background_operation_speed(fbe_raid_group_background_op_t bg_op, fbe_u32_t bg_op_speed)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_raid_group_control_set_bg_op_speed_t   set_bg_op;

    set_bg_op.background_op =  bg_op;
    set_bg_op.background_op_speed = bg_op_speed;

    status = fbe_api_common_send_control_packet_to_class (FBE_RAID_GROUP_CONTROL_CODE_SET_BG_OP_SPEED ,
                                                            &set_bg_op,
                                                            sizeof(fbe_raid_group_control_set_bg_op_speed_t),
                                                            /* There is no rg class
                                                             * instances so we need to send 
                                                             * it to one of the leaf 
                                                             * classes. 
                                                             */
                                                             FBE_CLASS_ID_PARITY,                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);


    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status == FBE_STATUS_OK) 
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }


    return status;
} 
/***************************************************************
 * end fbe_api_provision_drive_set_background_operation_speed
 ***************************************************************/

/*!***************************************************************
 * @fn fbe_api_raid_group_get_background_operation_speed(
 *          fbe_raid_group_control_get_bg_op_speed_t *pvd_bg_op)
 ****************************************************************
 * @brief
 *  This API sends request to PVD to get background operation speed.
 *  It is only used for engineering purpose.
 *
 * @param *pvd_bg_op - pointer to get bg op speed
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  08/06/2012 - Created. Amit Dhaduk
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_raid_group_get_background_operation_speed(fbe_raid_group_control_get_bg_op_speed_t *rg_bg_op)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_raid_group_control_get_bg_op_speed_t   get_bg_op;

    status = fbe_api_common_send_control_packet_to_class (FBE_RAID_GROUP_CONTROL_CODE_GET_BG_OP_SPEED ,
                                                            &get_bg_op,
                                                            sizeof(fbe_raid_group_control_get_bg_op_speed_t),
                                                            /* There is no rg class
                                                             * instances so we need to send 
                                                             * it to one of the leaf 
                                                             * classes. 
                                                             */
                                                             FBE_CLASS_ID_PARITY,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);


    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status == FBE_STATUS_OK) 
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    rg_bg_op->rebuild_speed= get_bg_op.rebuild_speed;
    rg_bg_op->verify_speed= get_bg_op.verify_speed;

    return status;
} 
/***************************************************************
 * end fbe_api_raid_group_get_background_operation_speed
 ***************************************************************/

/*!***************************************************************
 * @fn ffbe_api_raid_group_set_update_peer_checkpoint_interval(
 *          fbe_u32_t update_peer_checkpoint_interval)
 ****************************************************************
 * @brief
 *  This API sets the period between updates to the peer for checkpoint(s)
 *  update.
 *
 * @param   update_peer_checkpoint_interval - Interval (in seconds)
 *          to update peer checkpoint(s)
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 * @note    There is currently no `get' function since this API
 *          is for test purposes only.
 *
 * @version
 *  03/22/2013  Ron Proulx  - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_raid_group_set_update_peer_checkpoint_interval(fbe_u32_t update_peer_checkpoint_interval)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_raid_group_class_set_update_peer_checkpoint_interval_t update_peer_interval;

    fbe_zero_memory(&update_peer_interval, sizeof(fbe_raid_group_class_set_update_peer_checkpoint_interval_t));
    update_peer_interval.peer_update_seconds = update_peer_checkpoint_interval;
    status = fbe_api_common_send_control_packet_to_class(FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_UPDATE_PEER_CHECKPOINT_INTERVAL,
                                                         &update_peer_interval,
                                                         sizeof(fbe_raid_group_class_set_update_peer_checkpoint_interval_t),
                                                         /* There is no rg class
                                                             * instances so we need to send 
                                                             * it to one of the leaf 
                                                             * classes. 
                                                             */
                                                         FBE_CLASS_ID_PARITY,
                                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                                         &status_info,
                                                         FBE_PACKAGE_ID_SEP_0);


    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status == FBE_STATUS_OK) 
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
} 
/***************************************************************
 * end fbe_api_raid_group_set_update_peer_checkpoint_interval()
 ***************************************************************/

/*!***************************************************************
 *  fbe_api_raid_group_get_stats()
 ****************************************************************
 * @brief
 *  This function fetches basic stats on the raid group.
 *
 * @param rg_object_id - RG object id
 * @param stats_p - pointer to RG stats to return.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  4/26/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_raid_group_get_stats(fbe_object_id_t rg_object_id, 
                            fbe_raid_group_get_statistics_t * stats_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    if (stats_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(stats_p, sizeof(fbe_raid_group_get_statistics_t));

    status = fbe_api_common_send_control_packet (FBE_RAID_GROUP_CONTROL_CODE_GET_STATS,
                                                 stats_p,
                                                 sizeof(fbe_raid_group_get_statistics_t),
                                                 rg_object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        if (status == FBE_STATUS_OK)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        return status;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_raid_group_get_stats
 **************************************/

/*!***************************************************************
 *  fbe_api_raid_group_set_bts_params()
 ****************************************************************
 * @brief
 *  This function get info with the given raid group.
 *
 * @param params_p - pointer to RG info
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  4/26/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_raid_group_set_bts_params(fbe_raid_group_set_default_bts_params_t * params_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    if (params_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(&status_info, sizeof(fbe_api_control_operation_status_info_t));

    status = fbe_api_common_send_control_packet_to_class(FBE_RAID_GROUP_CONTROL_CODE_SET_DEFAULT_BTS_PARAMS,
                                                         params_p,
                                                         sizeof(fbe_raid_group_set_default_bts_params_t),
                                                         FBE_CLASS_ID_PARITY,
                                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                                         &status_info,
                                                         FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        if (status == FBE_STATUS_OK)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        return status;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_raid_group_set_bts_params
 **************************************/

/*!***************************************************************
 *  fbe_api_raid_group_get_bts_params()
 ****************************************************************
 * @brief
 *  This function get info with the given raid group.
 *
 * @param params_p - pointer to RG info
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  4/26/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_raid_group_get_bts_params(fbe_raid_group_get_default_bts_params_t * params_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    if (params_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(params_p, sizeof(fbe_raid_group_get_default_bts_params_t));

    status = fbe_api_common_send_control_packet_to_class(FBE_RAID_GROUP_CONTROL_CODE_GET_DEFAULT_BTS_PARAMS,
                                                         params_p,
                                                         sizeof(fbe_raid_group_get_default_bts_params_t),
                                                         FBE_CLASS_ID_PARITY,
                                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                                         &status_info,
                                                         FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        if (status == FBE_STATUS_OK)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        return status;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_raid_group_get_bts_params
 **************************************/


/*!***************************************************************
 *  @fn fbe_api_set_raid_group_expand(fbe_object_id_t rg_object_id, fbe_lba_t new_capacity)
 ****************************************************************
 * @brief
 *  This function initiate the RG expansion
 *
 * @param rg_object_id - RG object id to exppand
 * @param new_capacity - New capacity.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  10/04/2013 - Created. Shay Harel
 *
 ****************************************************************/
/* FIXME - the caller should not have to pass in the edge capacity here; it's implied by the RG capacity */
fbe_status_t FBE_API_CALL fbe_api_raid_group_expand(fbe_object_id_t rg_object_id, fbe_lba_t new_capacity, fbe_lba_t new_edge_capacity)
{
	fbe_class_id_t									class_id;
	fbe_status_t									status;
	fbe_api_job_service_change_rg_info_t			change_info;
	fbe_job_service_error_type_t					error_type;
	fbe_status_t									job_status;
	fbe_u64_t										job_number;

    /*verify the user is not trying to to change a non RAID object, this one has a seperate function because it needs to persist*/
	status = fbe_api_get_object_class_id(rg_object_id, &class_id, FBE_PACKAGE_ID_SEP_0);
	if (status != FBE_STATUS_OK) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s, Failed to get object class id\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	if (class_id <= FBE_CLASS_ID_RAID_FIRST || class_id >= FBE_CLASS_ID_RAID_LAST) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s, The object id is not RG\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}

    change_info.new_size = new_capacity;
    change_info.new_edge_size = new_edge_capacity;
	change_info.update_type = FBE_UPDATE_RAID_TYPE_EXPAND_RG;
	change_info.object_id = rg_object_id;

	/*issue command to RAID*/
    status = fbe_api_job_service_change_rg_info(&change_info, &job_number);
	if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s, Failed to start update job\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	/*Wait for the job to complete*/
    status = fbe_api_common_wait_for_job(job_number, FBE_API_RG_WAIT_EXPANSION, &error_type, &job_status, NULL);
	if (status == FBE_STATUS_GENERIC_FAILURE) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed waiting for job\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
	}

	if (status == FBE_STATUS_GENERIC_FAILURE || status == FBE_STATUS_TIMEOUT) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed waiting for job\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
	}

	if (job_status != FBE_STATUS_OK) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s:job failed with error code:%d\n", __FUNCTION__, error_type);
	}

    return job_status;
}
/**************************************
 * end fbe_api_raid_group_expand()
 **************************************/
/*!***************************************************************
 *  fbe_api_raid_group_set_encryption_state()
 ****************************************************************
 * @brief
 *  Allows a client to modify the encryption state of the RG.
 *
 * @param rg_object_id - RG object id
 * @param new_state - New encryption state.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  10/30/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_raid_group_set_encryption_state(fbe_object_id_t rg_object_id, 
                                        fbe_raid_group_set_encryption_mode_t * set_enc_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    
    if (set_enc_p == NULL) {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(set_enc_p, sizeof(fbe_raid_group_set_encryption_mode_t));

    status = fbe_api_common_send_control_packet(FBE_RAID_GROUP_CONTROL_CODE_SET_ENCRYPTION_STATE,
                                                set_enc_p,
                                                sizeof(fbe_raid_group_set_encryption_mode_t),
                                                rg_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                      status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        if (status == FBE_STATUS_OK) {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        return status;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_raid_group_set_encryption_state
 **************************************/


/*!***************************************************************
 *  fbe_api_raid_group_get_lowest_drive_tier()
 ****************************************************************
 * @brief
 *  This function retrieves the lowest drive tier from the rg's nonpaged
 *
 * @param rg_object_id - RG object id
 * @param drive_tier_p - pointer to structure to report information
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  2/3/2014 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_raid_group_get_lowest_drive_tier(fbe_object_id_t rg_object_id, 
                                         fbe_get_lowest_drive_tier_t * drive_tier_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;


    if (drive_tier_p == NULL) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet(FBE_RAID_GROUP_CONTROL_CODE_GET_LOWEST_DRIVE_TIER,
                                                drive_tier_p,
                                                sizeof(fbe_get_lowest_drive_tier_t),
                                                rg_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

 
    return status;
}

/**************************************
 * end fbe_api_raid_group_get_lowest_drive_tier()
 **************************************/

/*!***************************************************************
 *  fbe_api_raid_group_set_raid_attributes()
 ****************************************************************
 * @brief
 *  This function sets the raid geometry attributes flags
 *
 * @param rg_object_id - RG object id
 * @param attributes - the raid geometry attributes flags to set
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  1/30/2014 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_raid_group_set_raid_attributes(fbe_object_id_t rg_object_id, 
                                       fbe_u32_t  attributes)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_raid_group_set_raid_attributes_t            raid_attributes;

    raid_attributes.raid_attribute_flags = attributes;
    status = fbe_api_common_send_control_packet(FBE_RAID_GROUP_CONTROL_CODE_SET_RAID_ATTRIBUTE,
                                                &raid_attributes,
                                                sizeof(fbe_raid_group_set_raid_attributes_t),
                                                rg_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    return status;
}
/**************************************
 * end fbe_api_raid_group_set_raid_attributes()
 **************************************/

/*!***************************************************************************
 * fbe_api_raid_group_class_get_extended_media_error_handling()  
 *****************************************************************************
 *
 * @brief   This function gets the EMEH (Extended Media Error Handling) for 
 *          the raid group class on the selected SP.
 *   (Thus this parameters affect all raid groups except specific exceptions).
 *
 * @param get_emeh_p - Address of EMEH structure to populate
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  03/11/2015  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_raid_group_class_get_extended_media_error_handling(fbe_raid_group_class_get_extended_media_error_handling_t *get_emeh_p)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_raid_group_class_get_extended_media_error_handling_t get_emeh;

    status = fbe_api_common_send_control_packet_to_class(FBE_RAID_GROUP_CONTROL_CODE_CLASS_GET_EXTENDED_MEDIA_ERROR_HANDLING,
                                                         &get_emeh,
                                                         sizeof(fbe_raid_group_class_get_extended_media_error_handling_t),
                                                         /* There is no rg class
                                                             * instances so we need to send 
                                                             * it to one of the leaf 
                                                             * classes. 
                                                             */
                                                         FBE_CLASS_ID_PARITY,
                                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                                         &status_info,
                                                         FBE_PACKAGE_ID_SEP_0);


    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        if (status == FBE_STATUS_OK) {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* Populate the request */
    *get_emeh_p = get_emeh;
    return status;
} 
/***************************************************************
 * end fbe_api_raid_group_class_get_extended_media_error_handling()
 ***************************************************************/

/*!***************************************************************************
 * fbe_api_raid_group_class_set_extended_media_error_handling()  
 ***************************************************************************** 
 * 
 * @brief This function sets the EMEH (Extended Media Error Handling) for the
 *        raid group class one the selected SP.
 *       (Thus this parameters affect all raid groups except specific exceptions).
 *
 * @param   set_mode - The new `mode' value: Disable EMEH or Increase
 *                        media error threshold or restore default values.
 * @param   b_change_threshold - If FBE_TRUE - Then change the media error
 *                        threshold (increase) with the value supplied. 
 * @param   percent_threshold_increase - The percent to increase the media
 *                        error threshold.
 * @param   b_persist - FBE_TRUE - If FBE_TRUE - Pesist the values after 
 *                        updating in memory.
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  03/11/2015  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_raid_group_class_set_extended_media_error_handling(fbe_raid_emeh_mode_t set_mode,
                                                     fbe_bool_t b_change_threshold,
                                                     fbe_u32_t percent_threshold_increase,
                                                     fbe_bool_t b_persist)

{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_raid_group_class_set_extended_media_error_handling_t set_emeh;

    /* Only certain modes are allowed.
     */
    switch(set_mode)
    {
        case FBE_RAID_EMEH_MODE_ENABLED_NORMAL:
        case FBE_RAID_EMEH_MODE_DISABLED:
            /* The above are supported mode for the set class operation.
             */
            break;

        case FBE_RAID_EMEH_MODE_THRESHOLDS_INCREASED:
        case FBE_RAID_EMEH_MODE_DEGRADED_HA:
        case FBE_RAID_EMEH_MODE_PACO_HA:
            /* The above are internally generated and therefore cannot be set.
             */
            // Fall thru
        case FBE_RAID_EMEH_MODE_INVALID:
        default:
            /* These mode are not allowed.
             */
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                           "%s: set_mode: %d not allowed.\n", 
                       __FUNCTION__, set_mode);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Populate the request.
     */
    set_emeh.set_mode = set_mode;
    set_emeh.b_persist = b_persist;
    set_emeh.b_change_threshold = b_change_threshold;
    if (b_change_threshold) {
        set_emeh.percent_threshold_increase = percent_threshold_increase;
    } 
    else 
    {
        set_emeh.percent_threshold_increase = 0;
    }
    status = fbe_api_common_send_control_packet_to_class(FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_EXTENDED_MEDIA_ERROR_HANDLING,
                                                         &set_emeh,
                                                         sizeof(fbe_raid_group_class_set_extended_media_error_handling_t),
                                                         /* There is no rg class
                                                             * instances so we need to send 
                                                             * it to one of the leaf 
                                                             * classes. 
                                                             */
                                                         FBE_CLASS_ID_PARITY,
                                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                                         &status_info,
                                                         FBE_PACKAGE_ID_SEP_0);


    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        if (status == FBE_STATUS_OK) 
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    return status;
} 
/***************************************************************
 * end fbe_api_raid_group_class_set_extended_media_error_handling()
 ***************************************************************/

/*!***************************************************************************
 * fbe_api_raid_group_get_extended_media_error_handling()  
 *****************************************************************************
 *
 * @brief   This function gets the EMEH (Extended Media Error Handling) for 
 *          the raid group specified.
 *
 * @param   rg_object_id - The raid group object id to get EMEH for
 * @param   get_emeh_p - Address of EMEH structure to populate
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  05/12/2015  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_raid_group_get_extended_media_error_handling(fbe_object_id_t rg_object_id,
                                                     fbe_raid_group_get_extended_media_error_handling_t *get_emeh_p)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_raid_group_get_extended_media_error_handling_t get_emeh;

    status = fbe_api_common_send_control_packet(FBE_RAID_GROUP_CONTROL_CODE_GET_EXTENDED_MEDIA_ERROR_HANDLING,
                                                &get_emeh,
                                                sizeof(fbe_raid_group_get_extended_media_error_handling_t),
                                                rg_object_id,
                                                FBE_PACKET_FLAG_TRAVERSE,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        if (status == FBE_STATUS_OK) 
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* Populate the request */
    *get_emeh_p = get_emeh;
    return status;
} 
/***************************************************************
 * end fbe_api_raid_group_class_get_extended_media_error_handling()
 ***************************************************************/

/*!***************************************************************************
 * fbe_api_raid_group_set_extended_media_error_handling()  
 ***************************************************************************** 
 * 
 * @brief This function sets the EMEH (Extended Media Error Handling) for the
 *        raid group specified by the passed object id.
 * 
 * @param   rg_object_id - The raid group object id to change EMEH for
 * @param   set_control - The new `control' value: Disable EMEH or Increase
 *                        media error threshold or restore default values.
 * @param   b_change_threshold - If FBE_TRUE - Then change the media error
 *                        threshold (increase) with the value supplied. 
 * @param   percent_threshold_increase - The percent to increase the media
 *                        error threshold.
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  05/12/2015  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_raid_group_set_extended_media_error_handling(fbe_object_id_t rg_object_id,
                                                     fbe_raid_emeh_command_t set_control,
                                                     fbe_bool_t b_change_threshold,
                                                     fbe_u32_t percent_threshold_increase)

{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_raid_group_set_extended_media_error_handling_t set_emeh;

    /* Not all commands are allowed thru the API.
     */
    switch (set_control)
    {
        case FBE_RAID_EMEH_COMMAND_RESTORE_NORMAL_MODE:
        case FBE_RAID_EMEH_COMMAND_INCREASE_THRESHOLDS:   
        case FBE_RAID_EMEH_COMMAND_DISABLE_EMEH:
        case FBE_RAID_EMEH_COMMAND_ENABLE_EMEH:
            break;

        case FBE_RAID_EMEH_COMMAND_INVALID:
        case FBE_RAID_EMEH_COMMAND_RAID_GROUP_DEGRADED:
        case FBE_RAID_EMEH_COMMAND_PACO_STARTED:
        default:
            /* These commands are not allowed.
             */
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                           "%s: set_control: %d not allowed.\n", 
                       __FUNCTION__, set_control);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Zero the request (which clears `b_is_paco).
     */
    fbe_zero_memory(&set_emeh, sizeof(fbe_raid_group_set_extended_media_error_handling_t));
    set_emeh.set_control = set_control;
    set_emeh.b_change_threshold = b_change_threshold;
    if (b_change_threshold) 
    {
        set_emeh.percent_threshold_increase = percent_threshold_increase;
    } 
    else 
    {
        set_emeh.percent_threshold_increase = 0;
    }
    status = fbe_api_common_send_control_packet(FBE_RAID_GROUP_CONTROL_CODE_SET_EXTENDED_MEDIA_ERROR_HANDLING,
                                                &set_emeh,
                                                sizeof(fbe_raid_group_set_extended_media_error_handling_t),
                                                rg_object_id,
                                                FBE_PACKET_FLAG_TRAVERSE,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        if (status == FBE_STATUS_OK) 
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    return status;
} 
/***************************************************************
 * end fbe_api_raid_group_set_extended_media_error_handling()
 ***************************************************************/

/*!***************************************************************************
 * fbe_api_raid_group_set_raid_group_debug_state()
 ***************************************************************************** 
 * 
 * @brief   This function is for internal use only and is used to change
 *          private raid group state values in the specified raid group object:
 *              o Local state mask
 *              o Clustered raid group flags
 *              o Set any raid group condition
 * 
 * @param   rg_object_id - The raid group object id to set debug state for
 * @param   set_local_state_mask - Set one or more of the local state flags
 * @param   set_clustered_flags - Set one or more of the clustered raid group
 *              flags
 * @param   set_raid_group_condition - Set a raid group condition (value should
 *              not include the class_id)
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 * @note    Internal use only
 *
 * @version
 *  08/05/2015  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_raid_group_set_raid_group_debug_state(fbe_object_id_t rg_object_id,
                                              fbe_u64_t set_local_state_mask,     
                                              fbe_u64_t set_clustered_flags,      
                                              fbe_u32_t set_raid_group_condition) 
{
    fbe_status_t                            status;
    fbe_api_control_operation_status_info_t status_info;
    fbe_raid_group_set_debug_state_t        set_debug_state;

    /* Populate the request.
     */
    fbe_zero_memory(&set_debug_state, sizeof(fbe_raid_group_set_debug_state_t));
    set_debug_state.local_state_mask = set_local_state_mask;
    set_debug_state.clustered_flags = set_clustered_flags;
    set_debug_state.raid_group_condition = set_raid_group_condition;

    /* Send the request.
     */
    status = fbe_api_common_send_control_packet(FBE_RAID_GROUP_CONTROL_CODE_SET_DEBUG_STATE,
                                                &set_debug_state,
                                                sizeof(fbe_raid_group_set_debug_state_t),
                                                rg_object_id,
                                                FBE_PACKET_FLAG_TRAVERSE,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        if (status == FBE_STATUS_OK) 
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    return status;
} 
/***************************************************************
 * end fbe_api_raid_group_set_raid_group_debug_state()
 ***************************************************************/

/*!***************************************************************
 *  fbe_api_raid_group_get_wear_level()
 ****************************************************************
 * @brief
 *  This function retrieves the wear level information from the raid group
 *
 * @param rg_object_id - RG object id
 * @param wear_level_p - pointer to structure to report information
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  7/1/2015 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_raid_group_get_wear_level(fbe_object_id_t rg_object_id, 
                                  fbe_raid_group_get_wear_level_t * wear_level_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    if (wear_level_p == NULL) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet(FBE_RAID_GROUP_CONTROL_CODE_GET_WEAR_LEVEL,
                                                wear_level_p,
                                                sizeof(fbe_raid_group_get_wear_level_t),
                                                rg_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

 
    return status;
}

/**************************************
 * end fbe_api_raid_group_get_wear_level()
 **************************************/

/*!****************************************************************************
 * fbe_api_raid_group_set_lifecycle_timer_interval()
 ******************************************************************************
 * @brief
 *   This function is used to set the lifecycle timer interval
 *   Once the timer is triggered and cleared, the interval would
 *   be set back to the default. 
 * 
 * @param rg_object_id - RG object id
 * @param timer_info_p - pointer to structure to set information
 *
 * @return  fbe_status_t  
 *
 * @author
 *   07/08/2015 - Created. Deanna Heng
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_raid_group_set_lifecycle_timer_interval(fbe_object_id_t rg_object_id, 
                                                                          fbe_lifecycle_timer_info_t * timer_info_p)
{
    fbe_api_control_operation_status_info_t status_info;
    fbe_status_t status;

    if (timer_info_p == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: NULL control pointer\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if ((timer_info_p->lifecycle_condition <= FBE_RAID_GROUP_SET_LIFECYCLE_TIMER_COND_INVALID)||
        (timer_info_p->lifecycle_condition >= FBE_RAID_GROUP_SET_LIFECYCLE_TIMER_COND_LAST)) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: invalid raid group condition: 0x%X first: 0x%x last: 0x%x\n",
                      __FUNCTION__, timer_info_p->lifecycle_condition,
                      FBE_RAID_GROUP_SET_LIFECYCLE_TIMER_COND_INVALID,
                      FBE_RAID_GROUP_SET_LIFECYCLE_TIMER_COND_LAST); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet(FBE_RAID_GROUP_CONTROL_CODE_SET_LIFECYCLE_TIMER,
                                                timer_info_p,
                                                sizeof(fbe_lifecycle_timer_info_t),
                                                rg_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}
/***************************************************************
 * end fbe_api_raid_group_set_lifecycle_timer_interval()
 ***************************************************************/

/*!***************************************************************
 *  fbe_api_raid_group_set_chunks_per_rebuild()
 ****************************************************************
 * @brief
 *  This function sets the number of chunks to rebuild per rebuild cycle.
 *  There is no get function since this is for test purposes only
 *
 * @param chunks_per_rebuild - number of chunks per rebuild io
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  3/13/2015 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_raid_group_set_chunks_per_rebuild(fbe_u32_t num_chunks_per_rebuild)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_raid_group_class_set_chunks_per_rebuild_t   chunks_per_rebuild;

    fbe_zero_memory(&chunks_per_rebuild, sizeof(fbe_raid_group_class_set_chunks_per_rebuild_t));
    chunks_per_rebuild.chunks_per_rebuild = num_chunks_per_rebuild;
    status = fbe_api_common_send_control_packet_to_class(FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_CHUNKS_PER_REBUILD,
                                                         &chunks_per_rebuild,
                                                         sizeof(fbe_raid_group_class_set_chunks_per_rebuild_t),
                                                         /* There is no rg class
                                                             * instances so we need to send 
                                                             * it to one of the leaf 
                                                             * classes. 
                                                             */
                                                         FBE_CLASS_ID_PARITY,
                                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                                         &status_info,
                                                         FBE_PACKAGE_ID_SEP_0);


    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status == FBE_STATUS_OK) 
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }


    return status;

}
/**************************************
 * end fbe_api_raid_group_set_chunks_per_rebuild()
 **************************************/

/******************************************
 * end file fbe_api_raid_group_interface.c
 ******************************************/
