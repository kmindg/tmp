/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_provision_drive_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the fbe_provision_drive interface for the storage extent package.
 *
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * @ingroup fbe_api_provision_drive_interface
 *
 * @version
 *   11/20/2009:  Created. Peter Puhov
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_provision_drive.h"
#include "fbe/fbe_port.h"
#include "fbe/fbe_enclosure.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_discovery_interface.h"

/**************************************
                Local variables
**************************************/

/*******************************************
                Local functions
********************************************/
static fbe_status_t fbe_api_provision_drive_send_download_async_callback (fbe_packet_t * packet, fbe_packet_completion_context_t context);
static void fbe_api_translate_job_error_to_user_copy_status(fbe_job_service_error_type_t job_error_code,
                                                fbe_provision_drive_copy_to_status_t* copy_status);

/*!***************************************************************
 * @fn fbe_api_provision_drive_calculate_capacity(
 *        fbe_api_provision_drive_calculate_capacity_info_t * capacity_info)
 ****************************************************************
 * @brief
 *  This function calculates capacity information based on input values.
 *
 * @param capacity_info - pointer to the capacity info
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  11/20/09 - Created. Peter Puhov
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_calculate_capacity(fbe_api_provision_drive_calculate_capacity_info_t * capacity_info)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_provision_drive_control_class_calculate_capacity_t calculate_capacity;

    calculate_capacity.imported_capacity = capacity_info->imported_capacity;
    calculate_capacity.block_edge_geometry = capacity_info->block_edge_geometry;
    calculate_capacity.exported_capacity = 0;

    status = fbe_api_common_send_control_packet_to_class (FBE_PROVISION_DRIVE_CONTROL_CODE_CLASS_CALCULATE_CAPACITY,
                                                            &calculate_capacity,
                                                            sizeof(fbe_provision_drive_control_class_calculate_capacity_t),
                                                            FBE_CLASS_ID_PROVISION_DRIVE,
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

    capacity_info->exported_capacity = calculate_capacity.exported_capacity;
    return status;
}


/*!***************************************************************
 * @fn fbe_api_provision_drive_set_configuration(
 *       fbe_object_id_t object_id, 
 *       fbe_api_provision_drive_set_configuration_t * configuration)
 ****************************************************************
 * @brief
 *  This function is for internal (debugging) use ONLY.
 *
 * @param object_id - object ID
 * @param configuration - pointer to the pvd set configuration that will be returned
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  12/09/09 - Created. Peter Puhov
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_provision_drive_set_configuration(fbe_object_id_t object_id, 
                                                                    fbe_api_provision_drive_set_configuration_t * configuration)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_provision_drive_configuration_t         set_configuration;

    set_configuration.configured_capacity = configuration->configured_capacity;
    set_configuration.configured_physical_block_size = configuration->configured_physical_block_size;
    set_configuration.config_type = configuration->config_type;
    set_configuration.sniff_verify_state = configuration->sniff_verify_state;	

    status = fbe_api_common_send_control_packet (FBE_PROVISION_DRIVE_CONTROL_CODE_SET_CONFIGURATION,
                                                 &set_configuration,
                                                 sizeof(fbe_provision_drive_configuration_t),
                                                 object_id,
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
    return status;
}


/*!****************************************************************************
 * @fn fbe_api_provision_drive_clear_verify_report(
 *       fbe_object_id_t   in_provision_drive_object_id,
 *       fbe_packet_attr_t in_attribute)
 ******************************************************************************
 *
 * @brief
 *    This function issues a sep control packet to clear the sniff verify
 *    report on the specified provision drive. 
 *
 * @param   in_provision_drive_object_id  -  provision drive object id
 * @param   in_attribute                  -  packet attribute flags
 *
 * @return  fbe_status_t                  -  status of this operation
 *
 * @version
 *    02/12/2010 - Created. Randy Black
 *
 ******************************************************************************/

fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_clear_verify_report(
                                             fbe_object_id_t   in_provision_drive_object_id,
                                             fbe_packet_attr_t in_attribute
                                           )
{
    fbe_api_control_operation_status_info_t  status_info;
    fbe_status_t                             status;    

    // issue sep control packet to clear sniff verify report on specified
    // provision drive; no buffer is needed to process this request
    status = fbe_api_common_send_control_packet( FBE_PROVISION_DRIVE_CONTROL_CODE_CLEAR_VERIFY_REPORT,
                                                 NULL,
                                                 0,
                                                 in_provision_drive_object_id,
                                                 in_attribute,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0
                                               );

    // check if any errors occurred for this request
    if ( (status != FBE_STATUS_OK) || (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) )
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n",
                       __FUNCTION__, status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier );

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}   // end fbe_api_provision_drive_clear_verify_report()

// fbe_api_provision_drive_disable_verify() replaced by fbe_api_job_service_update_pvd_sniff_verify()
// fbe_api_provision_drive_enable_verify() replaced by fbe_api_job_service_update_pvd_sniff_verify()


/*!****************************************************************************
 * @fn fbe_api_provision_drive_get_verify_report(
 *        fbe_object_id_t    in_provision_drive_object_id,
 *        fbe_packet_attr_t  in_attribute,
 *        fbe_provision_drive_verify_report_t* out_verify_report_p)
 ******************************************************************************
 *
 * @brief
 *    This function issues a sep control packet to get the sniff verify report
 *    for the specified provision drive.
 *
 * @param   in_provision_drive_object_id  -  provision drive object id
 * @param   in_attribute                  -  packet attribute flags
 * @param   out_verify_report_p           -  verify report output parameter
 *
 * @return  fbe_status_t                  -  status of this operation
 *
 * @version
 *    02/12/2010 - Created. Randy Black
 *
 ******************************************************************************/

fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_verify_report(
                                           fbe_object_id_t    in_provision_drive_object_id,
                                           fbe_packet_attr_t  in_attribute,
                                           fbe_provision_drive_verify_report_t* out_verify_report_p
                                         )
{
    fbe_api_control_operation_status_info_t  status_info;
    fbe_status_t                             status;    

    // check verify report output parameter
    if ( out_verify_report_p == NULL )
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: null buffer pointer\n", __FUNCTION__ );
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // issue sep control packet to get the sniff verify report
    status = fbe_api_common_send_control_packet( FBE_PROVISION_DRIVE_CONTROL_CODE_GET_VERIFY_REPORT,
                                                 out_verify_report_p,
                                                 sizeof (fbe_provision_drive_verify_report_t),
                                                 in_provision_drive_object_id,
                                                 in_attribute,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0
                                               );

    // check if any errors occurred for this request
    if ( (status != FBE_STATUS_OK) || (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) )
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n",
                       __FUNCTION__, status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier );

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}   // end fbe_api_provision_drive_get_verify_report()


/*!****************************************************************************
 * @fn fbe_api_provision_drive_get_verify_status(
 *       fbe_object_id_t    in_provision_drive_object_id,
 *       fbe_packet_attr_t  in_attribute,
 *       fbe_provision_drive_get_verify_status_t* out_verify_status_p)
 ******************************************************************************
 *
 * @brief
 *    This function issues a sep control packet to get sniff verify status for
 *    the specified provision drive.
 *
 * @param   in_provision_drive_object_id  -  provision drive object id
 * @param   in_attribute                  -  packet attribute flags
 * @param   out_verify_status_p           -  verify status output parameter
 *
 * @return  fbe_status_t                  -  status of this operation
 *
 * @version
 *    02/12/2010 - Created. Randy Black
 *
 ******************************************************************************/

fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_verify_status(
                                           fbe_object_id_t    in_provision_drive_object_id,
                                           fbe_packet_attr_t  in_attribute,
                                           fbe_provision_drive_get_verify_status_t* out_verify_status_p
                                         )
{
    fbe_api_control_operation_status_info_t  status_info;
    fbe_status_t                             status;    

    // check verify status output parameter
    if ( out_verify_status_p == NULL )
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: null buffer pointer\n", __FUNCTION__ );
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // issue sep control packet to get the sniff verify status
    status = fbe_api_common_send_control_packet( FBE_PROVISION_DRIVE_CONTROL_CODE_GET_VERIFY_STATUS,
                                                 out_verify_status_p,
                                                 sizeof (fbe_provision_drive_get_verify_status_t),
                                                 in_provision_drive_object_id,
                                                 in_attribute,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0
                                               );

    // check if any errors occurred for this request
    if ( (status != FBE_STATUS_OK) || (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) )
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n",
                       __FUNCTION__, status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier );

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}   // end fbe_api_provision_drive_get_verify_status()


/*!***************************************************************
 * @fn fbe_api_provision_drive_get_zero_checkpoint(
 *      fbe_object_id_t object_id, 
 *      fbe_lba_t * zero_checkpoint)
 ****************************************************************
 * @brief
 *  This function is for internal (debugging) use ONLY.
 *
 * @param object_id - object ID
 * @param zero_checkpoint - pointer to zero checkpoint that will be returned
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  12/09/09 - Created. Amit Dhaduk.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_zero_checkpoint(fbe_object_id_t object_id, fbe_lba_t * zero_checkpoint)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_provision_drive_get_zero_checkpoint_t       get_zero_checkpoint;

    get_zero_checkpoint.zero_checkpoint = 0;

    status = fbe_api_common_send_control_packet(FBE_PROVISION_DRIVE_CONTROL_CODE_GET_ZERO_CHECKPOINT,
                                                &get_zero_checkpoint,
                                                sizeof(fbe_provision_drive_get_zero_checkpoint_t),
                                                object_id,
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

    *zero_checkpoint = get_zero_checkpoint.zero_checkpoint;
    return status;
}

/*!****************************************************************************
 * @fn fbe_api_provision_drive_set_verify_checkpoint(
 *        fbe_object_id_t    in_provision_drive_object_id,
 *        fbe_packet_attr_t  in_attribute,
 *        fbe_lba_t          in_checkpoint)
 ******************************************************************************
 *
 * @brief
 *    This function issues a sep control packet to set a new sniff verify
 *    checkpoint for the specified provision drive.
 *
 * @param   in_provision_drive_object_id  -  provision drive object id
 * @param   in_attribute                  -  packet attribute flags
 * @param   in_checkpoint                 -  new sniff verify checkpoint
 *
 * @return  fbe_status_t                  -  status of this operation
 *
 * @version
 *    02/12/2010 - Created. Randy Black
 *
 ******************************************************************************/

fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_set_verify_checkpoint(
                                               fbe_object_id_t    in_provision_drive_object_id,
                                               fbe_packet_attr_t  in_attribute,
                                               fbe_lba_t          in_checkpoint
                                             )
{
    fbe_provision_drive_set_verify_checkpoint_t set_verify_checkpoint;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_status_t                                status;

    // set new sniff verify checkpoint
    set_verify_checkpoint.verify_checkpoint = in_checkpoint;

    // issue sep control packet to set the sniff verify checkpoint
    status = fbe_api_common_send_control_packet( FBE_PROVISION_DRIVE_CONTROL_CODE_SET_VERIFY_CHECKPOINT,
                                                 &set_verify_checkpoint,
                                                 sizeof (fbe_provision_drive_set_verify_checkpoint_t),
                                                 in_provision_drive_object_id,
                                                 in_attribute,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0
                                               );

    // check if any errors occurred for this request
    if ( (status != FBE_STATUS_OK) || (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) )
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n",
                       __FUNCTION__, status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier );

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}   // end fbe_api_provision_drive_set_verify_checkpoint()

/*!***************************************************************
 * @fn fbe_api_provision_drive_get_verify_invalidate_checkpoint(
 *      fbe_object_id_t object_id, 
 *      fbe_lba_t * verify_invalidate_checkpoint)
 ****************************************************************
 * @brief
 *  This function is for internal (debugging) use ONLY.
 *
 * @param object_id - object ID
 * @param zero_checkpoint - pointer to verify invalidate checkpoint that will be returned
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  03/17/12 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_verify_invalidate_checkpoint(fbe_object_id_t object_id, fbe_lba_t * verify_invalidate_checkpoint)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_provision_drive_get_verify_invalidate_checkpoint_t       get_verify_invalidate_checkpoint;

    get_verify_invalidate_checkpoint.verify_invalidate_checkpoint = 0;

    status = fbe_api_common_send_control_packet(FBE_PROVISION_DRIVE_CONTROL_CODE_GET_VERIFY_INVALIDATE_CHECKPOINT,
                                                &get_verify_invalidate_checkpoint,
                                                sizeof(fbe_provision_drive_get_verify_invalidate_checkpoint_t),
                                                object_id,
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

    *verify_invalidate_checkpoint = get_verify_invalidate_checkpoint.verify_invalidate_checkpoint;
    return status;
}

/*!****************************************************************************
 * @fn fbe_api_provision_drive_set_verify_invalidate_checkpoint(
 *        fbe_object_id_t    in_provision_drive_object_id,
 *        fbe_packet_attr_t  in_attribute,
 *        fbe_lba_t          in_checkpoint)
 ******************************************************************************
 *
 * @brief
 *    This function issues a sep control packet to set a new verify invalidate
 *    checkpoint for the specified provision drive.
 *
 * @param   in_provision_drive_object_id  -  provision drive object id
 * @param   in_checkpoint                 -  new verify invalidate checkpoint
 *
 * @return  fbe_status_t                  -  status of this operation
 *
 * @version
 *    03/17/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/

fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_set_verify_invalidate_checkpoint(fbe_object_id_t    in_provision_drive_object_id,
                                                         fbe_lba_t          in_checkpoint
                                                         )
{
    fbe_provision_drive_set_verify_invalidate_checkpoint_t set_verify_invalidate_checkpoint;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_status_t                                status;

    // set new sniff verify checkpoint
    set_verify_invalidate_checkpoint.verify_invalidate_checkpoint = in_checkpoint;

    // issue sep control packet to set the sniff verify checkpoint
    status = fbe_api_common_send_control_packet( FBE_PROVISION_DRIVE_CONTROL_CODE_SET_VERIFY_INVALIDATE_CHECKPOINT,
                                                 &set_verify_invalidate_checkpoint,
                                                 sizeof (fbe_provision_drive_set_verify_invalidate_checkpoint_t),
                                                 in_provision_drive_object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0
                                               );

    // check if any errors occurred for this request
    if ( (status != FBE_STATUS_OK) || (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) )
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n",
                       __FUNCTION__, status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier );

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}   // end fbe_api_provision_drive_set_verify_invalidate_checkpoint()


/*!***************************************************************
 * @fn fbe_api_provision_drive_set_zero_checkpoint(
 *       fbe_object_id_t object_id, 
 *       fbe_lba_t zero_checkpoint)
 ****************************************************************
 * @brief
 *  This function is for internal (debugging) use ONLY.
 *
 * @param object_id - object ID
 * @param zero_checkpoint - zero checkpoint
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  12/09/09 - Created. Peter Puhov
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_set_zero_checkpoint(fbe_object_id_t object_id, fbe_lba_t zero_checkpoint)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_provision_drive_control_set_zero_checkpoint_t set_zero_checkpoint;

    set_zero_checkpoint.background_zero_checkpoint = zero_checkpoint;

    status = fbe_api_common_send_control_packet (FBE_PROVISION_DRIVE_CONTROL_CODE_SET_ZERO_CHECKPOINT,
                                                    &set_zero_checkpoint,
                                                    sizeof(fbe_provision_drive_control_set_zero_checkpoint_t),
                                                    object_id,
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

    return status;
}

/*!***************************************************************
 * @fn fbe_api_provision_drive_initiate_disk_zeroing(
 *      fbe_object_id_t             provision_drive_object_id)
 ****************************************************************
 * @brief
 *  This function initiates a disk zeroing operation for provision drive object.
 *
 * @param provision_drive_object_id      - The provision drive object ID
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  1/18/2010 - Created.  Amit Dhaduk
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_provision_drive_initiate_disk_zeroing(
                            fbe_object_id_t             provision_drive_object_id)
{

    fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;


    /* this request does not required to pass buffer information */
    status = fbe_api_common_send_control_packet (FBE_PROVISION_DRIVE_CONTROL_CODE_INITIATE_DISK_ZEROING,
                                                NULL,
                                                0,
                                                provision_drive_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}   
