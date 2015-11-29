#include "base_enclosure_private.h"
#include "fbe_transport_memory.h"
#include "fbe_eses_enclosure_private.h"

static fbe_status_t fbe_base_enclosure_send_get_server_info_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_base_enclosure_discovery_transport_get_drive_location_info(fbe_base_enclosure_t * base_enclosure, fbe_packet_t * packet);
static fbe_status_t fbe_base_enclosure_discovery_transport_drive_spinup_ctrl(fbe_base_enclosure_t *base_enclosure, 
                                                                               fbe_packet_t *packet);

fbe_status_t 
fbe_base_enclosure_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context)
{
    fbe_base_enclosure_t * base_enclosure = NULL;
    fbe_status_t status;

    base_enclosure = (fbe_base_enclosure_t *)fbe_base_handle_to_pointer(object_handle);


    fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader(base_enclosure),
                        "%s entry\n", __FUNCTION__);

    /* We are not interested in any event at this time, so just forward it to the super class */
    status = fbe_base_discovering_event_entry(object_handle, event_type, event_context);

    return status; 
}

fbe_status_t 
fbe_base_enclosure_discovery_transport_entry(fbe_base_enclosure_t * base_enclosure, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_payload_discovery_opcode_t discovery_opcode;
    fbe_status_t status;

    fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader(base_enclosure),
                        "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);
    fbe_payload_discovery_get_opcode(payload_discovery_operation, &discovery_opcode);
    
    switch(discovery_opcode){
        case FBE_PAYLOAD_DISCOVERY_OPCODE_GET_DRIVE_LOCATION_INFO:
            status = fbe_base_enclosure_discovery_transport_get_drive_location_info(base_enclosure, packet);
            break;

        case FBE_PAYLOAD_DISCOVERY_OPCODE_GET_SPINUP_CREDIT:
        case FBE_PAYLOAD_DISCOVERY_OPCODE_RELEASE_SPINUP_CREDIT:
            status = fbe_base_enclosure_discovery_transport_drive_spinup_ctrl(base_enclosure, packet);
            break;

        default:
            status = fbe_base_discovering_discovery_transport_entry((fbe_base_discovering_t *) base_enclosure, packet);
            break;
    }

    return status;
}

/**************************************************************************
* fbe_base_enclosure_send_get_server_info_command()                  
***************************************************************************
*
* DESCRIPTION
*       Send IO block to get the server sas address and port number.
*
* PARAMETERS
*       base_enclosure - The pointer to the fbe_base_enclosure_t.
*       packet - The pointer to the fbe_packet_t.
*
* RETURN VALUES
*       fbe_status_t.
*
* NOTES
*
* HISTORY
*   15-Aug-2008 PHE - Created.
*   19-Sept-2008 NChiu - moved from viper enclosure
***************************************************************************/
fbe_status_t fbe_base_enclosure_send_get_server_info_command(fbe_base_enclosure_t * base_enclosure, 
                                                             fbe_packet_t *packet)
{
    fbe_status_t status;
    fbe_packet_t * new_packet = NULL;
    fbe_u8_t * buffer = NULL;
    fbe_sg_element_t  * sg_list = NULL; 
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_object_id_t         my_object_id;

    fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader(base_enclosure),
                        "%s entry \n", __FUNCTION__);

    /* Allocate packet */
    new_packet = fbe_transport_allocate_packet();
    if (new_packet == NULL){
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, fbe_transport_allocate_packet failed \n",
                            __FUNCTION__ );

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    buffer = fbe_transport_allocate_buffer(); // Allocate one chunk of memory.
    if (buffer == NULL){
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, fbe_transport_allocate_buffer failed \n",
                            __FUNCTION__ );

        fbe_transport_release_packet(new_packet);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    fbe_transport_initialize_packet(new_packet);

    payload = fbe_transport_get_payload_ex(new_packet);
    payload_discovery_operation = fbe_payload_ex_allocate_discovery_operation(payload);

    fbe_base_object_get_object_id((fbe_base_object_t *)base_enclosure, &my_object_id);
    /* Build the io block which requests the server sas address via the discovery edge. */
    fbe_payload_discovery_build_get_server_info(payload_discovery_operation, my_object_id);

    /* Provide memory for the responce */
    sg_list = (fbe_sg_element_t  *)buffer;
    sg_list[0].count = sizeof(fbe_payload_discovery_get_server_info_data_t);
    sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);

    /* We need to assosiate newly allocated packet with original one */
    fbe_transport_add_subpacket(packet, new_packet);

    fbe_transport_set_cancel_function(packet, fbe_base_object_packet_cancel_function, (fbe_base_object_t *) base_enclosure);
    fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)base_enclosure, packet);
  
    status = fbe_transport_set_completion_function(new_packet, fbe_base_enclosure_send_get_server_info_completion, base_enclosure);

    /* We are sending discovery packet via discovery edge */
    status = fbe_base_discovered_send_functional_packet((fbe_base_discovered_t *) base_enclosure, new_packet);

    return status;
}


