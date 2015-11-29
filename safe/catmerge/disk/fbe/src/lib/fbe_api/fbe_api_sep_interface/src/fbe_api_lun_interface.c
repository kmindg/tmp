/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_lun_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the fbe_lun interface for the storage extent package.
 *
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * @ingroup fbe_api_lun_interface
 *
 * @version
 *   11/12/2009:  Created. MEV
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_lun.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "fbe/fbe_base_config.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe_private_space_layout.h"

/*************************
 *   FORWARD DECLARATIONS
 *************************/
static void fbe_lun_info_get_status(fbe_api_lun_get_lun_info_t lun_info,
                                    fbe_u16_t num_data_disk,
                                    fbe_lba_t       rg_checkpoint, 
                                    fbe_api_lun_get_status_t *lun_status);

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

#define LU_WAIT_TIMEOUT 60000

typedef struct lun_job_wait_s{
	fbe_semaphore_t create_semaphore;
	fbe_object_id_t created_lun_id;
}lun_job_wait_t;


/*!***************************************************************************
 *          fbe_api_lun_initiate_verify()
 *****************************************************************************
 * @brief
 *  This function initiates a verify operation on a LUN.
 *
 * @param in_lun_object_id      - The LUN object ID
 * @param in_attribute          - packet attribute flag
 * @param in_verify_type        - type of verify (read-only, normal)
 * @param in_b_verify_entire_lun - FBE_TRUE - Verify the entire extent of the lun
 * @param in_start_lba          - Start lba (within lun extent) to start the
 *                                verify at.  FBE_LUN_VERIFY_START_LBA_LUN_START
 *                                Indicates the start of the lun extent
 * @param in_blocks             - Number of blocks to verify (maybe rounded up 
 *                                to element boundry). FBE_LUN_VERIFY_BLOCKS_ENTIRE_LUN
 *                                is a special value that indicates that the 
 *                                entire lun should be verified.
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  11/12/09 - Created. MEV
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lun_initiate_verify(
                            fbe_object_id_t             in_lun_object_id, 
                            fbe_packet_attr_t           in_attribute,
                            fbe_lun_verify_type_t       in_verify_type,
                            fbe_bool_t                  in_b_verify_entire_lun,
                            fbe_lba_t                   in_start_lba,
                            fbe_block_count_t           in_blocks)
{
    fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_lun_initiate_verify_t                   lun_initiate_verify;

    /*! Populate the initiate verify request.
     *
     *  @note We don't validate any parameters.
     */
    fbe_zero_memory((void *)&lun_initiate_verify, sizeof(fbe_lun_initiate_verify_t));
    lun_initiate_verify.verify_type = in_verify_type;
    lun_initiate_verify.b_verify_entire_lun = in_b_verify_entire_lun;
    if (in_b_verify_entire_lun == FBE_TRUE)
    {
        lun_initiate_verify.start_lba = FBE_LUN_VERIFY_START_LBA_LUN_START;
        lun_initiate_verify.block_count = FBE_LUN_VERIFY_BLOCKS_ENTIRE_LUN;
    }
    else
    {
        lun_initiate_verify.start_lba = in_start_lba;
        lun_initiate_verify.block_count = in_blocks;
    }

    status = fbe_api_common_send_control_packet (FBE_LUN_CONTROL_CODE_INITIATE_VERIFY,
                                                 &lun_initiate_verify, 
                                                 sizeof(fbe_lun_initiate_verify_t),
                                                 in_lun_object_id,
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
}   /*end fbe_api_lun_initiate_verify()*/


