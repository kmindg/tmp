/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_bvd_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the fbe_bvd interface for the storage extent package.
 *
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * @ingroup fbe_api_bvd_interface
 *
 * @version
 *
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
#include "fbe/fbe_bvd_interface.h"

/**************************************
                Local variables
**************************************/

/*******************************************
                Local functions
********************************************/

/****************************************************************/
/** @fn fbe_api_bvd_interface_get_downstream_attr(fbe_object_id_t bvd_object_id, fbe_block_path_attr_flags_t* downstream_attr_p, fbe_lun_number_t lu_number)
 ****************************************************************
 * @brief
 *  This function returns the BVD object's downstream atribute field.
 *
 * @param bvd_object_id         - BVD object ID
 * @param downstream_event_p    - pointer to BVD object event
 * @param lu_number		- The lu number we want to know the event for
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_bvd_interface_get_downstream_attr(fbe_object_id_t bvd_object_id, fbe_volume_attributes_flags* downstream_attr_p, fbe_object_id_t lu_object_id)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_bvd_interface_get_downstream_attr_t     bvd_downstream_attr;


    bvd_downstream_attr.lun_object_id = lu_object_id;
    status = fbe_api_common_send_control_packet( FBE_BVD_INTERFACE_CONTROL_CODE_GET_ATTR,
                                                 &bvd_downstream_attr,
                                                 sizeof (fbe_bvd_interface_get_downstream_attr_t),
                                                 bvd_object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0
                                               );
    
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status,
                       status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    *downstream_attr_p = bvd_downstream_attr.attr;
    return status;    
}

/*!*******************************************************************
 * @fn fbe_api_bvd_interface_get_bvd_object_id(fbe_object_id_t *object_id )
 *********************************************************************
 * @brief
 *  This function returns the object id of the BVD Object.
 *
 * @param  object_id - pointer to the object ID
 *
 * @return fbe_status_t, success or failure
 *
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_bvd_interface_get_bvd_object_id(fbe_object_id_t *object_id)
{
	fbe_topology_control_get_bvd_id_t	        get_bvd_id;
	fbe_status_t								status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_control_operation_status_info_t		status_info;

    if (object_id == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    get_bvd_id.bvd_object_id = FBE_OBJECT_ID_INVALID;

    status = fbe_api_common_send_control_packet_to_service (FBE_TOPOLOGY_CONTROL_CODE_GET_BVD_OBJECT_ID,
															 &get_bvd_id,
															 sizeof(fbe_topology_control_get_bvd_id_t),
															 FBE_SERVICE_ID_TOPOLOGY,
															 FBE_PACKET_FLAG_NO_ATTRIB,
															 &status_info,
															 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        if ((status_info.control_operation_status == FBE_PAYLOAD_CONTROL_STATUS_FAILURE) && 
			 (status_info.control_operation_qualifier == FBE_OBJECT_ID_INVALID)) 
        {
            /*there was not object in this location, just return invalid id and good status*/
            *object_id = FBE_OBJECT_ID_INVALID;
            return FBE_STATUS_OK;
        }

        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
            status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *object_id = get_bvd_id.bvd_object_id;

    return status;

}

/*!*******************************************************************
 * @fn fbe_api_bvd_interface_enable_peformance_statistics(void)
 *********************************************************************
 * @brief
 *  This function enables the performance statistics assoicated with the BVD object
 *
 * @param  
 *
 * @return fbe_status_t, success or failure
 *
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_bvd_interface_enable_peformance_statistics(void)
{
    fbe_status_t                                 status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t      status_info;
    fbe_object_id_t                              bvd_id;


    /*first get BVD object ID*/
    status = fbe_api_bvd_interface_get_bvd_object_id(&bvd_id);
    if (status != FBE_STATUS_OK ){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:failed to get BVD ID\n", __FUNCTION__);
        return status;
    }

    /*and ask it to get stat*/
    status = fbe_api_common_send_control_packet (FBE_BVD_INTERFACE_CONTROL_CODE_ENABLE_PERFORMANCE_STATS,
                                                 NULL,
                                                 0,
                                                 bvd_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status,
                       status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
  

    return status;

}

