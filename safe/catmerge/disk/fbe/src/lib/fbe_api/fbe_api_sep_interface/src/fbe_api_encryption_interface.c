/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_encryption_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the fbe_api_encryption_interface for the storage extent package.
 *
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * @ingroup fbe_api_encryption_interface
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_encryption_interface.h"
#include "fbe/fbe_base_config.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_common_job_notification.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_resume_prom_interface.h"
#include "generic_utils_lib.h"  /*getResumeFieldOffset*/
#include "fbe/fbe_api_board_interface.h"

#define FBE_API_ENCRYPTION_JOB_WAIT_TIMEOUT	30000 

/*!***************************************************************
 * @fn fbe_api_set_system_encryption_info(fbe_system_encryption_info_t* encryption_info_p)
 ****************************************************************
 * @brief
 *  This function sets encryption information related information from
 *  the system
 *
 * @param encryption_info_p - pointer to the strucutre to get encryption information
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL  fbe_api_set_system_encryption_info(fbe_system_encryption_info_t *encryption_info_p)
{
    fbe_status_t                                   		 	status;
    fbe_api_job_service_change_system_encryption_info_t		system_encryption_change_job;
    fbe_job_service_error_type_t							error_type;
    fbe_status_t											job_status;

    if (encryption_info_p == NULL){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL buffer pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(&system_encryption_change_job.system_encryption_info, encryption_info_p, sizeof(fbe_system_encryption_info_t));

    /*send the command that will eventually start the job*/
    status = fbe_api_job_service_set_system_encryption_info(&system_encryption_change_job);
    if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to start job\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /*Wait for the job to complete*/
    status = fbe_api_common_wait_for_job(system_encryption_change_job.job_number, FBE_API_ENCRYPTION_JOB_WAIT_TIMEOUT, &error_type, &job_status, NULL);
    if (status == FBE_STATUS_GENERIC_FAILURE || status == FBE_STATUS_TIMEOUT) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed waiting for job\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (job_status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s:job failed with error code:%d\n", __FUNCTION__, error_type);
    }
    
    return job_status;

} /* end of fbe_api_set_system_encryption_info()*/

fbe_status_t FBE_API_CALL  fbe_api_set_encryption_paused(fbe_bool_t pause)
{
    return fbe_api_job_service_set_encryption_paused(pause);
}

/*!***************************************************************
 * @fn fbe_api_get_system_encryption_info(fbe_system_encryption_info_t* encryption_info_p)
 ****************************************************************
 * @brief
 *  This function gets encryption information related information from
 *  the system
 *
 * @param encryption_info_p - pointer to the strucutre to get encryption information
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_system_encryption_info(fbe_system_encryption_info_t *encryption_info_p)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    if (encryption_info_p == NULL){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL buffer pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    status = fbe_api_common_send_control_packet_to_class (FBE_BASE_CONFIG_CONTROL_CODE_GET_SYSTEM_ENCRYPTION_INFO,
                                                          encryption_info_p,
                                                          sizeof(fbe_system_encryption_info_t),
                                                          FBE_CLASS_ID_BASE_CONFIG,
                                                          FBE_PACKET_FLAG_NO_ATTRIB,
                                                          &status_info,
                                                          FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;

} /* end of fbe_api_get_system_encryption_info() */

/*!***************************************************************
 * @fn fbe_api_enable_system_encryption(void)
 ****************************************************************
 * @brief
 *  This function enables the overall system encryption feature
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enable_system_encryption(fbe_u64_t stamp1, fbe_u64_t stamp2)
{
    fbe_status_t                                    status;
    fbe_system_encryption_info_t					encryption_info;

    status = fbe_api_get_system_encryption_info(&encryption_info);
    if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to get system info\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (encryption_info.encryption_mode == FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED) {
        return FBE_STATUS_NO_ACTION;
    }

    /* set encryption mode */
    encryption_info.encryption_mode   = FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED;
    encryption_info.encryption_stamp1 = stamp1;
    encryption_info.encryption_stamp2 = stamp2;
    
    status = fbe_api_set_system_encryption_info(&encryption_info);
    if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to set system info\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

} /* end of fbe_api_enable_system_encryption() */

