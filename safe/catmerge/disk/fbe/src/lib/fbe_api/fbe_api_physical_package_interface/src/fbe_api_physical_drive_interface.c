/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_physical_drive_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Port related APIs.
 *      
 * @ingroup fbe_api_physical_package_interface_class_files
 * @ingroup fbe_api_physical_drive_interface
 * 
 * @version
 *      10/14/08    sharel - Created
 *    
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_payload_block_operation.h"
#include "fbe_discovery_transport.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_block_transport.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe/fbe_api_perfstats_interface.h"

/**************************************
                Local variables
**************************************/

/*******************************************
                Local functions
********************************************/
static fbe_status_t fbe_api_physical_drive_get_drive_information_asynch_callback (fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_api_physical_drive_fw_download_start_async_callback (fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_api_physical_drive_fw_download_async_callback (fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_api_physical_drive_fw_download_stop_async_callback (fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_api_physical_drive_get_fuel_gauge_asynch_callback (fbe_packet_t * packet, fbe_packet_completion_context_t context);
static void         fbe_api_physical_drive_send_pass_thru_completion(void * context);
static fbe_status_t fbe_api_physical_drive_send_pass_thru_asynch_callback (fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_api_physical_drive_drivegetlog_asynch_callback (fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_api_physical_drive_send_download_async_callback (fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_api_physical_drive_interface_generic_completion(fbe_packet_t * packet, 
                                                                        fbe_packet_completion_context_t context);
static fbe_status_t fbe_api_physical_drive_get_bms_op_asynch_callback (fbe_packet_t * packet, fbe_packet_completion_context_t context);


/*!***************************************************************
 * @fn fbe_api_physical_drive_reset(
 *    fbe_object_id_t object_id, 
 *    fbe_packet_attr_t attribute)
 *****************************************************************
 * @brief
 *  This function sends a command tro reset a drive.
 *
 * @param object_id  - The logical drive object id to send request to
 * @param attribute  - attribute
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *    10/14/08: sharel  created
 *    
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_reset(fbe_object_id_t object_id, 
                                                       fbe_packet_attr_t attribute)
{

    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t     status_info;
    
    
     status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_RESET_DRIVE,
                                                  NULL,
                                                  0,
                                                  object_id,
                                                  attribute,
                                                  &status_info,
                                                  FBE_PACKAGE_ID_PHYSICAL);

     if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }


    return status;

}

/*!***************************************************************
 * @fn fbe_api_physical_drive_mode_select(
 *    fbe_object_id_t object_id)
 *****************************************************************
 * @brief
 *  This function sends command to do mode sense then select.
 *
 * @param object_id  - The logical drive object id to send request to
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *    04/01/13: Wayne Garrett - created
 *    
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_mode_select(fbe_object_id_t object_id)
{

    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t     status_info;
    
    
     status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_MODE_SELECT,
                                                  NULL,
                                                  0,
                                                  object_id,
                                                  FBE_PACKET_FLAG_NO_ATTRIB,
                                                  &status_info,
                                                  FBE_PACKAGE_ID_PHYSICAL);

     if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:pkt err:%d, pkt qual:%d, pyld err:%d, pyld qual:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}


/*!***************************************************************
 * @fn fbe_api_physical_drive_get_dieh_info(
 *    fbe_object_id_t object_id, 
 *    fbe_physical_drive_dieh_stats_t * dieh_stats, 
 *    fbe_packet_attr_t attribute)
 *****************************************************************
 * @brief
 *  This function gets the DIEH Info for a given object
 *
 * @param object_id  - The logical drive object id to send request to
 * @param dieh_stats - pointer to the dieh_stats
 * @param attribute - attribute
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  09/05/08: Ashok Rana - Created
 *  12/15/11: Wayne Garrett - Changed function to get all DIEH Info
 *
 ****************************************************************/
 fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_dieh_info(fbe_object_id_t  object_id, 
                                                   fbe_physical_drive_dieh_stats_t * dieh_stats, 
                                                   fbe_packet_attr_t attribute)


{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    if (dieh_stats == NULL){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: input buffer is NULL\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DIEH_INFO,
                                                 dieh_stats,
                                                 sizeof(fbe_physical_drive_dieh_stats_t),
                                                 object_id,
                                                 attribute,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);


    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}


/*!***************************************************************
 * @fn fbe_api_physical_drive_get_error_counts(
 *    fbe_object_id_t object_id, 
 *    fbe_physical_drive_mgmt_get_error_counts_t * error_counts, 
 *    fbe_packet_attr_t attribute)
 *****************************************************************
 * @brief
 *  This function gets the error counts for given object.
 *
 * @param object_id  - The logical drive object id to send request to
 * @param error_counts - pointer to the error counts
 * @param attribute - attribute
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  09/05/08: Ashok Rana  Created
 *
 ****************************************************************/
 fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_error_counts(fbe_object_id_t  object_id, 
                                                     fbe_physical_drive_mgmt_get_error_counts_t * error_counts, 
                                                     fbe_packet_attr_t attribute)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_physical_drive_dieh_stats_t                 *dieh_stats = NULL;

    if (error_counts == NULL)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: input buffer is NULL\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    dieh_stats = (fbe_physical_drive_dieh_stats_t *)fbe_api_allocate_memory(sizeof(fbe_physical_drive_dieh_stats_t));
    if (dieh_stats == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:unable to allocate memory\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DIEH_INFO,
                                                 dieh_stats,
                                                 sizeof(fbe_physical_drive_dieh_stats_t),
                                                 object_id,
                                                 attribute,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        fbe_api_free_memory(dieh_stats);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(error_counts, &dieh_stats->error_counts, sizeof(fbe_physical_drive_mgmt_get_error_counts_t));

    fbe_api_free_memory(dieh_stats);

    return status;
}

/*!***************************************************************
 * @fn fbe_api_physical_drive_clear_error_counts(
 *    fbe_object_id_t object_id, 
 *    fbe_packet_attr_t attribute)
 *****************************************************************
 * @brief
 *  This function clears the error counts for given object.
 *
 * @param object_id  - The logical drive object id to send request to
 * @param attribute - attribute
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  09/05/08: Ashok Rana  Created
 *
 ****************************************************************/
 fbe_status_t FBE_API_CALL fbe_api_physical_drive_clear_error_counts(fbe_object_id_t  object_id, fbe_packet_attr_t attribute)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t     status_info;

    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_CLEAR_ERROR_COUNTS,
                                                 NULL,
                                                 0,
                                                 object_id,
                                                 attribute,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        if (status == FBE_STATUS_OK)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}


/*!***************************************************************
 * @fn fbe_api_physical_drive_set_object_queue_depth(
 *    fbe_object_id_t object_id,
 *    fbe_u32_t depth,
 *    fbe_packet_attr_t attribute)
 *****************************************************************
 * @brief
 *  This function sets the queue depth of a specific object.
 *
 * @param object_id  - The drive to set the depth
 * @param depth - how deep to set
 * @param attribute - attribute
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  12/08/08: sharel - created
 *
 ****************************************************************/
 fbe_status_t FBE_API_CALL fbe_api_physical_drive_set_object_queue_depth(fbe_object_id_t  object_id, fbe_u32_t depth, fbe_packet_attr_t attribute)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_physical_drive_mgmt_qdepth_t            set_depth;

    set_depth.qdepth = depth;

    /*pay attention we would set teh FBE_PACKET_FLAG_TRAVERSE because all times the ioctls are actually ment to the logical drive*/
    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_QDEPTH,
                                                 &set_depth,
                                                 sizeof(fbe_physical_drive_mgmt_qdepth_t),
                                                 object_id,
                                                 attribute,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_physical_drive_get_object_queue_depth(
 *    fbe_object_id_t object_id,
 *    fbe_u32_t* depth,
 *    fbe_packet_attr_t attribute)
 *****************************************************************
 * @brief
 *  This function gets the queue depth of a specific drive object.
 *
 * @param object_id  - The drive to set the depth
 * @param depth - how deep to set
 * @param attribute - attribute
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  12/08/08: sharel - created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_object_queue_depth(fbe_object_id_t  object_id, fbe_u32_t *depth, fbe_packet_attr_t attribute)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_physical_drive_mgmt_qdepth_t                get_depth;

    if (depth == NULL || object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_QDEPTH,
                                                 &get_depth,
                                                 sizeof(fbe_physical_drive_mgmt_qdepth_t),
                                                 object_id,
                                                 attribute,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *depth = get_depth.qdepth;

    return status;

}

/*!***************************************************************
 * @fn fbe_api_physical_drive_get_drive_queue_depth(
 *    fbe_object_id_t object_id,
 *    fbe_u32_t* depth,
 *    fbe_packet_attr_t attribute)
 *****************************************************************
 * @brief
 *  This function gets the queue depth of the apcual drive for a specific drive object.
 *
 * @param object_id  - The drive to set the depth
 * @param depth - how deep to set
 * @param attribute - attribute
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  12/08/08: sharel - created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_drive_queue_depth(fbe_object_id_t  object_id, fbe_u32_t *depth, fbe_packet_attr_t attribute)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_physical_drive_mgmt_qdepth_t                get_depth;

    if (depth == NULL || object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DRIVE_QDEPTH,
                                                 &get_depth,
                                                 sizeof(fbe_physical_drive_mgmt_qdepth_t),
                                                 object_id,
                                                 attribute,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *depth = get_depth.qdepth;

    return status;

}

/*!***************************************************************
 * @fn fbe_api_physical_drive_get_drive_information(
 *    fbe_object_id_t  object_id, 
 *    fbe_physical_drive_information_t *get_drive_information, 
 *    fbe_packet_attr_t attribute) 
 *****************************************************************
 * @brief
 *  This function gets the drive information.
 *
 * @param object_id - the drive to set the depth
 * @param get_drive_information  - return value in pointer
 * @param attribute - send deep first or not
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *    12/12/08: CLC - created
 *    
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_drive_information(fbe_object_id_t  object_id, 
                                                                       fbe_physical_drive_information_t *get_drive_information, 
                                                                       fbe_packet_attr_t attribute)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_physical_drive_mgmt_get_drive_information_t *drive_mgmt_get_drive_information;

    if (get_drive_information == NULL || object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    drive_mgmt_get_drive_information = (fbe_physical_drive_mgmt_get_drive_information_t*)fbe_api_allocate_memory (sizeof(fbe_physical_drive_mgmt_get_drive_information_t));


    if (drive_mgmt_get_drive_information == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:unable to allocate memory\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   

    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DRIVE_INFORMATION,
                                                 drive_mgmt_get_drive_information,
                                                 sizeof(fbe_physical_drive_mgmt_get_drive_information_t),
                                                 object_id,
                                                 attribute,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        fbe_api_free_memory (drive_mgmt_get_drive_information);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(get_drive_information, &drive_mgmt_get_drive_information->get_drive_info, sizeof(fbe_physical_drive_information_t));
    
    fbe_api_free_memory (drive_mgmt_get_drive_information);

    return status;
}

/*!***************************************************************
 * @fn fbe_api_physical_drive_is_wr_cache_enabled(
 *    fbe_object_id_t  object_id, 
 *    fbe_bool_t *enabled, 
 *    fbe_packet_attr_t attribute)
 *****************************************************************
 * @brief
 *  This function checks if the write cache of this object is enabled.
 *
 * @param object_id - the drive to check if cache enabled
 * @param enabled - pointer to return info
 * @param attribute - send deep first or not
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  12/08/08: sharel - created   
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_is_wr_cache_enabled(fbe_object_id_t  object_id, 
                                                                     fbe_bool_t *enabled, 
                                                                     fbe_packet_attr_t attribute)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
   fbe_api_control_operation_status_info_t          status_info;
    fbe_physical_drive_mgmt_rd_wr_cache_value_t     get_cache_value;

    if (enabled == NULL || object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_IS_WR_CACHE_ENABLED,
                                                 &get_cache_value,
                                                 sizeof(fbe_physical_drive_mgmt_rd_wr_cache_value_t),
                                                 object_id,
                                                 attribute,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    *enabled = get_cache_value.fbe_physical_drive_mgmt_rd_wr_cache_value;

    return status;

}

/*!***************************************************************
 * @fn fbe_api_physical_drive_get_default_io_timeout(
 *    fbe_object_id_t  object_id, 
 *    fbe_time_t *timeout,
 *    fbe_packet_attr_t attribute)
 *****************************************************************
 * @brief
 *  This function gets the default timeout the drive would use for IO.
 *
 * @param object_id  -the drive to check if cache enabled
 * @param timeout - pointer to return info
 * @param attribute - send deep first or not
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  03/12/08: sharel - created   
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_default_io_timeout(fbe_object_id_t  object_id, 
                                                                        fbe_time_t *timeout, 
                                                                        fbe_packet_attr_t attribute)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_physical_drive_mgmt_default_timeout_t       get_timeout;
    fbe_api_control_operation_status_info_t         status_info;

    if (timeout == NULL || object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DEFAULT_IO_TIMEOUT,
                                                 &get_timeout,
                                                 sizeof(fbe_physical_drive_mgmt_default_timeout_t),
                                                 object_id,
                                                 attribute,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    *timeout = get_timeout.timeout;

    return status;

}

/*!***************************************************************
 * @fn fbe_api_physical_drive_set_default_io_timeout(
 *    fbe_object_id_t  object_id, 
 *    fbe_time_t timeout, 
 *    fbe_packet_attr_t attribute)
 *****************************************************************
 * @brief
 *  This function set the default timeout the drive would use for IO.
 *
 * @param object_id - the drive to check if cache enabled
 * @param timeout  - value to set in msec
 * @param attribute - send deep first or not
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  03/12/08: sharel - created  
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_set_default_io_timeout(fbe_object_id_t  object_id, 
                                                                        fbe_time_t timeout, 
                                                                        fbe_packet_attr_t attribute)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_physical_drive_mgmt_default_timeout_t       set_timeout;
    fbe_api_control_operation_status_info_t         status_info;

    if (timeout == 0|| object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    set_timeout.timeout = timeout;
    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_DEFAULT_IO_TIMEOUT,
                                                 &set_timeout,
                                                 sizeof(fbe_physical_drive_mgmt_default_timeout_t),
                                                 object_id,
                                                 attribute,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_get_physical_drive_object_id_by_location(
 *    fbe_u32_t port, 
 *    fbe_u32_t enclosure, 
 *    fbe_u32_t slot, 
 *    fbe_object_id_t *object_id)
 *****************************************************************
 * @brief
 *  This function returns the object id for a logical drive location in the topology.
 *
 * @param port  - port
 * @param enclosure - enclosure
 * @param slot - slot for the requested drive
 * @param object_id - pointer to the drive
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  7/21/09: sharel created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_physical_drive_object_id_by_location(fbe_u32_t port, 
                                                                           fbe_u32_t enclosure, 
                                                                           fbe_u32_t slot, 
                                                                           fbe_object_id_t *object_id)
{
    fbe_topology_control_get_physical_drive_by_location_t       topology_control_get_drive_by_location;
    fbe_status_t                                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t                     status_info;

    if (object_id == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }


    topology_control_get_drive_by_location.port_number = port;
    topology_control_get_drive_by_location.enclosure_number = enclosure;
    topology_control_get_drive_by_location.slot_number = slot;
    topology_control_get_drive_by_location.physical_drive_object_id = FBE_OBJECT_ID_INVALID;

     status = fbe_api_common_send_control_packet_to_service (FBE_TOPOLOGY_CONTROL_CODE_GET_PHYSICAL_DRIVE_BY_LOCATION,
                                                             &topology_control_get_drive_by_location,
                                                             sizeof(fbe_topology_control_get_physical_drive_by_location_t),
                                                             FBE_SERVICE_ID_TOPOLOGY,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_PHYSICAL);

     if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
         if ((status_info.control_operation_status == FBE_PAYLOAD_CONTROL_STATUS_FAILURE) && 
             (status_info.control_operation_qualifier == FBE_OBJECT_ID_INVALID)) {
             /*there was not object in this location, just return invalid id (was set before)and good status*/
              *object_id = FBE_OBJECT_ID_INVALID;
             return FBE_STATUS_OK;
         }

        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
     }

     *object_id = topology_control_get_drive_by_location.physical_drive_object_id;

    return status;

}

/*!***************************************************************
 * @fn fbe_api_physical_drive_power_cycle(
 *    fbe_object_id_t object_id,
 *    fbe_u32_t duration)
 *****************************************************************
 * @brief
 *  This function sends a command to a drive for power-cycle.
 *
 * @param object_id  - drive to power-cycle
 * @param duration - duration
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *    07/30/08: chenl6 created
 *    
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_power_cycle(fbe_object_id_t object_id, fbe_u32_t duration)
{

    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t     status_info;
    
    
    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_POWER_CYCLE,
                                                  &duration,
                                                  sizeof (fbe_u32_t),
                                                  object_id,
                                                  FBE_PACKET_FLAG_NO_ATTRIB,
                                                  &status_info,
                                                  FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
            status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!***************************************************************
 * @fn fbe_api_physical_drive_reset_slot(fbe_object_id_t object_id)
 *****************************************************************
 * @brief
 *  This function sends a command to a drive for RESET PHY.
 *
 * @param object_id  - drive to reset
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *    07/30/08: chenl6 created
 *    
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_reset_slot(fbe_object_id_t object_id)
{

    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t     status_info;
    
    
    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_RESET_SLOT,
                                                  NULL,
                                                  0,
                                                  object_id,
                                                  FBE_PACKET_FLAG_NO_ATTRIB,
                                                  &status_info,
                                                  FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
            status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!***************************************************************
 * @fn fbe_api_physical_drive_set_write_uncorrectable(
 *    fbe_object_id_t object_id,
 *    fbe_lba_t lba, 
 *    fbe_packet_attr_t attribute)
 *****************************************************************
 * @brief
 *  This function sends a command to a drive for power-cycle.
 *
 * @param object_id  - the SATA drive to send a write uncorrectable to
 * @param lba - value to set in address(HEX)
 * @param attribute - attribute
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *    07/31/09: CLC - created    
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_set_write_uncorrectable(fbe_object_id_t  object_id, fbe_lba_t lba, fbe_packet_attr_t attribute)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_physical_drive_mgmt_set_write_uncorrectable_t       set_lba;
    fbe_api_control_operation_status_info_t         status_info;

    if (lba == 0 || object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    set_lba.lba = lba;

    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_SATA_WRITE_UNCORRECTABLE,
                                                 &set_lba,
                                                 sizeof(fbe_physical_drive_mgmt_set_write_uncorrectable_t),
                                                 object_id,
                                                 attribute,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/*********************************************************************
*            fbe_api_create_read_uncorrectable ()
*********************************************************************
*
*  Description: Create a read uncorrectable error on a given LBA of a 
*               specified SAS drive.
*
*  Inputs: object id  - Specifies the SAS drive on which to create the error.
*          lba - Specifies where to create the error.
*
*  Outputs: None
*
*  Return Value: success or failure

*  History:
*    08/21/2013: Dipak Patel - Ported from Joe B.'s code on R31 code base
*********************************************************************/
fbe_status_t fbe_api_create_read_uncorrectable(fbe_object_id_t  object_id, fbe_lba_t lba, fbe_packet_attr_t attribute)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_physical_drive_mgmt_create_read_uncorrectable_t*  read_uncor_info = NULL;
    fbe_u8_t * buffer = NULL;   
    fbe_sg_element_t  * sg_list = NULL;
    fbe_u32_t resp_size = 1040 + (2 * sizeof(fbe_sg_element_t));
    fbe_u32_t i = 0;
    
    // First want to create a struct to hold the LBA and also the size of the ecc field
    read_uncor_info = (fbe_physical_drive_mgmt_create_read_uncorrectable_t *)fbe_api_allocate_memory (sizeof(fbe_physical_drive_mgmt_create_read_uncorrectable_t));
    if (read_uncor_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:unable to allocate memory\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    buffer = fbe_api_allocate_memory (resp_size);
    if (buffer == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:unable to allocate memory\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    sg_list = (fbe_sg_element_t  *)buffer;
    sg_list[0].count = 1024;
    sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);
    
    sg_list[1].count = 0;
    sg_list[1].address = NULL;
    
    // Set the LBA and the block size (smaller than what we would expect).
    read_uncor_info->lba = lba;
    read_uncor_info->long_block_size = 520; 

    /* First send a read long with 520 bytes specified for transfer length
       send a 10 byte or 16 byte read long as approrpiate
       use fbe_api_common_send_control_packet_with_sg_list   
       it will fail -- in it's completion function determine the correct transfer length
       sg list will be created in the executer function and the xfer length will be
       filled in in the executer completion function */
    
    /* The fbe_api_common_send_control_packet is synchronous and also frees the memory allocated
       for the packet */
    status = fbe_api_common_send_control_packet_with_sg_list(FBE_PHYSICAL_DRIVE_CONTROL_CODE_READ_LONG,
                                                             read_uncor_info,
                                                             sizeof (fbe_physical_drive_mgmt_create_read_uncorrectable_t),
                                                             object_id,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             sg_list,
                                                             1, /* The number of sg elements in the list */
                                                             &status_info,
                                                             FBE_PACKAGE_ID_PHYSICAL);
    
    /* we don't really particularly care what the status is at this point - we expected to
       get a check condition and read_uncor_info->long_block_size has been set to the difference
       between the requested length minus the actual length in bytes. */

    /* Now send a read long with the calculated valid byte transfer length
       send a 10 byte or 16 byte read long as approrpiate
       use fbe_api_common_send_control_packet_with_sg_list */

    read_uncor_info->long_block_size = 520 - read_uncor_info->long_block_size;

    /* The fbe_api_common_send_control_packet is synchronous and also frees the memory allocated
       for the packet */
    status = fbe_api_common_send_control_packet_with_sg_list(FBE_PHYSICAL_DRIVE_CONTROL_CODE_READ_LONG,
                                                             read_uncor_info,
                                                             sizeof (fbe_physical_drive_mgmt_create_read_uncorrectable_t),
                                                             object_id,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             sg_list,
                                                             1, /* The number of sg elements in the list */
                                                             &status_info,
                                                             FBE_PACKAGE_ID_PHYSICAL);
    
    /* Since this read long contains a valid byte transfer length we expect it to work.
       Thus the read long data in the sg list should be valid*/
    
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        /* The read long has failed so free up the resources  */
        fbe_api_free_memory (read_uncor_info);
        fbe_api_free_memory (buffer);
        /* and return failure */
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Fourth invalidate all of the data in the read buffer and then
       send a write long with the modified (invalidated) buffer 
       use a 10 or 16 byte write long as appropriate     
       use fbe_api_common_send_control_packet_with_sg_list */
    
    
    for (i = 0; i < 520; i++)
    {
        (sg_list[0].address)[i] = (sg_list[0].address)[i] ^ 0xFFFF;
    }
    
    status = fbe_api_common_send_control_packet_with_sg_list(FBE_PHYSICAL_DRIVE_CONTROL_CODE_WRITE_LONG,
                                                             read_uncor_info,
                                                             sizeof (fbe_physical_drive_mgmt_create_read_uncorrectable_t),
                                                             object_id,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             sg_list,
                                                             1, /* The number of sg elements in the list */
                                                             &status_info,
                                                             FBE_PACKAGE_ID_PHYSICAL);
    
    /* It is expected that the write long will work correctly thus
     *  creating an incorrectable read for the LBA requested but we will
     *  check the status to make sure.
     */
    
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        /* The write long has failed so free up the resources  */
        fbe_api_free_memory (read_uncor_info);
        fbe_api_free_memory (buffer);
        /* and return failure */
        return FBE_STATUS_GENERIC_FAILURE;
    }   
    
    /* If we get here the write long has worked so free the memory for the sg list and the
     * fbe_physical_drive_mgmt_create_read_uncorrectable_t structure 
     */
    fbe_api_free_memory (read_uncor_info);
    fbe_api_free_memory (buffer);   
    
    return status;
}

/*!***************************************************************
 * @fn fbe_api_physical_drive_receive_diag_info(
 *    fbe_object_id_t object_id,
 *    fbe_physical_drive_mgmt_receive_diag_page_t * diag_info)
 *****************************************************************
 * @brief
 *  This function sends a command to a drive for power-cycle.
 *
 * @param object_id  - The drive to set the depth
 * @param diag_info - pointer to the diag info structure
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  09/10/08: chenl6 created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_receive_diag_info(fbe_object_id_t object_id, 
                                                      fbe_physical_drive_mgmt_receive_diag_page_t * diag_info)
{

    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_physical_drive_mgmt_receive_diag_page_t * receive_diag_info;
    
    if (diag_info == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    receive_diag_info = 
        (fbe_physical_drive_mgmt_receive_diag_page_t *)fbe_api_allocate_memory (sizeof(fbe_physical_drive_mgmt_receive_diag_page_t));


    if (receive_diag_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:unable to allocate memory\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    receive_diag_info->page_code = diag_info->page_code;
    
    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_RECEIVE_DIAG_PAGE,
                                                  receive_diag_info,
                                                  sizeof (fbe_physical_drive_mgmt_receive_diag_page_t),
                                                  object_id,
                                                  FBE_PACKET_FLAG_NO_ATTRIB,
                                                  &status_info,
                                                  FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
            status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        fbe_api_free_memory (receive_diag_info);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(diag_info, receive_diag_info, sizeof(fbe_physical_drive_mgmt_receive_diag_page_t));
    
    fbe_api_free_memory (receive_diag_info);

    return status;
}

/*!***************************************************************
 * @fn fbe_api_physical_drive_get_disk_error_log(fbe_object_id_t object_id, 
 *     fbe_lba_t lba, fbe_u8_t * data_buffer)
 *****************************************************************
 *
 * @brief
 *   This function sends a command to get a disk error log.
 *
 * @param object_id   - Disk info
 * @param lba         - LBA
 * @param data_buffer - pointer to data buffer
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *    09/01/10: CLC created
 *    
 ****************************************************************/
 fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_disk_error_log(fbe_object_id_t object_id, 
                                                                    fbe_lba_t lba, fbe_u8_t * data_buffer)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_physical_drive_mgmt_get_disk_error_log_t * get_lba = NULL;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_u8_t * buffer = NULL;
    fbe_u32_t resp_size = DC_TRANSFER_SIZE + (2 * sizeof(fbe_sg_element_t));
    
    if (data_buffer == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    get_lba = (fbe_physical_drive_mgmt_get_disk_error_log_t *)fbe_api_allocate_memory (sizeof(fbe_physical_drive_mgmt_get_disk_error_log_t));
    if (get_lba == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:unable to allocate memory\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    buffer = fbe_api_allocate_memory (resp_size);
    if (buffer == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:unable to allocate memory\n",__FUNCTION__);
        fbe_api_free_memory (get_lba);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_list = (fbe_sg_element_t  *)buffer;
    sg_list[0].count = DC_TRANSFER_SIZE;
    sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    get_lba->lba = lba;
       
    status = fbe_api_common_send_control_packet_with_sg_list(FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DISK_ERROR_LOG,
                                                             get_lba,
                                                             sizeof (fbe_physical_drive_mgmt_get_disk_error_log_t),
                                                             object_id,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             sg_list,
                                                             1,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
            status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        fbe_copy_memory(data_buffer, "Error:lba ", (fbe_u32_t)FBE_MIN(sizeof("Error:lba "), DC_TRANSFER_SIZE-1));
        data_buffer[DC_TRANSFER_SIZE-1] = '\0'; 
        
        fbe_api_free_memory (get_lba);
        fbe_api_free_memory (buffer);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(data_buffer, sg_list[0].address, DC_TRANSFER_SIZE);
    
    fbe_api_free_memory (get_lba);
    fbe_api_free_memory (buffer);

    return status;
}

/*!***************************************************************
 * @fn fbe_api_physical_drive_get_cached_drive_info(
 *    fbe_object_id_t object_id,
 *    fbe_physical_drive_information_t *get_drive_information, 
 *    fbe_packet_attr_t attribute) 
 *****************************************************************
 * @brief
 *  This function gets the cached drive information.
 *
 * @param object_id  - The drive to set the depth
 * @param get_drive_information - pointer to the drive info
 * @param attribute - attribute info
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  10/30/09: chenl6 - created
 *
 ****************************************************************/
fbe_status_t fbe_api_physical_drive_get_cached_drive_info(fbe_object_id_t  object_id, 
                                                          fbe_physical_drive_information_t *get_drive_information, 
                                                          fbe_packet_attr_t attribute)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_physical_drive_mgmt_get_drive_information_t *drive_mgmt_get_drive_information;

    if (get_drive_information == NULL || object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    drive_mgmt_get_drive_information = (fbe_physical_drive_mgmt_get_drive_information_t*)fbe_api_allocate_memory (sizeof(fbe_physical_drive_mgmt_get_drive_information_t));
    if (drive_mgmt_get_drive_information == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:unable to allocate memory\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_CACHED_DRIVE_INFO,
                                                 drive_mgmt_get_drive_information,
                                                 sizeof(fbe_physical_drive_mgmt_get_drive_information_t),
                                                 object_id,
                                                 attribute,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        fbe_api_free_memory (drive_mgmt_get_drive_information);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(get_drive_information, &drive_mgmt_get_drive_information->get_drive_info, sizeof(fbe_physical_drive_information_t));
    fbe_api_free_memory (drive_mgmt_get_drive_information);

    return status;
}

/*!***************************************************************
 * @fn fbe_api_physical_drive_get_drive_information_asynch(
 *    fbe_object_id_t object_id,
 *    fbe_physical_drive_information_asynch_t *get_drive_information) 
 *****************************************************************
 * @brief
 *  This function gets the drive asynch info.
 *
 * @param object_id  - The drive to set the depth
 * @param get_drive_information - pointer to the drive info
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  10/30/09: chenl6 - created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_drive_information_asynch(fbe_object_id_t  object_id, 
                                                                 fbe_physical_drive_information_asynch_t *get_drive_information)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_physical_drive_mgmt_get_drive_information_t *drive_mgmt_get_drive_information;
    fbe_packet_t *                      packet = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;

    if (get_drive_information == NULL || object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    drive_mgmt_get_drive_information = (fbe_physical_drive_mgmt_get_drive_information_t*)fbe_api_allocate_memory (sizeof(fbe_physical_drive_mgmt_get_drive_information_t));
    if (drive_mgmt_get_drive_information == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:unable to allocate memory\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Allocate packet.*/
    packet = fbe_api_get_contiguous_packet();
    if (packet == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for packet\n", __FUNCTION__); 
        fbe_api_free_memory (drive_mgmt_get_drive_information);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    fbe_payload_control_build_operation (control_operation,
                                         FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DRIVE_INFORMATION,
                                         drive_mgmt_get_drive_information,
                                         sizeof(fbe_physical_drive_mgmt_get_drive_information_t));

    fbe_transport_set_completion_function (packet, fbe_api_physical_drive_get_drive_information_asynch_callback, get_drive_information);
    

    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              FBE_PACKAGE_ID_PHYSICAL,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id); 
    
    /* Mark packet with the attribute, either traverse or not */
    fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_NO_ATTRIB);

    status = fbe_api_common_send_control_packet_to_driver(packet);

    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
        /* Only trace when it is an error we do not expect.*/
         fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s error in sending packet, status:%d\n", __FUNCTION__, status);  
         fbe_payload_ex_release_control_operation(payload, control_operation);
         fbe_api_return_contiguous_packet(packet);
         fbe_api_free_memory (drive_mgmt_get_drive_information);
         return status;
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_api_physical_drive_get_drive_information_asynch_callback(
 *    fbe_packet_t * packet,
 *    fbe_packet_completion_context_t context)
 *****************************************************************
 * @brief
 *  This function gets drive asynch callback. 
 *
 * @param packet  - packet info
 * @param context - context of the callback
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  09/10/08: chenl6 created
 *
 ****************************************************************/
static fbe_status_t fbe_api_physical_drive_get_drive_information_asynch_callback(fbe_packet_t * packet, 
                                                                                  fbe_packet_completion_context_t context)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_physical_drive_information_asynch_t * get_drive_information = (fbe_physical_drive_information_asynch_t *)context;
    fbe_physical_drive_mgmt_get_drive_information_t *drive_mgmt_get_drive_information;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_payload_control_status_qualifier_t control_status_qualifier = 0;

    status = fbe_transport_get_status_code(packet);
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation, &control_status_qualifier);
    if (status != FBE_STATUS_OK || control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:status:%d, payload status:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, control_status, control_status_qualifier);
    }

    fbe_payload_control_get_buffer(control_operation, &drive_mgmt_get_drive_information);
    fbe_copy_memory(&get_drive_information->get_drive_info, &drive_mgmt_get_drive_information->get_drive_info, sizeof(fbe_physical_drive_information_t));

    fbe_payload_ex_release_control_operation(payload, control_operation);
    fbe_api_return_contiguous_packet(packet);

    fbe_api_free_memory (drive_mgmt_get_drive_information);

    /* call callback fundtion */
    (get_drive_information->completion_function) (context);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_api_physical_drive_fail_drive(
 *  fbe_object_id_t  object_id, 
 *  fbe_base_physical_drive_death_reason_t reason)
 *    
 *****************************************************************
 * @brief
 *  This function fails a drive  
 *
 * @param object_id - Drive being failed
 * @param reason - Reason for being failed
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  05/18/10: Created.  Wayne Garrett
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_fail_drive(fbe_object_id_t  object_id, fbe_base_physical_drive_death_reason_t reason)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;       
    fbe_api_control_operation_status_info_t     status_info;
    
    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_FAIL_DRIVE,
                                                 &reason,
                                                 sizeof(fbe_base_physical_drive_death_reason_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
            status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
        
    return FBE_STATUS_OK;   
}

/*!***************************************************************
 * @fn fbe_api_physical_drive_enter_glitch_drive(fbe_object_id_t  object_id, fbe_time_t glitch_time)
 *    
 *****************************************************************
 * @brief
 *  This function glitches a drive  
 *
 * 
 * @return fbe_status_t - success or failure
 *
 * @version
 *  08/17/12: Modified.  kothal
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_enter_glitch_drive(fbe_object_id_t  object_id, fbe_time_t glitch_time)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;    
    fbe_packet_t *                          packet = NULL;
    fbe_payload_ex_t *                      payload = NULL;
    fbe_payload_control_operation_t *       control_operation = NULL;   
    fbe_payload_control_status_t            control_status;
    
    packet = fbe_api_get_contiguous_packet();
    if (packet == NULL) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to allocate packet\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    
    fbe_payload_control_build_operation (control_operation,
                                         FBE_PHYSICAL_DRIVE_CONTROL_ENTER_GLITCH_MODE,
                                         &glitch_time,
                                         sizeof(fbe_time_t));
    
    fbe_api_common_send_control_packet_asynch(packet,
                                              object_id,
                                              NULL,
                                              NULL,
                                              FBE_PACKET_FLAG_NO_ATTRIB,
                                              FBE_PACKAGE_ID_PHYSICAL);    
    
    status = fbe_transport_get_status_code(packet);
    fbe_payload_control_get_status(control_operation, &control_status);
    
    if (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK || status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d\n", __FUNCTION__, status);
    }
    
    fbe_payload_ex_release_control_operation(payload, control_operation);

    fbe_api_return_contiguous_packet(packet);/*no need to destory !*/   
    
    return FBE_STATUS_OK; 
}


/*!***************************************************************
 * @fn fbe_api_physical_drive_fw_download_start_asynch( 
 *           fbe_physical_drive_fwdl_start_asynch_t *start_info)
 *    
 *****************************************************************
 * @brief
 *  This function is the first step in the firmware download
 *  process. PDO will determine if a download can proceed and
 *  send the status via a completion function.
 *
 * @param start_info - Drive for firmware download
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  07/05/10: Created.  Wayne Garrett
 *
 ****************************************************************/ 
fbe_status_t FBE_API_CALL 
fbe_api_physical_drive_fw_download_start_asynch(fbe_physical_drive_fwdl_start_asynch_t *start_info )
{      
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                      packet = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_physical_drive_fwdl_start_response_t * pkt_response;

    if (start_info == NULL || start_info->drive_object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
   /* Allocate packet.*/
    packet = fbe_api_get_contiguous_packet();
    if (packet == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    /* Allocate response data. */
    pkt_response = (fbe_physical_drive_fwdl_start_response_t *) fbe_api_allocate_memory (sizeof (fbe_physical_drive_fwdl_start_response_t));
    if (pkt_response == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for packet\n", __FUNCTION__); 
        fbe_api_free_memory(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_copy_memory(pkt_response, &start_info->response, sizeof(fbe_physical_drive_fwdl_start_response_t));

    payload = fbe_transport_get_payload_ex (packet);
    
    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    fbe_payload_control_build_operation (control_operation,
                                         FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_START,
                                         pkt_response,   
                                         sizeof(fbe_physical_drive_fwdl_start_response_t));

    fbe_transport_set_completion_function (packet, 
                                           fbe_api_physical_drive_fw_download_start_async_callback, 
                                           start_info);
    

    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              FBE_PACKAGE_ID_PHYSICAL,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              start_info->drive_object_id); 
    
    /* Mark packet with the attribute, either traverse or not */
    fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_NO_ATTRIB);

    status = fbe_api_common_send_control_packet_to_driver(packet);

    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
        /* Only trace when it is an error we do not expect.*/
         fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s error in sending packet, status:%d\n", __FUNCTION__, status);  
         fbe_payload_ex_release_control_operation(payload, control_operation); 
         fbe_api_return_contiguous_packet(packet);
         return status;
    }
    
    return FBE_STATUS_OK;    
}

/*!***************************************************************
 * @fn fbe_api_physical_drive_fw_download_start_async_callback(
 *    fbe_packet_t * packet,
 *    fbe_packet_completion_context_t context)
 *****************************************************************
 * @brief
 *  This function handles the start firmware download asynch callback. 
 *
 * @param packet  - packet info
 * @param context - context of the callback
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  06/09/10: Created.  Wayne Garrett
 *
 ****************************************************************/
static fbe_status_t 
fbe_api_physical_drive_fw_download_start_async_callback (fbe_packet_t * packet, 
                                                         fbe_packet_completion_context_t context)
{
    fbe_status_t  status = FBE_STATUS_OK;
    fbe_physical_drive_fwdl_start_asynch_t * start_info = 
            (fbe_physical_drive_fwdl_start_asynch_t *)context;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_payload_control_status_qualifier_t control_status_qualifier = 0;
    fbe_physical_drive_fwdl_start_response_t *pkt_response;
    

    status = fbe_transport_get_status_code(packet);
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation, &control_status_qualifier);

    start_info->completion_status = FBE_STATUS_OK;
    if (status != FBE_STATUS_OK || control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:status:%d, payload status:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, control_status, control_status_qualifier);
        start_info->completion_status = FBE_STATUS_GENERIC_FAILURE;
        start_info->response.proceed = FBE_FALSE;
    }

    fbe_payload_control_get_buffer(control_operation, &pkt_response);
    fbe_copy_memory(&start_info->response, &pkt_response, sizeof(fbe_physical_drive_fwdl_start_response_t));

    fbe_payload_ex_release_control_operation(payload, control_operation);
    fbe_api_return_contiguous_packet(packet); 
    fbe_api_free_memory(pkt_response);

    /* call callback function */
    (start_info->completion_function) (context);
    
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_api_physical_drive_fw_download_asynch( 
 *       fbe_object_id_t                       object_id,
 *       const fbe_sg_element_t *              sg_list, 
 *       fbe_u32_t                             sg_blocks,
 *       fbe_physical_drive_fw_info_asynch_t * fw_info)
 *
 *****************************************************************
 * @brief
 *  This function issues firmware download asynchronously.
 *
 * @param object_id - Drive for firmware download
 * @param sg_list - SG List
 * @param sg_blocks - SG Blocks
 * @param fw_info - pointer to Drive FW Info
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  06/09/10: Created.  Wayne Garrett
 *
 ****************************************************************/  
fbe_status_t FBE_API_CALL 
fbe_api_physical_drive_fw_download_asynch(fbe_object_id_t                       object_id,
                                          const fbe_sg_element_t *              sg_list, 
                                          fbe_u32_t                             sg_blocks,
                                          fbe_physical_drive_fw_info_asynch_t * fw_info)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                      packet = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_physical_drive_mgmt_fw_download_t * download_info = NULL;
    fbe_sg_element_t *                  download_sg_list = NULL;
    const fbe_sg_element_t *            sg_element = NULL;
    fbe_sg_element_t *                  dl_element = NULL;
    fbe_u32_t                           size = 0;
    fbe_u32_t                           sg_list_count = 0;
    fbe_u32_t                           i;

    if (fw_info == NULL || object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

   
    /* Allocate packet.*/
    packet = fbe_api_get_contiguous_packet();
    if (packet == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload = fbe_transport_get_payload_ex (packet);
    
    control_operation = fbe_payload_ex_allocate_control_operation(payload);


    download_info = (fbe_physical_drive_mgmt_fw_download_t *)fbe_api_allocate_memory (sizeof (fbe_physical_drive_mgmt_fw_download_t));
    if (download_info == NULL) {
        fbe_payload_ex_release_control_operation(payload, control_operation);
        fbe_api_return_contiguous_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /*we are ready to populate the packet with the information, let's put some default stuff in the buffer*/
    download_info->download_error_code = 0;
    download_info->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
    download_info->qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE;
    download_info->transfer_count = sg_blocks; /*the # of bytes to send*/

    fbe_payload_control_build_operation (control_operation,
                                         FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD,
                                         download_info,
                                         sizeof(fbe_physical_drive_mgmt_fw_download_t));

    sg_element = sg_list;
    while (size < sg_blocks)
    {
        size += sg_element->count;
        sg_list_count++;
        sg_element++;
    }

    /*we also allocate sg entries, */
    download_sg_list = (fbe_sg_element_t *)fbe_api_allocate_contiguous_memory ((sizeof (fbe_sg_element_t) * (sg_list_count + 1)));
    if (download_sg_list == NULL) {
        fbe_payload_ex_release_control_operation(payload, control_operation);
        fbe_transport_destroy_packet(packet); 
        fbe_api_free_memory(download_info);
        fbe_api_return_contiguous_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }else{
        fbe_zero_memory(download_sg_list, (sizeof (fbe_sg_element_t) * (sg_list_count +1)));/*this is how we quicky set up the terminating element as well*/
    }    

    /*now set up the sg list correctly. The terminating element is set by default since we zeroed out memory block at the top*/
    sg_element = sg_list;
    dl_element = download_sg_list;
    for (i = 0; i < sg_list_count; i++)
    {
        dl_element->address = sg_element->address;
        dl_element->count = sg_element->count;
        dl_element++;
        sg_element++;
    }
    fbe_payload_ex_set_sg_list(payload, download_sg_list, sg_list_count);

    fbe_transport_set_completion_function (packet, fbe_api_physical_drive_fw_download_async_callback, fw_info);
    

    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              FBE_PACKAGE_ID_PHYSICAL,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id); 
    
    /* Mark packet with the attribute, either traverse or not */
    fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_NO_ATTRIB);

    status = fbe_api_common_send_control_packet_to_driver(packet);

    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
        /* Only trace when it is an error we do not expect.*/
         fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s error in sending packet, status:%d\n", __FUNCTION__, status);  
         fbe_api_free_memory(download_info);
         fbe_api_free_contiguous_memory(download_sg_list);
         fbe_payload_ex_release_control_operation(payload, control_operation);
         fbe_api_return_contiguous_packet(packet);
         return status;
    }
    
    return FBE_STATUS_OK;    
}

/*!***************************************************************
 * @fn fbe_api_physical_drive_fw_download_async_callback(
 *    fbe_packet_t * packet,
 *    fbe_packet_completion_context_t context)
 *****************************************************************
 * @brief
 *  This function gets firmware download asynch callback. 
 *
 * @param packet  - packet info
 * @param context - context of the callback
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  06/09/10: Created.  Wayne Garrett
 *
 ****************************************************************/
static fbe_status_t 
fbe_api_physical_drive_fw_download_async_callback (fbe_packet_t * packet, 
                                                   fbe_packet_completion_context_t context)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_physical_drive_fw_info_asynch_t *   fw_info = 
            (fbe_physical_drive_fw_info_asynch_t *)context;
    fbe_payload_ex_t *                         payload = NULL;
    fbe_payload_control_operation_t *       control_operation = NULL;
    fbe_payload_control_status_t            control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_payload_control_status_qualifier_t  control_status_qualifier = 0;
    fbe_physical_drive_mgmt_fw_download_t * download_info;
    fbe_sg_element_t *                      sg_element;
    fbe_u32_t                               sg_count = 0;

    status = fbe_transport_get_status_code(packet);
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation, &control_status_qualifier);

    fw_info->completion_status = FBE_STATUS_OK;
    if (status != FBE_STATUS_OK || control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:status:%d, payload status:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, control_status, control_status_qualifier);
        fw_info->completion_status = FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &download_info);

    /* clean up */
    fbe_api_free_memory(download_info);
    fbe_payload_ex_get_sg_list(payload, &sg_element, &sg_count);
    fbe_api_free_contiguous_memory(sg_element);
    fbe_payload_ex_release_control_operation(payload, control_operation);

    fbe_api_return_contiguous_packet(packet);


    /* call callback function */
    (fw_info->completion_function) (context);
    
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_api_physical_drive_fw_download_stop_asynch( 
 *  fbe_physical_drive_fwdl_stop_asynch_t *stop_info)
 *    
 *****************************************************************
 * @brief
 *  This function issues firmware download stop command, which
 *  is asynchronous.  The stop command will inform  PDO to do
 *  any necessary cleanup and have it transition back to READY
 *  state.
 *
 * @param stop_info - Drive stopping firmware download
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  06/09/10: Created.  Wayne Garrett
 *
 ****************************************************************/  
