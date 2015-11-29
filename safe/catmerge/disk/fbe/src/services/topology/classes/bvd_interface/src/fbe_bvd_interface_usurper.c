/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_bvd_interface_usurper.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the bvd_interface object code for the usurper.
 * 
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_object.h"
#include "fbe_bvd_interface_private.h"
#include "fbe/fbe_bvd_interface.h"
#include "fbe/fbe_payload_control_operation.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_sep_shim.h"
#include "fbe/fbe_api_common.h"
#include "fbe_transport_memory.h"
#include "VolumeAttributes.h"


/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/
static fbe_status_t bvd_interface_register_lun(fbe_bvd_interface_t *bvd_object_p, fbe_packet_t * packet_p);
static fbe_status_t bvd_interface_create_edge(fbe_bvd_interface_t *bvd_object_p, fbe_packet_t * packet_p);
static fbe_status_t bvd_interface_get_attr(fbe_bvd_interface_t *bvd_object_p, fbe_packet_t *packet_p);

static fbe_status_t fbe_bvd_interface_allocate_edge_ptr(fbe_bvd_interface_t *bvd_object_p, fbe_block_edge_t **edge_p);
static fbe_status_t fbe_bvd_interface_get_os_dev_info(fbe_bvd_interface_t *bvd_object_p, fbe_edge_index_t client_index, os_device_info_t **os_dev_info);
static fbe_status_t fbe_bvd_interface_send_attach_edge(fbe_bvd_interface_t *bvd_object_p, fbe_packet_t * packet_p, fbe_block_transport_control_attach_edge_t * attach_edge);
static fbe_status_t fbe_bvd_interface_usurper_enable_performace_statistics(fbe_bvd_interface_t* bvd_object_p,
                                                        fbe_packet_t* packet_p);
static fbe_status_t fbe_bvd_interface_usurper_disable_performace_statistics(fbe_bvd_interface_t* bvd_object_p,
                                                        fbe_packet_t* packet_p);
static fbe_status_t fbe_bvd_interface_usurper_get_performace_statistics(fbe_bvd_interface_t* bvd_object_p,
                                                        fbe_packet_t* packet_p);

static fbe_status_t bvd_interface_get_lun_attributes(fbe_packet_t * packet_p, os_device_info_t * os_device_info);
static fbe_status_t fbe_bvd_interface_usurper_clear_performace_statistics(fbe_bvd_interface_t* bvd_object_p, fbe_packet_t* packet_p);
static fbe_status_t fbe_bvd_interface_usurper_set_rq_method(fbe_bvd_interface_t* bvd_object_p, fbe_packet_t* packet_p);
static fbe_status_t fbe_bvd_interface_usurper_set_alert_time(fbe_bvd_interface_t* bvd_object_p, fbe_packet_t* packet_p);
static fbe_status_t fbe_bvd_interface_usurper_shutdown(fbe_bvd_interface_t* bvd_object_p, fbe_packet_t* packet_p);
void fbe_bvd_interface_usurper_enable_pp_group_priority(void);
void fbe_bvd_interface_usurper_disable_pp_group_priority(void);
static fbe_status_t fbe_bvd_interface_usurper_unexport_lun(fbe_bvd_interface_t* bvd_object_p, fbe_packet_t* packet_p);

extern fbe_status_t fbe_sep_init_get_board_object_id(fbe_object_id_t *board_id);

/*****************************************
 * FUNCTIONS
 *****************************************/
