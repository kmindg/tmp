/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_usurper.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the logical drive object code for the usurper.
 * 
 * @ingroup logical_drive_class_files
 * 
 * @version
 *   10/30/2007:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_logical_drive.h"
#include "fbe_logical_drive_private.h"
#include "fbe_logical_drive_io_command.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe/fbe_memory.h"
#include "fbe_imaging_structures.h"
#include "fbe_private_space_layout.h"

#define FBE_LDO_ZERO_OPERATION_MAX_BLK_COUNT   0x800  /*one chunk size*/


/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/

static fbe_status_t
fbe_ldo_get_attributes( fbe_logical_drive_t *const logical_drive_p, fbe_packet_t * const packet_p );
static fbe_status_t
fbe_ldo_set_attributes( fbe_logical_drive_t *const logical_drive_p, fbe_packet_t * const packet_p );
static fbe_status_t 
fbe_ldo_open_block_edge_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t 
fbe_ldo_attach_block_edge_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t 
fbe_ldo_detach_block_edge_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t 
fbe_ldo_get_object_id_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t 
fbe_ldo_get_block_edge_info(fbe_logical_drive_t * logical_drive, fbe_packet_t * packet_p);
static fbe_status_t 
fbe_ldo_get_identity_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);

static fbe_status_t logical_drive_get_drive_info(fbe_logical_drive_t * logical_drive, fbe_packet_t * packet_p);
static fbe_status_t logical_drive_attach_block_edge(fbe_logical_drive_t * logical_drive, fbe_packet_t * packet);
static fbe_status_t logical_drive_detach_block_edge(fbe_logical_drive_t * logical_drive, fbe_packet_t * packet);
static fbe_status_t fbe_logical_drive_set_edge_tap_hook(fbe_logical_drive_t * logical_drive_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_logical_drive_remove_edge_tap_hook(fbe_logical_drive_t * logical_drive_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_logical_drive_control_get_negotiate_info(fbe_logical_drive_t * logical_drive, fbe_packet_t * packet_p);
static fbe_status_t 
fbe_logical_drive_send_block_operation(fbe_logical_drive_t * logical_drive_p, 
                                     fbe_packet_t * packet_p);

static fbe_status_t fbe_logical_drive_generate_ica_stamp(fbe_logical_drive_t * logical_drive_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_logical_drive_generate_and_send_ica_stamp(fbe_logical_drive_t * logical_drive_p, fbe_packet_t * packet_p);

static fbe_status_t fbe_logical_drive_carve_memory_for_buffer_and_sgl(fbe_sg_element_t **sg_list_p, fbe_u8_t *write_buffer, fbe_bool_t preserve_ddbs);
static fbe_status_t fbe_logical_drive_generate_and_send_ica_stamp_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);

static fbe_status_t fbe_logical_drive_set_pvd_eol(fbe_logical_drive_t * logical_drive, fbe_packet_t * packet);
static fbe_status_t fbe_logical_drive_clear_pvd_eol(fbe_logical_drive_t * logical_drive, fbe_packet_t * packet);
static fbe_status_t fbe_logical_drive_read_ica_stamp(fbe_logical_drive_t * logical_drive_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_logical_drive_send_read_ica_stamp(fbe_logical_drive_t * logical_drive_p, fbe_packet_t * packet_p);

static fbe_status_t 
fbe_logical_drive_send_read_ica_stamp_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t fbe_logical_drive_clear_logical_errors(fbe_logical_drive_t * logical_drive_p, fbe_packet_t * packet_p);

static fbe_status_t 
fbe_logical_drive_zero_specific_region_continue_work(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t 
fbe_logical_drive_zero_specific_region(fbe_logical_drive_t * logical_drive_p, fbe_packet_t * packet_p, fbe_lba_t start_lba, fbe_block_count_t block_count);



/*!***************************************************************
 * fbe_logical_drive_control_entry()
 ****************************************************************
 * @brief
 *  This function is the management packet entry point for
 *  operations to the logical drive object.
 *
 * @param object_handle - The logical drive handle.
 * @param packet_p - The io packet that is arriving.
 *
 * @return fbe_status_t
 * 
 * @ref FBE_LOGICAL_DRIVE_CONTROL_CODE_GET_ATTRIBUTES
 * 
 * @author
 *  10/22/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_logical_drive_control_entry(fbe_object_handle_t object_handle, 
                                fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_logical_drive_control_code_t control_code;
    fbe_logical_drive_t * logical_drive_p = NULL;

    logical_drive_p = (fbe_logical_drive_t *)fbe_base_handle_to_pointer(object_handle);

    control_code = fbe_transport_get_control_code(packet_p);

    switch (control_code)
    {
        case FBE_LOGICAL_DRIVE_CONTROL_CODE_GET_ATTRIBUTES:
            /* Return our attributes.
             */
            status = fbe_ldo_get_attributes(logical_drive_p, packet_p);
            fbe_transport_set_status(packet_p, status, 0);
            fbe_ldo_complete_packet(packet_p);
            break;

        case FBE_LOGICAL_DRIVE_CONTROL_CODE_SET_ATTRIBUTES:
            /* Set our attributes.
             */
            status = fbe_ldo_set_attributes(logical_drive_p, packet_p);
            fbe_transport_set_status(packet_p, status, 0);
            fbe_ldo_complete_packet(packet_p);
            break;

        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO:
            status = fbe_ldo_get_block_edge_info(logical_drive_p, packet_p);
            break;

        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_NEGOTIATE_BLOCK_SIZE:
            status = fbe_logical_drive_control_get_negotiate_info(logical_drive_p, packet_p);
            break;

        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_SET_EDGE_TAP_HOOK:
            status = fbe_logical_drive_set_edge_tap_hook(logical_drive_p, packet_p);
            break;
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_REMOVE_EDGE_TAP_HOOK:
            status = fbe_logical_drive_remove_edge_tap_hook(logical_drive_p, packet_p);
            break;
#if 0
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_EXIT_POWER_SAVE:
            /*the request to get out of hibernation to the PDO would goes through us so we would act on it, and pass it down
            we would jump to activate and wait for the edge from the PDO to be enabled so we can get back to ready
            */
            fbe_lifecycle_set_cond(&fbe_logical_drive_lifecycle_const,
                                   (fbe_base_object_t*)logical_drive_p,
                                    FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE);

			status = fbe_base_discovered_control_entry(object_handle, packet_p);
			break;
#endif
        case FBE_LOGICAL_DRIVE_CONTROL_GET_DRIVE_INFO:
                status = logical_drive_get_drive_info(logical_drive_p, packet_p);
                break;

        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_ATTACH_EDGE:
            status = logical_drive_attach_block_edge(logical_drive_p, packet_p);			        
            break;

        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_DETACH_EDGE:
            status = logical_drive_detach_block_edge(logical_drive_p, packet_p);
            break;
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_BLOCK_COMMAND:
            status = fbe_logical_drive_send_block_operation(logical_drive_p, packet_p);
            break;
		case FBE_LOGICAL_DRIVE_CONTROL_GENERATE_ICA_STAMP:
			status = fbe_logical_drive_generate_ica_stamp(logical_drive_p, packet_p);
			break;
		case FBE_LOGICAL_DRIVE_CONTROL_READ_ICA_STAMP:
			status = fbe_logical_drive_read_ica_stamp(logical_drive_p, packet_p);
			break;
		case FBE_LOGICAL_DRIVE_CONTROL_SET_PVD_EOL:
			status = fbe_logical_drive_set_pvd_eol(logical_drive_p, packet_p);
			fbe_transport_set_status(packet_p, status, 0);
			fbe_ldo_complete_packet(packet_p);
			break;
		case FBE_LOGICAL_DRIVE_CONTROL_CLEAR_PVD_EOL:
			status = fbe_logical_drive_clear_pvd_eol(logical_drive_p, packet_p);
			fbe_transport_set_status(packet_p, status, 0);
			fbe_ldo_complete_packet(packet_p);
			break;
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_CLEAR_LOGICAL_ERRORS:
            status = fbe_logical_drive_clear_logical_errors(logical_drive_p, packet_p);
            break;
		default:
            /* Allow the base discovered object to handle all other ioctls.
             */
            status = fbe_base_discovered_control_entry(object_handle, packet_p);
            break;
    }
    return status;
}
/* end fbe_logical_drive_control_entry() */

/*!***************************************************************
 * fbe_ldo_get_attributes()
 ****************************************************************
 * @brief
 *  Return the attributes of the logical drive like exported
 *  and imported block sizes.
 * 
 * @param logical_drive_p - The logical drive handle.
 * @param packet_p - Packet for the get attributes call.
 *
 * @return FBE_STATUS_OK
 *
 * @author
 *  06/12/08 - Created. RPF
 *
 ****************************************************************/
static fbe_status_t
fbe_ldo_get_attributes( fbe_logical_drive_t *const logical_drive_p,
                        fbe_packet_t * const packet_p )
{
    fbe_logical_drive_attributes_t *attrib_p = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_u32_t index;

    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);   

    /* Translate all fields in the read capacity.
     */        
    fbe_payload_control_get_buffer(control_operation, &attrib_p); 

    attrib_p->imported_block_size = fbe_ldo_get_imported_block_size(logical_drive_p);    
	fbe_block_transport_edge_get_capacity(&logical_drive_p->block_edge, &attrib_p->imported_capacity);

    /* Copy the identify infos into the input buffer.
     */
    for (index = 0; index < FBE_BLOCK_TRANSPORT_CONTROL_IDENTIFY_LENGTH; index++)
    {
        attrib_p->initial_identify.identify_info[index] = logical_drive_p->identity.identify_info[index];
        attrib_p->last_identify.identify_info[index] = logical_drive_p->last_identity.identify_info[index];
    }

    /* Copy over the server information.
     */
    attrib_p->server_info.server_object_id = logical_drive_p->object_id;
    fbe_block_transport_server_get_number_of_clients(&logical_drive_p->block_transport_server,
                                                     &attrib_p->server_info.number_of_clients);

    /* ! @TODO
    attrib_p->server_info.b_optional_queued = (!fbe_queue_is_empty(&logical_drive_p->block_transport_server.priority_optional_queue_head));
    attrib_p->server_info.b_low_queued = (!fbe_queue_is_empty(&logical_drive_p->block_transport_server.priority_low_queue_head));
    attrib_p->server_info.b_normal_queued = (!fbe_queue_is_empty(&logical_drive_p->block_transport_server.priority_normal_queue_head));
    attrib_p->server_info.b_urgent_queued = (!fbe_queue_is_empty(&logical_drive_p->block_transport_server.priority_urgent_queue_head));
    */
    /* bts.attributes is redefined as fbe_atomic_t, but only lower 32bit is used.
     * Truncate to u32 here to make sure we don't break something.
     */
    attrib_p->server_info.attributes = (fbe_u32_t)logical_drive_p->block_transport_server.attributes;
    attrib_p->server_info.outstanding_io_count = (fbe_u32_t)(logical_drive_p->block_transport_server.outstanding_io_count & FBE_BLOCK_TRANSPORT_SERVER_GATE_MASK);
    attrib_p->server_info.outstanding_io_max = logical_drive_p->block_transport_server.outstanding_io_max;
    attrib_p->server_info.tag_bitfield = logical_drive_p->block_transport_server.tag_bitfield;
    return FBE_STATUS_OK;
}
/* end fbe_ldo_get_attributes */

/*!***************************************************************
 * fbe_ldo_set_attributes()
 ****************************************************************
 * @brief
 *  Allows a client to set the attributes of the drive.
 * 
 * @param packet_p - The packet for the set attributes.
 * @param logical_drive_p - The logical drive handle. 
 *
 * @return FBE_STATUS_OK
 *
 * @author
 *  06/12/08 - Created. RPF
 *
 ****************************************************************/
static fbe_status_t
fbe_ldo_set_attributes( fbe_logical_drive_t *const logical_drive_p,
                        fbe_packet_t * const packet_p )
{
    fbe_logical_drive_attributes_t *attrib_p = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);    
    
    /* Translate all fields in the read capacity.
     */        
    fbe_payload_control_get_buffer(control_operation, &attrib_p); 

    fbe_logical_drive_lock(logical_drive_p);

    /* Setup the imported and exported capacities.
     */
    fbe_ldo_set_imported_capacity(logical_drive_p, attrib_p->imported_capacity);

    /* We're done with updating the sizes, unlock now.
     */
    fbe_logical_drive_unlock(logical_drive_p);

    fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, 
                          FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_CREATE_OBJECT,
                          "logical drive: 0x%p %s Set attributes."
                          "imported block size: 0x%x imported capacity: 0x%llx\n", 
                          logical_drive_p, __FUNCTION__, 
                          attrib_p->imported_block_size,
                          (unsigned long long)attrib_p->imported_capacity);
    return FBE_STATUS_OK;
}
/* end fbe_ldo_set_attributes */

