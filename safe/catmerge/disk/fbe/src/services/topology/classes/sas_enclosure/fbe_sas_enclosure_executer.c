/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_sas_enclosure_executer.c
 ***************************************************************************
 *
 *  @brief
 *  This file contains condition handler for SAS enclosure class.
 *
 * @ingroup fbe_enclosure_class
 * 
 * HISTORY
 *   11/20/2008:  Created file header - NC
 *
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "sas_enclosure_private.h"
#include "edal_sas_enclosure_data.h"
#include "fbe_transport_memory.h"
#include "fbe_scsi.h"
#include "swap_exports.h"

/* Forward declarations */
static fbe_status_t fbe_sas_enclosure_get_smp_address_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_enclosure_send_inquiry_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_sas_enclosure_send_get_element_list_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

/* static fbe_status_t sas_enclosure_discovery_transport_get_port_object_id(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet); */
static fbe_status_t
fbe_sas_enclosure_edge_state_change_event_entry(fbe_sas_enclosure_t * sas_enclosure,
                                                 fbe_event_context_t event_context);

fbe_status_t 
fbe_sas_enclosure_get_smp_address(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_packet_t * new_packet = NULL;
    /* fbe_io_block_t * io_block = NULL; */
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;

    fbe_u8_t * buffer = NULL;
    fbe_object_id_t my_object_id;
    fbe_sg_element_t  * sg_list = NULL; 


    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                        "%s entry\n", __FUNCTION__);
    /* Allocate packet */
    new_packet = fbe_transport_allocate_packet();
    if(new_packet == NULL) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                            "%s fbe_transport_allocate_packet fail \n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    buffer = fbe_transport_allocate_buffer();
    if(buffer == NULL) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                            "%s fbe_transport_allocate_buffer fail \n", __FUNCTION__);

        fbe_transport_release_packet(new_packet);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_packet(new_packet);
    fbe_base_object_get_object_id((fbe_base_object_t *)sas_enclosure, &my_object_id);
    payload = fbe_transport_get_payload_ex(new_packet);
    payload_discovery_operation = fbe_payload_ex_allocate_discovery_operation(payload);
    fbe_payload_discovery_build_get_protocol_address(payload_discovery_operation, my_object_id);

    /* Provide memory for the responce */
    sg_list = (fbe_sg_element_t  *)buffer;
    sg_list[0].count = sizeof(fbe_payload_discovery_get_protocol_address_data_t);
    sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);

    /* We need to assosiate newly allocated packet with original one */
    fbe_transport_add_subpacket(packet, new_packet);

    fbe_transport_set_cancel_function(packet, fbe_base_object_packet_cancel_function, (fbe_base_object_t *)sas_enclosure);
    fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)sas_enclosure, packet);

    status = fbe_transport_set_completion_function(new_packet, fbe_sas_enclosure_get_smp_address_completion, sas_enclosure);

    /* Note we can not send the packet right away.
        It is possible that blocking is set or queue depth is exeded.
        So, we should "submit" "send" the packet to the bouncer for evaluation.
        Right now we have no bouncer implemented, but as we will have it we will use it.
    */
    /* We are sending discovery packet via discovery edge */
    status = fbe_base_discovered_send_functional_packet((fbe_base_discovered_t *) sas_enclosure, new_packet);

    return status;
}

static fbe_status_t 
fbe_sas_enclosure_get_smp_address_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_enclosure_t * sas_enclosure = NULL;
    fbe_object_id_t my_object_id;
    fbe_packet_t * master_packet = NULL;
    fbe_sg_element_t  * sg_list = NULL; 
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_payload_discovery_get_protocol_address_data_t * payload_discovery_get_protocol_address_data = NULL;
    fbe_status_t status;  

    sas_enclosure = (fbe_sas_enclosure_t *)context;

    master_packet = (fbe_packet_t *)fbe_transport_get_master_packet(packet);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);

    payload = fbe_transport_get_payload_ex(packet);
    payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    /* We only need to process the response when we have good packet status */
    if (status == FBE_STATUS_OK)
    {
        payload_discovery_get_protocol_address_data = (fbe_payload_discovery_get_protocol_address_data_t *)sg_list[0].address;
        fbe_base_object_get_object_id((fbe_base_object_t *)sas_enclosure, &my_object_id);
        sas_enclosure->sasEnclosureSMPAddress = payload_discovery_get_protocol_address_data->address.sas_address;
        sas_enclosure->smp_address_generation_code = payload_discovery_get_protocol_address_data->generation_code;

        /*!  Naizhing:  We redefined enclosure_number in voyager
        * branch.  Instead of calling
        * fbe_base_enclosure_set_enclosure_number() we now call the
        * following:
        */
        fbe_base_enclosure_set_my_position((fbe_base_enclosure_t *) sas_enclosure, 
            payload_discovery_get_protocol_address_data->chain_depth);

        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                            "%s. sas_address = %llX, my_object_id :%d \n",
                            __FUNCTION__, (unsigned long long)sas_enclosure->sasEnclosureSMPAddress, my_object_id);
    }
    /* We need to remove packet from master queue */
    fbe_transport_remove_subpacket(packet);

    fbe_transport_release_buffer(sg_list);
    fbe_payload_ex_release_discovery_operation(payload, payload_discovery_operation);
    fbe_transport_release_packet(packet);

    /* Remove master packet from the termination queue. */
    fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *)sas_enclosure, master_packet);

    fbe_transport_set_status(master_packet, status, 0);
    fbe_transport_complete_packet(master_packet);

    return FBE_STATUS_OK;
}


