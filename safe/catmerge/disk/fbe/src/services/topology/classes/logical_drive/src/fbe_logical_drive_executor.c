 /***************************************************************************
  * Copyright (C) EMC Corporation 2008
  * All rights reserved.
  * Licensed material -- property of EMC Corporation
  ***************************************************************************/
 
 /*!**************************************************************************
  * @file fbe_logical_drive_executor.c
  ***************************************************************************
  *
  * @brief
  *  This file contains the executer functions for the logical drive.
  * 
  *  This includes the fbe_logical_drive_io_entry() function which is our entry
  *  point for functional packets.
  * 
  *  This also includes the support for the negotiate block size, identify and
  *  other block commands that are straightforward to service.
  * 
  *  The further handling of other functional packets such as
  *  read/write/verify/write same is continued in fbe_logical_drive_io.c.
  * 
  * @ingroup logical_drive_class_files
  * 
  * HISTORY
  *   7/25/2008:  Created. RPF
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

 /*************************
  *   FUNCTION DEFINITIONS
  *************************/
static fbe_status_t 
fbe_ldo_negotiate_block_size(fbe_logical_drive_t * const logical_drive, 
                             fbe_packet_t * const packet_p);
static fbe_status_t
fbe_ldo_handle_identify(fbe_logical_drive_t * const logical_drive_p, 
                        fbe_packet_t * const packet_p);

static fbe_status_t
ldo_calculate_geometry_and_optimum_block_size_for_packet(fbe_logical_drive_t * const logical_drive_p,
                                 fbe_block_edge_geometry_t * block_edge_geometry,
                                 fbe_block_size_t * optimum_block_size);

/*!**************************************************************
 * @fn fbe_logical_drive_block_transport_entry(
 *            fbe_transport_entry_context_t context,
 *            fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *   This is the entry function that the block transport will
 *   call to pass a packet to the logical drive.
 *  
 * @param context - The logical drive ptr. 
 * @param packet_p - The packet to process.
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  1/19/2009 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_logical_drive_block_transport_entry(fbe_transport_entry_context_t context, 
                                                     fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_logical_drive_t * logical_drive_p = NULL;

    logical_drive_p = (fbe_logical_drive_t *)context;

    /* Simply hand this off to the block transport handler.
     */
    status = fbe_logical_drive_block_transport_handle_io(logical_drive_p, packet_p);
    return status;
}
/**************************************
 * end fbe_logical_drive_block_transport_entry()
 **************************************/

/*!***************************************************************
 * @fn fbe_logical_drive_io_entry(fbe_object_handle_t object_handle,
 *                                fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function is the entry point for
 *  functional transport operations to the logical drive object.
 *  The FBE infrastructure will call this function for our object.
 *
 * @param object_handle - The logical drive handle.
 * @param packet_p - The io packet that is arriving.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_GENERIC_FAILURE if transport is unknown,
 *                 otherwise we return the status of the transport handler.
 *
 * HISTORY:
 *  10/22/07 - Created. RPF
 *
 ****************************************************************/

fbe_status_t 
fbe_logical_drive_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_logical_drive_t * logical_drive_p;
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    fbe_payload_block_operation_opcode_t block_operation_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID;

    /* First, get the opcode. */
    fbe_payload_block_get_opcode(block_operation_p, &block_operation_opcode);

    logical_drive_p = (fbe_logical_drive_t *) fbe_base_handle_to_pointer(object_handle);

	/* If our edge geometry is 520 - 520: just send the I/O down */
	if(logical_drive_p->block_edge.block_edge_geometry == FBE_BLOCK_EDGE_GEOMETRY_520_520){
		if (block_operation_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ ||
             block_operation_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE ||
             block_operation_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY ||
             block_operation_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME)
		{
			status = fbe_block_transport_send_functional_packet(&logical_drive_p->block_edge, packet_p);
			return status;
		}
	}

    status = fbe_block_transport_server_bouncer_entry(&logical_drive_p->block_transport_server,
                                                      packet_p,
                                                      logical_drive_p /* Context */);

	if(status == FBE_STATUS_OK){ /* Keep stack small */
		status = fbe_logical_drive_block_transport_handle_io(logical_drive_p, packet_p);
	}


    return status;
}
/* end fbe_logical_drive_io_entry() */