/*!*******************************************************************
 * @fn fbe_api_bvd_interface_disable_peformance_statistics(void)
 *********************************************************************
 * @brief
 *  This function enables the performance statistics assoicated with the BVD object
 *
 * @param  
 *
 * @return fbe_status_t, success or failure
 *
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_bvd_interface_disable_peformance_statistics(void)
{
    fbe_status_t                                 status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t      status_info;
    fbe_object_id_t                              bvd_id;


    /*first get BVD object ID*/
    status = fbe_api_bvd_interface_get_bvd_object_id(&bvd_id);
    if (status != FBE_STATUS_OK ){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:failed to get BVD ID\n", __FUNCTION__);
        return status;
    }

    /*and ask it to get stat*/
    status = fbe_api_common_send_control_packet (FBE_BVD_INTERFACE_CONTROL_CODE_DISABLE_PERFORMANCE_STATS,
                                                 NULL,
                                                 0,
                                                 bvd_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status,
                       status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
  

    return status;

}

/*!*******************************************************************
 * @fn fbe_api_bvd_interface_get_peformance_statistics()
 *********************************************************************
 * @brief
 *  This function get the performance statistics assoicated with the BVD object
 *
 * @param  bvd_perf_stats - pointer to the performance statistics structure to fill
 *
 * @return fbe_status_t, success or failure
 *
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_bvd_interface_get_peformance_statistics(fbe_sep_shim_get_perf_stat_t *bvd_perf_stats)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_object_id_t                             bvd_id;

    if (bvd_perf_stats == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*first get BVD object ID*/
    status = fbe_api_bvd_interface_get_bvd_object_id(&bvd_id);
    if (status != FBE_STATUS_OK ){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:failed to get BVD ID\n", __FUNCTION__);
        return status;
    }

    /*and ask it to get stat*/
    status = fbe_api_common_send_control_packet (FBE_BVD_INTERFACE_CONTROL_CODE_GET_PERFORMANCE_STATS,
                                                 bvd_perf_stats,
                                                 sizeof(fbe_sep_shim_get_perf_stat_t),
                                                 bvd_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status,
                       status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
  

    return status;

}


/*!*******************************************************************
 * @fn fbe_api_bvd_interface_clear_peformance_statistics(void)
 *********************************************************************
 * @brief
 *  This function clears the performance statistics assoicated with the BVD object
 *
 * @param  
 *
 * @return fbe_status_t, success or failure
 *
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_bvd_interface_clear_peformance_statistics(void)
{
	fbe_status_t                                 status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t      status_info;
    fbe_object_id_t                              bvd_id;


    /*first get BVD object ID*/
    status = fbe_api_bvd_interface_get_bvd_object_id(&bvd_id);
    if (status != FBE_STATUS_OK ){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:failed to get BVD ID\n", __FUNCTION__);
        return status;
    }

    /*and ask it to get stat*/
    status = fbe_api_common_send_control_packet (FBE_BVD_INTERFACE_CONTROL_CODE_CLEAR_PERFORMANCE_STATS,
                                                 NULL,
                                                 0,
                                                 bvd_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status,
                       status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
  

    return status;

}