/****************************************************************
 * fbe_sas_enclosure_event_entry()
 ****************************************************************
 * DESCRIPTION:
 *  This function handles events to the enclosure object.
 *
 * PARAMETERS:
 *  object_handle - The object receiving the event
 *  event_type - The type of event.
 *  event_context - The context for the event.
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  8/5/08 - updated. bphilbin
 *
 ****************************************************************/
fbe_status_t 
fbe_sas_enclosure_event_entry(fbe_object_handle_t object_handle, 
                              fbe_event_type_t event_type, 
                              fbe_event_context_t event_context)
{
    fbe_sas_enclosure_t * sas_enclosure = NULL;
    fbe_status_t status;

    sas_enclosure = (fbe_sas_enclosure_t *)fbe_base_handle_to_pointer(object_handle);


    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                        "%s entry\n", __FUNCTION__);
                        
    switch(event_type)
    {
    case FBE_EVENT_TYPE_EDGE_STATE_CHANGE:
        status = fbe_sas_enclosure_edge_state_change_event_entry(sas_enclosure, event_context); 
        break;
    default:
        status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
        break;
    }
    if(status == FBE_STATUS_MORE_PROCESSING_REQUIRED)
    {
        /*
         * This class did not handle the event, go to the superclass
         * to try to handle it.
         */
        status = fbe_base_enclosure_event_entry(object_handle, event_type, event_context);
    }
    

    return status; 
} /* End of fbe_sas_enclosure_event_entry */

/****************************************************************
 * fbe_sas_enclosure_edge_state_change_event_entry()
 ****************************************************************
 * DESCRIPTION:
 *  This function handles edge state change events.
 *
 * PARAMETERS:
 *  sas_viper_enclosure - The object receiving the event
 *  event_context - hidden in this void* is a pointer to the edge
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK 
 *                   - no error.
 *                 FBE_STATUS_MORE_PROCESSING_REQUIRED
 *                   - imply the event could not be handled here.  
 *
 * HISTORY:
 *  8/5/08 - updated. bphilbin
 *
 ****************************************************************/