/*!***************************************************************************
 *          fbe_api_lun_initiate_zero()
 *****************************************************************************
 * @brief
 *  This function initiates a zero operation on a LUN.
 *
 * @param in_lun_object_id      - The LUN object ID
 *
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  1/5/12 - Created He Wei
 *  16/07/2012 - Modified by Jingcheng Zhang for force init zero
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lun_initiate_zero(fbe_object_id_t in_lun_object_id)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    fbe_lun_init_zero_t init_lun_zero_req;

    /*The user API shouldn't force init zero and must consider the ndb_b flag of LUN*/
    init_lun_zero_req.force_init_zero = FBE_FALSE;

    status = fbe_api_common_send_control_packet(FBE_LUN_CONTROL_CODE_INITIATE_ZERO,
                                                &init_lun_zero_req, 
                                                sizeof (fbe_lun_init_zero_t),
                                                in_lun_object_id,
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

/*!***************************************************************************
 *          fbe_api_lun_initiate_hard_zero()
 *****************************************************************************
 * @brief
 *  This function initiates a hard zero operation on a LUN.
 *
 * @param in_lun_object_id      - The LUN object ID
 * @param in_lba                - The start lba
 * @param in_blocks             - The blocks to be zeroed
 * @param in_threads            - How many zero write IOs in parallel
 *
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  27/12/14 - Created Geng Han
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lun_initiate_hard_zero(fbe_object_id_t in_lun_object_id,
                                                         fbe_lba_t in_lba,
                                                         fbe_block_count_t in_blocks,
                                                         fbe_u64_t in_threads,
                                                         fbe_bool_t clear_paged_metadata)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    fbe_lun_hard_zero_t lun_hard_zero_req;

    lun_hard_zero_req.lba = in_lba;
    lun_hard_zero_req.blocks = in_blocks;
    lun_hard_zero_req.threads = in_threads;
    lun_hard_zero_req.clear_paged_metadata = clear_paged_metadata;

    status = fbe_api_common_send_control_packet(FBE_LUN_CONTROL_CODE_INITIATE_HARD_ZERO,
                                                &lun_hard_zero_req,
                                                sizeof (fbe_lun_hard_zero_t),
                                                in_lun_object_id,
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


/*!***************************************************************************
 *          fbe_api_lun_initiate_reboot_zero()
 *****************************************************************************
 * @brief
 *  This function initiates a zero operation on a LUN when reboot.Only available at Active SP
 *
 * @param in_lun_object_id      - The LUN object ID
 *
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  2/6/12 - Created He Wei
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lun_initiate_reboot_zero(fbe_object_id_t in_lun_object_id)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_base_config_control_metadata_get_info_t metadata_get_info;
    fbe_base_config_nonpaged_metadata_t *base_config_nonpaged_metadata_p;
    fbe_api_control_operation_status_info_t status_info;
    fbe_lifecycle_state_t requested_lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_config_generation_t generation_number;
    fbe_cmi_service_get_info_t cmi_info;

    status = fbe_api_cmi_service_get_info(&cmi_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Get CMI message failed\n", __FUNCTION__);
        return status;
    }

    if(cmi_info.sp_state != FBE_CMI_STATE_ACTIVE)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Should on active side\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /*First, get the target LUN's generation number*/
    status = fbe_api_common_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_METADATA_GET_INFO,
                                                &metadata_get_info,
                                                sizeof(fbe_base_config_control_metadata_get_info_t),
                                                in_lun_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Get LUN's generation number failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    base_config_nonpaged_metadata_p = (fbe_base_config_nonpaged_metadata_t*)metadata_get_info.nonpaged_data.data;
    generation_number = base_config_nonpaged_metadata_p->generation_number;

    /*Change the LUN's generation number to a different one*/
    generation_number++;

    requested_lifecycle_state = FBE_LIFECYCLE_STATE_FAIL;
    status = fbe_api_set_object_to_state(in_lun_object_id, requested_lifecycle_state, FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Failed set object 0x%x, state to %d ", 
                       __FUNCTION__, in_lun_object_id, requested_lifecycle_state);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    status = fbe_api_wait_for_object_lifecycle_state(in_lun_object_id,requested_lifecycle_state, 
                                                     20000, FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Failed wait object 0x%x, state to %d ",
                       __FUNCTION__, in_lun_object_id, requested_lifecycle_state);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet(FBE_LUN_CONTROL_CODE_SET_NONPAGED_GENERATION_NUM,
                                                &generation_number, 
                                                sizeof(fbe_config_generation_t),
                                                in_lun_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Update LUN's generation number failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    return status;
}


/*!***************************************************************
 * @fn fbe_api_lun_get_verify_report(fbe_object_id_t             in_lun_object_id,
                                    fbe_packet_attr_t           in_attribute,
                                    fbe_lun_verify_report_t*    out_verify_report_p)
 ****************************************************************
 * @brief
 *  This function gets the LUN verify report.
 *
 * @param in_lun_object_id      - The LUN object ID
 * @param in_attribute          - packet attribute flag
 * @param out_verify_report_p   - pointer to the verify report data
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  11/12/09 - Created. MEV
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lun_get_verify_report(
                            fbe_object_id_t             in_lun_object_id,
                            fbe_packet_attr_t           in_attribute,
                            fbe_lun_verify_report_t*    out_verify_report_p)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    if (out_verify_report_p == NULL) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL buffer pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_LUN_CONTROL_CODE_GET_VERIFY_REPORT,
                                                 out_verify_report_p,
                                                 sizeof(fbe_lun_verify_report_t),
                                                 in_lun_object_id,
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
}   // end fbe_api_lun_get_verify_report()

/*!********************************************************************
 * @fn fbe_api_lun_trespass_op(fbe_object_id_t             in_lun_object_id,
                               fbe_packet_attr_t           in_attribute,
                               fbe_lun_trespass_op_t*      in_lun_trespass_op_p)
 *********************************************************************
 * @brief
 *  This function issues trespass related control code 
 *
 * @param in_lun_object_id          - The LUN object ID
 * @param in_attribute              - packet attribute flag
 * @param in_lun_trespass_op_p      - pointer to the trespass operation
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  11/17/09 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lun_trespass_op(fbe_object_id_t             in_lun_object_id,
                                     fbe_packet_attr_t           in_attribute,
                                     fbe_lun_trespass_op_t*      in_lun_trespass_op_p)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    if (in_lun_trespass_op_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL buffer pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_LUN_CONTROL_CODE_TRESPASS_OP,
                                                 in_lun_trespass_op_p,
                                                 sizeof(fbe_lun_trespass_op_t),
                                                 in_lun_object_id,
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
}   // end fbe_api_lun_trespass_op()


/*!***************************************************************
 * @fn fbe_api_lun_get_verify_status(fbe_object_id_t                 in_lun_object_id,
                                    fbe_packet_attr_t               in_attribute,
                                    fbe_lun_get_verify_status_t*    out_verify_status_p)
 ****************************************************************
 * @brief
 *  This function gets the status of a LUN verify operation.
 *
 * @param in_lun_object_id      - The LUN object ID
 * @param in_attribute          - packet attribute flag
 * @param out_verify_status_p   - pointer to the verify status data
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  11/12/09 - Created. MEV
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lun_get_verify_status(
                            fbe_object_id_t                 in_lun_object_id,
                            fbe_packet_attr_t               in_attribute,
                            fbe_lun_get_verify_status_t*    out_verify_status_p)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    if (out_verify_status_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL buffer pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_LUN_CONTROL_CODE_GET_VERIFY_STATUS,
                                                 out_verify_status_p,
                                                 sizeof(fbe_lun_get_verify_status_t),
                                                 in_lun_object_id,
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
}   // end fbe_api_lun_get_verify_status()


/*!***************************************************************
 * @fn fbe_api_lun_clear_verify_report(fbe_object_id_t             in_lun_object_id, 
                                       fbe_packet_attr_t           in_attribute)
 ****************************************************************
 * @brief
 *  This function clears the verify report on a LUN.
 *
 * @param in_lun_object_id      - pointer to the LUN object
 * @param in_attribute          - attributes
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  11/23/09 - Created. MEV
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lun_clear_verify_report(
                            fbe_object_id_t             in_lun_object_id, 
                            fbe_packet_attr_t           in_attribute)
{
    fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;


    status = fbe_api_common_send_control_packet (FBE_LUN_CONTROL_CODE_CLEAR_VERIFY_REPORT,
                                                 NULL, 
                                                 0,
                                                 in_lun_object_id,
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

}   // end fbe_api_lun_clear_verify_report()


/*!***************************************************************
 * @fn fbe_api_lun_calculate_imported_capacity(
 *        fbe_api_lun_calculate_capacity_info_t * capacity_info)
 ****************************************************************
 * @brief
 *  This function calculates imported capacity based on the given exported
 *  capacity.
 * 
 * @param capacity_info          - pointer to lun capacity information 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  11/01/11 - Modified. Vera Wang
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lun_calculate_imported_capacity(
                            fbe_api_lun_calculate_capacity_info_t * capacity_info)
{
    fbe_status_t                                            status;
    fbe_api_control_operation_status_info_t                 status_info;
    fbe_lun_control_class_calculate_capacity_t     calculate_capacity;

    calculate_capacity.exported_capacity = capacity_info->exported_capacity;
    calculate_capacity.imported_capacity = 0;
    calculate_capacity.lun_align_size = capacity_info->lun_align_size;

    /* send control packet to lun class */
    status = fbe_api_common_send_control_packet_to_class (FBE_LUN_CONTROL_CODE_CLASS_CALCULATE_IMPORTED_CAPACITY,
                                                          &calculate_capacity,
                                                          sizeof(fbe_lun_control_class_calculate_capacity_t),
                                                          FBE_CLASS_ID_LUN,
                                                          FBE_PACKET_FLAG_NO_ATTRIB, 
                                                          &status_info,
                                                          FBE_PACKAGE_ID_SEP_0);

    /* check the packet completion status */
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    capacity_info->imported_capacity = calculate_capacity.imported_capacity;
    return status;

}  // end fbe_api_lun_calculate_imported_capacity()


/*!***************************************************************
 * @fn fbe_api_lun_calculate_exported_capacity(
 *        fbe_api_lun_calculate_capacity_info_t * capacity_info)
 ****************************************************************
 * @brief
 *  This function calculates exported capacity based on the given imported
 *  capacity.
 * 
 * @param capacity_info          - pointer to lun capacity information 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  11/01/11 - Created. Vera Wang
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lun_calculate_exported_capacity(
                            fbe_api_lun_calculate_capacity_info_t * capacity_info)
{
    fbe_status_t                                            status;
    fbe_api_control_operation_status_info_t                 status_info;
    fbe_lun_control_class_calculate_capacity_t     calculate_capacity;

    calculate_capacity.imported_capacity = capacity_info->imported_capacity;
    calculate_capacity.exported_capacity = 0;
    calculate_capacity.lun_align_size = capacity_info->lun_align_size; 

    /* send control packet to lun class */
    status = fbe_api_common_send_control_packet_to_class (FBE_LUN_CONTROL_CODE_CLASS_CALCULATE_EXPORTED_CAPACITY,
                                                          &calculate_capacity,
                                                          sizeof(fbe_lun_control_class_calculate_capacity_t),
                                                          FBE_CLASS_ID_LUN,
                                                          FBE_PACKET_FLAG_NO_ATTRIB,
                                                          &status_info,
                                                          FBE_PACKAGE_ID_SEP_0);

    /* check the packet completion status */
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    capacity_info->exported_capacity = calculate_capacity.exported_capacity;
    return status;

}  // end fbe_api_lun_calculate_exported_capacity()


