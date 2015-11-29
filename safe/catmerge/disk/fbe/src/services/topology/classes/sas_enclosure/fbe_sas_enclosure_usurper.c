/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_sas_enclosure_usurper.c
 ***************************************************************************
 *
 *  @brief
 *  This file contains control related handler for SAS enclosure
 *
 * @ingroup fbe_enclosure_class
 * 
 * HISTORY
 *   11/11/2008:  Added file header. - NC
 *   11/11/2008:  Added SMP edge.    - NC
 *
 ***************************************************************************/
#include "fbe_sas_port.h"
#include "fbe_base_discovered.h"
#include "sas_enclosure_private.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"

/* Forward declaration */
static fbe_status_t sas_enclosure_attach_detach_edge_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_enclosure_open_ssp_edge_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_enclosure_get_ssp_edge_info(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet);
static fbe_status_t sas_enclosure_set_ssp_edge_tap_hook(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet_p);
static fbe_status_t sas_enclosure_open_smp_edge_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_enclosure_get_smp_edge_info(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet);

/*!**************************************************************
 * @fn fbe_sas_enclosure_control_entry(
 *                         fbe_object_handle_t object_handle, 
 *                         fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function is the entry point for control
 *  operations to the sas enclosure object.
 *
 * @param object_handle - Handler to the enclosure object.
 * @param packet_p - The control packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK, if no error
 *
 * HISTORY:
 *   11/11/2008:  Added function header. NC
 *
 ****************************************************************/
fbe_status_t 
fbe_sas_enclosure_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_enclosure_t * sas_enclosure = NULL;
    fbe_payload_control_operation_opcode_t control_code;

    sas_enclosure = (fbe_sas_enclosure_t *)fbe_base_handle_to_pointer(object_handle);
    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "%s entry \n", __FUNCTION__);

    control_code = fbe_transport_get_control_code(packet);
    switch(control_code) {
        case FBE_SSP_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO:
            status = sas_enclosure_get_ssp_edge_info( sas_enclosure, packet);
            break;
        case FBE_SMP_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO:
            status = sas_enclosure_get_smp_edge_info( sas_enclosure, packet);
            break;
        case FBE_SSP_TRANSPORT_CONTROL_CODE_SET_EDGE_TAP_HOOK:
            status = sas_enclosure_set_ssp_edge_tap_hook(sas_enclosure, packet);
            break;

        default:
            status = fbe_base_enclosure_control_entry(object_handle, packet);
            break;
    }

    return status;
}

/*!**************************************************************
 * @fn fbe_sas_enclosure_attach_ssp_edge(
 *                         fbe_sas_enclosure_t * sas_enclosure, 
 *                         fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function attach ssp (protocol) edge to port object.
 * This edge is used to carry out SCSI IO request to the expander.
 *
 * @param sas_enclosure - Handler to the enclosure object.
 * @param packet_p - The control packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK, if no error
 *
 * HISTORY:
 *   11/11/2008:  Added function header. NC
 *
 ****************************************************************/