static fbe_status_t
fbe_sas_enclosure_edge_state_change_event_entry(fbe_sas_enclosure_t * sas_enclosure,
                                                 fbe_event_context_t event_context)
{
    fbe_path_state_t edge_state;
    fbe_status_t status;
    fbe_transport_id_t transport_id;
    fbe_path_attr_t path_attr;    
    fbe_lifecycle_state_t lifecycle_state;


    
    
    /* Get the transport type from the edge pointer. */
    fbe_base_transport_get_transport_id((fbe_base_edge_t *) event_context, &transport_id);

    /* 
     * Determine if this is coming from the discovery edge, the SSP edge, or the SMP edge based on the
     * transport type.
     */
    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "%s Entry for transport_id: %d\n",__FUNCTION__, transport_id);
    switch(transport_id) {
        case FBE_TRANSPORT_ID_SSP:
            /* SSP will be handled by sas_enclosure */
            status = FBE_STATUS_OK;

            fbe_ssp_transport_get_path_state(&sas_enclosure->ssp_edge, &edge_state);

            /* Check the SSP edge states */
            switch(edge_state)
            {
            case FBE_PATH_STATE_ENABLED:
                fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "SSP edge state Enabled\n");
                /* 
                 * In the case where the path state is enabled we can now check the attribute
                 * of the SSP (protocol) edge.
                 */
                status = fbe_base_transport_get_path_attributes((
                                    fbe_base_edge_t *) (&sas_enclosure->ssp_edge), &path_attr);
                if(path_attr & FBE_SSP_PATH_ATTR_CLOSED)
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_LOW,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "SSP edge CLOSED\n");

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

                        return status;
                    }

                    status = fbe_lifecycle_get_state(&fbe_sas_enclosure_lifecycle_const,
                                    (fbe_base_object_t*)sas_enclosure,
                                    &lifecycle_state);
                    
                    if((status == FBE_STATUS_OK) && (lifecycle_state == FBE_LIFECYCLE_STATE_READY))
                    {
                        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                              FBE_TRACE_LEVEL_INFO,
                                              fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) sas_enclosure),
                                              "Changing state from READY to ACTIVATE. Reason: SES port logged out.\n");
                    }

                    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_LOW,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "setting open SSP Edge condition\n");

                    status = fbe_lifecycle_set_cond(&fbe_sas_enclosure_lifecycle_const,
                                                    (fbe_base_object_t*)sas_enclosure,
                                                    FBE_SAS_ENCLOSURE_LIFECYCLE_COND_OPEN_SSP_EDGE);

                    if (status != FBE_STATUS_OK) 
                    {
                        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                            "%s, can't set OPEN SSP EDGE condition, status: 0x%x.\n",
                                            __FUNCTION__, status);

                        return status;
                    }
                }
                break;
            case FBE_PATH_STATE_GONE:
                fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                    "%s  SSP edge GONE\n", __FUNCTION__);

                /* The path state is gone, so we will transition ourselves to 
                 * destroy. 
                 */
                status = fbe_lifecycle_set_cond(&fbe_sas_enclosure_lifecycle_const,
                                                (fbe_base_object_t*)sas_enclosure,
                                                FBE_BASE_OBJECT_LIFECYCLE_COND_DESTROY);

                if (status != FBE_STATUS_OK) 
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "%s, can't set DESTROY condition, status: 0x%x.\n",
                                        __FUNCTION__, status);

                    return status;
                }
                break;
            case FBE_PATH_STATE_DISABLED:
                fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                    "SSP edge state DISABLED\n");

            default:
				fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                    "SSP edge state %d\n", edge_state);
                

                status = fbe_lifecycle_get_state(&fbe_sas_enclosure_lifecycle_const,
                                    (fbe_base_object_t*)sas_enclosure,
                                    &lifecycle_state);

                if((status == FBE_STATUS_OK) && (lifecycle_state == FBE_LIFECYCLE_STATE_READY))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_INFO,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) sas_enclosure),
                                            "Changing state from READY to ACTIVATE. Reason: Port object is not in READY state.\n");
                }

                fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                    "setting open SSP Edge condition\n");
                status = fbe_lifecycle_set_cond(&fbe_sas_enclosure_lifecycle_const,
                                                (fbe_base_object_t*)sas_enclosure,
                                                FBE_SAS_ENCLOSURE_LIFECYCLE_COND_OPEN_SSP_EDGE);

                if (status != FBE_STATUS_OK) 
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "%s, can't set OPEN SSP EDGE condition, status: 0x%x.\n",
                                        __FUNCTION__, status);

                    return status;
                }
                break;
            } /* End of edge_state switch */
            break;  // SSP edge
        case FBE_TRANSPORT_ID_SMP:
            /* SMP will be handled by sas_enclosure */
            status = FBE_STATUS_OK;

            fbe_smp_transport_get_path_state(&sas_enclosure->smp_edge, &edge_state);
            fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_LOW,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "%s, SMP edge state %d\n",__FUNCTION__, edge_state);

            /* Check the SMP edge states */
            switch(edge_state)
            {
            case FBE_PATH_STATE_ENABLED:
                /* 
                 * In the case where the path state is enabled we can now check the attribute
                 * of the SMP edge.
                 */
                status = fbe_base_transport_get_path_attributes((
                                    fbe_base_edge_t *) (&sas_enclosure->smp_edge), &path_attr);
                fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_LOW,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "%s, SMP edge State Enabled attributes 0x%x\n",__FUNCTION__, path_attr);
                /* monitor needs to try to reopen the edge is it's closed */
                if(path_attr & FBE_SMP_PATH_ATTR_CLOSED)
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_LOW,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "SMP edge CLOSED\n");

                    status = fbe_lifecycle_get_state(&fbe_sas_enclosure_lifecycle_const,
                                    (fbe_base_object_t*)sas_enclosure,
                                    &lifecycle_state);
    
                    if((status == FBE_STATUS_OK) && (lifecycle_state == FBE_LIFECYCLE_STATE_READY))
                    {
                        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                              FBE_TRACE_LEVEL_INFO,
                                              fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) sas_enclosure),
                                              "Changing state from READY to ACTIVATE. Reason: Expander Logged out.\n");
                    }

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

                        return status;
                    }

                    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                          FBE_TRACE_LEVEL_DEBUG_LOW,
                                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) sas_enclosure),
                                          "Setting Open SMP Edge condition.\n");


                    status = fbe_lifecycle_set_cond(&fbe_sas_enclosure_lifecycle_const,
                                                    (fbe_base_object_t*)sas_enclosure,
                                                    FBE_SAS_ENCLOSURE_LIFECYCLE_COND_OPEN_SMP_EDGE);

                    if (status != FBE_STATUS_OK) 
                    {
                        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                            "%s, can't set OPEN SMP EDGE condition, status: 0x%x.\n",
                                            __FUNCTION__, status);

                        return status;
                    }
                }
                /* If any child login status change, we need to refresh our children list */
                if(path_attr & FBE_SMP_PATH_ATTR_ELEMENT_CHANGE)
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_LOW,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "SMP edge ELEMENT_CHANGE\n");
                    
                    status = fbe_lifecycle_set_cond(&fbe_sas_enclosure_lifecycle_const,
                                                    (fbe_base_object_t*)sas_enclosure,
                                                    FBE_SAS_ENCLOSURE_LIFECYCLE_COND_GET_ELEMENT_LIST);
                    if (status != FBE_STATUS_OK) 
                    {
                        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                            "%s, can't set GET ELEMENT LIST condition, status: 0x%x.\n",
                                            __FUNCTION__, status);

                        return status;
                    }
                }
                break;
            case FBE_PATH_STATE_GONE:
                fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                    "SMP edge GONE. Destroying...\n");

                /* The path state is gone, so we will transition ourselves to 
                 * destroy. 
                 */
                status = fbe_lifecycle_set_cond(&fbe_sas_enclosure_lifecycle_const,
                                                (fbe_base_object_t*)sas_enclosure,
                                                FBE_BASE_OBJECT_LIFECYCLE_COND_DESTROY);
                if (status != FBE_STATUS_OK) 
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "%s, can't set DESTROY condition, status: 0x%x.\n",
                                        __FUNCTION__, status);

                    return status;
                }
                break;
            case FBE_PATH_STATE_DISABLED:
            default:
                fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                    "SMP edge state Disabled %d\n", edge_state);

                status = fbe_lifecycle_get_state(&fbe_sas_enclosure_lifecycle_const,
                                    (fbe_base_object_t*)sas_enclosure,
                                    &lifecycle_state);
    
                if((status == FBE_STATUS_OK) && (lifecycle_state == FBE_LIFECYCLE_STATE_READY))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                          FBE_TRACE_LEVEL_INFO,
                                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) sas_enclosure),
                                          "Changing state from READY to ACTIVATE. Reason:Port object is not in READY state.\n");
                }

                status = fbe_lifecycle_set_cond(&fbe_sas_enclosure_lifecycle_const,
                                                (fbe_base_object_t*)sas_enclosure,
                                                FBE_SAS_ENCLOSURE_LIFECYCLE_COND_GET_SMP_ADDRESS);
                fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                          FBE_TRACE_LEVEL_DEBUG_LOW,
                                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) sas_enclosure),
                                          "Setting Get SMP Address condition.\n");

                if (status != FBE_STATUS_OK) 
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "%s, can't set GET_SMP_ADDRESS condition, status: 0x%x.\n",
                                        __FUNCTION__, status);

                    return status;
                }

                
                status = fbe_lifecycle_set_cond(&fbe_sas_enclosure_lifecycle_const,
                                                (fbe_base_object_t*)sas_enclosure,
                                                FBE_SAS_ENCLOSURE_LIFECYCLE_COND_OPEN_SMP_EDGE);
                 fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                          FBE_TRACE_LEVEL_DEBUG_LOW,
                                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) sas_enclosure),
                                          "Setting Open SMP Edge condition.\n");

                if (status != FBE_STATUS_OK) 
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "%s, can't set ELEMENT_LIST condition, status: 0x%x.\n",
                                        __FUNCTION__, status);

                    return status;
                }
                break;
            } /* End of edge_state switch */
            break;  // SMP edge
        default:
            /* discovery edge is handled by base_discovered class, 
             * just return OK status to have it passed up.
             */
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
            break;
    }/* End of transport_id switch */

    /* 
     * A status of FBE_STATUS_MORE_PROCESSING_REQUIRED means that 
     * the state/attribute could not be handled here.
     * In most cases it will be handled by the superclass.  The calling function will
     * refer to the superclass in these cases.
     */
    return status;
} /* End of fbe_sas_enclosure_edge_state_change_event_entry */