fbe_status_t fbe_bvd_interface_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet_p)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_bvd_interface_t *                   bvd_object_p = NULL;
    fbe_payload_ex_t *                     payload_p = NULL;
    fbe_payload_control_operation_t *       control_p = NULL;
    fbe_payload_control_operation_opcode_t  opcode = 0;

    bvd_object_p = (fbe_bvd_interface_t *)fbe_base_handle_to_pointer(object_handle);
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_opcode(control_p, &opcode);

    switch (opcode){
    case FBE_BVD_INTERFACE_CONTROL_CODE_REGISTER_LUN:
        status = bvd_interface_register_lun(bvd_object_p, packet_p);
        break;
    case FBE_BVD_INTERFACE_CONTROL_CODE_GET_ATTR:
        status = bvd_interface_get_attr(bvd_object_p, packet_p);
        break;
    case FBE_BVD_INTERFACE_CONTROL_CODE_ENABLE_PERFORMANCE_STATS:
        status = fbe_bvd_interface_usurper_enable_performace_statistics(bvd_object_p, packet_p);
        break;
    case FBE_BVD_INTERFACE_CONTROL_CODE_DISABLE_PERFORMANCE_STATS:
        status = fbe_bvd_interface_usurper_disable_performace_statistics(bvd_object_p, packet_p);
        break;
    case FBE_BVD_INTERFACE_CONTROL_CODE_GET_PERFORMANCE_STATS:
        status = fbe_bvd_interface_usurper_get_performace_statistics(bvd_object_p, packet_p);
        break;
	case FBE_BVD_INTERFACE_CONTROL_CODE_CLEAR_PERFORMANCE_STATS:
		status = fbe_bvd_interface_usurper_clear_performace_statistics(bvd_object_p, packet_p);
		break;

	case FBE_BVD_INTERFACE_CONTROL_CODE_ENABLE_ASYNC_IO:
		fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
								FBE_TRACE_LEVEL_INFO,
								FBE_TRACE_MESSAGE_ID_INFO,
								"fbe_sep_shim_set_async_io(FBE_TRUE)\n");

		fbe_sep_shim_set_async_io(FBE_TRUE);
		fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
		status = fbe_transport_complete_packet(packet_p);
		break;

	case FBE_BVD_INTERFACE_CONTROL_CODE_DISABLE_ASYNC_IO:
		fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
								FBE_TRACE_LEVEL_INFO,
								FBE_TRACE_MESSAGE_ID_INFO,
								"fbe_sep_shim_set_async_io(FBE_FALSE)\n");

		fbe_sep_shim_set_async_io(FBE_FALSE);
		fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
		status = fbe_transport_complete_packet(packet_p);
		break;
	case FBE_BVD_INTERFACE_CONTROL_CODE_ENABLE_ASYNC_IO_COMPL:
		fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
								FBE_TRACE_LEVEL_INFO,
								FBE_TRACE_MESSAGE_ID_INFO,
								"fbe_sep_shim_set_async_io_compl(FBE_TRUE)\n");

		fbe_sep_shim_set_async_io_compl(FBE_TRUE);
		fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
		status = fbe_transport_complete_packet(packet_p);
		break;

	case FBE_BVD_INTERFACE_CONTROL_CODE_DISABLE_ASYNC_IO_COMPL:
		fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
								FBE_TRACE_LEVEL_INFO,
								FBE_TRACE_MESSAGE_ID_INFO,
								"fbe_sep_shim_set_async_io_compl(FBE_FALSE)\n");

		fbe_sep_shim_set_async_io_compl(FBE_FALSE);
		fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
		status = fbe_transport_complete_packet(packet_p);
		break;

    case FBE_BVD_INTERFACE_CONTROL_CODE_SET_RQ_METHOD:
        status = fbe_bvd_interface_usurper_set_rq_method(bvd_object_p, packet_p);
        break;

    case FBE_BVD_INTERFACE_CONTROL_CODE_SET_ALERT_TIME:
        status = fbe_bvd_interface_usurper_set_alert_time(bvd_object_p, packet_p);
        break;

	case FBE_BVD_INTERFACE_CONTROL_CODE_ENABLE_GROUP_PRIORITY:
		fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
								FBE_TRACE_LEVEL_INFO,
								FBE_TRACE_MESSAGE_ID_INFO,
								"fbe_transport_enable_group_priority()\n");
		fbe_transport_enable_group_priority();
		fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
		status = fbe_transport_complete_packet(packet_p);
		break;

    case FBE_BVD_INTERFACE_CONTROL_CODE_DISABLE_GROUP_PRIORITY:
		fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
								FBE_TRACE_LEVEL_INFO,
								FBE_TRACE_MESSAGE_ID_INFO,
								"fbe_transport_disable_group_priority()\n");
		fbe_transport_disable_group_priority();
		fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
		status = fbe_transport_complete_packet(packet_p);
		break;

	case FBE_BVD_INTERFACE_CONTROL_CODE_ENABLE_PP_GROUP_PRIORITY:
		fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
								FBE_TRACE_LEVEL_INFO,
								FBE_TRACE_MESSAGE_ID_INFO,
								"fbe_transport_enable_pp_group_priority()\n");
		fbe_bvd_interface_usurper_enable_pp_group_priority();
		fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
		status = fbe_transport_complete_packet(packet_p);
		break;

    case FBE_BVD_INTERFACE_CONTROL_CODE_DISABLE_PP_GROUP_PRIORITY:
		fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
								FBE_TRACE_LEVEL_INFO,
								FBE_TRACE_MESSAGE_ID_INFO,
								"fbe_transport_disable_pp_group_priority()\n");
		fbe_bvd_interface_usurper_disable_pp_group_priority();
		fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
		status = fbe_transport_complete_packet(packet_p);
		break;

    case FBE_BVD_INTERFACE_CONTROL_CODE_SHUTDOWN:
        status = fbe_bvd_interface_usurper_shutdown(bvd_object_p, packet_p);
        break;

    case FBE_BVD_INTERFACE_CONTROL_CODE_UNEXPORT_LUN:
        status = fbe_bvd_interface_usurper_unexport_lun(bvd_object_p, packet_p);
        break;

    default:
        /* Allow the  object to handle all other ioctls.*/
        status = fbe_base_config_control_entry(object_handle, packet_p);

		if((FBE_STATUS_TRAVERSE == status) && (FBE_CLASS_ID_BVD_INTERFACE == fbe_base_object_get_class_id(fbe_base_pointer_to_handle((fbe_base_t *)bvd_object_p)))){
                fbe_base_object_trace((fbe_base_object_t*) bvd_object_p,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s Can't Traverse for Most derived FBE_CLASS_ID_BVD_INTERFACE class. Opcode: 0x%X\n",
                                      __FUNCTION__, opcode);

                fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
                status = fbe_transport_complete_packet(packet_p);
            }

        break;
    }
    return status;
}