fbe_status_t
fbe_sas_enclosure_attach_ssp_edge(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet)
{
    fbe_ssp_transport_control_attach_edge_t ssp_transport_control_attach_edge; /* sent to the sas_port object */
    fbe_path_state_t path_state;
    fbe_object_id_t my_object_id;
    fbe_object_id_t port_object_id;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    fbe_status_t status;
    fbe_semaphore_t sem;

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "%s entry \n", __FUNCTION__);

    /* Build the edge. ( We have to perform some sanity checking here) */ 
    fbe_ssp_transport_get_path_state(&sas_enclosure->ssp_edge, &path_state);
    if(path_state != FBE_PATH_STATE_INVALID) { /* discovery edge may be connected only once */
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                            "path_state %X\n",path_state);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_discovered_get_port_object_id((fbe_base_discovered_t *)sas_enclosure, &port_object_id);

    fbe_ssp_transport_set_server_id(&sas_enclosure->ssp_edge, port_object_id);
    /* We have to provide sas_address and generation code to help port object to identify the client */

    fbe_ssp_transport_set_transport_id(&sas_enclosure->ssp_edge);

    fbe_base_object_get_object_id((fbe_base_object_t *)sas_enclosure, &my_object_id);

    fbe_ssp_transport_set_client_id(&sas_enclosure->ssp_edge, my_object_id);
    fbe_ssp_transport_set_client_index(&sas_enclosure->ssp_edge, 0); /* We have only one ssp edge */

    ssp_transport_control_attach_edge.ssp_edge = &sas_enclosure->ssp_edge;

    /* use semaphore because ssp_transport_control_attach_edge is from stack */
    fbe_semaphore_init(&sem, 0, 1);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

    status =  fbe_payload_control_build_operation(control_operation, 
                                                FBE_SSP_TRANSPORT_CONTROL_CODE_ATTACH_EDGE, 
                                                &ssp_transport_control_attach_edge, 
                                                sizeof(fbe_ssp_transport_control_attach_edge_t));

    status = fbe_transport_set_completion_function(packet, sas_enclosure_attach_detach_edge_completion, &sem);


    /* We are sending control packet, so we have to form address manually */
    /* Set packet address */
    fbe_transport_set_address(  packet,
                                FBE_PACKAGE_ID_PHYSICAL,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                port_object_id);

    /* Control packets should be sent via service manager */
    status = fbe_service_manager_send_control_packet(packet);

    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

    return FBE_STATUS_OK;
}