/*!**************************************************************
 * fbe_ldo_get_identity()
 ****************************************************************
 * @brief
 *  This function issues the identify command to the physical drive.
 *
 * @param  logical_drive_p - The logical drive to issue the request for.
 * @param  packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  7/2/2008 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_ldo_get_identity(fbe_logical_drive_t * const logical_drive_p, 
                                  fbe_packet_t * const packet_p)
{
    fbe_packet_t * new_packet_p = NULL;
    fbe_object_id_t port_object_id;
    fbe_status_t status;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p;
    fbe_u8_t * buffer = NULL;
    fbe_sg_element_t * sg_list = NULL;

    fbe_base_object_trace(  (fbe_base_object_t *)logical_drive_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    /* Get object id to send the packet to.
     */
    fbe_base_discovered_get_port_object_id((fbe_base_discovered_t *)logical_drive_p, &port_object_id);

    /* Allocate packet.
     */
    new_packet_p = fbe_transport_allocate_packet();
    if (new_packet_p == NULL)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)logical_drive_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "fbe_ldo_get_identity fbe_transport_allocate_packet failed\n");

        fbe_transport_set_status(packet_p, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Create the block command identify.
     */
    fbe_transport_initialize_packet(new_packet_p);
    payload_p = fbe_transport_get_payload_ex(new_packet_p);
    block_operation_p = fbe_payload_ex_allocate_block_operation(payload_p);
    status =  fbe_payload_block_build_identify(block_operation_p);

    /* Allocate buffer for identify device info */
    buffer = fbe_transport_allocate_buffer();

    if (buffer == NULL){
        fbe_base_object_trace(  (fbe_base_object_t *)logical_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, fbe_transport_allocate_buffer failed\n",
                                __FUNCTION__);

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        fbe_transport_release_buffer(buffer);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_list = (fbe_sg_element_t  *)buffer;    
    sg_list[0].count = FBE_BLOCK_TRANSPORT_CONTROL_IDENTIFY_LENGTH;
    sg_list[0].address = (fbe_u8_t*) &logical_drive_p->last_identity;    
    sg_list[1].count = 0;
    sg_list[1].address = NULL;  

    fbe_payload_ex_set_sg_list(payload_p, sg_list, 1);    

    /* We need to associate the new packet with original one.
     */
    fbe_transport_add_subpacket(packet_p, new_packet_p);

    /* Set our cancel and completion function.  Also put the master packet on 
     * the terminator queue. 
     */
    fbe_transport_set_cancel_function(packet_p, fbe_base_object_packet_cancel_function, (fbe_base_object_t *)logical_drive_p);
    fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)logical_drive_p, packet_p);
    status = fbe_transport_set_completion_function(new_packet_p, fbe_ldo_get_identity_completion, logical_drive_p);

    /* Send this functional packet to our block transport edge.
     */
    status = fbe_ldo_send_io_packet(logical_drive_p, new_packet_p);
    fbe_transport_release_buffer(buffer);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_ldo_get_identity()
 ******************************************/

/*!**************************************************************
 * fbe_ldo_get_identity_completion()
 ****************************************************************
 * @brief
 *  This function handles the completion of attaching the block edge.
 *
 * @param  packet_p - The packet requesting this operation.
 * @param  context - the completion context (the logical drive).
 *
 * @return status - The status of the operation.
 *
 * @author
 *  7/14/2008 - Created. RPF
 *
 ****************************************************************/