/**************************************************************************
* fbe_base_enclosure_send_get_server_info_completion()                  
***************************************************************************
*
* DESCRIPTION
*       It is the completion function for "send get server info command"
*
* PARAMETERS
*       packet - The pointer to the fbe_packet_t.
*       context - The completion context.
*
* RETURN VALUES
*       fbe_status_t.
*
* NOTES
*
* HISTORY
*   15-Aug-2008 PHE - Created.
***************************************************************************/
static fbe_status_t fbe_base_enclosure_send_get_server_info_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_base_enclosure_t * base_enclosure = (fbe_base_enclosure_t *)context;
    fbe_sg_element_t  * sg_list = NULL; 
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_packet_t * master_packet = NULL;    
    fbe_payload_discovery_get_server_info_data_t *  payload_discovery_get_server_info_data = NULL;
    status = fbe_transport_get_status_code(packet);

    payload = fbe_transport_get_payload_ex(packet);
    payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);
    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    /* We HAVE to check status of packet before we continue */
    if(status != FBE_STATUS_OK){ /* Something bad happen to child, just release packet */
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s fbe_transport_get_status_code failed, status: 0x%x.\n",
                            __FUNCTION__, status);
        
    }
    else
    {
        /* The packet completes successfully.*/
        payload_discovery_get_server_info_data = (fbe_payload_discovery_get_server_info_data_t *)sg_list[0].address;

        base_enclosure->server_address = payload_discovery_get_server_info_data->address;
        base_enclosure->port_number = payload_discovery_get_server_info_data->port_number;
        base_enclosure->enclosure_number = payload_discovery_get_server_info_data->position;
        base_enclosure->component_id = payload_discovery_get_server_info_data->component_id;

        fbe_base_enclosure_set_logheader(base_enclosure);
    } 

    fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_LOW,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)base_enclosure),
                        "%s entry, Port %d, server addr %llX. \n",
                        __FUNCTION__, base_enclosure->port_number,
                        (unsigned long long)base_enclosure->server_address.sas_address);
    
    
    /* Get the master packet */
    master_packet = fbe_transport_get_master_packet(packet); 

    /* Release the memory chunk allocated for the sg_list. */
    fbe_transport_release_buffer(sg_list);

    fbe_payload_ex_release_discovery_operation(payload, payload_discovery_operation);

    /* Remove the packet from the  master_packet subpacket queue. */
    fbe_transport_remove_subpacket(packet);

    /* Release the memory chunk allocated for the packet. */
    fbe_transport_release_packet(packet);

    /* Remove master packet from the termination queue. */
    fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *)base_enclosure, master_packet);

    /* Set the status code for the master packet.*/
    fbe_transport_set_status(master_packet, status, 0);

    /* Complete the master packet.*/
    fbe_transport_complete_packet(master_packet);

    return FBE_STATUS_OK;
}