/*!*************************************************************************
* @fn sas_enclosure_attach_detach_edge_completion(
*                    fbe_packet_t * packet, 
*                    fbe_packet_completion_context_t context)                  
***************************************************************************
*
* @brief
*       This function handles the completion of attaching/detaching 
*  ssp/smp edge and resume the attach/detach process.
*
* @param      packet - The pointer to the fbe_packet_t.
* @param      context - The pointer to the context (semaphore).
*
* @return      fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*   11/11/2008:  Added function header. NC
***************************************************************************/
static fbe_status_t 
sas_enclosure_attach_detach_edge_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    /* We are going to release the operation first */
    if (packet != NULL )
    {
        payload = fbe_transport_get_payload_ex(packet);
    }
    if (payload != NULL)
    {
        control_operation = fbe_payload_ex_get_control_operation(payload);
    }
    if (control_operation != NULL)
    {
        fbe_payload_ex_release_control_operation(payload, control_operation);
    }
    else
    {
        fbe_topology_class_trace(FBE_CLASS_ID_SAS_ENCLOSURE,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s, unable to release control operation. packet %p, payload %p\n",
                                __FUNCTION__, 
                                packet, 
                                payload);
    }

    if (sem != NULL)
    {
        fbe_semaphore_release(sem, 0, 1, FALSE);
    }
    else
    {
        fbe_topology_class_trace(FBE_CLASS_ID_SAS_ENCLOSURE,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s, no semaphore to release. packet %p\n",
                                __FUNCTION__, 
                                packet);
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_sas_enclosure_detach_ssp_edge(
 *                         fbe_sas_enclosure_t * sas_enclosure, 
 *                         fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function dettach ssp edge from port object
 *
 * @param sas_enclosure - Handler to the enclosure object.
 * @param packet_p - The control packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK, if no error
 *
 * HISTORY:
 *   11/11/2008:  Added function header. NC
 *   11/12/2008:  Moved from fbe_sas_enclosure_main.c. NC
 *
 ****************************************************************/
fbe_status_t 
fbe_sas_enclosure_detach_ssp_edge(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet)
{
    fbe_ssp_transport_control_detach_edge_t ssp_detach_edge; /* sent to the server object */
    fbe_payload_ex_t * payload = NULL;
    fbe_path_state_t path_state;
    fbe_payload_control_operation_t * control_operation = NULL;

    fbe_status_t status;
    fbe_semaphore_t sem;

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "%s entry\n", __FUNCTION__);

    status = fbe_ssp_transport_get_path_state(&sas_enclosure->ssp_edge, &path_state);
    /* If we don't have an edge to detach, complete the packet and return Good status. */
    if(status != FBE_STATUS_OK || path_state == FBE_PATH_STATE_INVALID)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_DEBUG_LOW,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                            "%s SSP edge is Invalid. No need to detach it.\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    ssp_detach_edge.ssp_edge = &sas_enclosure->ssp_edge;

    /* use semaphore because ssp_detach_edge is from stack */
    fbe_semaphore_init(&sem, 0, 1);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

    status =  fbe_payload_control_build_operation(control_operation, 
                                                FBE_SSP_TRANSPORT_CONTROL_CODE_DETACH_EDGE, 
                                                &ssp_detach_edge, 
                                                sizeof(fbe_ssp_transport_control_detach_edge_t));

    status = fbe_transport_set_completion_function(packet, sas_enclosure_attach_detach_edge_completion, &sem);

    fbe_ssp_transport_send_control_packet(&sas_enclosure->ssp_edge, packet);

    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * @fn fbe_sas_enclosure_open_ssp_edge(
 *                         fbe_sas_enclosure_t * sas_enclosure, 
 *                         fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function opens ssp (protocol) edge to port object 
 * with SES virtual port address of the enclosure.
 *
 * @param sas_enclosure - Handler to the enclosure object.
 * @param packet_p - The control packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK, if no error
 *
 * HISTORY:
 *   11/11/2008:  Added function header. NC
 *
 ****************************************************************/
fbe_status_t
fbe_sas_enclosure_open_ssp_edge(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet)
{
    fbe_ssp_transport_control_open_edge_t * ssp_transport_control_open_edge = NULL; /* sent to the sas_port object */
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_status_t status;
    fbe_u8_t * buffer;

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "%s entry\n", __FUNCTION__);

    buffer = fbe_transport_allocate_buffer();
    if(buffer == NULL) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                            "%s fbe_transport_allocate_buffer fail\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    ssp_transport_control_open_edge = (fbe_ssp_transport_control_open_edge_t *)buffer;
    /* Open the SSP edge using the SES virtual port address for the enclosure */
    ssp_transport_control_open_edge->sas_address = sas_enclosure->ses_port_address;
    ssp_transport_control_open_edge->generation_code = sas_enclosure->ses_port_address_generation_code; 
    ssp_transport_control_open_edge->status = FBE_SSP_TRANSPORT_CONTROL_CODE_OPEN_EDGE_STATUS_INVALID;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

    status =  fbe_payload_control_build_operation(control_operation, 
                                                FBE_SSP_TRANSPORT_CONTROL_CODE_OPEN_EDGE, 
                                                ssp_transport_control_open_edge, 
                                                sizeof(fbe_ssp_transport_control_open_edge_t));

    status = fbe_transport_set_completion_function(packet, sas_enclosure_open_ssp_edge_completion, sas_enclosure);

    status = fbe_ssp_transport_send_control_packet(&sas_enclosure->ssp_edge, packet);

    return status;
}

/*!*************************************************************************
* @fn sas_enclosure_open_ssp_edge_completion(
*                    fbe_packet_t * packet, 
*                    fbe_packet_completion_context_t context)                  
***************************************************************************
*
* @brief
*       This function handles the completion of opening ssp edge.
*
* @param      packet - The pointer to the fbe_packet_t.
* @param      context - The pointer to the context (fbe_sas_enclosure_t).
*
* @return      fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*   11/11/2008:  Added function header. NC
***************************************************************************/
static fbe_status_t 
sas_enclosure_open_ssp_edge_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_enclosure_t * sas_enclosure = NULL;
    fbe_ssp_transport_control_open_edge_t * ssp_transport_control_open_edge = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_status_t status;

    sas_enclosure = (fbe_sas_enclosure_t *)context;

    if (packet != NULL)
    {
        payload = fbe_transport_get_payload_ex(packet);
    }

    if (payload != NULL)
    {
        control_operation = fbe_payload_ex_get_control_operation(payload);
    }

    if (control_operation != NULL)
    {
        fbe_payload_control_get_buffer(control_operation, &ssp_transport_control_open_edge);
    }

    if (ssp_transport_control_open_edge != NULL)
    {
        if (ssp_transport_control_open_edge->status == FBE_SSP_TRANSPORT_CONTROL_CODE_OPEN_EDGE_STATUS_NO_DEVICE)
        {
            /* If port tells us it's not there, we'd better try to get it again */
            status = fbe_lifecycle_set_cond(&fbe_sas_enclosure_lifecycle_const,
                                        (fbe_base_object_t*)sas_enclosure,
                                        FBE_SAS_ENCLOSURE_LIFECYCLE_COND_GET_VIRTUAL_PORT_ADDRESS);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                    "%s, can't set GET VIRTUAL PORT ADDRESS condition, status: 0x%x.\n",
                                    __FUNCTION__, status);

                return FBE_STATUS_OK;
            }
        }
        fbe_transport_release_buffer(ssp_transport_control_open_edge);
    }
    else
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_INFO,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                            "%s, unable to release buffer. packet %p, payload %p, control %p \n", 
                                            __FUNCTION__,packet,payload,control_operation);

        
    }
    if (control_operation != NULL)
    {
        fbe_payload_ex_release_control_operation(payload, control_operation);
    }
    else
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_INFO,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                            "%s, unable to release control operation. packet %p, payload %p\n", 
                                            __FUNCTION__, packet, payload);
        
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn sas_enclosure_get_ssp_edge_info(
 *                         fbe_sas_enclosure_t * sas_enclosure, 
 *                         fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function fills in information about the ssp edge into payload buffer
 *
 * @param sas_enclosure - Handler to the enclosure object.
 * @param packet_p - The control packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK, if no error
 *
 * HISTORY:
 *   11/11/2008:  Added function header. NC
 *
 ****************************************************************/
static fbe_status_t 
sas_enclosure_get_ssp_edge_info(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet)
{
    fbe_ssp_transport_control_get_edge_info_t * get_edge_info = NULL; /* INPUT */
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;   

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);

    control_operation = fbe_payload_ex_get_control_operation(payload); 
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &get_edge_info);
    if(get_edge_info == NULL) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                            "fbe_payload_control_get_buffer failed\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_ssp_transport_get_client_id(&sas_enclosure->ssp_edge, &get_edge_info->base_edge_info.client_id);
    status = fbe_ssp_transport_get_server_id(&sas_enclosure->ssp_edge, &get_edge_info->base_edge_info.server_id);

    status = fbe_ssp_transport_get_client_index(&sas_enclosure->ssp_edge, &get_edge_info->base_edge_info.client_index);
    status = fbe_ssp_transport_get_server_index(&sas_enclosure->ssp_edge, &get_edge_info->base_edge_info.server_index);

    status = fbe_ssp_transport_get_path_state(&sas_enclosure->ssp_edge, &get_edge_info->base_edge_info.path_state);
    status = fbe_ssp_transport_get_path_attributes(&sas_enclosure->ssp_edge, &get_edge_info->base_edge_info.path_attr);

    get_edge_info->base_edge_info.transport_id = FBE_TRANSPORT_ID_SSP;

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_sas_enclosure_attach_smp_edge(
 *                         fbe_sas_enclosure_t * sas_enclosure, 
 *                         fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function attach smp edge to port object. 
 * This edge is created to receive children login/logout
 * information for the same enclosure.  Get_child_list operation
 * is also carried out on this edge.
 *
 * @param sas_enclosure - Handler to the enclosure object.
 * @param packet_p - The control packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK, if no error
 *
 * HISTORY:
 *   11/11/2008:  created. NC
 *
 ****************************************************************/
fbe_status_t
fbe_sas_enclosure_attach_smp_edge(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet)
{
    fbe_smp_transport_control_attach_edge_t smp_transport_control_attach_edge; /* sent to the sas_port object */
    fbe_path_state_t path_state;
    fbe_object_id_t my_object_id;
    fbe_object_id_t port_object_id;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    fbe_status_t status;
    fbe_semaphore_t sem;


    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "%s entry\n", __FUNCTION__);
    

    /* Build the edge. ( We have to perform some sanity checking here) */ 
    fbe_smp_transport_get_path_state(&sas_enclosure->smp_edge, &path_state);
    if(path_state != FBE_PATH_STATE_INVALID) { /* discovery edge may be connected only once */
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                            "path_state %X\n", path_state);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_discovered_get_port_object_id((fbe_base_discovered_t *)sas_enclosure, &port_object_id);

    fbe_smp_transport_set_server_id(&sas_enclosure->smp_edge, port_object_id);
    /* We have to provide sas_address and generation code to help port object to identify the client */

    fbe_smp_transport_set_transport_id(&sas_enclosure->smp_edge);

    fbe_base_object_get_object_id((fbe_base_object_t *)sas_enclosure, &my_object_id);

    fbe_smp_transport_set_client_id(&sas_enclosure->smp_edge, my_object_id);
    fbe_smp_transport_set_client_index(&sas_enclosure->smp_edge, 0); /* We have only one smp edge */

    smp_transport_control_attach_edge.smp_edge = &sas_enclosure->smp_edge;

    /* use semaphore because smp_transport_control_attach_edge is from stack */
    fbe_semaphore_init(&sem, 0, 1);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

    status =  fbe_payload_control_build_operation(control_operation, 
                                                FBE_SMP_TRANSPORT_CONTROL_CODE_ATTACH_EDGE, 
                                                &smp_transport_control_attach_edge, 
                                                sizeof(fbe_smp_transport_control_attach_edge_t));

    status = fbe_transport_set_completion_function(packet, sas_enclosure_attach_detach_edge_completion, &sem);


    /* We are sending control packet, so we have to form address manually */
    /* Set packet address */
    fbe_transport_set_address(  packet,
                                FBE_PACKAGE_ID_PHYSICAL,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                port_object_id);

    /* Control packets should be sent via service manager */
    status = fbe_service_manager_send_control_packet(packet);

    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

    return FBE_STATUS_OK;
}
/*!**************************************************************
 * @fn fbe_sas_enclosure_detach_smp_edge(
 *                         fbe_sas_enclosure_t * sas_enclosure, 
 *                         fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function dettach smp edge to port object
 * with the SMP address of the expander.
 *
 * @param sas_enclosure - Handler to the enclosure object.
 * @param packet_p - The control packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK, if no error
 *
 * HISTORY:
 *   11/11/2008:  created. NC
 *
 ****************************************************************/
