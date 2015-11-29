/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_logical_drive_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Logical Drive related APIs.
 * 
 * @ingroup fbe_api_physical_package_interface_class_files
 * @ingroup fbe_api_logical_drive_interface
 * 
 * @version
 *   10/14/08    sharel - Created
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
#include "fbe/fbe_logical_drive.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_block_transport.h"
#include "fbe/fbe_api_logical_drive_interface.h"
#include "fbe/fbe_scsi_interface.h"

/**************************************
                Local variables
**************************************/

/*******************************************
                Local functions
********************************************/
static fbe_status_t fbe_api_get_drive_block_size_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t send_block_payload_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t  fbe_api_logical_drive_interface_generate_ica_stamp_completion(fbe_packet_t * packet, 
																				   fbe_packet_completion_context_t context);

static fbe_status_t  fbe_api_logical_drive_interface_read_ica_stamp_completion(fbe_packet_t * packet, 
																				   fbe_packet_completion_context_t context);


/*!***************************************************************
 * @fn fbe_api_logical_drive_get_drive_block_size(
 *     const fbe_object_id_t object_id,
 *     fbe_block_transport_negotiate_t *const negotiate_p,
 *     fbe_payload_block_operation_status_t *const block_status_p,
 *     fbe_payload_block_operation_qualifier_t *const block_qualifier) 
 *****************************************************************
 * @brief
 *  This function sends a syncronous mgmt packet to get block size (i.e.
 *  negotiate) the block size.
 *
 * @param object_id        - The logical drive object id to send request to
 * @param negotiate_p      - pointer to the negotiate date
 * @param block_status_p   - Pointer to the block status
 * @param block_qualifier  - Pointer to the block qualifier
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  10/14/08: sharel    created
 *
 ****************************************************************/
 fbe_status_t FBE_API_CALL fbe_api_logical_drive_get_drive_block_size(const fbe_object_id_t object_id,
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
                                          fbe_api_get_drive_block_size_completion, 
                                          &sem);

    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_PHYSICAL,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id); 

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
 * @fn fbe_api_get_drive_block_size_completion(
 *     fbe_packet_t * packet, 
 *     fbe_packet_completion_context_t context) 
 *****************************************************************
 * @brief
 *  This is completion function for fbe_api_logical_drive_get_drive_block_size.
 *
 * @param packet - pointer to the completing packet
 * @param context - completion context with relevant completion information (semaphore to take down)
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  6/29/07: sharel created
 *
 ****************************************************************/
static fbe_status_t fbe_api_get_drive_block_size_completion(fbe_packet_t * packet, 
                                                            fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
    return(FBE_STATUS_OK);
} 

