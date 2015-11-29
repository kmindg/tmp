 /***************************************************************************
  * Copyright (C) EMC Corporation 2008
  * All rights reserved.
  * Licensed material -- property of EMC Corporation
  ***************************************************************************/

 /*!**************************************************************************
  * @file fbe_striper_executor.c
  ***************************************************************************
  *
  * @brief
  *  This file contains the executer functions for the striper.
  * 
  *  This includes the fbe_striper_io_entry() function which is our entry
  *  point for functional packets.
  * 
  * @ingroup striper_class_files
  * 
  * @version
  *   07/22/2009:  Created. RPF
  *
  ***************************************************************************/
 
 /*************************
  *   INCLUDE FILES
  *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_striper.h"
#include "fbe_striper_private.h"
#include "fbe/fbe_winddk.h"
#include "fbe_service_manager.h"

 /*************************
  *   FUNCTION DEFINITIONS
  *************************/

static fbe_status_t fbe_striper_send_read_rekey_alloc_complete(fbe_memory_request_t * memory_request_p, 
                                                               fbe_memory_completion_context_t in_context);
static fbe_status_t fbe_striper_send_read_rekey_subpacket(fbe_packet_t * packet_p,
                                                          fbe_base_object_t *  base_object_p,
                                                          fbe_u16_t         data_disks);
static fbe_status_t fbe_striper_send_read_rekey_completion(fbe_packet_t * packet_p,
                                                           fbe_packet_completion_context_t context);
static fbe_status_t fbe_striper_read_rekey_subpacket_complete(fbe_packet_t * packet_p,
                                                              fbe_packet_completion_context_t context);
static fbe_status_t fbe_striper_send_read_rekey_subpacket(fbe_packet_t * packet_p,
                                                          fbe_base_object_t *  base_object_p,
                                                          fbe_u16_t         data_disks);
/*!***************************************************************
 * fbe_striper_io_entry()
 ****************************************************************
 * @brief
 *  This function is the entry point for
 *  functional transport operations to the striper object.
 *  The FBE infrastructure will call this function for our object.
 *
 * @param object_handle - The striper handle.
 * @param packet_p - The io packet that is arriving.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_GENERIC_FAILURE if transport is unknown,
 *                 otherwise we return the status of the transport handler.
 *
 * @author
 *  07/22/09 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_striper_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet_p)
{
    /*! Call the base class to start the I/O.
     */
    return fbe_raid_group_io_entry(object_handle, packet_p);
}
/* end fbe_striper_io_entry() */

/*!***************************************************************
 * fbe_striper_send_command_to_mirror_objects()
 ****************************************************************
 * @brief
 *  This function will pass the command to the downstream mirror objects
 *
 * @param packet_p 
 * @param base_object_p
 * @param raid_geometry_p
 *
 * @return fbe_status_t
 * 
 * @author
 *   1/27/2011 - created Ashwin Tamilarasan
 *
 ****************************************************************/
fbe_status_t fbe_striper_send_command_to_mirror_objects(fbe_packet_t * packet_p,
                                                        fbe_base_object_t * base_object_p,
                                                        fbe_raid_geometry_t * raid_geometry_p)
{
    fbe_u16_t          data_disks;
    fbe_status_t       status; 
  
   /* Get the number of data disks and send command down
      to the data disks which will be equal to the number of mirror objects     
   */
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    fbe_transport_set_completion_function(packet_p, fbe_striper_send_command_to_mirror_objects_completion, base_object_p);     
    status = fbe_striper_create_subpackets_and_send_down_to_mirror_objects(packet_p, base_object_p, data_disks); 
    
    return status;  

}// End fbe_striper_send_command_to_mirror_objects