fbe_status_t 
fbe_sas_enclosure_detach_smp_edge(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet)
{
    fbe_smp_transport_control_detach_edge_t smp_detach_edge; /* sent to the server object */
    fbe_payload_ex_t * payload = NULL;
    fbe_path_state_t path_state;
    fbe_payload_control_operation_t * control_operation = NULL;

    fbe_status_t status;
    fbe_semaphore_t sem;

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "%s entry\n", __FUNCTION__);

    status = fbe_smp_transport_get_path_state(&sas_enclosure->smp_edge, &path_state);
    /* If we don't have an edge to detach, complete the packet and return Good status. */
    if(status != FBE_STATUS_OK || path_state == FBE_PATH_STATE_INVALID)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_DEBUG_LOW,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                            "%s SMP edge is Invalid. No need to detach it.\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    smp_detach_edge.smp_edge = &sas_enclosure->smp_edge;

    /* use semaphore because smp_detach_edge is from stack */
    fbe_semaphore_init(&sem, 0, 1);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

    status =  fbe_payload_control_build_operation(control_operation, 
                                                FBE_SMP_TRANSPORT_CONTROL_CODE_DETACH_EDGE, 
                                                &smp_detach_edge, 
                                                sizeof(fbe_smp_transport_control_detach_edge_t));


    status = fbe_transport_set_completion_function(packet, sas_enclosure_attach_detach_edge_completion, &sem);
    fbe_smp_transport_send_control_packet(&sas_enclosure->smp_edge, packet);
    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_sas_enclosure_open_smp_edge(
 *                         fbe_sas_enclosure_t * sas_enclosure, 
 *                         fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function opens smp edge to port object with ses_port_address
 *
 * @param sas_enclosure - Handler to the enclosure object.
 * @param packet_p - The control packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK, if no error
 *
 * HISTORY:
 *   11/11/2008:  created. NC
 *
 ****************************************************************/
fbe_status_t
fbe_sas_enclosure_open_smp_edge(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet)
{
    fbe_smp_transport_control_open_edge_t * smp_transport_control_open_edge = NULL; /* sent to the sas_port object */
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_status_t status;
    fbe_u8_t * buffer;

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "%s entry\n", __FUNCTION__);

    buffer = fbe_transport_allocate_buffer();
    if(buffer == NULL) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                            "%s fbe_transport_allocate_buffer fail\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    smp_transport_control_open_edge = (fbe_smp_transport_control_open_edge_t *)buffer;
    /* Open the SMP edge using the SMP address for the enclosure */
    smp_transport_control_open_edge->sas_address = sas_enclosure->sasEnclosureSMPAddress;
    smp_transport_control_open_edge->generation_code = sas_enclosure->smp_address_generation_code; 
    smp_transport_control_open_edge->status = FBE_SMP_TRANSPORT_CONTROL_CODE_OPEN_EDGE_STATUS_INVALID;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

    status =  fbe_payload_control_build_operation(control_operation, 
                                                FBE_SMP_TRANSPORT_CONTROL_CODE_OPEN_EDGE, 
                                                smp_transport_control_open_edge, 
                                                sizeof(fbe_smp_transport_control_open_edge_t));

    status = fbe_transport_set_completion_function(packet, sas_enclosure_open_smp_edge_completion, sas_enclosure);
    status = fbe_smp_transport_send_control_packet(&sas_enclosure->smp_edge, packet);
    return status;
}

