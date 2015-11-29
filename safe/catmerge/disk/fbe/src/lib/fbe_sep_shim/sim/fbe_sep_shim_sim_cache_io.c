/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2009, 2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_sep_shim_sim_cache_io.c
 ***************************************************************************
 *
 *  Description
 *      Simulation implementation for the SEP shim of cache io handling
 *      
 *    
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_trace_interface.h"
#include "fbe/fbe_library_interface.h"
#include "fbe_service_manager.h"
#include "fbe_topology.h"
#include "fbe/fbe_queue.h"
#include "fbe_sep_shim_private_interface.h"
#include "bvd_interface.h"
#include "simulation/cache_to_mcr_transport.h"
#include "simulation/cache_to_mcr_transport_server_interface.h"

// #include "flare_ioctls.h" It can not be included in simulation
        // Standard WDM IO_STACK_LOCATION::Flags for IRP_MJ_READ/WRITE
        // #define SL_KEY_SPECIFIED                0x01
        // #define SL_OVERRIDE_VERIFY_VOLUME       0x02
        // #define SL_WRITE_THROUGH                0x04  // FUA Force unit access
        // #define SL_FT_SEQUENTIAL_WRITE          0x08  // DPO unimportant to cache

        /* 
         * Setting this flag indicate that the request wants to run a low priority.
         */
        #define SL_K10_LOW_PRIORITY             0x01 

        /*
         * FLARE_SGL_INFO.Flags definition
         */
        /* 
         * Indicates that checksum should be checked in the data. 
         */
        #define SGL_FLAG_VALIDATE_DATA          0x1

        /* 
         * Indicates a write verify opcode should be created. 
         */
        #define SGL_FLAG_VERIFY_BEFORE_WRITE 0x2

        /*
         *FUA Force unit access
         */
        #define SGL_FLAG_WRITE_THROUGH                0x04

        /* 
         * Indicates that it is non cached write and RAID driver should take care of marking
         * LUN as dirty or clean. 
         */
        #define SGL_FLAG_NON_CACHED_WRITE 0x8

/* End of Flags from flare_ioctls.h */



static fbe_status_t
fbe_sep_shim_sim_mj_read_write_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context);

static fbe_status_t
fbe_sep_shim_sim_sgl_read_write_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context);

static fbe_status_t fbe_sep_shim_cache_io_process_mj_read(fbe_sep_shim_io_struct_t * sep_shim_io_struct);
static fbe_status_t fbe_sep_shim_cache_io_process_mj_write(fbe_sep_shim_io_struct_t * sep_shim_io_struct);
static fbe_status_t fbe_sep_shim_cache_io_process_mj_write_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_sep_shim_cache_io_translate_status(fbe_payload_block_operation_t * block_operation, cache_to_mcr_serialized_io_t *  serialized_io);
static fbe_status_t fbe_sep_shim_cache_io_process_mj_read_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_sep_shim_cache_io_process_sgl_write(fbe_sep_shim_io_struct_t * sep_shim_io_struct);
static fbe_status_t fbe_sep_shim_cache_io_process_sgl_read(fbe_sep_shim_io_struct_t * sep_shim_io_struct);
static void * fbe_sep_shim_io_fix_sg_pointers(fbe_u8_t *ioBuffer, fbe_u32_t element_count);
static fbe_status_t fbe_sep_shim_cache_io_process_sgl_write_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_sep_shim_cache_io_process_sgl_read_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t fbe_sep_shim_convert_irp_flags(fbe_payload_block_operation_t * block_operation_p, fbe_irp_stack_flags_t irp_Flags);
static fbe_status_t fbe_sep_shim_convert_sgl_info_flags(fbe_payload_block_operation_t * block_operation_p, UINT_16 sgl_info_flags);

/************************************************************************************************************************************************/

void fbe_sep_shim_sim_mj_read_write(cache_to_mcr_operation_code_t opcode,
                                    cache_to_mcr_serialized_io_t * serialized_io,
                                    server_command_completion_function_t completion_function,
                                    void * completion_context)
{
    /*for all the asynch IOs, we first allocate memory and then we continue processing*/
    fbe_sep_shim_io_struct_t *  sep_shim_io_struct = NULL;
    fbe_status_t                status= FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                   number_of_chunks = 0;
    fbe_memory_chunk_size_t     chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_PACKET;
    fbe_u32_t                   transfer_size = 0;
    fbe_lba_t                   lba = 0;
    fbe_block_count_t           blocks = 0;
    cache_to_mcr_mj_read_write_cmd_t *  read_write_cmd = NULL;

    /* we assume 512 alignment here */
    read_write_cmd = (cache_to_mcr_mj_read_write_cmd_t  *)((fbe_u8_t *)(serialized_io) + sizeof(cache_to_mcr_serialized_io_t));
    lba =  read_write_cmd->lba /(LONGLONG)FBE_BYTES_PER_BLOCK;
    transfer_size = read_write_cmd->byte_count;
    blocks = read_write_cmd->byte_count/ FBE_BYTES_PER_BLOCK;

    if ((transfer_size%FBE_BYTES_PER_BLOCK)!=0)
    {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s protocol error, wrong request size %d\n", 
                        __FUNCTION__, transfer_size);
        serialized_io->status = FBE_STATUS_GENERIC_FAILURE;
        completion_function((fbe_u8_t *)serialized_io, completion_context);
        return;
        
    }

    /* this will include the memory for a packet */
    status = fbe_sep_shim_calculate_allocation_chunks(blocks, &number_of_chunks, &chunk_size);
    
    status = fbe_sep_shim_get_io_structure(serialized_io, &sep_shim_io_struct);
    if (sep_shim_io_struct == NULL) {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s Unable to allocate memory\n", __FUNCTION__);
        serialized_io->status = FBE_STATUS_GENERIC_FAILURE;
        completion_function((fbe_u8_t *)serialized_io, completion_context);
        return;
    }
    
    /*make sure we know which IO caused us to generate this structure*/
    sep_shim_io_struct->associated_io = (void *)serialized_io;

    /*and store the completion functions and context for later completing the IO*/
    serialized_io->completion_context = completion_context;
    serialized_io->completion_function = (void *)completion_function;
    
    /*and allocate a packet*/
    fbe_memory_request_init(&sep_shim_io_struct->memory_request);
    fbe_memory_build_chunk_request(&sep_shim_io_struct->memory_request, 
                                   chunk_size,
                                   number_of_chunks, 
                                   0, /* Lowest resource priority */
                                   FBE_MEMORY_REQUEST_IO_STAMP_INVALID,
                                   (fbe_memory_completion_function_t)fbe_sep_shim_sim_mj_read_write_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                   sep_shim_io_struct);

    status = fbe_memory_request_entry(&sep_shim_io_struct->memory_request);

    if ((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING)) {  
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s bad status from fbe_memory_request_entry:%d \n", __FUNCTION__, status);
        serialized_io->status = FBE_STATUS_GENERIC_FAILURE;
        completion_function((fbe_u8_t *)serialized_io, completion_context);
        return;
    }

    return ;

}