/*!***************************************************************
 * @fn fbe_api_logical_drive_get_attributes(
 *     fbe_object_id_t object_id, 
 *     fbe_logical_drive_attributes_t * const attributes_p) 
 *****************************************************************
 * @brief
 *  This gets the attributes of the logical drive.
 *
 * @param object_id - object ID
 * @param attributes_p - pointer to logical drive attributes
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  10/14/08: sharel    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_logical_drive_get_attributes(fbe_object_id_t object_id, 
                                                               fbe_logical_drive_attributes_t * const attributes_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t     status_info;

    if (attributes_p == NULL){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: input buffer is NULL\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    status = fbe_api_common_send_control_packet (FBE_LOGICAL_DRIVE_CONTROL_CODE_GET_ATTRIBUTES,
                                                 attributes_p,
                                                 sizeof(fbe_logical_drive_attributes_t),
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
 * @fn fbe_api_logical_drive_set_pvd_eol
 *****************************************************************
 * @brief
 *  This sets the EOL bit on the PVD edge
 *
 * @param object_id - object ID
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  12/5/11: Jason White    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_logical_drive_set_pvd_eol(fbe_object_id_t object_id)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t     status_info;

    status = fbe_api_common_send_control_packet (FBE_LOGICAL_DRIVE_CONTROL_SET_PVD_EOL,
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
 * @fn fbe_api_logical_drive_clear_pvd_eol
 *****************************************************************
 * @brief
 *  This sets the EOL bit on the PVD edge
 *
 * @param object_id - object ID
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  12/5/11: Jason White    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_logical_drive_clear_pvd_eol(fbe_object_id_t object_id)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t     status_info;

    status = fbe_api_common_send_control_packet (FBE_LOGICAL_DRIVE_CONTROL_CLEAR_PVD_EOL,
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
 * @fn fbe_api_logical_drive_set_attributes(
 *     fbe_object_id_t object_id, 
 *     fbe_logical_drive_attributes_t * const attributes_p) 
 *****************************************************************
 * @brief
 *  This sets the attributes of the logical drive.
 *
 * @param object_id - object ID
 * @param attributes_p - pointer to logical drive attributes
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  10/14/08: sharel    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_logical_drive_set_attributes(fbe_object_id_t object_id, 
                                                               fbe_logical_drive_attributes_t * const attributes_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t     status_info;

    if (attributes_p == NULL){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: input buffer is NULL\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    status = fbe_api_common_send_control_packet (FBE_LOGICAL_DRIVE_CONTROL_CODE_SET_ATTRIBUTES,
                                                 attributes_p,
                                                 sizeof(fbe_logical_drive_attributes_t),
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
 * @fn fbe_api_logical_drive_interface_send_block_payload(
 *                 fbe_object_id_t object_id,
 *                 fbe_payload_block_operation_opcode_t opcode,
 *                 fbe_lba_t lba,
 *                 fbe_block_count_t blocks,
 *                 fbe_block_size_t block_size,
 *                 fbe_block_size_t opt_block_size,
 *                 fbe_sg_element_t * const sg_p,
 *                 fbe_payload_pre_read_descriptor_t *const pre_read_desc_p,
 *                 fbe_packet_attr_t *const transport_qualifier_p,
 *                 fbe_api_block_status_values_t *const block_status_p)
 ****************************************************************
 * @brief
 *  This function sends a block command for the given params.
 *
 * @param object_id - Object being sent to.
 * @param opcode - Operation to use for this command.
 * @param lba - Start logical block address of command.
 * @param blocks - Total blocks for transfer.
 * @param block_size - Block size to use in command.
 * @param opt_block_size - Optimal block size to use in command.
 * @param sg_p - Scatter gather pointer.
 * @param pre_read_desc_p - The edge descriptor pointer for this cmd.
 * @param transport_qualifier_p - The qualifier from the transport.
 * @param block_status_p - The status of the block payload.
 *
 * @return
 *  fbe_status_t FBE_API_CALL - FBE_STATUS_OK - if no error.
 *
 * @version
 *  08/29/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_logical_drive_interface_send_block_payload(fbe_object_id_t object_id,
                                                   fbe_payload_block_operation_opcode_t opcode,
                                                   fbe_lba_t lba,
                                                   fbe_block_count_t blocks,
                                                   fbe_block_size_t block_size,
                                                   fbe_block_size_t opt_block_size,
                                                   fbe_sg_element_t * const sg_p,
                                                   fbe_payload_pre_read_descriptor_t * const pre_read_desc_p,
                                                   fbe_packet_attr_t * const transport_qualifier_p,
                                                   fbe_api_block_status_values_t * const block_status_p)
{
    fbe_status_t status;
    fbe_packet_t *packet_p;
    fbe_block_edge_t edge;
    fbe_payload_ex_t *payload_p;
    fbe_payload_block_operation_t *block_operation_p;
    fbe_semaphore_t sem;

    /* Initialize the status values to invalid values.
     */
    block_status_p->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
    block_status_p->qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;
    block_status_p->media_error_lba = FBE_LBA_INVALID;
    block_status_p->pdo_object_id = FBE_OBJECT_ID_INVALID;
    fbe_semaphore_init(&sem, 0, 1);

    /* We fill in an edge, so that the packet will have an associated edge.
     */
    fbe_zero_memory(&edge, sizeof (edge));
    edge.offset = 0;
    edge.capacity = blocks * block_size;
    edge.base_edge.server_id = object_id;
    edge.block_edge_geometry = FBE_BLOCK_EDGE_GEOMETRY_520_520;
    edge.base_edge.path_state = FBE_PATH_STATE_ENABLED;
    fbe_base_transport_set_transport_id((fbe_base_edge_t*)&edge, FBE_TRANSPORT_ID_BLOCK);

    /* Allocate packet.  If the allocation fails, return an error.
     */
    packet_p = fbe_api_get_contiguous_packet ();
    if (packet_p == NULL) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                      "%s fbe_transport_allocate_packet fail", __FUNCTION__);   
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Initialize and fill out the packet for this block command.
     */
    fbe_transport_set_cpu_id(packet_p, 0);
    //packet_p->base_edge = (fbe_base_edge_t*)&edge;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_allocate_block_operation(payload_p);

    /* First build the block operation.
     * We also need to manually increment the payload level 
     * since we will not be sending this on an edge. 
     */
    status = fbe_payload_block_build_operation(block_operation_p,
                                               opcode,
                                               lba,
                                               blocks,
                                               block_size,
                                               opt_block_size,
                                               pre_read_desc_p);

    fbe_payload_ex_increment_block_operation_level(payload_p);

    /* Set the sg ptr into the packet.
     */
    status = fbe_payload_ex_set_sg_list(payload_p, sg_p, 0);

    /* Send the packet.
     */
    status = fbe_api_common_send_io_packet(packet_p,
                                           object_id,
                                           send_block_payload_completion,
                                           &sem,
                                           NULL,
                                           NULL,
                                           FBE_PACKAGE_ID_PHYSICAL);

    /* No need check for error status since our completion is always called.
     * We convert pending to OK, since it's not really an error status. 
     */
    if (status == FBE_STATUS_PENDING)
    {
        status = FBE_STATUS_OK;
    }

    /*we block here and wait for the callback function to take the semaphore down*/
    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

    /* Stash the block status and qualifier before returning.
     */
    fbe_payload_block_get_status(block_operation_p, &block_status_p->status);
    fbe_payload_block_get_qualifier(block_operation_p, &block_status_p->qualifier);
    fbe_payload_ex_get_media_error_lba(payload_p, &block_status_p->media_error_lba);
    //fbe_payload_ex_get_pdo_object_id(payload_p, &block_status_p->pdo_object_id);
    /* Free up the packet memory.
     */
    fbe_api_return_contiguous_packet(packet_p); 
    
    return status;
}