fbe_status_t FBE_API_CALL 
fbe_api_physical_drive_fw_download_stop_asynch(fbe_physical_drive_fwdl_stop_asynch_t *stop_info)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                      packet = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;

    if (stop_info == NULL || stop_info->drive_object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    /* Allocate packet.*/
    packet = fbe_api_get_contiguous_packet();
    if (packet == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload = fbe_transport_get_payload_ex (packet);
    
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

    fbe_payload_control_build_operation (control_operation,
                                         FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_STOP,
                                         NULL,
                                         0);

    fbe_transport_set_completion_function (packet, fbe_api_physical_drive_fw_download_stop_async_callback, stop_info);
    
    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              FBE_PACKAGE_ID_PHYSICAL,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              stop_info->drive_object_id); 
    
    /* Mark packet with the attribute, either traverse or not */
    fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_NO_ATTRIB);

    status = fbe_api_common_send_control_packet_to_driver(packet);

    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
         fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s error in sending packet, status:%d\n", __FUNCTION__, status);  
         fbe_payload_ex_release_control_operation(payload, control_operation);
         fbe_api_return_contiguous_packet(packet);
         return status;
    }
    
    return FBE_STATUS_OK;    
}

/*!***************************************************************
 * @fn fbe_api_physical_drive_fw_download_stop_async_callback(
 *    fbe_packet_t * packet,
 *    fbe_packet_completion_context_t context)
 *****************************************************************
 * @brief
 *  This function is the callback for firmware-download-stop
 *  command.
 *
 * @param packet  - packet info
 * @param context - context of the callback
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  07/01/10: Created.  Wayne Garrett
 *
 ****************************************************************/