static fbe_status_t
fbe_sep_shim_sim_mj_read_write_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context)
{

    fbe_packet_t *              packet = NULL;
    fbe_memory_header_t *       master_memory_header = NULL;
    fbe_sep_shim_io_struct_t *  sep_shim_io_struct = (fbe_sep_shim_io_struct_t *)context;
    fbe_u8_t                    operation = 0;
    fbe_sg_element_t *          sg_list_p;
    fbe_memory_header_t *       current_memory_header = NULL;
    fbe_u32_t                   transfer_size = 0;
    fbe_u8_t *                  buffer_p = NULL;
    fbe_u32_t                   sg_list_size = 0;
    fbe_u32_t                   current_size = 0;
    fbe_u32_t                   chunks_used = 0;
    fbe_u32_t                   chunk_tranfer_size;
    fbe_u32_t                   xor_move_size = 0;
    fbe_status_t                fbe_status;
    cache_to_mcr_serialized_io_t * serialized_io;
    server_command_completion_function_t server_completion_func;
    cache_to_mcr_mj_read_write_cmd_t *  read_write_cmd = NULL;


    if ((sep_shim_io_struct == NULL) || (sep_shim_io_struct->associated_io == NULL)){
        /* This should never happen.*/        
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s:Invalid Parameter sep_shim_io_struct 0x%p\n",
                            __FUNCTION__, sep_shim_io_struct);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the io strcture pointer of the original io structure. We need it for error conditions.*/
    serialized_io = (cache_to_mcr_serialized_io_t *)sep_shim_io_struct->associated_io;
    server_completion_func = (server_command_completion_function_t)serialized_io->completion_function;

    /*now we should get a memory chunk we can use for the packet*/
    if (fbe_memory_request_is_allocation_complete(memory_request) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s:insufficient memory\n",__FUNCTION__);
        serialized_io->status = FBE_STATUS_GENERIC_FAILURE;
        server_completion_func((fbe_u8_t *)serialized_io, serialized_io->completion_context);
        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return FBE_STATUS_OK;
    }
    current_memory_header = master_memory_header = memory_request->ptr;
    /* number_of_chunks includes the one for the packet, but sg_list need one entry for termination */
    sg_list_size = (master_memory_header->number_of_chunks+1) * sizeof(fbe_sg_element_t);
    /* round the sg list size to align with 64-bit boundary. */
    sg_list_size += (sg_list_size % sizeof(fbe_u64_t));

    /* prepare memory to do xor move */
    xor_move_size = sizeof(fbe_xor_mem_move_t);
    xor_move_size += (xor_move_size % sizeof(fbe_u64_t));

    /* if packet and sg_list and xor_mov memory can fit in the last chunk */
    if (master_memory_header->memory_chunk_size > (fbe_memory_chunk_size_t)((sizeof(fbe_packet_t)+sg_list_size+xor_move_size)))
    {        
        /* find the last chunk */
        for (chunks_used = 0; chunks_used < master_memory_header->number_of_chunks -1; chunks_used ++)
        {
            current_memory_header = current_memory_header->u.hptr.next_header;
        }

        /* carve packet from the end of the last chunk */
        buffer_p = current_memory_header->data + master_memory_header->memory_chunk_size - sizeof(fbe_packet_t);
        sep_shim_io_struct->packet = (fbe_packet_t *)buffer_p;

        /* carve out sg_list memory above that */
        buffer_p -= sg_list_size;
        sg_list_p = (fbe_sg_element_t *) buffer_p;
        sep_shim_io_struct->context = (void *)sg_list_p;

        /* carve out xor_mov memory above that */
        buffer_p -= xor_move_size;
        sep_shim_io_struct->xor_mem_move_ptr = (void *)buffer_p;
    }
    else
    {
        /*
         * If xor_move_size + packet size is greater than chunck size, the chunck size
         * must be just for the packet.  In this case, we have allocated 2 chuncks to fit
         * everything.  There will be no problem fitting sg_list with 
         * the xor_mov in the same chunk.
         */
        if (master_memory_header->number_of_chunks < 2)
        {
            fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s:insufficient memory chunk %d\n",
                                __FUNCTION__, master_memory_header->number_of_chunks);
            
           serialized_io->status = FBE_STATUS_GENERIC_FAILURE;
           server_completion_func((fbe_u8_t *)serialized_io, serialized_io->completion_context);

            fbe_sep_shim_return_io_structure(sep_shim_io_struct);

            return FBE_STATUS_OK;
        }
        /* find the second to last chunk */
        for (chunks_used = 0; chunks_used < master_memory_header->number_of_chunks -2; chunks_used ++)
        {
            current_memory_header = current_memory_header->u.hptr.next_header;
        }

        /* put sg_list and xor_mov memory in this chunk */
        buffer_p = current_memory_header->data;
        sg_list_p = (fbe_sg_element_t *) buffer_p;
        sep_shim_io_struct->context = (void *)sg_list_p;
        buffer_p += sg_list_size;
        sep_shim_io_struct->xor_mem_move_ptr = (void *)buffer_p;

        /* put packet to the last chunk */
        current_memory_header = current_memory_header->u.hptr.next_header;
        buffer_p = current_memory_header->data;
        sep_shim_io_struct->packet = (fbe_packet_t *)buffer_p;

        buffer_p -= sg_list_size;
    }

    /* Debug trace:
        Saw atleast one instance where the allocated memory pointed to a packet ememory where 
        packet->magic_number was FBE_MAGIC_NUMBER_BASE_PACKET
        */
    if (sep_shim_io_struct->packet->magic_number == FBE_MAGIC_NUMBER_BASE_PACKET){
        /* TODO: Debug this scenario and identify which code path leaked the packet.*/
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: Packet 0x%p. Allocated packet was not previously destroyed??\n",
                                                __FUNCTION__, packet);
        /* We are ignoring this for now since this was supposed to be newly allocated memory.*/
        /* TODO: Debug this later.*/
    }
    fbe_zero_memory(sep_shim_io_struct->packet,sizeof(fbe_packet_t));
    /*init packet here and use later*/
    fbe_status = fbe_transport_initialize_sep_packet(sep_shim_io_struct->packet);
    if (fbe_status != FBE_STATUS_OK) {
        serialized_io->status = FBE_STATUS_GENERIC_FAILURE;
        server_completion_func((fbe_u8_t *)serialized_io, serialized_io->completion_context);
        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return FBE_STATUS_OK;
    }

    if (master_memory_header->memory_chunk_size==FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO)
    {
        chunk_tranfer_size = FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64 * FBE_BE_BYTES_PER_BLOCK;
    }
    else
    {
        chunk_tranfer_size = FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_PACKET * FBE_BE_BYTES_PER_BLOCK;
    }

    switch (serialized_io->user_control_code){
    case CACHE_TO_MCR_TRANSPORT_MJ_READ:
    case CACHE_TO_MCR_TRANSPORT_MJ_WRITE:
    case CACHE_TO_MCR_TRANSPORT_DCA_WRITE:
    case CACHE_TO_MCR_TRANSPORT_DCA_READ:
        /* 512 to 520 */
        read_write_cmd = (cache_to_mcr_mj_read_write_cmd_t  *)((fbe_u8_t *)(serialized_io) + sizeof(cache_to_mcr_serialized_io_t));
        transfer_size = read_write_cmd->byte_count / FBE_BYTES_PER_BLOCK * FBE_BE_BYTES_PER_BLOCK;

        /* get the next memory header. */
        current_memory_header = master_memory_header;
        while ((transfer_size > 0)&& (current_memory_header!=NULL))
        {
            current_size = chunk_tranfer_size ;
            if (current_size>transfer_size)
            {
                current_size = transfer_size;
            }
            buffer_p = current_memory_header->data;
            fbe_sg_element_init(sg_list_p, current_size, buffer_p);
            fbe_sg_element_increment(&sg_list_p);

            transfer_size -= current_size;
            /* next chunk */
            current_memory_header = current_memory_header->u.hptr.next_header;
        }
        fbe_sg_element_terminate(sg_list_p);

        if (transfer_size!=0)
        {
            fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s:more memory needed %d\n",
                                __FUNCTION__, transfer_size);
            serialized_io->status = FBE_STATUS_GENERIC_FAILURE;
            server_completion_func((fbe_u8_t *)serialized_io, serialized_io->completion_context);
            fbe_sep_shim_return_io_structure(sep_shim_io_struct);

            return FBE_STATUS_OK;
        }

        if ((serialized_io->user_control_code == CACHE_TO_MCR_TRANSPORT_MJ_READ) || (serialized_io->user_control_code == CACHE_TO_MCR_TRANSPORT_DCA_READ))
        {
            fbe_status = fbe_sep_shim_cache_io_process_mj_read(sep_shim_io_struct);
        }
        else if ((serialized_io->user_control_code == CACHE_TO_MCR_TRANSPORT_MJ_WRITE) || (serialized_io->user_control_code == CACHE_TO_MCR_TRANSPORT_DCA_WRITE)) 
        {
            fbe_status = fbe_sep_shim_cache_io_process_mj_write(sep_shim_io_struct);
        }
        if (fbe_status != FBE_STATUS_PENDING){
            fbe_sep_shim_return_io_structure(sep_shim_io_struct);
            serialized_io->status = FBE_STATUS_GENERIC_FAILURE;
            server_completion_func((fbe_u8_t *)serialized_io, serialized_io->completion_context);
        }
        break;
    default: 
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s:invalid MajorFunction: %X\n",__FUNCTION__, operation);
        serialized_io->status = FBE_STATUS_GENERIC_FAILURE;
        server_completion_func((fbe_u8_t *)serialized_io, serialized_io->completion_context);

        fbe_sep_shim_return_io_structure(sep_shim_io_struct);

        break;
    
    }

    return FBE_STATUS_OK;

}