/**************************************************************************
* fbe_sas_enclosure_build_inquiry_packet()                  
***************************************************************************
*
* DESCRIPTION
*       This function builds inquiry request subpacket, and fills in 
*  SCSI inquiry CDBs.
*
* PARAMETERS
*       p_object - The pointer to the fbe_base_object_t.
*       packet - The pointer to the fbe_packet_t.
*
* RETURN VALUES
*       subpacket created.
*
* NOTES
*
* HISTORY
*   09-Sept-2008 NChiu - Created.
***************************************************************************/
fbe_packet_t * 
fbe_sas_enclosure_build_inquiry_packet(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet)
{
    fbe_u8_t * buffer = NULL;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;


    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                        "%s entry\n", __FUNCTION__);
    

    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);

    if(payload_cdb_operation == NULL) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                            "%s fbe_payload_ex_allocate_cdb_operation fail\n", __FUNCTION__);

        return NULL;
    }

    buffer = fbe_transport_allocate_buffer();
    if(buffer == NULL) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                            "%s fbe_transport_allocate_buffer fail\n", __FUNCTION__);


        fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);

        return NULL;
    }

    sg_list = (fbe_sg_element_t  *)buffer;
    sg_list[0].count = FBE_SCSI_INQUIRY_DATA_SIZE;
    sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);
    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, FBE_SCSI_INQUIRY_DATA_SIZE);

    fbe_payload_cdb_build_inquiry(payload_cdb_operation, FBE_SAS_ENCLOSURE_INQUIRY_TIMEOUT);

    fbe_transport_set_completion_function(packet, sas_enclosure_send_inquiry_completion, sas_enclosure);
    
    return packet;
}