static fbe_status_t 
fbe_api_physical_drive_fw_download_stop_async_callback (fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_physical_drive_fwdl_stop_asynch_t * stop_info = context;
    fbe_payload_ex_t *                         payload = NULL;
    fbe_payload_control_operation_t *       control_operation = NULL;
    fbe_payload_control_status_t            control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_payload_control_status_qualifier_t  control_status_qualifier = 0;

    status = fbe_transport_get_status_code(packet);
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation, &control_status_qualifier);

    stop_info->completion_status = FBE_STATUS_OK;
    if (status != FBE_STATUS_OK || control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:status:%d, payload status:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, control_status, control_status_qualifier);
        stop_info->completion_status = FBE_STATUS_GENERIC_FAILURE;
    }

    /* cleanup */
    fbe_payload_ex_release_control_operation(payload, control_operation);
    fbe_api_return_contiguous_packet(packet);

    /* call callback function */
    (stop_info->completion_function) (context);
    
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_api_physical_drive_set_drive_death_reason(
 *    fbe_object_id_t  object_id, 
 *    FBE_DRIVE_DEAD_REASON death_reason, 
 *    fbe_packet_attr_t attribute)
 *****************************************************************
 * @brief
 *  This function is set the drive death reason of a specific object.
 *
 * @param object_id  - the drive to set the death reason
 * @param dead_reason - Reason why drive dies
 * @param attribute - attribute
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *    04/08/2010: CLC - created
 *    
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_set_drive_death_reason(fbe_object_id_t  object_id, 
     FBE_DRIVE_DEAD_REASON death_reason, fbe_packet_attr_t attribute)
{
    fbe_status_t                                  status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t       status_info;
    fbe_physical_drive_mgmt_drive_death_reason_t  set_death_reason;

    if (object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    set_death_reason.death_reason = death_reason;    /*pay attention we would set teh FBE_PACKET_FLAG_TRAVERSE because all times the ioctls are actually ment to the logical drive*/
    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_DRIVE_DEATH_REASON,
                                                 &set_death_reason,
                                                 sizeof(fbe_physical_drive_mgmt_drive_death_reason_t),
                                                 object_id,
                                                 attribute,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet:error:%d qual:%d; payload:error:%d, qual:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

 /*********************************************************************
 *            fbe_api_physical_drive_dc_rcv_diag_enabled ()
 *********************************************************************
 *
 *  Description: This function enable collecting receive diagnostic page once per day..
 *
 *  Inputs: object id
 *
 *  Return Value: success or failure
 *
 *  History:
 *    10/10/2010: CLC created
 *    
 *********************************************************************/
fbe_status_t fbe_api_physical_drive_dc_rcv_diag_enabled(fbe_object_id_t object_id, fbe_u8_t flag)
{

    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t     status_info;
    
    
    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_DC_RCV_DIAG_ENABLED,
                                                  &flag,
                                                  sizeof (fbe_u8_t),
                                                  object_id,
                                                  FBE_PACKET_FLAG_NO_ATTRIB,
                                                  &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
            status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!***************************************************************
 * @fn fbe_api_physical_drive_health_check()
 *****************************************************************
 * @brief
 *  This function sends a command to a drive to enable a HEALTH CHECK..
 *
 * @param object_id  - drive on which to force the health check.
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *    05/01/2012 - Created. Darren Insko
 *    
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_health_check(fbe_object_id_t object_id)
{

    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t     status_info;
 
    if (object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_HEALTH_CHECK_INITIATE,
                                                  NULL,
                                                  0,
                                                  object_id,
                                                  FBE_PACKET_FLAG_NO_ATTRIB,
                                                  &status_info,
                                                  FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
            status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_physical_drive_health_check_test()
 *****************************************************************
 * @brief
 *  This function sends a command to a drive to test full HEALTH CHECK code path.
 *
 * @param object_id  - drive on which to force the health check.
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *    05/01/2012 - Created. Darren Insko
 *    
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_health_check_test(fbe_object_id_t object_id)
{

    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t     status_info;
 
    if (object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_HEALTH_CHECK_TEST,
                                                  NULL,
                                                  0,
                                                  object_id,
                                                  FBE_PACKET_FLAG_NO_ATTRIB,
                                                  &status_info,
                                                  FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
            status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}


/*!***************************************************************
 * @fn fbe_api_physical_drive_enable_disable_perf_stats()
 *****************************************************************
 * @brief
 *  This function sends a command to enable or disable the performance
 *  statistics for a specific drive.
 *
 * @param object_id  - PDO on which to enable or disable perf stats
 * @param set_enable - TRUE when enabling or FALSE when disabling stats
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *    05/07/2012 - Created. Darren Insko
 *    
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_enable_disable_perf_stats(fbe_object_id_t object_id,
                                                                           fbe_bool_t b_set_enable)
{

    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t     status_info;
 
    if (object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_ENABLE_DISABLE_PERF_STATS,
                                                 &b_set_enable,
                                                 sizeof(fbe_bool_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d\n", __FUNCTION__, status);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_physical_drive_get_perf_stats()
 *****************************************************************
 * @brief
 *  This function gets a copy of the PDO's performance statistics.
 *
 * WARNING: this function was only created because we needed a way to copy performance
 * statistics across the transport buffer so fbe_hw_test could access the counter space instead
 * of copying the entire container on each check! Use the PERFSTATS service to access the performance
 * statistics in non-test environments.
 *
 * @param object_id  - PDO on which to enable or disable perf stats
 * @param pdo_counters - pointer to a pdo perfstats struct
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *    07/12/2012 - Created. Ryan Gadsby
 *    
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_perf_stats(fbe_object_id_t object_id,
                                                                fbe_pdo_performance_counters_t *pdo_counters)
{

    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t     status_info;
 
    if (object_id >= FBE_OBJECT_ID_INVALID) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:object_id can't be FBE_OBJECT_ID_INVALID\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (pdo_counters == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:pdo_counters can't be NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    
    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_PERF_STATS,
                                                 pdo_counters,
                                                 sizeof(fbe_pdo_performance_counters_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d\n", __FUNCTION__, status);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*********************************************************************
 * @fn fbe_api_physical_drive_get_fuel_gauge_asynch()
 *********************************************************************
 * @brief:
 *      Async fuel gauge command.
 *
 *  @param object_id - The object id to send request to
 *  @param fuel_gauge_op - pointer to the information passed from the the client
 *
 *  @return fbe_status_t, success or failure
 *
 *  History:
 *    11/16/2010: chenl6 - created
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_fuel_gauge_asynch(fbe_object_id_t object_id, 
                                           fbe_physical_drive_fuel_gauge_asynch_t * fuel_gauge_op)                                           
{

    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;    
    fbe_u32_t           sg_item = 0;
    fbe_packet_t *      packet = NULL;

    if (fuel_gauge_op == NULL || object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }  

    /* set next sg element size and address for the response buffer. */
    fbe_sg_element_init(&(fuel_gauge_op->sg_element[sg_item]), 
                        fuel_gauge_op->get_log_page.transfer_count, 
                        fuel_gauge_op->response_buf);
    sg_item++;
    fbe_sg_element_terminate(&fuel_gauge_op->sg_element[sg_item]);
    sg_item++;

    packet = fbe_api_get_contiguous_packet();/*no need to touch or initialize, it's taken from a pool and taken care of*/
    if (packet == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    

    
    status = fbe_api_common_send_control_packet_with_sg_list_async(packet,
                                                                   FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_LOG_PAGES,
                                                                   &fuel_gauge_op->get_log_page,
                                                                   sizeof(fbe_physical_drive_mgmt_get_log_page_t),                                           
                                                                   object_id, 
                                                                   FBE_PACKET_FLAG_NO_ATTRIB,
                                                                   fuel_gauge_op->sg_element, 
                                                                   sg_item,
                                                                   fbe_api_physical_drive_get_fuel_gauge_asynch_callback, 
                                                                   fuel_gauge_op,
                                                                   FBE_PACKAGE_ID_PHYSICAL);


    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
        /* Only trace when it is an error we do not expect.*/
         fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s fbe_api_common_send_control_packet_with_sg_list_async failed, status: 0x%x.\n", __FUNCTION__, status);  
    }
    
    return status;
} // end fbe_api_physical_drive_get_fuel_gauge_asynch

/*********************************************************************
 * @fn fbe_api_physical_drive_get_fuel_gauge_asynch_callback()
 *********************************************************************
 * @brief:
 *      callback function for fuel gauge command.
 *
 *  @param packet - pointer to fbe_packet_t.
 *  @param context - packet completion context
 *
 *  @return fbe_status_t, success or failure
 *
 *  History:
 *    11/16/2010: chenl6 - created
 *********************************************************************/
static fbe_status_t fbe_api_physical_drive_get_fuel_gauge_asynch_callback (fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_physical_drive_fuel_gauge_asynch_t *fuel_gauge_op = (fbe_physical_drive_fuel_gauge_asynch_t *)context;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_payload_control_status_qualifier_t control_status_qualifier = 0;

    status = fbe_transport_get_status_code(packet);
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation, &control_status_qualifier);

    if ((status != FBE_STATUS_OK) || 
        (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:status:%d, payload status:%d & qulf:%d, page 0x%X\n", __FUNCTION__,
                       status, control_status, control_status_qualifier, fuel_gauge_op->get_log_page.page_code);

        status = FBE_STATUS_COMPONENT_NOT_FOUND;
    }          

    /* Fill the status */
    fuel_gauge_op->status = status;

    fbe_api_return_contiguous_packet(packet);/*no need to destory or free it's returned to a queue and reused*/

    /* call callback fundtion */
    (fuel_gauge_op->completion_function) (context);

    return status;
}


/*!***************************************************************
   @fn fbe_status_t FBE_API_CALL fbe_api_physical_drive_send_pass_thru() 
 ****************************************************************
 * @brief
 *  This function sends pass through commands to PDO.  This will block
 *  until cmd completes from drive.   Use _async version if blocking
 *  is not an option.
 *
 *  @param object_id - The object id to send request to
 *  @param pass_thru_op - pointer to the information passed from the the client
 *
 * @return
 *  fbe_status_t
 *
 *  History:
 *    10/10/2013: Wayne Garrett - created
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_send_pass_thru(fbe_object_id_t object_id, 
                                                                fbe_physical_drive_send_pass_thru_t * pass_thru_op)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_semaphore_t get_mp_completion_semaphore;
    fbe_physical_drive_send_pass_thru_asynch_t  pass_thru_asynch;

    fbe_semaphore_init(&get_mp_completion_semaphore, 0, 1);

    pass_thru_asynch.pass_thru = *pass_thru_op;
    pass_thru_asynch.completion_function = fbe_api_physical_drive_send_pass_thru_completion;
    pass_thru_asynch.context = &get_mp_completion_semaphore;

    status = fbe_api_physical_drive_send_pass_thru_asynch(object_id, &pass_thru_asynch);
    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s failed. status: %d\n", __FUNCTION__, status);
        fbe_semaphore_destroy(&get_mp_completion_semaphore);
        return status;
    }  

    fbe_semaphore_wait(&get_mp_completion_semaphore, NULL);
    fbe_semaphore_destroy(&get_mp_completion_semaphore);

    /* copy result status back */
    pass_thru_op->cmd.payload_cdb_scsi_status = pass_thru_asynch.pass_thru.cmd.payload_cdb_scsi_status;
    pass_thru_op->cmd.port_request_status     = pass_thru_asynch.pass_thru.cmd.port_request_status;
    pass_thru_op->cmd.transfer_count          = pass_thru_asynch.pass_thru.cmd.transfer_count;
    fbe_copy_memory(pass_thru_op->cmd.sense_info_buffer, pass_thru_asynch.pass_thru.cmd.sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);

    return FBE_STATUS_OK;
}

/*!***************************************************************
   @fn fbe_api_physical_drive_send_pass_thru_completion
 ****************************************************************
 * @brief
 *  This is the completion function for the pass_thru synchronous function.
 *  The synchronous version wraps the asynch version so completion function
 *  is needed.
 *
 *  @param context - semaphore to release
 *
 *  @return void
 *
 *  History:
 *    10/10/2013: Wayne Garrett - created
 ****************************************************************/
static void fbe_api_physical_drive_send_pass_thru_completion(void * context)
{
    fbe_semaphore_t * get_mp_completion_semaphore_p = (fbe_semaphore_t*)context;
    fbe_semaphore_release(get_mp_completion_semaphore_p, 0, 1 ,FALSE);
}

/*!***************************************************************
   @fn fbe_status_t FBE_API_CALL fbe_api_physical_drive_send_pass_thru_asynch() 
 ****************************************************************
 * @brief
 *  This function sends pass through commands to PDO.
 *
 *  @param object_id - The object id to send request to
 *  @param pass_thru_op - pointer to the information passed from the the client
 *
 * @return
 *  fbe_status_t
 *
 *  History:
 *    12/2/2010: chenl6 - created
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_send_pass_thru_asynch(fbe_object_id_t object_id, 
                                           fbe_physical_drive_send_pass_thru_asynch_t * pass_thru_op)                                           
{
    fbe_status_t status;
    fbe_sg_element_t *sg_element, *sg_element_head;
    fbe_packet_t * packet = NULL;

    if (object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }  

    if (pass_thru_op->completion_function == NULL || pass_thru_op->pass_thru.response_buf == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Must have a valid completion function and response buf\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_element = sg_element_head = (fbe_sg_element_t *)fbe_api_allocate_memory(sizeof(fbe_sg_element_t) * 2);
    if (sg_element == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for sg list\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*init the sg list*/
    fbe_sg_element_init(sg_element++, 
                        pass_thru_op->pass_thru.cmd.transfer_count, 
                        pass_thru_op->pass_thru.response_buf);
    fbe_sg_element_terminate(sg_element);

    /*init the sg list*/
    pass_thru_op->pass_thru.cmd.payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    pass_thru_op->pass_thru.cmd.sense_buffer_lengh = FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE;

    packet = fbe_api_get_contiguous_packet();/*no need to touch or initialize, it's taken from a pool and taken care of*/
    if (packet == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for packet\n", __FUNCTION__); 
        fbe_api_free_memory(sg_element_head);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    
    status = fbe_api_common_send_control_packet_with_sg_list_async(packet,
                                                                   FBE_PHYSICAL_DRIVE_CONTROL_CODE_SEND_PASS_THRU_CDB,
                                                                   &pass_thru_op->pass_thru.cmd,
                                                                   sizeof(fbe_physical_drive_mgmt_send_pass_thru_cdb_t),                                           
                                                                   object_id, 
                                                                   FBE_PACKET_FLAG_NO_ATTRIB,
                                                                   sg_element_head, 
                                                                   2,
                                                                   fbe_api_physical_drive_send_pass_thru_asynch_callback, 
                                                                   pass_thru_op,
                                                                   FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
        /* Only trace when it is an error we do not expect.*/
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s error in sending packet, status:%d\n", __FUNCTION__, status);          
        fbe_api_free_memory(sg_element_head);
    }

    
    return status;
}


/*********************************************************************
 * @fn fbe_api_physical_drive_send_pass_thru_asynch_callback()
 *********************************************************************
 * @brief:
 *      callback function for pass thru command.
 *
 *  @param packet - pointer to fbe_packet_t.
 *  @param context - packet completion context
 *
 *  @return fbe_status_t
 *
 *  History:
 *    12/2/2010: chenl6 - created
 *********************************************************************/
static fbe_status_t fbe_api_physical_drive_send_pass_thru_asynch_callback(fbe_packet_t *packet, fbe_packet_completion_context_t context)
{
    fbe_status_t                                 status = FBE_STATUS_OK;
    fbe_physical_drive_send_pass_thru_asynch_t * pass_thru_op = (fbe_physical_drive_send_pass_thru_asynch_t *)context;
    fbe_payload_ex_t *                           payload = NULL;
    fbe_payload_control_operation_t *            control_operation = NULL;
    fbe_payload_control_status_t                 control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_sg_element_t *                           sg_element;
    fbe_u32_t                                    sg_count = 0;

    status = fbe_transport_get_status_code(packet);
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_ex_get_sg_list(payload, &sg_element, &sg_count);

    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:status:%d\n", __FUNCTION__, status);
    }          

    /* Fill the status */
    pass_thru_op->pass_thru.status = status;

    /* call callback fundtion */
    (pass_thru_op->completion_function) (pass_thru_op->context);

    fbe_api_return_contiguous_packet(packet);/*no need to destroy of fee, it's part of a queue*/
    fbe_api_free_memory(sg_element);

    return FBE_STATUS_OK;
}


/*********************************************************************
 * @fn fbe_api_physical_drive_drivegetlog_asynch()
 *********************************************************************
 * @brief:
 *      Async drivegetlog command.
 *
 *  @param object_id - The object id to send request to
 *  @param smart_dump_op - pointer to the information passed from the the client
 *
 *  @return fbe_status_t, success or failure
 *
 *  History:
 *    03/28/2011: chenl6 - created
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_drivegetlog_asynch(fbe_object_id_t object_id, 
                                           fbe_physical_drive_get_smart_dump_asynch_t * smart_dump_op)                                           
{

    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;    
    fbe_u32_t           sg_item = 0;
    fbe_packet_t *      packet = NULL;

    if (smart_dump_op == NULL || object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }  

    /* set next sg element size and address for the response buffer. */
    fbe_sg_element_init(&(smart_dump_op->sg_element[sg_item]), 
                        smart_dump_op->get_smart_dump.transfer_count, 
                        smart_dump_op->response_buf);
    sg_item++;
    fbe_sg_element_terminate(&smart_dump_op->sg_element[sg_item]);
    sg_item++;

    packet = fbe_api_get_contiguous_packet();/*no need to touch or initialize, it's taken from a pool and taken care of*/
    if (packet == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    

    status = fbe_api_common_send_control_packet_with_sg_list_async(packet,
                                                                   FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_SMART_DUMP,
                                                                   &smart_dump_op->get_smart_dump,
                                                                   sizeof(fbe_physical_drive_mgmt_get_smart_dump_t),                                           
                                                                   object_id, 
                                                                   FBE_PACKET_FLAG_NO_ATTRIB,
                                                                   smart_dump_op->sg_element, 
                                                                   sg_item,
                                                                   fbe_api_physical_drive_drivegetlog_asynch_callback, 
                                                                   smart_dump_op,
                                                                   FBE_PACKAGE_ID_PHYSICAL);


    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
        /* Only trace when it is an error we do not expect.*/
         fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s fbe_api_common_send_control_packet_with_sg_list_async failed, status: 0x%x.\n", __FUNCTION__, status);  
    }
    
    return FBE_STATUS_OK;
} // end fbe_api_physical_drive_drivegetlog_asynch

/*********************************************************************
 * @fn fbe_api_physical_drive_drivegetlog_asynch_callback()
 *********************************************************************
 * @brief:
 *      callback function for drivegetlog command.
 *
 *  @param packet - pointer to fbe_packet_t.
 *  @param context - packet completion context
 *
 *  @return fbe_status_t, success or failure
 *
 *  History:
 *    03/28/2011: chenl6 - created
 *********************************************************************/
static fbe_status_t fbe_api_physical_drive_drivegetlog_asynch_callback (fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_physical_drive_get_smart_dump_asynch_t *smart_dump_op = (fbe_physical_drive_get_smart_dump_asynch_t *)context;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_payload_control_status_qualifier_t control_status_qualifier = 0;

    status = fbe_transport_get_status_code(packet);
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation, &control_status_qualifier);

    if ((status != FBE_STATUS_OK) || 
        (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:status:%d, payload status:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, control_status, control_status_qualifier);

        status = FBE_STATUS_GENERIC_FAILURE;
    }          

    /* Fill the status */
    smart_dump_op->status = status;

    fbe_api_return_contiguous_packet(packet);/*no need to destory or free it's returned to a queue and reused*/

    /* call callback fundtion */
    (smart_dump_op->completion_function) (context);

    return status;
}