/*!***************************************************************
 * @fn fbe_api_lun_calculate_max_lun_size(fbe_u32_t in_rg_id, fbe_lba_t in_capacity, 
 *                                       fbe_u32_t in_num_luns, fbe_lba_t* out_capacity_p)
 *
 ****************************************************************
 * @brief
 *   Calculate max LUN size based on RG number, capacity used, and number of luns.
 *   It will be used by IOCTL_FLARE_ADM_BIND_GET_CONFIG.
 * @param in_rg_id       - RG ID
 * @param in_capacity    - capacity that used
 * @param in_num_luns    - number of LUNs
 * @param out_capacity_p - pointer to the capacity
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  11/03/11 - Created. Vera Wang
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lun_calculate_max_lun_size(fbe_u32_t in_rg_id, fbe_lba_t in_capacity, 
                                               fbe_u32_t in_num_luns, fbe_lba_t* out_capacity_p)
{
    fbe_object_id_t rg_object_id;
    fbe_api_raid_group_get_info_t  get_rg_info;
    fbe_api_lun_calculate_capacity_info_t capacity_info;
    fbe_status_t status = FBE_STATUS_OK;

    // Get RG Object ID based on RG number
    status = fbe_api_database_lookup_raid_group_by_number(in_rg_id, &rg_object_id);
    if ( status != FBE_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Failed to get RG ObjID based on RG:%d, Sts=0x%x\n", __FUNCTION__, in_rg_id, status);
        return status;
    } 

    // Get the RAID Info 
    fbe_zero_memory(&get_rg_info, sizeof(get_rg_info));
    status = fbe_api_raid_group_get_info(rg_object_id, &get_rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Failed to get RG info for ObjID:0x%x, Sts=0x%x\n", __FUNCTION__, rg_object_id, status);
        return status;
    }
    
    // If the input capacity is 0, then get the contiguous free space and set it to input capacity.
    if (in_capacity == 0)
    {
        fbe_block_transport_control_get_max_unused_extent_size_t max_unused_extent_size;
        status = fbe_api_raid_group_get_max_unused_extent_size(rg_object_id, &max_unused_extent_size);
        in_capacity = max_unused_extent_size.extent_size;
    }

    if (in_capacity == 0)
    {
        // there is no capacity left
        *out_capacity_p = 0; 
        return FBE_STATUS_OK;
    }
 
    //Calculate lun exported capacity--lun max size 
    capacity_info.imported_capacity = in_capacity / in_num_luns;
    capacity_info.lun_align_size = get_rg_info.lun_align_size;
    status = fbe_api_lun_calculate_exported_capacity(&capacity_info);
    *out_capacity_p = capacity_info.exported_capacity; 

    return status;
}//end fbe_api_lun_calculate_max_lun_size()

/*!***************************************************************
 * @fn fbe_api_lun_calculate_cache_zero_bit_map_size_to_remove(
 *        fbe_api_lun_calculate_cache_zero_bitmap_blocks_t * cache_zero_bitmap_blocks_info)
 ****************************************************************
 * @brief
 *  This function calculates cache zero bitmap block count based on the given lun
 *  capacity.
 * 
 * @param cache_zero_bitmap_blocks_info - pointer to lun cache zero bitmap block information 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK        - if no error.
 *
 * @version
 *  06/03/13 - Created. Sandeep Chaudhari
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lun_calculate_cache_zero_bit_map_size_to_remove(
                            fbe_api_lun_calculate_cache_zero_bitmap_blocks_t * cache_zero_bitmap_blocks_info)
{
    fbe_status_t                                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t                     status_info = {0};
    fbe_lun_control_class_calculate_cache_zero_bitmap_blocks_t  calculate_cache_zero_bitmap_blocks_info = {0};

    calculate_cache_zero_bitmap_blocks_info.lun_capacity_in_blocks = cache_zero_bitmap_blocks_info->lun_capacity_in_blocks;
    calculate_cache_zero_bitmap_blocks_info.cache_zero_bitmap_block_count = 0;

    /* send control packet to lun class */
    status = fbe_api_common_send_control_packet_to_class (FBE_LUN_CONTROL_CODE_CLASS_CALCULATE_CACHE_ZERO_BIT_MAP_SIZE_TO_REMOVE,
                                                          &calculate_cache_zero_bitmap_blocks_info,
                                                          sizeof(fbe_lun_control_class_calculate_cache_zero_bitmap_blocks_t),
                                                          FBE_CLASS_ID_LUN,
                                                          FBE_PACKET_FLAG_NO_ATTRIB,
                                                          &status_info,
                                                          FBE_PACKAGE_ID_SEP_0);

    /* check the packet completion status */
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    cache_zero_bitmap_blocks_info->cache_zero_bitmap_block_count = calculate_cache_zero_bitmap_blocks_info.cache_zero_bitmap_block_count ;
    return status;

}  // end fbe_api_lun_calculate_cache_zero_bit_map_size_to_remove()