/*!***************************************************************
 * @fn send_block_payload_completion(
 *     fbe_packet_t * packet, 
 *     fbe_packet_completion_context_t context) 
 *****************************************************************
 * @brief
 *  This is a completion function for 
 *  fbe_api_logical_drive_interface_send_block_payload.
 *
 * @param packet - completion packet
 * @param context - packet completion context
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  10/17/08: sharel    created
 *
 ****************************************************************/
static fbe_status_t send_block_payload_completion(fbe_packet_t * packet, 
                                                  fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
    return(FBE_STATUS_OK);

}

/*!***************************************************************
 * @fn fbe_api_get_logical_drive_object_id_by_location(
 *     fbe_u32_t port, 
 *     fbe_u32_t enclosure, 
 *     fbe_u32_t slot, 
 *     fbe_object_id_t *object_id) 
 *****************************************************************
 * @brief
 *  This returns the object id for a logical drive location in the topology.
 *
 * @param port - port info
 * @param enclosure - enclosure
 * @param slot - slot info
 * @param object_id - pointer to object ID
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  7/21/09: sharel created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_logical_drive_object_id_by_location(fbe_u32_t port, 
                                                                          fbe_u32_t enclosure, 
                                                                          fbe_u32_t slot, 
                                                                          fbe_object_id_t *object_id)
{
    fbe_topology_control_get_drive_by_location_t        topology_control_get_drive_by_location;
    fbe_status_t                                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t             status_info;

    if (object_id == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }


    topology_control_get_drive_by_location.port_number = port;
    topology_control_get_drive_by_location.enclosure_number = enclosure;
    topology_control_get_drive_by_location.slot_number = slot;
    topology_control_get_drive_by_location.logical_drive_object_id = FBE_OBJECT_ID_INVALID;

     status = fbe_api_common_send_control_packet_to_service (FBE_TOPOLOGY_CONTROL_CODE_GET_DRIVE_BY_LOCATION,
                                                             &topology_control_get_drive_by_location,
                                                             sizeof(fbe_topology_control_get_drive_by_location_t),
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

     *object_id = topology_control_get_drive_by_location.logical_drive_object_id;

     return status;
}

/*!***************************************************************
 * @fn fbe_api_logical_drive_get_id_by_serial_number(
 *     const fbe_u8_t *sn, 
 *     fbe_u32_t sn_array_size, 
 *     fbe_object_id_t *object_id) 
 *****************************************************************
 * @brief
 *  This returns the object id of a logical drive connected to a 
 *  physical drive with the requested serial number.
 *
 * @param sn            - pointer to the serial number
 * @param sn_array_size - serial number's array length
 * @param object_id - pointer to object ID
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  1/28/10: sharel created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_logical_drive_get_id_by_serial_number(const fbe_u8_t *sn, fbe_u32_t sn_array_size, fbe_object_id_t *object_id)
{
    fbe_topology_control_get_logical_drive_by_sn_t      get_id_by_sn;
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

    status = fbe_api_common_send_control_packet_to_service (FBE_TOPOLOGY_CONTROL_CODE_GET_LOGICAL_DRIVE_BY_SERIAL_NUMBER,
                                                             &get_id_by_sn,
                                                             sizeof(fbe_topology_control_get_logical_drive_by_sn_t),
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
 * fbe_api_logical_drive_get_negotiate_info() 
 *****************************************************************
 * @brief
 *  Returns negotiate info for a given logical drive
 *
 * @param object_id - object id of logical drive.
 * @param negotiate_info_p - pointer to negotiate information.
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  5/20/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_logical_drive_get_negotiate_info(fbe_object_id_t object_id,
                                              fbe_block_transport_negotiate_t *negotiate_info_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;                        

    if (negotiate_info_p == NULL) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, negotiate ptr is null\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (object_id == FBE_OBJECT_ID_INVALID) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, object id is FBE_OBJECT_ID_INVALID\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet(FBE_BLOCK_TRANSPORT_CONTROL_CODE_NEGOTIATE_BLOCK_SIZE,
                                                negotiate_info_p,
                                                sizeof(fbe_block_transport_negotiate_t),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_logical_drive_get_negotiate_info()
 **************************************/