static fbe_status_t fbe_sep_shim_cache_io_process_mj_read(fbe_sep_shim_io_struct_t * sep_shim_io_struct)
{

    fbe_status_t                        fbe_status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                      packet_p = NULL;
    fbe_payload_ex_t *                 sep_payload_p = NULL;
    fbe_payload_block_operation_t *     block_operation_p = NULL;
    fbe_sg_element_t *                  sg_p = NULL;
    fbe_u32_t                           transfer_size = 0;
    fbe_lba_t                           lba = 0;
    fbe_block_count_t                   blocks = 0;
    fbe_block_size_t                    exp_block_size = 0, imp_block_size = 0;
    cache_to_mcr_serialized_io_t *      serialized_io = (cache_to_mcr_serialized_io_t *)sep_shim_io_struct->associated_io;
    cache_to_mcr_mj_read_write_cmd_t *  read_write_cmd = (cache_to_mcr_mj_read_write_cmd_t  *)((fbe_u8_t *)(serialized_io) + sizeof(cache_to_mcr_serialized_io_t));
    fbe_block_edge_t *                  bvd_block_edge_ptr = (fbe_block_edge_t *)serialized_io->block_edge;

    /* we assume 512 alignment here */
    read_write_cmd = (cache_to_mcr_mj_read_write_cmd_t  *)((fbe_u8_t *)(serialized_io) + sizeof(cache_to_mcr_serialized_io_t));
    lba =  read_write_cmd->lba /(LONGLONG)FBE_BYTES_PER_BLOCK;
    transfer_size = read_write_cmd->byte_count;
    blocks = read_write_cmd->byte_count/ FBE_BYTES_PER_BLOCK;
    
    packet_p = sep_shim_io_struct->packet;
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_allocate_block_operation(sep_payload_p);
    if (block_operation_p == NULL) {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: Null block operation\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_block_transport_get_exported_block_size(bvd_block_edge_ptr->block_edge_geometry, &exp_block_size);
    fbe_block_transport_get_physical_block_size(bvd_block_edge_ptr->block_edge_geometry, &imp_block_size);

    fbe_status = fbe_payload_block_build_operation(block_operation_p,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                                   lba,
                                                   blocks,
                                                   exp_block_size,
                                                   imp_block_size,
                                                   NULL/*HACK for now, we don't need it*/);

    /* Based on flags in the IRP, set the corresponding flags in the block operation payload.
     */
    fbe_sep_shim_convert_irp_flags(block_operation_p, serialized_io->irp_flags);
    fbe_payload_block_set_flag(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_CHECK_CHECKSUM);

    sg_p = (fbe_sg_element_t *)sep_shim_io_struct->context;
    fbe_payload_ex_set_sg_list(sep_payload_p, sg_p, 0);

    fbe_transport_set_completion_function(packet_p, 
                                          fbe_sep_shim_cache_io_process_mj_read_completion, 
                                          sep_shim_io_struct);


    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              serialized_io->bvd_object_id); 

    /*make sure the packet has our edge*/
    packet_p->base_edge = &bvd_block_edge_ptr->base_edge;

    #if 0/*FIXME, take care of cancellation later*/
    EmcpalIrpMarkPending(PIrp);

    old_cancel_routine = EmcpalIrpCancelRoutineSet(PIrp, sep_handle_cancel_irp_function);
    if (EmcpalIrpIsCancelInProgress(PIrp)){
      old_cancel_routine = EmcpalIrpCancelRoutineSet(PIrp, NULL);
      if (old_cancel_routine) {
         EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_CANCELLED); 
         EmcpalExtIrpInformationFieldSet(PIrp, 0);
         EmcpalIrpCompleteRequest(PIrp);
         return EMCPAL_STATUS_CANCELLED;

      }
    }

    //Store context for cancellation
    EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = (PVOID)packet_p;    
    #endif

    fbe_topology_send_io_packet(packet_p);
    
    return FBE_STATUS_PENDING;
}