/***************************************************************
 * end fbe_api_provision_drive_initiate_disk_zeroing()
 ***************************************************************/
/*!***************************************************************
 * @fn fbe_api_provision_drive_initiate_consumed_area_zeroing(
 *      fbe_object_id_t             provision_drive_object_id)
 ****************************************************************
 * @brief
 *  This function initiates a consumed area zeroing operation for provision drive object.
 *
 * @param provision_drive_object_id      - The provision drive object ID
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_provision_drive_initiate_consumed_area_zeroing(
                            fbe_object_id_t             provision_drive_object_id)
{

    fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;


    /* this request does not required to pass buffer information */
    status = fbe_api_common_send_control_packet (FBE_PROVISION_DRIVE_CONTROL_CODE_INITIATE_CONSUMED_AREA_ZEROING,
                                                NULL,
                                                0,
                                                provision_drive_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}   
/***************************************************************
 * end fbe_api_provision_drive_initiate_consumed_area_zeroing()
 ***********************************************************/

/*!***************************************************************
 * @fn fbe_api_provision_drive_get_obj_id_by_location_from_topology(
 *       fbe_u32_t port,
 *       fbe_u32_t encl,
 *       fbe_u32_t slot,
 *       fbe_object_id_t *object_id)
 ****************************************************************
 * @brief 
 *    get the object of a PVD in the desired location that represent the PDO location
 *
 * @param port - port
 * @param encl - enclosure
 * @param slot - slot of PDO
 * @param object_id - pointer to the object ID that will be returned
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  14/05/2013 - Created.  Jian Gao
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_provision_drive_get_obj_id_by_location_from_topology(fbe_u32_t port,
                                                                         fbe_u32_t encl,
                                                                         fbe_u32_t slot,
                                                                         fbe_object_id_t *object_id)
{
    fbe_status_t                                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t                     status_info = {0};
    fbe_topology_control_get_provision_drive_id_by_location_t   get_pvd;

    get_pvd.port_number = port;
    get_pvd.enclosure_number = encl;
    get_pvd.slot_number = slot;

    /* this request does not required to pass buffer information */
    status = fbe_api_common_send_control_packet (FBE_TOPOLOGY_CONTROL_CODE_GET_PROVISION_DRIVE_BY_LOCATION,
                                                 &get_pvd,
                                                 sizeof(fbe_topology_control_get_provision_drive_id_by_location_t),
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "PVD get_obj_id_by_location: %d_%d_%d, pkt err:%d, pkt qual:%d, pyld err:%d, pyld qual:%d\n", 
                       port, encl, slot, status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    *object_id = get_pvd.object_id;

    return status;
}


/*!***************************************************************
 * @fn fbe_api_provision_drive_get_obj_id_by_location(
 *       fbe_u32_t port,
 *       fbe_u32_t encl,
 *       fbe_u32_t slot,
 *       fbe_object_id_t *object_id)
 ****************************************************************
 * @brief 
 *    get the object of a PVD in the desired location that represent the PDO location
 *
 * @param port - port
 * @param encl - enclosure
 * @param slot - slot of PDO
 * @param object_id - pointer to the object ID that will be returned
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  2/04/2010 - Created.  Shay Harel
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_provision_drive_get_obj_id_by_location(fbe_u32_t port,
                                                                         fbe_u32_t encl,
                                                                         fbe_u32_t slot,
                                                                         fbe_object_id_t *object_id)
{
    fbe_status_t                                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t                     status_info = {0};
    fbe_topology_control_get_provision_drive_id_by_location_t   get_pvd;

    get_pvd.port_number = port;
    get_pvd.enclosure_number = encl;
    get_pvd.slot_number = slot;

    /* this request does not required to pass buffer information */
    status = fbe_api_common_send_control_packet (FBE_TOPOLOGY_CONTROL_CODE_GET_PROVISION_DRIVE_BY_LOCATION_FROM_NON_PAGED,
                                                 &get_pvd,
                                                 sizeof(fbe_topology_control_get_provision_drive_id_by_location_t),
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "PVD get_obj_id_by_location: packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    *object_id = get_pvd.object_id;

    return status;
}

fbe_status_t FBE_API_CALL
fbe_api_provision_drive_metadata_paged_write(fbe_object_id_t object_id, 
                                                fbe_api_provisional_drive_paged_metadata_t * paged_write)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_provision_drive_control_paged_metadata_t  paged_metadata_write;

    paged_metadata_write.metadata_offset = paged_write->metadata_offset;
    paged_metadata_write.metadata_record_data_size = paged_write->metadata_record_data_size;
    paged_metadata_write.metadata_repeat_count = paged_write->metadata_repeat_count;

    paged_metadata_write.metadata_bits.valid_bit = paged_write->metadata_bits.valid_bit;
    paged_metadata_write.metadata_bits.need_zero_bit = paged_write->metadata_bits.need_zero_bit;
    paged_metadata_write.metadata_bits.user_zero_bit = paged_write->metadata_bits.user_zero_bit;
    paged_metadata_write.metadata_bits.consumed_user_data_bit = paged_write->metadata_bits.consumed_user_data_bit;
    paged_metadata_write.metadata_bits.unused_bit = paged_write->metadata_bits.unused_bit;


    status = fbe_api_common_send_control_packet (FBE_PROVISION_DRIVE_CONTROL_CODE_METADATA_PAGED_WRITE,
                                                &paged_metadata_write,
                                                sizeof(fbe_provision_drive_control_paged_metadata_t),
                                                object_id,
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

    return status;
}

/*!***************************************************************
 * @fn fbe_api_provision_drive_get_background_priorities(fbe_object_id_t object_id,
  fbe_provision_drive_set_priorities_t* get_priorities)
 ****************************************************************
 * @brief
 *  This function get the priorities the provisoned drive is using
 *  when doing background operations
 *
 * @param object_id - object ID
 * @param get_priorities - pointer to structure to get data
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_provision_drive_get_background_priorities(fbe_object_id_t object_id,
                                                                            fbe_provision_drive_set_priorities_t* get_priorities)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    if (get_priorities == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_PROVISION_DRIVE_CONTROL_CODE_GET_BACKGROUND_PRIORITIES,
                                                get_priorities,
                                                sizeof(fbe_provision_drive_set_priorities_t),
                                                object_id,
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

    return status;

}

/*!***************************************************************
 * @fn fbe_api_provision_drive_set_background_priorities(fbe_object_id_t object_id,
  fbe_provision_drive_set_priorities_t* set_priorities)
 ****************************************************************
  * @brief
 *  This function set the priorities the provisoned drive is using
 *  when doing background operations
 *
 * @param object_id - object ID
 * @param get_priorities - pointer to structure to set data
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_provision_drive_set_background_priorities(fbe_object_id_t object_id,
                                                                            fbe_provision_drive_set_priorities_t* set_priorities)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    if (set_priorities == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_PROVISION_DRIVE_CONTROL_CODE_SET_BACKGROUND_PRIORITIES,
                                                set_priorities,
                                                sizeof(fbe_provision_drive_set_priorities_t),
                                                object_id,
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

    return status;

}

/*!***************************************************************
 * @fn fbe_api_provision_drive_get_location(
 *     fbe_object_id_t object_id, 
 *     fbe_port_number_t * port_number,
 *     fbe_enclosure_number_t * enclosure_number,
 *     fbe_enclosure_slot_number_t * slot_number)
 *****************************************************************
 * @brief
 *   This function returns the drive location(port\Encl\slot) of the PVD object.
 *
 * @param object_id     - object to query.
 * @param port_number   - what port object is on.
 * @param enclosure_number   - what enclosure_number object is on.
 * @param slot_number   - slot number return.
 *
 * @return fbe_status_t - success or failure.
 *
 * @version
 *  04/30/10: shanmugam    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_provision_drive_get_location(fbe_object_id_t object_id, 
                                                                fbe_u32_t * port_number, 
                                                                fbe_u32_t * enclosure_number, 
                                                                fbe_u32_t * slot_number)
{
    fbe_status_t                                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_provision_drive_get_physical_drive_location_t   pvd_get_drive_location;
    fbe_api_control_operation_status_info_t             status_info;

    /* initialize port enclosure slot as invalid. */
    pvd_get_drive_location.port_number = FBE_PORT_NUMBER_INVALID;
    pvd_get_drive_location.enclosure_number = FBE_ENCLOSURE_NUMBER_INVALID;
    pvd_get_drive_location.slot_number = FBE_SLOT_NUMBER_INVALID;

    /* send provision drive get location to get the drive location. */
    status = fbe_api_common_send_control_packet (FBE_PROVISION_DRIVE_CONTROL_CODE_GET_DRIVE_LOCATION,
                                                 &pvd_get_drive_location,
                                                 sizeof (fbe_provision_drive_get_physical_drive_location_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_TRAVERSE,
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
   
    *port_number = pvd_get_drive_location.port_number;
    *enclosure_number = pvd_get_drive_location.enclosure_number;
    *slot_number = pvd_get_drive_location.slot_number;
    fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:got port number:%d encl number:%d slot number:%d for obj:0x%X\n", __FUNCTION__, *port_number, *enclosure_number, *slot_number, object_id);

    return status;
}

/*!***************************************************************
 * @fn fbe_api_provision_drive_get_info(
 *     fbe_object_id_t object_id, 
 *     fbe_provision_drive_info_t *provision_drive_info_p)
 *****************************************************************
 * @brief
 *   This function returns the basic info of the PVD object.
 *
 * @param object_id     - object to query.
 * @param provision_drive_info_p   - pvd drive info return.
 *
 * @return fbe_status_t - success or failure.
 *
 * @version
 *  08/10/10: nchiu    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_provision_drive_get_info(fbe_object_id_t object_id, 
                                                      fbe_api_provision_drive_info_t * provision_drive_info_p)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_provision_drive_info_t              provision_drive_info;
    fbe_api_control_operation_status_info_t status_info;
    
    status = fbe_api_common_send_control_packet (FBE_PROVISION_DRIVE_CONTROL_CODE_GET_PROVISION_DRIVE_INFO,
                                                 &provision_drive_info,
                                                 sizeof (fbe_provision_drive_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_TRAVERSE|FBE_PACKET_FLAG_INTERNAL,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
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

    provision_drive_info_p->capacity = provision_drive_info.capacity;
    provision_drive_info_p->config_type = provision_drive_info.config_type;
    provision_drive_info_p->configured_physical_block_size = provision_drive_info.configured_physical_block_size;
    provision_drive_info_p->paged_metadata_lba = provision_drive_info.paged_metadata_lba;
    provision_drive_info_p->paged_metadata_capacity = provision_drive_info.paged_metadata_capacity;
    provision_drive_info_p->paged_mirror_offset = provision_drive_info.paged_mirror_offset;
    provision_drive_info_p->total_chunks = provision_drive_info.total_chunks;
    provision_drive_info_p->chunk_size = provision_drive_info.chunk_size;
    provision_drive_info_p->port_number = provision_drive_info.port_number;
    provision_drive_info_p->enclosure_number = provision_drive_info.enclosure_number;
    provision_drive_info_p->slot_number = provision_drive_info.slot_number;
    provision_drive_info_p->drive_type = provision_drive_info.drive_type;
    fbe_copy_memory(provision_drive_info_p->serial_num,
                    provision_drive_info.serial_num,
                    FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1);
    provision_drive_info_p->max_drive_xfer_limit = provision_drive_info.max_drive_xfer_limit;
    provision_drive_info_p->debug_flags = provision_drive_info.debug_flags;
    provision_drive_info_p->end_of_life_state = provision_drive_info.end_of_life_state;
    provision_drive_info_p->media_error_lba = provision_drive_info.media_error_lba;
    provision_drive_info_p->zero_on_demand = provision_drive_info.zero_on_demand;
    provision_drive_info_p->is_system_drive = provision_drive_info.is_system_drive;
    provision_drive_info_p->default_offset = provision_drive_info.default_offset;
    provision_drive_info_p->default_chunk_size = provision_drive_info.default_chunk_size;
    provision_drive_info_p->zero_checkpoint = provision_drive_info.zero_checkpoint;
    provision_drive_info_p->slf_state = provision_drive_info.slf_state;
    provision_drive_info_p->spin_down_qualified = provision_drive_info.spin_down_qualified;
    provision_drive_info_p->drive_fault_state = provision_drive_info.drive_fault_state;
    provision_drive_info_p->power_save_capable = provision_drive_info.power_save_capable;
    provision_drive_info_p->percent_rebuilt = provision_drive_info.percent_rebuilt;
    provision_drive_info_p->created_after_encryption_enabled = provision_drive_info.created_after_encryption_enabled;
    provision_drive_info_p->scrubbing_in_progress = provision_drive_info.scrubbing_in_progress;
    provision_drive_info_p->swap_pending = provision_drive_info.swap_pending;
    provision_drive_info_p->flags = provision_drive_info.flags;

    return status;
}

/*!***************************************************************
 * @fn fbe_api_provision_drive_get_metadata_memory(
 *      fbe_object_id_t object_id, 
 *      fbe_provision_drive_control_get_metadata_memory_t * provisional_drive_control_get_metadata_memory)
 ****************************************************************
 * @brief
 *  This function is for internal (debugging) use ONLY.
 *
 * @param object_id - object ID
 * @param zero_checkpoint - pointer to zero checkpoint that will be returned
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  06/08/2010 - Created. Peter Puhov
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_metadata_memory(fbe_object_id_t object_id, 
                                            fbe_provision_drive_control_get_metadata_memory_t * provisional_drive_control_get_metadata_memory)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;


    status = fbe_api_common_send_control_packet (FBE_PROVISION_DRIVE_CONTROL_CODE_GET_METADATA_MEMORY,
                                                    provisional_drive_control_get_metadata_memory,
                                                    sizeof(fbe_provision_drive_control_get_metadata_memory_t),
                                                    object_id,
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

    return status;
}


/*!***************************************************************
 * @fn fbe_api_provision_drive_get_versioned_metadata_memory(
 *      fbe_object_id_t object_id, 
 *      fbe_provision_drive_control_get_versioned_metadata_memory_t * provisional_drive_control_get_metadata_memory)
 ****************************************************************
 * @brief
 *  This function is for internal (debugging and test) use ONLY.
 *
 * @param object_id - object ID
 * @param provisional_drive_control_get_metadata_memory - pointer to get metadata memory buffer
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  04/24/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_versioned_metadata_memory(fbe_object_id_t object_id, 
                                            fbe_provision_drive_control_get_versioned_metadata_memory_t * provisional_drive_control_get_metadata_memory)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;


    status = fbe_api_common_send_control_packet (FBE_PROVISION_DRIVE_CONTROL_CODE_GET_VERSIONED_METADATA_MEMORY,
                                                    provisional_drive_control_get_metadata_memory,
                                                    sizeof(fbe_provision_drive_control_get_versioned_metadata_memory_t),
                                                    object_id,
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

    return status;
}



/*!***************************************************************
 * @fn fbe_api_provision_drive_set_debug_flag(
 *       fbe_object_id_t object_id, 
 *       fbe_u32_t in_flag)
 ****************************************************************
 * @brief
 *  This function is for internal (debugging) use ONLY.
 *
 * This API is used to set PVD debug tracing flag to enhance debugging functionality.
 *  User can also set multiple flags at a time. 
 *  e.g. use value of "FBE_PROVISION_DRIVE_DEBUG_TRACE_BGZ | FBE_PROVISION_DRIVE_DEBUG_TRACE_ZOD"
 *        to set debug tracing flag for background zeroing and zero on demand togather.
 *
 * @param object_id - object ID
 * @param in_flag - debug flag
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  09/07/10 - Created. Amit Dhaduk
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_set_debug_flag(fbe_object_id_t in_object_id,
                                    fbe_provision_drive_debug_flags_t in_flag)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_provision_drive_set_debug_trace_flag_t     debug_trace_flag;
    

    debug_trace_flag.trace_flag = in_flag;
    status = fbe_api_common_send_control_packet (FBE_PROVISION_DRIVE_CONTROL_CODE_SET_DEBUG_FLAGS,        
                                                    &debug_trace_flag,
                                                    sizeof(fbe_provision_drive_set_debug_trace_flag_t),
                                                    in_object_id,
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

    return status;
} 
/***************************************************************
 * end fbe_api_provision_drive_set_debug_flag
 ***************************************************************/

/*!***************************************************************
 * @fn fbe_api_provision_drive_set_class_debug_flag(
 *        fbe_provision_drive_debug_flags_t in_flag)
 ****************************************************************
 * @brief
 *  This function sends request to set debug flag at pvd class level.
 *
 * @param in_flag - debug flag to set at pvd class level
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  09/20/2010 - Created. Amit Dhaduk
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_set_class_debug_flag(fbe_provision_drive_debug_flags_t in_flag)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_provision_drive_set_debug_trace_flag_t     debug_trace_flag;

    debug_trace_flag.trace_flag = in_flag;

    status = fbe_api_common_send_control_packet_to_class (FBE_PROVISION_DRIVE_CONTROL_CODE_SET_CLASS_DEBUG_FLAGS,
                                                            &debug_trace_flag,
                                                            sizeof(fbe_provision_drive_set_debug_trace_flag_t),
                                                            FBE_CLASS_ID_PROVISION_DRIVE,
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


    return status;
} 
/***************************************************************
 * end fbe_api_provision_drive_set_class_debug_flag
 ***************************************************************/

/*!****************************************************************************
 * fbe_api_provision_drive_set_config_hs_flag()
 ******************************************************************************
 * @brief
 * When FBE_TRUE is passed in, the system will consider ONLY drives
 * that were marked as HS when building a list of HS candidates
 *
 *  @param 
 *
 * @return status - The status of the operation.
 *
 * @author
 *  3/09/2012 - Created. Vera Wang
 ******************************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_set_config_hs_flag(fbe_bool_t enable)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_topology_mgmt_set_pvd_hs_flag_t				set_flag;

    set_flag.enable = enable;

    /* send the control packet and wait for its completion. */
    status = fbe_api_common_send_control_packet(FBE_TOPOLOGY_CONTROL_CODE_SET_PVD_HS_FLAG_CONTROL,
                                                &set_flag,
                                                sizeof(fbe_topology_mgmt_set_pvd_hs_flag_t),
                                                FBE_OBJECT_ID_INVALID,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                NULL,
                                                FBE_PACKAGE_ID_SEP_0);
    
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d.\n", __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/***************************************************************
 * end fbe_api_provision_drive_set_config_pvd_as_hs_flag_to_true
 ***************************************************************/

/*!****************************************************************************
 * fbe_api_provision_drive_get_config_hs_flag()
 ******************************************************************************
 * @brief
 * When FBE_TRUE is passed in, the system will consider ONLY drives
 * that were marked as HS when building a list of HS candidates
 *
 *  @param b_is_test_spare_config_set_p - Pointer to `is test spare config' bool.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  3/09/2012 - Created. Vera Wang
 ******************************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_config_hs_flag(fbe_bool_t *b_is_test_spare_config_set_p)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_topology_mgmt_get_pvd_hs_flag_t				test_spare_config_set;

    /* Initialize output to default.*/
    *b_is_test_spare_config_set_p = FBE_FALSE;

    /* send the control packet and wait for its completion. */
    status = fbe_api_common_send_control_packet(FBE_TOPOLOGY_CONTROL_CODE_GET_PVD_HS_FLAG_CONTROL,
                                                &test_spare_config_set,
                                                sizeof(fbe_topology_mgmt_get_pvd_hs_flag_t),
                                                FBE_OBJECT_ID_INVALID,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                NULL,
                                                FBE_PACKAGE_ID_SEP_0);
    
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d.\n", __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *b_is_test_spare_config_set_p = test_spare_config_set.b_is_test_spare_enabled;

    return status;
}
/***************************************************************
 * end fbe_api_provision_drive_get_config_hs_flag
 ***************************************************************/