static fbe_status_t 
fbe_ldo_get_identity_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_logical_drive_t * logical_drive_p = NULL;
    fbe_packet_t * master_packet_p = NULL;
    fbe_status_t status;

    logical_drive_p = (fbe_logical_drive_t *)context;
    master_packet_p = (fbe_packet_t *)fbe_transport_get_master_packet(packet_p);

    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)logical_drive_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "get_identity failed status %X\n", status);
    }
    /* We need to remove packet from master queue.
     */
    fbe_transport_remove_subpacket(packet_p);

    /* Copy the status back to the master.
     */
    fbe_transport_copy_packet_status(packet_p, master_packet_p);

    /* Free the resources we allocated previously.
     */
    fbe_transport_release_packet(packet_p);

    /* Remove master packet from the termination queue.
     */
    fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *)logical_drive_p, master_packet_p);

    /* Complete the original packet.
     */
    fbe_transport_complete_packet(master_packet_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_ldo_get_identity_completion()
 **************************************/

/*!**************************************************************
 * fbe_ldo_send_get_object_id_command()
 ****************************************************************
 * @brief
 *  This function gets the object id of the physical drive.
 *
 * @param  logical_drive_p - The logical drive to issue the request for.
 * @param  packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  7/14/2008 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_ldo_send_get_object_id_command(fbe_logical_drive_t * logical_drive_p, 
                                   fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_packet_t * new_packet_p = NULL;
	fbe_payload_ex_t * payload = NULL;
	fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_u8_t * buffer = NULL;
    fbe_sg_element_t  * sg_list = NULL; 

    fbe_base_object_trace((fbe_base_object_t*)logical_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Allocate packet.
     * If this fails, then we will return an error since we cannot continue 
     * without a packet. 
     */
    new_packet_p = fbe_transport_allocate_packet();
    if (new_packet_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s fbe_transport_allocate_packet fail", __FUNCTION__);

        fbe_transport_set_status(packet_p, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Allocate buffer.
     * If this fails, then we will return an error since we cannot continue 
     * without a packet. 
     */
    buffer = fbe_transport_allocate_buffer();
    if (buffer == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s fbe_transport_allocate_buffer fail", __FUNCTION__);

        fbe_transport_release_packet(new_packet_p);
        fbe_transport_set_status(packet_p, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Build the get port id operation.
     */
    fbe_transport_initialize_packet(new_packet_p);
	payload = fbe_transport_get_payload_ex(new_packet_p);
	payload_discovery_operation = fbe_payload_ex_allocate_discovery_operation(payload);
    fbe_payload_discovery_build_get_port_object_id(payload_discovery_operation);

    /* Provide memory for the response.
     */
    sg_list = (fbe_sg_element_t  *)buffer;
    sg_list[0].count = sizeof(fbe_payload_discovery_get_port_object_id_data_t);
    sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

	fbe_payload_ex_set_sg_list(payload, sg_list, 1);

    /* We need to associate the new packet with original one.
     */
    fbe_transport_add_subpacket(packet_p, new_packet_p);

    fbe_transport_set_cancel_function(packet_p, fbe_base_object_packet_cancel_function, (fbe_base_object_t *)logical_drive_p);
    fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)logical_drive_p, packet_p);

    status = fbe_transport_set_completion_function(new_packet_p, fbe_ldo_get_object_id_completion, logical_drive_p);

    /* We are sending discovery packet via discovery edge.
     */
    status = fbe_ldo_send_discovery_packet((fbe_logical_drive_t *) logical_drive_p, new_packet_p);

    return status;
}
/**************************************
 * end fbe_ldo_send_get_object_id_command()
 **************************************/

/*!**************************************************************
 * fbe_ldo_get_object_id_completion()
 ****************************************************************
 * @brief
 *  This function handles the completion of the request to fetch
 *  the object id of the physical drive.
 *
 * @param  packet_p - The packet requesting this operation.
 * @param  context - the completion context (the logical drive).
 *
 * @return status - The status of the operation.
 *
 * @author
 *  7/14/2008 - Created. RPF
 *
 ****************************************************************/
static fbe_status_t 
fbe_ldo_get_object_id_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_logical_drive_t * logical_drive_p = NULL;
    fbe_packet_t * master_packet_p = NULL;
    fbe_sg_element_t  * sg_list = NULL; 
	fbe_payload_ex_t * payload = NULL;
	fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_payload_discovery_get_port_object_id_data_t * payload_discovery_get_port_object_id_data = NULL;
    fbe_status_t status;

    logical_drive_p = (fbe_logical_drive_t *)context;

    master_packet_p = (fbe_packet_t *)fbe_transport_get_master_packet(packet_p);
	payload = fbe_transport_get_payload_ex(packet_p);
	payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);
	fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);


    payload_discovery_get_port_object_id_data = (fbe_payload_discovery_get_port_object_id_data_t *)sg_list[0].address;


    /* Only clear the condition if the packet completion status is OK.
     */
    status = fbe_transport_get_status_code(packet_p);
    if (status == FBE_STATUS_OK)
    {
        fbe_ldo_set_object_id(logical_drive_p,
                              payload_discovery_get_port_object_id_data->port_object_id);

        fbe_base_object_trace((fbe_base_object_t*)logical_drive_p,
                              FBE_TRACE_LEVEL_DEBUG_HIGH,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s port_object_id = %X\n", __FUNCTION__, 
                              fbe_ldo_get_object_id(logical_drive_p));
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)logical_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "logical drive %s failed status = 0x%x\n", 
                              __FUNCTION__, status);
    } /* end if status is not OK */

    /* We need to remove packet from master queue.
     */
    fbe_transport_remove_subpacket(packet_p);

	fbe_payload_ex_release_discovery_operation(payload, payload_discovery_operation);
    fbe_transport_release_buffer(sg_list);
    fbe_transport_release_packet(packet_p);

    /* Remove master packet from the termination queue.
     */
    fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *)logical_drive_p, master_packet_p);

    if (status == FBE_STATUS_OK)
    {
        /* If the status is good, we can continue.
         */
        fbe_ldo_attach_block_edge(logical_drive_p, master_packet_p);
    }
    else
    {
        /* Complete the master packet.
         */
        fbe_transport_set_status(master_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(master_packet_p);
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

}
/**************************************
 * end fbe_ldo_get_object_id_completion()
 **************************************/

/*!**************************************************************
 * fbe_ldo_attach_block_edge() 
 ****************************************************************
 * @brief
 *  This function attaches the block edge.
 *
 * @param  logical_drive_p - The logical drive to issue the request for.
 * @param  packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  7/14/2008 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_ldo_attach_block_edge(fbe_logical_drive_t *logical_drive_p, fbe_packet_t * packet_p) 
{
    fbe_block_transport_control_attach_edge_t *block_transport_control_attach_edge_p; 
    fbe_packet_t * new_packet_p = NULL;
    fbe_path_state_t path_state;
    fbe_object_id_t my_object_id;
    fbe_object_id_t object_id;
    fbe_u8_t * buffer = NULL;
    fbe_status_t status;
	fbe_package_id_t my_package_id;

    object_id = fbe_ldo_get_object_id(logical_drive_p);

    fbe_base_object_trace(  (fbe_base_object_t *)logical_drive_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    /* Build the edge. (We have to perform some sanity checking here).
     */
    fbe_block_transport_get_path_state(&logical_drive_p->block_edge, &path_state);

    /* Perform some sanity checking on the path state here.
     */
    if (path_state != FBE_PATH_STATE_INVALID)
    {    
        /* Discovery edge may be connected only once.
         */
        fbe_base_object_trace(  (fbe_base_object_t *)logical_drive_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                                "path_state %X\n",path_state);

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_block_transport_set_transport_id(&logical_drive_p->block_edge);
    fbe_base_object_get_object_id((fbe_base_object_t *)logical_drive_p, &my_object_id);
    fbe_block_transport_set_server_id(&logical_drive_p->block_edge, object_id);
    fbe_block_transport_set_client_id(&logical_drive_p->block_edge, my_object_id);
    fbe_block_transport_set_client_index(&logical_drive_p->block_edge, 0);    /* We have only one block edge */

	fbe_get_package_id(&my_package_id);
	fbe_block_transport_edge_set_client_package_id(&logical_drive_p->block_edge, my_package_id);

    /* Allocate packet.
     */
    new_packet_p = fbe_transport_allocate_packet();
    if (new_packet_p == NULL)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)logical_drive_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "fbe_transport_allocate_packet failed\n");

        fbe_transport_set_status(packet_p, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Allocate buffer.
     * If this fails, then we will return an error since we cannot continue 
     * without a packet. 
     */
    buffer = fbe_transport_allocate_buffer();
    if (buffer == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s fbe_transport_allocate_buffer fail", __FUNCTION__);

        fbe_transport_release_packet(new_packet_p);
        fbe_transport_set_status(packet_p, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Get the pointer to the structure we will send to the physical drive. 
     * Fill it in with the edge we are sending. 
     */
    block_transport_control_attach_edge_p = (fbe_block_transport_control_attach_edge_t *)buffer;
    block_transport_control_attach_edge_p->block_edge = &logical_drive_p->block_edge;

    /* Build the attach edge control packet.
     */
    fbe_transport_initialize_packet(new_packet_p);
	/*
    io_block = fbe_transport_get_io_block(new_packet_p);
    status =  fbe_io_block_build_control(io_block, 
                                         FBE_BLOCK_TRANSPORT_CONTROL_CODE_ATTACH_EDGE, 
                                         block_transport_control_attach_edge_p, 
                                         sizeof(fbe_block_transport_control_attach_edge_t));
	*/
	fbe_transport_build_control_packet(new_packet_p, 
									FBE_BLOCK_TRANSPORT_CONTROL_CODE_ATTACH_EDGE,
									block_transport_control_attach_edge_p,
									sizeof(fbe_block_transport_control_attach_edge_t),
									sizeof(fbe_block_transport_control_attach_edge_t),
									NULL,
									NULL);

    /* We need to associate the new packet with original one.
     */
    fbe_transport_add_subpacket(packet_p, new_packet_p);

    /* Set our cancel and completion function.  Also put the master packet on 
     * the terminator queue. 
     */
    fbe_transport_set_cancel_function(packet_p, fbe_base_object_packet_cancel_function, (fbe_base_object_t *)logical_drive_p);
    fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)logical_drive_p, packet_p);
    status = fbe_transport_set_completion_function(new_packet_p, fbe_ldo_attach_block_edge_completion, logical_drive_p);

    /* We are sending control packet, so we have to form address manually.
     */
    fbe_transport_set_address(  new_packet_p,
                                FBE_PACKAGE_ID_PHYSICAL,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                object_id);

    /* Control packets should be sent via service manager.
     */
    status = fbe_ldo_send_control_packet(logical_drive_p, new_packet_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_ldo_attach_block_edge()
 **************************************/

/*!**************************************************************
 * fbe_ldo_attach_block_edge_completion()
 ****************************************************************
 * @brief
 *  This function handles the completion of attaching the block edge.
 *
 * @param  packet_p - The packet requesting this operation.
 * @param  context - the completion context (the logical drive).
 *
 * @return status - The status of the operation.
 *
 * @author
 *  7/14/2008 - Created. RPF
 *
 ****************************************************************/
static fbe_status_t 
fbe_ldo_attach_block_edge_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_logical_drive_t * logical_drive_p = NULL;
    fbe_packet_t * master_packet_p = NULL;
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_buffer_t control_buffer;
    fbe_lba_t capacity;
    fbe_block_edge_geometry_t block_edge_geometry;

    logical_drive_p = (fbe_logical_drive_t *)context;
    master_packet_p = (fbe_packet_t *)fbe_transport_get_master_packet(packet_p);

    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)logical_drive_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "logical_drive_attach_edge failed status %X\n", status);
    }
    else
    {
	fbe_block_transport_edge_get_geometry(&logical_drive_p->block_edge, &block_edge_geometry);
	fbe_block_transport_edge_get_capacity(&logical_drive_p->block_edge, &capacity);

	switch(block_edge_geometry){
            case FBE_BLOCK_EDGE_GEOMETRY_512_512:
                capacity = (capacity * 512) / 520;	
                break;
            case FBE_BLOCK_EDGE_GEOMETRY_520_520:
                break;
            case FBE_BLOCK_EDGE_GEOMETRY_4096_4096:
                capacity = (capacity * 4096) / 520;	
                break;
            case FBE_BLOCK_EDGE_GEOMETRY_4160_4160:
                capacity = (capacity * 4160) / 520;	
                break;
            default:
                fbe_base_object_trace((fbe_base_object_t *) logical_drive_p, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s Invalid block_edge_geometry %d \n", __FUNCTION__, block_edge_geometry);
                capacity = 0;
	}
        fbe_block_transport_server_set_capacity(&logical_drive_p->block_transport_server, 
                                                capacity);
    }
    /* We need to remove packet from master queue.
     */
    fbe_transport_remove_subpacket(packet_p);

    /* Copy the status back to the master.
     */
    fbe_transport_copy_packet_status(packet_p, master_packet_p);

    /* Free the resources we allocated previously.
     */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);    
    fbe_payload_control_get_buffer(control_operation, &control_buffer);
    fbe_transport_release_buffer(control_buffer);
    fbe_transport_release_packet(packet_p);

    /* Remove master packet from the termination queue.
     */
    fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *)logical_drive_p, master_packet_p);

    /* Complete the original packet.
     */
    fbe_transport_complete_packet(master_packet_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_ldo_drive_attach_block_edge_completion()
 **************************************/

/*!**************************************************************
 * fbe_ldo_open_block_edge()
 ****************************************************************
 * @brief
 *  This function opens the block edge.
 *
 * @param  logical_drive_p - The logical drive to issue the request for.
 * @param  packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  7/14/2008 - Created. RPF
 *
 ****************************************************************/
fbe_status_t
fbe_ldo_open_block_edge(fbe_logical_drive_t * logical_drive_p, fbe_packet_t * packet_p)
{
    /* Sent to the logical_drive object to open the edge.
     */
    fbe_block_transport_control_open_edge_t *block_transport_control_open_edge_p = NULL;
    fbe_packet_t * new_packet_p = NULL;
    /*fbe_io_block_t * io_block = NULL;*/
    fbe_u8_t * buffer = NULL;
    fbe_status_t status;

    fbe_base_object_trace(  (fbe_base_object_t *)logical_drive_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    /* Allocate packet */
    new_packet_p = fbe_transport_allocate_packet();
    if (new_packet_p == NULL)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)logical_drive_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "fbe_transport_allocate_packet failed\n");

        fbe_transport_set_status(packet_p, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Allocate buffer.
     * If this fails, then we will return an error since we cannot continue 
     * without a packet. 
     */
    buffer = fbe_transport_allocate_buffer();
    if (buffer == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s fbe_transport_allocate_buffer fail", __FUNCTION__);

        fbe_transport_release_packet(new_packet_p);
        fbe_transport_set_status(packet_p, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Note that currently there is nothing in this structure for us to set. 
     * However, it must be filled with memory since the receiver checks for 
     * NULL. 
     */
    block_transport_control_open_edge_p = (fbe_block_transport_control_open_edge_t *) buffer;

    /* Build our control packet.
     */
    fbe_transport_initialize_packet(new_packet_p);
	/*
    io_block = fbe_transport_get_io_block(new_packet_p);
    status =  fbe_io_block_build_control(io_block, 
                                         FBE_BLOCK_TRANSPORT_CONTROL_CODE_OPEN_EDGE, 
                                         block_transport_control_open_edge_p, 
                                         sizeof(fbe_block_transport_control_open_edge_t));
	*/
	fbe_transport_build_control_packet(new_packet_p, 
										FBE_BLOCK_TRANSPORT_CONTROL_CODE_OPEN_EDGE,
										block_transport_control_open_edge_p,
										sizeof(fbe_block_transport_control_open_edge_t),
										sizeof(fbe_block_transport_control_open_edge_t),
										NULL,
										NULL);

    /* We need to associate the new packet with original one.
     */
    fbe_transport_add_subpacket(packet_p, new_packet_p);

    /* Set our cancel and completion function.  Also put the master packet on 
     * the terminator queue. 
     */
    fbe_transport_set_cancel_function(packet_p, fbe_base_object_packet_cancel_function, (fbe_base_object_t *)logical_drive_p);
    fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)logical_drive_p, packet_p);
    status = fbe_transport_set_completion_function(new_packet_p, fbe_ldo_open_block_edge_completion, logical_drive_p);

    /* Send the packet on the block edge.
     */
    fbe_block_transport_send_control_packet(&logical_drive_p->block_edge, new_packet_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_ldo_open_block_edge()
 **************************************/

/*!**************************************************************
 * fbe_ldo_open_block_edge_completion()
 ****************************************************************
 * @brief
 *  This function handles the completion of opening the block edge.
 *
 * @param  packet_p - The packet requesting this operation.
 * @param  context - the completion context (the logical drive).
 *
 * @return status - The status of the operation.
 *
 * @author
 *  7/14/2008 - Created. RPF
 *
 ****************************************************************/
static fbe_status_t 
fbe_ldo_open_block_edge_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_logical_drive_t * logical_drive_p = NULL;
    fbe_packet_t * master_packet_p = NULL;
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_buffer_t control_buffer;

    logical_drive_p = (fbe_logical_drive_t *)context;
    master_packet_p = (fbe_packet_t *)fbe_transport_get_master_packet(packet_p);

    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)logical_drive_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "logical_drive_open_block_edge failed status %X\n", status);
    }

    /* We need to remove packet from master queue.
     */
    fbe_transport_remove_subpacket(packet_p);

    /* Copy the status back to the master.
     */
    fbe_transport_copy_packet_status(packet_p, master_packet_p);

    /* Free the resources we allocated previously.
     */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);    
    fbe_payload_control_get_buffer(control_operation, &control_buffer);    
    fbe_transport_release_buffer(control_buffer);
    fbe_transport_release_packet(packet_p);

    /* Remove master packet from the termination queue.
     */
    fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *)logical_drive_p, master_packet_p);

    /* Complete the original packet.
     */
    fbe_transport_complete_packet(master_packet_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_ldo_open_block_edge_completion()
 **************************************/

/*!**************************************************************
 * fbe_ldo_detach_block_edge()
 ****************************************************************
 * @brief
 *  This function detaches the block edge from the physical drive.
 *
 * @param  logical_drive_p - The logical drive to issue the request for.
 * @param  packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  7/14/2008 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_ldo_detach_block_edge(fbe_logical_drive_t * logical_drive_p, fbe_packet_t * packet_p)
{
    /* This structure is sent to the server object to detach the edge.
     */
    fbe_block_transport_control_detach_edge_t *block_transport_control_detach_edge_p = NULL;
    fbe_packet_t * new_packet_p = NULL;
    /* fbe_io_block_t * io_block = NULL; */
    fbe_u8_t * buffer = NULL;
    fbe_status_t status;
	fbe_path_state_t path_state;


    fbe_base_object_trace(  (fbe_base_object_t *)logical_drive_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    status = fbe_block_transport_get_path_state(&logical_drive_p->block_edge, &path_state);
    /* If we don't have an edge to detach, complete the packet and return Good status. */
    if(status != FBE_STATUS_OK || path_state == FBE_PATH_STATE_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t*)logical_drive_p,
                                 FBE_TRACE_LEVEL_DEBUG_LOW,
                                 FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                                 "%s Block edge is Invalid. No need to detach it.\n", __FUNCTION__);

        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    /* Allocate packet */
    new_packet_p = fbe_transport_allocate_packet();
    if (new_packet_p == NULL)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)logical_drive_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "fbe_transport_allocate_packet failed\n");
        fbe_transport_set_status(packet_p, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Allocate buffer.
     * If this fails, then we will return an error since we cannot continue 
     * without a packet. 
     */
    buffer = fbe_transport_allocate_buffer();
    if (buffer == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s fbe_transport_allocate_buffer fail", __FUNCTION__);

        fbe_transport_release_packet(new_packet_p);
        fbe_transport_set_status(packet_p, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Get the pointer to the structure to send and fill it in with the edge.
     */
    block_transport_control_detach_edge_p = (fbe_block_transport_control_detach_edge_t *) buffer;
    block_transport_control_detach_edge_p->block_edge = &logical_drive_p->block_edge;

    /* Build our control packet.
     */
    fbe_transport_initialize_packet(new_packet_p);
	/*
    io_block = fbe_transport_get_io_block(new_packet_p);
    status =  fbe_io_block_build_control(io_block, 
                                         FBE_BLOCK_TRANSPORT_CONTROL_CODE_DETACH_EDGE, 
                                         block_transport_control_detach_edge_p, 
                                         sizeof(fbe_block_transport_control_detach_edge_t));
	*/

	fbe_transport_build_control_packet(new_packet_p, 
									FBE_BLOCK_TRANSPORT_CONTROL_CODE_DETACH_EDGE,
									block_transport_control_detach_edge_p,
									sizeof(fbe_block_transport_control_detach_edge_t),
									sizeof(fbe_block_transport_control_detach_edge_t),
									NULL,
									NULL);

    /* We need to associate the new packet with original one.
     */
    fbe_transport_add_subpacket(packet_p, new_packet_p);

    /* Since this is a control packet we need to set the address.
     */
    fbe_transport_set_address(new_packet_p,
                              FBE_PACKAGE_ID_PHYSICAL,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              fbe_ldo_get_object_id(logical_drive_p));

    /* Set our cancel and completion function.  Also put the master packet on 
     * the terminator queue. 
     */
    fbe_transport_set_cancel_function(packet_p, fbe_base_object_packet_cancel_function, (fbe_base_object_t *)logical_drive_p);
    fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)logical_drive_p, packet_p);
    status = fbe_transport_set_completion_function(new_packet_p, fbe_ldo_detach_block_edge_completion, logical_drive_p);

    fbe_ldo_send_control_packet(logical_drive_p, new_packet_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_ldo_detach_block_edge()
 **************************************/

/*!**************************************************************
 * fbe_ldo_detach_block_edge_completion()
 ****************************************************************
 * @brief
 *  This function handles the completion of detaching the block edge.
 *
 * @param  packet_p - The packet requesting this operation.
 * @param  context - the completion context (the logical drive).
 *
 * @return status - The status of the operation.
 *
 * @author
 *  7/14/2008 - Created. RPF
 *
 ****************************************************************/
static fbe_status_t 
fbe_ldo_detach_block_edge_completion(fbe_packet_t * packet_p, 
                                     fbe_packet_completion_context_t context)
{
    fbe_logical_drive_t * logical_drive_p = NULL;
    fbe_packet_t * master_packet_p = NULL;
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_buffer_t control_buffer;

    logical_drive_p = (fbe_logical_drive_t *)context;
    master_packet_p = (fbe_packet_t *)fbe_transport_get_master_packet(packet_p);

    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)logical_drive_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "logical_drive_open_block_edge failed status %X\n", status);
    }

    /* We need to remove packet from master queue.
     */
    fbe_transport_remove_subpacket(packet_p);

    /* Copy the status back to the master.
     */
    fbe_transport_copy_packet_status(packet_p, master_packet_p);

    /* Free the resources we allocated previously.
     */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);    
    fbe_payload_control_get_buffer(control_operation, &control_buffer); 
    fbe_transport_release_buffer(control_buffer);
    fbe_transport_release_packet(packet_p);

    /* Remove master packet from the termination queue.
     */
    fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *)logical_drive_p, master_packet_p);

    /* Complete the original packet.
     */
    fbe_transport_complete_packet(master_packet_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_ldo_detach_block_edge_completion
 **************************************/

/*!**************************************************************
 * fbe_ldo_send_control_packet()
 ****************************************************************
 * @brief
 *  This function issues a control packet.
 *
 * @param logical_drive_p - The logical drive to issue the request for.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  7/14/2008 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_ldo_send_control_packet(fbe_logical_drive_t * logical_drive_p, 
                            fbe_packet_t * packet_p)
{
    fbe_status_t status;

    status = fbe_service_manager_send_control_packet(packet_p);
    return status;
}
/**************************************
 * end fbe_ldo_send_control_packet()
 **************************************/

/*!**************************************************************
 * fbe_ldo_send_discovery_packet()
 ****************************************************************
 * @brief
 *  This function issues a discovery packet on the discovery edge.
 *
 * @param logical_drive_p - The logical drive to issue the request for.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  7/14/2008 - Created. RPF
 *
 ****************************************************************/

fbe_status_t 
fbe_ldo_send_discovery_packet(fbe_logical_drive_t * logical_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;

    status = fbe_base_discovered_send_functional_packet((fbe_base_discovered_t *)logical_drive_p, packet_p);

    return status;
}
/**************************************
 * end fbe_ldo_send_discovery_packet()
 **************************************/

/*!**************************************************************
 * fbe_ldo_get_block_edge_info()
 ****************************************************************
 * @brief
 *  This function returns the edge info for our block edge.
 *
 * @param logical_drive_p - The logical drive to get edge info for.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  8/15/2008 - Created. Peter Puhov.
 *
 ****************************************************************/
static fbe_status_t
fbe_ldo_get_block_edge_info(fbe_logical_drive_t * logical_drive, fbe_packet_t * packet_p)
{
    fbe_block_transport_control_get_edge_info_t * get_edge_info = NULL;    /* INPUT */
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;  

    fbe_base_object_trace(  (fbe_base_object_t *)logical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &get_edge_info);
    if (get_edge_info == NULL)
    {

        fbe_base_object_trace(  (fbe_base_object_t *)logical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "fbe_payload_control_get_buffer failed\n");

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_block_transport_get_client_id(&logical_drive->block_edge, &get_edge_info->base_edge_info.client_id);
    status = fbe_block_transport_get_server_id(&logical_drive->block_edge, &get_edge_info->base_edge_info.server_id);

    status = fbe_block_transport_get_client_index(&logical_drive->block_edge, &get_edge_info->base_edge_info.client_index);
    status = fbe_block_transport_get_server_index(&logical_drive->block_edge, &get_edge_info->base_edge_info.server_index);

    status = fbe_block_transport_get_path_state(&logical_drive->block_edge, &get_edge_info->base_edge_info.path_state);
    status = fbe_block_transport_get_path_attributes(&logical_drive->block_edge, &get_edge_info->base_edge_info.path_attr);
    
    fbe_block_transport_edge_get_exported_block_size(&logical_drive->block_edge, &get_edge_info->exported_block_size);
    fbe_block_transport_get_physical_block_size(logical_drive->block_edge.block_edge_geometry, &get_edge_info->imported_block_size);
    fbe_block_transport_edge_get_capacity(&logical_drive->block_edge, &get_edge_info->capacity);
    get_edge_info->offset = fbe_block_transport_edge_get_offset(&logical_drive->block_edge);

    get_edge_info->base_edge_info.transport_id = FBE_TRANSPORT_ID_BLOCK;

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_ldo_get_block_edge_info()
 **************************************/
/*!**************************************************************
 * fbe_logical_drive_control_get_negotiate_info()
 ****************************************************************
 * @brief
 *  This function returns negotiate info via a control packet.
 *
 * @param logical_drive_p - The logical drive to get negotiate info for.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  5/20/2010 - Created. Swati Fursule
 *
 ****************************************************************/
static fbe_status_t
fbe_logical_drive_control_get_negotiate_info(fbe_logical_drive_t * logical_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_block_transport_negotiate_t* negotiate_block_size_p = NULL;

    fbe_base_object_trace(  (fbe_base_object_t *)logical_drive_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &negotiate_block_size_p);
    if (negotiate_block_size_p == NULL)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)logical_drive_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* See if we can calculate the negotiate information.
     */
    status = fbe_ldo_calculate_negotiate_info( logical_drive_p, negotiate_block_size_p  );

    /* Transport status is set based on the status returned from calculating.
     */
    fbe_transport_set_status(packet_p, status, 0);

    if (status != FBE_STATUS_OK)
    {
        /* The only way this can fail is if the combination of block sizes are
         * not supported. 
         */
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

        fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                              "logical drive: negotiate invalid block size. Geometry:0x%x req:0x%x "
                              "req opt size:0x%x\n",
                              logical_drive_p->block_edge.block_edge_geometry, 
                              negotiate_block_size_p->requested_block_size,
                              negotiate_block_size_p->requested_optimum_block_size);
    }
    else
    {
        /* Negotiate was successful.
         */
        
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }

    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_logical_drive_control_get_negotiate_info()
 **************************************/
/*!**************************************************************
 * fbe_logical_drive_set_edge_tap_hook()
 ****************************************************************
 * @brief
 *  This function sets the edge tap hook function for our block edge.
 *
 * @param logical_drive_p - Our object to set hook for.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @revision
 *  12/22/2009 - Created. Swati Fursule
 *
 ****************************************************************/
static fbe_status_t
fbe_logical_drive_set_edge_tap_hook(fbe_logical_drive_t * logical_drive_p, fbe_packet_t * packet_p)
{
    fbe_transport_control_set_edge_tap_hook_t * hook_info_p = NULL;    /* INPUT */
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL; 

    fbe_base_object_trace(  (fbe_base_object_t *)logical_drive_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &hook_info_p);
    if (hook_info_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)logical_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer failed\n");

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* The only valid edge indexes are 0 and FBE_U32_MAX (meaning all edges).
     */
    if ((hook_info_p->edge_index != 0) && (hook_info_p->edge_index != FBE_U32_MAX)) 
    {
        fbe_base_object_trace((fbe_base_object_t *)logical_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s index %d out of range\n",
                              __FUNCTION__, hook_info_p->edge_index);

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Try to set the hook into the edge.
     */
    status = fbe_block_transport_edge_set_hook(&logical_drive_p->block_edge, hook_info_p->edge_tap_hook);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)logical_drive_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s unable to set edge hook status: %d\n",
                                __FUNCTION__, status);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;    
    }
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_logical_drive_set_edge_tap_hook()
 **************************************/
/*!**************************************************************
 * fbe_logical_drive_set_edge_tap_hook()
 ****************************************************************
 * @brief
 *  This function removes the edge tap hook function for our block edge.
 *
 * @param logical_drive_p - Our object to set hook for.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  09/10/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
static fbe_status_t
fbe_logical_drive_remove_edge_tap_hook(fbe_logical_drive_t * logical_drive_p, fbe_packet_t * packet_p)
{
    fbe_transport_control_remove_edge_tap_hook_t * hook_info_p = NULL;    /* INPUT */
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL; 

    fbe_base_object_trace(  (fbe_base_object_t *)logical_drive_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &hook_info_p);
    if (hook_info_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)logical_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer failed\n");

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Try to set the hook into the edge.
     */
    status = fbe_block_transport_edge_remove_hook(&logical_drive_p->block_edge, packet_p);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)logical_drive_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s unable to set edge hook status: %d\n",
                                __FUNCTION__, status);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;    
    }
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_logical_drive_remove_edge_tap_hook()
 **************************************/