/*!**************************************************************
 * @fn fbe_logical_drive_block_transport_handle_io(  
 *                          fbe_logical_drive_t *logical_drive_p, 
 *                          fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function is the entry point for block transport
 *  operations to the logical drive object.  We would only
 *  expect this to be called by our logical drive io entry function.
 *
 * @param logical_drive_p - The logical drive.
 * @param packet_p - The io packet that is arriving.
 *
 * @return fbe_status_t.
 *
 * HISTORY:
 *  07/25/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_logical_drive_block_transport_handle_io(fbe_logical_drive_t *logical_drive_p, 
                                        fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_block_size_t requested_block_size = 0;
    fbe_block_size_t imported_block_size = 0;
    fbe_block_size_t optimum_block_size = 0;
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    fbe_payload_block_operation_opcode_t block_operation_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID;
	fbe_block_edge_t * block_edge = NULL;
	fbe_block_edge_geometry_t block_edge_geometry = FBE_BLOCK_EDGE_GEOMETRY_INVALID;

    /* First, get the opcodes.
     */
    fbe_payload_block_get_opcode(block_operation_p, &block_operation_opcode);

    if (block_operation_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_NEGOTIATE_BLOCK_SIZE)
    {
        status = fbe_ldo_negotiate_block_size(logical_drive_p, packet_p);
        return status;
    }
    fbe_payload_block_get_block_size(block_operation_p, &requested_block_size);

	block_edge = (fbe_block_edge_t *)fbe_transport_get_edge(packet_p);

    if (block_edge != NULL)
    {
	    fbe_block_transport_edge_get_geometry(block_edge, &block_edge_geometry);
	    fbe_block_transport_edge_get_optimum_block_size(block_edge, &optimum_block_size);
    }
    else
    {
        ldo_calculate_geometry_and_optimum_block_size_for_packet(logical_drive_p,&block_edge_geometry,&optimum_block_size);
    }

    switch(block_edge_geometry)
    {
		case FBE_BLOCK_EDGE_GEOMETRY_512_520:
			imported_block_size = 512;
			break;
		case FBE_BLOCK_EDGE_GEOMETRY_520_520:
			imported_block_size = 520;
			break;
        case FBE_BLOCK_EDGE_GEOMETRY_4096_4096:
        case FBE_BLOCK_EDGE_GEOMETRY_4096_520:
            imported_block_size = 4096;
            break;
        case FBE_BLOCK_EDGE_GEOMETRY_4160_4160:
        case FBE_BLOCK_EDGE_GEOMETRY_4160_520:
            imported_block_size = 4160;
            break;
		default:
			fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, 
								FBE_TRACE_LEVEL_ERROR, 
								FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
								"%s Invalid block_edge_geometry %d\n", __FUNCTION__, block_edge_geometry);

			fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
			fbe_transport_complete_packet(packet_p);
			return FBE_STATUS_PENDING;
    };

    /* We next check for pass through commands that need to look at the block 
     * size. 
     */
    if (requested_block_size == imported_block_size)
    {
        /* Since the imported and exported sizes are the same, we just send 
         * this through to the edge.
         */
        status = fbe_ldo_send_io_packet(logical_drive_p, packet_p);
        return status;
    }
    else if ( (requested_block_size == 0) || (optimum_block_size == 0) )
    {
        /* If the requested block size or requested optimum size are zero then
         * return an error. 
         */
        fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                              "logical drive: unexpected block sizes req:0x%x opt:0x%x Geometry:0x%x\n", 
                              requested_block_size, optimum_block_size,
                              logical_drive_p->block_edge.block_edge_geometry);
        /* Transport status on the packet is OK since it was received OK. 
         * The payload status is set to bad since this command is not formed 
         * correctly. 
         */
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_ldo_set_payload_status(packet_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                   FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_SIZE);
        fbe_ldo_complete_packet(packet_p);
        return FBE_STATUS_PENDING;
    }
    else if ( ( (requested_block_size * optimum_block_size) > 
                FBE_LDO_MAX_OPTIMUM_BLOCK_SIZE ) )
    {
        /* If the requested optimum size is larger than we ever expect to see, then reject
         * it.  The reason we do this is to protect against completely invalid and 
         * unexpected block sizes or optimum block sizes. 
         * This is an arbitrary and artificial limit, but helps to protect against us 
         * going off into the weeds if someone sends us something completely bugus (and 
         * also very large). 
         */
        fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                              "logical drive: unexpected opt block sizes req:0x%x opt:0x%x Geometry:0x%x\n", 
                              requested_block_size, optimum_block_size,
                              logical_drive_p->block_edge.block_edge_geometry);
        /* Transport status on the packet is OK since it was received OK. 
         * The payload status is set to bad since this command is not formed 
         * correctly. 
         */
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_ldo_set_payload_status(packet_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                   FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_SIZE);
        fbe_ldo_complete_packet(packet_p);
        return FBE_STATUS_PENDING;
    }
    else if (block_operation_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ ||
             block_operation_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE ||
             block_operation_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY ||
             block_operation_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME)
    {
        if (fbe_ldo_resources_required(logical_drive_p, packet_p))
        {
            /* These operations need additional handling.
             * For now we will allocate our context structure. 
             * Eventually this context will come from the packet. 
             */
            fbe_ldo_io_cmd_allocate(packet_p,
                                    (fbe_memory_completion_function_t)fbe_ldo_io_cmd_allocate_memory_callback, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                    logical_drive_p);
        }
        else
        {
            /* We handle the packet inline for most cases where we do not need 
             * to allocate resources.  The packet has most all the resources we 
             * need. 
             */
            switch (block_operation_opcode)
            {
                case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:
                case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY:
                    fbe_ldo_handle_write(logical_drive_p, packet_p);
                    break;
                case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME:
                    fbe_ldo_handle_write_same(logical_drive_p, packet_p);
                    break;
                case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:
                default:
                    fbe_ldo_handle_read(logical_drive_p, packet_p);
                    break;
            };
        }
        /* Always return pending.  We will return status later once we are complete.
         */
        return FBE_STATUS_PENDING;
    }
    else if (block_operation_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY)
    {
        /* Handle the verify.
         * We never allocate additional resources for a verify, and thus we 
         * always handle it immediately. 
         */
        status = fbe_ldo_handle_verify(logical_drive_p, packet_p);
        return status;
    }
    else if (block_operation_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_IDENTIFY)
    {
        /* Handle the identify command.
         */
        status = fbe_ldo_handle_identify(logical_drive_p, packet_p);
        return status;
    }
    else 
    {
        /* Everthing else gets passed through to the edge.
         */
        status = fbe_ldo_send_io_packet(logical_drive_p, packet_p);
        return status;
    }
    /* Always return pending.  We will return status later once we are complete.
     */
    return FBE_STATUS_PENDING;
}
/* end fbe_logical_drive_block_transport_handle_io() */