/*!****************************************************************************
 * fbe_api_provision_drive_get_spare_drive_pool()
 ******************************************************************************
 * @brief
 * This function sends the GET spare drive pool control packet to topology 
 * service and receives the list of PVD object ID which can be configured as 
 * spare.
 *
 *  @param in_spare_drive_pool_p - Spare drive pool pointer.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  14/01/2011 - Created. Vishnu Sharma
 ******************************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_spare_drive_pool(fbe_api_provisional_drive_get_spare_drive_pool_t *in_spare_drive_pool_p)
{
    fbe_u32_t                                   index = 0;
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_topology_control_get_spare_drive_pool_t topology_spare_drive_pool;
    fbe_bool_t                                  b_is_test_spare_enabled = FBE_FALSE;


    /* Determine the type of spare drive to populate pool with.*/
    status = fbe_api_provision_drive_get_config_hs_flag(&b_is_test_spare_enabled);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: Failed to get hs flag. status: 0x%x\n",
                      __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (b_is_test_spare_enabled == FBE_FALSE)
    {
        topology_spare_drive_pool.desired_spare_drive_type = FBE_TOPOLOGY_GET_SPARE_DRIVE_TYPE_UNCONSUMED;
    }
    else
    {
        topology_spare_drive_pool.desired_spare_drive_type = FBE_TOPOLOGY_GET_SPARE_DRIVE_TYPE_TEST_SPARE;
    }

    /* initialize the topology spare drive pool request. */
    topology_spare_drive_pool.number_of_spare = 0;
    for (index = 0; index < FBE_MAX_SPARE_OBJECTS; index++)
    {
        topology_spare_drive_pool.spare_object_list[index] = FBE_OBJECT_ID_INVALID;
    }

    /* send the control packet and wait for its completion. */
    status = fbe_api_common_send_control_packet(FBE_TOPOLOGY_CONTROL_CODE_GET_SPARE_DRIVE_POOL,
                                                     &topology_spare_drive_pool,
                                                     sizeof(fbe_topology_control_get_spare_drive_pool_t),
                                                     FBE_OBJECT_ID_INVALID,
                                                     FBE_PACKET_FLAG_NO_ATTRIB,
                                                     &status_info,
                                                     FBE_PACKAGE_ID_SEP_0);
    
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    in_spare_drive_pool_p->number_of_spare = topology_spare_drive_pool.number_of_spare;
    fbe_copy_memory (in_spare_drive_pool_p->spare_object_list, 
                     topology_spare_drive_pool.spare_object_list,
                     (sizeof(fbe_object_id_t) * topology_spare_drive_pool.number_of_spare));

    return status;
}

/***************************************************************
 * end fbe_api_provision_drive_get_spare_drive_pool
 ***************************************************************/



/*!****************************************************************************
 * @fn fbe_api_provision_drive_set_eol_state(
 *       fbe_object_id_t   in_provision_drive_object_id,
 *       fbe_packet_attr_t in_attribute)
 ******************************************************************************
 *
 * @brief
 *    This function issues a sep control packet to set end of life state
 *    for the specified provision drive. 
 *
 * @param   in_provision_drive_object_id  -  provision drive object id
 * @param   in_attribute                  -  packet attribute flags
 *
 * @return  fbe_status_t                  -  status of this operation
 *
 * @version
 *    02/28/2011 - Created. Vishnu Sharma
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_set_eol_state(fbe_object_id_t   in_provision_drive_object_id,
                                      fbe_packet_attr_t in_attribute)
{
    fbe_api_control_operation_status_info_t  status_info;
    fbe_status_t                             status;    

    // issue sep control packet to set EOL  on specified
    // provision drive; no buffer is needed to process this request
    status = fbe_api_common_send_control_packet( FBE_PROVISION_DRIVE_CONTROL_CODE_SET_EOL_STATE,
                                                 NULL,
                                                 0,
                                                 in_provision_drive_object_id,
                                                 in_attribute,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0
                                               );

    // check if any errors occurred for this request
    if ( (status != FBE_STATUS_OK) || (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) )
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n",
                       __FUNCTION__, status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier );

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}   
/***************************************************************
 * end fbe_api_provision_drive_set_eol_state()
 ***************************************************************/

/*!****************************************************************************
 * @fn fbe_api_provision_drive_clear_eol_state(
 *       fbe_object_id_t   in_provision_drive_object_id,
 *       fbe_packet_attr_t in_attribute)
 ******************************************************************************
 *
 * @brief
 *    This function issues a sep control packet to clear the end of life condition
 *    for the specified provision drive. 
 *
 * @param   in_provision_drive_object_id  -  provision drive object id
 * @param   in_attribute                  -  packet attribute flags
 *
 * @return  fbe_status_t                  -  status of this operation
 *
 * @version
 *  03/27/2012  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_clear_eol_state(fbe_object_id_t   in_provision_drive_object_id,
                                      fbe_packet_attr_t in_attribute)
{
    fbe_api_control_operation_status_info_t  status_info;
    fbe_status_t                             status;    

    // issue sep control packet to clear EOL  on specified
    // provision drive; no buffer is needed to process this request
    status = fbe_api_common_send_control_packet( FBE_PROVISION_DRIVE_CONTROL_CODE_CLEAR_EOL_STATE,
                                                 NULL,
                                                 0,
                                                 in_provision_drive_object_id,
                                                 in_attribute,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0
                                               );

    // check if any errors occurred for this request
    if ( (status != FBE_STATUS_OK) || (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) )
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n",
                       __FUNCTION__, status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier );

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}
/***************************************************************
 * end fbe_api_provision_drive_clear_eol_state()
 ***************************************************************/

/*!***************************************************************
 * @fn fbe_api_provision_drive_send_download_asynch()
 ****************************************************************
 * @brief
 *  This function sends the DOWNLOAD request to PVD.
 *
 * @param object_id - object ID
 * @param fw_info - pointer to download context
 *
 * @return
 *  fbe_status_t - status
 *
 * @version
 *  03/09/2011 - Created. chenl6
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_send_download_asynch(fbe_object_id_t                       object_id,
                                             fbe_api_provision_drive_send_download_asynch_t * fw_info)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                      packet = NULL;
    fbe_payload_ex_t *                 payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_provision_drive_send_download_t * download_info = &(fw_info->download_info);

    /* Allocate packet.*/
    packet = (fbe_packet_t *) fbe_api_allocate_contiguous_memory (sizeof (fbe_packet_t));
    if (packet == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s: unable to allocate memory for packet\n", __FUNCTION__); 
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_transport_initialize_sep_packet(packet);
    payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

    fbe_payload_control_build_operation (control_operation,
                                         FBE_PROVISION_DRIVE_CONTROL_CODE_SEND_DOWNLOAD,
                                         download_info,
                                         sizeof(fbe_provision_drive_send_download_t));

    fbe_payload_ex_set_sg_list (payload, fw_info->download_sg_list, 1);

    fbe_transport_set_completion_function (packet, fbe_api_provision_drive_send_download_async_callback, fw_info);
    

    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id); 
    
    /* Mark packet with the attribute, either traverse or not */
    fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_NO_ATTRIB);

    status = fbe_api_common_send_control_packet_to_driver(packet);

    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
        /* Only trace when it is an error we do not expect.*/
         fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s error in sending packet, status:%d\n", __FUNCTION__, status);  
    }
    
    return FBE_STATUS_OK;    
}

/*!***************************************************************
 * @fn fbe_api_provision_drive_send_download_async_callback()
 ****************************************************************
 * @brief
 *  This function processes the callback of DOWNLOAD request.
 *
 * @param packet - pinter to packet
 * @param context - pointer to context
 *
 * @return
 *  fbe_status_t - status
 *
 * @version
 *  03/09/2011 - Created. chenl6
 ****************************************************************/
static fbe_status_t 
fbe_api_provision_drive_send_download_async_callback (fbe_packet_t * packet, 
                                                   fbe_packet_completion_context_t context)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_api_provision_drive_send_download_asynch_t *   fw_info = 
            (fbe_api_provision_drive_send_download_asynch_t *)context;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *       control_operation = NULL;
    fbe_payload_control_status_t            control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_payload_control_status_qualifier_t  control_status_qualifier = 0;

    status = fbe_transport_get_status_code(packet);
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation, &control_status_qualifier);

    fw_info->completion_status = FBE_STATUS_OK;
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
    fbe_transport_destroy_packet(packet);
    fbe_api_free_contiguous_memory(packet);

    /* call callback function */
    (fw_info->completion_function) (context);
    
    return FBE_STATUS_OK;
}


/*!***************************************************************
 * @fn fbe_api_provision_drive_abort_download()
 ****************************************************************
 * @brief
 *  This function sends abort DOWNLOAD request to PVD.
 *
 * @param object_id - object ID
 *
 * @return
 *  fbe_status_t - status
 *
 * @version
 *  03/09/2011 - Created. chenl6
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_abort_download(fbe_object_id_t object_id)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t  status_info;

    /* send the control packet and wait for its completion. */
    status = fbe_api_common_send_control_packet(FBE_PROVISION_DRIVE_CONTROL_CODE_ABORT_DOWNLOAD,
                                                NULL,
                                                0,
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);
    
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
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
 * @fn fbe_api_provision_drive_get_download_info()
 ****************************************************************
 * @brief
 *  This function sends abort DOWNLOAD request to PVD.
 *
 * @param object_id - object ID
 * @param get_download_p - ptr to information structure
 *
 * @return
 *  fbe_status_t - status
 *
 * @version
 *  10/31/2011 - Created. chenl6
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_download_info(fbe_object_id_t object_id, fbe_provision_drive_get_download_info_t * get_download_p)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_provision_drive_get_download_info_t get_download_info;
    fbe_api_control_operation_status_info_t status_info;
    
    status = fbe_api_common_send_control_packet (FBE_PROVISION_DRIVE_CONTROL_CODE_GET_DOWNLOAD_INFO,
                                                 &get_download_info,
                                                 sizeof (fbe_provision_drive_get_download_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    get_download_p->download_originator = get_download_info.download_originator;
    get_download_p->local_state = get_download_info.local_state;
    get_download_p->clustered_flag = get_download_info.clustered_flag;
    get_download_p->peer_clustered_flag = get_download_info.peer_clustered_flag;

    return status;
}


/*!***************************************************************
 * @fn fbe_api_provision_drive_get_health_check_info()
 ****************************************************************
 * @brief
 *  This function gets Health Check state info from PVD.
 *
 * @param object_id - PVD object ID
 * @param get_health_check_p - ptr to information structure
 *
 * @return
 *  fbe_status_t - status
 *
 * @version
 *  11/26/2013 - Created. Wayne Garrett
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_health_check_info(fbe_object_id_t object_id, fbe_provision_drive_get_health_check_info_t * get_health_check_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_provision_drive_get_health_check_info_t get_health_check_info = {0};
    fbe_api_control_operation_status_info_t status_info = {0};

    status = fbe_api_common_send_control_packet (FBE_PROVISION_DRIVE_CONTROL_CODE_GET_HEALTH_CHECK_INFO,
                                                 &get_health_check_info,
                                                 sizeof (fbe_provision_drive_get_health_check_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    get_health_check_p->originator = get_health_check_info.originator;
    get_health_check_p->local_state = get_health_check_info.local_state;
    get_health_check_p->clustered_flag = get_health_check_info.clustered_flag;
    get_health_check_p->peer_clustered_flag = get_health_check_info.peer_clustered_flag;

    return status;
}


/*!****************************************************************************
 * fbe_api_provision_drive_update_config_type()
 ******************************************************************************
 * @brief
 * This function creates/unconfigures HS.
 *
 *  @param update_pvd_config_type_p - Update PVD config data pointer.
 *
 * @return status - The status of the operation.
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_update_config_type(fbe_api_provision_drive_update_config_type_t*  update_pvd_config_type_p)
{
    fbe_status_t                                              status = FBE_STATUS_OK;
    fbe_api_job_service_update_provision_drive_config_type_t  update_pvd_config_type_job = {0};
    fbe_job_service_error_type_t                              error_type = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    fbe_status_t                                              job_status = FBE_STATUS_OK;

    /* fill in all the information we need */
    update_pvd_config_type_job.config_type = update_pvd_config_type_p->config_type;
    update_pvd_config_type_job.pvd_object_id = update_pvd_config_type_p->pvd_object_id;
    
    fbe_copy_memory(&update_pvd_config_type_job.opaque_data,
                    update_pvd_config_type_p->opaque_data,
                    FBE_DATABASE_PROVISION_DRIVE_OPAQUE_DATA_MAX);
    
    /* start the job */
    status = fbe_api_job_service_update_provision_drive_config_type(&update_pvd_config_type_job);
    

    if (status != FBE_STATUS_OK) 
    {
        if (update_pvd_config_type_job.config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "_update_config_type: Failed to create HS. PVD=0x%x,Status=0x%x\n", 
                          update_pvd_config_type_job.pvd_object_id, status);
        }
        else
        {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "_update_config_type: Failed to unconfigure HS. PVD=0x%x,Status=0x%x\n", 
                          update_pvd_config_type_job.pvd_object_id, status);
        }

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* wait for it */
    status = fbe_api_common_wait_for_job(update_pvd_config_type_job.job_number, 
                                         PVD_UPDATE_CONFIG_TYPE_WAIT_TIMEOUT, 
                                         &error_type, 
                                         &job_status,
                                         NULL);

    if ((status != FBE_STATUS_OK) || (job_status != FBE_STATUS_OK) || (error_type != FBE_JOB_SERVICE_ERROR_NO_ERROR)) 
    {
        /* Don't report an error if the drive is already configured as the
         * requested type.
         */
        if ((status == FBE_STATUS_OK)     &&
            (job_status == FBE_STATUS_OK)    )
        {
            /* If the drive is already configured as the requested type print a
             * warning.
             */
            if ((update_pvd_config_type_p->config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED) &&
                (error_type == FBE_JOB_SERVICE_ERROR_PVD_IS_CONFIGURED_AS_UNCONSUMED)                    )
            {
                fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                              "_update_config_type: error_type: %d pvd: 0x%x already configured as: %d\n",
                              error_type, update_pvd_config_type_job.pvd_object_id, update_pvd_config_type_p->config_type);
                return FBE_STATUS_OK;
            }
            if ((update_pvd_config_type_p->config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE) &&
                (error_type == FBE_JOB_SERVICE_ERROR_PVD_IS_CONFIGURED_AS_SPARE)                    )
            {
                fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                              "_update_config_type: error_type: %d pvd: 0x%x already configured as: %d\n",
                              error_type, update_pvd_config_type_job.pvd_object_id, update_pvd_config_type_p->config_type);
                return FBE_STATUS_OK;
            }
        }

        /* Else unexpected error
         */
        if (update_pvd_config_type_job.config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                          "_update_config_type: Wait for creating HS failed. PVD=0x%X, Status=0x%X, Job Status=0x%X, Job Error=0x%x\n",
                          update_pvd_config_type_job.pvd_object_id, status, job_status, error_type);
        }
        else
        {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                          "_update_config_type: Wait for unconfiguring HS failed. PVD=0x%X, Status=0x%X, Job Status=0x%X, Job Error=0x%x\n",
                          update_pvd_config_type_job.pvd_object_id, status, job_status, error_type);
        }

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

} // end fbe_api_provision_drive_update_config_type()