/*
 * This method is called when in the sas_enclosure is in the SPECIALIZED state and has not yet send inquiry
 * The sas_enclosure_send_inquiry creates a new io packet with inquiry CDB
 * and sends it to the ssp_edge.
 */
fbe_status_t 
fbe_sas_enclosure_send_inquiry(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet)
{
    fbe_packet_t * new_packet = NULL;
    fbe_status_t status;
    fbe_payload_ex_t *payload = NULL;

    payload = fbe_transport_get_payload_ex(packet);

    /* Make control operation allocated in monitor as the current operation. */
    status = fbe_payload_ex_increment_control_operation_level(payload);

    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    /* new_packet will point to packet itself */
    new_packet = fbe_sas_enclosure_build_inquiry_packet(sas_enclosure, packet);

    /* We need to fail the original request in case of an error */
    if (new_packet == NULL){
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                            "%s, fbe_transport_allocate_buffer failed\n", __FUNCTION__);

        

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_ssp_transport_send_functional_packet(&sas_enclosure->ssp_edge, new_packet);

    return status;
}

/*
 * This is the completion function for the inquiry packet sent to sas_enclosure.
 */
static fbe_status_t 
sas_enclosure_send_inquiry_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_enclosure_t * sas_enclosure = NULL;
    fbe_sg_element_t  * sg_list = NULL; 
    fbe_u8_t vendor_id[FBE_SCSI_INQUIRY_VENDOR_ID_SIZE + 1];
    fbe_u8_t product_id[FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE + 1];    
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_status_t status;  
    fbe_payload_cdb_scsi_status_t    scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD;        
    fbe_port_request_status_t payload_request_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
    fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
    fbe_payload_control_operation_t * payload_control_operation = NULL;

    sas_enclosure = (fbe_sas_enclosure_t *)context;

    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);


    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    /* bad scsi status and bad payload status will be translated to failed control status.
     * this will leave the condition untouched and retried in 3 sec.
     */
    if (status == FBE_STATUS_OK)
    {
        fbe_payload_cdb_get_scsi_status(payload_cdb_operation, &scsi_status);
        if(scsi_status == FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD) 
        {
            status = fbe_payload_cdb_get_request_status(payload_cdb_operation, &payload_request_status);

            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                    FBE_TRACE_LEVEL_INFO,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) sas_enclosure),
                                    "inquiry_completion, get_request_status failed 0x%x.\n", 
                                    status); 
                control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
            }
            else
            {
                switch (payload_request_status)
                {
                case FBE_PORT_REQUEST_STATUS_SUCCESS:
                case FBE_PORT_REQUEST_STATUS_DATA_UNDERRUN:
                    // ignore underrun
                    control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
                    break;
                default:
                    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                    FBE_TRACE_LEVEL_INFO,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) sas_enclosure),
                                    "inquiry_completion, payload status 0x%x.\n", 
                                    payload_request_status); 
                    // force the retry
                    control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
                    break;
                }
            }
        }
        else
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                    FBE_TRACE_LEVEL_INFO,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) sas_enclosure),
                                    "inquiry_completion, scsi status 0x%x.\n", 
                                    scsi_status); 
            // force the retry.
            control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
        }
    }

    /* only process the inq data when control status is OK */
    if (control_status == FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_zero_memory(vendor_id, FBE_SCSI_INQUIRY_VENDOR_ID_SIZE + 1);
        fbe_copy_memory(vendor_id, ((fbe_u8_t *)(sg_list[0].address)) + FBE_SCSI_INQUIRY_VENDOR_ID_OFFSET, FBE_SCSI_INQUIRY_VENDOR_ID_SIZE);          

        fbe_zero_memory(product_id, FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE + 1);
        fbe_copy_memory(product_id, ((fbe_u8_t *)(sg_list[0].address)) + FBE_SCSI_INQUIRY_PRODUCT_ID_OFFSET, FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE);

        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                            "%s,  vendor_id  = %s\n", 
                            __FUNCTION__ , vendor_id);

        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                            "%s, product_id  = %s\n", 
                            __FUNCTION__ , product_id);


        if(strncmp(product_id, "ESES Enclosure  ", strlen("ESES Enclosure  ")) == 0)
        {
            sas_enclosure->sasEnclosureProductType = FBE_SAS_ENCLOSURE_PRODUCT_TYPE_ESES;
        }
        else
        {
            sas_enclosure->sasEnclosureProductType = FBE_SAS_ENCLOSURE_PRODUCT_TYPE_INVALID;
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                            "%s unsupported enclosure product id: 0x%s\n", 
                            __FUNCTION__ , product_id);

        }
    }
    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
    fbe_transport_release_buffer(sg_list);

    /* Afer releasing the cdb operation, the control operation becomes the current operation. */
    payload_control_operation = fbe_payload_ex_get_control_operation(payload);

    if(payload_control_operation != NULL)
    {
        /* Set the control operation status. */
        fbe_payload_control_set_status(payload_control_operation, control_status);
    }

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_enclosure_send_get_element_list_command(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_packet_t * new_packet = NULL;

    new_packet = fbe_sas_enclosure_build_get_element_list_packet(sas_enclosure, packet);

    if (new_packet == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                            "%s, fbe_transport_allocate_packet failed\n", 
                            __FUNCTION__ );

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Note we can not send the packet right away.
        It is possible that blocking is set or queue depth is exeded.
        So, we should "submit" "send" the packet to the bouncer for evaluation.
        Right now we have no bouncer implemented, but as we will have it we will use it.
    */
    /* We are sending control packet via smp edge */
    status = fbe_sas_enclosure_send_smp_functional_packet(sas_enclosure, new_packet);

    return status;
}