/*!***************************************************************
 * @fn fbe_api_disable_system_encryption(void)
 ****************************************************************
 * @brief
 *  This function diasbales the overall system encryption feature
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_disable_system_encryption(void)
{
    fbe_status_t                                    status;
    fbe_system_encryption_info_t					encryption_info;

    status = fbe_api_get_system_encryption_info(&encryption_info);
    if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to get system info\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* set to be unencrypted mode */
    encryption_info.encryption_mode = FBE_SYSTEM_ENCRYPTION_MODE_UNENCRYPTED;
    
    status = fbe_api_set_system_encryption_info(&encryption_info);
    if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to set system info\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

} /* end of fbe_api_disable_system_encryption() */

/*!***************************************************************
 * @fn fbe_api_clear_system_encryption(void)
 ****************************************************************
 * @brief
 *  This function sets the overall system encryption feature to none
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_clear_system_encryption(void)
{
    fbe_status_t                                    status;
    fbe_system_encryption_info_t					encryption_info;

    status = fbe_api_get_system_encryption_info(&encryption_info);
    if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to get system info\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* set encryption mode to none */
    encryption_info.encryption_mode = FBE_SYSTEM_ENCRYPTION_MODE_INVALID;
    
    status = fbe_api_set_system_encryption_info(&encryption_info);
    if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to set system info\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

} /* end of fbe_api_clear_system_encryption() */


/*!***************************************************************
 * @fn fbe_api_change_rg_encryption_mode()
 ****************************************************************
 * @brief
 *  This function sets encryption information related information from
 *  the system
 *
 * @param object_id - rg to change
 * @param mode - new encryption mode.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 *
 ****************************************************************/
fbe_status_t fbe_api_change_rg_encryption_mode(fbe_object_id_t object_id,
                                               fbe_base_config_encryption_mode_t mode)
{
    fbe_status_t                                 status;
    fbe_api_job_service_update_encryption_mode_t update_encryption_mode_job;
    fbe_job_service_error_type_t				 error_type;
    fbe_status_t								 job_status;

    update_encryption_mode_job.object_id = object_id;
    update_encryption_mode_job.encryption_mode = mode;
    status = fbe_api_job_service_update_encryption_mode(&update_encryption_mode_job);
    if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to start job\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /*Wait for the job to complete*/
    status = fbe_api_common_wait_for_job(update_encryption_mode_job.job_number, FBE_API_ENCRYPTION_JOB_WAIT_TIMEOUT, &error_type, &job_status, NULL);
    if (status == FBE_STATUS_GENERIC_FAILURE || status == FBE_STATUS_TIMEOUT) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed waiting for job\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (job_status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s:job failed with error code:%d\n", __FUNCTION__, error_type);
    }
    
    return job_status;

} /* end of fbe_api_change_rg_encryption_mode()*/

/*!***************************************************************
 * @fn fbe_api_encryption_is_block_encrypted()
 ****************************************************************
 * @brief
 *  This function checks if particular block(s) are encrypted or not
 * on the given drive
 * 
 * @param bus - bus
 * @param encl - enclosure
 * @param slot - slot
 * @param pba - Address to check
 * @param blocks_to_check - Number of blocks to check
 * @param is_encrypted - OUTPUT - If the block(s) are encrypted or not
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @history
 *  11/04/13   - Ashok Tamilarasan - Created
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_encryption_check_block_encrypted(fbe_u32_t bus,
                                                                   fbe_u32_t enclosure,
                                                                   fbe_u32_t slot,
                                                                   fbe_lba_t pba,
                                                                   fbe_block_count_t blocks_to_check,
                                                                   fbe_block_count_t *num_encrypted)
{
    fbe_object_id_t pvd_object_id;
    fbe_provision_drive_validate_block_encryption_t block_encryption_info;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_status_t   status;

    status = fbe_api_provision_drive_get_obj_id_by_location(bus, enclosure, slot, &pvd_object_id);

    if (status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Unable to get PVD object ID\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* For now just restrict the validation to 100 blocks */
    if(blocks_to_check > 100) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Block count unexpected %d\n", __FUNCTION__, (fbe_u32_t)blocks_to_check);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    block_encryption_info.encrypted_pba = pba;
    block_encryption_info.num_blocks_to_check = blocks_to_check;

    status = fbe_api_common_send_control_packet (FBE_PROVISION_DRIVE_CONTROL_CODE_VALIDATE_BLOCK_ENCRYPTION,
                                                &block_encryption_info,
                                                sizeof(fbe_provision_drive_validate_block_encryption_t),
                                                pvd_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

       if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    *num_encrypted = block_encryption_info.blocks_encrypted;

    return status;

}