/****************************************************************
 * fbe_base_enclosure_discovery_transport_get_drive_location_info()
 ****************************************************************
 * DESCRIPTION:
 *  This function handles incoming requests on our discovery to get
 *  the port #, enclosure position,slot # and bank width.
 *
 * PARAMETERS:
 *  base_enclosure - our object context
 *  packet - request packet
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  16-June-2009 CLC - created. 
 *
 ****************************************************************/
static fbe_status_t 
fbe_base_enclosure_discovery_transport_get_drive_location_info(fbe_base_enclosure_t * base_enclosure, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_payload_discovery_get_drive_location_info_t * payload_discovery_get_drive_location_data = NULL;
    fbe_edge_index_t  slotIndex = 0;
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t slot_num = FBE_ENCLOSURE_VALUE_INVALID;

    fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader(base_enclosure),
                        "%s entry \n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);
    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    payload_discovery_get_drive_location_data = (fbe_payload_discovery_get_drive_location_info_t *)sg_list[0].address;
    
    // Get the enclosure local lcc side id.
    encl_status = fbe_eses_enclosure_get_local_lcc_side_id((fbe_eses_enclosure_t*)base_enclosure, 
                                     &(payload_discovery_get_drive_location_data->sp_id));
                                     
    if(encl_status != FBE_ENCLOSURE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s Unable to get sp_id from base enclosure, status: 0x%x.\n",
                            __FUNCTION__, encl_status);        
    }

    fbe_base_enclosure_get_port_number(base_enclosure, 
                                        &(payload_discovery_get_drive_location_data->port_number));

    fbe_base_enclosure_get_enclosure_number(base_enclosure, &(payload_discovery_get_drive_location_data->enclosure_number));

    payload_discovery_get_drive_location_data->bank_width = fbe_base_enclosure_get_number_of_slots_per_bank(base_enclosure);
    
    /* As a for now we have one client only with this particular id */
    status = fbe_base_discovering_get_server_index_by_client_id(
        (fbe_base_discovering_t *) base_enclosure, 
        payload_discovery_operation->command.common_command.client_id,
        &slotIndex);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s fbe_base_discovering_get_server_index_by_client_id fail, status: 0x%x.\n",
                            __FUNCTION__, status);        

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(slotIndex >= fbe_base_enclosure_get_number_of_slots(base_enclosure)) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader(base_enclosure),
                                "get_bus_enclosure_slot, invalid slot %d.\n", 
                                slotIndex);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    // get the slot number
    encl_status = fbe_base_enclosure_edal_getU8_thread_safe(base_enclosure,
                                                FBE_ENCL_DRIVE_SLOT_NUMBER,   // attribute
                                                FBE_ENCL_DRIVE_SLOT,        // Component type
                                                slotIndex,                       // drive component index
                                                &slot_num); 
    if (encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    payload_discovery_get_drive_location_data->slot_number = slot_num;

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}

/*!**************************************************************************
 * @fn fbe_base_enclosure_discovery_transport_drive_spinup_permit(
 *      fbe_eses_enclosure_t *eses_enclosure, 
 *      fbe_packet_t *packet)
 ***************************************************************************
 *
 * @brief
 *       Function to update the drive spinup permission related discovery edge attributes.
 * 
 * @param  base_enclosure - The object receiving the event
 * @param  packet - The pointer to the fbe_packet_t.
 *
 * @return
 *       fbe_status_t.
 *
 * NOTES
 *
 * HISTORY
 *   03-Dec-2009 PHE - Created.
 ***************************************************************************/