/****************************************************************
 * fbe_sas_enclosure_send_get_element_list_completion()
 ****************************************************************
 * DESCRIPTION:
 *  This function handles the completion of get_element_list.
 *  It updates virtual phy address of the enclosure.
 *
 * PARAMETERS:
 *  packet - io packet pointer
 *  context - completion conext
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK 
 *                   - no error.
 *
 * HISTORY:
 *  8/15/08 - add comment, NCHIU
 *
 ****************************************************************/
static fbe_status_t 
fbe_sas_enclosure_send_get_element_list_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_enclosure_t * sas_enclosure = NULL;
    fbe_packet_t * master_packet = NULL;
    fbe_sg_element_t  * sg_list = NULL; 
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_smp_operation_t * payload_smp_operation = NULL;
    fbe_payload_smp_get_element_list_data_t * payload_smp_get_element_list_data = NULL;
    fbe_u32_t i;
    fbe_status_t status;
    fbe_payload_smp_status_t smp_status;
    fbe_payload_smp_status_qualifier_t smp_status_qualifier;

    sas_enclosure = (fbe_sas_enclosure_t *)context;

    master_packet = (fbe_packet_t *)fbe_transport_get_master_packet(packet);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);

    payload = fbe_transport_get_payload_ex(packet);
    if (payload != NULL)
    {
        payload_smp_operation = fbe_payload_ex_get_smp_operation(payload);
        if (payload_smp_operation == NULL)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
        if (sg_list == NULL)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    /* TO DO: we need to check on scsi status */
    if (status == FBE_STATUS_OK)
    {
        fbe_payload_smp_get_status(payload_smp_operation, &smp_status);
        /* element list is only valid when smp status is good */
        if (FBE_PAYLOAD_SMP_STATUS_OK == smp_status)
        {
            payload_smp_get_element_list_data = (fbe_payload_smp_get_element_list_data_t *)sg_list[0].address;

            /* I don't know if it's possible, but if we do get some bad stuff back.
            * let's not to process the return if that's the case
            */
            if (payload_smp_get_element_list_data->number_of_elements>=FBE_PAYLOAD_SMP_ELEMENT_LIST_SIZE)
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                    FBE_TRACE_LEVEL_INFO,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                    "%s, too many children (%d) \n", 
                                    __FUNCTION__ , payload_smp_get_element_list_data->number_of_elements);

                status = FBE_STATUS_GENERIC_FAILURE;
            }
            else
            {
                for(i = 0 ; i < payload_smp_get_element_list_data->number_of_elements; i++)
                {
                    /* Let's look if we got a virtual PHY */
                    if(payload_smp_get_element_list_data->element_list[i].element_type == FBE_PAYLOAD_SMP_ELEMENT_TYPE_VIRTUAL) 
                    {
                        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_DEBUG_LOW,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                            "%s, VIRTUAL PHY element %d sas_address %X.\n", 
                                            __FUNCTION__ , i, payload_smp_get_element_list_data->number_of_elements );

                        /* We will temporarily assume that enclosure number is the same as enclosure chain number */
                        if (0 == payload_smp_get_element_list_data->element_list[i].enclosure_chain_depth)
                        {
                            fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                                FBE_TRACE_LEVEL_ERROR,
                                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                                "%s, EnclosureDepth for SES is 0.  This should never be the case.\n", 
                                                __FUNCTION__ );

                            // update fault information and fail the enclosure
                            status = fbe_base_enclosure_handle_errors((fbe_base_enclosure_t *) sas_enclosure, 
                                                            0,
                                                            0,
                                                            FBE_ENCL_FLTSYMPT_INVALID_SES_ENCL_DEPTH,
                                                            payload_smp_get_element_list_data->element_list[i].enclosure_chain_depth);

                            // trace out the error if we can't handle the error
                            if(!ENCL_STAT_OK(status))
                            {
                                fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                                FBE_TRACE_LEVEL_ERROR,
                                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) sas_enclosure),
                                                "%s can't set FAIL condition, status: 0x%X\n",
                                                __FUNCTION__, status);
                            }
                        }
                        else
                        {
                            fbe_base_enclosure_set_my_position((fbe_base_enclosure_t *) sas_enclosure, 
                                                                    (payload_smp_get_element_list_data->element_list[i].enclosure_chain_depth -
                                                                    FBE_SAS_TOPOLOGY_DEPTH_ADJUSTMENT));

                            if(payload_smp_get_element_list_data->element_list[i].address.sas_address != sas_enclosure->ses_port_address)
                            {
                                /*
                                 * The address has changed, set the condition to reopen the edge.
                                 */

                                fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                                    "%s, setting OPEN SSP EDGE condition.\n", 
                                                    __FUNCTION__ );

                                
                                status = fbe_lifecycle_set_cond(&fbe_sas_enclosure_lifecycle_const,
                                                                (fbe_base_object_t*)sas_enclosure,
                                                                FBE_SAS_ENCLOSURE_LIFECYCLE_COND_OPEN_SSP_EDGE);

                                if (status != FBE_STATUS_OK) 
                                {
                                    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                                        FBE_TRACE_LEVEL_ERROR,
                                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                                        "%s, can't set OPEN SSP EDGE condition, status: 0x%x.\n",
                                                        __FUNCTION__, status);
                                }

                            }
                            
                            /* Save that virtual phy address */
                            sas_enclosure->ses_port_address = payload_smp_get_element_list_data->element_list[i].address.sas_address;
                            sas_enclosure->ses_port_address_generation_code = payload_smp_get_element_list_data->element_list[i].generation_code;
                        }
                    }
                }
            } // number_of_elements<FBE_BASE_SERVICE_MAX_ELEMENTS
        }
        else
        {
            /* bad smp status */
            fbe_payload_smp_get_status_qualifier(payload_smp_operation, &smp_status_qualifier);

            fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) sas_enclosure),
                                "%s: smp status not OK, qualifier: 0x%X\n",
                                __FUNCTION__, smp_status_qualifier);

            /* NO_DEVICE should trigger reset ride through, not failing enclosure */
            if (smp_status_qualifier==FBE_PAYLOAD_SMP_ELEMENT_STATUS_EXT_NO_DEVICE)
            {
                fbe_sas_enclosure_check_reset_ride_through_needed((fbe_base_object_t*)sas_enclosure);
                /* and we can ignore what we get from port object */
            }
            else
            {
                /* we need to fail the enclosure */
                status = fbe_base_enclosure_handle_errors((fbe_base_enclosure_t *)sas_enclosure,
                                                            FBE_ENCL_INVALID_COMPONENT_TYPE,
                                                            0,
                                                            FBE_ENCL_FLTSYMPT_MINIPORT_FAULT_ENCL,
                                                            smp_status_qualifier);
                // trace out the error if we can't handle the error
                if(!ENCL_STAT_OK(status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) sas_enclosure),
                                        "%s can't set FAIL condition, status: 0x%X\n",
                                        __FUNCTION__, status);
                }
            }
        }
    }
    /* We need to remove packet from master queue */
    fbe_transport_remove_subpacket(packet);

    if (payload_smp_operation != NULL)
    {
        fbe_payload_ex_release_smp_operation(payload, payload_smp_operation);
    }

    if (sg_list != NULL)
    {
        fbe_transport_release_buffer(sg_list);
    }

    fbe_transport_release_packet(packet);

    /* Remove master packet from the termination queue. */
    fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *)sas_enclosure, master_packet);

    fbe_transport_set_status(master_packet, status, 0);
    fbe_transport_complete_packet(master_packet);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_sas_enclosure_build_get_element_list_packet(
 *                    fbe_sas_enclosure_t * sas_enclosure, 
 *                    fbe_packet_t * packet)                  
 ****************************************************************
 * @brief
 *  This function build the packet to get children list through SMP edge.
 *
 * @param  sas_enclosure - our object context
 * @param  packet - request packet
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/20/08 - Added header. NC
 *
 ****************************************************************/