/*!***************************************************************
 * @fn fbe_api_process_encryption_keys(fbe_job_service_encryption_key_operation_t  operation, 
 *                                     fbe_encryption_key_table_t  *keys_p)
 ****************************************************************
 * @brief
 *  This function processes the following job:
 *  - FBE_JOB_SERVICE_SETUP_ENCRYPTION_KEYS
 *  - FBE_JOB_SERVICE_UPDATE_ENCRYPTION_KEYS
 *  - FBE_JOB_SERVICE_SETUP_ENCRYPTION_KEYS
 *
 * @param encryption_info_p - pointer to the strucutre to get encryption information
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @history
 *  19-Nov-13   - Created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_process_encryption_keys(fbe_job_service_encryption_key_operation_t in_operation, fbe_encryption_key_table_t *keys_p)
{
    fbe_status_t                                     status = FBE_STATUS_OK;
    fbe_status_t                                     job_status = FBE_STATUS_OK;
    fbe_job_service_error_type_t                     job_error_type = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    if (keys_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL buffer pointer\n", __FUNCTION__);

        return status;
    }

    switch (in_operation) 
    {
        case FBE_JOB_SERVICE_SETUP_ENCRYPTION_KEYS:
            /*send the command that will eventually start the job*/
            status = fbe_api_job_service_setup_encryption_keys(keys_p, &job_status, &job_error_type);
            if ((status != FBE_STATUS_OK) || (job_status != FBE_STATUS_OK))
            {
                fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to start job, j_status=0x%x, job_type=0x%x,\n", 
                              __FUNCTION__, job_status, job_error_type);
                return status;
            }
            break;

        case FBE_JOB_SERVICE_UPDATE_ENCRYPTION_KEYS:
            /* To be added later when fbe_api_job_service_delete_encryption_keys define */
            break;

        case FBE_JOB_SERVICE_DELETE_ENCRYPTION_KEYS:
            /* To be added later when fbe_api_job_service_delete_encryption_keys define */
            break;

        default:
            break;
        
    }
    
    return status;

} /* end of fbe_api_process_encryption_keys()*/

/*!***************************************************************
 * @fn fbe_api_get_hardware_ssv_data
 ****************************************************************
 * @brief
 *  This function collects ssv from the array
 *
 * @param ssv_p - pointer to the strucutre to get ssv
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @history
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_hardware_ssv_data(fbe_encryption_hardware_ssv_t *ssv_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_object_id_t board_object_id = FBE_OBJECT_ID_INVALID;
    
    /* First get the object id of the board */
    status = fbe_api_get_board_object_id(&board_object_id);

    if (board_object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(ssv_p, sizeof(fbe_encryption_hardware_ssv_t));

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_HARDWARE_SSV_DATA,
                                                 ssv_p,
                                                 sizeof(fbe_encryption_hardware_ssv_t),
                                                 board_object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {

        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_get_system_encryption_info(fbe_system_encryption_info_t* encryption_info_p)
 ****************************************************************
 * @brief
 *  This function gets pvd key information.
 *
 * @param encryption_info_p - pointer to the strucutre to get encryption information
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_pvd_key_info(fbe_object_id_t pvd_object_id,
                                                   fbe_provision_drive_get_key_info_t *key_info_p)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    if (key_info_p == NULL){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL buffer pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    status = fbe_api_common_send_control_packet(FBE_PROVISION_DRIVE_CONTROL_CODE_GET_DEK_INFO,
                                                key_info_p,
                                                sizeof(fbe_provision_drive_get_key_info_t),
                                                pvd_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;

}