/*!***************************************************************
 * @fn fbe_api_lun_set_power_saving_policy(fbe_object_id_t                 in_lun_object_id,
                                            fbe_lun_set_power_saving_policy_t*    in_policy_p)
 ****************************************************************
 * @brief
 *  This function sets the power saving policy of the LU
 *
 * @param in_lun_object_id      - The LUN object ID
 * @param in_policy_p   - pointer to the power saving policy of the LU
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lun_set_power_saving_policy(
                            fbe_object_id_t                 in_lun_object_id,
                            fbe_lun_set_power_saving_policy_t*    in_policy_p)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    if (in_policy_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL buffer pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_LUN_CONTROL_CODE_SET_POWER_SAVING_POLICY,
                                                 in_policy_p,
                                                 sizeof(fbe_lun_set_power_saving_policy_t),
                                                 in_lun_object_id,
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
}   // end fbe_api_lun_set_power_saving_policy()


/*!***************************************************************
 * @fn fbe_api_lun_get_power_saving_policy(fbe_object_id_t                 in_lun_object_id,
                                            fbe_lun_set_power_saving_policy_t*    in_policy_p)
 ****************************************************************
 * @brief
 *  This function gets the power saving policy of the LU
 *
 * @param in_lun_object_id      - The LUN object ID
 * @param in_policy_p   - pointer to the power saving policy of the LU
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lun_get_power_saving_policy(
                            fbe_object_id_t                 in_lun_object_id,
                            fbe_lun_set_power_saving_policy_t*    in_policy_p)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    if (in_policy_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL buffer pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_LUN_CONTROL_CODE_GET_POWER_SAVING_POLICY,
                                                 in_policy_p,
                                                 sizeof(fbe_lun_set_power_saving_policy_t),
                                                 in_lun_object_id,
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
}   // end fbe_api_lun_set_power_saving_policy()


/*!***************************************************************
 * @fn fbe_api_lun_get_zero_status(fbe_object_id_t   lun_object_id,
                                   fbe_api_lun_get_zero_percentage_t *   fbe_api_lun_get_zero_percentage_p)
 ****************************************************************
 * @brief
 *  This function is used to get the zero percentage for the LU
 *  object.
 *
 * @param in_lun_object_id              - The LUN object ID
 * @param fbe_api_lun_get_zero_status_p - pointer to the zero status.
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lun_get_zero_status(fbe_object_id_t  lun_object_id,
                                                      fbe_api_lun_get_zero_status_t * fbe_api_lun_get_zero_status_p)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_lun_get_zero_status_t                       lun_get_zero_status;

    if (fbe_api_lun_get_zero_status_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL buffer pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* initialize zero percentage and checkpoint as zero. */
    lun_get_zero_status.zero_percentage = 0;
    lun_get_zero_status.zero_checkpoint = 0;

    /* send the usurper command to lu object. */
    status = fbe_api_common_send_control_packet (FBE_LUN_CONTROL_CODE_GET_ZERO_STATUS,
                                                 &lun_get_zero_status,
                                                 sizeof(fbe_lun_get_zero_status_t),
                                                 lun_object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if(status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* return zero status for the lun object. */
    fbe_api_lun_get_zero_status_p->zero_checkpoint = lun_get_zero_status.zero_checkpoint;
    fbe_api_lun_get_zero_status_p->zero_percentage = lun_get_zero_status.zero_percentage;
    return FBE_STATUS_OK;

}   // end fbe_api_lun_get_zero_status()

/*!***************************************************************
 * @fn fbe_api_create_lun(fbe_api_lun_create_t *lu_create_strcut,
                                             fbe_bool_t wait_ready, 
                                             fbe_u32_t ready_timeout_msec,
                                             fbe_object_id_t *lu_object_id,
                                             fbe_job_service_error_type_t *job_error_type)
 ****************************************************************
 * @brief
 *  This function creates a LUN. It is synchronous and will return once the LU is created.
 *  If selected, the function will also wait until the lu will be ready.
 *  If the user sets the lun_number of lu_create_strcut as FBE_LUN_ID_INVALID,
 *  MCR will fill the number and return it on the same place.
 * 
 *  If 0 is passed in lu_create_strcut->capacity, we will try to fit
 *  a LUN at the end of the RG and return the size in create_strcut->capacity.
 *
 *
 * @param lu_create_strcut - The data needed for LU creation
 * @param wait_ready - does the user want to wait for the LU to become ready.
 * @param ready_timeout_msec - how long to wait for the LU to become ready
 * @param job_error_type - pointer to job error type which will be used by Admin.
 *
 * @return
 *  FBE_STATUS_GENERIC_FAILURE - unable to create LUN
 *  FBE_STATUS_INSUFFICIENT_RESOURCES - Can't create a LUN becuse the size if bigger than the free size
 *  FBE_STATUS_OK - everything is good, lun created
 *
 *
 * 11/15/2012 Modified by Vera Wang
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_create_lun(fbe_api_lun_create_t *lu_create_strcut,
                                             fbe_bool_t wait_ready, 
                                             fbe_u32_t ready_timeout_msec,
                                             fbe_object_id_t *lu_object_id,
                                             fbe_job_service_error_type_t *job_error_type)
{
    fbe_status_t                         status, job_status;
    fbe_api_job_service_lun_create_t     lu_create_job = {0};
	fbe_block_transport_control_get_max_unused_extent_size_t get_extent;
	fbe_object_id_t 					 rg_object_id;
    fbe_object_id_t                      lun_object_id = FBE_OBJECT_ID_INVALID;

    if (job_error_type == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: failed to pass job_error_type pointer. \n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
#ifndef NO_EXT_POOL_ALIAS
    status = fbe_api_database_lookup_ext_pool_by_number(lu_create_strcut->raid_group_id, &rg_object_id);
    if (status == FBE_STATUS_OK && rg_object_id != FBE_OBJECT_ID_INVALID)
    {
        fbe_api_job_service_create_ext_pool_lun_t create_lun_p;
        create_lun_p.lun_id = lu_create_strcut->lun_number;
        create_lun_p.pool_id = lu_create_strcut->raid_group_id;
        create_lun_p.capacity = lu_create_strcut->capacity;
        create_lun_p.world_wide_name = lu_create_strcut->world_wide_name;
        create_lun_p.user_defined_name = lu_create_strcut->user_defined_name;
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s: EXT POOL LUN pool %d lun %d capacity 0x%llx. \n", 
			__FUNCTION__, create_lun_p.pool_id, create_lun_p.lun_id, create_lun_p.capacity); 
        status = fbe_api_job_service_create_ext_pool_lun(&create_lun_p);
        if (status != FBE_STATUS_OK) {
            *job_error_type = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to create EXT POOL LUN\n", __FUNCTION__);
        }
	    if (lu_object_id != NULL) {
            *lu_object_id = create_lun_p.lun_object_id;
        }
        *job_error_type = FBE_JOB_SERVICE_ERROR_NO_ERROR;
        return status;
    }
#endif

    /*fill in all the information we need*/
    lu_create_job.raid_type = lu_create_strcut->raid_type;
    lu_create_job.raid_group_id = lu_create_strcut->raid_group_id;
    lu_create_job.lun_number = lu_create_strcut->lun_number;
    lu_create_job.capacity = lu_create_strcut->capacity;
    lu_create_job.placement = lu_create_strcut->placement;
    lu_create_job.addroffset = lu_create_strcut->addroffset;
    lu_create_job.ndb_b = lu_create_strcut->ndb_b;
    lu_create_job.noinitialverify_b = lu_create_strcut->noinitialverify_b;
    lu_create_job.export_lun_b = lu_create_strcut->export_lun_b;
    lu_create_job.bind_time = fbe_get_time();
    lu_create_job.user_private = lu_create_strcut->user_private;
    lu_create_job.wait_ready = wait_ready;
    lu_create_job.ready_timeout_msec = ready_timeout_msec;
    fbe_copy_memory (&lu_create_job.world_wide_name, &lu_create_strcut->world_wide_name, sizeof(lu_create_job.world_wide_name));
    fbe_copy_memory (&lu_create_job.user_defined_name, &lu_create_strcut->user_defined_name, sizeof(lu_create_job.user_defined_name));

	/*to preserve some crazy legacy interface, when capacity passed in is 0, it means the upper layers just
	want to bind a lun on whatever is left of the RG, so let's see if we ahve any left*/
    if (lu_create_strcut->is_system_lun) {
        lu_create_job.is_system_lun = FBE_TRUE;
        lu_create_job.lun_id = lu_create_strcut->lun_id;
    } else if (lu_create_strcut->capacity == 0) {

		/*now we need to get the RG object id from the database*/
		status = fbe_api_database_lookup_raid_group_by_number(lu_create_strcut->raid_group_id, &rg_object_id);
		if (status != FBE_STATUS_OK) {
			fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: failed to get rg object ID, status:%X\n", __FUNCTION__,  status); 
			*job_error_type = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
            return FBE_STATUS_GENERIC_FAILURE;
		}

		status = fbe_api_raid_group_get_max_unused_extent_size(rg_object_id, &get_extent);
		if (status != FBE_STATUS_OK) {
			fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: failed to get remaining extent on RG, status:%X\n", __FUNCTION__,  status); 
			*job_error_type = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
            return FBE_STATUS_GENERIC_FAILURE;
		}

		if (get_extent.extent_size != 0) {
			fbe_api_lun_calculate_max_lun_size(lu_create_strcut->raid_group_id, get_extent.extent_size, 1, &lu_create_job.capacity);
			lu_create_strcut->capacity = lu_create_job.capacity;/*return to the user whaty what we did*/
		}else{
			fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: no space left on RG\n", __FUNCTION__); 
			*job_error_type = FBE_JOB_SERVICE_ERROR_REQUEST_BEYOND_CURRENT_RG_CAPACITY;
            return FBE_STATUS_GENERIC_FAILURE;/*if the user already sent us 0 and we gave him everything we had and he keeps doing that, too bad*/
		}
	}

    /*start the job*/
    status = fbe_api_job_service_lun_create(&lu_create_job);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to start LU creation job\n", __FUNCTION__);
        return status;
    }

	/*wait for it*/
	status = fbe_api_common_wait_for_job(lu_create_job.job_number,
                                         ready_timeout_msec,
                                         job_error_type,
                                         &job_status,
                                         &lun_object_id);

	if ((status != FBE_STATUS_OK) || (job_status != FBE_STATUS_OK) || (*job_error_type != FBE_JOB_SERVICE_ERROR_NO_ERROR)) {
		fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s:job errors: status:%d, job_code:%d\n",__FUNCTION__, job_status, *job_error_type);
		
		if (*job_error_type == FBE_JOB_SERVICE_ERROR_REQUEST_BEYOND_CURRENT_RG_CAPACITY) {
			return FBE_STATUS_INSUFFICIENT_RESOURCES;
		}else{
			return FBE_STATUS_GENERIC_FAILURE;
		}
	}

	/* The LUN object id will be returned from job notification */
	if (lu_object_id != NULL) {
        *lu_object_id = lun_object_id;
    }

	/*let's populate the lun number the DB assigned to us (if needed)*/
	if (lu_create_strcut->lun_number == FBE_LUN_ID_INVALID) {
		status = fbe_api_database_lookup_lun_by_object_id(lun_object_id, &lu_create_strcut->lun_number);
		if (status != FBE_STATUS_OK) {
			fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: can't get assigned lun number for id:0x%x\n",
					  __FUNCTION__, lun_object_id);

			return status;
		}
	}
    
    return status;
}