fbe_packet_t * 
fbe_sas_enclosure_build_get_element_list_packet(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_packet_t * new_packet = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_smp_operation_t * payload_smp_operation = NULL;
    fbe_u8_t * buffer = NULL;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_address_t address;

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                        "%s entry \n", __FUNCTION__ );

    /* Allocate packet */
    new_packet = fbe_transport_allocate_packet();
    if(new_packet == NULL) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                            "%s fbe_transport_allocate_packet fail\n", __FUNCTION__ );

        return NULL;
    }

    buffer = fbe_transport_allocate_buffer();
    if(buffer == NULL) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                            "%s fbe_transport_allocate_buffer fail\n", __FUNCTION__ );


        fbe_transport_release_packet(new_packet);
        return NULL;
    }

    fbe_transport_initialize_packet(new_packet);

    payload = fbe_transport_get_payload_ex(new_packet);
    payload_smp_operation = fbe_payload_ex_allocate_smp_operation(payload);
    
    address.sas_address = sas_enclosure->sasEnclosureSMPAddress;
    fbe_payload_smp_build_get_element_list(payload_smp_operation, 
                                           address, 
                                           sas_enclosure->smp_address_generation_code);

    /* Provide memory for the response */
    sg_list = (fbe_sg_element_t  *)buffer;
    sg_list[0].count = sizeof(fbe_payload_smp_get_element_list_data_t);
    sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);

    fbe_zero_memory(sg_list[0].address, sg_list[0].count);
    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);

    /* We need to assosiate newly allocated packet with original one */
    fbe_transport_add_subpacket(packet, new_packet);

    fbe_transport_set_cancel_function(packet, fbe_base_object_packet_cancel_function, (fbe_base_object_t *)sas_enclosure);
    fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)sas_enclosure, packet);

    status = fbe_transport_set_completion_function(new_packet, fbe_sas_enclosure_send_get_element_list_completion, sas_enclosure);

    return new_packet;
}

fbe_status_t 
fbe_sas_enclosure_discovery_transport_entry(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_payload_discovery_opcode_t discovery_opcode;
    fbe_status_t status;


    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                        "%s entry\n", __FUNCTION__ );


    payload = fbe_transport_get_payload_ex(packet);
    payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);
    fbe_payload_discovery_get_opcode(payload_discovery_operation, &discovery_opcode);
    
    switch(discovery_opcode){
#if 0
        case FBE_PAYLOAD_DISCOVERY_OPCODE_GET_PORT_OBJECT_ID:
            status = sas_enclosure_discovery_transport_get_port_object_id(sas_enclosure, packet);
            break;
#endif
        default:
            status = fbe_base_enclosure_discovery_transport_entry((fbe_base_enclosure_t *) sas_enclosure, packet);

            break;
    }

    return status;
}