/*!***************************************************************
 * fbe_api_logical_drive_negotiate_and_validate_block_size()
 ****************************************************************
 * @brief
 *  This function negotiates the block size and validates the returned
 *  information.
 *
 * @param object_id - The object_id to test.
 * @param exported_block_size - The bytes per block in the exported block size.
 * @param exported_optimal_block_size - Number of blocks in exported block size.
 * @param expected_imported_block_size - This is what we expect to be returned
 *                                       for the imported block size.
 * @param expected_imported_optimal_block_size - This is what we expect to be
 *                                           returned for the imported optimal
 *                                           block size.
 * @return 
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * @date
 *  05/20/2010 - Created. Rob Foley
 * 
 ****************************************************************/

fbe_status_t FBE_API_CALL
fbe_api_logical_drive_negotiate_and_validate_block_size(fbe_object_id_t object_id,
                                                        fbe_block_size_t exported_block_size,
                                                        fbe_block_size_t exported_optimal_block_size,
                                                        fbe_block_size_t expected_imported_block_size,
                                                        fbe_block_size_t expected_imported_optimal_block_size)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_block_transport_negotiate_t capacity;

    /* Read the capacity in so that we refresh the correct info to be used below..
     */
    capacity.requested_block_size = exported_block_size;

    /* If the optimal block size is zero, the client does not care what it
     * is.  We'll find out what is supported and fill it in. 
     */
    if (exported_optimal_block_size == 0)
    {
        capacity.requested_optimum_block_size = exported_block_size;
    }
    else
    {
        capacity.requested_optimum_block_size = exported_optimal_block_size;
    }

    status = fbe_api_logical_drive_get_negotiate_info( object_id, &capacity );
    if (status != FBE_STATUS_OK) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s negotiate failed with status %d\n", __FUNCTION__, status);
        return status; 
    }

    /* Make sure the exported block size and exported optimal block size is as
     * we negotiated. 
     */
    if (exported_block_size != capacity.block_size)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s expected block size 0x%x != negotiate block size 0x%x\n", 
                      __FUNCTION__, exported_block_size, capacity.block_size);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (exported_optimal_block_size != capacity.optimum_block_size)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s expected optimum block size 0x%x != negotiate optimum block size 0x%x\n", 
                      __FUNCTION__, exported_optimal_block_size, capacity.optimum_block_size);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (expected_imported_block_size != capacity.physical_block_size)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s expected imported block size 0x%x != negotiate imported block size 0x%x\n", 
                      __FUNCTION__, expected_imported_block_size, capacity.physical_block_size);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (expected_imported_optimal_block_size != capacity.physical_optimum_block_size)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s expected imported_optimal block size 0x%x != negotiate phys opt block size 0x%x\n", 
                      __FUNCTION__, expected_imported_optimal_block_size, capacity.physical_optimum_block_size);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_api_logical_drive_negotiate_and_validate_block_size()
 **************************************/