/*!****************************************************************************
 * fbe_api_provision_drive_get_max_opaque_data_size()
 ******************************************************************************
 * @brief
 * This function returns the max size the opaque data can be.
 *
 *  @param max_opaque_data_size - OUT the returned size.
 *
 * @return status - The status of the operation.
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_max_opaque_data_size(fbe_u32_t * max_opaque_data_size)
{
    *max_opaque_data_size = FBE_DATABASE_PROVISION_DRIVE_OPAQUE_DATA_MAX;
    return FBE_STATUS_OK;
} // end fbe_api_provision_drive_get_max_opaque_data_size()

/*!****************************************************************************
 * fbe_api_provision_drive_get_opaque_data()
 ******************************************************************************
 * @brief
 * This function returns the max size the opaque data can be.
 *
 *  @param max_opaque_data_size - OUT the returned size.
 *
 * @return status - The status of the operation.
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_opaque_data(fbe_object_id_t object_id, fbe_u32_t max_opaque_data_size, fbe_u8_t * opaque_data_ptr)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t  status_info;

    if (max_opaque_data_size > FBE_DATABASE_PROVISION_DRIVE_OPAQUE_DATA_MAX){
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                      "%s: max data size 0x%x is greater than expected 0x%x.\n",
                      __FUNCTION__, max_opaque_data_size, FBE_DATABASE_PROVISION_DRIVE_OPAQUE_DATA_MAX);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* send the control packet and wait for its completion. */
    status = fbe_api_common_send_control_packet(FBE_PROVISION_DRIVE_CONTROL_CODE_GET_OPAQUE_DATA,
                                                opaque_data_ptr,
                                                max_opaque_data_size,
                                                object_id,
                                                FBE_PACKET_FLAG_INTERNAL,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);
    
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    
    return FBE_STATUS_OK;  

} // end fbe_api_provision_drive_get_opaque_data()

/*!****************************************************************************
 * fbe_api_provision_drive_get_pool_id()
 ******************************************************************************
 * @brief
 * This function returns the pool id for the provision drive. The pool-id will
 * tell the user which storage pool this PVD is part of.
 *
 *  @param pool_id - OUT the returned size.
 *
 * @return status - The status of the operation.
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_pool_id(fbe_object_id_t object_id, fbe_u32_t *pool_id)
{
    fbe_status_t                             status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t  status_info;
    fbe_provision_drive_control_pool_id_t    get_pool_id;

    /* send the control packet and wait for its completion. */
    status = fbe_api_common_send_control_packet(FBE_PROVISION_DRIVE_CONTROL_CODE_GET_POOL_ID,
                                                &get_pool_id,
                                                sizeof(fbe_provision_drive_control_pool_id_t),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);
    
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__, 
                       status, status_info.packet_qualifier, 
                       status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK)
        {
            return status;
        }
        else
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    *pool_id = get_pool_id.pool_id;
    
    return FBE_STATUS_OK;  

} // end fbe_api_provision_drive_get_pool_id()

/*!****************************************************************************
 * fbe_api_provision_drive_update_pool_id()
 ******************************************************************************
 * @brief
 * This function sets/updates the PVD pool-id.
 *
 * @param update_pvd_pool_id - Update PVD pool-id data pointer.
 *
 * @return status - The status of the operation.
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_update_pool_id(fbe_api_provision_drive_update_pool_id_t *update_pvd_pool_id)
{
    fbe_api_job_service_update_pvd_pool_id_t        job_update_pvd_pool_id;
    fbe_status_t                                    status;
    fbe_status_t                                    job_status;
    fbe_job_service_error_type_t                    js_error_type;

    /* initialize error type as no error. */
    js_error_type = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    job_update_pvd_pool_id.pvd_object_id = update_pvd_pool_id->object_id;
    job_update_pvd_pool_id.pvd_pool_id = update_pvd_pool_id->pool_id;

    /* update the provision drive pool-id. */
    status = fbe_api_job_service_update_pvd_pool_id(&job_update_pvd_pool_id);
    if (status != FBE_STATUS_OK) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "PVD update_pool_id: Failed. PVD=0x%x, Status=0x%x\n", 
                      job_update_pvd_pool_id.pvd_object_id, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* wait for the notification from the job service. */
    status = fbe_api_common_wait_for_job(job_update_pvd_pool_id.job_number,
                                         PVD_UPDATE_CONFIG_TYPE_WAIT_TIMEOUT,
                                         &js_error_type,
                                         &job_status,
                                         NULL);

    if ((status != FBE_STATUS_OK) || (job_status != FBE_STATUS_OK) || (js_error_type != FBE_JOB_SERVICE_ERROR_NO_ERROR)) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                      "PVD update_pool_id: timedout. PVD=0x%X, Status=0x%X, Job Status=0x%X, Job Error=0x%x\n",
                      job_update_pvd_pool_id.pvd_object_id, status, job_status, js_error_type);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_provision_drive_get_zero_percentage(fbe_object_id_t pvd_object_id,
 *                                                 fbe_u16_t * zero_percentage)
 ****************************************************************
 * @brief
 *  This function to get the disk zeroing percentage complete.
 *
 * @param object_id - object ID
 * @param zero_percentage - pointer to disk zeroing percent complete
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  09/12/11 - Created. Sonali K.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_zero_percentage(fbe_object_id_t pvd_object_id, fbe_u16_t * zeroing_percentage)
{
    fbe_status_t    status;        
    fbe_api_provision_drive_info_t provision_drive_info;
    fbe_lba_t       zero_chkpt = 0;   

    /* Get the PVD exported capacity
     * exported capacity is PVD total capacity from upper perspective
     * PVD capacity is stored in provision_drive_info.capacity
     * and got via fbe_api_get_pvd_object_info()
     * Provision drive capacity is calculated by block
     */
    status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info);
    if(status != FBE_STATUS_OK)
    {       
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                      "Provision drive object(object id: 0x%x) does not exist. Status=0x%x\n",
                      pvd_object_id,status);
        return status;
    }
    
    /* Get the PVD zeroed capacity
     * zero checkpoint means the number of PVD blocks have been zeroed
     * zeroed capacity is stored in zero_chkpt
     */
    status = fbe_api_provision_drive_get_zero_checkpoint(pvd_object_id, &zero_chkpt);
    if(status != FBE_STATUS_OK)
    {
       
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                      "%s : Provision drive(object id: 0x%x) get checkpoint failed. Status=0x%x\n",
                      __FUNCTION__,pvd_object_id,status);
        return status;
    }
    
    /*Find whether background zeroing is already completed for the provision drive object*/
    if(zero_chkpt >= provision_drive_info.capacity)
    {
        *zeroing_percentage = 100;
    }
    else
    {
        /*Calculate zeroing percentage*/
        *zeroing_percentage = (fbe_u16_t)(zero_chkpt * 100 / provision_drive_info.capacity);
    }
    fbe_api_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
                      "\nProvision Drive Zeroing Percentage: %d\n",
                      *zeroing_percentage);      
    return status;
}


/*!***************************************************************************
 * @fn fbe_api_copy_get_source_virtual_drive(fbe_object_id_t source_pvd_id,
 *                                           fbe_object_id_t *vd_object_id_p,
 *                       fbe_provision_drive_copy_to_status_t *copy_status_p)
 *****************************************************************************
 *
 * @brief   This function is used to locate the virtual drive associated with
 *          the source drive of a copy operation.  This is required since the
 *          interface for copy operations is the pvd identifier associated with
 *          source virtual drive.
 *
 * @param   source_pvd_id - The object id for the source provision drive
 * @param   vd_object_id_p - Address of virtual drive id to populate
 * @param   copy_status_p - Address of copy status (result)
 *
 * @return  status - FBE_STATUS_OK - Located exactly (1) upstream virtual drive
 *                   Other - Something went wrong (selected source pvd is bad)
 *
 * @note    There should be NO validation checks in this method!
 *          It is up to SEP to perform the proper source and destination
 *          validation checks.
 *
 * @version
 *  06/14/2012  Ron Proulx  - Updated.
 *
 *****************************************************************************/
static fbe_status_t FBE_API_CALL 
fbe_api_copy_get_source_virtual_drive(fbe_object_id_t source_pvd_id,
                                      fbe_object_id_t *vd_object_id_p,
                                      fbe_provision_drive_copy_to_status_t *copy_status_p)
{
    fbe_status_t                                status = FBE_STATUS_OK; 
    fbe_bool_t                                  b_is_system_pvd = FBE_FALSE;
    fbe_u32_t                                   upstream_object_index;
    fbe_u32_t                                   num_of_vds = 0;
    fbe_class_id_t                              class_id;
    fbe_object_id_t                             vd_object_id;          
    fbe_api_base_config_upstream_object_list_t  upstream_object_list;

    /* Get the vd object id which hook up to the given source pvd.*/
    *copy_status_p = FBE_PROVISION_DRIVE_COPY_TO_STATUS_INTER_OP_FAILURE;
    status = fbe_api_base_config_get_upstream_object_list(source_pvd_id, &upstream_object_list);
    if(status != FBE_STATUS_OK)
    {       
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                      "%s: The source provision drive object(object id: 0x%x) failed to get upstream object list. Status=0x%x\n",
                       __FUNCTION__, source_pvd_id, status);
        *copy_status_p = FBE_PROVISION_DRIVE_COPY_TO_STATUS_INTER_OP_FAILURE;
        return status;
    }

    /*! @note We allow copy operations on system drives (we will only copy the
     *        data associated with the user raid group.  No system data will be
     *        copied).  Therefore we first check if the pvd is a system drive.
     */
    status = fbe_api_database_is_system_object(source_pvd_id, &b_is_system_pvd);
    if(status != FBE_STATUS_OK)
    {       
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                      "%s: The source provision drive object(object id: 0x%x) failed to detemine is system. Status=0x%x\n",
                       __FUNCTION__, source_pvd_id, status);
        *copy_status_p = FBE_PROVISION_DRIVE_COPY_TO_STATUS_INTER_OP_FAILURE;
        return status;
    }

    /* Walk thru the list of objects and validate that there is exactly (1) 
     * virtual drive.
     */
    for (upstream_object_index = 0; upstream_object_index < upstream_object_list.number_of_upstream_objects; upstream_object_index++)
    {
        /* Get the class id of the object.
         */
        status = fbe_api_get_object_class_id(upstream_object_list.upstream_object_list[upstream_object_index],
                                             &class_id, FBE_PACKAGE_ID_SEP_0);
        if (status != FBE_STATUS_OK)
        {       
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                          "%s: Failed to get class id for source pvd obj: 0x%x upstream index: %d obj: 0x%x\n",
                           __FUNCTION__, source_pvd_id, upstream_object_index, 
                          upstream_object_list.upstream_object_list[upstream_object_index]);
            *copy_status_p = FBE_PROVISION_DRIVE_COPY_TO_STATUS_INTER_OP_FAILURE;
            return status;
        }

        /* If the class id is `virtual drive' increment found count and set
         * virtual drive id.
         */
        if (class_id == FBE_CLASS_ID_VIRTUAL_DRIVE)
        {
            num_of_vds++;
            vd_object_id = upstream_object_list.upstream_object_list[upstream_object_index];
        }

    } /* end for all upstream objects */

    /* Now validate the results.  First validate that at least (1) virtual
     * virtual drive object was found (this error shoulkd be a warning).
     */
    if (num_of_vds == 0)
    {       
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                      "%s: No upstream vd for source pvd obj: 0x%x is system drive: %d num upstream: %d\n",
                      __FUNCTION__, source_pvd_id, b_is_system_pvd, upstream_object_list.number_of_upstream_objects);
        *copy_status_p = FBE_PROVISION_DRIVE_COPY_TO_STATUS_NO_UPSTREAM_RAID_GROUP;
        return FBE_STATUS_NO_DEVICE;
    }

    /* Now check if more than (1) virtual drive was found.
     */
    if (num_of_vds > 1)
    {       
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                      "%s: Found: %d vds for source pvd obj: 0x%x is system drive: %d num upstream: %d\n",
                      __FUNCTION__, num_of_vds, source_pvd_id, b_is_system_pvd, upstream_object_list.number_of_upstream_objects);
        *copy_status_p = FBE_PROVISION_DRIVE_COPY_TO_STATUS_UNEXPECTED_DRIVE_CONFIGURATION;
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /*! @note Although there are other checks we could perform (more than (1)
     *        upstream for non-system drive etc, it is up to SEP to perform
     *        additional validations.
     */
    *copy_status_p = FBE_PROVISION_DRIVE_COPY_TO_STATUS_NO_ERROR;
    *vd_object_id_p = vd_object_id;

    return status;
}
/************************************************************ 
 * end fbe_api_copy_get_source_virtual_drive() 
 ************************************************************/