/*!***************************************************************
 * @fn fbe_api_destroy_lun(fbe_api_lun_destroy_t *lu_destroy_strcut,
 *                         fbe_bool_t wait_destroy,
 *                         fbe_u32_t ready_timeout_msec,
 *                         fbe_job_service_error_type_t *job_error_type)
 ****************************************************************
 * @brief
 *  This function creates a LUN. It is synchronous and will return once the LU is created.
 *  If selected, the function will also wait until the lu will be ready
 *
 * @param lu_destroy_strcut - The data needed for LU destruction
 * @param wait_destroy - does the user want to wait for the LU to be gone completly
 * @param destroy_timeout_msec - how long to wait for the LU to be gone
 * @param job_error_type - pointer to job_error_type which is used by Admin
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_destroy_lun(fbe_api_lun_destroy_t *lu_destroy_strcut,
                                             fbe_bool_t wait_destroy, 
                                             fbe_u32_t destroy_timeout_msec,
                                             fbe_job_service_error_type_t *job_error_type)
{
    fbe_status_t 							status = FBE_STATUS_OK;
    fbe_api_job_service_lun_destroy_t		lu_destroy_job = {0};
    fbe_status_t							job_status = FBE_STATUS_OK;
    fbe_object_id_t                         lun_object_id = FBE_OBJECT_ID_INVALID;

    if (job_error_type == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: failed to pass job_error_type pointer. \n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

#ifndef NO_EXT_POOL_ALIAS
    status = fbe_api_database_lookup_ext_pool_lun_by_number(lu_destroy_strcut->lun_number, &lun_object_id);
    if (status == FBE_STATUS_OK && lun_object_id != FBE_OBJECT_ID_INVALID)
    {
        fbe_api_job_service_destroy_ext_pool_lun_t destroy_lun_p;
        destroy_lun_p.lun_id = lu_destroy_strcut->lun_number;
        destroy_lun_p.pool_id = FBE_POOL_ID_INVALID;
        return fbe_api_job_service_destroy_ext_pool_lun(&destroy_lun_p);
    }
#endif

    /*fill in all the information we need*/
    lu_destroy_job.lun_number = lu_destroy_strcut->lun_number;
    lu_destroy_job.allow_destroy_broken_lun = lu_destroy_strcut->allow_destroy_broken_lun;
    lu_destroy_job.wait_destroy = wait_destroy;
    lu_destroy_job.destroy_timeout_msec = destroy_timeout_msec;

    /*start the job*/
    status = fbe_api_job_service_lun_destroy(&lu_destroy_job);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to start LU destruction job\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*wait for it*/
    status = fbe_api_common_wait_for_job(lu_destroy_job.job_number, destroy_timeout_msec, job_error_type, &job_status, NULL);
    if (status != FBE_STATUS_OK || job_status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: wait for job failed with status: 0x%X, job status:0x%X, job error:0x%x\n",
                      __FUNCTION__, status, job_status, *job_error_type);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* JOB service returned error and so cannot destroy LUN.. */
    if(*job_error_type != FBE_JOB_SERVICE_ERROR_NO_ERROR)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                      "LUN cannot be deleted.. job_error_type: 0x%x (%s)\n",
                      *job_error_type, 
                      fbe_api_job_service_notification_error_code_to_string(*job_error_type));

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_lun_enable_peformance_stats(fbe_object_id_t             in_lun_object_id)
 ****************************************************************
 * @brief
 *  This function enables performance statistics on the lun object id
 *
 * @param in_lun_object_id      - pointer to the LUN object
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  12/13/10 - Created. Swati Fursule
 *  4/24/2012 - Updated for compatibility with perfstats service Ryan Gadsby
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lun_enable_peformance_stats(fbe_object_id_t in_lun_object_id)
{
    fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;


    status = fbe_api_common_send_control_packet (FBE_LUN_CONTROL_CODE_ENABLE_PERFORMANCE_STATS,
                                                 NULL, 
                                                 0,
                                                 in_lun_object_id,
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

}   // end fbe_api_lun_enable_peformance_stats()

/*!***************************************************************
 * @fn fbe_api_lun_disable_peformance_stats(fbe_object_id_t             in_lun_object_id)
 ****************************************************************
 * @brief
 *  This function disables performance statistics on the lun object id
 *
 * @param in_lun_object_id      - pointer to the LUN object
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  12/13/10 - Created. Swati Fursule
 *  4/24/2012 - Updated for compatibility with perfstats service Ryan Gadsby
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lun_disable_peformance_stats(fbe_object_id_t             in_lun_object_id)
{
    fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;


    status = fbe_api_common_send_control_packet (FBE_LUN_CONTROL_CODE_DISABLE_PERFORMANCE_STATS,
                                                 NULL, 
                                                 0,
                                                 in_lun_object_id,
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

}   // end fbe_api_lun_disable_peformance_stats()

/*!***************************************************************
 * @fn fbe_api_lun_get_peformance_stats(fbe_object_id_t                 in_lun_object_id,
                                        fbe_lun_performance_counters_t  *lun_stats)
 ****************************************************************
 * @brief
 *  This function gets the performance statistics on the lun object id
 *
 * WARNING: this function was only created because we needed a way to copy performance
 * statistics across the transport buffer so fbe_hw_test could access the counter space instead
 * of copying the entire container on each check! Use the PERFSTATS service to access the performance
 * statistics in non-test environments.
 * 
 * @param in_lun_object_id      - pointer to the LUN object
 * @param lun_stats             - pointer to a lun stat object
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  7/11/2012 - Created.Ryan Gadsby
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lun_get_performance_stats(fbe_object_id_t                  in_lun_object_id,
                                                           fbe_lun_performance_counters_t   *lun_stats)
{
    fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;

    if (in_lun_object_id >= FBE_OBJECT_ID_INVALID) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:object_id can't be FBE_OBJECT_ID_INVALID\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (lun_stats == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:pdo_counters can't be NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_LUN_CONTROL_CODE_GET_PERFORMANCE_STATS,
                                                 lun_stats, 
                                                 sizeof(fbe_lun_performance_counters_t),
                                                 in_lun_object_id,
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

}   // end fbe_api_lun_get_peformance_stats()

/*!***************************************************************
 * @fn fbe_api_lun_get_lun_info(fbe_object_id_t   lun_object_id,
                                fbe_api_lun_get_lun_info_t *   fbe_api_lun_get_lun_info_p)
 ****************************************************************
 * @brief
 *  This function is used to get information for the LU
 *  object.
 *
 * @param in_lun_object_id            - The LUN object ID
 * @param fbe_api_lun_get_lun_info_p  - pointer to the lun info.
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lun_get_lun_info(fbe_object_id_t  lun_object_id,
                                                   fbe_api_lun_get_lun_info_t * fbe_api_lun_get_lun_info_p)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_lun_get_lun_info_t                          lun_get_lun_info;
	fbe_u32_t										i;

    if (fbe_api_lun_get_lun_info_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL buffer pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    lun_get_lun_info.peer_lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;

	for(i = 0; i < 6; i++) { /* 6 reties 0.5 sec each - 3 sec. total */
		/* send the usurper command to lu object. */
		status = fbe_api_common_send_control_packet (FBE_LUN_CONTROL_CODE_GET_LUN_INFO,
													 &lun_get_lun_info,
													 sizeof(fbe_lun_get_lun_info_t),
													 lun_object_id,
													 FBE_PACKET_FLAG_NO_ATTRIB,
													 &status_info,
													 FBE_PACKAGE_ID_SEP_0);

		if(status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
			fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
							status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

			if(status == FBE_STATUS_BUSY){
				fbe_api_sleep(500);
			} else {
				return FBE_STATUS_GENERIC_FAILURE;
			}
		} else {/* Good status */
			/* return zero status for the lun object. */
			fbe_api_lun_get_lun_info_p->peer_lifecycle_state = lun_get_lun_info.peer_lifecycle_state;
			fbe_api_lun_get_lun_info_p->capacity = lun_get_lun_info.capacity;
			fbe_api_lun_get_lun_info_p->offset = lun_get_lun_info.offset;
			fbe_api_lun_get_lun_info_p->b_initial_verify_already_run = lun_get_lun_info.b_initial_verify_already_run;
			fbe_api_lun_get_lun_info_p->b_initial_verify_requested = lun_get_lun_info.b_initial_verify_requested;
			return FBE_STATUS_OK;
		}
	}//for(i = 0; i < 6; i++) { /* 6 reties 0.5 sec each */
    
	/* We reached total timeout */
    return FBE_STATUS_GENERIC_FAILURE;

}   // end fbe_api_lun_get_lun_info()