static fbe_status_t bvd_interface_register_lun(fbe_bvd_interface_t *bvd_object_p, fbe_packet_t * packet_p)
{
    fbe_bvd_interface_register_lun_t *              register_lun_p = NULL;    
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t *                             payload_p = NULL;
    fbe_payload_control_operation_t *               control_operation_p = NULL;  
    fbe_payload_control_buffer_length_t             length = 0;
    os_device_info_t *                              os_device_info_p = NULL;
    fbe_object_id_t                                 my_object_id = FBE_OBJECT_ID_INVALID;
    fbe_package_id_t                                my_package_id;

    fbe_block_edge_t *                              edge_p = NULL;
    fbe_block_transport_control_attach_edge_t       attach_edge;

    
    fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);  

    fbe_payload_control_get_buffer(control_operation_p, &register_lun_p);
    if (register_lun_p == NULL){
        fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &length);
    if (length != sizeof(fbe_bvd_interface_register_lun_t)){
        fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s buffer lenght don't match with fbe_bvd_interface_register_lun_t\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now attach the edge. */
    fbe_base_object_get_object_id((fbe_base_object_t *)bvd_object_p, &my_object_id);

    /*we need to get the edge by actually allocating a structure that contains the edge*/
    status = fbe_bvd_interface_allocate_edge_ptr (bvd_object_p, &edge_p);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed to allocate edge ptr\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
	}
    
    fbe_block_transport_set_transport_id(edge_p);
    fbe_block_transport_set_client_id(edge_p, my_object_id);
    fbe_block_transport_set_server_id(edge_p, register_lun_p->server_id);
    fbe_block_transport_set_client_index(edge_p, register_lun_p->server_id); /* use the lun object id as the client index */

    /* Initialize capacity and offset of the edge to zero, LUN will set these
     * values based on capacity of the LUN.
     */
    fbe_block_transport_edge_set_capacity(edge_p, register_lun_p->capacity);
    fbe_block_transport_edge_set_offset(edge_p, 0);

    attach_edge.block_edge = edge_p;

    fbe_get_package_id(&my_package_id);
    fbe_block_transport_edge_set_client_package_id(edge_p, my_package_id);

    status = fbe_bvd_interface_send_attach_edge((fbe_bvd_interface_t *)bvd_object_p, packet_p, &attach_edge) ;
    if(status != FBE_STATUS_OK){
        fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_bvd_interface_send_attach_edge failed\n", __FUNCTION__);
        fbe_bvd_interface_destroy_edge_ptr(bvd_object_p, attach_edge.block_edge);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Connected edge from BVD to LUN#:%d\n", __FUNCTION__, register_lun_p->lun_number);

    os_device_info_p = bvd_interface_block_edge_to_os_device(edge_p);
    os_device_info_p->lun_number = register_lun_p->lun_number;
    os_device_info_p->bvd_object_id = my_object_id;/*we use this when IO come so we push the packet via topology to the bvd object*/
	os_device_info_p->current_attributes  = os_device_info_p->previous_attributes = 0;
    os_device_info_p->export_lun = register_lun_p->export_lun_b;
    os_device_info_p->export_device = NULL;
   
    /*and give it to the other layer, which would export it as an OS device*/
    status = fbe_bvd_interface_add_os_device(bvd_object_p, os_device_info_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s: Failed to add to OS device.Status=0x%x\n", __FUNCTION__, status);
        /* clean up before exit */
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);

        return FBE_STATUS_GENERIC_FAILURE;
    }

#ifdef C4_INTEGRATED
    if (register_lun_p->export_lun_b)
    {
        status = fbe_bvd_interface_export_lun(bvd_object_p, os_device_info_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)bvd_object_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s: Failed to export lun. Status=0x%x\n", __FUNCTION__, status);
            /* clean up before exit */
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet_p);

            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
#endif

    /* Everything completed. Return OK status. */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}