static fbe_status_t
logical_drive_get_drive_info(fbe_logical_drive_t * logical_drive, fbe_packet_t * packet_p)
{
    fbe_logical_drive_control_get_drive_info_t * get_drive_info = NULL;    /* INPUT */    
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;  

    fbe_base_object_trace(  (fbe_base_object_t *)logical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &get_drive_info);
    if (get_drive_info == NULL)
    {

        fbe_base_object_trace(  (fbe_base_object_t *)logical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "fbe_payload_control_get_buffer failed\n");

		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    get_drive_info->port_number = logical_drive->port_number;
    get_drive_info->enclosure_number = logical_drive->enclosure_number;
    get_drive_info->slot_number = logical_drive->slot_number;
    fbe_block_transport_server_get_capacity(&logical_drive->block_transport_server, 
                                            &get_drive_info->exported_capacity); 

    strncpy(get_drive_info->identify.identify_info, logical_drive->last_identity.identify_info,
            sizeof(get_drive_info->identify.identify_info) );
    
	fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}

static fbe_status_t 
logical_drive_attach_block_edge(fbe_logical_drive_t * logical_drive, fbe_packet_t * packet)
{
    fbe_block_transport_control_attach_edge_t * block_transport_control_attach_edge = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_status_t status;
    fbe_block_edge_geometry_t block_edge_geometry;
    fbe_traffic_priority_t	traffic_priority = FBE_TRAFFIC_PRIORITY_INVALID;

    fbe_base_object_trace(  (fbe_base_object_t*)logical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &block_transport_control_attach_edge);
    if(block_transport_control_attach_edge == NULL){
        fbe_base_object_trace((fbe_base_object_t *) logical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "fbe_payload_control_get_buffer fail\n");
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_block_transport_control_attach_edge_t)){
        fbe_base_object_trace((fbe_base_object_t *) logical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Right now we do not know all the path state and path attributes rules,
     * so we will do something straight forward and may be not exactly correct.
     */
    fbe_block_transport_server_lock(&logical_drive->block_transport_server);

    status = fbe_block_transport_server_attach_edge(&logical_drive->block_transport_server, 
                                                    block_transport_control_attach_edge->block_edge,
                                                    &fbe_logical_drive_lifecycle_const,
                                                    (fbe_base_object_t *)logical_drive );

    if (status != FBE_STATUS_OK) {
		fbe_block_transport_server_unlock(&logical_drive->block_transport_server);
        fbe_base_object_trace((fbe_base_object_t *) logical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s error %d attaching edge\n", __FUNCTION__, status);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
	
#if 0 /* Block edge geometry */
    /* Set the capacity and block size in the edge. */
	imported_block_size = fbe_ldo_get_imported_block_size(logical_drive);

    fbe_block_transport_edge_set_imported_block_size(block_transport_control_attach_edge->block_edge, imported_block_size);

	/* Assume default 520 for exported block_size */
	exported_block_size = 520;
	fbe_block_transport_edge_set_exported_block_size(block_transport_control_attach_edge->block_edge, exported_block_size);

	/* Recalculate capacity */
	fbe_block_transport_edge_get_capacity(&logical_drive->block_edge, &capacity);
	capacity = (capacity * imported_block_size) / exported_block_size;	
#endif

	fbe_block_transport_edge_get_geometry(&logical_drive->block_edge, &block_edge_geometry);

	switch(block_edge_geometry){
            case FBE_BLOCK_EDGE_GEOMETRY_512_512:
                    fbe_block_transport_edge_set_geometry(block_transport_control_attach_edge->block_edge, FBE_BLOCK_EDGE_GEOMETRY_512_520);
                    break;
            case FBE_BLOCK_EDGE_GEOMETRY_520_520:
                    fbe_block_transport_edge_set_geometry(block_transport_control_attach_edge->block_edge, FBE_BLOCK_EDGE_GEOMETRY_520_520);
                    break;
            case FBE_BLOCK_EDGE_GEOMETRY_4096_4096:
                fbe_block_transport_edge_set_geometry(block_transport_control_attach_edge->block_edge, FBE_BLOCK_EDGE_GEOMETRY_4096_520);
                break;
            case FBE_BLOCK_EDGE_GEOMETRY_4160_4160:
                fbe_block_transport_edge_set_geometry(block_transport_control_attach_edge->block_edge, FBE_BLOCK_EDGE_GEOMETRY_4160_520);
                break;
            default:
			fbe_block_transport_server_unlock(&logical_drive->block_transport_server);
			fbe_base_object_trace((fbe_base_object_t *) logical_drive, 
									FBE_TRACE_LEVEL_ERROR,
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"%s Invalid block_edge_geometry %d \n", __FUNCTION__, block_edge_geometry);
			fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
			fbe_transport_complete_packet(packet);
			return FBE_STATUS_GENERIC_FAILURE;
	}
    /* Optimize so that I/Os flow directly to the pdo and skip the logical drive when no block virtualization is needed.
     */
    if (block_edge_geometry == FBE_BLOCK_EDGE_GEOMETRY_520_520)
    {
        block_transport_control_attach_edge->block_edge->object_handle = logical_drive->block_edge.object_handle;
        block_transport_control_attach_edge->block_edge->io_entry = logical_drive->block_edge.io_entry;
    }

	/*see what is the traffic on the physical drive and bubble it up*/
	traffic_priority = fbe_block_transport_edge_get_traffic_priority(&logical_drive->block_edge);
	fbe_block_transport_edge_set_traffic_priority(block_transport_control_attach_edge->block_edge, traffic_priority);

    fbe_block_transport_server_unlock(&logical_drive->block_transport_server);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t 
logical_drive_detach_block_edge(fbe_logical_drive_t * logical_drive, fbe_packet_t * packet)
{
    fbe_block_transport_control_detach_edge_t * block_transport_control_detach_edge = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_u32_t number_of_clients;
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    fbe_base_object_trace(  (fbe_base_object_t*)logical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &block_transport_control_detach_edge); 
    if(block_transport_control_detach_edge == NULL){
        fbe_base_object_trace((fbe_base_object_t *) logical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "fbe_payload_control_get_buffer fail\n");
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_block_transport_control_detach_edge_t)){
        fbe_base_object_trace((fbe_base_object_t *) logical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Right now we do not know all the path state and path attributes rules,
       so we will do something straight forward and may be not exactly correct.
     */
    fbe_block_transport_server_lock(&logical_drive->block_transport_server);
    fbe_block_transport_server_detach_edge(&logical_drive->block_transport_server, 
                                        block_transport_control_detach_edge->block_edge);
    fbe_block_transport_server_unlock(&logical_drive->block_transport_server);

    fbe_block_transport_server_get_number_of_clients(&logical_drive->block_transport_server, &number_of_clients);

    /* It is possible that we waiting for discovery server to become empty.
        It whould be nice if we can reschedule monitor when we have no clients any more 
    */
    fbe_base_object_trace(  (fbe_base_object_t *)logical_drive,
                            FBE_TRACE_LEVEL_DEBUG_LOW /*FBE_TRACE_LEVEL_DEBUG_HIGH*/,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s number_of_clients = %d\n", __FUNCTION__, number_of_clients);

    if(number_of_clients == 0){
        status = fbe_lifecycle_reschedule(&fbe_logical_drive_lifecycle_const,
                                        (fbe_base_object_t *) logical_drive,
                                        (fbe_lifecycle_timer_msec_t) 0); /* Immediate reschedule */
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/*!**************************************************************
 * fbe_logical_drive_send_block_operation()
 ****************************************************************
 * @brief
 *  This function sends block operations for this object.
 *
 * @param logical_drive_p - The logical drive object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @revision
 *  08/16/2010  Swati Fursule  - Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_logical_drive_send_block_operation(fbe_logical_drive_t * logical_drive_p, 
                                     fbe_packet_t * packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;

    status = fbe_block_transport_send_block_operation(
     (fbe_block_transport_server_t*)&logical_drive_p->block_transport_server,
     packet_p,
     (fbe_base_object_t *)logical_drive_p);
    return status;
}
/**************************************
 * end fbe_logical_drive_send_block_operation()
 **************************************/

/*!**************************************************************
 * fbe_logical_drive_generate_ica_stamp()
 ****************************************************************
 * @brief
 *  This function generates the ICA stamp on the drive
 *
 * @param logical_drive_p - The logical drive object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 *
 ****************************************************************/
static fbe_status_t fbe_logical_drive_generate_ica_stamp(fbe_logical_drive_t * logical_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t * 					payload = NULL;
    fbe_payload_control_operation_t * 	control_operation = NULL;  
	fbe_block_size_t					exported_block_size;
    fbe_logical_drive_generate_ica_stamp_t *buffer = NULL;

	payload = fbe_transport_get_payload_ex(packet);
	control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_base_object_trace(  (fbe_base_object_t *)logical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

	/*are we allowed to do that*/
	if ((logical_drive->port_number != 0) ||
		(logical_drive->enclosure_number != 0) ||
		(!fbe_private_space_layout_does_region_use_fru(FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_ICA_FLAGS, logical_drive->slot_number))) {

		fbe_base_object_trace((fbe_base_object_t *) logical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, illegal drive position for ICA:p=%d, e=%d, s=%d\n", __FUNCTION__,
								logical_drive->port_number,
								logical_drive->enclosure_number,
								logical_drive->slot_number);

		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
	}

    fbe_payload_control_get_buffer(control_operation, &buffer);
    if(buffer == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *) logical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, This function requires a buffer\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
        
	/*we'll support it for 520 only for now*/
	fbe_block_transport_edge_get_exported_block_size(&logical_drive->block_edge, &exported_block_size);
	if (exported_block_size != FBE_BE_BYTES_PER_BLOCK) {
		fbe_base_object_trace((fbe_base_object_t *) logical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, block size is=%d, this operation is supported on %d only\n", __FUNCTION__, exported_block_size, FBE_BE_BYTES_PER_BLOCK);

		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
	}
    
	return fbe_logical_drive_generate_and_send_ica_stamp(logical_drive, packet);
}

/**************************************
 * end fbe_logical_drive_generate_ica_stamp()
 **************************************/

static fbe_status_t fbe_logical_drive_generate_and_send_ica_stamp(fbe_logical_drive_t * logical_drive, fbe_packet_t * packet)
{
    fbe_status_t						status;
	fbe_u8_t *							write_buffer = NULL;
    fbe_sg_element_t *					sg_list_p = NULL;
	fbe_payload_control_operation_t * 	control_operation = NULL;  
	fbe_payload_ex_t * 					payload = NULL;
	fbe_payload_block_operation_t	*	block_operation = NULL;
	fbe_private_space_layout_region_t	psl_region_info;
	fbe_packet_t *						new_packet_p = NULL;
    fbe_logical_drive_generate_ica_stamp_t * control_buffer = NULL;
    
	payload = fbe_transport_get_payload_ex(packet);
	control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &control_buffer);
    
    write_buffer = fbe_transport_allocate_buffer();

	if(write_buffer == NULL) {
		 fbe_base_object_trace((fbe_base_object_t *) logical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, can't get memory\n", __FUNCTION__);

		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

    control_buffer->write_buffer_ptr = write_buffer;


	/*let's carve the memory and sg lists and fill it up*/
    status = fbe_logical_drive_carve_memory_for_buffer_and_sgl(&sg_list_p, write_buffer, control_buffer->preserve_ddbs);
	if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *) logical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, can't format memory for ICA write\n", __FUNCTION__);

		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}


	new_packet_p = fbe_transport_allocate_packet();
    if (new_packet_p == NULL){
        fbe_base_object_trace(  (fbe_base_object_t *)logical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s_allocate_packet failed\n", __FUNCTION__);
        fbe_transport_release_buffer(write_buffer);
		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    fbe_transport_initialize_packet(new_packet_p);
    payload = fbe_transport_get_payload_ex(new_packet_p);
    block_operation = fbe_payload_ex_allocate_block_operation(payload);  
    
    fbe_transport_add_subpacket(packet, new_packet_p);

    fbe_transport_set_cancel_function(packet, fbe_base_object_packet_cancel_function, (fbe_base_object_t *)logical_drive);
    fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)logical_drive, packet);
    
	/*we are good to go, let's send the packet down*/
    fbe_payload_ex_set_sg_list(payload, sg_list_p, 2);

    /*get some information about the ICA region*/
	fbe_private_space_layout_get_region_by_region_id(FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_ICA_FLAGS, &psl_region_info);

	fbe_payload_block_build_operation(block_operation,
									  FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
									  psl_region_info.starting_block_address,
									  1,/*we care about the first block only for now*/
									  FBE_BE_BYTES_PER_BLOCK,
									  1,
									  NULL);

	
	fbe_transport_set_completion_function(new_packet_p,
										  fbe_logical_drive_generate_and_send_ica_stamp_completion,
										  logical_drive);

    
    /* Send this functional packet to our block transport edge.*/
    status = fbe_ldo_send_io_packet(logical_drive, new_packet_p);
    
	return status;
}

/*this function fills the buffers for the ICA stamp 
IT IS NOT COMPLETED YET AND WILL SERVE SIMULATION ONLY RIGHT NOW !!!!!*/
static fbe_status_t fbe_logical_drive_carve_memory_for_buffer_and_sgl(fbe_sg_element_t **sg_list_p,
																	  fbe_u8_t *write_buffer,
                                                                      fbe_bool_t preserve_ddbs)
{
	
    fbe_u8_t *					chunk_start_address = write_buffer;
	fbe_imaging_flags_t *		imaging_flags;
	fbe_u32_t					image_count = 0;
    
    /*let's start with the data buffer*/
    fbe_zero_memory(write_buffer, FBE_BE_BYTES_PER_BLOCK);

	/*skip to the sgl area*/
	chunk_start_address += FBE_BE_BYTES_PER_BLOCK;

	/*populate it, no need to terminate explicitly since we zero out the memory*/
	*sg_list_p = (fbe_sg_element_t *)chunk_start_address;
	fbe_zero_memory(*sg_list_p, sizeof(fbe_sg_element_t) * 2);
	(*sg_list_p)->address = write_buffer;
	(*sg_list_p)->count = FBE_BE_BYTES_PER_BLOCK;

    /*let's fill the memory with the pattern we like*/
	imaging_flags = (fbe_imaging_flags_t *)(write_buffer);
	fbe_zero_memory(imaging_flags, sizeof(fbe_imaging_flags_t));

    fbe_copy_memory(imaging_flags->magic_string, FBE_IMAGING_FLAGS_MAGIC_STRING, FBE_IMAGING_FLAGS_MAGIC_STRING_LENGTH);
	imaging_flags->invalidate_configuration = FBE_IMAGING_FLAGS_TRUE;
	
	for (image_count = 0; image_count < FBE_IMAGING_IMAGE_TYPE_MAX; image_count++) {
		imaging_flags->images_installed[image_count] = FBE_IMAGING_FLAGS_FALSE;
	}

    if(preserve_ddbs)
    {
        imaging_flags->images_installed[FBE_IMAGING_IMAGE_TYPE_DDBS] = FBE_IMAGING_FLAGS_TRUE;
    }

    return FBE_STATUS_OK;

}

static fbe_status_t 
fbe_logical_drive_generate_and_send_ica_stamp_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_logical_drive_t * 					logical_drive = (fbe_logical_drive_t * )context;
	fbe_payload_control_operation_t * 		control_operation = NULL;  
	fbe_payload_ex_t * 						payload = NULL;
	fbe_payload_block_operation_t	*		block_operation = NULL;
	fbe_payload_block_operation_status_t	block_status;
	fbe_packet_t * 							master_packet_p = NULL;
    fbe_logical_drive_generate_ica_stamp_t * control_buffer = NULL;
    
    master_packet_p = fbe_transport_get_master_packet(packet);
	fbe_transport_remove_subpacket(packet);

	/*first let's get the block status of the IO*/
	payload = fbe_transport_get_payload_ex(packet);
	block_operation = fbe_payload_ex_get_block_operation(payload);  
	fbe_payload_block_get_status(block_operation, &block_status);
	fbe_payload_ex_release_block_operation(payload, block_operation);
    
	/*now the control operation of the master packets*/
	payload = fbe_transport_get_payload_ex(master_packet_p);
	control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_buffer(control_operation, &control_buffer);
    fbe_transport_release_buffer(control_buffer->write_buffer_ptr);
    
	fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *)logical_drive, master_packet_p);

	fbe_transport_release_packet(packet);/*don't need it anymore*/

	if (FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS != block_status) {
		fbe_base_object_trace((fbe_base_object_t *) logical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s,failed to write ICA stamp\n", __FUNCTION__);

		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(master_packet_p);
		return FBE_STATUS_OK;

	}

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(master_packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(master_packet_p);
    
	return FBE_STATUS_OK;
}


/*!**************************************************************
 * fbe_logical_drive_set_pvd_eol()
 ****************************************************************
 * @brief
 *  This function sets the EOL bit on the PVD edge
 *
 * @param logical_drive_p - The logical drive object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 *
 ****************************************************************/
static fbe_status_t fbe_logical_drive_set_pvd_eol(fbe_logical_drive_t * logical_drive, fbe_packet_t * packet)
{
	fbe_status_t status;

	fbe_base_object_trace( (fbe_base_object_t *)logical_drive,
                           FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "%s entry\n", __FUNCTION__);

    status = fbe_block_transport_server_set_path_attr(&(logical_drive->block_transport_server),
    										 0,
    										 FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE);

	return status;

}

/*!**************************************************************
 * fbe_logical_drive_clear_pvd_eol()
 ****************************************************************
 * @brief
 *  This function clears the EOL bit on the PVD edge
 *
 * @param logical_drive_p - The logical drive object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 *
 ****************************************************************/
static fbe_status_t fbe_logical_drive_clear_pvd_eol(fbe_logical_drive_t * logical_drive, fbe_packet_t * packet)
{
	fbe_status_t status;

	fbe_base_object_trace( (fbe_base_object_t *)logical_drive,
                           FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "%s entry\n", __FUNCTION__);

    status = fbe_block_transport_server_clear_path_attr(&(logical_drive->block_transport_server),
    										 0,
    										 FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE);

	return status;

}

/*!**************************************************************
 * fbe_logical_drive_read_ica_stamp()
 ****************************************************************
 * @brief
 *  This function read the ICA stamp from the drive
 *  !!!! the memory for read buffer has to be contiguos !!!
 *
 * @param logical_drive_p - The logical drive object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 *
 ****************************************************************/
static fbe_status_t fbe_logical_drive_read_ica_stamp(fbe_logical_drive_t * logical_drive, fbe_packet_t * packet)
{
	fbe_payload_ex_t * 					payload = NULL;
    fbe_payload_control_operation_t * 	control_operation = NULL;  
	fbe_block_size_t					exported_block_size;
	fbe_logical_drive_read_ica_stamp_t *get_ica_stamp;

	payload = fbe_transport_get_payload_ex(packet);
	control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_base_object_trace(  (fbe_base_object_t *)logical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

	/*are we allowed to do that*/
	if ((logical_drive->port_number != 0) ||
		(logical_drive->enclosure_number != 0) ||
		(logical_drive->slot_number >= 3)) {

		fbe_base_object_trace((fbe_base_object_t *) logical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, illegal drive position for ICA:p=%d, e=%d, s=%d\n", __FUNCTION__,
								logical_drive->port_number,
								logical_drive->enclosure_number,
								logical_drive->slot_number);

		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
	}

    fbe_payload_control_get_buffer(control_operation, &get_ica_stamp);
	if (get_ica_stamp == NULL) {
		fbe_base_object_trace((fbe_base_object_t *) logical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, can't find bufer to read to\n", __FUNCTION__);

		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	/*we'll support it for 520 only for now*/
	fbe_block_transport_edge_get_exported_block_size(&logical_drive->block_edge, &exported_block_size);
	if (exported_block_size != FBE_BE_BYTES_PER_BLOCK) {
		fbe_base_object_trace((fbe_base_object_t *) logical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, block size is=%d, this operation is supported on %d only\n", __FUNCTION__, exported_block_size, FBE_BE_BYTES_PER_BLOCK);

		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
	}
    
	return fbe_logical_drive_send_read_ica_stamp(logical_drive, packet);

}

static fbe_status_t fbe_logical_drive_send_read_ica_stamp(fbe_logical_drive_t * logical_drive, fbe_packet_t * packet)
{
	fbe_status_t						status;
	fbe_u8_t *							read_buffer = NULL;
	fbe_payload_control_operation_t * 	control_operation = NULL;  
	fbe_payload_ex_t * 					payload = NULL;
	fbe_payload_block_operation_t	*	block_operation = NULL;
	fbe_sg_element_t *					sg_element;
	fbe_packet_t *						new_packet_p = NULL;
	fbe_private_space_layout_region_t	psl_region_info;
    
	payload = fbe_transport_get_payload_ex(packet);
	control_operation = fbe_payload_ex_get_control_operation(payload);  

	fbe_payload_control_get_buffer(control_operation, &read_buffer);
	if (read_buffer == NULL) {
		fbe_base_object_trace((fbe_base_object_t *) logical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, can't find bufer to read to\n", __FUNCTION__);

		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

    new_packet_p = fbe_transport_allocate_packet();
    if (new_packet_p == NULL){
        fbe_base_object_trace(  (fbe_base_object_t *)logical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s_allocate_packet failed\n", __FUNCTION__);

		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    fbe_transport_initialize_packet(new_packet_p);
    payload = fbe_transport_get_payload_ex(new_packet_p);
    block_operation = fbe_payload_ex_allocate_block_operation(payload);
    
    fbe_transport_add_subpacket(packet, new_packet_p);

    fbe_transport_set_cancel_function(packet, fbe_base_object_packet_cancel_function, (fbe_base_object_t *)logical_drive);
    fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)logical_drive, packet);

	/*sg stuff*/
	fbe_zero_memory(read_buffer, sizeof(fbe_logical_drive_read_ica_stamp_t));/*will also make the terminating sgl null*/
	sg_element = (fbe_sg_element_t *)(fbe_u8_t *)((fbe_u8_t*)read_buffer + FBE_BE_BYTES_PER_BLOCK);
	sg_element->address = read_buffer;
	sg_element->count = FBE_BE_BYTES_PER_BLOCK;
    fbe_payload_ex_set_sg_list(payload, sg_element, 2);

    /*get some information about the ICA region*/
	fbe_private_space_layout_get_region_by_region_id(FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_ICA_FLAGS, &psl_region_info);

	fbe_payload_block_build_operation(block_operation,
									  FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
									  psl_region_info.starting_block_address,
									  1,/*we care about the first block only for now*/
									  FBE_BE_BYTES_PER_BLOCK,
									  1,
									  NULL);

	
	fbe_transport_set_completion_function(new_packet_p,
										  fbe_logical_drive_send_read_ica_stamp_completion,
										  logical_drive);

    
    /* Send this functional packet to our block transport edge.*/
    status = fbe_ldo_send_io_packet(logical_drive, new_packet_p);
    
	return status;

}

static fbe_status_t 
fbe_logical_drive_send_read_ica_stamp_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_logical_drive_t * 					logical_drive = (fbe_logical_drive_t * )context;
	fbe_payload_control_operation_t * 		control_operation = NULL;  
	fbe_payload_ex_t * 						payload = NULL;
	fbe_payload_block_operation_t	*		block_operation = NULL;
	fbe_payload_block_operation_status_t	block_status;
	fbe_packet_t * 							master_packet_p = NULL;
    
    master_packet_p = fbe_transport_get_master_packet(packet);
	fbe_transport_remove_subpacket(packet);

	/*first let's get the block status of the IO*/
	payload = fbe_transport_get_payload_ex(packet);
	block_operation = fbe_payload_ex_get_block_operation(payload);  
	fbe_payload_block_get_status(block_operation, &block_status);
	fbe_payload_ex_release_block_operation(payload, block_operation);
    
	/*now the control operation of the master packets*/
	payload = fbe_transport_get_payload_ex(master_packet_p);
	control_operation = fbe_payload_ex_get_control_operation(payload);
   
	fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *)logical_drive, master_packet_p);

	fbe_transport_release_packet(packet);/*don't need it anymore*/

	if (FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS != block_status) {
		fbe_base_object_trace((fbe_base_object_t *) logical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s,failed to read ICA stamp\n", __FUNCTION__);

		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(master_packet_p);
		return FBE_STATUS_OK;

	}

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(master_packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(master_packet_p);
    
	return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_logical_drive_clear_logical_errors()
 ******************************************************************************
 * @brief
 *  Clear out any logical errors we have on the upstream edge.
 * 
 * @param logical_drive_p - The virtual drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  11/28/2011 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t
fbe_logical_drive_clear_logical_errors(fbe_logical_drive_t * logical_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t                    status;
    fbe_payload_ex_t                *payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_block_edge_t                *edge_p = (fbe_block_edge_t *)fbe_transport_get_edge(packet_p);
    fbe_edge_index_t                edge_index;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    /* Only clear our attribute on the edge this was sent down.
     */
    if (edge_p != NULL)
    {
        fbe_object_id_t client_id;
        fbe_block_transport_get_client_id(edge_p, &client_id);
        fbe_block_transport_get_server_index(edge_p, &edge_index);
        status = fbe_block_transport_server_clear_path_attr_all_servers(&logical_drive_p->block_transport_server,
                                                                        FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS);
        if (status == FBE_STATUS_OK) 
        { 
            fbe_base_object_trace((fbe_base_object_t *)logical_drive_p,
                                  FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "clear FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS from client 0x%x %d\n", 
                                  client_id, edge_index);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *)logical_drive_p,
                                  FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "error %d clearing FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS from client 0x%x %d\n", 
                                  status, client_id, edge_index);
        }
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)logical_drive_p,
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s no edge in packet %p\n", __FUNCTION__, packet_p);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }

    status = fbe_block_transport_send_control_packet(&logical_drive_p->block_edge, packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_logical_drive_clear_logical_errors()
 ******************************************************************************/

/*This implementation is removed because memory request operation can not be used in physical package*/
#if 0
static fbe_status_t 
fbe_logical_drive_zero_specific_region(fbe_logical_drive_t * logical_drive_p, fbe_packet_t * packet_p, 
                                                                                                                    fbe_lba_t start_lba, fbe_block_count_t block_count)
{
    fbe_payload_ex_t *                 payload  = NULL;
    fbe_payload_control_operation_t *     control_operation = NULL;  
    fbe_payload_block_operation_t*  block_operation = NULL;
    fbe_memory_request_t *              memory_request_p = NULL;
    fbe_status_t                        status;

    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation =  fbe_payload_ex_get_control_operation(payload);

    block_operation = fbe_payload_ex_allocate_block_operation(payload);
    fbe_payload_block_build_operation(block_operation,
                                                                            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                                                            start_lba,
                                                                            block_count,
                                                                            FBE_BE_BYTES_PER_BLOCK,
                                                                            1,
                                                                            NULL);
    fbe_payload_ex_increment_block_operation_level(payload);   
    

    memory_request_p = fbe_transport_get_memory_request(packet_p);

    fbe_memory_build_chunk_request(memory_request_p,
                                   FBE_MEMORY_CHUNK_SIZE_FOR_PACKET,
                                   1,
                                   fbe_transport_get_resource_priority(packet_p),
                                   fbe_transport_get_io_stamp(packet_p),
                                   fbe_logical_drive_zero_specific_region_allocation_completion,
                                   logical_drive_p);

    status = fbe_memory_request_entry(memory_request_p);
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    return FBE_STATUS_OK;
    
}
#endif

/*!***************************************************************
 * @fn fbe_logical_drive_zero_specific_region
 ****************************************************************
 * @brief
 *  this function sends the io packet to zero the specified region on the disk
 *  
 * @param[in] logical_drive - the object pointer of this LDO object
 * @param[in] request_packet - the request packet sent by the requester
 * @param[in] start_lba - the start LBA of the zeroing region
 * @param[in] block_count - the size of the zeroing region
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no errror.
 *
 * @version
 *  03/04/2012 - Created. zphu.
 *
 ****************************************************************/
static fbe_status_t 
fbe_logical_drive_zero_specific_region(fbe_logical_drive_t * logical_drive_p, fbe_packet_t * request_packet, 
                                                                                                                    fbe_lba_t start_lba, fbe_block_count_t block_count)

{
    fbe_payload_ex_t *                  request_payload_p = NULL;
    fbe_packet_t *                          new_packet;
    fbe_payload_ex_t *                     new_payload_p = NULL;
    fbe_payload_control_operation_t *         request_control_operation_p = NULL;
    fbe_payload_block_operation_t *         new_block_operation_p = NULL;
    fbe_status_t                            status;
    fbe_ldo_zero_region_context_t*   zero_context;
    fbe_sg_element_t *                      sg_list_p = NULL;

    /*@zphu: get the pointer to original packet. */
    request_payload_p = fbe_transport_get_payload_ex(request_packet);

    /*@zphu: get the control operation*/
    request_control_operation_p =  fbe_payload_ex_get_control_operation(request_payload_p);

    /*@zphu: allocate memory for subpacket memory
    * @zphu: REMEMBER: fbe_memory_request_entry can never be used in physical package*/
    new_packet = fbe_transport_allocate_packet();
    if(NULL == new_packet)
    {
        fbe_base_object_trace((fbe_base_object_t *) logical_drive_p, 
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fail allocate subpacket memory\n", 
                            __FUNCTION__);
        fbe_payload_control_set_status(request_control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_packet(new_packet);

    /*@zphu: allocate memory for zero context
    *@zphu:  REMEMBER: fbe_memory_request_entry can never be used in physical package*/
    zero_context = (fbe_ldo_zero_region_context_t*)fbe_transport_allocate_buffer();
    if(NULL == zero_context)
    {
        fbe_base_object_trace((fbe_base_object_t *) logical_drive_p, 
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fail allocate ldo zero region context memory\n", 
                            __FUNCTION__);
        fbe_transport_release_packet(new_packet);
        fbe_payload_control_set_status(request_control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*@zphu:  initialize SG element with zeroed buffer. */
    sg_list_p = fbe_zero_get_bit_bucket_sgl();

    /*@zphu: popular the new packet*/
    new_payload_p = fbe_transport_get_payload_ex(new_packet);
    
    /*@zphu:  set the sg list for the write same zero request. */
    fbe_payload_ex_set_sg_list(new_payload_p, sg_list_p, 1);

    /*initialize the zero context*/
    zero_context->block_count = block_count;
    zero_context->start_lba = start_lba;
    zero_context->logical_drive_p = logical_drive_p;

    /*@zphu: calculate the block operation address for the first io*/
    if(block_count < FBE_LDO_ZERO_OPERATION_MAX_BLK_COUNT)
    {
        zero_context->next_lba = start_lba + block_count;
    }
    else
    {
        /*@zphu: max block count is FBE_LDO_ZERO_OPERATION_MAX_BLK_COUNT for an io*/
        zero_context->next_lba = start_lba + FBE_LDO_ZERO_OPERATION_MAX_BLK_COUNT;
        block_count = FBE_LDO_ZERO_OPERATION_MAX_BLK_COUNT;
    }
    
    /*@zphu:  allocate and initialize the block operation. */
    new_block_operation_p = fbe_payload_ex_allocate_block_operation(new_payload_p);
    fbe_payload_block_build_operation(new_block_operation_p,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME,
                                      start_lba,
                                      block_count,
                                      FBE_BE_BYTES_PER_BLOCK,
                                      1,
                                      NULL);
    fbe_payload_ex_increment_block_operation_level(new_payload_p);
    
    /*@zphu:  set completion functon to handle the write same request completion. */
    fbe_transport_set_completion_function(new_packet,
                                          fbe_logical_drive_zero_specific_region_continue_work,
                                          zero_context);
    fbe_transport_add_subpacket(request_packet, new_packet);
    
    fbe_transport_set_cancel_function(new_packet, fbe_base_object_packet_cancel_function, (fbe_base_object_t *)logical_drive_p);
    fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)logical_drive_p, request_packet);

    /*@zphu: now send the packet*/
    status = fbe_ldo_send_io_packet(logical_drive_p, new_packet);

    return status;
}

/*!***************************************************************
 * @fn fbe_logical_drive_zero_specific_region
 ****************************************************************
 * @brief
 *  this function continue to zero the specified region
 *  it will calculate the lba and block count of the next zeroing region
 *  FBE_LDO_ZERO_OPERATION_MAX_BLK_COUNT block count is limit for one io
 *  
 * @param[in] packet_p - the subpacket sent to downstream object
 * @param[in] context - the completion context, where the lba of next io is stored
 *
 * @return
 *  fbe_status_t - FBE_STATUS_MORE_PROCESSING_REQUIRED - if no errror.
 *
 * @version
 *  03/04/2012 - Created. zphu.
 *
 ****************************************************************/
static fbe_status_t 
fbe_logical_drive_zero_specific_region_continue_work(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_ldo_zero_region_context_t *   zero_context = (fbe_ldo_zero_region_context_t *)context;
    fbe_logical_drive_t *                 logical_drive_p = zero_context->logical_drive_p;
    fbe_packet_t *                          request_packet_p = NULL;
    fbe_payload_ex_t *                         sub_payload_p = NULL;
    fbe_payload_ex_t *                     payload_p = NULL;
    fbe_payload_control_operation_t *         master_control_operation_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_status_t                                   status;
    fbe_payload_block_operation_status_t  block_operation_status;
    fbe_lba_t                                        start_lba;
    fbe_block_count_t                        block_count;
    fbe_sg_element_t *                      sg_list_p = NULL;

    /*@zphu:  get the master packet and remove the subpacket. */
    request_packet_p = fbe_transport_get_master_packet(packet_p);
    fbe_transport_remove_subpacket(packet_p);

    /*@zphu: get master packet's payload and control operation*/
    payload_p = fbe_transport_get_payload_ex(request_packet_p);
    master_control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    /*@zphu: get subpacket's payload and block operation*/
    sub_payload_p = fbe_transport_get_payload_ex(packet_p);
    
    /*@zphu:  get the subpacket's operation. */
    block_operation_p = fbe_payload_ex_get_block_operation(sub_payload_p);

    /*@zphu:  Verify that the write request subpacket completed ok. */
    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *) logical_drive_p, 
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s write failed. lba:0x%llx cnt:0x%llx\n", 
                            __FUNCTION__,
			    (unsigned long long)block_operation_p->lba,
			    (unsigned long long)block_operation_p->block_count);
        fbe_transport_release_buffer((void*)zero_context);
        fbe_transport_release_packet(packet_p);
        fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *) logical_drive_p, request_packet_p);

        fbe_payload_control_set_status(master_control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(request_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet_p);
        return FBE_STATUS_OK;
    }
    
    fbe_payload_block_get_status(block_operation_p, &block_operation_status);
    if (block_operation_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {
        fbe_base_object_trace((fbe_base_object_t *) logical_drive_p, 
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s block operation error. lba:0x%llx cnt:0x%llx\n", 
                            __FUNCTION__,
			    (unsigned long long)block_operation_p->lba,
			    (unsigned long long)block_operation_p->block_count);
        fbe_transport_release_buffer((void*)zero_context);
        fbe_transport_release_packet(packet_p);
        fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *) logical_drive_p, request_packet_p);

        fbe_payload_control_set_status(master_control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(request_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet_p);
        return FBE_STATUS_OK;
    }

    fbe_base_object_trace((fbe_base_object_t *) logical_drive_p, 
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s zero region 0x%llx - 0x%llx success\n", 
                        __FUNCTION__,
			(unsigned long long)block_operation_p->lba,
			(unsigned long long)(block_operation_p->lba + block_operation_p->block_count - 1));

    /*@zphu: reinitialize the subpacket for next io*/
    fbe_payload_ex_release_block_operation(sub_payload_p, block_operation_p);
    fbe_transport_reuse_packet(packet_p);

    /*@zphu: When we arrive here, the write io is successfully returned. Now process next write*/

    /*@zphu: If we have written the last block, then finish the zeroing operation*/
    if(zero_context->next_lba >= zero_context->start_lba + zero_context->block_count)
    {
        fbe_transport_release_buffer((void*)zero_context);
        fbe_transport_release_packet(packet_p);
        fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *) logical_drive_p, request_packet_p);

        fbe_payload_control_set_status(master_control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(request_packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(request_packet_p);
        return FBE_STATUS_OK;
    }

    /*@zphu: Calculate the next block range where we will write zero*/
    start_lba = zero_context->next_lba;
    block_count = zero_context->block_count - (zero_context->next_lba - zero_context->start_lba);
    if(block_count < FBE_LDO_ZERO_OPERATION_MAX_BLK_COUNT)
    {
        zero_context->next_lba = start_lba + block_count;
    }
    else
    {
        /*@zphu: max block count is FBE_LDO_ZERO_OPERATION_MAX_BLK_COUNT for an io*/
        zero_context->next_lba = start_lba + FBE_LDO_ZERO_OPERATION_MAX_BLK_COUNT;
        block_count = FBE_LDO_ZERO_OPERATION_MAX_BLK_COUNT;
    }

    /*@zphu: get the zero buffer sg list*/
    sg_list_p = fbe_zero_get_bit_bucket_sgl();
    if(NULL == sg_list_p)
    {
        fbe_transport_release_buffer((void*)zero_context);
        fbe_transport_release_packet(packet_p);
        fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *) logical_drive_p, request_packet_p);

        fbe_payload_control_set_status(master_control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(request_packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(request_packet_p);
        return FBE_STATUS_OK;
    }

    /*@zphu:  set the sg list for the write same zero request. */
    fbe_payload_ex_set_sg_list(sub_payload_p, sg_list_p, 1);

    /*@zphu:  allocate and initialize the block operation. */
    block_operation_p = fbe_payload_ex_allocate_block_operation(sub_payload_p);
    fbe_payload_block_build_operation(block_operation_p,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME,
                                      start_lba,
                                      block_count,
                                      FBE_BE_BYTES_PER_BLOCK,
                                      1,
                                      NULL);
    fbe_payload_ex_increment_block_operation_level(sub_payload_p);
    
    /*@zphu:  set completion functon to handle the write same request completion. */
    fbe_transport_set_completion_function(packet_p,
                                          fbe_logical_drive_zero_specific_region_continue_work,
                                          zero_context);
    fbe_transport_add_subpacket(request_packet_p, packet_p);
    
    fbe_transport_set_cancel_function(packet_p, fbe_base_object_packet_cancel_function, (fbe_base_object_t *)logical_drive_p);

    /*@zphu: now send the packet*/
    fbe_ldo_send_io_packet(logical_drive_p, packet_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/******************************
 * end fbe_logical_drive_usurper.c
 ******************************/