/*!***************************************************************
 *  @fn fbe_api_lun_get_rebuild_status(fbe_object_id_t lun_object_id,
 *                                     fbe_api_lun_get_status_t *lun_rebuild_status)
 ****************************************************************
 * @brief
 *  This function returns the rebuild status for LUN. 
 *
 * @param lun_object_id - LUN object id
 * @param lun_rebuild_status - ptr to data structure that will hold
 *                          rebuild status for lun
 *
 * @return
 *  fbe_status_t - status 
 *
 * @version
 *  7/09/2011 - Created. Sonali K
 *  6/18/2012 - Modifed. Vera Wang 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lun_get_rebuild_status(fbe_object_id_t lun_object_id, 
                                                         fbe_api_lun_get_status_t *lun_rebuild_status_p)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_lun_rebuild_status_t                        rebuild_status;

    if (lun_rebuild_status_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL buffer pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* send the usurper command to lu object. */
    status = fbe_api_common_send_control_packet (FBE_LUN_CONTROL_CODE_GET_REBUILD_STATUS,
                                                 &rebuild_status,
                                                 sizeof(fbe_lun_rebuild_status_t),
                                                 lun_object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if(status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* return zero status for the lun object. */
    lun_rebuild_status_p->checkpoint = rebuild_status.rebuild_checkpoint;
    lun_rebuild_status_p->percentage_completed = rebuild_status.rebuild_percentage;
    
    return FBE_STATUS_OK;
}

/*!***************************************************************
 *  @fn fbe_api_lun_get_bgverify_status(fbe_object_id_t lun_object_id,
 *                                     fbe_api_lun_get_status_t *lun_bgverify_status,
 *                                     fbe_lun_verify_type_t verify_type)
 ****************************************************************
 * @brief
 *  This function returns the bgverify status for LUN
 *
 * @param lun_object_id - LUN object id
 * @param lun_bgverify_status - ptr to data structure that will hold
 *                          bg verify status for lun
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  7/09/2011 - Created. Sonali K
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lun_get_bgverify_status(fbe_object_id_t lun_object_id, 
                                                          fbe_api_lun_get_status_t *lun_bgverify_status,
                                                          fbe_lun_verify_type_t verify_type)
{
    fbe_api_base_config_downstream_object_list_t downstream_lun_object_list;
    fbe_status_t  status = FBE_STATUS_OK;    
    fbe_api_lun_get_lun_info_t lun_info;      
    fbe_api_rg_get_status_t rg_bgverify_status;    
    fbe_api_raid_group_get_info_t rg_info; 
    fbe_object_id_t rg_object_id;
               
    status = fbe_api_lun_get_lun_info(lun_object_id, &lun_info);
     
    if (status != FBE_STATUS_OK) 
    {
        return status;
    }
    /*Find the RG object id to which the LUN belongs i.e. find downstream object of the lun*/ 
    status = fbe_api_base_config_get_downstream_object_list(lun_object_id, &downstream_lun_object_list);    
    
    if (status != FBE_STATUS_OK) 
    {
        return status;
    }
    rg_object_id = downstream_lun_object_list.downstream_object_list[0];
    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
      
    if (status != FBE_STATUS_OK) 
    {          
        return status; 
    }      
    status = fbe_api_raid_group_get_bgverify_status(rg_object_id, &rg_bgverify_status,verify_type);   

    if (status != FBE_STATUS_OK) 
    {          
        return status; 
    }
    fbe_lun_info_get_status(lun_info,rg_info.num_data_disk,rg_bgverify_status.checkpoint,lun_bgverify_status);

    fbe_api_trace(FBE_TRACE_LEVEL_INFO,                   
                  "BG Verify checkpoint for LUN 0x%x is 0x%llx and percentage completed is %d\n",
                  lun_object_id,
		  (unsigned long long)lun_bgverify_status->checkpoint,
		  lun_bgverify_status->percentage_completed );
 
    return status;
}

/*!***************************************************************
 *  @fn fbe_lun_info_get_status(fbe_api_lun_get_lun_info_t lun_info,
 *                              fbe_u16_t num_data_disk,
 *                              fbe_lba_t       rg_checkpoint, 
 *                              fbe_api_lun_get_status_t *lun_status)
 ****************************************************************
 * @brief
 *  This function calculates and returns the status for LUN
 *
 * @param lun_info -  data structure that holds lun info
 * @param num_data_disk -  no of data disks in RG
 * @param rg_checkpoint -  RG checkpoint
 * @param lun_status - ptr to data structure that will hold
 *                     status for lun
 *
 * @return
 *  fbe_status_t - void
 *
 * @version
 *  7/09/2011 - Created. Sonali K
 *
 ****************************************************************/
static void fbe_lun_info_get_status(fbe_api_lun_get_lun_info_t lun_info,
                                            fbe_u16_t num_data_disk,
                                            fbe_lba_t       rg_checkpoint, 
                                            fbe_api_lun_get_status_t *lun_status)
{   
    fbe_lba_t  lun_per_drive_offset;
    fbe_lba_t  lun_per_drive_capacity;

    lun_per_drive_offset = lun_info.offset/num_data_disk;
  
     /*Calculate the LUN rebuild percentage*/

     lun_per_drive_capacity = lun_info.capacity/num_data_disk;     

     if(lun_per_drive_offset >= rg_checkpoint)
     {
         lun_status->percentage_completed = 0;
         lun_status->checkpoint = 0; 
     }
     else if ((lun_per_drive_offset + lun_per_drive_capacity) < rg_checkpoint)
     {
         lun_status->percentage_completed = 100;
         lun_status->checkpoint = FBE_LBA_INVALID;

     }else
     {
         lun_status->checkpoint = rg_checkpoint - lun_per_drive_offset;
         lun_status->percentage_completed = (fbe_u16_t)((lun_status->checkpoint * 100)/lun_per_drive_capacity);
     }    
}

/*!***************************************************************
 *  @fn fbe_api_lun_initiate_verify_on_all_existing_luns(fbe_lun_verify_type_t in_verify_type)
 ****************************************************************
 * @brief
 *  This function runs a verify on all existing non system LUNs.
 *
 * @param in_verify_type -  what type of verify to run
 
 * @return
 *  fbe_status_t - FBE_STATUS_OK if good
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lun_initiate_verify_on_all_existing_luns(fbe_lun_verify_type_t in_verify_type)
{
	fbe_status_t		status;
	fbe_u32_t			lun_count;
	fbe_object_id_t	*	object_array = NULL;

	/*let's go over all luns and start the verify*/
	status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &lun_count );
	if (status != FBE_STATUS_OK) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to get total LUNs in the system\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	status = fbe_api_enumerate_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &object_array, &lun_count);
	if (status != FBE_STATUS_OK) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to get a list of LUNs\n", __FUNCTION__);
        if(object_array != NULL)
        {
            fbe_api_free_memory(object_array);
        }
		return FBE_STATUS_GENERIC_FAILURE;
	}

	for (;lun_count > 0; lun_count--) {
        if (!fbe_private_space_layout_object_id_is_system_lun(object_array[lun_count-1]))  
        {
            status = fbe_api_lun_initiate_verify(object_array[lun_count - 1],
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 in_verify_type,
                                                 FBE_TRUE,
                                                 FBE_LUN_VERIFY_START_LBA_LUN_START,
                                                 FBE_LUN_VERIFY_BLOCKS_ENTIRE_LUN);

            if (status != FBE_STATUS_OK) {
                fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s, failed to initiate verify on LU ID 0x%X\n", 
                              __FUNCTION__, object_array[lun_count - 1]);
            }
        }
	}

	fbe_api_free_memory(object_array);
	return  FBE_STATUS_OK;
}

/*!***************************************************************
 *  @fn fbe_api_lun_clear_verify_reports_on_all_existing_luns()
 ****************************************************************
 * @brief
 *  This function clears the verify reports on all existing LUNs
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK if good
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lun_clear_verify_reports_on_all_existing_luns(void)
{

	fbe_status_t		status;
	fbe_u32_t			lun_count;
	fbe_object_id_t	*	object_array = NULL;

	/*let's go over all luns and start the verify*/
	status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &lun_count );
	if (status != FBE_STATUS_OK) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to get total LUNs in the system\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	status = fbe_api_enumerate_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &object_array, &lun_count);
	if (status != FBE_STATUS_OK) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to get a list of LUNs\n", __FUNCTION__);
		if(object_array != NULL)
        {
            fbe_api_free_memory(object_array);
        }
		return FBE_STATUS_GENERIC_FAILURE;
	}

	for (;lun_count > 0; lun_count--) {
        status = fbe_api_lun_clear_verify_report(object_array[lun_count - 1], FBE_PACKET_FLAG_NO_ATTRIB);

		if (status != FBE_STATUS_OK) {
			fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s, failed to clear verify on LU ID 0x%X\n", 
						  __FUNCTION__, object_array[lun_count - 1]);

		}

	}

	fbe_api_free_memory(object_array);
	return  FBE_STATUS_OK;
}