static fbe_status_t bvd_interface_create_edge(fbe_bvd_interface_t *bvd_object_p, fbe_packet_t * packet_p)
{

    fbe_block_transport_control_create_edge_t *     create_edge_p = NULL;    
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t *                             payload_p = NULL;
    fbe_payload_control_operation_t *               control_operation_p = NULL;  
    fbe_block_edge_t *                              edge_p = NULL;
    fbe_object_id_t                                 my_object_id = FBE_OBJECT_ID_INVALID;
    fbe_block_transport_control_attach_edge_t       attach_edge;
    fbe_package_id_t                                my_package_id;


    fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);  

    fbe_payload_control_get_buffer(control_operation_p, &create_edge_p);
    if (create_edge_p == NULL){
        fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    /* Now attach the edge. */
    fbe_base_object_get_object_id((fbe_base_object_t *)bvd_object_p, &my_object_id);

    /*we need to get the edge by actually allocating a structure that contains the edge*/
    status = fbe_bvd_interface_allocate_edge_ptr (bvd_object_p, &edge_p);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed to allocate edge ptr\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
	}
    
    fbe_block_transport_set_transport_id(edge_p);
    fbe_block_transport_set_server_id(edge_p, create_edge_p->server_id);
    fbe_block_transport_set_client_id(edge_p, my_object_id);
    fbe_block_transport_set_client_index(edge_p, create_edge_p->client_index);

    /* Initialize capacity and offshore of the edge to zero, LUN will set these
     * values based on capacity of the LUN.
     */
    fbe_block_transport_edge_set_capacity(edge_p, create_edge_p->capacity);
    fbe_block_transport_edge_set_offset(edge_p, 0);

    attach_edge.block_edge = edge_p;

    fbe_get_package_id(&my_package_id);
    fbe_block_transport_edge_set_client_package_id(edge_p, my_package_id);

    status = fbe_bvd_interface_send_attach_edge((fbe_bvd_interface_t *)bvd_object_p, packet_p, &attach_edge) ;
    if(status != FBE_STATUS_OK){
        fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_bvd_interface_send_attach_edge failed\n", __FUNCTION__);
        fbe_bvd_interface_destroy_edge_ptr(bvd_object_p, attach_edge.block_edge);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_bvd_interface_allocate_edge_ptr(fbe_bvd_interface_t *bvd_object_p, fbe_block_edge_t **edge_p)
{
    os_device_info_t *      os_device_info = NULL;

    /*we allocate a new structure that would have the information about the edge and other things*/

    os_device_info = (os_device_info_t *) fbe_memory_ex_allocate(sizeof(os_device_info_t));
	if (os_device_info == NULL) {
		fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s, can't allocate memory or LU\n", __FUNCTION__);

		return FBE_STATUS_GENERIC_FAILURE;

	}

    fbe_zero_memory(os_device_info, sizeof(os_device_info_t));

    /*add this edge to the list of edges we have*/
    fbe_queue_element_init(&os_device_info->queue_element);
	fbe_queue_element_init(&os_device_info->cache_event_queue_element);
	os_device_info->cache_event_signaled = FBE_FALSE;

    fbe_base_object_lock((fbe_base_object_t*)bvd_object_p);
    fbe_queue_push(&bvd_object_p->server_queue_head, &os_device_info->queue_element);
    fbe_base_object_unlock((fbe_base_object_t*)bvd_object_p);

    /*return the edge to the caller*/
    *edge_p = &os_device_info->block_edge;

    return FBE_STATUS_OK;
}

/* !!!!!!!!!! use this function with caution, the poiner you get might go away if the LUN is removed from a different thread !!!!!!!!*/
static fbe_status_t fbe_bvd_interface_get_os_dev_info(fbe_bvd_interface_t *bvd_object_p, fbe_edge_index_t requested_index, os_device_info_t **os_dev_info_out)
{
    fbe_queue_element_t     *current_queue_element = NULL, *first_queue_element = NULL, *next_element = NULL;
    os_device_info_t *      os_device_info = NULL;
    fbe_edge_index_t        current_index = 0;
    
    /*go over the list and look for a match beween the clinet indexes*/
    fbe_base_object_lock((fbe_base_object_t*)bvd_object_p);
    first_queue_element = fbe_queue_front(&bvd_object_p->server_queue_head);/*the start is also the end*/
    current_queue_element = first_queue_element;

	/*any chance we are empty ?*/
	if (first_queue_element == NULL) {
		fbe_base_object_unlock((fbe_base_object_t*)bvd_object_p);
		fbe_base_object_trace((fbe_base_object_t *)bvd_object_p,
							  FBE_TRACE_LEVEL_ERROR,
							  FBE_TRACE_MESSAGE_ID_INFO,
							  "%s: No LUs attached yet\n", __FUNCTION__);

		*os_dev_info_out = NULL;
		return FBE_STATUS_GENERIC_FAILURE;
	}

    do{
        next_element = current_queue_element->next;
        os_device_info = bvd_interface_queue_element_to_os_device(current_queue_element);
        fbe_block_transport_get_client_index(&os_device_info->block_edge, &current_index);

        if (current_index == requested_index) {
            fbe_base_object_unlock((fbe_base_object_t*)bvd_object_p);
            *os_dev_info_out = os_device_info;
            return FBE_STATUS_OK;
        }

        current_queue_element = next_element;/*go to next one*/
    }while (first_queue_element->next != current_queue_element->next);

    fbe_base_object_unlock((fbe_base_object_t*)bvd_object_p);

    /*if we got here we did not find the index and it's bad*/
    return FBE_STATUS_GENERIC_FAILURE;
}

static fbe_status_t 
fbe_bvd_interface_send_attach_edge(fbe_bvd_interface_t *bvp_p, fbe_packet_t * packet_p, fbe_block_transport_control_attach_edge_t * attach_edge) 
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t *             payload_p = NULL;
    fbe_payload_control_operation_t *control_p = NULL;


    fbe_base_object_trace(  (fbe_base_object_t *)bvp_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    /* Allocate control operation. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_allocate_control_operation(payload_p);

    fbe_payload_control_build_operation(control_p,
                                        FBE_BLOCK_TRANSPORT_CONTROL_CODE_ATTACH_EDGE,
                                        attach_edge,
                                        sizeof(fbe_block_transport_control_attach_edge_t));

    /* Set our completion function. */
     fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    /* We are sending control packet, so we have to form address manually. */
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              attach_edge->block_edge->base_edge.server_id);

    /* Control packets should be sent via service manager. */
    status = fbe_service_manager_send_control_packet(packet_p);

    fbe_transport_wait_completion(packet_p);

    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(  (fbe_base_object_t *)bvp_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed status %X\n", 
                                __FUNCTION__, status);

        fbe_payload_ex_release_control_operation(payload_p, control_p);
        return status;
    }

    /* Free the resources we allocated previously.*/
    fbe_payload_ex_release_control_operation(payload_p, control_p);

    return FBE_STATUS_OK;
}
/****************************************************************
 * fbe_bvd_interface_destroy_edge_ptr()
 ****************************************************************
 * @brief
 * This function is used to remove an edge from the BVD object's
 * server list.
 *
 * @param bvd_object_p  - pointer to BVD object (IN)
 * @param edge_p        - pointer to edge to remove (IN)
 *
 * @return status - The status of the operation.
 *
 * @author
 *  03/2010 - Created. rundbs
 ****************************************************************/