/*!***************************************************************
 * @fn fbe_api_physical_drive_send_download_asynch()
 ****************************************************************
 * @brief
 *  This function sends the DOWNLOAD request to PDO.
 *
 * @param object_id - object ID
 * @param fw_info - pointer to download context
 *
 * @return
 *  fbe_status_t - status
 *
 * @version
 *  11/23/2011 - Created. chenl6
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_physical_drive_send_download_asynch(fbe_object_id_t                       object_id,
                                            fbe_physical_drive_fw_download_asynch_t * fw_info)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                      packet = NULL;
    fbe_physical_drive_mgmt_fw_download_t * download_info = &(fw_info->download_info);

    /* Allocate packet.*/
    packet = fbe_api_get_contiguous_packet();/*no need to touch or initialize, it's taken from a pool and taken care of*/
    if (packet == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet_with_sg_list_async(packet,
                                                                   FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD,
                                                                   download_info,
                                                                   sizeof(fbe_physical_drive_mgmt_fw_download_t),                                           
                                                                   object_id, 
                                                                   FBE_PACKET_FLAG_NO_ATTRIB,
                                                                   fw_info->download_sg_list, 
                                                                   1,
                                                                   fbe_api_physical_drive_send_download_async_callback, 
                                                                   fw_info,
                                                                   FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
        /* Only trace when it is an error we do not expect.*/
         fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s error in sending packet, status:%d\n", __FUNCTION__, status);  
    }
    
    return FBE_STATUS_OK;    
}