static fbe_status_t fbe_sep_shim_cache_io_process_mj_write(fbe_sep_shim_io_struct_t * sep_shim_io_struct)
{
    fbe_status_t                        fbe_status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                      packet_p = NULL;
    fbe_payload_ex_t *                 sep_payload_p = NULL;
    fbe_payload_block_operation_t *     block_operation_p = NULL;
    fbe_sg_element_t *                  sg_p = NULL;
    fbe_u32_t                           transfer_size = 0;
    fbe_lba_t                           lba = 0;
    fbe_block_count_t                   blocks = 0;
    fbe_block_size_t                    exp_block_size = 0, imp_block_size = 0;
    void *                              userbuffer = NULL;
    fbe_sg_element_t                    sg_source[2];   
    fbe_xor_mem_move_t *                xor_mem_move = (fbe_xor_mem_move_t *)sep_shim_io_struct->xor_mem_move_ptr;
    cache_to_mcr_serialized_io_t *      serialized_io = (cache_to_mcr_serialized_io_t *)sep_shim_io_struct->associated_io;
    cache_to_mcr_mj_read_write_cmd_t *  read_write_cmd = (cache_to_mcr_mj_read_write_cmd_t  *)((fbe_u8_t *)(serialized_io) + sizeof(cache_to_mcr_serialized_io_t));
    fbe_u8_t *                          buffer_start = (fbe_u8_t *)((fbe_u8_t *)read_write_cmd + sizeof(cache_to_mcr_mj_read_write_cmd_t));
    fbe_block_edge_t *                  bvd_block_edge_ptr = (fbe_block_edge_t *)serialized_io->block_edge;
    
    lba = read_write_cmd->lba /(LONGLONG)FBE_BYTES_PER_BLOCK;
    transfer_size = read_write_cmd->byte_count;
    blocks = transfer_size/ FBE_BYTES_PER_BLOCK;

    packet_p = sep_shim_io_struct->packet;
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_allocate_block_operation(sep_payload_p);
    if (block_operation_p == NULL) {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: Null block operation\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_block_transport_get_exported_block_size(bvd_block_edge_ptr->block_edge_geometry, &exp_block_size);
    fbe_block_transport_get_physical_block_size(bvd_block_edge_ptr->block_edge_geometry, &imp_block_size);

    fbe_status = fbe_payload_block_build_operation(block_operation_p,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                                   lba,
                                                   blocks,
                                                   exp_block_size,
                                                   imp_block_size,
                                                   NULL/*HACK for now, we don't need it*/);

    /* Based on flags in the IRP, set the corresponding flags in the block operation payload.
     */
    fbe_sep_shim_convert_irp_flags(block_operation_p, serialized_io->irp_flags);

    //1. Setup SGLs - source and dest   
    sg_p = (fbe_sg_element_t *)sep_shim_io_struct->context;
    userbuffer = (void *)buffer_start;
    fbe_sg_element_init(&sg_source[0],transfer_size,userbuffer);
    fbe_sg_element_terminate(&sg_source[1]);

    //2. Do XOR copy from 512 byte buffer to 520 byte buffer.
    fbe_zero_memory(xor_mem_move, sizeof(fbe_xor_mem_move_t));
    xor_mem_move->status = FBE_XOR_STATUS_NO_ERROR;
    // Don't check the checksum (since we are generating it)
    // Don't set FBE_XOR_OPTION_CHK_CRC
    xor_mem_move->option = 0;
    xor_mem_move->disks = 1;
    xor_mem_move->fru[0].source_sg_p = sg_source;
    xor_mem_move->fru[0].dest_sg_p = sg_p;
    xor_mem_move->fru[0].count     = blocks;
    
    fbe_status = fbe_xor_lib_mem_move(xor_mem_move,FBE_XOR_MOVE_COMMAND_MEM512_TO_MEM520,
                                  0,1,0,0,0);
    if ((fbe_status != FBE_STATUS_OK)||
         (xor_mem_move->status != FBE_XOR_STATUS_NO_ERROR))
    {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: fbe_xor_mem_move failed Status: %d , %d\n",
                           __FUNCTION__, fbe_status,xor_mem_move->status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_ex_set_sg_list(sep_payload_p, sg_p, 0);

    fbe_transport_set_completion_function(packet_p, 
                                          fbe_sep_shim_cache_io_process_mj_write_completion,
                                          sep_shim_io_struct);


    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              serialized_io->bvd_object_id); 

    /*make sure the packet has our edge*/
    packet_p->base_edge = &bvd_block_edge_ptr->base_edge;

    /*FIXME - take care of cancellation by setting cancalation on the client first and then handling the opcode for cancellation*/
    #if 0
    old_cancel_routine = EmcpalIrpCancelRoutineSet(PIrp, sep_handle_cancel_irp_function);
    if (EmcpalIrpIsCancelInProgress(PIrp)){
      old_cancel_routine = EmcpalIrpCancelRoutineSet(PIrp, NULL);
      if (old_cancel_routine) {
         EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_CANCELLED); 
         EmcpalExtIrpInformationFieldSet(PIrp, 0);
         EmcpalIrpCompleteRequest(PIrp);
         return EMCPAL_STATUS_CANCELLED;

      }
    }

    //Store context for cancellation
    EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = (PVOID)packet_p;
    #endif

    fbe_topology_send_io_packet(packet_p);
    
    return FBE_STATUS_PENDING;

}

static fbe_status_t fbe_sep_shim_cache_io_process_mj_write_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sep_shim_io_struct_t *              sep_shim_io_struct = (fbe_sep_shim_io_struct_t *)context;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_block_operation_t *         block_operation = NULL;
    fbe_status_t                            fbe_status = FBE_STATUS_GENERIC_FAILURE;
    cache_to_mcr_serialized_io_t *          serialized_io = (cache_to_mcr_serialized_io_t *)sep_shim_io_struct->associated_io;
    server_command_completion_function_t    server_completion_func = (server_command_completion_function_t)serialized_io->completion_function;

    payload = fbe_transport_get_payload_ex (packet);
    block_operation = fbe_payload_ex_get_block_operation(payload);

    fbe_status  = fbe_transport_get_status_code(packet);
    if (fbe_status != FBE_STATUS_CANCELED){
        /* Translate the fbe status to an IRP status.*/
        fbe_sep_shim_cache_io_translate_status(block_operation, serialized_io);
    }else{
        serialized_io->status_information = 0;
        serialized_io->status = EMCPAL_STATUS_CANCELLED;
    }

    #if 0/*FIXME later*/
    EmcpalIrpCancelRoutineSet (PIrp, NULL);
    #endif

    fbe_payload_ex_release_block_operation(payload, block_operation);/*don't need it anymore*/

    server_completion_func((fbe_u8_t *)serialized_io, serialized_io->completion_context);

    fbe_sep_shim_return_io_structure(sep_shim_io_struct);

    return FBE_STATUS_OK;

}