/*!***************************************************************
 * @fn fbe_api_copy_to_replacement_disk(fbe_object_id_t source_pvd_id,
 *                                      fbe_object_id_t destination_pvd_id)
 ****************************************************************
 * @brief
 *  This function is to start the drive swap in job and copy from
 *  source PVD to destination PVD.
 *
 * @param fbe_object_id_t source_pvd_id
 * @param fbe_object_id_t destination_pvd_id
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *               - FBE_STATUS_NO_DEVICE if there is no device connected to the drive.
 *               - FBE_STATUS_BUSY if copy on the drive is already in progress. 
 *
 * @note    There should be NO validation checks in this method!
 *          It is up to SEP to perform the proper source and destination
 *          validation checks.
 *
 * @version
 *  03/16/12 - Created. Vera Wang
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_copy_to_replacement_disk(fbe_object_id_t source_pvd_id, fbe_object_id_t destination_pvd_id, fbe_provision_drive_copy_to_status_t *copy_status)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_status_t                                    job_status = FBE_STATUS_OK;
    fbe_object_id_t                                 vd_object_id = FBE_OBJECT_ID_INVALID;          
    fbe_api_job_service_drive_swap_in_request_t     drive_swap_in_request = {0};
    fbe_job_service_error_type_t                    job_error_type = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* Get the vd object id which hook up to the given source pvd.*/
    status = fbe_api_copy_get_source_virtual_drive(source_pvd_id, &vd_object_id, copy_status);
    if (status != FBE_STATUS_OK)
    {    
        /* This is a warning since it is a user request.
         */
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                      "%s: Failed to get vd for source pvd: 0x%x dest pvd: 0x%x status: 0x%x\n",
                       __FUNCTION__, source_pvd_id, destination_pvd_id, status);
        if (*copy_status == FBE_PROVISION_DRIVE_COPY_TO_STATUS_NO_ERROR)
        {
            *copy_status = FBE_PROVISION_DRIVE_COPY_TO_STATUS_INTER_OP_FAILURE;
        }
        return status;
    }

    /* Fill the job request. */
    drive_swap_in_request.vd_object_id = vd_object_id;
    drive_swap_in_request.original_object_id = source_pvd_id;
    drive_swap_in_request.spare_object_id = destination_pvd_id; 
    drive_swap_in_request.command_type = FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND;
    job_error_type = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* Issue the job service request. */
    status = fbe_api_job_service_drive_swap_in_request(&drive_swap_in_request);
    if(status != FBE_STATUS_OK)
    {       
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                      "%s: Failed to send a drive swap in job request.\n", __FUNCTION__);
        return status;
    }

    status = fbe_api_common_wait_for_job(drive_swap_in_request.job_number,
                                         FBE_JOB_SERVICE_DRIVE_SWAP_TIMEOUT,
                                         &job_error_type,
                                         &job_status,
                                         NULL);

    fbe_api_translate_job_error_to_user_copy_status(job_error_type, copy_status);
    if ((status != FBE_STATUS_OK) || (job_status != FBE_STATUS_OK) || (job_error_type != FBE_JOB_SERVICE_ERROR_NO_ERROR)) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: wait for job failed with status: 0x%X, job status:0x%X, job error:0x%x\n",
                      __FUNCTION__, status, job_status, job_error_type);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
} // End fbe_api_copy_to_replacement_disk()

/*!***************************************************************
 * @fn fbe_api_provision_drive_user_copy(fbe_object_id_t source_pvd_id)
 ****************************************************************
 * @brief
 *  This function to start the drive swap in job and copy from the source pvd
 *  to an automatically selected drive for a given pvd. After that, source drive will be killed.
 *  This is a replacement to the legacy ADM_PROACTIVE_SPARE_DATA.
 *
 * @param fbe_object_id_t(in) source_pvd_id        
 *        copy_status(out) - user copy status which is translated from job error type
 *                           and can be used for different failures.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *               - FBE_STATUS_GENERIC_FAILURE if there are some errors. 
 * 
 * @version
 *  03/16/12 - Created. Vera Wang
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_user_copy(fbe_object_id_t source_pvd_id, fbe_provision_drive_copy_to_status_t* copy_status)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_status_t                                    job_status;
    fbe_object_id_t                                 vd_object_id;          
    fbe_api_job_service_drive_swap_in_request_t     drive_swap_in_request;
    fbe_job_service_error_type_t                    job_error_type;

    /* Get the vd object id which hook up to the given source pvd.*/
    status = fbe_api_copy_get_source_virtual_drive(source_pvd_id, &vd_object_id, copy_status);
    if (status != FBE_STATUS_OK)
    {    
        /* This is a warning since it is a user request.
         */
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                      "%s: Failed to get vd for source pvd: 0x%x status: 0x%x\n",
                       __FUNCTION__, source_pvd_id, status);
        if (*copy_status == FBE_PROVISION_DRIVE_COPY_TO_STATUS_NO_ERROR)
        {
            *copy_status = FBE_PROVISION_DRIVE_COPY_TO_STATUS_INTER_OP_FAILURE;
        }
        return status;
    }

    /* Fill the job request. */
    drive_swap_in_request.vd_object_id = vd_object_id;
    drive_swap_in_request.original_object_id = source_pvd_id;
    drive_swap_in_request.spare_object_id = FBE_OBJECT_ID_INVALID; 
    drive_swap_in_request.command_type = FBE_SPARE_INITIATE_USER_COPY_COMMAND;

    job_error_type = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* Issue the job service request. */
    status = fbe_api_job_service_drive_swap_in_request(&drive_swap_in_request);
    if(status != FBE_STATUS_OK)
    {       
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                      "%s: Failed to send a drive swap in job request.\n", __FUNCTION__);
        return status;
    }

    status = fbe_api_common_wait_for_job(drive_swap_in_request.job_number,
                                         FBE_JOB_SERVICE_DRIVE_SWAP_TIMEOUT,
                                         &job_error_type,
                                         &job_status,
                                         NULL);
    fbe_api_translate_job_error_to_user_copy_status(job_error_type, copy_status);
    if ((status != FBE_STATUS_OK) || (job_status != FBE_STATUS_OK) || (job_error_type != FBE_JOB_SERVICE_ERROR_NO_ERROR)) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: wait for job failed with status: 0x%X, job status:0x%X, job error:0x%x\n",
                      __FUNCTION__, status, job_status, job_error_type);       

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
} // End fbe_api_provision_drive_user_copy()

/*!***************************************************************************
 * @fn fbe_api_provision_drive_get_vd_object_id(fbe_object_id_t source_pvd_id,
 *                                              fbe_object_id_t *vd_object_id_p)
 *****************************************************************************
 *
 * @brief   This function is used to locate the virtual drive associated with
 *          a provision drive.
 *
 * @param   source_pvd_id - The object id for the source provision drive
 * @param   vd_object_id_p - Address of virtual drive id to populate
 *
 * @return  status - FBE_STATUS_OK - Located exactly (1) upstream virtual drive
 *                   Other - Something went wrong (selected source pvd is bad)
 *
 * @note    There should be NO validation checks in this method!
 *          It is up to SEP to perform the proper source and destination
 *          validation checks.
 *
 * @version
 *  06/18/2012  Ron Proulx  - Updated.
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_provision_drive_get_vd_object_id(fbe_object_id_t source_pvd_id,
                                                                   fbe_object_id_t *vd_object_id_p)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_provision_drive_copy_to_status_t    copy_status = FBE_PROVISION_DRIVE_COPY_TO_STATUS_UNEXPECTED_ERROR;

    /* Get the vd object id which hook up to the given source pvd.*/
    status = fbe_api_copy_get_source_virtual_drive(source_pvd_id, vd_object_id_p, &copy_status);
    if (status != FBE_STATUS_OK)
    {    
        /* This is a warning since it is a user request.
         */
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                      "%s: Failed to get vd for source pvd: 0x%x status: 0x%x\n",
                       __FUNCTION__, source_pvd_id, status);
        return status;
    }

    return status;
}
// end fbe_api_provision_drive_get_vd_object_id()

/*!**************************************************************
 * fbe_api_translate_job_error_to_user_copy_status()
 ****************************************************************
 * @brief
 * translates job error code to user copy status 
 *
 * @param   job error code(in) - matching job error code         
 *          copy_status(out) - user copy status
 * 
 * @return  None.
 *
 * @author
 * 03/27/2012 - Created. Vera Wang
 *
 ****************************************************************/

static void fbe_api_translate_job_error_to_user_copy_status(fbe_job_service_error_type_t job_error_code,
                                                fbe_provision_drive_copy_to_status_t* copy_status)
{
    switch (job_error_code) {
        case FBE_JOB_SERVICE_ERROR_NO_ERROR:
            *copy_status = FBE_PROVISION_DRIVE_COPY_TO_STATUS_NO_ERROR;
            break;
        case FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR:
            *copy_status = FBE_PROVISION_DRIVE_COPY_TO_STATUS_INTER_OP_FAILURE;
            break;
        case FBE_JOB_SERVICE_ERROR_TIMEOUT:
            *copy_status = FBE_PROVISION_DRIVE_COPY_TO_STATUS_EXCEEDED_ALLOTTED_TIME;
            break;
        case FBE_JOB_SERVICE_ERROR_INVALID_SPARE_OBJECT_ID:
            *copy_status = FBE_PROVISION_DRIVE_COPY_TO_STATUS_SPARE_INVALID_ID;
            break;
        case FBE_JOB_SERVICE_ERROR_SPARE_NOT_READY:
            *copy_status= FBE_PROVISION_DRIVE_COPY_TO_STATUS_SPARE_NOT_READY;
            break;
        case FBE_JOB_SERVICE_ERROR_SPARE_REMOVED:
            *copy_status = FBE_PROVISION_DRIVE_COPY_TO_STATUS_SPARE_REMOVED;
            break;
        case FBE_JOB_SERVICE_ERROR_SPARE_BUSY:
            *copy_status = FBE_PROVISION_DRIVE_COPY_TO_STATUS_SPARE_BUSY;
            break;
        case FBE_JOB_SERVICE_ERROR_INVALID_DESIRED_SPARE_DRIVE_TYPE:
            *copy_status = FBE_PROVISION_DRIVE_COPY_TO_STATUS_PERF_TIER_MISMATCH;
            break;
        case FBE_JOB_SERVICE_ERROR_BLOCK_SIZE_MISMATCH:
            *copy_status = FBE_PROVISION_DRIVE_COPY_TO_STATUS_BLOCK_SIZE_MISMATCH;
            break;
        case FBE_JOB_SERVICE_ERROR_CAPACITY_MISMATCH:
            *copy_status = FBE_PROVISION_DRIVE_COPY_TO_STATUS_CAPACITY_MISMATCH;
            break;
        case FBE_JOB_SERVICE_ERROR_OFFSET_MISMATCH:
        case FBE_JOB_SERVICE_ERROR_SYS_DRIVE_MISMATCH:
            *copy_status = FBE_PROVISION_DRIVE_COPY_TO_STATUS_SYS_DRIVE_MISMATCH;
            break;
        case FBE_JOB_SERVICE_ERROR_SWAP_UNEXPECTED_ERROR:
            *copy_status = FBE_PROVISION_DRIVE_COPY_TO_STATUS_UNEXPECTED_ERROR;
            break;
        case FBE_JOB_SERVICE_ERROR_SWAP_PROACTIVE_SPARE_NOT_REQUIRED:
            *copy_status = FBE_PROVISION_DRIVE_COPY_TO_STATUS_PROACTIVE_SPARE_NOT_REQUIRED;
            break;
        case FBE_JOB_SERVICE_ERROR_SWAP_VIRTUAL_DRIVE_BROKEN:
            *copy_status = FBE_PROVISION_DRIVE_COPY_TO_STATUS_VIRTUAL_DRIVE_BROKEN;
            break;
        case FBE_JOB_SERVICE_ERROR_SWAP_CURRENT_VIRTUAL_DRIVE_CONFIG_MODE_DOESNT_SUPPORT:
            *copy_status = FBE_PROVISION_DRIVE_COPY_TO_STATUS_VIRTUAL_DRIVE_CONFIG_MODE_DOESNT_SUPPORT;
            break;
        case FBE_JOB_SERVICE_ERROR_SWAP_COPY_DESTINATION_DRIVE_NOT_HEALTHY:
            *copy_status = FBE_PROVISION_DRIVE_COPY_TO_STATUS_DESTINATION_DRIVE_NOT_HEALTHY;
            break;
        case FBE_JOB_SERVICE_ERROR_SPARE_RAID_GROUP_NOT_REDUNDANT:
            *copy_status = FBE_PROVISION_DRIVE_COPY_TO_STATUS_COPY_NOT_ALLOWED_TO_NON_REDUNDANT_RAID_GROUP;
            break;
        case FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_BROKEN:
            *copy_status = FBE_PROVISION_DRIVE_COPY_TO_STATUS_COPY_NOT_ALLOWED_TO_BROKEN_RAID_GROUP;
            break;
        case FBE_JOB_SERVICE_ERROR_SPARE_RAID_GROUP_UNCONSUMED:
            *copy_status = FBE_PROVISION_DRIVE_COPY_TO_STATUS_COPY_NOT_ALLOWED_TO_UNCONSUMED_RAID_GROUP;
            break;
        case FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_DEGRADED:
            *copy_status = FBE_PROVISION_DRIVE_COPY_TO_STATUS_RAID_GROUP_IS_DEGRADED;
            break;
        case FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_DENIED:
            *copy_status = FBE_PROVISION_DRIVE_COPY_TO_UPSTREAM_RAID_GROUP_DENIED;
            break;
        case FBE_JOB_SERVICE_ERROR_INVALID_SWAP_COMMAND:
            *copy_status = FBE_PROVISION_DRIVE_COPY_TO_STATUS_UNSUPPORTED_COMMAND;
            break;
        case FBE_JOB_SERVICE_ERROR_PRESENTLY_NO_SPARES_AVAILABLE:
            *copy_status = FBE_PROVISION_DRIVE_COPY_TO_STATUS_NO_SPARES_AVAILABLE;
            break;
        case FBE_JOB_SERVICE_ERROR_PRESENTLY_NO_SUITABLE_SPARE:
            *copy_status = FBE_PROVISION_DRIVE_COPY_TO_STATUS_NO_SUITABLE_SPARES;
            break;
        case FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_HAS_COPY_IN_PROGRESS:
            *copy_status = FBE_PROVISION_DRIVE_COPY_TO_STATUS_RG_COPY_ALREADY_IN_PROGRESS;
            break;
        case FBE_JOB_SERVICE_ERROR_PRESENTLY_SOURCE_DRIVE_DEGRADED:
            *copy_status = FBE_PROVISION_DRIVE_COPY_TO_STATUS_SOURCE_BUSY;
            break;
        case FBE_JOB_SERVICE_ERROR_SWAP_COPY_DESTINATION_HAS_UPSTREAM_RAID_GROUP:
            *copy_status = FBE_PROVISION_DRIVE_COPY_TO_STATUS_SPARE_BUSY;
            break;
        default:
            *copy_status = FBE_PROVISION_DRIVE_COPY_TO_STATUS_UNKNOWN;
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: Unknown job error code: %d Can not map to copy status.\n",
                       __FUNCTION__, job_error_code);
            break;
    }
    return;
}

/*!***************************************************************
 * @fn fbe_api_provision_drive_set_removed_timestamp(
 *       fbe_object_id_t object_id, 
 *       fbe_system_time_t removed_timestamp)
 ****************************************************************
 * @brief
 *  This function is for internal (debugging) use ONLY.
 *
 * @param object_id - object ID
 * @param removed_timestamp
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_set_removed_timestamp(fbe_object_id_t object_id, fbe_system_time_t removed_timestamp,fbe_bool_t is_persist)
{
    fbe_status_t                                        status;
    fbe_api_control_operation_status_info_t             status_info;
    fbe_provision_drive_control_set_removed_timestamp_t set_removed_timestamp;
    fbe_u32_t                                           retry_count = 0;
    fbe_bool_t                                          b_is_system_pvd = FBE_FALSE;

    fbe_copy_memory(&set_removed_timestamp.removed_tiem_stamp ,
                    &removed_timestamp,
                    sizeof(fbe_system_time_t));
    set_removed_timestamp.is_persist_b = is_persist;

    status = fbe_api_database_is_system_object(object_id, &b_is_system_pvd);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    /* the control code FBE_PROVISION_DRIVE_CONTROL_CODE_SET_REMOVED_TIMESTAMP will write and persist PVD's nonpaged metadata
     * if the PVD is a system PVD, the packet might be failed back with QUIESCED,
     * since system PVD's non-paged metadata operation could not be queieced.
     * that requires we should have a retry latter
     */
    retry_count = (b_is_system_pvd ? 60 : 1);


    while (retry_count > 0)
    {
        status = fbe_api_common_send_control_packet (FBE_PROVISION_DRIVE_CONTROL_CODE_SET_REMOVED_TIMESTAMP,
                                                     &set_removed_timestamp,
                                                     sizeof(fbe_provision_drive_control_set_removed_timestamp_t),
                                                     object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB,
                                                     &status_info,
                                                     FBE_PACKAGE_ID_SEP_0);

        if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
            fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                           status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

            if (status != FBE_STATUS_OK) {
                if (b_is_system_pvd) {
                    retry_count--;
                    fbe_api_sleep(1000);
                    continue;
                } else{
                    return status;
                }
            }else{
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        retry_count--;
    }

    return status;
}

fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_removed_timestamp(fbe_object_id_t object_id, fbe_system_time_t * removed_timestamp)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_provision_drive_get_removed_timestamp_t       get_removed_timestamp;

    fbe_zero_memory(&get_removed_timestamp.removed_tiem_stamp,sizeof(fbe_system_time_t));
    fbe_zero_memory(removed_timestamp,sizeof(fbe_system_time_t));

    status = fbe_api_common_send_control_packet(FBE_PROVISION_DRIVE_CONTROL_CODE_GET_REMOVED_TIMESTAMP,
                                                &get_removed_timestamp,
                                                sizeof(fbe_provision_drive_get_removed_timestamp_t),
                                                object_id,
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
    fbe_copy_memory(removed_timestamp,
                    &get_removed_timestamp.removed_tiem_stamp,
                    sizeof(fbe_system_time_t));
    return status;
}

/*!***************************************************************
 * @fn fbe_api_provision_drive_set_slf_enabled(
 *      fbe_bool_t                             slf_enabled)
 *****************************************************************
 * @brief
 *  This function enable/disables single loop failure feature.
 *
 * @param slf_enabled      - enable/disable slf
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  4/30/2012 - Created.  Lili Chen
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_provision_drive_set_slf_enabled(fbe_bool_t slf_enabled)
{

    fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;

    status = fbe_api_common_send_control_packet_to_class (FBE_PROVISION_DRIVE_CONTROL_CODE_SET_SLF_ENABLED,
                                                          &slf_enabled,
                                                          sizeof(fbe_bool_t),
                                                          FBE_CLASS_ID_PROVISION_DRIVE,
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

    return status;
}   
/***************************************************************
 * end fbe_api_provision_drive_set_slf_enabled()
 ***************************************************************/


/*!***************************************************************
 * @fn fbe_api_provision_drive_get_slf_enabled(
 *      fbe_bool_t                             slf_enabled)
 *****************************************************************
 * @brief
 *  This function enable/disables single loop failure feature.
 *
 * @param slf_enabled      - pointer to slf enabled
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  10/01/2012 - Created.  Lili Chen
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_provision_drive_get_slf_enabled(fbe_bool_t *slf_enabled)
{

    fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_bool_t                                  is_slf_enabled;

    status = fbe_api_common_send_control_packet_to_class (FBE_PROVISION_DRIVE_CONTROL_CODE_GET_SLF_ENABLED,
                                                          &is_slf_enabled,
                                                          sizeof(fbe_bool_t),
                                                          FBE_CLASS_ID_PROVISION_DRIVE,
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

    *slf_enabled = is_slf_enabled;
    return status;
}   
/***************************************************************
 * end fbe_api_provision_drive_get_slf_enabled()
 ***************************************************************/

/*!***************************************************************
 * @fn fbe_api_provision_drive_is_slf(
 *      fbe_bool_t                    is_slf)
 *****************************************************************
 * @brief
 *  This function enable/disables single loop failure feature.
 *
 * @param is_slf      - whether the drive is in slf
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  05/08/2012 - Created.  Lili Chen
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_provision_drive_is_slf(fbe_object_id_t object_id, fbe_bool_t * is_slf)
{

    fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_provision_drive_get_slf_state_t         get_slf;

    status = fbe_api_common_send_control_packet(FBE_PROVISION_DRIVE_CONTROL_CODE_GET_SLF_STATE,
                                                &get_slf,
                                                sizeof(fbe_provision_drive_get_slf_state_t),
                                                object_id,
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

    *is_slf = get_slf.is_drive_slf;

    return status;
}   
/***************************************************************
 * end fbe_api_provision_drive_is_slf()
 ***************************************************************/

/*!***************************************************************
 * @fn fbe_api_provision_drive_get_zero_checkpiont_offset(fbe_object_id_t pvd_object_id,
 *                                                                                           fbe_u64_t * zero_checkpiont_offset)
 ****************************************************************
 * @brief
 *  This function to get the zero check piont offset.
 *  NOTE!!!! it is only used for kozma_prutkov_nonpaged_metadata_operation_test
 *  DOn't used it anywhere else
 * @param object_id - object ID
 * @param zero_checkpiont_offset_p - pointer to zero checkpiont offset
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  05/17/12 - Created. zhangy.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_zero_checkpiont_offset(fbe_object_id_t pvd_object_id, fbe_u64_t * zero_checkpiont_offset)
{
	*zero_checkpiont_offset = CSX_CAST_PTR_TO_PTRMAX(&((fbe_provision_drive_nonpaged_metadata_t *)0)->zero_checkpoint);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_api_provision_drive_test_slf()
 *****************************************************************
 * @brief
 *  This function enable/disables single loop failure feature.
 *
 * @param provision_drive_object_id - object id
 * @param op - block operation read/write
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  05/26/2012 - Created.  Lili Chen
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_provision_drive_test_slf(
                            fbe_object_id_t             provision_drive_object_id,
                            fbe_u32_t op)
{

    fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;


    /* this request does not required to pass buffer information */
    status = fbe_api_common_send_control_packet (FBE_PROVISION_DRIVE_CONTROL_CODE_TEST_SLF,
                                                &op,
                                                sizeof(fbe_u32_t),
                                                provision_drive_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}   
/***************************************************************
 * end fbe_api_provision_drive_test_slf()
 ***************************************************************/

/*!**************************************************************
 * fbe_api_provision_drive_get_max_read_chunks()
 ****************************************************************
 * @brief
 *  Determine the max number of chunks we can get at a time.
 *
 * @param num_chunks_p - number of chunks returned.               
 *
 * @return fbe_status_t   
 *
 * @author
 *  6/4/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_provision_drive_get_max_read_chunks(fbe_u32_t *num_chunks_p)
{
    *num_chunks_p = FBE_PAYLOAD_METADATA_MAX_DATA_SIZE / sizeof(fbe_raid_group_paged_metadata_t);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_api_provision_drive_get_max_read_chunks()
 ******************************************/
/*!**************************************************************
 * fbe_api_provision_drive_get_paged_metadata()
 ****************************************************************
 * @brief
 *  Get a set of paged metadata.
 *
 *  The chunk count should not exceed that returned by
 *  fbe_api_provision_drive_get_max_read_chunks()
 *
 * @param pvd_object_id - object ID to get chunks from.
 * @param chunk_offset - Chunk number to start read.
 * @param chunk_count - Number of chunks to read and number of
 *                      chunks of memory in paged_md_p
 * @param paged_md_p - Ptr to array of chunk_count number of chunks
 * @param b_force_unit_access - FBE_TRUE - Get the paged data from disk (not from cache)
 *                              FBE_FALSE - Allow the data to come from cache          
 *
 * @return fbe_status_t
 *
 * @author
 *  6/4/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_provision_drive_get_paged_metadata(fbe_object_id_t pvd_object_id,
                                                        fbe_chunk_index_t chunk_offset,
                                                        fbe_chunk_index_t chunk_count,
                                                        fbe_provision_drive_paged_metadata_t *paged_md_p,
                                                        fbe_bool_t b_force_unit_access)
{
    fbe_api_provision_drive_info_t pvd_info;
    fbe_status_t  status = FBE_STATUS_OK;
    fbe_chunk_index_t total_chunks;
    fbe_u64_t metadata_offset = 0;
    fbe_u32_t max_chunks_per_request;
    fbe_api_base_config_metadata_paged_get_bits_t get_bits;
    fbe_u32_t wait_msecs = 0;

    /* Get the information about the capacity so we can calculate number of chunks.
     */
    status = fbe_api_provision_drive_get_info(pvd_object_id, &pvd_info);

    if (status != FBE_STATUS_OK) 
    { 
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: pvd get info failed, obj 0x%x, status: 0x%x\n", 
                        __FUNCTION__, pvd_object_id, status);
        return status; 
    }

    total_chunks = pvd_info.total_chunks;

    if (chunk_offset > total_chunks)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: input chunk offset 0x%llx is greater than total chunks 0x%llx\n", 
                        __FUNCTION__, (unsigned long long)chunk_offset, (unsigned long long)total_chunks);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_api_provision_drive_get_max_read_chunks(&max_chunks_per_request);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: get max paged failed, obj 0x%x, status: 0x%x\n", 
                        __FUNCTION__, pvd_object_id, status);
        return status; 
    }
    if (chunk_count > max_chunks_per_request)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: input chunk count 0x%llx > max allowed 0x%x\n", 
                        __FUNCTION__, (unsigned long long)chunk_count, max_chunks_per_request);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    metadata_offset = chunk_offset * sizeof(fbe_provision_drive_paged_metadata_t);

    get_bits.metadata_offset = metadata_offset;
    get_bits.metadata_record_data_size = (fbe_u32_t)(chunk_count * sizeof(fbe_provision_drive_paged_metadata_t));

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
    status = fbe_api_base_config_metadata_paged_get_bits(pvd_object_id, &get_bits);
    while (status != FBE_STATUS_OK)
    {
        if (wait_msecs > 30000)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: get bits failed, obj 0x%x offset 0x%llx status: 0x%x\n", 
                          __FUNCTION__, pvd_object_id, (unsigned long long)metadata_offset, status);
            return FBE_STATUS_TIMEOUT;
        }
        fbe_api_sleep(500);
        wait_msecs += 500;

        status = fbe_api_base_config_metadata_paged_get_bits(pvd_object_id, &get_bits);
    }
    if (status == FBE_STATUS_OK)
    {
        fbe_u32_t chunk_index;
        fbe_provision_drive_paged_metadata_t *paged_p = (fbe_provision_drive_paged_metadata_t *)&get_bits.metadata_record_data[0];

        for (chunk_index = 0; chunk_index < chunk_count; chunk_index++)
        {
            paged_md_p[chunk_index] = paged_p[chunk_index]; 
        }
    }
    return status;
}
/******************************************
 * end fbe_api_provision_drive_get_paged_metadata()
 ******************************************/

/*!***************************************************************
 * @fn fbe_api_provision_drive_map_lba_to_chunk()
 *****************************************************************
 * @brief
 *  Map a host lba on a pvd into a chunk index
 *
 * @param object_id     - object to query.
 * @param provision_drive_info_p   - pvd map info to return.
 *
 * @return fbe_status_t - success or failure.
 *
 * @version
 *  6/4/2012 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_provision_drive_map_lba_to_chunk(fbe_object_id_t object_id, 
                                                                   fbe_provision_drive_map_info_t * provision_drive_info_p)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    
    status = fbe_api_common_send_control_packet(FBE_PROVISION_DRIVE_CONTROL_CODE_MAP_LBA_TO_CHUNK,
                                                provision_drive_info_p,
                                                sizeof(fbe_provision_drive_map_info_t),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
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
}
 
/*!****************************************************************************
 * fbe_api_provision_drive_set_config_type()
 ******************************************************************************
 * @brief
 * This function sets the config type of PVD to Failed or to unconsumed spare.
 *
 *  @param update_pvd_config_type_p - Update PVD config data pointer.
 *
 * @return status - The status of the operation.
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_set_config_type(fbe_api_provision_drive_update_config_type_t*  update_pvd_config_type_p)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_control_operation_status_info_t status_info;
     fbe_provision_drive_configuration_t update_pvd_config_type;

    /* fill in all the information we need */
    update_pvd_config_type.update_type = FBE_UPDATE_PVD_TYPE;
    update_pvd_config_type.update_type_bitmask = 0;
    update_pvd_config_type.config_type = update_pvd_config_type_p->config_type;
    object_id = update_pvd_config_type_p->pvd_object_id;


    status = fbe_api_common_send_control_packet(FBE_PROVISION_DRIVE_CONTROL_CODE_UPDATE_CONFIGURATION,
                                                &update_pvd_config_type,
                                                sizeof(update_pvd_config_type),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
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
}


/*!***************************************************************
 * @fn fbe_api_provision_drive_clear_drive_fault()
 *****************************************************************
 * @brief
 *  This function clears drive fault bit in PVD.
 *
 * @param provision_drive_object_id - object id
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  07/16/2012 - Created.  Lili Chen
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_provision_drive_clear_drive_fault(
                            fbe_object_id_t             provision_drive_object_id)
{

    fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;


    /* this request does not required to pass buffer information */
    status = fbe_api_common_send_control_packet(FBE_PROVISION_DRIVE_CONTROL_CODE_CLEAR_DRIVE_FAULT,
                                                NULL,
                                                0,
                                                provision_drive_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}   
/***************************************************************
 * end fbe_api_provision_drive_clear_drive_fault()
 ***************************************************************/

/*!***************************************************************
 * @fn fbe_api_provision_drive_set_background_operation_speed(
 *          fbe_provision_drive_background_op_t bg_op, 
 *          fbe_u32_t bg_op_speed)
 ****************************************************************
 * @brief
 *  This API sends request to PVD to set background operation speed.
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
fbe_api_provision_drive_set_background_operation_speed(fbe_provision_drive_background_op_t bg_op, fbe_u32_t bg_op_speed)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_provision_drive_control_set_bg_op_speed_t   set_bg_op;

    set_bg_op.background_op =  bg_op;
    set_bg_op.background_op_speed = bg_op_speed;

    status = fbe_api_common_send_control_packet_to_class (FBE_PROVISION_DRIVE_CONTROL_CODE_SET_BG_OP_SPEED ,
                                                            &set_bg_op,
                                                            sizeof(fbe_provision_drive_control_set_bg_op_speed_t),
                                                            FBE_CLASS_ID_PROVISION_DRIVE,
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


    return status;
} 
/***************************************************************
 * end fbe_api_provision_drive_set_background_operation_speed
 ***************************************************************/

/*!***************************************************************
 * @fn fbe_api_provision_drive_get_background_operation_speed(
 *          fbe_provision_drive_control_get_bg_op_speed_t *pvd_bg_op)
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
fbe_api_provision_drive_get_background_operation_speed(fbe_provision_drive_control_get_bg_op_speed_t *pvd_bg_op)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_provision_drive_control_get_bg_op_speed_t   get_bg_op;

    status = fbe_api_common_send_control_packet_to_class (FBE_PROVISION_DRIVE_CONTROL_CODE_GET_BG_OP_SPEED ,
                                                            &get_bg_op,
                                                            sizeof(fbe_provision_drive_control_get_bg_op_speed_t),
                                                            FBE_CLASS_ID_PROVISION_DRIVE,
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

    pvd_bg_op->disk_zero_speed = get_bg_op.disk_zero_speed;
    pvd_bg_op->sniff_speed = get_bg_op.sniff_speed;

    return status;
} 
/***************************************************************
 * end fbe_api_provision_drive_get_background_operation_speed
 ***************************************************************/