/*!***************************************************************
 *  @fn fbe_api_lun_clear_verify_reports_on_all_existing_luns()
 ****************************************************************
 * @brief
 *  This function clears the verify reports on all existing LUNs
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK if good
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lun_clear_verify_reports_on_rg(fbe_object_id_t in_rg_id)
{

	fbe_status_t								status;
    fbe_api_base_config_upstream_object_list_t  upstream_object_list;


	status = fbe_api_base_config_get_upstream_object_list(in_rg_id, &upstream_object_list);
    if(status != FBE_STATUS_OK){
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, can't get upstream LUNs list\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for (;upstream_object_list.number_of_upstream_objects > 0; upstream_object_list.number_of_upstream_objects --) {
        status = fbe_api_lun_clear_verify_report(upstream_object_list.upstream_object_list[upstream_object_list.number_of_upstream_objects - 1],
												 FBE_PACKET_FLAG_NO_ATTRIB);

		if (status != FBE_STATUS_OK) {
			fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s, failed to clear verify on LU ID 0x%X\n", 
						  __FUNCTION__, upstream_object_list.upstream_object_list[upstream_object_list.number_of_upstream_objects - 1]);

		}
	}
 
    return  FBE_STATUS_OK;
}

/*!***************************************************************
 *  @fn fbe_api_update_lun_wwn(fbe_object_id_t lu_object_id,
 *                             fbe_assigned_wwid_t* world_wide_name, 
 *                             fbe_job_service_error_type_t *job_error_type)
 *****************************************************************
 * @brief
 *  This function updates a LUN WWN. It is synchronous and will return once the LU WWN is updated.
 *
 * @param world_wide_name - The data needed to change the LU
 * @param lu_object_id - The object to change
 * 
 * @return
 *  fbe_status_t
 * 
 * @version
 *  04/16/2011 - Created. Vera Wang
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_update_lun_wwn(fbe_object_id_t lu_object_id,
                                                 fbe_assigned_wwid_t* world_wide_name,
                                                 fbe_job_service_error_type_t *job_error_type)
{
    fbe_status_t                            status;
    fbe_api_job_service_lun_update_t        lu_update_job = {0};
    fbe_status_t                            job_status;

    if (job_error_type == NULL) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to pass job error type pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /*fill in field we would like to update*/
    lu_update_job.object_id = lu_object_id;
    lu_update_job.update_type = FBE_LUN_UPDATE_WWN;
    fbe_copy_memory (&lu_update_job.world_wide_name, world_wide_name, sizeof(lu_update_job.world_wide_name));

    /*start the job*/
    status = fbe_api_job_service_lun_update(&lu_update_job);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to start LU update job\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*wait for it*/
    status = fbe_api_common_wait_for_job(lu_update_job.job_number, LU_WAIT_TIMEOUT, job_error_type, &job_status, NULL);
    if (status != FBE_STATUS_OK || job_status != FBE_STATUS_OK || *job_error_type != FBE_JOB_SERVICE_ERROR_NO_ERROR) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: wait for job failed with status: 0x%X, job status:0x%X, job error:0x%x\n",
                      __FUNCTION__, status, job_status, *job_error_type);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}

/*!***************************************************************
 *  @fn fbe_api_update_lun_udn(fbe_object_id_t lu_object_id,
 *                             fbe_user_defined_lun_name_t* user_defined_name,
 *                             fbe_job_service_error_type_t  *job_error_type)
 ***************************************************************** 
 * @brief
 *  This function updates a LUN user defined name. It is synchronous and will return once the LU UDN is updated.
 *
 * @param fbe_user_defined_lun_name_t - The data needed to change the LU
 * @param lu_object_id - The object to change
 * 
 * @return
 *  fbe_status_t
 * 
 *  @version
 *  04/16/2011 - Created. Vera Wang
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_update_lun_udn(fbe_object_id_t lu_object_id,
                                                 fbe_user_defined_lun_name_t* user_defined_name,
                                                 fbe_job_service_error_type_t *job_error_type)
{
    fbe_status_t                            status;
    fbe_api_job_service_lun_update_t        lu_update_job = {0};
    fbe_status_t                            job_status;
 
    /*fill in all the information we need*/
    lu_update_job.object_id = lu_object_id;
    lu_update_job.update_type = FBE_LUN_UPDATE_UDN;
    fbe_copy_memory (&lu_update_job.user_defined_name, user_defined_name, sizeof(lu_update_job.user_defined_name));

    /*start the job*/
    status = fbe_api_job_service_lun_update(&lu_update_job);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to start LU update job\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*wait for it*/
    status = fbe_api_common_wait_for_job(lu_update_job.job_number, LU_WAIT_TIMEOUT, job_error_type, &job_status, NULL);
    if (status != FBE_STATUS_OK || job_status != FBE_STATUS_OK || *job_error_type != FBE_JOB_SERVICE_ERROR_NO_ERROR) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: wait for job failed with status: 0x%X, job status:0x%X, job error:0x%x\n",
                      __FUNCTION__, status, job_status, *job_error_type);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}

/*!***************************************************************
 *  @fn fbe_api_update_lun_attributes(fbe_object_id_t lu_object_id,
 *                                    fbe_u32_t attributes, 
 *                                    fbe_job_service_error_type_t  *job_error_type)
 *****************************************************************
 * @brief
 *  This function updates a LUN user defined name. It is synchronous and will return once the LU UDN is updated.
 *
 * @param attributes - The data needed to change the LU
 * @param lu_object_id - The object to change
 * 
 * @return
 *  fbe_status_t
 * 
 *  @version
 *  04/16/2011 - Created. Vera Wang
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_update_lun_attributes(fbe_object_id_t lu_object_id,
                                                        fbe_u32_t attributes,
                                                        fbe_job_service_error_type_t  *job_error_type)
{
    fbe_status_t                            status;
    fbe_api_job_service_lun_update_t        lu_update_job = {0};
    fbe_status_t                            job_status;

    /*fill in all the information we need*/
    lu_update_job.object_id = lu_object_id;
    lu_update_job.update_type = FBE_LUN_UPDATE_ATTRIBUTES;
    lu_update_job.attributes = attributes;

    /*start the job*/
    status = fbe_api_job_service_lun_update(&lu_update_job);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to start LU update job\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*wait for it*/
    status = fbe_api_common_wait_for_job(lu_update_job.job_number, LU_WAIT_TIMEOUT, job_error_type, &job_status, NULL);
    if (status != FBE_STATUS_OK || job_status != FBE_STATUS_OK || *job_error_type != FBE_JOB_SERVICE_ERROR_NO_ERROR) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: wait for job failed with status: 0x%X, job status:0x%X, job error:0x%x\n",
                      __FUNCTION__, status, job_status, *job_error_type);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}


fbe_status_t FBE_API_CALL fbe_api_lun_set_write_bypass_mode( fbe_object_id_t in_lun_object_id, fbe_bool_t write_bypass_mode)
{
    fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;


	if(write_bypass_mode){
		status = fbe_api_common_send_control_packet (FBE_LUN_CONTROL_CODE_ENABLE_WRITE_BYPASS,
													 NULL, 
													 0,
													 in_lun_object_id,
													 FBE_PACKET_FLAG_NO_ATTRIB,
													 &status_info,
													 FBE_PACKAGE_ID_SEP_0);
	} else {
		status = fbe_api_common_send_control_packet (FBE_LUN_CONTROL_CODE_DISABLE_WRITE_BYPASS,
													 NULL, 
													 0,
													 in_lun_object_id,
													 FBE_PACKET_FLAG_NO_ATTRIB,
													 &status_info,
													 FBE_PACKAGE_ID_SEP_0);

	}

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;

}

/*!***************************************************************
 * fbe_api_lun_clear_unexpected_errors()
 ****************************************************************
 * @brief
 *  Clear out the error info to allow the lun to come ready.
 *
 * @param in_lun_object_id            - The LUN object ID
 * @param fbe_api_lun_get_lun_info_p  - pointer to the lun info.
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lun_clear_unexpected_errors(fbe_object_id_t lun_object_id)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t status_info;

    status = fbe_api_common_send_control_packet(FBE_LUN_CONTROL_CODE_CLEAR_UNEXPECTED_ERROR_INFO,
                                                NULL, 0,
                                                lun_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if(status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed packet error:%d, packet qualifier:%d\n", 
                      __FUNCTION__,status, status_info.packet_qualifier);
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "payload error:%d, payload qualifier:%d\n", 
                      status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
  return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_lun_clear_unexpected_errors()
 **************************************/