/*!****************************************************************************
 * fbe_striper_create_subpackets_and_send_down_to_mirror_objects()
 ******************************************************************************
 * @brief
 *  This function will create allocate subpackets to send the command down.
 *  For a RAID 10 request, it needs to be sent to the mirror
 *  objects. Each mirror object needs to handle the command for RAID 10
 *
 * @param packet_p       
 * @param base_object_p
 * @param data_disks
 *
 * @return status
 *
 * @author
 *  1/20/2011 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t 
fbe_striper_create_subpackets_and_send_down_to_mirror_objects(fbe_packet_t * packet_p,
                                               fbe_base_object_t *  base_object_p,
                                               fbe_u16_t         data_disks)
                                                                       
{

    fbe_memory_request_t *                  memory_request_p = NULL;         
    fbe_memory_request_priority_t           memory_request_priority = 0;
    fbe_status_t                            status;    
    

    memory_request_p = fbe_transport_get_memory_request(packet_p);
    memory_request_priority = fbe_transport_get_resource_priority(packet_p);
    fbe_transport_set_resource_priority(packet_p, memory_request_priority + 1);

    /* build the memory request for allocation. */
    status = fbe_memory_build_chunk_request(memory_request_p,
                                   FBE_MEMORY_CHUNK_SIZE_FOR_PACKET,
                                   data_disks,
                                   memory_request_priority,
                                   fbe_transport_get_io_stamp(packet_p),
                                   (fbe_memory_completion_function_t)fbe_striper_send_down_to_mirror_objects_memory_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                   base_object_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(base_object_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,                                        
                              "Setup for memory allocation failed, status:0x%x, line:%d\n", status, __LINE__);        
        fbe_transport_set_status(packet_p, status, 0);        
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Issue the memory request to memory service. */
    status = fbe_memory_request_entry(memory_request_p);
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {
        fbe_base_object_trace(base_object_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,                                        
                              "USER_VERIFY:memory allocation failed, status:0x%x, line:%d\n", status, __LINE__);        
        fbe_transport_set_status(packet_p, status, 0);        
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
      

}// End fbe_striper_create_subpackets_and_send_down_to_mirror_objects

/*!**************************************************************
 * fbe_striper_send_down_to_mirror_objects_memory_allocation_completion()
 ****************************************************************
 * @brief
 *  This function will pass the command to the downstream mirror objects
 *  The memory was allocated for the subpackets that will be sent dowm
 *  to the mirror objects plus a memory for some buffer
 *
 * @param memory_request_p  
 * @param in_context   
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/19/2011 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/
fbe_status_t fbe_striper_send_down_to_mirror_objects_memory_allocation_completion(fbe_memory_request_t * memory_request_p, 
                                                                                  fbe_memory_completion_context_t in_context)
{
//    fbe_status_t                            status;
    fbe_packet_t *                          sub_packet_p;
    fbe_packet_t *                          sub_packet_array[FBE_PARITY_MAX_WIDTH];
    fbe_packet_t *                          packet_p                = NULL;
    fbe_payload_block_operation_t *         new_block_operation_p   = NULL;
    fbe_payload_block_operation_t *         block_operation_p       = NULL;
    fbe_payload_ex_t *                      new_payload_p           = NULL;
    fbe_payload_ex_t *                      payload_p               = NULL;    
    fbe_base_object_t *                     base_object_p           = NULL;
    fbe_striper_t *                         striper_p               = NULL;    
    fbe_memory_header_t *                   memory_header_p         = NULL;
    fbe_memory_header_t *                   next_memory_header_p    = NULL;
    fbe_raid_geometry_t *                   raid_geometry_p         = NULL;
    fbe_block_edge_t *                      edge_p                  = NULL; 
    fbe_u16_t                               edge_index              = 0;
    fbe_u16_t                               data_disks;
    fbe_optimum_block_size_t                optimum_block_size;

    
    base_object_p   = (fbe_base_object_t *)in_context;
    striper_p       = (fbe_striper_t *)base_object_p;

    raid_geometry_p = fbe_raid_group_get_raid_geometry((fbe_raid_group_t *)striper_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);

    // get the pointer to original packet.
    packet_p            = fbe_transport_memory_request_to_packet(memory_request_p);    
    payload_p           = fbe_transport_get_payload_ex(packet_p);
    block_operation_p   = fbe_payload_ex_get_block_operation(payload_p);

    /* Check allocation status */
    if(fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_base_object_trace((fbe_base_object_t *)striper_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,                                        
                              "%s: memory request: 0x%p state: %d allocation failed \n",
                              __FUNCTION__, memory_request_p, memory_request_p->request_state);        
        fbe_transport_set_status(packet_p, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);        
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_block_transport_edge_get_optimum_block_size(((fbe_base_config_t *)striper_p)->block_edge_p, &optimum_block_size);
    
    // Get the memory that was allocated
    memory_header_p = fbe_memory_get_first_memory_header(memory_request_p);
    next_memory_header_p = memory_header_p;

    fbe_transport_set_cancel_function(packet_p, fbe_base_object_packet_cancel_function, base_object_p);

    for(edge_index = 0; edge_index < FBE_PARITY_MAX_WIDTH; edge_index++){
        sub_packet_array[edge_index] = NULL;
    }

    /* Access the memory request and initiate verify request to the mirror objects */
    edge_index = 0;
    while(next_memory_header_p != NULL)
    {
        sub_packet_p = (fbe_packet_t *)fbe_memory_header_to_data(next_memory_header_p);
        fbe_transport_initialize_sep_packet(sub_packet_p);    
                 
        // Allocate new payload    
        new_payload_p = fbe_transport_get_payload_ex(sub_packet_p);
        new_block_operation_p = fbe_payload_ex_allocate_block_operation(new_payload_p);
        
        fbe_payload_block_build_operation(new_block_operation_p,
                                          block_operation_p->block_opcode,
                                          block_operation_p->lba / data_disks,
                                          block_operation_p->block_count / data_disks,
                                          FBE_BE_BYTES_PER_BLOCK,
                                          optimum_block_size,
                                          NULL);      
    
        /* Set our completion function. */
        fbe_transport_set_completion_function(sub_packet_p, 
                                              fbe_striper_initiate_verify_subpacket_completion,
                                              base_object_p);
    
        /* Add the newly created packet in subpacket queue and put the master packet to initiate_verify queue. */    
        fbe_transport_add_subpacket(packet_p, sub_packet_p);
        sub_packet_array[edge_index] = sub_packet_p;
        edge_index++;
        // Send the packet to the mirror object attached to the striper object.
        fbe_memory_get_next_memory_header(next_memory_header_p, &next_memory_header_p); 
    }

    for(edge_index = 0; edge_index < FBE_PARITY_MAX_WIDTH; edge_index++){
        if(sub_packet_array[edge_index] != NULL){
            fbe_base_config_get_block_edge((fbe_base_config_t *)striper_p, &edge_p, edge_index);
            fbe_block_transport_send_functional_packet(edge_p, sub_packet_array[edge_index]);
        }
    }
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

}   // End fbe_striper_initiate_verify_memory_allocation_completion()

/*!****************************************************************************
 * fbe_striper_initiate_verify_subpacket_completion()
 ******************************************************************************
 * @brief
 *  This function checks the subpacket status, copies the status to the master packet.
 *  It releases the control operation and frees the subpacket. It completes the
 *  master packet
 *
 * @param packet_p  
 * @param context             
 *
 * @return status 
 *
 * @author
 *  11/17/2010 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_striper_initiate_verify_subpacket_completion(fbe_packet_t * packet_p,
                                                       fbe_packet_completion_context_t context)
{ 
    fbe_base_object_t *                     base_object_p = NULL;      
    fbe_packet_t *                          master_packet_p = NULL;    
    fbe_payload_ex_t *                      payload_p = NULL;
    fbe_payload_ex_t *                      master_payload_p = NULL;    
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_payload_block_operation_t *         master_block_operation_p = NULL;
    fbe_status_t                            status;
    fbe_payload_block_operation_status_t    block_status;
    fbe_bool_t                              is_empty;
    fbe_payload_block_operation_status_t    master_block_status;
    fbe_payload_block_operation_status_t    child_block_status;
    fbe_payload_block_operation_qualifier_t child_block_qualifier;
    
    
    base_object_p   = (fbe_base_object_t *)context;
    master_packet_p = (fbe_packet_t *)fbe_transport_get_master_packet(packet_p);        

    /* Get the subpacket & master packet payload and block operation. */
    payload_p           = fbe_transport_get_payload_ex(packet_p);
    block_operation_p   = fbe_payload_ex_get_block_operation(payload_p);

    master_payload_p           = fbe_transport_get_payload_ex(master_packet_p);
    master_block_operation_p   = fbe_payload_ex_get_block_operation(master_payload_p);

    status              = fbe_transport_get_status_code(packet_p);
    fbe_payload_block_get_status(block_operation_p, &block_status);

    fbe_base_object_trace( base_object_p,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "Marking for verify completed for RAID 10, status 0x%x\n", status);

    fbe_transport_copy_packet_status(packet_p, master_packet_p);    

    /* If status is not good then complete the master packet with error. */
    if((status != FBE_STATUS_OK) || (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
    {
        fbe_base_object_trace( base_object_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "Marking for verify failed, status 0x%x\n", status);
    }

    fbe_payload_block_get_status(block_operation_p, &child_block_status);
    fbe_payload_block_get_qualifier(block_operation_p, &child_block_qualifier);
    fbe_payload_block_get_status(master_block_operation_p, &master_block_status);

    /* If master packet block status is invalid then set the status of child to 
     * master. If master packet has a valid block status then copy child packet 
     * status only if child status is higher precedence.
     */
    if(FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID == master_block_status)
    {
        fbe_payload_block_set_status(master_block_operation_p, child_block_status, child_block_qualifier);
    }
    else if(fbe_raid_group_utils_get_block_operation_error_precedece(child_block_status) >
            fbe_raid_group_utils_get_block_operation_error_precedece(master_block_status))
    {
        fbe_payload_block_set_status(master_block_operation_p, child_block_status, child_block_qualifier);
    }

    /* Free the resources we allocated previously. */    
    fbe_payload_ex_release_block_operation(payload_p, block_operation_p);

    //fbe_transport_destroy_packet(packet_p);
    
    fbe_transport_remove_subpacket_is_queue_empty(packet_p, &is_empty);

    if(is_empty)
    {
        fbe_transport_destroy_subpackets(master_packet_p);
        fbe_transport_complete_packet(master_packet_p); 
    }
    
    return status;

}// End fbe_striper_initiate_verify_subpacket_completion

/*!****************************************************************************
 * fbe_striper_send_command_to_mirror_objects_completion()
 ******************************************************************************
 * @brief
 *  This function looks at the subpacket queue and if it is empty,
 *  frees the memory that was allocated for verify
 *
 * @param packet_p  
 * @param context   
 *
 * @return status 
 *
 * @author
 *  11/18/2010 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_striper_send_command_to_mirror_objects_completion(fbe_packet_t * packet_p,
                                                       fbe_packet_completion_context_t context)
{  
    fbe_memory_request_t *                               memory_request_p = NULL;
     
    memory_request_p = fbe_transport_get_memory_request(packet_p);
    //fbe_memory_request_get_entry_mark_free(memory_request_p, &memory_ptr);
    //fbe_memory_free_entry(memory_ptr);       
	fbe_memory_free_request_entry(memory_request_p);

    return FBE_STATUS_OK;

}// End fbe_striper_send_command_to_mirror_objects_completion


/*!***************************************************************
 * fbe_striper_send_read_rekey()
 ****************************************************************
 * @brief
 *  This function will pass the command to the downstream mirror objects
 *
 * @param packet_p 
 * @param base_object_p
 * @param raid_geometry_p
 *
 * @return fbe_status_t
 * 
 * @author
 *  12/10/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_striper_send_read_rekey(fbe_packet_t * packet_p,
                                         fbe_base_object_t * base_object_p,
                                         fbe_raid_geometry_t * raid_geometry_p)
{
    fbe_u16_t          data_disks;
    fbe_status_t       status; 

   /* Get the number of data disks and send command down
    *  to the data disks which will be equal to the number of mirror objects     
    */
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    fbe_transport_set_completion_function(packet_p, fbe_striper_send_read_rekey_completion, base_object_p);     
    status = fbe_striper_send_read_rekey_subpacket(packet_p, base_object_p, data_disks); 
    
    return status;  
}
/**************************************
 * end fbe_striper_send_read_rekey
 **************************************/

/*!****************************************************************************
 * fbe_striper_send_read_rekey_subpacket()
 ******************************************************************************
 * @brief
 *  This function will create allocate subpackets to send the command down.
 *  For a RAID 10 request, it needs to be sent to the mirror
 *  objects. Each mirror object needs to handle the command for RAID 10
 *
 * @param packet_p       
 * @param base_object_p
 * @param data_disks
 *
 * @return status
 *
 * @author
 *  12/10/2013 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t 
fbe_striper_send_read_rekey_subpacket(fbe_packet_t * packet_p,
                                      fbe_base_object_t *  base_object_p,
                                      fbe_u16_t         data_disks)
{

    fbe_memory_request_t *                  memory_request_p = NULL;         
    fbe_memory_request_priority_t           memory_request_priority = 0;
    fbe_status_t                            status;    
    

    memory_request_p = fbe_transport_get_memory_request(packet_p);
    memory_request_priority = fbe_transport_get_resource_priority(packet_p);
    fbe_transport_set_resource_priority(packet_p, memory_request_priority + 1);

    /* build the memory request for allocation. */
    status = fbe_memory_build_chunk_request(memory_request_p,
                                   FBE_MEMORY_CHUNK_SIZE_FOR_PACKET,
                                   data_disks,
                                   memory_request_priority,
                                   fbe_transport_get_io_stamp(packet_p),
                                   (fbe_memory_completion_function_t)fbe_striper_send_read_rekey_alloc_complete,
                                   base_object_p);
    if(status != FBE_STATUS_OK) {
        fbe_base_object_trace(base_object_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,                                        
                              "Setup for memory allocation failed, status:0x%x, line:%d\n", status, __LINE__);        
        fbe_transport_set_status(packet_p, status, 0);        
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Issue the memory request to memory service. */
    status = fbe_memory_request_entry(memory_request_p);
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING)) {
        fbe_base_object_trace(base_object_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,                                        
                              "rekey read memory allocation failed, status:0x%x, line:%d\n", status, __LINE__);        
        fbe_transport_set_status(packet_p, status, 0);        
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
      

}
/**************************************
 * end fbe_striper_send_read_rekey_subpacket
 **************************************/