/*!***************************************************************
 * @fn fbe_ldo_handle_identify(fbe_logical_drive_t * const logical_drive_p,
 *                             fbe_packet_t *const packet_p)
 ****************************************************************
 * @brief
 *  This function handles the identify block transport functional packet.
 *  We simply return the identify information that is stored in the logical
 *  drive.
 * 
 *  The identify info is contained within the control packet.
 *  The identify information can be used by the caller to uniquely
 *  identify the device.
 *
 * @param logical_drive_p - The logical drive pointer.
 * @param packet_p - The functional packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE on error or
 *                        FBE_STATUS_OK on success.
 *
 * HISTORY:
 *  10/30/07 - Created. RPF
 *
 ****************************************************************/
static fbe_status_t
fbe_ldo_handle_identify(fbe_logical_drive_t * const logical_drive_p, 
                        fbe_packet_t * const packet_p)
{
    fbe_block_transport_identify_t* identify_info_p = NULL;
    fbe_u32_t index;

    /* First get a pointer to the block command.
     */
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_sg_element_t *sg_p;

    fbe_payload_ex_get_sg_list(payload_p, &sg_p, NULL);

    /* Next, get a ptr to the identify info.
     */
    identify_info_p = (fbe_block_transport_identify_t *)fbe_sg_element_address(sg_p);

    if (identify_info_p == NULL)
    {
        /* We always expect to receive a buffer.
         */
        fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "logical drive: 0x%p %s identify buffer null.\n", 
                              logical_drive_p, __FUNCTION__);

        /* Transport status is good, but 
         * since the operation failed the packet status is bad. 
         */
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_ldo_set_payload_status(packet_p, 
                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                   FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_SG);
        fbe_ldo_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Copy the identify info into the input buffer.
     */
    for (index = 0; index < FBE_BLOCK_TRANSPORT_CONTROL_IDENTIFY_LENGTH; index++)
    {
        identify_info_p->identify_info[index] = logical_drive_p->identity.identify_info[index];
    }

    /* Payload and packet status are success.
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_ldo_set_payload_status(packet_p, 
                               FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                               FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    fbe_ldo_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/* end fbe_ldo_handle_identify */

/*!***************************************************************
 * @fn fbe_ldo_negotiate_block_size(fbe_logical_drive_t * const
 *                          logical_drive_p, fbe_packet_t * const packet_p)
 ****************************************************************
 * @brief
 *  This function handles the negotiate block size packet.
 * 
 *  This allows the caller to determine which block size and optimal block size
 *  they can use when sending I/O requests.
 *
 * @param logical_drive_p - The logical drive handle.
 * @param packet_p - The mgmt packet that is arriving.
 *
 * @return fbe_status_t
 *
 * HISTORY:
 *  10/30/07 - Created. RPF
 *
 ****************************************************************/
static fbe_status_t
fbe_ldo_negotiate_block_size(fbe_logical_drive_t * const logical_drive_p, 
                             fbe_packet_t * const packet_p)
{
    fbe_block_transport_negotiate_t* negotiate_block_size_p = NULL;

    /* First get a pointer to the block command.
     */
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_sg_element_t *sg_p;
    fbe_status_t status;

    fbe_payload_ex_get_sg_list(payload_p, &sg_p, NULL);

    if (sg_p == NULL)
    {
        /* We always expect to receive a buffer.
         */
        fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                              "logical drive: negotiate sg null. 0x%p %s\n", 
                              logical_drive_p, __FUNCTION__);
        /* Transport status on the packet is OK since it was received OK. 
         * The payload status is set to bad since this command is not formed 
         * correctly. 
         */
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_ldo_set_payload_status(packet_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                   FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NULL_SG);
        fbe_ldo_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Next, get a ptr to the negotiate info. 
     */
    negotiate_block_size_p = (fbe_block_transport_negotiate_t*)fbe_sg_element_address(sg_p);

    if (negotiate_block_size_p == NULL)
    {
        /* We always expect to receive a buffer.
         */
        fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                              "logical drive: negotiate buffer null. 0x%p %s \n", 
                              logical_drive_p, __FUNCTION__);
        /* Transport status on the packet is OK since it was received OK. 
         * The payload status is set to bad since this command is not formed 
         * correctly. 
         */
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_ldo_set_payload_status(packet_p, 
                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                   FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_SG);
        fbe_ldo_complete_packet(packet_p);
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
        fbe_ldo_set_payload_status(packet_p, 
                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                   FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_SIZE);

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
        fbe_ldo_set_payload_status(packet_p, 
                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                   FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    }

    fbe_ldo_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/* end fbe_ldo_negotiate_block_size */

/*!***************************************************************
 * @fn fbe_ldo_calculate_negotiate_info(
 *      fbe_logical_drive_t * const logical_drive_p,
 *      fbe_block_transport_negotiate_t * const negotiate_info_p)
 ****************************************************************
 * @brief
 *  This function attempts to calculate the information needed to be passed back
 *  for the negotiate block size command.  If this negotiation fails then this
 *  function will return an error.
 *
 * @param logical_drive_p - The logical drive.
 * @param negotiate_info_p - The buffer to store negotiate info in.
 *
 * @return FBE_STATUS_OK - Indicates that we support this block size.
 *        FBE_STATUS_GENERIC_FAILURE - Indicates this block size is not
 *                                     supported.
 *
 * HISTORY:
 *  05/14/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t
fbe_ldo_calculate_negotiate_info(fbe_logical_drive_t * const logical_drive_p,
                                 fbe_block_transport_negotiate_t * const negotiate_info_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_block_size_t imported_block_size = fbe_ldo_get_imported_block_size(logical_drive_p);

    /* Default the imported optimum block size to 1.
     */
    fbe_block_size_t imported_optimum_block_size = 1;
    fbe_block_size_t exported_optimum_block_size;

    /* Get the optimum block size for the given combination of
     * imported and client requested exported block size.
     */
    status = ldo_get_optimum_block_sizes(imported_block_size,
                                         negotiate_info_p->requested_block_size,
                                         &imported_optimum_block_size,
                                         &exported_optimum_block_size);

    if (status == FBE_STATUS_OK)
    {
        /* Block sizes recognized, we have valid optimum block sizes.
         */
        fbe_lba_t capacity;
	    fbe_block_transport_edge_get_capacity(&logical_drive_p->block_edge, &capacity);

        /* The block size negotiation was successful, just set the block size to 
         * the size requested. 
         */
        negotiate_info_p->block_size = negotiate_info_p->requested_block_size;

        /* If the user is requesting 1 for opt block size, then we will end up
         * giving them the default. 
         *  
         * If the imported block size is the same as the exported block size, 
         * then we do not allow setting the optimum block size. 
         *  
         * If the user is requesting a opt block size that is different than the
         * value we calculated above, then we will do additional calculations 
         * below to determine if this size can be used. 
         */
        if (negotiate_info_p->requested_optimum_block_size != 1 &&
            fbe_ldo_get_imported_block_size(logical_drive_p) != negotiate_info_p->requested_block_size &&
            negotiate_info_p->requested_optimum_block_size != exported_optimum_block_size)
        {
            /* Calculate the exported optimum block size to use.
             */
            exported_optimum_block_size = 
                fbe_ldo_calculate_exported_optimum_block_size(negotiate_info_p->requested_block_size,
                                                              negotiate_info_p->requested_optimum_block_size,
                                                              imported_block_size);

            /* Next, calculate the imported optimum block size for this exported 
             * optimum block size. 
             */
            imported_optimum_block_size = 
                fbe_ldo_calculate_imported_optimum_block_size(negotiate_info_p->requested_block_size,
                                                              exported_optimum_block_size,
                                                              imported_block_size);
        }/* end if changing exported optimum block size */

        /* Save the value we calculated for optimum block size.
         */
        negotiate_info_p->optimum_block_size = exported_optimum_block_size;

        /* Convert from the imported block size capacity to the exported block size capacity.
         */
        capacity = fbe_ldo_map_imported_lba_to_exported_lba(negotiate_info_p->requested_block_size,
                                                            exported_optimum_block_size,
                                                            imported_optimum_block_size,
                                                            capacity);

        /* Round capacity down so it is a multiple of the optimum block size.
         */
        capacity -= (capacity % exported_optimum_block_size);

        negotiate_info_p->block_count = capacity;
        negotiate_info_p->optimum_block_alignment = 0;
        negotiate_info_p->physical_block_size = imported_block_size;
        negotiate_info_p->physical_optimum_block_size = imported_optimum_block_size;
    }
    else
    {
        /* This block size is not supported.
         */
    }
    return status;
}
/* end fbe_ldo_calculate_negotiate_info() */


/*!***************************************************************
 * @fn ldo_calculate_geometry_and_optimum_block_size_for_packet(
 *                          fbe_logical_drive_t * const logical_drive_p,
 *                          fbe_block_edge_geometry_t * block_edge_geometry,
 *                           fbe_block_size_t * optimum_block_size)
 ****************************************************************
 * @brief
 *  This function attempts to calculate the information needed to be passed back
 *  for the negotiate block size command.  If this negotiation fails then this
 *  function will return an error.
 *
 * @param logical_drive_p - The logical drive.
 * @param negotiate_info_p - The buffer to store negotiate info in.
 *
 * @return FBE_STATUS_OK - Indicates that we support this block size.
 *        FBE_STATUS_GENERIC_FAILURE - Indicates this block size is not
 *                                     supported.
 *
 * HISTORY:
 *  05/14/08 - Created. RPF
 *
 ****************************************************************/
static fbe_status_t
ldo_calculate_geometry_and_optimum_block_size_for_packet(fbe_logical_drive_t * const logical_drive_p,
                                 fbe_block_edge_geometry_t * block_edge_geometry,
                                 fbe_block_size_t * optimum_block_size)
{
    fbe_block_edge_geometry_t edge_geometry = FBE_BLOCK_EDGE_GEOMETRY_INVALID;

    if ((block_edge_geometry == NULL) || (optimum_block_size == NULL)){
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* A temporary hack for now. */
	fbe_block_transport_edge_get_geometry(&(logical_drive_p->block_edge), &edge_geometry);
    switch(edge_geometry){
        case FBE_BLOCK_EDGE_GEOMETRY_512_512:
            *block_edge_geometry = FBE_BLOCK_EDGE_GEOMETRY_512_520;
            break;
        case FBE_BLOCK_EDGE_GEOMETRY_520_520:
            *block_edge_geometry = FBE_BLOCK_EDGE_GEOMETRY_520_520;
            break;
        case FBE_BLOCK_EDGE_GEOMETRY_4096_4096:
            *block_edge_geometry = FBE_BLOCK_EDGE_GEOMETRY_4096_520;
            break;
        case FBE_BLOCK_EDGE_GEOMETRY_4160_4160:
            *block_edge_geometry = FBE_BLOCK_EDGE_GEOMETRY_4160_520;
            break;
    }
	fbe_block_transport_get_optimum_block_size(*block_edge_geometry, optimum_block_size);

    return FBE_STATUS_OK;

}
/* end fbe_ldo_calculate_negotiate_info() */

/*************************
 * end file fbe_logical_drive_executer.c
 *************************/
 
 