/*!****************************************************************************
 * @fn fbe_api_provision_drive_clear_all_pvds_drive_states(void)
 ******************************************************************************
 *
 * @brief
 *    This function clears the EOL and Drive Fault State for all PVDs. 
 *
 * @param   none
 *
 * @return  fbe_status_t                  -  status of this operation
 *
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_provision_drive_clear_all_pvds_drive_states(void)
{
    fbe_status_t 		status = FBE_STATUS_OK;
    fbe_object_id_t*	pvd_list = NULL;
    fbe_u32_t           pvd_count;


    fbe_api_trace(FBE_TRACE_LEVEL_INFO, "Start clearing all PVDs EOL and Drive Fault States.\n");

    /* Get the total PVDs available */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PROVISION_DRIVE, FBE_PACKAGE_ID_SEP_0, &pvd_count);
    if (status != FBE_STATUS_OK) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "_clear_all_pvds_drive_states: Failed to get total PVDs; pvd_count=%d.\n", pvd_count);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (pvd_count > 0) 
    {
        status = fbe_api_enumerate_class(FBE_CLASS_ID_PROVISION_DRIVE, FBE_PACKAGE_ID_SEP_0, &pvd_list, &pvd_count);
        if (status != FBE_STATUS_OK) 
        {
            if(pvd_list != NULL)
            {
                fbe_api_free_memory(pvd_list);
            }
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "_clear_all_pvds_drive_states: Failed to get all PVDs.\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }

        for (;pvd_count > 0; pvd_count--)
        {                                                                                                                                   
            fbe_u32_t  pvd_index;
            fbe_api_provision_drive_info_t provision_drive_info;

            pvd_index = pvd_count -1;

            if (pvd_list[pvd_index] != FBE_OBJECT_ID_INVALID)
            {    
                status = fbe_api_provision_drive_get_info(pvd_list[pvd_index], &provision_drive_info);
                if(status != FBE_STATUS_OK)
                {
                    fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "_clear_all_pvds_drive_states: Failed to get PVD:0x%x Info.\n",
                                  pvd_list[pvd_index]);
                    continue;
                }
    
                /* Clear Marked Offline state if it is set */    
                if (provision_drive_info.swap_pending == FBE_TRUE) 
                {
                    status = fbe_api_provision_drive_clear_swap_pending(pvd_list[pvd_index]);
                    if (status != FBE_STATUS_OK) 
                    {
                        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "_clear_all_pvds_drive_states: Failed to clear Marked Offline for PVD:0x%x\n",
                                      pvd_list[pvd_index]);
                    }
                }

                /* Clear EOL state if it is set */    
                if (provision_drive_info.end_of_life_state == FBE_TRUE) 
                {
                    status = fbe_api_provision_drive_clear_eol_state(pvd_list[pvd_index], FBE_PACKET_FLAG_NO_ATTRIB);
                    if (status != FBE_STATUS_OK) 
                    {
                        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "_clear_all_pvds_drive_states: Failed to clear EOL for PVD:0x%x\n",
                                      pvd_list[pvd_index]);
                    }
                }

                /* Clear Drive fault state if it is set */
                if (provision_drive_info.drive_fault_state == FBE_TRUE) 
                {
                    status = fbe_api_provision_drive_clear_drive_fault(pvd_list[pvd_index]);
                    if (status != FBE_STATUS_OK) 
                    {
                        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "_clear_all_pvds_drive_states: Failed to clear Fault state for PVD:0x%x\n",
                                      pvd_list[pvd_index]);
                    }                
                }
            }
        }

        fbe_api_free_memory(pvd_list);
    }

    return status;

} /* end fbe_api_provision_drive_clear_all_pvds_drive_states() */


/*!****************************************************************************
 *          fbe_api_provision_drive_disable_spare_select_unconsumed()
 ******************************************************************************
 *
 * @brief   Disable provision drives configured as `unconsumed' from being
 *          used as spares.
 *
 * @param   None. 
 *
 * @return status - The status of the operation.
 *
 * @author
 *  09/21/2012  Ron Proulx  - Created.
 ******************************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_disable_spare_select_unconsumed(void)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_topology_mgmt_update_disable_spare_select_unconsumed_t update_disable_spare_select_unconsumed;

    /* Set the flag to True which will disable ALL provision drives configued
     * as `unconsumed' from being used as a spare.
     */
    fbe_zero_memory(&update_disable_spare_select_unconsumed, sizeof(fbe_topology_mgmt_update_disable_spare_select_unconsumed_t));
    update_disable_spare_select_unconsumed.b_disable_spare_select_unconsumed = FBE_TRUE;

    /* send the control packet and wait for its completion. */
    status = fbe_api_common_send_control_packet(FBE_TOPOLOGY_CONTROL_CODE_UPDATE_DISABLE_SPARE_SELECT_UNCONSUMED,
                                                &update_disable_spare_select_unconsumed,
                                                sizeof(fbe_topology_mgmt_update_disable_spare_select_unconsumed_t),
                                                FBE_OBJECT_ID_INVALID,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                NULL,
                                                FBE_PACKAGE_ID_SEP_0);
    
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d.\n", __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/***************************************************************
 * end fbe_api_provision_drive_disable_spare_select_unconsumed()
 ***************************************************************/

/*!****************************************************************************
 *          fbe_api_provision_drive_enable_spare_select_unconsumed()
 ******************************************************************************
 *
 * @brief   Enable provision drives configured as `unconsumed' to be used as
 *          spares (this is the default).
 *
 * @param   None. 
 *
 * @return status - The status of the operation.
 *
 * @author
 *  09/21/2012  Ron Proulx  - Created.
 ******************************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_enable_spare_select_unconsumed(void)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_topology_mgmt_update_disable_spare_select_unconsumed_t update_disable_spare_select_unconsumed;

    /* Set the flag to False which will enable ALL provision drives configued
     * as `unconsumed' to be used as a spare drive.
     */
    fbe_zero_memory(&update_disable_spare_select_unconsumed, sizeof(fbe_topology_mgmt_update_disable_spare_select_unconsumed_t));
    update_disable_spare_select_unconsumed.b_disable_spare_select_unconsumed = FBE_FALSE;

    /* send the control packet and wait for its completion. */
    status = fbe_api_common_send_control_packet(FBE_TOPOLOGY_CONTROL_CODE_UPDATE_DISABLE_SPARE_SELECT_UNCONSUMED,
                                                &update_disable_spare_select_unconsumed,
                                                sizeof(fbe_topology_mgmt_update_disable_spare_select_unconsumed_t),
                                                FBE_OBJECT_ID_INVALID,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                NULL,
                                                FBE_PACKAGE_ID_SEP_0);
    
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d.\n", __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/***************************************************************
 * end fbe_api_provision_drive_enable_spare_select_unconsumed()
 ***************************************************************/

/*!****************************************************************************
 *          fbe_api_provision_drive_get_disable_spare_select_unconsumed()
 ****************************************************************************** 
 * 
 * @brief   If b_is_disable_spare_select_unconsumed_set_p:
 *              o FBE_TRUE - This means provision drives that are configure as
 *                  `unconsumed' (a.k.a. `automatic') will NOT be eligiable as
 *                  spares.
 *              o FBE_FALSE - (Default) Any drive configured as `unconsumed' is
 *                  is eligible for use as a spare (`automatic' spare).
 *
 * @param   b_is_disable_spare_select_unconsumed_set_p - Is disable unconsumed set
 *
 * @return status - The status of the operation.
 *
 * @author
 *  09/21/2012  Ron Proulx  - Created.
 ******************************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_disable_spare_select_unconsumed(fbe_bool_t *b_is_disable_spare_select_unconsumed_set_p)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_topology_mgmt_get_disable_spare_select_unconsumed_t disable_spare_select_unconsumed;

    /* Initialize output to default.*/
    fbe_zero_memory(&disable_spare_select_unconsumed, sizeof(fbe_topology_mgmt_update_disable_spare_select_unconsumed_t));
    *b_is_disable_spare_select_unconsumed_set_p = FBE_FALSE;

    /* send the control packet and wait for its completion. */
    status = fbe_api_common_send_control_packet(FBE_TOPOLOGY_CONTROL_CODE_GET_DISABLE_SPARE_SELECT_UNCONSUMED,
                                                &disable_spare_select_unconsumed,
                                                sizeof(fbe_topology_mgmt_get_disable_spare_select_unconsumed_t),
                                                FBE_OBJECT_ID_INVALID,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                NULL,
                                                FBE_PACKAGE_ID_SEP_0);
    
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d.\n", __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *b_is_disable_spare_select_unconsumed_set_p = disable_spare_select_unconsumed.b_disable_spare_select_unconsumed;

    return status;
}
/*******************************************************************
 * end fbe_api_provision_drive_get_disable_spare_select_unconsumed()
 *******************************************************************/

/*!****************************************************************************
 *          fbe_api_provision_drive_send_logical_error()
 ****************************************************************************** 
 * 
 * @brief   This function sends a logical error to PVD.
 *
 * @param   object_id - PVD object ID.
 *          error - logical error type.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  02/12/2013  Lili Chen  - Created.
 ******************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_provision_drive_send_logical_error(fbe_object_id_t object_id, fbe_block_transport_error_type_t error)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_block_transport_control_logical_error_t logical_error;

    logical_error.pvd_object_id = object_id;
    logical_error.error_type = error;

    status = fbe_api_common_send_control_packet (FBE_BLOCK_TRANSPORT_CONTROL_CODE_UPDATE_LOGICAL_ERROR_STATS,
                                                 &logical_error,
                                                 sizeof(logical_error),
                                                 object_id,
                                                 FBE_PACKET_FLAG_TRAVERSE,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/*******************************************************************
 * end fbe_api_provision_drive_send_logical_error()
 *******************************************************************/
/*!***************************************************************
 *  fbe_api_provision_drive_get_paged_bits()
 ****************************************************************
 * @brief
 *  This function returns the summation of the paged bits
 *  from the pvd's paged metadata.
 *
 * @param pvd_object_id - RG object id
 * @param paged_info_p - Ptr to paged summation structure.
 * @param b_force_unit_access - FBE_TRUE - Get the paged data from disk (not from cache)
 *                              FBE_FALSE - Allow the data to come from cache   
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  4/9/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_provision_drive_get_paged_bits(fbe_object_id_t pvd_object_id,
                                       fbe_api_provision_drive_get_paged_info_t *paged_info_p,
                                       fbe_bool_t b_force_unit_access)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_provision_drive_info_t pvd_info;
    fbe_u32_t index;
    fbe_chunk_index_t total_chunks;
    fbe_chunk_index_t chunks_remaining;
    fbe_u64_t metadata_offset = 0;
    fbe_chunk_index_t current_chunks;
    fbe_chunk_index_t chunks_per_request;
    fbe_api_base_config_metadata_paged_get_bits_t get_bits;

    if (paged_info_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Clear this out since we will be adding to it.
     */
    fbe_zero_memory(paged_info_p, sizeof(fbe_api_provision_drive_get_paged_info_t));

    /* Get the information about the capacity so we can calculate number of chunks.
     */
    status = fbe_api_provision_drive_get_info(pvd_object_id, &pvd_info);

    if (status != FBE_STATUS_OK) 
    { 
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: rg get info status: 0x%x\n", __FUNCTION__, status);
        return status; 
    }

    total_chunks = pvd_info.capacity / FBE_RAID_DEFAULT_CHUNK_SIZE;
    chunks_remaining = total_chunks;
    chunks_per_request = FBE_PAYLOAD_METADATA_MAX_DATA_SIZE / sizeof(fbe_provision_drive_paged_metadata_t);

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

    /* Loop over all the chunks, and sum up the bits across all chunks. 
     * We will fetch as many chunks as we can per request (chunks_per_request).
     */
    while (chunks_remaining > 0)
    {
        fbe_u32_t wait_msecs = 0;
        fbe_provision_drive_paged_metadata_t *paged_p = (fbe_provision_drive_paged_metadata_t *)&get_bits.metadata_record_data[0];

        current_chunks = FBE_MIN(chunks_per_request, chunks_remaining);
        if (current_chunks > FBE_U32_MAX)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: current_chunks: %d > FBE_U32_MAX\n", __FUNCTION__, (int)current_chunks);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        get_bits.metadata_offset = metadata_offset;
        get_bits.metadata_record_data_size = (fbe_u32_t)(current_chunks * sizeof(fbe_provision_drive_paged_metadata_t));

        /* Fetch the bits for this set of chunks.  It is possible to get errors back
         * if the object is quiescing, so just retry briefly. 
         */
        status = fbe_api_base_config_metadata_paged_get_bits(pvd_object_id, &get_bits);
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

            status = fbe_api_base_config_metadata_paged_get_bits(pvd_object_id, &get_bits);
        }
        /* For each chunk in this request keep track of how many were marked NR.
         */
        for ( index = 0; index < current_chunks; index++)
        {
            fbe_u32_t combinations_index = 0;
            paged_info_p->chunk_count++;
            if (paged_p->valid_bit)
            {
                paged_info_p->num_valid_chunks++;
                combinations_index |= 1;
            }
            if (paged_p->need_zero_bit)
            {
                paged_info_p->num_nz_chunks++;

                combinations_index |= (1 << 1);
            }
            if (paged_p->user_zero_bit)
            {
                paged_info_p->num_uz_chunks++;
                combinations_index |= (1 << 2);
            }
            if (paged_p->consumed_user_data_bit)
            {
                paged_info_p->num_cons_chunks++;
                combinations_index |= (1 << 3);
            }
            paged_info_p->bit_combinations_count[combinations_index]++;
            paged_p++;
        }
        metadata_offset += current_chunks * sizeof(fbe_provision_drive_paged_metadata_t);
        chunks_remaining -= current_chunks;
    }
    return status;
}
/**************************************
 * end fbe_api_provision_drive_get_paged_bits()
 **************************************/
/*!****************************************************************************
 *          fbe_api_provision_drive_update_pool_id_list()
 ****************************************************************************** 
 * 
 * @brief   This function updates a list of pvd's pool id.
 *
 * @param   pvd_pool_list.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  04/07/2013  Hongpo Gao  - Created.
 ******************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_provision_drive_update_pool_id_list(fbe_api_provision_drive_update_pool_id_list_t *pvd_pool_list)
{
    fbe_api_job_service_update_multi_pvds_pool_id_t        job_update_multi_pvd_pool_id;
    fbe_status_t                                    status;
    fbe_status_t                                    job_status;
    fbe_job_service_error_type_t                    js_error_type = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    if (pvd_pool_list == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "Input arguments is NULL. \n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Check if the pvd pool data exceed the max allowed number. There is limit in database transaction  */
    if (pvd_pool_list->pvd_pool_data_list.total_elements > MAX_UPDATE_PVD_POOL_ID)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "The total elements number(%d) exceeds the max allowed number(%d).\n",
                      pvd_pool_list->pvd_pool_data_list.total_elements, MAX_UPDATE_PVD_POOL_ID);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(&job_update_multi_pvd_pool_id, sizeof(fbe_api_job_service_update_multi_pvds_pool_id_t));
    fbe_copy_memory(&job_update_multi_pvd_pool_id.pvd_pool_data_list, &pvd_pool_list->pvd_pool_data_list,
                    sizeof(job_update_multi_pvd_pool_id.pvd_pool_data_list));

    /* update the provision drive pool-id. */
    status = fbe_api_job_service_update_multi_pvds_pool_id(&job_update_multi_pvd_pool_id);
    if (status != FBE_STATUS_OK) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "PVD update_multi_pvd_pool_id: Failed.  Status=0x%x\n", status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* wait for the notification from the job service. */
    status = fbe_api_common_wait_for_job(job_update_multi_pvd_pool_id.job_number,
                                         PVD_UPDATE_CONFIG_TYPE_WAIT_TIMEOUT,
                                         &js_error_type,
                                         &job_status,
                                         NULL);

    if ((status != FBE_STATUS_OK) || (job_status != FBE_STATUS_OK) || (js_error_type != FBE_JOB_SERVICE_ERROR_NO_ERROR)) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                      "PVD update_multi_pvd_pool_id: timedout. Status=0x%X, Job Status=0x%X, Job Error=0x%x\n",
                      status, job_status, js_error_type);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/*******************************************************************
 * end fbe_api_provision_drive_update_pool_id_list()
 *******************************************************************/