/*!**************************************************************
 * fbe_striper_send_read_rekey_alloc_complete()
 ****************************************************************
 * @brief
 *  This function will pass the command to the downstream mirror objects
 *  The memory was allocated for the subpackets that will be sent dowm
 *  to the mirror objects plus a memory for some buffer
 *
 * @param memory_request_p  
 * @param in_context   
 *
 * @return status - The status of the operation.
 *
 * @author
 *  12/10/2013 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_striper_send_read_rekey_alloc_complete(fbe_memory_request_t * memory_request_p, 
                                                               fbe_memory_completion_context_t in_context)
{
    fbe_packet_t *                          sub_packet_p;
    fbe_packet_t *                          sub_packet_array[FBE_PARITY_MAX_WIDTH];
    fbe_packet_t *                          packet_p                = NULL;
    fbe_payload_block_operation_t *         new_block_operation_p   = NULL;
    fbe_payload_block_operation_t *         block_operation_p       = NULL;
    fbe_payload_ex_t *                      new_payload_p           = NULL;
    fbe_payload_ex_t *                      payload_p               = NULL;    
    fbe_base_object_t *                     base_object_p           = NULL;
    fbe_striper_t *                         striper_p               = NULL;    
    fbe_memory_header_t *                   memory_header_p         = NULL;
    fbe_memory_header_t *                   next_memory_header_p    = NULL;
    fbe_raid_geometry_t *                   raid_geometry_p         = NULL;
    fbe_block_edge_t *                      edge_p                  = NULL; 
    fbe_u16_t                               edge_index              = 0;
    fbe_u16_t                               data_disks;
    fbe_optimum_block_size_t                optimum_block_size;
    
    base_object_p   = (fbe_base_object_t *)in_context;
    striper_p       = (fbe_striper_t *)base_object_p;

    raid_geometry_p = fbe_raid_group_get_raid_geometry((fbe_raid_group_t *)striper_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);

    // get the pointer to original packet.
    packet_p            = fbe_transport_memory_request_to_packet(memory_request_p);    
    payload_p           = fbe_transport_get_payload_ex(packet_p);
    block_operation_p   = fbe_payload_ex_get_block_operation(payload_p);

    /* Check allocation status */
    if(fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE) {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_base_object_trace((fbe_base_object_t *)striper_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,                                        
                              "%s: memory request: 0x%p state: %d allocation failed \n",
                              __FUNCTION__, memory_request_p, memory_request_p->request_state);        
        fbe_transport_set_status(packet_p, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);        
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set status to good now,
     * We will set status to bad on master if any one of the subpackets fails.
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);

    fbe_block_transport_edge_get_optimum_block_size(((fbe_base_config_t *)striper_p)->block_edge_p, &optimum_block_size);
    
    // Get the memory that was allocated
    memory_header_p = fbe_memory_get_first_memory_header(memory_request_p);
    next_memory_header_p = memory_header_p;

    fbe_transport_set_cancel_function(packet_p, fbe_base_object_packet_cancel_function, base_object_p);

    for(edge_index = 0; edge_index < FBE_PARITY_MAX_WIDTH; edge_index++){
        sub_packet_array[edge_index] = NULL;
    }

    /* Access the memory request and initiate request to the mirror objects */
    edge_index = 0;
    while(next_memory_header_p != NULL) {
        sub_packet_p = (fbe_packet_t *)fbe_memory_header_to_data(next_memory_header_p);
        fbe_transport_initialize_sep_packet(sub_packet_p);    
                 
        // Allocate new payload    
        new_payload_p = fbe_transport_get_payload_ex(sub_packet_p);
        new_block_operation_p = fbe_payload_ex_allocate_block_operation(new_payload_p);
        
        fbe_payload_block_build_operation(new_block_operation_p,
                                          block_operation_p->block_opcode,
                                          block_operation_p->lba,
                                          block_operation_p->block_count,
                                          FBE_BE_BYTES_PER_BLOCK,
                                          optimum_block_size,
                                          NULL);      
    
        /* Set our completion function. */
        fbe_transport_set_completion_function(sub_packet_p, 
                                              fbe_striper_read_rekey_subpacket_complete,
                                              base_object_p);
    
        /* Add the newly created packet in subpacket queue and put the master packet to subpacket queue. */
        fbe_transport_add_subpacket(packet_p, sub_packet_p);
        sub_packet_array[edge_index] = sub_packet_p;
        edge_index++;
        // Send the packet to the mirror object attached to the striper object.
        fbe_memory_get_next_memory_header(next_memory_header_p, &next_memory_header_p); 
    }

    for(edge_index = 0; edge_index < FBE_PARITY_MAX_WIDTH; edge_index++){
        if(sub_packet_array[edge_index] != NULL){
            fbe_base_config_get_block_edge((fbe_base_config_t *)striper_p, &edge_p, edge_index);
            fbe_block_transport_send_functional_packet(edge_p, sub_packet_array[edge_index]);
        }
    }
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

}
/**************************************
 * end fbe_striper_send_read_rekey_alloc_complete
 **************************************/