/*!***************************************************************
 * @fn fbe_api_physical_drive_send_download_async_callback()
 ****************************************************************
 * @brief
 *  This function processes the callback of async DOWNLOAD request.
 *
 * @param packet - pinter to packet
 * @param context - pointer to context
 *
 * @return
 *  fbe_status_t - status
 *
 * @version
 *  11/23/2011 - Created. chenl6
 ****************************************************************/
static fbe_status_t 
fbe_api_physical_drive_send_download_async_callback (fbe_packet_t * packet, 
                                                     fbe_packet_completion_context_t context)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_physical_drive_fw_download_asynch_t *   fw_info = 
            (fbe_physical_drive_fw_download_asynch_t *)context;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *       control_operation = NULL;
    fbe_payload_control_status_t            control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_payload_control_status_qualifier_t  control_status_qualifier = 0;

    status = fbe_transport_get_status_code(packet);
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation, &control_status_qualifier);

    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:status:%d\n", __FUNCTION__, status);
        fw_info->completion_status = status;
    }
    else if (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:control status:%d\n", __FUNCTION__, control_status);
        fw_info->completion_status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        fw_info->completion_status = FBE_STATUS_OK;
    }

    /* clean up */
    fbe_api_return_contiguous_packet(packet);/*no need to destroy of free, it's part of a queue and taken care off*/

    /* call callback function */
    (fw_info->completion_function) (context);
    
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_api_physical_drive_abort_download()
 ****************************************************************
 * @brief
 *  This function sends abort DOWNLOAD request to PDO.
 *
 * @param object_id - object ID
 *
 * @return
 *  fbe_status_t - status
 *
 * @version
 *  11/23/2011 - Created. chenl6
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_physical_drive_abort_download(fbe_object_id_t object_id)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t  status_info;

    /* send the control packet and wait for its completion. */
    status = fbe_api_common_send_control_packet(FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_ABORT,
                                                NULL,
                                                0,
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_PHYSICAL);
    
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    
    return FBE_STATUS_OK;    
}