/*!***************************************************************
 * fbe_api_lun_disable_unexpected_errors()
 ****************************************************************
 * @brief
 *  Disable the unexpected error handling for this LUN so that
 *  no matter how many errors we get, it will not fail the LUN.
 *
 * @param in_lun_object_id            - The LUN object ID
 * @param fbe_api_lun_get_lun_info_p  - pointer to the lun info.
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lun_disable_unexpected_errors(fbe_object_id_t lun_object_id)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t status_info;

    status = fbe_api_common_send_control_packet(FBE_LUN_CONTROL_CODE_DISABLE_FAIL_ON_UNEXPECTED_ERROR,
                                                0, 0,
                                                lun_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if(status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed packet error:%d, packet qualifier:%d\n", 
                      __FUNCTION__,status, status_info.packet_qualifier);
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "payload error:%d, payload qualifier:%d\n", 
                      status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
  return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_lun_disable_unexpected_errors()
 **************************************/

/*!***************************************************************
 * fbe_api_lun_enable_unexpected_errors()
 ****************************************************************
 * @brief
 *  Enable the unexpected error handling for this LUN so that if the
 *  ratio of successes to failures exceeds a threshold we will fail the LUN.
 *
 * @param in_lun_object_id            - The LUN object ID
 * @param fbe_api_lun_get_lun_info_p  - pointer to the lun info.
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lun_enable_unexpected_errors(fbe_object_id_t lun_object_id)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t status_info;

    status = fbe_api_common_send_control_packet(FBE_LUN_CONTROL_CODE_ENABLE_FAIL_ON_UNEXPECTED_ERROR,
                                                NULL, 0,
                                                lun_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if(status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed packet error:%d, packet qualifier:%d\n", 
                      __FUNCTION__,status, status_info.packet_qualifier);
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "payload error:%d, payload qualifier:%d\n", 
                      status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
  return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_lun_enable_unexpected_errors()
 **************************************/

/*!***************************************************************
 *  fbe_api_lun_map_lba()
 ****************************************************************
 * @brief
 *  This function maps an lba of a lun to a physical address.
 *
 * @param lun_object_id - LUN's object id
 * @param rg_map_info_p - pointer to RG info which maps this LUN's lba
 *                        to a pba.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  7/10/2012 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_lun_map_lba(fbe_object_id_t lun_object_id, 
                    fbe_raid_group_map_info_t * rg_map_info_p)
{
    fbe_status_t status;
    fbe_database_lun_info_t lun_info;

    lun_info.lun_object_id = lun_object_id;
    status = fbe_api_database_get_lun_info(&lun_info);

    if (status != FBE_STATUS_OK) 
    { 
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "lun object id: 0x%x get lun info failed with status %d\n",
                      lun_object_id, status);
        return status; 
    }
    if (rg_map_info_p->lba >= lun_info.capacity)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "lun object id: 0x%x Input lba 0x%llx >= lun capacity 0x%llx\n",
                      lun_object_id, (unsigned long long)rg_map_info_p->lba, (unsigned long long)lun_info.capacity);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    rg_map_info_p->lba += lun_info.offset;

    status = fbe_api_raid_group_map_lba(lun_info.raid_group_obj_id, rg_map_info_p);
    return status;
}
/**************************************
 * end fbe_api_lun_map_lba()
 **************************************/

/*!***************************************************************
 *  @fn fbe_api_lun_initiate_verify_on_all_user_and_system_luns(fbe_lun_verify_type_t in_verify_type)
 ****************************************************************
 * @brief
 *  This function runs a verify on all existing system and non system LUNs.
 *
 * @param in_verify_type -  what type of verify to run
 
 * @return
 *  fbe_status_t - FBE_STATUS_OK if good
 *
 * @version
 *  10/5/2012 - Created. Harshal Wanjari
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lun_initiate_verify_on_all_user_and_system_luns(fbe_lun_verify_type_t in_verify_type)
{
	fbe_status_t		status;
	fbe_u32_t			lun_count;
	fbe_object_id_t	*	object_array = NULL;

	/*let's go over all luns and start the verify*/
	status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &lun_count );
	if (status != FBE_STATUS_OK) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to get total LUNs in the system\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	status = fbe_api_enumerate_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &object_array, &lun_count);
	if (status != FBE_STATUS_OK) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to get a list of LUNs\n", __FUNCTION__);
		if(object_array != NULL)
        {
            fbe_api_free_memory(object_array);
        }
		return FBE_STATUS_GENERIC_FAILURE;
	}

	for (;lun_count > 0; lun_count--) 
        {
            status = fbe_api_lun_initiate_verify(object_array[lun_count - 1],
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 in_verify_type,
                                                 FBE_TRUE,
                                                 FBE_LUN_VERIFY_START_LBA_LUN_START,
                                                 FBE_LUN_VERIFY_BLOCKS_ENTIRE_LUN);

            if (status != FBE_STATUS_OK) {
                fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s, failed to initiate verify on LU ID 0x%llX\n", 
                              __FUNCTION__, (unsigned long long)object_array[lun_count - 1]);
            }
	}

	fbe_api_free_memory(object_array);
	return  FBE_STATUS_OK;
}


/*!***************************************************************
 * fbe_api_lun_enable_io_loopback()
 ****************************************************************
 * @brief
 *  Enable the IO loopback for this LUN.
 *
 * @param lun_object_id             - The LUN object ID
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lun_enable_io_loopback(fbe_object_id_t lun_object_id)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t status_info;

    status = fbe_api_common_send_control_packet(FBE_LUN_CONTROL_CODE_ENABLE_IO_LOOPBACK,
                                                NULL, 0,
                                                lun_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if(status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed packet error:%d, packet qualifier:%d\n", 
                      __FUNCTION__,status, status_info.packet_qualifier);
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "payload error:%d, payload qualifier:%d\n", 
                      status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_lun_enable_io_loopback()
 **************************************/

/*!***************************************************************
 * fbe_api_lun_disable_io_loopback()
 ****************************************************************
 * @brief
 *  Enable the IO loopback for this LUN.
 *
 * @param lun_object_id             - The LUN object ID
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lun_disable_io_loopback(fbe_object_id_t lun_object_id)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t status_info;

    status = fbe_api_common_send_control_packet(FBE_LUN_CONTROL_CODE_DISABLE_IO_LOOPBACK,
                                                NULL, 0,
                                                lun_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if(status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed packet error:%d, packet qualifier:%d\n", 
                      __FUNCTION__,status, status_info.packet_qualifier);
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "payload error:%d, payload qualifier:%d\n", 
                      status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_lun_disable_io_loopback()
 **************************************/

/*!***************************************************************
 * fbe_api_lun_get_io_loopback()
 ****************************************************************
 * @brief
 *  Get the IO loopback for this LUN.
 *
 * @param lun_object_id             - The LUN object ID
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lun_get_io_loopback(fbe_object_id_t lun_object_id, fbe_bool_t *enabled_p)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t status_info;
    fbe_bool_t enabled;

    status = fbe_api_common_send_control_packet(FBE_LUN_CONTROL_CODE_GET_IO_LOOPBACK,
                                                &enabled, sizeof(fbe_bool_t),
                                                lun_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if(status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed packet error:%d, packet qualifier:%d\n", 
                      __FUNCTION__,status, status_info.packet_qualifier);
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "payload error:%d, payload qualifier:%d\n", 
                      status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *enabled_p = enabled;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_lun_get_io_loopback()
 **************************************/

/*!***************************************************************
 * fbe_api_lun_prepare_for_power_shutdown()
 ****************************************************************
 * @brief
 *  Mark a LUN as clean
 *
 * @param lun_object_id             - The LUN object ID
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lun_prepare_for_power_shutdown(fbe_object_id_t lun_object_id)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t status_info;

    status = fbe_api_common_send_control_packet(FBE_LUN_CONTROL_CODE_PREPARE_FOR_POWER_SHUTDOWN,
                                                NULL, 0,
                                                lun_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if(status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed packet error:%d, packet qualifier:%d\n", 
                      __FUNCTION__,status, status_info.packet_qualifier);
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "payload error:%d, payload qualifier:%d\n", 
                      status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_lun_get_io_loopback()
 **************************************/

/*************************
 * end file fbe_api_lun_interface.c
 *************************/