static fbe_status_t fbe_sep_shim_cache_io_translate_status(fbe_payload_block_operation_t * block_operation, cache_to_mcr_serialized_io_t *  serialized_io)
{
    fbe_payload_block_operation_status_t    block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
    fbe_payload_block_operation_qualifier_t block_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;

    /* Fetch the block status and qualifier of the block operation.
     */
    fbe_payload_block_get_status(block_operation, &block_status);
    fbe_payload_block_get_qualifier(block_operation, &block_qualifier);

    /* Handle each FBE status appropriately, setting the corresponding Status 
     * in the IRP.
     */
    switch (block_status)
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS:
            /* Set the Number of bytes transferred.
             */
            serialized_io->status_information = block_operation->block_count * block_operation->block_size;
            serialized_io->status = EMCPAL_STATUS_SUCCESS;
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR:
            /* Set the Number of bytes transferred even though there was an error. This is because
             * media errors still transfer as much data as possible. Locations that were uncorrectable
             * contain invalidated blocks.
             */
            serialized_io->status_information = block_operation->block_count * block_operation->block_size;
            serialized_io->status = EMCPAL_STATUS_DISK_CORRUPT_ERROR;
            break;
            
        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED:

            switch(block_qualifier)
            {
                case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CRC_ERROR:
                    /* The data provided contained a CRC error. No data was transfered.
                     */
                    serialized_io->status_information = 0;
                    serialized_io->status = EMCPAL_STATUS_CRC_ERROR;
                    break;

                default:
                    /* Set the Number of bytes transferred to zero on error.
                    */
                    serialized_io->status_information = 0;
                    serialized_io->status = EMCPAL_STATUS_UNSUCCESSFUL;
                    break;
            }
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED:
            /* Set the Number of bytes transferred.
             */
            serialized_io->status_information = 0;
            serialized_io->status = EMCPAL_STATUS_CANCELLED;
            break;

        default:
            /* Set the Number of bytes transferred to zero on error.
             */
            serialized_io->status_information = 0;
            serialized_io->status = EMCPAL_STATUS_UNSUCCESSFUL;
            break;
    }
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_sep_shim_cache_io_process_mj_read_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sep_shim_io_struct_t *              sep_shim_io_struct = (fbe_sep_shim_io_struct_t *)context;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_block_operation_t *         block_operation = NULL;
    void *                                  userbuffer = NULL;
    fbe_sg_element_t *                      sg_p = NULL;
    fbe_u32_t                               sg_list_count = 0;
    fbe_u32_t                               transfer_size = 0;
    fbe_block_count_t                       blocks = 0;
    fbe_xor_mem_move_t *                    xor_mem_move = (fbe_xor_mem_move_t *)sep_shim_io_struct->xor_mem_move_ptr;
    fbe_status_t                            fbe_status = FBE_STATUS_GENERIC_FAILURE;
    fbe_sg_element_t                        sg_user[2];
    cache_to_mcr_serialized_io_t *          serialized_io = (cache_to_mcr_serialized_io_t *)sep_shim_io_struct->associated_io;
    server_command_completion_function_t    server_completion_func = (server_command_completion_function_t)serialized_io->completion_function;
    cache_to_mcr_mj_read_write_cmd_t *      read_write_cmd = (cache_to_mcr_mj_read_write_cmd_t  *)((fbe_u8_t *)(serialized_io) + sizeof(cache_to_mcr_serialized_io_t));
    fbe_u8_t *                              buffer_start = (fbe_u8_t *)((fbe_u8_t *)read_write_cmd + sizeof(cache_to_mcr_mj_read_write_cmd_t));

    payload = fbe_transport_get_payload_ex (packet);
    block_operation = fbe_payload_ex_get_block_operation(payload);

    fbe_status  = fbe_transport_get_status_code(packet);
    if (fbe_status != FBE_STATUS_CANCELED){
        /* Translate the fbe status to an IRP status.*/
        fbe_sep_shim_cache_io_translate_status(block_operation, serialized_io);
    }else{
        serialized_io->status_information = 0;
        serialized_io->status = EMCPAL_STATUS_CANCELLED;
    }

    fbe_payload_ex_get_sg_list(payload,&sg_p,&sg_list_count);

    if (serialized_io->status == EMCPAL_STATUS_SUCCESS)
       {
        /* Copy READ data to user buffer.*/
        transfer_size = (fbe_u32_t)serialized_io->status_information;
        blocks = transfer_size/FBE_BE_BYTES_PER_BLOCK ;
        //1. Setup SGLs - source and dest   
        userbuffer = buffer_start;

            if((blocks*FBE_BYTES_PER_BLOCK) > FBE_U32_MAX)
            {
                    /* We do not expect the sg element to exceed 32bit limit
                     */
                    fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, 
                                       "%s: sg element count crossing 32bit limit 0x%llx\n",
                                       __FUNCTION__,
				       (unsigned long long)(blocks*FBE_BYTES_PER_BLOCK));
                    serialized_io->status_information = 0;
                    serialized_io->status = EMCPAL_STATUS_UNSUCCESSFUL;
            }
            else
            {
                fbe_sg_element_init(&sg_user[0], (fbe_u32_t)(blocks * FBE_BYTES_PER_BLOCK), userbuffer);
                fbe_sg_element_terminate(&sg_user[1]);
                serialized_io->status_information = blocks * FBE_BYTES_PER_BLOCK; /* Map to user block size*/
        
                //3. Do XOR copy from 512 byte buffer to 520 byte buffer.
                fbe_zero_memory(xor_mem_move, sizeof(fbe_xor_mem_move_t));
                xor_mem_move->status = FBE_XOR_STATUS_NO_ERROR;
                // We have completed a simulated non-cached read.  Check the checksums
                xor_mem_move->option = FBE_XOR_OPTION_CHK_CRC;
                xor_mem_move->disks = 1;
                xor_mem_move->fru[0].source_sg_p = sg_p;
                xor_mem_move->fru[0].dest_sg_p = sg_user; 
                xor_mem_move->fru[0].count     = blocks;
        
                fbe_status = fbe_xor_lib_mem_move(xor_mem_move,FBE_XOR_MOVE_COMMAND_MEM520_TO_MEM512,
                                      0,1,0,0,0);
                if ((fbe_status != FBE_STATUS_OK)||
                    (xor_mem_move->status != FBE_XOR_STATUS_NO_ERROR))
                {
                    fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: fbe_xor_mem_move failed Status: %d , %d\n",
                                       __FUNCTION__, fbe_status,xor_mem_move->status);

                    /* We do not want to pass bogus data to user.
                     * Fail the IO.
                     */
                    serialized_io->status_information = 0;
                    serialized_io->status = EMCPAL_STATUS_UNSUCCESSFUL;
                }
        }
    }

    /*FIXME later*/
    #if 0
    EmcpalIrpCancelRoutineSet (PIrp, NULL);
    #endif

    fbe_payload_ex_release_block_operation(payload, block_operation);

    server_completion_func((fbe_u8_t *)serialized_io, serialized_io->completion_context);

    fbe_sep_shim_return_io_structure(sep_shim_io_struct);

    return FBE_STATUS_OK;
}