fbe_status_t fbe_bvd_interface_destroy_edge_ptr(fbe_bvd_interface_t *bvd_object_p, fbe_block_edge_t *edge_p)
{
    fbe_queue_element_t*                queue_element_p         = NULL;
    os_device_info_t*                   os_dev_info_p           = NULL;
    fbe_block_edge_t*                   bvd_server_list_edge_p  = NULL;
    fbe_bool_t                          found                   = FBE_FALSE;

    fbe_base_object_lock((fbe_base_object_t*)bvd_object_p);

    /* Look for edge on BVD server list */
    queue_element_p = fbe_queue_front(&bvd_object_p->server_queue_head);

    while (queue_element_p) {

        os_dev_info_p = bvd_interface_queue_element_to_os_device(queue_element_p);
        bvd_server_list_edge_p = &os_dev_info_p->block_edge;

        if(bvd_server_list_edge_p != edge_p){
            queue_element_p = fbe_queue_next(&bvd_object_p->server_queue_head, queue_element_p);
            continue;
        }

        /* Found the edge; remove it from the list and free its memory */
        found = FBE_TRUE;
        fbe_queue_remove(queue_element_p);

		/*any chance cache still has an IRP waiting for our notification ?
		This is rare and should happen only if we force delete via FBE CLI and cache is not aware of that.*/
		if (os_dev_info_p->state_change_notify_ptr != NULL) {

			fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
									FBE_TRACE_LEVEL_WARNING,
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"%s: Edge to LUN is removed while cache still has IRP:0x%p\n", 
									__FUNCTION__, os_dev_info_p->state_change_notify_ptr);
			
		}
        /* remove cache_event_queue_element before freeing os_dev_info_p*/
        fbe_spinlock_lock(&bvd_object_p->cache_notification_queue_lock);
        fbe_queue_remove(&os_dev_info_p->cache_event_queue_element);
        fbe_spinlock_unlock(&bvd_object_p->cache_notification_queue_lock);
        
#ifdef C4_INTEGRATED
        if (os_dev_info_p->export_lun) {
            fbe_bvd_interface_unexport_lun(bvd_object_p, os_dev_info_p);
        }
#endif

        fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: free os_dev_info of lun num 0x%x\n", 
                                __FUNCTION__, os_dev_info_p->lun_number);
		
        /*and free the memory for it*/
        fbe_memory_ex_release(os_dev_info_p);
        break;
    }

    fbe_base_object_unlock((fbe_base_object_t*)bvd_object_p);

    if (FBE_IS_FALSE(found))
    {
        fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s: edge 0x%p not found on BVD server list\n", 
                                __FUNCTION__, edge_p);
    }

    return FBE_STATUS_OK;
}
/**********************************************************
 * end fbe_bvd_interface_destroy_edge_ptr()
 **********************************************************/

/*!**************************************************************
 * bvd_interface_get_attr()
 ****************************************************************
 * @brief
 * This function is used to get the downstream attribute field from
 * the BVD object.  This is used for testing purposes. 
 *
 * @param bvd_object_p  - pointer to BVD object (IN)
 * @param packet_p      - pointer to incoming "get attr" packet (IN)
 *
 * @return status - The status of the operation.
 *
 * @author
 *  03/2010 - Created. rundbs
 ****************************************************************/