/*!****************************************************************************
 * fbe_striper_read_rekey_subpacket_complete()
 ******************************************************************************
 * @brief
 *  This function checks the subpacket status, copies the status to the master packet.
 *  It releases the control operation and frees the subpacket. It completes the
 *  master packet
 *
 * @param packet_p  
 * @param context             
 *
 * @return status 
 *
 * @author
 *  12/10/2013 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t fbe_striper_read_rekey_subpacket_complete(fbe_packet_t * packet_p,
                                                              fbe_packet_completion_context_t context)
{ 
    fbe_base_object_t *                     base_object_p = NULL;      
    fbe_packet_t *                          master_packet_p = NULL;    
    fbe_payload_ex_t *                      payload_p = NULL;
    fbe_payload_ex_t *                      master_payload_p = NULL;    
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_payload_block_operation_t *         master_block_operation_p = NULL;
    fbe_status_t                            status;
    fbe_payload_block_operation_status_t    block_status;
    fbe_bool_t                              is_empty;
    fbe_payload_block_operation_status_t    master_block_status;
    fbe_payload_block_operation_qualifier_t master_block_qualifier;
    fbe_payload_block_operation_status_t    child_block_status;
    fbe_payload_block_operation_qualifier_t child_block_qualifier;
    fbe_lba_t                               min_lba;
    fbe_lba_t                               child_lba;
    
    base_object_p   = (fbe_base_object_t *)context;
    master_packet_p = (fbe_packet_t *)fbe_transport_get_master_packet(packet_p);        

    master_payload_p           = fbe_transport_get_payload_ex(master_packet_p);
    master_block_operation_p   = fbe_payload_ex_get_block_operation(master_payload_p);

    status              = fbe_transport_get_status_code(packet_p);

    payload_p           = fbe_transport_get_payload_ex(packet_p);
    block_operation_p   = fbe_payload_ex_get_block_operation(payload_p);
    fbe_payload_block_get_status(block_operation_p, &block_status);

    fbe_base_object_trace( base_object_p,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "read/rekey completed for RAID 10, status 0x%x bls/q: 0x%x/0x%x 0x%llx\n", 
                           status, block_operation_p->status, block_operation_p->status_qualifier,
                           payload_p->media_error_lba);

    /* If status is not good then complete the master packet with error. */
    if((status != FBE_STATUS_OK) || (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)) {
        fbe_base_object_trace( base_object_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "read/rekey failed, status 0x%x\n", status);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_FAILED, 0);
    }
    
    fbe_transport_remove_subpacket_is_queue_empty(packet_p, &is_empty);

    if(is_empty) {
        fbe_queue_element_t * queue_element_p = NULL;
        fbe_queue_element_t * next_element_p = NULL;
        fbe_packet_t * sub_packet_p = NULL;

        queue_element_p = fbe_queue_front(&master_packet_p->subpacket_queue_head);
        master_block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
        master_block_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;
        min_lba = FBE_LBA_INVALID;

        while (queue_element_p != NULL) {
            next_element_p = fbe_queue_next(&master_packet_p->subpacket_queue_head, queue_element_p);
            sub_packet_p = fbe_transport_subpacket_queue_element_to_packet(queue_element_p);
            
            payload_p           = fbe_transport_get_payload_ex(sub_packet_p);
            block_operation_p   = fbe_payload_ex_get_block_operation(payload_p);

            fbe_payload_block_get_status(block_operation_p, &child_block_status);
            fbe_payload_block_get_qualifier(block_operation_p, &child_block_qualifier);
            fbe_payload_ex_get_media_error_lba(payload_p, &child_lba);

            if (child_block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {
                /* We will not overwrite a continue qualifier since it means that we have something to do.
                 * We also will not overwrite another bad status since the precidence handler took care of it for us. 
                 */
                if ( ((master_block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID) ||
                      (master_block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)) &&
                     (master_block_qualifier != FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CONTINUE_REKEY)) {

                    if ((child_block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CONTINUE_REKEY) ||
                        (child_block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NOTHING_TO_REKEY)) {
                        fbe_payload_block_set_status(master_block_operation_p, 
                                                     child_block_status, 
                                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CONTINUE_REKEY);
                        min_lba = FBE_MIN(min_lba, child_lba);
                    }
                }
            }
            /* If master packet block status is invalid then set the status of child to 
             * master. If master packet has a valid block status then copy child packet 
             * status only if child status is higher precedence.
             */
            if (FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID == master_block_status) {
                master_block_status = child_block_status;
                master_block_qualifier = child_block_qualifier;
                fbe_payload_block_set_status(master_block_operation_p, child_block_status, child_block_qualifier);
            } else if (fbe_raid_group_utils_get_block_operation_error_precedece(child_block_status) >
                       fbe_raid_group_utils_get_block_operation_error_precedece(master_block_status)) {
                fbe_payload_block_set_status(master_block_operation_p, child_block_status, child_block_qualifier);
            }
            /* Free the resources we allocated previously. */    
            fbe_payload_ex_release_block_operation(payload_p, block_operation_p);
            queue_element_p = next_element_p;
        }
        fbe_payload_ex_set_media_error_lba(master_payload_p, min_lba);
        fbe_transport_destroy_subpackets(master_packet_p);
        fbe_transport_complete_packet(master_packet_p); 
    }
    return status;
}
/**************************************
 * end fbe_striper_read_rekey_subpacket_complete
 **************************************/