void fbe_sep_shim_sim_sgl_read_write(cache_to_mcr_serialized_io_t * serialized_io,
                                     server_command_completion_function_t completion_function,
                                     void * completion_context)
{
    fbe_sep_shim_io_struct_t *  sep_shim_io_struct = NULL;
    fbe_status_t                status;

    status = fbe_sep_shim_get_io_structure(serialized_io, &sep_shim_io_struct);
    if (sep_shim_io_struct == NULL) {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s Unable to allocate memory\n", __FUNCTION__);
        return ;
    }

    /*make sure we know which IO caused us to generate this structure*/
    sep_shim_io_struct->associated_io = (void *)serialized_io;
    serialized_io->completion_function = completion_function;
    serialized_io->completion_context = completion_context;

    /*and allocate a packet*/
    fbe_memory_request_init(&sep_shim_io_struct->memory_request);
    fbe_memory_build_chunk_request(&sep_shim_io_struct->memory_request, 
                                   FBE_MEMORY_CHUNK_SIZE_FOR_PACKET,
                                   1, /* we need one packet */
                                   0, /* Lowest resource priority */
                                   FBE_MEMORY_REQUEST_IO_STAMP_INVALID,
                                   (fbe_memory_completion_function_t)fbe_sep_shim_sim_sgl_read_write_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */ 
                                   sep_shim_io_struct);

    status = fbe_memory_request_entry(&sep_shim_io_struct->memory_request);

    if ((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING)) {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s:failed to ask for memory request\n",__FUNCTION__);
        serialized_io->status = FBE_STATUS_GENERIC_FAILURE;
        serialized_io->status_information = 0;
        completion_function((fbe_u8_t *)serialized_io, completion_context);
    }

    return ;

}