/*!*******************************************************************
 * @fn fbe_api_bvd_interface_enable_async_io(void)
 *********************************************************************
 * @brief
 *  This function enables async I/O handling by SEP *
 *
 * @param  
 *
 * @return fbe_status_t, success or failure
 *
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_bvd_interface_enable_async_io(void)
{
    fbe_status_t                                 status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t      status_info;
    fbe_object_id_t                              bvd_id;


    /*first get BVD object ID*/
    status = fbe_api_bvd_interface_get_bvd_object_id(&bvd_id);
    if (status != FBE_STATUS_OK ){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:failed to get BVD ID\n", __FUNCTION__);
        return status;
    }

    /*and ask it to get stat*/
    status = fbe_api_common_send_control_packet (FBE_BVD_INTERFACE_CONTROL_CODE_ENABLE_ASYNC_IO,
                                                 NULL,
                                                 0,
                                                 bvd_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status,
                       status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
  

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_bvd_interface_disable_async_io(void)
 *********************************************************************
 * @brief
 *  This function disables async I/O handling by SEP
 *
 * @param  
 *
 * @return fbe_status_t, success or failure
 *
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_bvd_interface_disable_async_io(void)
{
    fbe_status_t                                 status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t      status_info;
    fbe_object_id_t                              bvd_id;


    /*first get BVD object ID*/
    status = fbe_api_bvd_interface_get_bvd_object_id(&bvd_id);
    if (status != FBE_STATUS_OK ){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:failed to get BVD ID\n", __FUNCTION__);
        return status;
    }

    /*and ask it to get stat*/
    status = fbe_api_common_send_control_packet (FBE_BVD_INTERFACE_CONTROL_CODE_DISABLE_ASYNC_IO,
                                                 NULL,
                                                 0,
                                                 bvd_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status,
                       status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
  

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_bvd_interface_enable_group_priority(fbe_bool_t apply_to_pp)
 *********************************************************************
 * @brief
 *  This function enabled group priority queuing by SEP
 *
 * @param  
 *  fbe_bool_t apply_to_pp - should this toogle the physical pacakge
 * @return fbe_status_t, success or failure
 *
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_bvd_interface_enable_group_priority(fbe_bool_t apply_to_pp)
{
    fbe_status_t                                 status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t      status_info;
    fbe_object_id_t                              bvd_id;
    fbe_payload_control_operation_opcode_t       op_code;


    /*first get BVD object ID*/
    status = fbe_api_bvd_interface_get_bvd_object_id(&bvd_id);
    if (status != FBE_STATUS_OK ){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:failed to get BVD ID\n", __FUNCTION__);
        return status;
    }

    if (apply_to_pp) {
        op_code = FBE_BVD_INTERFACE_CONTROL_CODE_ENABLE_PP_GROUP_PRIORITY;
    } else {
        op_code = FBE_BVD_INTERFACE_CONTROL_CODE_ENABLE_GROUP_PRIORITY;
    }
    
    /*and ask it to get stat*/
    status = fbe_api_common_send_control_packet (op_code,
                                                 NULL,
                                                 0,
                                                 bvd_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status,
                       status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
  

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_bvd_interface_disable_group_priority(fbe_bool_t apply_to_pp)
 *********************************************************************
 * @brief
 *  This function disables group priority queuing by SEP
 *
 * @param  
 *  fbe_bool_t apply_to_pp - should this toogle the physical pacakge
 * @return fbe_status_t, success or failure
 *
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_bvd_interface_disable_group_priority(fbe_bool_t apply_to_pp)
{
    fbe_status_t                                 status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t      status_info;
    fbe_object_id_t                              bvd_id;
    fbe_payload_control_operation_opcode_t       op_code;


    /*first get BVD object ID*/
    status = fbe_api_bvd_interface_get_bvd_object_id(&bvd_id);
    if (status != FBE_STATUS_OK ){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:failed to get BVD ID\n", __FUNCTION__);
        return status;
    }

    if (apply_to_pp) {
        op_code = FBE_BVD_INTERFACE_CONTROL_CODE_ENABLE_PP_GROUP_PRIORITY;
    } else {
        op_code = FBE_BVD_INTERFACE_CONTROL_CODE_ENABLE_GROUP_PRIORITY;
    }

    /*and ask it to get stat*/
    status = fbe_api_common_send_control_packet (op_code,
                                                 NULL,
                                                 0,
                                                 bvd_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status,
                       status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
  

    return status;
}

fbe_status_t FBE_API_CALL fbe_api_bvd_interface_enable_async_io_compl(void)
{
    fbe_status_t                                 status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t      status_info;
    fbe_object_id_t                              bvd_id;


    /*first get BVD object ID*/
    status = fbe_api_bvd_interface_get_bvd_object_id(&bvd_id);
    if (status != FBE_STATUS_OK ){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:failed to get BVD ID\n", __FUNCTION__);
        return status;
    }

    /*and ask it to get stat*/
    status = fbe_api_common_send_control_packet (FBE_BVD_INTERFACE_CONTROL_CODE_ENABLE_ASYNC_IO_COMPL,
                                                 NULL,
                                                 0,
                                                 bvd_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status,
                       status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
  

    return status;
}


fbe_status_t FBE_API_CALL fbe_api_bvd_interface_disable_async_io_compl(void)
{
    fbe_status_t                                 status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t      status_info;
    fbe_object_id_t                              bvd_id;


    /*first get BVD object ID*/
    status = fbe_api_bvd_interface_get_bvd_object_id(&bvd_id);
    if (status != FBE_STATUS_OK ){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:failed to get BVD ID\n", __FUNCTION__);
        return status;
    }

    /*and ask it to get stat*/
    status = fbe_api_common_send_control_packet (FBE_BVD_INTERFACE_CONTROL_CODE_DISABLE_ASYNC_IO_COMPL,
                                                 NULL,
                                                 0,
                                                 bvd_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status,
                       status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
  

    return status;
}

fbe_status_t FBE_API_CALL fbe_api_bvd_interface_set_rq_method(fbe_transport_rq_method_t rq_method)
{
    fbe_status_t                                 status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t      status_info;
    fbe_object_id_t                              bvd_id;
	fbe_sep_shim_set_rq_method_t				set_rq_method;

    /*first get BVD object ID*/
    status = fbe_api_bvd_interface_get_bvd_object_id(&bvd_id);
    if (status != FBE_STATUS_OK ){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:failed to get BVD ID\n", __FUNCTION__);
        return status;
    }

	set_rq_method.rq_method = rq_method;
    /*and ask it to get stat*/
    status = fbe_api_common_send_control_packet (FBE_BVD_INTERFACE_CONTROL_CODE_SET_RQ_METHOD,
                                                 &set_rq_method,
                                                 sizeof(fbe_sep_shim_set_rq_method_t),
                                                 bvd_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status,
                       status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
  
    return status;
}


fbe_status_t FBE_API_CALL fbe_api_bvd_interface_set_alert_time(fbe_u32_t alert_time)
{
    fbe_status_t                                 status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t      status_info;
    fbe_object_id_t                              bvd_id;
	fbe_sep_shim_set_alert_time_t				set_alert_time;

    /*first get BVD object ID*/
    status = fbe_api_bvd_interface_get_bvd_object_id(&bvd_id);
    if (status != FBE_STATUS_OK ){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:failed to get BVD ID\n", __FUNCTION__);
        return status;
    }

	set_alert_time.alert_time = alert_time;
    /*and ask it to get stat*/
    status = fbe_api_common_send_control_packet (FBE_BVD_INTERFACE_CONTROL_CODE_SET_ALERT_TIME,
                                                 &set_alert_time,
                                                 sizeof(fbe_sep_shim_set_alert_time_t),
                                                 bvd_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status,
                       status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
  
    return status;
}
/*!*******************************************************************
 * fbe_api_bvd_interface_shutdown()
 *********************************************************************
 * @brief
 *  Send a shutdown command to BVD.  This stops all I/Os in preparation for reboot.
 *
 * @param None.
 *
 * @return fbe_status_t, success or failure
 *
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_bvd_interface_shutdown(void)
{
    fbe_status_t                                 status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t      status_info;
    fbe_object_id_t                              bvd_id;

    /*first get BVD object ID*/
    status = fbe_api_bvd_interface_get_bvd_object_id(&bvd_id);
    if (status != FBE_STATUS_OK ){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:failed to get BVD ID\n", __FUNCTION__);
        return status;
    }

    status = fbe_api_common_send_control_packet (FBE_BVD_INTERFACE_CONTROL_CODE_SHUTDOWN,
                                                 NULL, /* No buffer */
                                                 0,    /* Buffer length 0 */
                                                 bvd_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status,
                       status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
  

    return status;
}

/*!***************************************************************
 * fbe_api_bvd_interface_unexport_lun()
 ****************************************************************
 * @brief
 *  Unexport LUN from blockshim
 *
 * @param lun_object_id             - The LUN object ID
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_bvd_interface_unexport_lun(fbe_object_id_t lun_object_id)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t status_info;
    fbe_object_id_t bvd_id;

    /*first get BVD object ID*/
    status = fbe_api_bvd_interface_get_bvd_object_id(&bvd_id);
    if (status != FBE_STATUS_OK ){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:failed to get BVD ID\n", __FUNCTION__);
        return status;
    }

    status = fbe_api_common_send_control_packet(FBE_BVD_INTERFACE_CONTROL_CODE_UNEXPORT_LUN,
                                                &lun_object_id, sizeof(fbe_object_id_t),
                                                bvd_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if((status != FBE_STATUS_OK && status != FBE_STATUS_NO_DEVICE) || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed packet error:%d, packet qualifier:%d\n", 
                      __FUNCTION__,status, status_info.packet_qualifier);
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "payload error:%d, payload qualifier:%d\n", 
                      status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