static fbe_status_t bvd_interface_get_attr(fbe_bvd_interface_t *bvd_object_p, fbe_packet_t *packet_p)
{

    fbe_bvd_interface_get_downstream_attr_t*    get_attr_p          = NULL;    
    fbe_status_t                                status              = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t*                          payload_p           = NULL;
    fbe_payload_control_operation_t*            control_operation_p = NULL;  
    fbe_payload_control_buffer_length_t         length              = 0;
    os_device_info_t *                          os_device_info = NULL;


    fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    /* Get the payload from the incoming packet */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);  

    fbe_payload_control_get_buffer(control_operation_p, &get_attr_p);
    if (get_attr_p == NULL){
        fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &length);
    if (length != sizeof(fbe_bvd_interface_get_downstream_attr_t)){
        fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s buffer length does not match fbe_bvd_interface_get_downstream_attr_t\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the os_dev_info of this LU*/
    status  = fbe_bvd_interface_get_os_dev_info(bvd_object_p, get_attr_p->lun_object_id, &os_device_info);
    if (status != FBE_STATUS_OK){
        /* A test should not see a ready LU object, and query BVD to check for the connection */
        fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s retryable get os dev info failure for LU: %d\n", __FUNCTION__, get_attr_p->lun_object_id);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    get_attr_p->attr = os_device_info->current_attributes;
	fbe_copy_memory(&get_attr_p->block_edge, &os_device_info->block_edge, sizeof(fbe_block_edge_t));
	get_attr_p->opaque_edge_ptr = (fbe_u64_t)&os_device_info->block_edge;/*used in rdgen to hook up directly to the edge*/

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/**********************************************************
 * end bvd_interface_get_attr()
 **********************************************************/

/*!**************************************************************
 * fbe_bvd_interface_usurper_enable_performace_statistics()
 ****************************************************************
 * @brief
 *  This function is used to enable performance statistics to 
 *  modify SP specific counters incremented by BVD object
 *  
 * @param bvd_object_p    - BVD object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  12/09/2010 - Created. Swati Fursule
 *
 ****************************************************************/
static fbe_status_t fbe_bvd_interface_usurper_enable_performace_statistics(fbe_bvd_interface_t* bvd_object_p,
                                                        fbe_packet_t* packet_p)
{
    fbe_status_t status = FBE_STATUS_OK; 

     fbe_sep_shim_perf_stat_enable();
    fbe_transport_set_status(packet_p, status, 0);
    status = fbe_transport_complete_packet(packet_p);
    return status;
}
/******************************
 * end fbe_bvd_interface_usurper_enable_performace_statistics()
 ******************************/


/*!**************************************************************
 * fbe_bvd_interface_usurper_disable_performace_statistics()
 ****************************************************************
 * @brief
 *  This function is used to disable performance statistics to not
 *  modify any SP specific counters.
 *  
 * @param bvd_object_p    - BVD object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  12/09/2010 - Created. Swati Fursule
 *
 ****************************************************************/
static fbe_status_t fbe_bvd_interface_usurper_disable_performace_statistics(fbe_bvd_interface_t* bvd_object_p,
                                                        fbe_packet_t* packet_p)
{
    fbe_status_t status = FBE_STATUS_OK; 
    /* modify b_perf_stats_enabled
     */
    fbe_sep_shim_perf_stat_disable();
    fbe_transport_set_status(packet_p, status, 0);
    status = fbe_transport_complete_packet(packet_p);
    return status;
}
/******************************
 * end fbe_bvd_interface_usurper_disable_performace_statistics()
 ******************************/

/*!**************************************************************
 * fbe_bvd_interface_usurper_get_performace_statistics()
 ****************************************************************
 * @brief
 *  This function is used to get performance statistics , SP specific 
 *  
 *  
 * @param bvd_object_p    - BVD object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  12/09/2010 - Created. Swati Fursule
 *
 ****************************************************************/
static fbe_status_t fbe_bvd_interface_usurper_get_performace_statistics(fbe_bvd_interface_t* bvd_object_p,
                                                        fbe_packet_t* packet_p)
{
    fbe_sep_shim_get_perf_stat_t*            	req_buffer_p = NULL;
    fbe_status_t                                status              = FBE_STATUS_OK;
    fbe_payload_ex_t*                          payload_p           = NULL;
    fbe_payload_control_operation_t*            control_operation_p = NULL;  
    fbe_payload_control_buffer_length_t         length              = 0;
    
    fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    /* Get the payload from the incoming packet */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);  

    fbe_payload_control_get_buffer(control_operation_p, &req_buffer_p);
    if (req_buffer_p == NULL){
        fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &length);
    if (length != sizeof(fbe_sep_shim_get_perf_stat_t)){
        fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s buffer lenght don't match with fbe_sp_performance_statistics_t\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Copy the the SP performace statistics information into the requester's buffer.
     */
	fbe_sep_shim_get_perf_stat(req_buffer_p);
	
	fbe_transport_run_queue_get_stats(req_buffer_p);

    fbe_transport_set_status(packet_p, status, 0);
    status = fbe_transport_complete_packet(packet_p);
    return status;
}
/******************************
 * end fbe_bvd_interface_usurper_get_performace_statistics()
 ******************************/

static fbe_status_t bvd_interface_get_lun_attributes(fbe_packet_t * packet,
													 os_device_info_t * os_device_info)
{
	fbe_block_edge_t *								edge = &os_device_info->block_edge;
    fbe_payload_control_operation_t *           	control_operation = NULL;
	fbe_payload_ex_t *                             	payload = NULL;
    fbe_payload_control_status_t					control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
    fbe_status_t									status = FBE_STATUS_GENERIC_FAILURE;
	fbe_lun_get_attributes_t						get_attr;

	payload = fbe_transport_get_payload_ex(packet);
	control_operation = fbe_payload_ex_allocate_control_operation(payload); 
	if(control_operation == NULL) {    
		return FBE_STATUS_GENERIC_FAILURE;    
	}

    fbe_payload_control_build_operation(control_operation,  
										FBE_LUN_CONTROL_CODE_GET_ATTRIBUTES,  
										&get_attr, 
										sizeof(fbe_lun_get_attributes_t)); 

	fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    
	status  = fbe_block_transport_send_control_packet(edge, packet);  
	if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING)) {    
        fbe_payload_ex_release_control_operation(payload, control_operation);
		return FBE_STATUS_GENERIC_FAILURE;    
	}

	fbe_transport_wait_completion(packet);

	status = fbe_transport_get_status_code(packet);
	fbe_payload_control_get_status(control_operation, &control_status);
	fbe_payload_ex_release_control_operation(payload, control_operation);

	if (status == FBE_STATUS_OK && control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) {
		os_device_info->current_attributes |= get_attr.lun_attr;
		return FBE_STATUS_OK;
	}else{
		return FBE_STATUS_GENERIC_FAILURE;
	}

}