static fbe_status_t
fbe_sep_shim_sim_sgl_read_write_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context)
{
    fbe_memory_header_t *       master_memory_header = NULL;
    fbe_sep_shim_io_struct_t *  sep_shim_io_struct = (fbe_sep_shim_io_struct_t *)context;
    fbe_status_t                status;
    cache_to_mcr_serialized_io_t * serialized_io;
    server_command_completion_function_t server_completion_func;

    if (sep_shim_io_struct == NULL){
        /* This should never happen.*/        
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s:sep_shim_io_struct is NULL.\n",
                            __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (sep_shim_io_struct->associated_io == NULL)
    {
        /* This should never happen.*/        
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: associated_io in sep_shim_io_struct is NULL.\n",
                            __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the io strcture pointer of the original io structure. We need it for error conditions.*/
    serialized_io = (cache_to_mcr_serialized_io_t *)sep_shim_io_struct->associated_io;
    server_completion_func = (server_command_completion_function_t)serialized_io->completion_function;

    if (fbe_memory_request_is_allocation_complete(memory_request) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s:insufficient memory\n",__FUNCTION__);
        serialized_io->status = FBE_STATUS_GENERIC_FAILURE;
        server_completion_func((fbe_u8_t *)serialized_io, serialized_io->completion_context);
        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return FBE_STATUS_OK;
    }

    /*now we should get a memory chunk we can use for the packet*/
    master_memory_header = fbe_memory_get_first_memory_header(memory_request);
    sep_shim_io_struct->packet = (fbe_packet_t *)fbe_memory_header_to_data(master_memory_header);
    fbe_zero_memory(sep_shim_io_struct->packet,sizeof(fbe_packet_t));
    /*init packet here and use later*/
    status = fbe_transport_initialize_sep_packet(sep_shim_io_struct->packet);
    if (status != FBE_STATUS_OK) {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to init packet\n",__FUNCTION__);

        serialized_io->status = FBE_STATUS_GENERIC_FAILURE;
        server_completion_func((fbe_u8_t *)serialized_io, serialized_io->completion_context);
        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch(serialized_io->user_control_code) {
    case CACHE_TO_MCR_TRANSPORT_SGL_READ:
        status = fbe_sep_shim_cache_io_process_sgl_read(sep_shim_io_struct);
        break;
    case CACHE_TO_MCR_TRANSPORT_SGL_WRITE:
        status = fbe_sep_shim_cache_io_process_sgl_write(sep_shim_io_struct);
        break;
    default:
        status = FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    if (status != FBE_STATUS_PENDING) {

        serialized_io->status = FBE_STATUS_GENERIC_FAILURE;
        serialized_io->status_information = 0;

        fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to send %s\n",
                           __FUNCTION__, serialized_io->user_control_code == CACHE_TO_MCR_TRANSPORT_SGL_READ ? "SGL_READ" : "SGL_WRITE");

        server_completion_func((fbe_u8_t *)serialized_io, serialized_io->completion_context);
        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return status;
    }

    return status;
}

static fbe_status_t fbe_sep_shim_cache_io_process_sgl_write(fbe_sep_shim_io_struct_t * sep_shim_io_struct)
{
    fbe_status_t                        fbe_status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                      packet_p = NULL;
    fbe_payload_ex_t *                 sep_payload_p = NULL;
    fbe_payload_block_operation_t *     block_operation_p = NULL;
    void *                              sg_p = NULL;
    fbe_lba_t                           lba = 0;
    fbe_block_count_t                   blocks = 0;
    fbe_block_size_t                    exp_block_size = 0, imp_block_size = 0;
    cache_to_mcr_serialized_io_t *      serialized_io = (cache_to_mcr_serialized_io_t *)sep_shim_io_struct->associated_io;
    cache_to_mcr_sgl_read_write_cmd_t * read_write_cmd = (cache_to_mcr_sgl_read_write_cmd_t  *)((fbe_u8_t *)(serialized_io) + sizeof(cache_to_mcr_serialized_io_t));
    fbe_u8_t *                          buffer_start = (fbe_u8_t *)((fbe_u8_t *)read_write_cmd + sizeof(cache_to_mcr_sgl_read_write_cmd_t));
    fbe_block_edge_t *                  bvd_block_edge_ptr = (fbe_block_edge_t *)serialized_io->block_edge;

    lba = read_write_cmd->lba /FBE_BYTES_PER_BLOCK;
    blocks = read_write_cmd->byte_count / FBE_BYTES_PER_BLOCK;

    packet_p = sep_shim_io_struct->packet;
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_allocate_block_operation(sep_payload_p);
    if (block_operation_p == NULL) {
        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return FBE_STATUS_GENERIC_FAILURE;

    }

    fbe_block_transport_get_exported_block_size(bvd_block_edge_ptr->block_edge_geometry, &exp_block_size);
    fbe_block_transport_get_physical_block_size(bvd_block_edge_ptr->block_edge_geometry, &imp_block_size);

    fbe_status = fbe_payload_block_build_operation(block_operation_p,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                                   lba,
                                                   blocks,
                                                   exp_block_size,
                                                   imp_block_size,
                                                   NULL/*HACK for now, we don't need it*/);

    /* Based on flags in the IRP, set the corresponding flags in the block operation payload.*/
    fbe_sep_shim_convert_irp_flags(block_operation_p, serialized_io->irp_flags);

    /* Set the sg ptr into the packet.*/
    sg_p = fbe_sep_shim_io_fix_sg_pointers(buffer_start, read_write_cmd->sgl_entries);
    fbe_status = fbe_payload_ex_set_sg_list(sep_payload_p, sg_p, 0);

    fbe_transport_set_completion_function(packet_p, 
                                          fbe_sep_shim_cache_io_process_sgl_write_completion, 
                                          sep_shim_io_struct);


    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              serialized_io->bvd_object_id); 

    /*make sure the packet has our edge*/
    packet_p->base_edge = &bvd_block_edge_ptr->base_edge;

    #if 0/*FIXME, take care of cancellation later*/
    EmcpalIrpMarkPending(PIrp);

    old_cancel_routine = EmcpalIrpCancelRoutineSet(PIrp, sep_handle_cancel_irp_function);
    if (EmcpalIrpIsCancelInProgress(PIrp)){
      old_cancel_routine = EmcpalIrpCancelRoutineSet(PIrp, NULL);
      if (old_cancel_routine) {
         EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_CANCELLED); 
         EmcpalExtIrpInformationFieldSet(PIrp, 0);
         EmcpalIrpCompleteRequest(PIrp);
         return EMCPAL_STATUS_CANCELLED;

      }
    }

    //Store context for cancellation
    EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = (PVOID)packet_p;    
    #endif

    fbe_topology_send_io_packet(packet_p);
    
    return FBE_STATUS_PENDING;
}

static fbe_status_t fbe_sep_shim_cache_io_process_sgl_read(fbe_sep_shim_io_struct_t * sep_shim_io_struct)
{
    fbe_status_t                        fbe_status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                      packet_p = NULL;
    fbe_payload_ex_t *                 sep_payload_p = NULL;
    fbe_payload_block_operation_t *     block_operation_p = NULL;
    void *                              sg_p = NULL;
    fbe_lba_t                           lba = 0;
    fbe_block_count_t                   blocks = 0;
    fbe_block_size_t                    exp_block_size = 0, imp_block_size = 0;
    cache_to_mcr_serialized_io_t *      serialized_io = (cache_to_mcr_serialized_io_t *)sep_shim_io_struct->associated_io;
    cache_to_mcr_sgl_read_write_cmd_t * read_write_cmd = (cache_to_mcr_sgl_read_write_cmd_t  *)((fbe_u8_t *)(serialized_io) + sizeof(cache_to_mcr_serialized_io_t));
    fbe_u8_t *                          buffer_start = (fbe_u8_t *)((fbe_u8_t *)read_write_cmd + sizeof(cache_to_mcr_sgl_read_write_cmd_t));
    fbe_block_edge_t *                  bvd_block_edge_ptr = (fbe_block_edge_t *)serialized_io->block_edge;

    lba = read_write_cmd->lba / FBE_BYTES_PER_BLOCK;
    blocks = read_write_cmd->byte_count / FBE_BYTES_PER_BLOCK;

    packet_p = sep_shim_io_struct->packet;
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_allocate_block_operation(sep_payload_p);
    if (block_operation_p == NULL) {
        fbe_sep_shim_return_io_structure(sep_shim_io_struct);
        return FBE_STATUS_GENERIC_FAILURE;

    }

    fbe_block_transport_get_exported_block_size(bvd_block_edge_ptr->block_edge_geometry, &exp_block_size);
    fbe_block_transport_get_physical_block_size(bvd_block_edge_ptr->block_edge_geometry, &imp_block_size);

    fbe_status = fbe_payload_block_build_operation(block_operation_p,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                                   lba,
                                                   blocks,
                                                   exp_block_size,
                                                   imp_block_size,
                                                   NULL/*HACK for now, we don't need it*/);

    /* Based on flags in the IRP, set the corresponding flags in the block operation payload.*/
    fbe_sep_shim_convert_irp_flags(block_operation_p, serialized_io->irp_flags);

    /* Set the sg ptr into the packet.*/
    sg_p = fbe_sep_shim_io_fix_sg_pointers(buffer_start, read_write_cmd->sgl_entries);
    fbe_status = fbe_payload_ex_set_sg_list(sep_payload_p, sg_p, 0);

    fbe_transport_set_completion_function(packet_p, 
                                          fbe_sep_shim_cache_io_process_sgl_read_completion, 
                                          sep_shim_io_struct);


    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              serialized_io->bvd_object_id); 

    /*make sure the packet has our edge*/
    packet_p->base_edge = &bvd_block_edge_ptr->base_edge;

    #if 0/*FIXME, take care of cancellation later*/
    EmcpalIrpMarkPending(PIrp);

    old_cancel_routine = EmcpalIrpCancelRoutineSet(PIrp, sep_handle_cancel_irp_function);
    if (EmcpalIrpIsCancelInProgress(PIrp)){
      old_cancel_routine = EmcpalIrpCancelRoutineSet(PIrp, NULL);
      if (old_cancel_routine) {
         EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_CANCELLED); 
         EmcpalExtIrpInformationFieldSet(PIrp, 0);
         EmcpalIrpCompleteRequest(PIrp);
         return EMCPAL_STATUS_CANCELLED;

      }
    }

    //Store context for cancellation
    EmcpalExtIrpCurrentStackDriverContextGet(PIrp)[0] = (PVOID)packet_p;    
    #endif

    fbe_topology_send_io_packet(packet_p);
    
    return FBE_STATUS_PENDING;
}