static fbe_status_t 
fbe_base_enclosure_discovery_transport_drive_spinup_ctrl(fbe_base_enclosure_t *base_enclosure, 
                                        fbe_packet_t *packet)
{
    fbe_payload_ex_t                       *payload = NULL;
    fbe_payload_discovery_operation_t   *payload_discovery_operation = NULL;    
    fbe_payload_discovery_opcode_t      discovery_opcode = FBE_PAYLOAD_DISCOVERY_OPCODE_INVALID;    
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_edge_index_t                    slot_num = 0;
    fbe_u32_t                           curr_discovery_path_attrs = 0;

    payload = fbe_transport_get_payload_ex(packet);
    payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);

    /* As a for now we have one client only with this particular id */
    status = fbe_base_discovering_get_server_index_by_client_id(
        (fbe_base_discovering_t *) base_enclosure, 
        payload_discovery_operation->command.common_command.client_id,
        &slot_num);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader(base_enclosure),
                                "drive_spinup_ctrl, get_server_index_by_client_id fail, client_id %d.\n",
                                payload_discovery_operation->command.common_command.client_id);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    if(slot_num >= fbe_base_enclosure_get_number_of_slots(base_enclosure)) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader(base_enclosure),
                                "drive_spinup_ctrl, invalid slot %d.\n", 
                                slot_num);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_discovery_get_opcode(payload_discovery_operation, &discovery_opcode);
    
    fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "drive_spinup_ctrl, discovery_opcode 0x%x, slot %d.\n", 
                            discovery_opcode, slot_num);

    status = fbe_base_discovering_get_path_attr((fbe_base_discovering_t *) base_enclosure, 
                                                slot_num, 
                                                &curr_discovery_path_attrs);
    if(status != FBE_STATUS_OK)
    {
        // It should not happen.
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                        FBE_TRACE_LEVEL_ERROR,
                        fbe_base_enclosure_get_logheader(base_enclosure),
                        "drive_spinup_ctrl, get_path_attr failed, slot: %d, status: 0x%x.\n",
                        slot_num, status); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
        
    }

    switch(discovery_opcode)
    {
        case FBE_PAYLOAD_DISCOVERY_OPCODE_GET_SPINUP_CREDIT:
            if(!(curr_discovery_path_attrs & FBE_DISCOVERY_PATH_ATTR_SPINUP_PENDING))
            {
                /* Set the attribute to indicate the request has been accepted.*/
                fbe_base_discovering_set_path_attr((fbe_base_discovering_t *) base_enclosure, 
                                                slot_num, 
                                                FBE_DISCOVERY_PATH_ATTR_SPINUP_PENDING);    
            }
            break;

        case FBE_PAYLOAD_DISCOVERY_OPCODE_RELEASE_SPINUP_CREDIT:
            if(curr_discovery_path_attrs & FBE_DISCOVERY_PATH_ATTR_SPINUP_PERMITTED)
            {
                /* Clear both attributes in case FBE_DISCOVERY_PATH_ATTR_SPINUP_PENDING
                 * got set again due to some timing window. 
                 */
                fbe_base_discovering_clear_path_attr((fbe_base_discovering_t *) base_enclosure, 
                                                slot_num, 
                                                (FBE_DISCOVERY_PATH_ATTR_SPINUP_PENDING |
                                                FBE_DISCOVERY_PATH_ATTR_SPINUP_PERMITTED));  

                base_enclosure->number_of_drives_spinningup--;
                
            }
            else
            {
                /* It should never happen. */
                fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader(base_enclosure),
                                "drive_spinup_ctrl, slot: %d, unexpected spinup perm returned.\n",
                                slot_num); 

                fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            break;

        default:
            fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader(base_enclosure),
                                    "drive_spinup_ctrl, slot: %d, unhandled discovery opcode: 0x%x.\n",
                                    slot_num, discovery_opcode);

            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_transport_set_status(packet, status, 0);
            fbe_transport_complete_packet(packet);
            return status;
            break;
    }// End of - switch(discovery_opcode)

    status = fbe_lifecycle_set_cond(&fbe_base_enclosure_lifecycle_const, 
                                        (fbe_base_object_t*)base_enclosure, 
                                        FBE_BASE_ENCLOSURE_LIFECYCLE_COND_DRIVE_SPINUP_CTRL_NEEDED);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader(base_enclosure),
                                "drive_spinup_ctrl, can't set DRIVE_SPINUP_CONTROL_NEEDED condition, status: 0x%x.\n",
                                status);
        
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return status;

}// End of function fbe_base_enclosure_discovery_transport_drive_spinup_ctrl

/* End of file fbe_base_enclosure_executer.c */


       