/*!****************************************************************************
 * fbe_striper_send_read_rekey_completion()
 ******************************************************************************
 * @brief
 *  This function looks at the subpacket queue and if it is empty,
 *  frees the memory that was allocated for read  rekey
 *
 * @param packet_p  
 * @param context   
 *
 * @return status 
 *
 * @author
 *  12/10/2013 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t fbe_striper_send_read_rekey_completion(fbe_packet_t * packet_p,
                                                           fbe_packet_completion_context_t context)
{  
    fbe_memory_request_t *                               memory_request_p = NULL;
     
    memory_request_p = fbe_transport_get_memory_request(packet_p);
	fbe_memory_free_request_entry(memory_request_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_striper_send_read_rekey_completion()
 **************************************/

/* This is VERY dangerous and unusual function. Please do not replicate it !!! */
fbe_status_t 
fbe_striper_check_ds_encryption_state(fbe_raid_group_t* raid_group_p, fbe_base_config_encryption_state_t expected_encryption_state)
{
    fbe_u16_t			  data_disks;
	fbe_raid_geometry_t * raid_geometry_p;
    fbe_block_edge_t *    edge_p;
    fbe_u16_t             edge_index;
	fbe_base_config_encryption_state_t encryption_state;
	fbe_base_config_t * ds_base_config;

	raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
 
    for(edge_index = 0; edge_index < data_disks; edge_index++){
		fbe_base_config_get_block_edge((fbe_base_config_t *)raid_group_p, &edge_p, edge_index);
		if(edge_p->object_handle == NULL){
			fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
								  FBE_TRACE_LEVEL_ERROR,
								  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,                                        
								  "%s: Invalid object_handle pos %d \n",
								  __FUNCTION__, edge_index);

			return FBE_STATUS_GENERIC_FAILURE;
		}
		ds_base_config = (fbe_base_config_t *) fbe_base_handle_to_pointer(edge_p->object_handle);
		fbe_base_config_get_encryption_state(ds_base_config, &encryption_state);

		if(encryption_state != expected_encryption_state){
			return FBE_STATUS_ATTRIBUTE_NOT_FOUND; /* This will indicate state mismatch */
		}
    }

	return FBE_STATUS_OK;
}
/* This is VERY dangerous and unusual function. Please do not replicate it !!! */
fbe_status_t 
fbe_striper_check_ds_encryption_chkpt(fbe_raid_group_t* raid_group_p, fbe_lba_t chkpt)
{
    fbe_u16_t			  data_disks;
	fbe_raid_geometry_t * raid_geometry_p;
    fbe_block_edge_t *    edge_p;
    fbe_u16_t             edge_index;
	fbe_lba_t             encryption_checkpoint;
	fbe_raid_group_t    *ds_raid_group_p = NULL;

	raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
 
    for(edge_index = 0; edge_index < data_disks; edge_index++){
		fbe_base_config_get_block_edge((fbe_base_config_t *)raid_group_p, &edge_p, edge_index);
		if(edge_p->object_handle == NULL){
			fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
								  FBE_TRACE_LEVEL_ERROR,
								  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,                                        
								  "%s: Invalid object_handle pos %d \n",
								  __FUNCTION__, edge_index);

			return FBE_STATUS_GENERIC_FAILURE;
		}
		ds_raid_group_p = (fbe_raid_group_t *) fbe_base_handle_to_pointer(edge_p->object_handle);
		encryption_checkpoint = fbe_raid_group_get_rekey_checkpoint(ds_raid_group_p);

		if(encryption_checkpoint != chkpt){
			return FBE_STATUS_ATTRIBUTE_NOT_FOUND; /* This will indicate state mismatch */
		}
    }

	return FBE_STATUS_OK;
}

/*************************
 * end file fbe_base_config_executer.c
 *************************/
 
 