/*!*************************************************************************
* @fn sas_enclosure_open_smp_edge_completion(
*                    fbe_packet_t * packet, 
*                    fbe_packet_completion_context_t context)                  
***************************************************************************
*
* @brief
*       This function handles the completion of opening smp edge.
*
* @param      packet - The pointer to the fbe_packet_t.
* @param      context - The pointer to the context (sas_enclosure object).
*
* @return      fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
 *   11/11/2008:  created. NC
***************************************************************************/
static fbe_status_t 
sas_enclosure_open_smp_edge_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_smp_transport_control_open_edge_t * smp_transport_control_open_edge = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_sas_enclosure_t * sas_enclosure = NULL;
    fbe_status_t status;
    fbe_lifecycle_state_t lifecycle_state;

    sas_enclosure = (fbe_sas_enclosure_t *)context;

    if (packet != NULL)
    {
        payload = fbe_transport_get_payload_ex(packet);
    }
    if (payload != NULL)
    {
        control_operation = fbe_payload_ex_get_control_operation(payload);
    }
    if (control_operation != NULL)
    {
        fbe_payload_control_get_buffer(control_operation, &smp_transport_control_open_edge);
    }
    if (smp_transport_control_open_edge != NULL)
    {
        if (smp_transport_control_open_edge->status == FBE_SMP_TRANSPORT_CONTROL_CODE_OPEN_EDGE_STATUS_NO_DEVICE)
        {

            status = fbe_lifecycle_get_state(&fbe_sas_enclosure_lifecycle_const,
                                    (fbe_base_object_t*)sas_enclosure,
                                    &lifecycle_state);
    
            if((status == FBE_STATUS_OK) && (lifecycle_state == FBE_LIFECYCLE_STATE_READY))
            {
                 fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                       FBE_TRACE_LEVEL_INFO,
                                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) sas_enclosure),
                                       "Changing state from READY to ACTIVATE. Reason: Failed to open SMP edge.\n");
            }
        
            /* If port tells us it's not there, we'd better try to get it again */
            status = fbe_lifecycle_set_cond(&fbe_sas_enclosure_lifecycle_const,
                                        (fbe_base_object_t*)sas_enclosure,
                                        FBE_SAS_ENCLOSURE_LIFECYCLE_COND_GET_SMP_ADDRESS);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                    "%s, can't set GET SMP ADDRESS condition, status: 0x%x.\n",
                                    __FUNCTION__, status);

                return FBE_STATUS_OK;
            }
        }
        fbe_transport_release_buffer(smp_transport_control_open_edge);
    }
    else
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_INFO,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                            "%s, unable to release buffer. packet %p, payload %p, control %p \n", 
                                            __FUNCTION__, packet, payload, control_operation  );
    }
    if (control_operation != NULL)
    {
        fbe_payload_ex_release_control_operation(payload, control_operation);
    }
    else
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_INFO,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                            "%s, unable to release control operation. packet %p, payload %p \n", 
                                            __FUNCTION__, packet, payload);
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn sas_enclosure_get_smp_edge_info(
 *                         fbe_sas_enclosure_t * sas_enclosure, 
 *                         fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function returns information about the smp edge
 *
 * @param sas_enclosure - Handler to the enclosure object.
 * @param packet_p - The control packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK, if no error
 *
 * HISTORY:
 *   11/11/2008:  created. NC
 *
 ****************************************************************/
static fbe_status_t 
sas_enclosure_get_smp_edge_info(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet)
{
    fbe_smp_transport_control_get_edge_info_t * get_edge_info = NULL; /* INPUT */
    fbe_status_t status;
    fbe_payload_control_buffer_length_t out_len = 0;

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "%s entry\n", __FUNCTION__);


   out_len = sizeof(fbe_smp_transport_control_get_edge_info_t);
   status = fbe_base_enclosure_get_packet_payload_control_data(packet,(fbe_base_enclosure_t*)sas_enclosure,TRUE,(fbe_payload_control_buffer_t *)&get_edge_info,out_len);
   if(status == FBE_STATUS_OK)
   {

    status = fbe_smp_transport_get_client_id(&sas_enclosure->smp_edge, &get_edge_info->base_edge_info.client_id);
    status = fbe_smp_transport_get_server_id(&sas_enclosure->smp_edge, &get_edge_info->base_edge_info.server_id);

    status = fbe_smp_transport_get_client_index(&sas_enclosure->smp_edge, &get_edge_info->base_edge_info.client_index);
    status = fbe_smp_transport_get_server_index(&sas_enclosure->smp_edge, &get_edge_info->base_edge_info.server_index);

    status = fbe_smp_transport_get_path_state(&sas_enclosure->smp_edge, &get_edge_info->base_edge_info.path_state);
    status = fbe_smp_transport_get_path_attributes(&sas_enclosure->smp_edge, &get_edge_info->base_edge_info.path_attr);

    get_edge_info->base_edge_info.transport_id = FBE_TRANSPORT_ID_SMP;
   }
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}