/*!**************************************************************
 * fbe_bvd_interface_usurper_clear_performace_statistics()
 ****************************************************************
 * @brief
 *  This function is used to clear the perf stats
 *  
 * @param bvd_object_p    - BVD object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  12/09/2010 - Created. Swati Fursule
 *
 ****************************************************************/
static fbe_status_t fbe_bvd_interface_usurper_clear_performace_statistics(fbe_bvd_interface_t* bvd_object_p,
                                                        fbe_packet_t* packet_p)
{
    fbe_status_t status = FBE_STATUS_OK; 
    /* modify b_perf_stats_enabled
     */
	fbe_sep_shim_perf_stat_clear();

	fbe_transport_run_queue_clear_stats();

    fbe_transport_set_status(packet_p, status, 0);
    status = fbe_transport_complete_packet(packet_p);
    return status;
}

static fbe_status_t fbe_bvd_interface_usurper_set_rq_method(fbe_bvd_interface_t* bvd_object_p,
                                                        fbe_packet_t* packet_p)
{
    fbe_sep_shim_set_rq_method_t*            	req_buffer_p = NULL;
    fbe_status_t                                status              = FBE_STATUS_OK;
    fbe_payload_ex_t*                          payload_p           = NULL;
    fbe_payload_control_operation_t*            control_operation_p = NULL;  
    fbe_payload_control_buffer_length_t         length              = 0;
    
    fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    /* Get the payload from the incoming packet */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);  

    fbe_payload_control_get_buffer(control_operation_p, &req_buffer_p);
    if (req_buffer_p == NULL){
        fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &length);
    if (length != sizeof(fbe_sep_shim_set_rq_method_t)){
        fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s buffer lenght don't match with fbe_sp_performance_statistics_t\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	fbe_sep_shim_set_rq_method(req_buffer_p->rq_method);

    fbe_transport_set_status(packet_p, status, 0);
    status = fbe_transport_complete_packet(packet_p);
    return status;
}

static fbe_status_t fbe_bvd_interface_usurper_set_alert_time(fbe_bvd_interface_t* bvd_object_p,
                                                        fbe_packet_t* packet_p)
{
    fbe_sep_shim_set_alert_time_t*            	req_buffer_p = NULL;
    fbe_status_t                                status              = FBE_STATUS_OK;
    fbe_payload_ex_t*                          payload_p           = NULL;
    fbe_payload_control_operation_t*            control_operation_p = NULL;  
    fbe_payload_control_buffer_length_t         length              = 0;
    
    fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    /* Get the payload from the incoming packet */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);  

    fbe_payload_control_get_buffer(control_operation_p, &req_buffer_p);
    if (req_buffer_p == NULL){
        fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &length);
    if (length != sizeof(fbe_sep_shim_set_alert_time_t)){
        fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s buffer lenght don't match with fbe_sp_performance_statistics_t\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	fbe_sep_shim_set_alert_time(req_buffer_p->alert_time);

    fbe_transport_set_status(packet_p, status, 0);
    status = fbe_transport_complete_packet(packet_p);
    return status;
}

void fbe_bvd_interface_usurper_enable_pp_group_priority(void)
{    
    fbe_status_t                                            status;
    fbe_packet_t *                                          packet_p = NULL;
    fbe_payload_ex_t *                                      payload_p = NULL;
    fbe_payload_control_operation_t *                       control_p = NULL;
    fbe_physical_board_group_policy_info_t                  group_policy;
    fbe_object_id_t                                         object_id;
    fbe_transport_external_run_queue_t                      funciton_p = NULL;

    /* Allocate packet.
     */
    packet_p = fbe_transport_allocate_packet();
    if (packet_p == NULL) 
    {
        return;
    }

    fbe_transport_initialize_packet(packet_p);
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_allocate_control_operation(payload_p);
    fbe_sep_init_get_board_object_id(&object_id);

    fbe_transport_run_queue_get_external_handler(&funciton_p);
    group_policy.fbe_transport_external_run_queue_current = funciton_p;

    fbe_transport_fast_queue_get_external_handler(&funciton_p);
    group_policy.fbe_transport_external_fast_queue_current = funciton_p;

    fbe_payload_control_build_operation(control_p,
                                        FBE_BASE_BOARD_CONTROL_CODE_ENABLE_PP_GROUP_PRIORITY,
                                        &group_policy,
                                        sizeof(fbe_physical_board_group_policy_info_t));
    /* Set packet address.
     */
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_PHYSICAL,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id);
    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_NO_ATTRIB);
    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    fbe_service_manager_send_control_packet(packet_p);

    /* Wait for completion.
     * The packet is always completed so the status above need not be checked.
     */
    fbe_transport_wait_completion(packet_p);

    /* Save status before we release packet.
     */
    status = fbe_transport_get_status_code(packet_p);

    fbe_transport_release_packet(packet_p);
}