/*!***************************************************************
 * @fn fbe_api_physical_drive_set_service_time
 *****************************************************************
 * @brief
 *  This function sets the service time of a given object.
 *
 * @param object_id - object id of physical drive
 * @param service_time_ms - New service time for pdo.
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  5/23/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_set_service_time(fbe_object_id_t  object_id,
                                                                  fbe_time_t service_time_ms)
{   
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_physical_drive_control_set_service_time_t set_service_time;

    set_service_time.service_time_ms = service_time_ms;

    status = fbe_api_common_send_control_packet(FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_SERVICE_TIME,
                                                &set_service_time,
                                                sizeof(fbe_physical_drive_control_set_service_time_t),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/**************************************
 * end fbe_api_physical_drive_set_service_time()
 **************************************/

/*!***************************************************************
 * @fn fbe_api_physical_drive_get_service_time
 *****************************************************************
 * @brief
 *  This function gets the service time of a given object.  There
 *  are multiple service time timeouts based on the command, if
 *  the client has modified the service time etc:
 *      o Current service time - client may change the service time
 *      o Default service time - Default I/O service time in ms
 *      o EMEH Service time - The increased service time when EMEH is enabled
 *      o ReMap Serviec time - The increase service time during remap
 *
 * @param object_id - object id of physical drive
 * @param service_time_ms - New service time for pdo.
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  06/22/2015  Ron Proulx  - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_service_time(fbe_object_id_t  object_id,
                                                                  fbe_physical_drive_control_get_service_time_t *get_service_time_p)
{   
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_physical_drive_control_get_service_time_t get_service_time;

    fbe_zero_memory(&get_service_time, sizeof(fbe_physical_drive_control_get_service_time_t));
    fbe_zero_memory(get_service_time_p, sizeof(fbe_physical_drive_control_get_service_time_t));
    status = fbe_api_common_send_control_packet(FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_SERVICE_TIME,
                                                &get_service_time,
                                                sizeof(fbe_physical_drive_control_get_service_time_t),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }
    get_service_time_p->current_service_time_ms = get_service_time.current_service_time_ms;
    get_service_time_p->default_service_time_ms = get_service_time.default_service_time_ms;
    get_service_time_p->emeh_service_time_ms = get_service_time.emeh_service_time_ms;
    get_service_time_p->remap_service_time_ms = get_service_time.remap_service_time_ms;

    return status;
}
/**************************************
 * end fbe_api_physical_drive_get_service_time()
 **************************************/