/*!**************************************************************
 * @fn sas_enclosure_set_ssp_edge_tap_hook(fbe_sas_enclosure_t * sas_enclosure, 
 *                                     fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function sets the edge tap hook function on ssp edge.
 *
 * @param sas_enclosure - The enclosure object to set the hook on.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * HISTORY:
 *  05/26/2009 - Created. Vicky Guo.
 *
 ****************************************************************/
static fbe_status_t
sas_enclosure_set_ssp_edge_tap_hook(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet_p)
{
    fbe_transport_control_set_edge_tap_hook_t * hook_info = NULL;    /* INPUT */
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_control_buffer_length_t out_len = 0;

    fbe_base_object_trace(  (fbe_base_object_t *)sas_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

   out_len = sizeof(fbe_transport_control_set_edge_tap_hook_t);
   status = fbe_base_enclosure_get_packet_payload_control_data(packet_p,(fbe_base_enclosure_t*)sas_enclosure,TRUE,(fbe_payload_control_buffer_t *)&hook_info,out_len);
   if(status == FBE_STATUS_OK)
   {
        /* ssp edge is a static member of sas_enclosure, 
           therefore &sas_enclosure->ssp_edge is always not NULL */
        status = fbe_ssp_transport_edge_set_hook(&sas_enclosure->ssp_edge, hook_info->edge_tap_hook);
    }
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return status;
}

/**************************************
 * end fbe_sas_enclosure_set_edge_tap_hook()
 **************************************/