void fbe_bvd_interface_usurper_disable_pp_group_priority(void)
{
    fbe_status_t                                            status;
    fbe_packet_t *                                          packet_p = NULL;
    fbe_payload_ex_t *                                      payload_p = NULL;
    fbe_payload_control_operation_t *                       control_p = NULL;
    fbe_physical_board_group_policy_info_t                  group_policy;
    fbe_object_id_t                                         object_id;

    /* Allocate packet.
     */
    packet_p = fbe_transport_allocate_packet();
    if (packet_p == NULL) 
    {
        return;
    }

    fbe_transport_initialize_packet(packet_p);
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_allocate_control_operation(payload_p);
    fbe_sep_init_get_board_object_id(&object_id);
    group_policy.fbe_transport_external_run_queue_current = NULL;

    fbe_payload_control_build_operation(control_p,
                                        FBE_BASE_BOARD_CONTROL_CODE_DISABLE_PP_GROUP_PRIORITY,
                                        &group_policy,
                                        sizeof(fbe_physical_board_group_policy_info_t));
    /* Set packet address.
     */
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_PHYSICAL,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id);
    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_NO_ATTRIB);
    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    fbe_service_manager_send_control_packet(packet_p);

    /* Wait for completion.
     * The packet is always completed so the status above need not be checked.
     */
    fbe_transport_wait_completion(packet_p);

    /* Save status before we release packet.
     */
    status = fbe_transport_get_status_code(packet_p);

    fbe_transport_release_packet(packet_p);
}
static fbe_status_t fbe_bvd_interface_usurper_shutdown(fbe_bvd_interface_t* bvd_object_p,
                                                       fbe_packet_t* packet_p)
{
    fbe_status_t                                status              = FBE_STATUS_OK;
    fbe_payload_ex_t*                           payload_p           = NULL;
    fbe_payload_control_operation_t*            control_operation_p = NULL;
    
    fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    /* Get the payload from the incoming packet */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);  
    
    status = fbe_sep_shim_shutdown_bvd();
    fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s status 0x%x\n", __FUNCTION__, status);
    fbe_transport_set_status(packet_p, status, 0);
    status = fbe_transport_complete_packet(packet_p);
    return status;
}

/*!**************************************************************
 * fbe_bvd_interface_usurper_unexport_lun()
 ****************************************************************
 * @brief
 *  This function is used to unexport blockshim device 
 *  
 *  
 * @param bvd_object_p    - BVD object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  11/05/2014 - Created. Jibing Dong
 *
 ****************************************************************/
static fbe_status_t fbe_bvd_interface_usurper_unexport_lun(fbe_bvd_interface_t* bvd_object_p,
                                                           fbe_packet_t* packet_p)
{
    fbe_status_t                                status              = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t*                           payload_p           = NULL;
    fbe_payload_control_operation_t*            control_operation_p = NULL;  
    fbe_object_id_t*                            lun_object_id_p     = NULL;
    fbe_payload_control_buffer_length_t         length              = 0;
    os_device_info_t*                           os_device_info_p    = NULL;

    fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    /* Get the payload from the incoming packet */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);  

    fbe_payload_control_get_buffer(control_operation_p, &lun_object_id_p);
    if (*lun_object_id_p == FBE_OBJECT_ID_INVALID)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &length);
    if (length != sizeof(fbe_object_id_t)){
        fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s buffer lenght don't match with fbe_object_id_t\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the os_dev_info of this LU*/
    status  = fbe_bvd_interface_get_os_dev_info(bvd_object_p, *lun_object_id_p, &os_device_info_p);
    if (status != FBE_STATUS_OK){
        /* A test should not see a ready LU object, and query BVD to check for the connection */
        fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s retryable get os dev info failure for LU: %d\n", __FUNCTION__, *lun_object_id_p);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

#ifdef C4_INTEGRATED
    if (os_device_info_p->export_lun)
    {
        status = fbe_bvd_interface_unexport_lun(bvd_object_p, os_device_info_p);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s failed to unexport blockshim device from LU: %d\n", __FUNCTION__, *lun_object_id_p);

            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        os_device_info_p->export_lun = FBE_FALSE;
        os_device_info_p->export_device = NULL;
    }
    else
    {
        fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s no device export for LU: %d\n", __FUNCTION__, *lun_object_id_p);
        status = FBE_STATUS_NO_DEVICE;
    }
#endif

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/******************************
 * end fbe_bvd_interface_usurper_unexport_lun()
 ******************************/

/******************************
 * end fbe_bvd_interface_usurper.c
 ******************************/