/*!***************************************************************
 * @fn fbe_api_provision_drive_get_miniport_key_handle(
 *      fbe_object_id_t object_id, 
 *      fbe_provision_drive_control_get_mp_key_handle_t * get_mp_key_handle)
 ****************************************************************
 * @brief
 *  This function is for internal (debugging) use ONLY.
 *  This returns the handle to key that is stored in the miniport.
 *
 * @param object_id - object ID
 * @param get_mp_key_handle - Miniport Key handle
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  03/17/12 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_miniport_key_handle(fbe_object_id_t object_id, fbe_provision_drive_control_get_mp_key_handle_t *get_mp_key_handle)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    
    status = fbe_api_common_send_control_packet(FBE_PROVISION_DRIVE_CONTROL_CODE_GET_MINIPORT_KEY_HANDLE,
                                                get_mp_key_handle,
                                                sizeof(fbe_provision_drive_control_get_mp_key_handle_t),
                                                object_id,
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

    return status;
}
/*******************************************************************
 * end fbe_api_provision_drive_get_miniport_key_handle()
 *******************************************************************/


/*!***************************************************************
 * @fn fbe_api_provision_drive_quiesce(fbe_object_id_t object_id)
 ****************************************************************
 * @brief
 *  This function starts quiesce for a provision drive object.
 *
 * @param object_id - object ID
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  03/14/14 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_quiesce(fbe_object_id_t object_id)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    status = fbe_api_common_send_control_packet (FBE_PROVISION_DRIVE_CONTROL_CODE_QUIESCE,
                                                 NULL, /* no struct*/
                                                 0, /* no size */
                                                 object_id,
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
/*******************************************************************
 * end fbe_api_provision_drive_quiesce()
 *******************************************************************/

/*!***************************************************************
 * @fn fbe_api_provision_drive_unquiesce(fbe_object_id_t object_id)
 ****************************************************************
 * @brief
 *  This function starts quiesce for a provision drive object.
 *
 * @param object_id - object ID
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  03/14/14 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_unquiesce(fbe_object_id_t object_id)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    status = fbe_api_common_send_control_packet (FBE_PROVISION_DRIVE_CONTROL_CODE_UNQUIESCE,
                                                 NULL, /* no struct*/
                                                 0, /* no size */
                                                 object_id,
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
/*******************************************************************
 * end fbe_api_provision_drive_unquiesce()
 *******************************************************************/

/*!***************************************************************
 * @fn fbe_api_provision_drive_get_clustered_flag(fbe_object_id_t object_id)
 ****************************************************************
 * @brief
 *  This function starts quiesce for a provision drive object.
 *
 * @param object_id - object ID
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  03/14/14 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_clustered_flag(fbe_object_id_t object_id, fbe_base_config_clustered_flags_t *flags)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_base_config_clustered_flags_t               base_config_clustered_flags;

    status = fbe_api_common_send_control_packet (FBE_PROVISION_DRIVE_CONTROL_CODE_GET_CLUSTERED_FLAG,
                                                 &base_config_clustered_flags, /* no struct*/
                                                 sizeof(fbe_base_config_clustered_flags_t), /* no size */
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    *flags = base_config_clustered_flags;

    return status;
}
/*******************************************************************
 * end fbe_api_provision_drive_get_clustered_flag()
 *******************************************************************/

/*!***************************************************************
 * @fn fbe_api_provision_drive_disable_paged_cache
 ****************************************************************
 * @brief
 *  This function disables paged metadata cache.
 *
 * @param object_id - object ID
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  05/15/14 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_disable_paged_cache(fbe_object_id_t object_id)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    status = fbe_api_common_send_control_packet (FBE_PROVISION_DRIVE_CONTROL_CODE_DISABLE_PAGED_CACHE,
                                                 NULL, /* no struct*/
                                                 0, /* no size */
                                                 object_id,
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
/*******************************************************************
 * end fbe_api_provision_drive_disable_paged_cache()
 *******************************************************************/

/*!***************************************************************
 * @fn fbe_api_provision_drive_get_paged_cache_info
 ****************************************************************
 * @brief
 *  This function gets paged metadata cache information.
 *
 * @param object_id - object ID
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  05/15/14 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_paged_cache_info(fbe_object_id_t object_id, fbe_provision_drive_get_paged_cache_info_t *get_info)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_provision_drive_get_paged_cache_info_t      get_paged_info;

    status = fbe_api_common_send_control_packet (FBE_PROVISION_DRIVE_CONTROL_CODE_GET_PAGED_CACHE_INFO,
                                                 &get_paged_info,
                                                 sizeof(fbe_provision_drive_get_paged_cache_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    get_info->miss_count = get_paged_info.miss_count;
    get_info->hit_count = get_paged_info.hit_count;

    return status;
}
/*******************************************************************
 * end fbe_api_provision_drive_get_paged_cache_info()
 *******************************************************************/

/*!***************************************************************
 * @fn fbe_api_provision_drive_set_swap_pending()
 *****************************************************************
 * @brief
 *  This function marks the specified provision drive: `logically 
 *  offline'.
 *
 * @param   pvd_object_id - provision drive object id
 *
 * @return
 *  fbe_status_t
 *
 * @note    See `fbe_api_provision_drive_set_set_pvd_swap_pending_with_reason'
 *          if you wish to specify the reason.
 *
 * @version
 *  06/05/2014  Ron Proulx  - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_provision_drive_set_swap_pending(fbe_object_id_t pvd_object_id)

{
    fbe_status_t                                status;
    fbe_api_provision_drive_info_t              provision_drive_info;
    fbe_provision_drive_set_swap_pending_t      set_pvd_swap_pending;
    fbe_provision_drive_swap_pending_reason_t   set_pvd_swap_pending_reason = FBE_PROVISION_DRIVE_DRIVE_SWAP_PENDING_REASON_PROACTIVE_COPY;
    fbe_api_control_operation_status_info_t     status_info;

    /* Determine the reason.
     */
    status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info);
    if (status == FBE_STATUS_OK) {
        if (provision_drive_info.end_of_life_state == FBE_FALSE) {
            set_pvd_swap_pending_reason = FBE_PROVISION_DRIVE_DRIVE_SWAP_PENDING_REASON_USER_COPY;
        }
    }

    /* Clear the set.
     */
    fbe_zero_memory(&set_pvd_swap_pending, sizeof(fbe_provision_drive_set_swap_pending_t));
    set_pvd_swap_pending.set_swap_pending_reason = set_pvd_swap_pending_reason;
    status = fbe_api_common_send_control_packet(FBE_PROVISION_DRIVE_CONTROL_SET_SWAP_PENDING,
                                                &set_pvd_swap_pending,
                                                sizeof(fbe_provision_drive_set_swap_pending_t),
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

    return status;
}   
/***************************************************************
 * end fbe_api_provision_drive_set_swap_pending()
 ***************************************************************/

/*!***************************************************************
 * @fn fbe_api_provision_drive_clear_swap_pending()
 *****************************************************************
 * @brief
 *  This function clears the `logical marked offline'.
 *
 * @param pvd_object_id - provision drive object id
 *
 * @return
 *  fbe_status_t

 *
 * @version
 *  06/05/2014  Ron Proulx  - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_provision_drive_clear_swap_pending(fbe_object_id_t pvd_object_id)

{
    fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;

    /* No payload.
     */
    status = fbe_api_common_send_control_packet(FBE_PROVISION_DRIVE_CONTROL_CLEAR_SWAP_PENDING,
                                                NULL,
                                                0,
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

    return status;
}   
/***************************************************************
 * end fbe_api_provision_drive_clear_swap_pending()
 ***************************************************************/

/*!***************************************************************
 * @fn fbe_api_provision_drive_get_swap_pending
 *****************************************************************
 * @brief
 *  This function gets the setting of `mark pvd offline' NP flags.
 *
 * @param   pvd_object_id - provision drive object id
 * @param   get_swap_pending_info_p - Pointer to reason (or none if not marked)
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  06/05/2014  Ron Proulx  - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_provision_drive_get_swap_pending(fbe_object_id_t pvd_object_id,
                                                                   fbe_api_provision_drive_get_swap_pending_t *get_swap_pending_info_p)

{
    fbe_status_t                            status;
    fbe_provision_drive_get_swap_pending_t  get_swap_pending;
    fbe_api_control_operation_status_info_t status_info;

    /* Clear the get.
     */
    fbe_zero_memory(&get_swap_pending, sizeof(fbe_provision_drive_get_swap_pending_t));
    status = fbe_api_common_send_control_packet(FBE_PROVISION_DRIVE_CONTROL_GET_SWAP_PENDING,
                                                &get_swap_pending,
                                                sizeof(fbe_provision_drive_get_swap_pending_t),
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

    get_swap_pending_info_p->get_swap_pending_reason = get_swap_pending.get_swap_pending_reason;
    return status;
}   
/***************************************************************
 * end fbe_api_provision_drive_get_swap_pending()
 ***************************************************************/

/*!***************************************************************
 * @fn fbe_api_provision_drive_set_set_pvd_swap_pending_with_reason()
 *****************************************************************
 * @brief
 *  This function marks the specified provision drive: `logically 
 *  offline' with the reason provided.
 *
 * @param   pvd_object_id - provision drive object id
 * @param   set_swap_pending_p - The reason to set the `swap pending'
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  06/05/2014  Ron Proulx  - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_provision_drive_set_set_pvd_swap_pending_with_reason(fbe_object_id_t pvd_object_id,
                                                               fbe_api_provision_drive_set_swap_pending_t *set_swap_pending_p)

{
    fbe_status_t                            status;
    fbe_provision_drive_set_swap_pending_t  set_pvd_swap_pending;
    fbe_api_control_operation_status_info_t status_info;

    /* Clear the set.
     */
    fbe_zero_memory(&set_pvd_swap_pending, sizeof(fbe_provision_drive_set_swap_pending_t));
    set_pvd_swap_pending.set_swap_pending_reason = set_swap_pending_p->set_swap_pending_reason;
    status = fbe_api_common_send_control_packet(FBE_PROVISION_DRIVE_CONTROL_SET_SWAP_PENDING,
                                                &set_pvd_swap_pending,
                                                sizeof(fbe_provision_drive_set_swap_pending_t),
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

    return status;
}   
/***************************************************************
 * end fbe_api_provision_drive_set_set_pvd_swap_pending_with_reason()
 ***************************************************************/

/*!***************************************************************
 * @fn fbe_api_provision_drive_set_eas_start()
 *****************************************************************
 * @brief
 *  This function sets EAS start to a provision drive.
 *
 * @param   pvd_object_id - provision drive object id
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  07/22/2014  Lili Chen  - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_provision_drive_set_eas_start(fbe_object_id_t pvd_object_id)
{
    fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;

    status = fbe_api_common_send_control_packet(FBE_PROVISION_DRIVE_CONTROL_SET_EAS_START,
                                                NULL,
                                                0,
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

    return status;
}   
/***************************************************************
 * end fbe_api_provision_drive_set_eas_start()
 ***************************************************************/

/*!***************************************************************
 * @fn fbe_api_provision_drive_set_eas_complete()
 *****************************************************************
 * @brief
 *  This function sets EAS complete to a provision drive.
 *
 * @param   pvd_object_id - provision drive object id
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  07/22/2014  Lili Chen  - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_provision_drive_set_eas_complete(fbe_object_id_t pvd_object_id)
{
    fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;

    status = fbe_api_common_send_control_packet(FBE_PROVISION_DRIVE_CONTROL_SET_EAS_COMPLETE,
                                                NULL,
                                                0,
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

    return status;
}   
/***************************************************************
 * end fbe_api_provision_drive_set_eas_complete()
 ***************************************************************/

/*!***************************************************************
 * @fn fbe_api_provision_drive_get_eas_state()
 *****************************************************************
 * @brief
 *  This function gets EAS state from a provision drive.
 *
 * @param   pvd_object_id - provision drive object id
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  07/22/2014  Lili Chen  - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_provision_drive_get_eas_state(fbe_object_id_t pvd_object_id,
                                                                fbe_bool_t * is_started, fbe_bool_t * is_complete)
{
    fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_provision_drive_get_eas_state_t         get_state_p;

    fbe_zero_memory(&get_state_p, sizeof(fbe_provision_drive_get_eas_state_t));
    status = fbe_api_common_send_control_packet(FBE_PROVISION_DRIVE_CONTROL_GET_EAS_STATE,
                                                &get_state_p,
                                                sizeof(fbe_provision_drive_get_eas_state_t),
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

    *is_started = get_state_p.is_started;
    *is_complete = get_state_p.is_complete;
    return status;
}   
/***************************************************************
 * end fbe_api_provision_drive_get_eas_state()
 ***************************************************************/

/*!***************************************************************
 * @fn fbe_api_provision_drive_get_ssd_statistics()
 *****************************************************************
 * @brief
 *  This function gets SSD drive statistics.
 *
 * @param   pvd_object_id - provision drive object id
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  10/03/2014  Lili Chen  - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_provision_drive_get_ssd_statistics(fbe_object_id_t pvd_object_id,
                                                                     fbe_api_provision_drive_get_ssd_statistics_t * get_stats_p)
{
    fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_provision_drive_get_ssd_statistics_t    get_stats;

    fbe_zero_memory(&get_stats, sizeof(fbe_provision_drive_get_ssd_statistics_t));
    status = fbe_api_common_send_control_packet(FBE_PROVISION_DRIVE_CONTROL_CODE_GET_SSD_STATISTICS,
                                                &get_stats,
                                                sizeof(fbe_provision_drive_get_ssd_statistics_t),
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

    fbe_copy_memory(&get_stats_p->get_ssd_statistics, &get_stats, sizeof(fbe_provision_drive_get_ssd_statistics_t));

    return status;
}   
/***************************************************************
 * end fbe_api_provision_drive_get_ssd_statistics()
 ***************************************************************/

/*!***************************************************************
 * @fn fbe_api_provision_drive_get_ssd_block_limits()
 *****************************************************************
 * @brief
 *  This function gets SSD drive statistics.
 *
 * @param   pvd_object_id - provision drive object id
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  10/03/2014  Lili Chen  - Created.
 *  4/20/2015   Deanna Heng - Modified. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_provision_drive_get_ssd_block_limits(fbe_object_id_t pvd_object_id,
                                                                       fbe_api_provision_drive_get_ssd_block_limits_t * get_block_limits_p)
{
    fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_provision_drive_get_ssd_block_limits_t    get_block_limits;

    fbe_zero_memory(&get_block_limits, sizeof(fbe_provision_drive_get_ssd_block_limits_t));
    status = fbe_api_common_send_control_packet(FBE_PROVISION_DRIVE_CONTROL_CODE_GET_SSD_BLOCK_LIMITS,
                                                &get_block_limits,
                                                sizeof(fbe_provision_drive_get_ssd_block_limits_t),
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

    fbe_copy_memory(&get_block_limits_p->get_ssd_block_limits, &get_block_limits, sizeof(fbe_provision_drive_get_ssd_block_limits_t));

    return status;
}   
/***************************************************************
 * end fbe_api_provision_drive_get_ssd_block_limits()
 ***************************************************************/

/*!***************************************************************
 * @fn fbe_api_provision_drive_set_wear_leveling_timer()
 *****************************************************************
 * @brief
 *  This function sets the wear leveling timer
 *
 * @param   pvd_object_id - provision drive object id
 * @param   wear_leveling_timer_sec - the wear leveling timer in sec
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  8/04/2015   Deanna Heng - Modified. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_provision_drive_set_wear_leveling_timer(fbe_u64_t wear_leveling_timer_sec)
{
    fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_u64_t                                  wear_leveling_timer;
    wear_leveling_timer = wear_leveling_timer_sec;

    status = fbe_api_common_send_control_packet_to_class (FBE_PROVISION_DRIVE_CONTROL_CODE_SET_CLASS_WEAR_LEVELING_TIMER,
                                                          &wear_leveling_timer,
                                                          sizeof(fbe_u64_t),
                                                          FBE_CLASS_ID_PROVISION_DRIVE,
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

    return status;
}   
/***************************************************************
 * end fbe_api_provision_drive_set_wear_leveling_timer()
 ***************************************************************/


/***********************************************
 * end file fbe_api_provision_drive_interface.c
 ***********************************************/