static void * fbe_sep_shim_io_fix_sg_pointers(fbe_u8_t *ioBuffer, fbe_u32_t element_count)
{
    fbe_sg_element_t *      sg_element = NULL;
    fbe_sg_element_t *      tmp_sg_element = NULL;
    fbe_sg_element_t *      sg_element_head = NULL;
    fbe_u32_t *             count = (fbe_u32_t *)ioBuffer;/*this is where the first count it*/
    fbe_u32_t               user_sg_count = 0;
    fbe_u8_t *              sg_addr = NULL;
    
    /*if we have 0 at the first entry it means we have no sg list there*/
    if (*count == 0) {
        return NULL;/*no sg list to process*/
    }

    /*since the buffer that comes from cache does not have the actual sg entries but only a list of counts, we need to allocate here our
    own sg list buffer and then free it upon completion. we add one for the zero termination*/
    sg_element_head = (fbe_sg_element_t *)malloc(sizeof(fbe_sg_element_t) * (element_count + 1));
    tmp_sg_element = sg_element_head;

    /*point to where we placed the data from the user which starts exactly after the location of the count values for each entry*/
    sg_addr = (fbe_u8_t *)(ioBuffer + (sizeof(sg_element->count) * (element_count)));

    /*go over the list and fix the pointers. We would need them to point correctly because we would use the address later
    to convert it to a physical address*/
    while (user_sg_count < element_count) {
        tmp_sg_element->address = sg_addr;
        tmp_sg_element->count = *count;

        /*move everyone forward*/
        sg_addr+= tmp_sg_element->count;
        tmp_sg_element++;
        count++;

        user_sg_count++;/*count how many we did*/
    }

    /*null termination*/
    tmp_sg_element->address = NULL;
    tmp_sg_element->count = 0;

   return (void *)sg_element_head;

}

static fbe_status_t fbe_sep_shim_cache_io_process_sgl_write_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sep_shim_io_struct_t *              sep_shim_io_struct = (fbe_sep_shim_io_struct_t *)context;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_block_operation_t *         block_operation = NULL;
    fbe_status_t                            fbe_status = FBE_STATUS_GENERIC_FAILURE;
    cache_to_mcr_serialized_io_t *          serialized_io = (cache_to_mcr_serialized_io_t *)sep_shim_io_struct->associated_io;
    server_command_completion_function_t    server_completion_func = (server_command_completion_function_t)serialized_io->completion_function;

    payload = fbe_transport_get_payload_ex (packet);
    block_operation = fbe_payload_ex_get_block_operation(payload);

    fbe_status  = fbe_transport_get_status_code(packet);
    if (fbe_status != FBE_STATUS_CANCELED){
        /* Translate the fbe status to an IRP status.*/
        fbe_sep_shim_cache_io_translate_status(block_operation, serialized_io);
    }else{
        serialized_io->status_information = 0;
        serialized_io->status = EMCPAL_STATUS_CANCELLED;
    }

    #if 0/*FIXME later*/
    EmcpalIrpCancelRoutineSet (PIrp, NULL);
    #endif

    fbe_payload_ex_release_block_operation(payload, block_operation);/*don't need it anymore*/

    server_completion_func((fbe_u8_t *)serialized_io, serialized_io->completion_context);

    fbe_sep_shim_return_io_structure(sep_shim_io_struct);

    return FBE_STATUS_OK;
    
}

static fbe_status_t fbe_sep_shim_cache_io_process_sgl_read_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{

    fbe_sep_shim_io_struct_t *              sep_shim_io_struct = (fbe_sep_shim_io_struct_t *)context;
    fbe_payload_ex_t *                     payload = (fbe_payload_ex_t *)0x12345;
    fbe_payload_block_operation_t *         block_operation = NULL;
    fbe_status_t                            fbe_status = FBE_STATUS_GENERIC_FAILURE;
    cache_to_mcr_serialized_io_t *          serialized_io = (cache_to_mcr_serialized_io_t *)sep_shim_io_struct->associated_io;
    server_command_completion_function_t    server_completion_func = (server_command_completion_function_t)serialized_io->completion_function;

    payload = fbe_transport_get_payload_ex (packet);
    block_operation = fbe_payload_ex_get_block_operation(payload);

    fbe_status  = fbe_transport_get_status_code(packet);
    if (fbe_status != FBE_STATUS_CANCELED){
        /* Translate the fbe status to an IRP status.*/
        fbe_sep_shim_cache_io_translate_status(block_operation, serialized_io);
    }else{
        serialized_io->status_information = 0;
        serialized_io->status = EMCPAL_STATUS_CANCELLED;
    }

    #if 0/*FIXME later*/
    EmcpalIrpCancelRoutineSet (PIrp, NULL);
    #endif

    fbe_payload_ex_release_block_operation(payload, block_operation);/*don't need it anymore*/

    server_completion_func((fbe_u8_t *)serialized_io, serialized_io->completion_context);

    fbe_sep_shim_return_io_structure(sep_shim_io_struct);

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_sep_shim_convert_irp_flags(fbe_payload_block_operation_t * block_operation_p, fbe_irp_stack_flags_t irp_Flags)
{
    CSX_ASSERT_AT_COMPILE_TIME(sizeof(fbe_irp_stack_flags_t) == sizeof(EMCPAL_IRP_STACK_FLAGS));
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_sep_shim_convert_sgl_info_flags(fbe_payload_block_operation_t * block_operation_p, UINT_16 sgl_info_flags)
{
    /* Based on the sgl info flags, set the appropriate flags in the block operation payload.
     */
    if (!(sgl_info_flags & SGL_FLAG_VALIDATE_DATA))
    {
        /* Block operations by default have the check checksum flag set. If the caller does NOT have SL_VALIDATE_DATA
         * set, clear the check checksum flag.
         */
        fbe_payload_block_clear_flag(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_CHECK_CHECKSUM);
    }

    if (sgl_info_flags & SGL_FLAG_WRITE_THROUGH)
    {
        /* This flag indicates that we should create a non cached write opcode.
         */
        block_operation_p->block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED;
    }

    if (sgl_info_flags & SGL_FLAG_VERIFY_BEFORE_WRITE)
    {
        /* This flag indicates that we should create a write verify opcode.
         */
        block_operation_p->block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE;
    }

    return FBE_STATUS_OK;
}

void fbe_sep_shim_init_waiting_request_data()
{
    /* No Op for simulation */
}

fbe_bool_t fbe_sep_shim_is_waiting_request_queue_empty(fbe_u32_t cpu_id)
{
    /* No Op for Simulation */
    return FBE_TRUE;
}
fbe_status_t fbe_sep_shim_enqueue_request(void *io_request, fbe_u32_t cpu_id)
{
    /* No Op for simulation */
    return FBE_STATUS_OK;
}
void fbe_sep_shim_wake_waiting_request(fbe_u32_t cpu_id)
{
    /* No Op for simulation */
}

void fbe_sep_shim_destroy_waiting_requests()
{
    /* No Op for simulation */
}