fbe_status_t FBE_API_CALL 
fbe_api_physical_drive_get_outstanding_io_count(fbe_object_id_t  object_id, fbe_u32_t * outstanding_io_count)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_physical_drive_mgmt_qdepth_t                get_depth;

    if (outstanding_io_count == NULL || object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    get_depth.outstanding_io_count = 0;

    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_QDEPTH,
                                                 &get_depth,
                                                 sizeof(fbe_physical_drive_mgmt_qdepth_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    * outstanding_io_count = get_depth.outstanding_io_count;

    return status;

}

/*!***************************************************************
 * @fn fbe_api_physical_drive_clear_eol(
 *    fbe_object_id_t object_id, 
 *    fbe_packet_attr_t attribute)
 *****************************************************************
 * @brief
 *  This function clears the EOL for the give physical drive
 *
 * @param object_id  - The physical drive object id to send request to
 * @param attribute - attribute
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  03/27/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_clear_eol(fbe_object_id_t  object_id, fbe_packet_attr_t attribute)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t     status_info;

    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_CLEAR_EOL,
                                                 NULL,
                                                 0,
                                                 object_id,
                                                 attribute,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/*****************************************
 * end fbe_api_physical_drive_clear_eol()
 *****************************************/

/*!***************************************************************
 * @fn fbe_api_physical_drive_update_drive_fault(
 *    fbe_object_id_t object_id, 
 *    fbe_u8_t flag)
 *****************************************************************
 * @brief
 *  This function sets the drive fault for the give physical drive
 *
 * @param object_id  - The physical drive object id to send request to
 * @param b_set_clear_drive_fault - set or clear the drive fault bit
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *   07/11/2012 - Created. kothal 
 *
 ****************************************************************/
fbe_status_t fbe_api_physical_drive_update_drive_fault(fbe_object_id_t object_id, fbe_bool_t b_set_clear_drive_fault)
{

    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_bool_t                              b_update_set_or_clear_drive_fault = b_set_clear_drive_fault;
    
    
    status = fbe_api_common_send_control_packet(FBE_PHYSICAL_DRIVE_CONTROL_CODE_UPDATE_DRIVE_FAULT,
                                                &b_update_set_or_clear_drive_fault,
                                                sizeof(fbe_bool_t),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet:error:%d qual:%d; payload:error:%d, qual:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!***************************************************************
 * @fn fbe_api_physical_drive_update_link_fault(
 *    fbe_object_id_t object_id, 
 *    fbe_u8_t flag)
 *****************************************************************
 * @brief
 *  This function updates the link fault for the give physical drive
 *
 * @param object_id  - The physical drive object id to send request to
 * @param b_set_clear_link_fault - set or clear the link fault bit
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *   07/11/2012 - Created. kothal 
 *
 ****************************************************************/
fbe_status_t fbe_api_physical_drive_update_link_fault(fbe_object_id_t object_id, fbe_bool_t b_set_clear_link_fault)
{

    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_bool_t                              b_update_set_or_clear_link_fault = b_set_clear_link_fault;
    
    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_UPDATE_LINK_FAULT,
                                                 &b_update_set_or_clear_link_fault,
                                                 sizeof (fbe_bool_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet:error:%d qual:%d; payload:error:%d, qual:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!***************************************************************
 * @fn fbe_api_physical_drive_set_crc_action(
 *    fbe_object_id_t object_id, 
 *    fbe_packet_attr_t attribute)
 *****************************************************************
 * @brief
 *  This function set the PDO action for logical crc errors it
 *  gets from RAID.
 *
 * @param object_id  - The physical drive object id to send request to
 * @param attribute - attribute
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  05/07/2012  Wayne Garrett  - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_set_crc_action(fbe_object_id_t  object_id, fbe_pdo_logical_error_action_t action)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_physical_drive_control_crc_action_t     crc_action;

    crc_action.action = action;
    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_CRC_ACTION,
                                                 &crc_action,
                                                 sizeof(crc_action),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/*****************************************
 * end fbe_api_physical_drive_set_crc_action()
 *****************************************/


/*!***************************************************************
 * @fn fbe_api_physical_drive_send_logical_error(
 *    fbe_block_transport_control_logical_error_t *error)
 *****************************************************************
 * @brief
 *  Notifies the Physical Drive of a logical error
 *
 * @param object_id  - The physical drive object id to send request to
 * @param error - the logical error
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  05/14/2012  Wayne Garrett  - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_send_logical_error(fbe_object_id_t object_id, fbe_block_transport_error_type_t error)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_block_transport_control_logical_error_t logical_error;

    //logical_error.pdo_object_id = object_id;
    logical_error.error_type = error;

    status = fbe_api_common_send_control_packet (FBE_BLOCK_TRANSPORT_CONTROL_CODE_UPDATE_LOGICAL_ERROR_STATS,
                                                 &logical_error,
                                                 sizeof(logical_error),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_physical_drive_get_logical_drive_state(
 * fbe_object_id_t  object_id, fbe_u8_t *logical_state)
 *****************************************************************
 * @brief
 *  This function gets the logical state of the drive.
 *
 * @param object_id  -  The physical drive object id 
 * @param logical state - pointer to return logical drive state info 
 *
 * @return fbe_status_t - success or failure
 *
 * @version   08/02/2012 kothal  - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_logical_drive_state(fbe_object_id_t  object_id, 
                                                                        fbe_block_transport_logical_state_t *logical_state) 
{
    fbe_status_t                                    status = FBE_STATUS_OK;   
    fbe_api_control_operation_status_info_t         status_info;

    if (object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }    
    
    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_LOGICAL_STATE,
                                                 logical_state,
                                                 sizeof(fbe_block_transport_logical_state_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {       
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet:error:%d qual:%d; payload:error:%d, qual:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }   

    return status;

}

fbe_status_t FBE_API_CALL 
fbe_api_physical_drive_set_object_io_throttle_MB(fbe_object_id_t  object_id, fbe_u32_t io_throttle_MB)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_physical_drive_mgmt_qdepth_t            set_depth;

    fbe_zero_memory(&set_depth, sizeof(fbe_physical_drive_mgmt_qdepth_t));

    set_depth.io_throttle_max = io_throttle_MB * 0x800; /* Convert MB to blocks */

    /*pay attention we would set teh FBE_PACKET_FLAG_TRAVERSE because all times the ioctls are actually ment to the logical drive*/
    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_IO_THROTTLE,
                                                 &set_depth,
                                                 sizeof(fbe_physical_drive_mgmt_qdepth_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_physical_drive_set_enhanced_queuing_timer()
 *****************************************************************
 * @brief
 *  This function sets enhanced queuing timer for a drive
 *
 * @param object_id  - The physical drive object id to send request to
 * @param lpq_timer  - lpq timer in ms
 * @param hpq_timer  - hpq timer in ms
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  08/22/2012  Lili Chen  - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_set_enhanced_queuing_timer(fbe_object_id_t object_id, fbe_u32_t lpq_timer, fbe_u32_t hpq_timer)
{

    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_physical_drive_mgmt_set_enhanced_queuing_timer_t set_timer;
    fbe_api_control_operation_status_info_t     status_info;
    
    set_timer.lpq_timer_ms = lpq_timer;   
    set_timer.hpq_timer_ms = hpq_timer;   
    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_ENHANCED_QUEUING_TIMER,
                                                  &set_timer,
                                                  sizeof(fbe_physical_drive_mgmt_set_enhanced_queuing_timer_t),
                                                  object_id,
                                                  FBE_PACKET_FLAG_NO_ATTRIB,
                                                  &status_info,
                                                  FBE_PACKAGE_ID_PHYSICAL);

     if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:pkt err:%d, pkt qual:%d, pyld err:%d, pyld qual:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/*****************************************
 * end fbe_api_physical_drive_set_enhanced_queuing_timer()
 *****************************************/

/*!***************************************************************
 * @fn fbe_api_physical_drive_get_rla_abort_required()
 *****************************************************************
 * @brief
 *  This function sends a command to get whether or not Read Look Ahead (RLA) abort is required.
 *
 * @param object_id  - PDO's ID on which to get whether or not RLA abort required.
 * @param b_is_required_p - pointer to a boolean indicating whether or not RLA abort is required.
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *    07/01/2014 - Created. Darren Insko
 *    
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_rla_abort_required(fbe_object_id_t object_id,
                                                                        fbe_bool_t *b_is_required_p)
{

    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t     status_info;
 
    if (object_id >= FBE_OBJECT_ID_INVALID || b_is_required_p == NULL)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: improper function parameter\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_RLA_ABORT_REQUIRED,
                                                 b_is_required_p,
                                                 sizeof(fbe_bool_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d\n", __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/*****************************************************
 * end fbe_api_physical_drive_get_rla_abort_required()
 *****************************************************/

/*!***************************************************************
 * @fn fbe_api_physical_drive_get_attributes(
 *     fbe_object_id_t object_id, 
 *     fbe_logical_drive_attributes_t * const attributes_p) 
 *****************************************************************
 * @brief
 *  This gets the attributes of the physical drive.  If a
 *  maintenance mode bit is set it's the callers responsiblity to not
 *  trust the attributes until maintenance issue is resolved.
 *
 * @param object_id - object ID
 * @param attributes_p - pointer to physical drive attributes
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_attributes(fbe_object_id_t object_id, 
                                                               fbe_physical_drive_attributes_t * const attributes_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t     status_info;

    if (attributes_p == NULL){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: input buffer is NULL\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_ATTRIBUTES,
                                                 attributes_p,
                                                 sizeof(fbe_physical_drive_attributes_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!********************************************************************
 *  fbe_api_physical_drive_get_phy_info ()
 *********************************************************************
 *
 *  Description: get the drive phy info. This command will traverse
 *  and finally gets satisfied in the enclosure object.
 *
 *  Inputs: object_id - PDO object id.
 *          drive_phy_info_p - The pointer to fbe_drive_phy_info_t
 *
 *  Return Value: success or failure
 *
 *  History:
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_phy_info(fbe_object_id_t object_id, 
                                                fbe_drive_phy_info_t * drive_phy_info_p)
{
    fbe_status_t    							status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t		status_info;

    if (drive_phy_info_p == NULL){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: input buffer is NULL\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    status = fbe_api_common_send_control_packet (FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_GET_DRIVE_PHY_INFO,
                                                 drive_phy_info_p,
                                                 sizeof(fbe_drive_phy_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_TRAVERSE,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}


/*!***************************************************************
 * @fn fbe_api_physical_drive_get_drive_block_size(
 *     const fbe_object_id_t object_id,
 *     fbe_block_transport_negotiate_t *const negotiate_p,
 *     fbe_payload_block_operation_status_t *const block_status_p,
 *     fbe_payload_block_operation_qualifier_t *const block_qualifier) 
 *****************************************************************
 * @brief
 *  This function sends a syncronous mgmt packet to get block size (i.e.
 *  negotiate) the block size.
 *
 * @param object_id        - The PDO drive object id to send request to
 * @param negotiate_p      - pointer to the negotiate date
 * @param block_status_p   - Pointer to the block status
 * @param block_qualifier  - Pointer to the block qualifier
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *
 ****************************************************************/
 fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_drive_block_size(const fbe_object_id_t object_id,
                                                         fbe_block_transport_negotiate_t *const negotiate_p,
                                                         fbe_payload_block_operation_status_t *const block_status_p,
                                                         fbe_payload_block_operation_qualifier_t *const block_qualifier)
 {
    fbe_packet_t                           *packet_p = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_semaphore_t                         sem;
    fbe_payload_ex_t *                         payload_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_sg_element_t                       *sg_p = NULL;
    fbe_cpu_id_t                            cpu_id;

    /* Validate parameters. The valid range of object ids is greater than
     * FBE_OBJECT_ID_INVALID and less than FBE_MAX_PHYSICAL_OBJECTS. 
     * (we don't validate the negotiate_p, the object should do that.
     */
    if ((object_id == FBE_OBJECT_ID_INVALID) || (object_id > FBE_MAX_PHYSICAL_OBJECTS)){
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*we will set a sempahore to block until the packet comes back,
    set up the packet with the correct opcode and then send it*/
    packet_p = fbe_api_get_contiguous_packet();
    if (packet_p == NULL){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Unable to allocate memory for packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_semaphore_init(&sem, 0, 1);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_allocate_block_operation(payload_p);

    /* Setup the sg using the internal packet sgs.
     * This creates an sg list using the negotiate information passed in. 
     */
    fbe_payload_ex_get_pre_sg_list(payload_p, &sg_p);
    fbe_payload_ex_set_sg_list(payload_p, sg_p, 0);
    fbe_sg_element_init(sg_p, sizeof(fbe_block_transport_negotiate_t), negotiate_p);
    fbe_sg_element_increment(&sg_p);
    fbe_sg_element_terminate(sg_p);

    status =  fbe_payload_block_build_negotiate_block_size(block_operation_p);
    fbe_payload_ex_increment_block_operation_level(payload_p);
    fbe_transport_set_completion_function(packet_p, 
                                          fbe_api_physical_drive_interface_generic_completion, 
                                          &sem);

    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_PHYSICAL,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id); 

    cpu_id = fbe_get_cpu_id();
    fbe_transport_set_cpu_id(packet_p, cpu_id);
    /*the send will be implemented differently in user and kernel space*/
    fbe_api_common_send_io_packet_to_driver(packet_p);
    
    /*we block here and wait for the callback function to take the semaphore down*/
    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

    /*check the packet status to make sure we have no errors*/
    status = fbe_transport_get_status_code (packet_p);
    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Packet request failed with status %d \n", __FUNCTION__, status); 
    }

    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    if (block_status_p != NULL) {
        fbe_payload_block_get_status(block_operation_p, block_status_p);
    }

    if (block_qualifier != NULL) {
        fbe_payload_block_get_qualifier(block_operation_p, block_qualifier);
    }
    fbe_api_return_contiguous_packet(packet_p);
    
    return status;
}

 /*!***************************************************************
 * @fn fbe_api_physical_drive_get_id_by_serial_number(
 *     const fbe_u8_t *sn, 
 *     fbe_u32_t sn_array_size, 
 *     fbe_object_id_t *object_id) 
 *****************************************************************
 * @brief
 *  This returns the object id of a physical drive with the requested serial number.
 *
 * @param sn            - pointer to the serial number
 * @param sn_array_size - serial number's array length
 * @param object_id - pointer to object ID
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_id_by_serial_number(const fbe_u8_t *sn, fbe_u32_t sn_array_size, fbe_object_id_t *object_id)
{
    fbe_topology_control_get_physical_drive_by_sn_t     get_id_by_sn;
    fbe_status_t                                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t             status_info;                        

    if (sn_array_size > FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, user passed illegal SN array size:%d\n", __FUNCTION__, sn_array_size);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (sn == NULL || object_id == NULL) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, NULL pointers passed in\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(&get_id_by_sn.product_serial_num, sn, sn_array_size);
    get_id_by_sn.sn_size = sn_array_size;

    status = fbe_api_common_send_control_packet_to_service (FBE_TOPOLOGY_CONTROL_CODE_GET_PHYSICAL_DRIVE_BY_SERIAL_NUMBER,
                                                             &get_id_by_sn,
                                                             sizeof(fbe_topology_control_get_physical_drive_by_sn_t),
                                                             FBE_SERVICE_ID_TOPOLOGY,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *object_id = get_id_by_sn.object_id;

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_api_physical_drive_set_pvd_eol
 *****************************************************************
 * @brief
 *  This sets the EOL bit on the PVD edge
 *
 * @param object_id - object ID
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_set_pvd_eol(fbe_object_id_t object_id)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t     status_info;

    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_END_OF_LIFE,
                                                 NULL,
                                                 0,
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}


/*!***************************************************************
 * fbe_api_logical_drive_interface_read_ica_stamp(fbe_object_id_t object_id, fbe_imaging_flags_t *ica_stamp)
 ****************************************************************
 * @brief
 *  This function reada the ICA stamp from the requested drive
 *
 * @param object_id - The object_id to stamp.
 * @param ica_stamp - memory to read into
 * @return 
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_interface_read_ica_stamp(fbe_object_id_t object_id, fbe_imaging_flags_t *ica_stamp)
{
    fbe_status_t 							status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *							packet = NULL;
    fbe_payload_ex_t *                     	payload = NULL;
    fbe_payload_control_operation_t *   	control_operation = NULL;
    fbe_semaphore_t							sem;
    fbe_payload_control_status_t			control_status;
    fbe_physical_drive_read_ica_stamp_t	*	imaging_flags = NULL;

    if (object_id == FBE_OBJECT_ID_INVALID) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, object id is FBE_OBJECT_ID_INVALID\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    imaging_flags = (fbe_physical_drive_read_ica_stamp_t *)fbe_api_allocate_contiguous_memory(sizeof(fbe_physical_drive_read_ica_stamp_t));
    if (imaging_flags == NULL) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to allocate memory to read in\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    packet = fbe_api_get_contiguous_packet();
    if (packet == NULL) {
        fbe_api_free_contiguous_memory(imaging_flags);
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to allocate packet\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_semaphore_init(&sem, 0, 1);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

    fbe_payload_control_build_operation (control_operation,
                                         FBE_PHYSICAL_DRIVE_CONTROL_READ_ICA_STAMP,
                                         imaging_flags,
                                         sizeof(fbe_physical_drive_read_ica_stamp_t));

    fbe_api_common_send_control_packet_asynch(packet,
                                              object_id,
                                              fbe_api_physical_drive_interface_generic_completion,
                                              &sem,
                                              FBE_PACKET_FLAG_NO_ATTRIB,
                                              FBE_PACKAGE_ID_PHYSICAL);

    status = fbe_semaphore_wait_ms(&sem, 30000);
    if (status != FBE_STATUS_OK) {
        fbe_semaphore_destroy(&sem);
        fbe_api_return_contiguous_packet(packet);/*no need to destory !*/
        fbe_api_free_contiguous_memory(imaging_flags);
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, timeout waiting for ICA stamp IO on object id:%d\n", __FUNCTION__, object_id);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_transport_get_status_code(packet);
    fbe_payload_control_get_status(control_operation, &control_status);

    if (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK || status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, error returned from ICA stamp on object id:%d\n", __FUNCTION__, object_id);
        status  = FBE_STATUS_GENERIC_FAILURE;
    }else{
        fbe_copy_memory(ica_stamp, imaging_flags, sizeof(fbe_imaging_flags_t));/*copy just what the user wants*/
    }

    fbe_payload_ex_release_control_operation(payload, control_operation);
    
    fbe_semaphore_destroy(&sem);
    fbe_api_return_contiguous_packet(packet);/*no need to destory !*/
    fbe_api_free_contiguous_memory(imaging_flags);

    return status;

}


/*!***************************************************************
 * fbe_api_physical_drive_interface_generate_ica_stamp(fbe_object_id_t object_id)
 ****************************************************************
 * @brief
 *  This function generates teh ICA stamp on the requested drive
 *
 * @param object_id - The object_id to stamp.
 * @return 
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_physical_drive_interface_generate_ica_stamp(fbe_object_id_t object_id)
{
    fbe_status_t 							status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *							packet = NULL;
    fbe_payload_ex_t *                     	payload = NULL;
    fbe_payload_control_operation_t *   	control_operation = NULL;
    fbe_semaphore_t							sem;
    fbe_payload_control_status_t			control_status;

    if (object_id == FBE_OBJECT_ID_INVALID) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, object id is FBE_OBJECT_ID_INVALID\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    packet = fbe_api_get_contiguous_packet();
    if (packet == NULL) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to allocate packet\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_semaphore_init(&sem, 0, 1);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

    fbe_payload_control_build_operation (control_operation,
                                         FBE_PHYSICAL_DRIVE_CONTROL_GENERATE_ICA_STAMP,
                                         NULL,
                                         0);

    fbe_api_common_send_control_packet_asynch(packet,
                                              object_id,
                                              fbe_api_physical_drive_interface_generic_completion,
                                              &sem,
                                              FBE_PACKET_FLAG_NO_ATTRIB,
                                              FBE_PACKAGE_ID_PHYSICAL);

    status = fbe_semaphore_wait_ms(&sem, 30000);
    if (status != FBE_STATUS_OK) {
        fbe_semaphore_destroy(&sem);
        fbe_api_return_contiguous_packet(packet);/*no need to destory !*/
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, timeout waiting for ICA stamp IO on object id:%d\n", __FUNCTION__, object_id);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_transport_get_status_code(packet);
    fbe_payload_control_get_status(control_operation, &control_status);

    if (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK || status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, error returned from ICA stamp on object id:%d\n", __FUNCTION__, object_id);
        status  = FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_ex_release_control_operation(payload, control_operation);
    
    fbe_semaphore_destroy(&sem);
    fbe_api_return_contiguous_packet(packet);/*no need to destory !*/

    return status;
}

static fbe_status_t  fbe_api_physical_drive_interface_generic_completion(fbe_packet_t * packet, 
                                                                         fbe_packet_completion_context_t context)
{
    fbe_semaphore_t *		sem = (fbe_semaphore_t *)context;
    fbe_semaphore_release(sem, 0, 1, FBE_FALSE);
    return FBE_STATUS_OK;
}

/*********************************************************************
 * @fn fbe_api_physical_drive_get_bms_op_asynch()
 *********************************************************************
 * @brief:
 *      Async fuel gauge command.
 *
 *  @param object_id - The object id to send request to
 *  @param bms_op_p - pointer to the information passed from the the client
 *
 *  @return fbe_status_t, success or failure
 *
 *  History:
 *    19-Nov-2013 - created
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_bms_op_asynch(fbe_object_id_t object_id, 
                                           fbe_physical_drive_bms_op_asynch_t * bms_op_p)                                           
{

    fbe_status_t        status = FBE_STATUS_OK;    
    fbe_u32_t           sg_item = 0;
    fbe_packet_t * packet = NULL;

    if (bms_op_p == NULL || object_id >= FBE_OBJECT_ID_INVALID) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: ObjID Invalid or bms op NULL\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }  

    if (bms_op_p->completion_function == NULL || bms_op_p->response_buf == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Must have a valid completion function and response buf\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // set next sg element size and address for the response buffer. 
    fbe_sg_element_init(&(bms_op_p->sg_element[sg_item]), 
                        bms_op_p->get_log_page.transfer_count, 
                        bms_op_p->response_buf);
    sg_item++;
    fbe_sg_element_terminate(&bms_op_p->sg_element[sg_item]);
    sg_item++;

    packet = fbe_api_get_contiguous_packet();//no need to touch or initialize, it's taken from a pool and taken care of
    if (packet == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    status = fbe_api_common_send_control_packet_with_sg_list_async(packet,
                                                                   FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_LOG_PAGES,
                                                                   &bms_op_p->get_log_page,
                                                                   sizeof(fbe_physical_drive_mgmt_get_log_page_t),                                           
                                                                   object_id, 
                                                                   FBE_PACKET_FLAG_NO_ATTRIB,
                                                                   bms_op_p->sg_element, 
                                                                   sg_item,
                                                                   fbe_api_physical_drive_get_bms_op_asynch_callback, 
                                                                   bms_op_p,
                                                                   FBE_PACKAGE_ID_PHYSICAL);


    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
        /* Only trace when it is an error we do not expect.*/
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s failed, status: 0x%x.\n", __FUNCTION__, status); 
        return status;
    }
    
    return FBE_STATUS_OK;
} // end fbe_api_physical_drive_get_bms_op_asynch

/*********************************************************************
 * @fn fbe_api_physical_drive_get_bms_op_asynch_callback()
 *********************************************************************
 * @brief:
 *      BMS op asynch callback
 *
 *  @param packet - pointer to fbe_packet_t.
 *  @param context - packet completion context
 *
 *  @return fbe_status_t, success or failure
 *
 *  History:
 *    19-Nov-2013: created
 *********************************************************************/
static fbe_status_t fbe_api_physical_drive_get_bms_op_asynch_callback (fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_physical_drive_bms_op_asynch_t *bms_op_p = (fbe_physical_drive_bms_op_asynch_t *)context;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_payload_control_status_qualifier_t control_status_qualifier = 0;

    status = fbe_transport_get_status_code(packet);
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation, &control_status_qualifier);

    if ((status != FBE_STATUS_OK) || 
        (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:status:%d, payload status:%d & qulf:%d, page 0x%X\n", __FUNCTION__,
                       status, control_status, control_status_qualifier, bms_op_p->get_log_page.page_code);

        status = FBE_STATUS_COMPONENT_NOT_FOUND;
    }          

    // Fill the status
    bms_op_p->status = status;

    fbe_api_return_contiguous_packet(packet);//no need to destory or free it's returned to a queue and reused

    // call callback fundtion
    (bms_op_p->completion_function) (context);

    return status;

} // end of fbe_api_physical_drive_get_bms_op_asynch_callback


/*!***************************************************************
   @fn fbe_status_t FBE_API_CALL fbe_api_physical_drive_sanitize() 
 ****************************************************************
 * @brief
 *  This function sends drive sanitie command to PDO.
 *
 *  @param object_id - The object id to send request to
 *  @param pattern - pattern to sanitize with
 *
 * @return
 *  fbe_status_t
 *
 *  History:
 *    7/2/2014: Wayne Garrett - created
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_sanitize(fbe_object_id_t object_id, 
                                                          fbe_scsi_sanitize_pattern_t pattern)                                           
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_payload_control_operation_opcode_t sanitize_opcode = FBE_PHYSICAL_DRIVE_CONTROL_CODE_INVALID;

    if (object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }  

    switch (pattern) 
    {
        case FBE_SCSI_FORMAT_UNIT_SANITIZE_PATTERN_ERASE_ONLY:
            sanitize_opcode = FBE_PHYSICAL_DRIVE_CONTROL_CODE_SANITIZE_DRIVE_ERASE_ONLY;
            break;

        case FBE_SCSI_FORMAT_UNIT_SANITIZE_PATTERN_OVERWRITE_AND_ERASE:
            sanitize_opcode = FBE_PHYSICAL_DRIVE_CONTROL_CODE_SANITIZE_DRIVE_OVERWRITE_AND_ERASE;
            break;

        case FBE_SCSI_FORMAT_UNIT_SANITIZE_PATTERN_AFSSI:
            sanitize_opcode = FBE_PHYSICAL_DRIVE_CONTROL_CODE_SANITIZE_DRIVE_AFSSI;
            break;

        case FBE_SCSI_FORMAT_UNIT_SANITIZE_PATTERN_NSA:
            sanitize_opcode = FBE_PHYSICAL_DRIVE_CONTROL_CODE_SANITIZE_DRIVE_NSA;
            break;

        default:
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, invalid pattern code %d \n", __FUNCTION__, pattern);
            return FBE_STATUS_GENERIC_FAILURE;
    }


     status = fbe_api_common_send_control_packet (sanitize_opcode,
                                                  NULL,
                                                  0,
                                                  object_id,
                                                  FBE_PACKET_FLAG_NO_ATTRIB,
                                                  &status_info,
                                                  FBE_PACKAGE_ID_PHYSICAL);

     if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet err:%d/%d, payload err%d/%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}



fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_sanitize_status(fbe_object_id_t object_id, fbe_physical_drive_sanitize_info_t *sanitize_status_p)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t     status_info;    

    if (object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }  


     status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_SANITIZE_STATUS,
                                                  sanitize_status_p,
                                                  sizeof(*sanitize_status_p),
                                                  object_id,
                                                  FBE_PACKET_FLAG_NO_ATTRIB,
                                                  &status_info,
                                                  FBE_PACKAGE_ID_PHYSICAL);

     if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet err:%d/%d, payload err%d/%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}



/*!***************************************************************
   @fn fbe_status_t FBE_API_CALL fbe_api_physical_drive_format_block_size() 
 ****************************************************************
 * @brief
 *  Reformats block size for a specified disk (pdo object id)
 *
 *  @param object_id - The object id to send request to
 *  @param block_size - block size (4160 or 520)
 *
 * @return
 *  fbe_status_t
 *
 *  History:
 *    8/22/2014: Wayne Garrett - created
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_physical_drive_format_block_size(fbe_object_id_t object_id, 
                                                                   fbe_u32_t block_size)                                           
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_physical_drive_set_format_block_size_t set_block_size = block_size;

    if (object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }  

    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_FORMAT_BLOCK_SIZE,
                                                 &set_block_size,
                                                 sizeof(set_block_size),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

     if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet err:%d/%d, payload err%d/%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}


fbe_status_t FBE_API_CALL fbe_api_physical_drive_set_dieh_media_threshold(fbe_object_id_t object_id, fbe_physical_drive_set_dieh_media_threshold_t *threshold_p)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t     status_info;

    if (object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }  

    status = fbe_api_common_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_DIEH_MEDIA_THRESHOLD,
                                                 threshold_p,
                                                 sizeof(*threshold_p),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

     if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet err:%d/%d, payload err%d/%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}


/*********************************************
 * end file fbe_api_physical_drive_interface.c
 *********************************************/