/*!********************************************************************
 *  fbe_api_logical_drive_get_phy_info ()
 *********************************************************************
 *
 *  Description: get the drive phy info. This command will traverse
 *  and finally gets satisfied in the enclosure object.
 *
 *  Inputs: object_id - LDO object id.
 *          drive_phy_info_p - The pointer to fbe_drive_phy_info_t
 *
 *  Return Value: success or failure
 *
 *  History:
 *    06/06/2011: PHE - Created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_logical_drive_get_phy_info(fbe_object_id_t object_id, 
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
 * fbe_api_logical_drive_interface_generate_ica_stamp(fbe_object_id_t object_id)
 ****************************************************************
 * @brief
 *  This function generates the ICA stamp on the requested drive
 *
 * @param object_id - The object_id to stamp.
 * @return 
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_logical_drive_interface_generate_ica_stamp(fbe_object_id_t object_id, fbe_bool_t preserve_ddbs)
{
    fbe_status_t 							status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *							packet = NULL;
	fbe_payload_ex_t *                     	payload = NULL;
    fbe_payload_control_operation_t *   	control_operation = NULL;
	fbe_semaphore_t							sem;
	fbe_payload_control_status_t			control_status;
    fbe_logical_drive_generate_ica_stamp_t * buffer;

    if (object_id == FBE_OBJECT_ID_INVALID) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, object id is FBE_OBJECT_ID_INVALID\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }


	packet = fbe_api_get_contiguous_packet();
	if (packet == NULL) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to allocate packet\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
	}

    buffer = (fbe_logical_drive_generate_ica_stamp_t *)fbe_api_allocate_contiguous_memory(sizeof(fbe_logical_drive_generate_ica_stamp_t));
	if(buffer == NULL) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to allocate buffer\n", __FUNCTION__);
        fbe_api_return_contiguous_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
	}

    buffer->preserve_ddbs = preserve_ddbs;

	fbe_semaphore_init(&sem, 0, 1);

    payload = fbe_transport_get_payload_ex(packet);
	control_operation = fbe_payload_ex_allocate_control_operation(payload);

	fbe_payload_control_build_operation(control_operation,
                                        FBE_LOGICAL_DRIVE_CONTROL_GENERATE_ICA_STAMP,
                                        buffer,
                                        sizeof(fbe_logical_drive_generate_ica_stamp_t));

    fbe_api_common_send_control_packet_asynch(packet,
											  object_id,
											  fbe_api_logical_drive_interface_generate_ica_stamp_completion,
											  &sem,
											  FBE_PACKET_FLAG_NO_ATTRIB,
											  FBE_PACKAGE_ID_PHYSICAL);

	status = fbe_semaphore_wait_ms(&sem, 30000);
	if (status != FBE_STATUS_OK) {
		fbe_semaphore_destroy(&sem);
		fbe_api_return_contiguous_packet(packet);/*no need to destory !*/
        fbe_api_free_contiguous_memory(buffer);
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
    fbe_api_free_contiguous_memory(buffer);

    return status;
}

static fbe_status_t  fbe_api_logical_drive_interface_generate_ica_stamp_completion(fbe_packet_t * packet, 
																				   fbe_packet_completion_context_t context)
{
	fbe_semaphore_t *		sem = (fbe_semaphore_t *)context;
	fbe_semaphore_release(sem, 0, 1, FBE_FALSE);
    return FBE_STATUS_OK;
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
fbe_status_t FBE_API_CALL fbe_api_logical_drive_interface_read_ica_stamp(fbe_object_id_t object_id, fbe_imaging_flags_t *ica_stamp)
{
	fbe_status_t 							status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *							packet = NULL;
	fbe_payload_ex_t *                     	payload = NULL;
    fbe_payload_control_operation_t *   	control_operation = NULL;
	fbe_semaphore_t							sem;
	fbe_payload_control_status_t			control_status;
	fbe_logical_drive_read_ica_stamp_t	*	imaging_flags = NULL;

    if (object_id == FBE_OBJECT_ID_INVALID) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, object id is FBE_OBJECT_ID_INVALID\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	imaging_flags = (fbe_logical_drive_read_ica_stamp_t *)fbe_api_allocate_contiguous_memory(sizeof(fbe_logical_drive_read_ica_stamp_t));
	if (imaging_flags == NULL) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to allocate memory to read in\n", __FUNCTION__);
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
                                         FBE_LOGICAL_DRIVE_CONTROL_READ_ICA_STAMP,
                                         imaging_flags,
                                         sizeof(fbe_logical_drive_read_ica_stamp_t));

    fbe_api_common_send_control_packet_asynch(packet,
											  object_id,
											  fbe_api_logical_drive_interface_read_ica_stamp_completion,
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

static fbe_status_t  fbe_api_logical_drive_interface_read_ica_stamp_completion(fbe_packet_t * packet, 
																				   fbe_packet_completion_context_t context)
{
	fbe_semaphore_t *		sem = (fbe_semaphore_t *)context;
	fbe_semaphore_release(sem, 0, 1, FBE_FALSE);
    return FBE_STATUS_OK;
}
