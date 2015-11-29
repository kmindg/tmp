/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_cms_lun_access_main.c
***************************************************************************
*
* @brief
*  This file contains persistance service functions related to LUN operations
*  
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_cms_private.h"
#include "fbe/fbe_sector.h"
#include "fbe_cms_lun_access.h"
#include "fbe/fbe_multicore_queue.h"
#include "fbe_cmm.h"
#include "fbe/fbe_xor_api.h"
#include "fbe_private_space_layout.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe_database.h"



/***************/
/*definitions*/
/***************/

typedef enum fbe_cms_lun_access_data_type_e{
	FBE_CMS_LUN_ACCESS_1_BLOCK_IO,
	FBE_CMS_LUN_ACCESS_MULTIPLE_BLOCK_IO,
	FBE_CMS_LUN_ACCESS_1MB_CHUNK_IO,
}fbe_cms_lun_access_data_type_t;

typedef struct fbe_cms_lun_access_io_header_s{
	fbe_queue_element_t				queue_element;/*must stay first for simplicity*/
	fbe_cms_lun_access_data_type_t	data_type;
	fbe_lba_t 						lba;
	fbe_u32_t 						user_data_size_in_bytes;
	fbe_u8_t *						io_buffer;/*the buffer we will use to do the disk IO*/
	fbe_u8_t *						user_data_ptr;/*the user data pointer*/
	fbe_cms_lun_access_completion 	completion_func;
	void *							context;
	fbe_sg_element_t 				sg_list[2];/*we always need only one*/
}fbe_cms_lun_access_io_header_t;

typedef struct fbe_cms_lun_access_small_io_buffer_s{
    fbe_u8_t							data[FBE_BE_BYTES_PER_BLOCK];
}fbe_cms_lun_access_small_io_buffer_t;

typedef struct fbe_cms_lun_access_small_io_memory_s{
    fbe_cms_lun_access_io_header_t				header;/*must stay first for simplicity*/
	fbe_cms_lun_access_small_io_buffer_t		memory;
}fbe_cms_lun_access_small_io_memory_t;

typedef struct fbe_cms_lun_access_big_write_buffer_s{
    fbe_u8_t							data[FBE_BE_BYTES_PER_BLOCK * FBE_CMS_LUN_ACCESS_MAX_BLOCKS_PER_TRANSACTION];
}fbe_cms_lun_access_big_io_buffer_t;

typedef struct fbe_cms_lun_access_big_io_memory_s{
    fbe_cms_lun_access_io_header_t			header;/*must stay first for simplicity*/
	fbe_cms_lun_access_big_io_buffer_t		memory;
}fbe_cms_lun_access_big_io_memory_t;

typedef struct fbe_cms_lun_seq_buffer_header_s{
    fbe_cms_lun_access_io_header_t			header;/*must stay first for simplicity*/
    fbe_u32_t								current_block;/*as the user writs data in, we progress this*/
	fbe_cms_lun_acces_buffer_id_t			buffer_id;
}fbe_cms_lun_seq_buffer_header_t;

typedef struct fbe_cms_lun_access_sequential_buffer_s{
    fbe_u8_t							data[1048576 - sizeof(fbe_cms_lun_seq_buffer_header_t)];/*so it fits nice it a 1MB chunk*/
}fbe_cms_lun_access_sequential_buffer_t;

typedef struct fbe_cms_lun_access_sequential_memory_s{
	fbe_cms_lun_seq_buffer_header_t			seq_header;
	fbe_cms_lun_access_sequential_buffer_t	memroy;
}fbe_cms_lun_access_sequential_memory_t;

typedef struct fbe_cms_lun_access_packet_memory_s{
	fbe_queue_element_t	queue_element;/*must stay first for simplicity*/
	fbe_packet_t packet;
}fbe_cms_lun_access_packet_memory_t;

#define FBE_CMS_LUN_ACCESS_IO_PACKET_CHUNK_COUNT 1 /*1MB will give us more than enough packets*/
#define FBE_CMS_LUN_ACCESS_SEQUENTIAL_CHUNK_COUNT 6 /*6 1Mb chunks for sequential writes*/
#define FBE_CMS_LUN_ACCESS_BIG_WRITE_CHUNK_COUNT 2 /*2 1Mb chunks for big writes*/
#define FBE_CMS_LUN_ACCESS_SMALL_WRITE_CHUNK_COUNT 1 /*1 1Mb chunk for small writes*/
#define FBE_CMS_LUN_ACCESS_SMALL_WRITE_ENTRY_SIZE sizeof(fbe_cms_lun_access_small_io_memory_t)
#define FBE_CMS_LUN_ACCESS_BIG_WRITE_ENTRY_SIZE sizeof(fbe_cms_lun_access_big_io_memory_t)
#define FBE_CMS_LUN_ACCESS_PACKET_ENTRY_SIZE sizeof(fbe_cms_lun_access_packet_memory_t)
#define FBE_CMS_LUN_ACCESS_SMALL_ENTRIES_PER_1MB_CHUNK (1048576 / FBE_CMS_LUN_ACCESS_SMALL_WRITE_ENTRY_SIZE) /*we should have ~1800*/
#define FBE_CMS_LUN_ACCESS_BIG_ENTRIES_PER_1MB_CHUNK (1048576 / FBE_CMS_LUN_ACCESS_BIG_WRITE_ENTRY_SIZE) /*we should have ~30*/
#define FBE_CMS_LUN_ACCESS_PACKETS_PER_1MB_CHUNK (1048576 / FBE_CMS_LUN_ACCESS_PACKET_ENTRY_SIZE) /*we should have ~500*/
#define FBE_CMS_LUN_ACCESS_TOTAL_CHUNK_COUNT FBE_CMS_LUN_ACCESS_SEQUENTIAL_CHUNK_COUNT + FBE_CMS_LUN_ACCESS_BIG_WRITE_CHUNK_COUNT + FBE_CMS_LUN_ACCESS_SMALL_WRITE_CHUNK_COUNT+ FBE_CMS_LUN_ACCESS_IO_PACKET_CHUNK_COUNT

typedef struct fbe_cms_lun_access_local_data_s{
    fbe_multicore_queue_t	small_io_memory_queue_head;/*to hold buffers for up to 512 of user data to be used for write thru of tags or small structures in CDR*/
	fbe_multicore_queue_t	small_io_outstanding_memory_queue_head;/*outstanding for memory_queue_head*/
	fbe_multicore_queue_t	packet_queue_head;/*to hold buffers for up to 512 of user data to be used for write thru of tags or small structures in CDR*/
	fbe_multicore_queue_t	outstanding_packet_queue_head;/*outstanding for memory_queue_head*/
	fbe_queue_head_t		sequential_buffer_memory_queue_head;/*to hold 1MB buffers so we can do sequenqial writes*/
	fbe_queue_head_t		sequential_buffer_outstanding_memory_queue_head;/*outstanding for sequential_buffer_memory_queue_head*/
	fbe_spinlock_t 			sequential_buffer_spinlock;/*lock for big_buffer_memory_queue_head and big_buffer_outstanding_memory_queue_head*/
	fbe_multicore_queue_t	big_io_memory_queue_head;/*to hold buffers for up to 512 * FBE_CMS_LUN_ACCESS_MAX_BLOCKS_PER_TRANSACTION of user data to be used for write thru or vaulting of buffers*/
	fbe_multicore_queue_t	big_io_outstanding_memory_queue_head;/*outstanding big_io_memory_queue_head*/
    CMM_POOL_ID				cmm_control_pool_id;
	CMM_CLIENT_ID			cmm_client_id;
	void *					cmm_chunk_addr[FBE_CMS_LUN_ACCESS_TOTAL_CHUNK_COUNT];
	fbe_cms_lun_acces_buffer_id_t			buffer_id;
}fbe_cms_lun_access_local_data_t;

/***************/
/*Local var.*/
/***************/
static fbe_cms_lun_access_local_data_t	fbe_cms_lun_access_data;
static fbe_object_id_t fbe_cms_lun_access_lun_map_table[] = 
{
	FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CMS_TAGS_LUN,/*= FBE_CMS_TAGS_LUN*/
	FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CMS_CDR_LUN,/*= FBE_CMS_CDR_LUN*/
	FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VCX_LUN_0/* = FBE_CMS_VAULT_LUN (we steal this lun until we have our own)*/
};

/***************/
/*declerations*/
/***************/
static fbe_status_t fbe_cms_lun_access_init_memory(void);
static fbe_status_t fbe_cms_lun_access_destroy_memory(void);

static fbe_status_t fbe_cms_lun_access_write_buffer(fbe_cpu_id_t cpu_id,
													fbe_cms_lun_type_t lun_type,
													fbe_cms_lun_access_io_header_t *write_header);

static fbe_status_t fbe_cms_lun_access_read_buffer(fbe_cpu_id_t cpu_id,
													fbe_cms_lun_type_t lun_type,
													fbe_cms_lun_access_io_header_t *read_header);

static fbe_packet_t * fbe_cms_lun_access_get_packet(fbe_cpu_id_t cpu_id);
static fbe_status_t fbe_cms_lun_access_write_buffer_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_cms_lun_access_read_buffer_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static void fbe_cms_lun_access_get_lun_object_id(fbe_cms_lun_type_t lun_type, fbe_object_id_t *lun_id);
static void fbe_cms_lun_access_wait_for_db_ready_state(void);
static void fbe_cms_lun_access_return_packet(fbe_packet_t *packet_p, fbe_cpu_id_t cpu_id);
static __forceinline fbe_cms_lun_access_packet_memory_t *  fbe_cms_lun_access_packet_to_packet_memory(fbe_packet_t *packet_p);
static void fbe_cms_return_io_memory(fbe_cms_lun_access_io_header_t *header, fbe_cpu_id_t cpu_id);

static fbe_status_t fbe_cms_lun_access_do_single_io(fbe_payload_block_operation_opcode_t operation,
													fbe_cms_lun_type_t lun_type,
													fbe_lba_t lba,
													fbe_u32_t data_size_in_bytes,
													fbe_u8_t * user_data_ptr,
													fbe_cms_lun_access_completion completion_func,
													void *context);

static fbe_cms_lun_access_sequential_memory_t * fbe_cms_lun_access_get_seq_buffer_ptr(fbe_cms_lun_acces_buffer_id_t buffer_id);
static fbe_status_t fbe_cms_lun_access_seq_buffer_commit_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_cms_lun_access_seq_buffer_read_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);


/********************************************************************************************************************************/
fbe_status_t fbe_cms_lun_access_init(void)
{
	
	fbe_cms_lun_access_init_memory();/*allocate the strucutres we'll use to persist the data*/

	/*let's wait for the DB service to be in ready state so we can use it's luns*/
	fbe_cms_lun_access_wait_for_db_ready_state();


	return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_lun_access_destroy(void)
{
	fbe_cms_lun_access_destroy_memory();
	return FBE_STATUS_OK;
}

/*
Persist a piece of data on a specific lun in a requested lba

lun_type - which of the CMS luns we write into,
lba - where on this LUN,
data_size_in_bytes how may bytes we want to write,
data_ptr - Virtual data address,
completion_func - completion function,
context - Completion context
*/
fbe_status_t fbe_cms_lun_access_persist_single_entry(fbe_cms_lun_type_t lun_type,
													 fbe_lba_t lba,
													 fbe_u32_t data_size_in_bytes,
													 fbe_u8_t * user_data_ptr,
													 fbe_cms_lun_access_completion completion_func,
													 void *context)

{
    return fbe_cms_lun_access_do_single_io(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
										   lun_type,
										   lba,
										   data_size_in_bytes,
										   user_data_ptr,
										   completion_func,
										   context);



}

static fbe_status_t fbe_cms_lun_access_init_memory(void)
{
    fbe_u32_t									chunk = 0;
	CMM_ERROR 									cmm_error;
	fbe_cpu_id_t								cpu_count = 0;
	fbe_cpu_id_t								current_cpu = 0;
	fbe_u32_t									structure_per_core = 0, current_structure = 0;
	fbe_cms_lun_access_sequential_memory_t *	seq_ptr = NULL;
	fbe_cms_lun_access_big_io_memory_t *		big_write_ptr = NULL;
	fbe_cms_lun_access_small_io_memory_t *	small_write_ptr = NULL;
	fbe_cms_lun_access_packet_memory_t *		packet_ptr = NULL;

	cpu_count = fbe_get_cpu_count();

    /*first of all, initialize all resources*/
	fbe_multicore_queue_init(&fbe_cms_lun_access_data.small_io_memory_queue_head);
	fbe_multicore_queue_init(&fbe_cms_lun_access_data.small_io_outstanding_memory_queue_head);
	fbe_multicore_queue_init(&fbe_cms_lun_access_data.big_io_memory_queue_head);
	fbe_multicore_queue_init(&fbe_cms_lun_access_data.big_io_outstanding_memory_queue_head);
	fbe_multicore_queue_init(&fbe_cms_lun_access_data.packet_queue_head);
	fbe_multicore_queue_init(&fbe_cms_lun_access_data.outstanding_packet_queue_head);
	fbe_queue_init(&fbe_cms_lun_access_data.sequential_buffer_memory_queue_head);
	fbe_queue_init(&fbe_cms_lun_access_data.sequential_buffer_outstanding_memory_queue_head);
	fbe_spinlock_init(&fbe_cms_lun_access_data.sequential_buffer_spinlock);
	fbe_cms_lun_access_data.buffer_id = 0;
	
    /*let's carve the memory first from CMM*/
    fbe_cms_lun_access_data.cmm_control_pool_id = cmmGetLongTermControlPoolID();
    cmmRegisterClient(fbe_cms_lun_access_data.cmm_control_pool_id, 
                      NULL, 
                      0,  /* Minimum size allocation */  
                      CMM_MAXIMUM_MEMORY,   
                      "FBE CMS LUN Access Memory", 
                       &fbe_cms_lun_access_data.cmm_client_id);

    for (chunk = 0; chunk < FBE_CMS_LUN_ACCESS_TOTAL_CHUNK_COUNT; chunk ++) {
        /* Allocate a 1MB chunk*/
        cmm_error = cmmGetMemoryVA(fbe_cms_lun_access_data.cmm_client_id, 1048576, &fbe_cms_lun_access_data.cmm_chunk_addr[chunk]);
        if (cmm_error != CMM_STATUS_SUCCESS) {
            cms_trace (FBE_TRACE_LEVEL_ERROR, "%s can't get cmm memory, err:%X\n", __FUNCTION__, cmm_error);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

	/*now, let's start using them, start with the big sequential chunks*/
	for (chunk = 0; chunk < FBE_CMS_LUN_ACCESS_SEQUENTIAL_CHUNK_COUNT; chunk ++) {
        seq_ptr = (fbe_cms_lun_access_sequential_memory_t *)fbe_cms_lun_access_data.cmm_chunk_addr[chunk];
		fbe_queue_push(&fbe_cms_lun_access_data.sequential_buffer_memory_queue_head, &seq_ptr->seq_header.header.queue_element);
    }

	/*and move to the big writes, we split them across the cores*/
    structure_per_core = FBE_CMS_LUN_ACCESS_BIG_ENTRIES_PER_1MB_CHUNK / cpu_count;/*how may to assign per core*/
	for (/*leave her empty so we progress from previou one*/; chunk < FBE_CMS_LUN_ACCESS_SEQUENTIAL_CHUNK_COUNT + FBE_CMS_LUN_ACCESS_BIG_WRITE_CHUNK_COUNT; chunk ++) {
		big_write_ptr = (fbe_cms_lun_access_big_io_memory_t *) fbe_cms_lun_access_data.cmm_chunk_addr[chunk];/*this is where our memory start*/
		for (current_cpu = 0; current_cpu < cpu_count; current_cpu++) {
			for (current_structure = 0; current_structure < structure_per_core; current_structure++) {
				fbe_multicore_queue_push(&fbe_cms_lun_access_data.big_io_memory_queue_head,
										 &big_write_ptr->header.queue_element,
										 current_cpu);


				big_write_ptr++;/*skip to next memory address*/
			}
		}
	}

	/*the small write*/
	structure_per_core = FBE_CMS_LUN_ACCESS_SMALL_ENTRIES_PER_1MB_CHUNK / cpu_count;/*how may to assign per core*/
	for (/*leave her empty so we progress from previou one*/; chunk < FBE_CMS_LUN_ACCESS_SEQUENTIAL_CHUNK_COUNT + FBE_CMS_LUN_ACCESS_BIG_WRITE_CHUNK_COUNT + FBE_CMS_LUN_ACCESS_SMALL_WRITE_CHUNK_COUNT; chunk ++) {
		small_write_ptr = (fbe_cms_lun_access_small_io_memory_t *) fbe_cms_lun_access_data.cmm_chunk_addr[chunk];/*this is where our memory start*/
		for (current_cpu = 0; current_cpu < cpu_count; current_cpu++) {
			for (current_structure = 0; current_structure < structure_per_core; current_structure++) {
				fbe_multicore_queue_push(&fbe_cms_lun_access_data.small_io_memory_queue_head,
										 &small_write_ptr->header.queue_element,
										 current_cpu);


				small_write_ptr++;/*skip to next memory address*/
			}
		}
	}

	/*and some for the packets*/
	structure_per_core = FBE_CMS_LUN_ACCESS_PACKETS_PER_1MB_CHUNK / cpu_count;/*how may to assign per core*/
    for (/*leave her empty so we progress from previou one*/;chunk < FBE_CMS_LUN_ACCESS_TOTAL_CHUNK_COUNT; chunk ++) {
        packet_ptr = (fbe_cms_lun_access_packet_memory_t *)fbe_cms_lun_access_data.cmm_chunk_addr[chunk];
		for (current_cpu = 0; current_cpu < cpu_count; current_cpu++) {
			for (current_structure = 0; current_structure < structure_per_core; current_structure++) {
				fbe_transport_initialize_sep_packet(&packet_ptr->packet);/*all packets are initialized and ready to use. Need to call 'resuse' when finished using them*/
				fbe_multicore_queue_push(&fbe_cms_lun_access_data.packet_queue_head,
										 &packet_ptr->queue_element,
										 current_cpu);


				packet_ptr++;/*skip to next memory address*/
			}
		}
    }

	cms_trace (FBE_TRACE_LEVEL_INFO, "%s Done, used %d CMM 1MB chunks\n",__FUNCTION__, FBE_CMS_LUN_ACCESS_TOTAL_CHUNK_COUNT);
		
	return FBE_STATUS_OK;
}

static fbe_status_t fbe_cms_lun_access_destroy_memory(void)
{
	fbe_u32_t			chunk;
    fbe_cpu_id_t		current_cpu = 0;
	fbe_u32_t			delay_count = 0;
	fbe_cpu_id_t 		cpu_count = fbe_get_cpu_count();
    
    /*check if there are in flight used structures*/
    fbe_cms_wait_for_multicore_queue_empty(&fbe_cms_lun_access_data.big_io_outstanding_memory_queue_head, 5, "Big Writes");
	fbe_cms_wait_for_multicore_queue_empty(&fbe_cms_lun_access_data.outstanding_packet_queue_head, 5, "Packet");
	fbe_cms_wait_for_multicore_queue_empty(&fbe_cms_lun_access_data.small_io_outstanding_memory_queue_head, 5, "Small Writes");
    
	fbe_spinlock_lock(&fbe_cms_lun_access_data.sequential_buffer_spinlock);
	while (!fbe_queue_is_empty(&fbe_cms_lun_access_data.sequential_buffer_outstanding_memory_queue_head)) {
		fbe_spinlock_unlock(&fbe_cms_lun_access_data.sequential_buffer_spinlock);
		fbe_thread_delay(100);
		if (delay_count == 0) {
			cms_trace (FBE_TRACE_LEVEL_WARNING, "%s seq. writes queue not empty, waiting...\n",__FUNCTION__);
		}
		delay_count ++;
		fbe_spinlock_lock(&fbe_cms_lun_access_data.sequential_buffer_spinlock);
	}

    if (!fbe_queue_is_empty(&fbe_cms_lun_access_data.sequential_buffer_outstanding_memory_queue_head)){
		fbe_spinlock_unlock(&fbe_cms_lun_access_data.sequential_buffer_spinlock);
		cms_trace (FBE_TRACE_LEVEL_ERROR, "%s seq. writes queue still not empty,can't wait !\n",__FUNCTION__);
	}else{
		fbe_spinlock_unlock(&fbe_cms_lun_access_data.sequential_buffer_spinlock);
	}

	fbe_multicore_queue_destroy(&fbe_cms_lun_access_data.small_io_memory_queue_head);
	fbe_multicore_queue_destroy(&fbe_cms_lun_access_data.small_io_outstanding_memory_queue_head);
	fbe_multicore_queue_destroy(&fbe_cms_lun_access_data.big_io_memory_queue_head);
	fbe_multicore_queue_destroy(&fbe_cms_lun_access_data.big_io_outstanding_memory_queue_head);
	fbe_multicore_queue_destroy(&fbe_cms_lun_access_data.packet_queue_head);
	fbe_multicore_queue_destroy(&fbe_cms_lun_access_data.outstanding_packet_queue_head);
    fbe_spinlock_destroy(&fbe_cms_lun_access_data.sequential_buffer_spinlock);

	 /*free CMM memory*/
    for (chunk = 0; chunk < FBE_CMS_LUN_ACCESS_TOTAL_CHUNK_COUNT; chunk ++) {
        cmmFreeMemoryVA(fbe_cms_lun_access_data.cmm_client_id, fbe_cms_lun_access_data.cmm_chunk_addr[chunk]);
    }
    
    /*unregister*/
    cmmDeregisterClient(fbe_cms_lun_access_data.cmm_client_id);

	return FBE_STATUS_OK;

}

static fbe_status_t fbe_cms_lun_access_write_buffer(fbe_cpu_id_t cpu_id,
													fbe_cms_lun_type_t lun_type,
													fbe_cms_lun_access_io_header_t *write_header)
{
	fbe_u32_t 								byte_count = write_header->user_data_size_in_bytes;
	fbe_u8_t *								user_data = write_header->user_data_ptr;
	fbe_u8_t *								buffer = write_header->io_buffer;
	fbe_u32_t 								checksum;
	fbe_sg_element_t *						temp_sg = write_header->sg_list;
	fbe_u32_t								total_bytes = 0;
    fbe_packet_t *							packet_p = NULL;
	fbe_payload_ex_t * 					payload;
    fbe_payload_block_operation_t * 		block_operation;
	fbe_block_count_t						block_count = 0;
	fbe_object_id_t							lun_object_id = FBE_OBJECT_ID_INVALID;
    
    /*let's copy the user data to the buffer while calculating checksums*/
	while (byte_count != 0) {
		if (byte_count < FBE_BYTES_PER_BLOCK) {
			fbe_copy_memory(buffer, user_data, byte_count);
			byte_count = 0;
		}else{
			fbe_copy_memory(buffer, user_data, FBE_BYTES_PER_BLOCK);
			byte_count -= FBE_BYTES_PER_BLOCK;
		}

		/*and generate CRC*/
		checksum = fbe_xor_lib_calculate_checksum((fbe_u32_t*)buffer);

		/*get to the end of the block*/
		buffer+= FBE_BYTES_PER_BLOCK;
		user_data+= FBE_BYTES_PER_BLOCK;

		/*copy CRC to end*/
        fbe_copy_memory(buffer, &checksum, sizeof(fbe_u32_t));

		total_bytes += FBE_BE_BYTES_PER_BLOCK;
		buffer+= (FBE_BE_BYTES_PER_BLOCK - FBE_BYTES_PER_BLOCK);/*point to the next block to copy into, right after the checksum*/
		block_count++;
	}


	/*point the SG list to the right place*/
	temp_sg->address = write_header->io_buffer;
	temp_sg->count = total_bytes;
	temp_sg++;
	temp_sg->address = NULL;
	temp_sg->count = 0;

	/*get a packet*/
	packet_p = fbe_cms_lun_access_get_packet(cpu_id);
    
	/*and set it up (it's already initialized)*/
	payload = fbe_transport_get_payload_ex(packet_p);
    if (payload == NULL) {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Can't get payload\n", __FUNCTION__);
        fbe_cms_lun_access_return_packet(packet_p, cpu_id);
		write_header->completion_func(write_header->context,
									  FBE_STATUS_GENERIC_FAILURE,
									  FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set up a block operation */
    block_operation = fbe_payload_ex_allocate_block_operation(payload);
    if (block_operation == NULL) {
        cms_trace(FBE_TRACE_LEVEL_ERROR,"%s Can't get block_operation\n", __FUNCTION__);
        fbe_cms_lun_access_return_packet(packet_p, cpu_id);
        write_header->completion_func(write_header->context,
									  FBE_STATUS_GENERIC_FAILURE,
									  FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                      write_header->lba,
                                      block_count,
                                      FBE_BE_BYTES_PER_BLOCK,
                                      0,     /* optimum block size (not used) */
                                      NULL); /* preread descriptor (not used) */

	fbe_payload_ex_increment_block_operation_level(payload);

    /* Put an sgl pointer in the packet payload. */
    fbe_payload_ex_set_sg_list(payload, write_header->sg_list, 2);

    /* Send send down write. */
    fbe_transport_set_completion_function(packet_p, fbe_cms_lun_access_write_buffer_completion, (void *)write_header);

	fbe_cms_lun_access_get_lun_object_id(lun_type, &lun_object_id);
	if (lun_object_id == FBE_OBJECT_ID_INVALID) {
		cms_trace(FBE_TRACE_LEVEL_ERROR,"%s Can't map lun id with type:%d\n", __FUNCTION__, lun_type);
        fbe_cms_lun_access_return_packet(packet_p, cpu_id);
        write_header->completion_func(write_header->context,
									  FBE_STATUS_GENERIC_FAILURE,
									  FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
        return FBE_STATUS_GENERIC_FAILURE;

	}

    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              lun_object_id);

	fbe_transport_set_cpu_id(packet_p, cpu_id);

    return fbe_topology_send_io_packet(packet_p);
}

static fbe_packet_t * fbe_cms_lun_access_get_packet(fbe_cpu_id_t cpu_id)
{
	fbe_cms_lun_access_packet_memory_t *	packet_memory = NULL;

	fbe_multicore_queue_lock(&fbe_cms_lun_access_data.packet_queue_head, cpu_id);

	packet_memory = (fbe_cms_lun_access_packet_memory_t *)fbe_multicore_queue_pop(&fbe_cms_lun_access_data.packet_queue_head, cpu_id);
	fbe_multicore_queue_push(&fbe_cms_lun_access_data.outstanding_packet_queue_head,
							 &packet_memory->queue_element,
							 cpu_id);
	
	fbe_multicore_queue_unlock(&fbe_cms_lun_access_data.packet_queue_head, cpu_id);

	return &packet_memory->packet;
}

static void fbe_cms_lun_access_get_lun_object_id(fbe_cms_lun_type_t lun_type, fbe_object_id_t *lun_id)
{
	*lun_id = fbe_cms_lun_access_lun_map_table[lun_type];
}

static fbe_status_t fbe_cms_lun_access_write_buffer_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t 							status;
    fbe_payload_ex_t *						payload;
    fbe_payload_block_operation_t * 		block_operation;
    fbe_payload_block_operation_status_t 	block_status;
	fbe_cms_lun_access_io_header_t *		write_header = (fbe_cms_lun_access_io_header_t *)context;
	fbe_cpu_id_t							cpu_id;
	fbe_cpu_id_t							current_cpu_id = fbe_get_cpu_id();

	fbe_transport_get_cpu_id(packet_p, &cpu_id);/*need to know on which core to return*/

	/*sanity check, need to handle in the future*/
	if (current_cpu_id != cpu_id) {
		cms_trace(FBE_TRACE_LEVEL_ERROR,"\n\n%s comp. cpu %d and sent cpu %d not the same\n\n", __FUNCTION__,current_cpu_id, cpu_id);
	}

    payload = fbe_transport_get_payload_ex(packet_p);
    if (payload == NULL) {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Can't get payload\n", __FUNCTION__);
        fbe_cms_lun_access_return_packet(packet_p, cpu_id);
		write_header->completion_func(write_header->context,
									  FBE_STATUS_GENERIC_FAILURE,
									  FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
        
        return FBE_STATUS_OK;
    }

    block_operation = fbe_payload_ex_get_block_operation(payload);
    if (block_operation == NULL) {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Can't get io block\n", __FUNCTION__);
        fbe_cms_lun_access_return_packet(packet_p, cpu_id);
		write_header->completion_func(write_header->context,
									  FBE_STATUS_GENERIC_FAILURE,
									  FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
        
        return FBE_STATUS_OK;
    }

    /* Verify that the request completed ok. */
    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK) {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s IO failed\n", __FUNCTION__);
        fbe_cms_lun_access_return_packet(packet_p, cpu_id);
		write_header->completion_func(write_header->context,
									  FBE_STATUS_GENERIC_FAILURE,
									  FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
        return FBE_STATUS_OK;
    }
    
    fbe_payload_block_get_status(block_operation, &block_status);
    if (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s bad status:x%llX\n", __FUNCTION__,
		  (unsigned long long)block_status);
        fbe_cms_lun_access_return_packet(packet_p, cpu_id);
		write_header->completion_func(write_header->context,
									  FBE_STATUS_GENERIC_FAILURE,
									  block_status);
        return FBE_STATUS_OK;
    }

    /* We are done with the operation. */
    fbe_payload_ex_release_block_operation(payload, block_operation);
    fbe_transport_reuse_packet(packet_p);
	write_header->completion_func(write_header->context, FBE_STATUS_OK, block_status);

	/*let's return the resources we claimed to run the IO*/
    fbe_cms_lun_access_return_packet(packet_p, cpu_id);
	fbe_cms_return_io_memory(write_header, cpu_id);


   return FBE_STATUS_OK;
}

static void fbe_cms_lun_access_wait_for_db_ready_state(void)
{
	fbe_database_control_get_state_t	get_state;
	fbe_u32_t							retry_count = 0;

    
   do{
	   cms_send_packet_to_service(FBE_DATABASE_CONTROL_CODE_GET_STATE,
								  &get_state,
								  sizeof(fbe_database_control_get_state_t),
								  FBE_SERVICE_ID_DATABASE,
								  FBE_PACKAGE_ID_SEP_0);

	   if (get_state.state == FBE_DATABASE_STATE_READY) {
		   break;
	   }

       fbe_thread_delay(1000);

	   if (retry_count == 0) {
			cms_trace(FBE_TRACE_LEVEL_INFO,"%s :waiting...\n", __FUNCTION__);
	   }

		retry_count++;
	} while (get_state.state != FBE_DATABASE_STATE_READY && retry_count < 300);

	if (get_state.state != FBE_DATABASE_STATE_READY) {
		cms_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,"%s :DB not ready in 5 minutes !!\n", __FUNCTION__);
	}

}

static void fbe_cms_lun_access_return_packet(fbe_packet_t *packet_p, fbe_cpu_id_t cpu_id)
{
	fbe_cms_lun_access_packet_memory_t *packet_memory;
    
    /*make sure it's ready for next use*/
	fbe_transport_reuse_packet(packet_p);

	/*and see where it the queue element we'll use to return it to*/
	packet_memory = fbe_cms_lun_access_packet_to_packet_memory(packet_p);

	fbe_multicore_queue_lock(&fbe_cms_lun_access_data.packet_queue_head, cpu_id);

	fbe_queue_remove(&packet_memory->queue_element);/*remove from outstanding queue*/

	/*and push to th regular queue*/
	fbe_multicore_queue_push_front(&fbe_cms_lun_access_data.packet_queue_head,
								   &packet_memory->queue_element,
								   cpu_id);	
	
	fbe_multicore_queue_unlock(&fbe_cms_lun_access_data.packet_queue_head, cpu_id);
}

static __forceinline fbe_cms_lun_access_packet_memory_t *  fbe_cms_lun_access_packet_to_packet_memory(fbe_packet_t *packet_p)
{
    return (fbe_cms_lun_access_packet_memory_t *)((fbe_u8_t *)packet_p - (fbe_u8_t *)(&((fbe_cms_lun_access_packet_memory_t *)0)->packet));
}

static void fbe_cms_return_io_memory(fbe_cms_lun_access_io_header_t *header, fbe_cpu_id_t cpu_id)
{
	fbe_multicore_queue_t * 					multicore_q_ptr = NULL;
    fbe_cms_lun_access_small_io_memory_t	*	small_io_memory = NULL;
	fbe_cms_lun_access_big_io_memory_t	*		big_io_memory = NULL;
    fbe_queue_element_t *						queue_element;

	/*based on the header type we'll see where to return it to*/
	switch (header->data_type) {
	case FBE_CMS_LUN_ACCESS_1_BLOCK_IO:
		small_io_memory = (fbe_cms_lun_access_small_io_memory_t	*)header;
		multicore_q_ptr = &fbe_cms_lun_access_data.small_io_memory_queue_head;
		queue_element = &small_io_memory->header.queue_element;
		break;
	case FBE_CMS_LUN_ACCESS_MULTIPLE_BLOCK_IO:
		big_io_memory = (fbe_cms_lun_access_big_io_memory_t	*)header;
		multicore_q_ptr = &fbe_cms_lun_access_data.big_io_memory_queue_head;
		queue_element = &big_io_memory->header.queue_element;
		break;
	default:
		cms_trace(FBE_TRACE_LEVEL_ERROR,"%s:illegal IO type:%d, memory lost !\n", __FUNCTION__, header->data_type);
		return;
	}

	fbe_multicore_queue_lock(multicore_q_ptr, cpu_id);

	fbe_queue_remove(queue_element);/*remove from outstanding queue*/

	/*and push to th regular queue*/
	fbe_multicore_queue_push_front(multicore_q_ptr,
								   queue_element,
								   cpu_id);	
	
	fbe_multicore_queue_unlock(multicore_q_ptr, cpu_id);

}

/*
Persist a piece of data on a specific lun in a requested lba

lun_type - which of the CMS luns we read from,
lba - where on this LUN,
data_size_in_bytes how may bytes we want to read,
data_ptr - Virtual data address,
completion_func - completion function,
context - Completion context
*/
fbe_status_t fbe_cms_lun_access_read_single_entry(fbe_cms_lun_type_t lun_type,
												  fbe_lba_t lba,
												  fbe_u32_t data_size_in_bytes,
												  fbe_u8_t * user_data_ptr,
												  fbe_cms_lun_access_completion completion_func,
												  void *context)

{
	return fbe_cms_lun_access_do_single_io(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
										   lun_type,
										   lba,
										   data_size_in_bytes,
										   user_data_ptr,
										   completion_func,
										   context);
}

static fbe_status_t fbe_cms_lun_access_read_buffer(fbe_cpu_id_t cpu_id,
												   fbe_cms_lun_type_t lun_type,
												   fbe_cms_lun_access_io_header_t *read_header)
{
    fbe_u8_t *								buffer = read_header->io_buffer;
    fbe_sg_element_t *						temp_sg = read_header->sg_list;
	fbe_u32_t								total_bytes = 0;
    fbe_packet_t *							packet_p = NULL;
	fbe_sep_payload_t * 					payload;
    fbe_payload_block_operation_t * 		block_operation;
	fbe_u32_t								block_count = 0;
	fbe_object_id_t							lun_object_id = FBE_OBJECT_ID_INVALID;
    
    /*let's see how many block we have*/
	if (read_header->user_data_size_in_bytes % FBE_BYTES_PER_BLOCK) {
		/*not round*/
		block_count = 1 + (read_header->user_data_size_in_bytes / FBE_BYTES_PER_BLOCK);
	}else{
		block_count = (read_header->user_data_size_in_bytes / FBE_BYTES_PER_BLOCK);
	}

	/*point the SG list to the right place*/
	temp_sg->address = read_header->io_buffer;
	temp_sg->count = block_count * FBE_BE_BYTES_PER_BLOCK;
	temp_sg++;
	temp_sg->address = NULL;
	temp_sg->count = 0;

	/*get a packet*/
	packet_p = fbe_cms_lun_access_get_packet(cpu_id);
    
	/*and set it up (it's already initialized)*/
	payload = fbe_transport_get_sep_payload(packet_p);
    if (payload == NULL) {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Can't get payload\n", __FUNCTION__);
        fbe_cms_lun_access_return_packet(packet_p, cpu_id);
		read_header->completion_func(read_header->context,
									  FBE_STATUS_GENERIC_FAILURE,
									  FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set up a block operation  */
    block_operation = fbe_sep_payload_allocate_block_operation(payload);
    if (block_operation == NULL) {
        cms_trace(FBE_TRACE_LEVEL_ERROR,"%s Can't get block_operation\n", __FUNCTION__);
        fbe_cms_lun_access_return_packet(packet_p, cpu_id);
        read_header->completion_func(read_header->context,
									  FBE_STATUS_GENERIC_FAILURE,
									  FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                      read_header->lba,
                                      block_count,
                                      FBE_BE_BYTES_PER_BLOCK,
                                      0,     /* optimum block size (not used) */
                                      NULL); /* preread descriptor (not used) */

	fbe_sep_payload_increment_block_operation_level(payload);

    /* Put an sgl pointer in the packet payload. */
    fbe_sep_payload_set_sg_list(payload, read_header->sg_list, 2);

    /* Send send down read. */
    fbe_transport_set_completion_function(packet_p, fbe_cms_lun_access_read_buffer_completion, (void *)read_header);

	fbe_cms_lun_access_get_lun_object_id(lun_type, &lun_object_id);
	if (lun_object_id == FBE_OBJECT_ID_INVALID) {
		cms_trace(FBE_TRACE_LEVEL_ERROR,"%s Can't map lun id with type:%d\n", __FUNCTION__, lun_type);
        fbe_cms_lun_access_return_packet(packet_p, cpu_id);
        read_header->completion_func(read_header->context,
									  FBE_STATUS_GENERIC_FAILURE,
									  FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
        return FBE_STATUS_GENERIC_FAILURE;

	}

    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              lun_object_id);

	fbe_transport_set_cpu_id(packet_p, cpu_id);

    return fbe_topology_send_io_packet(packet_p);
}

static fbe_status_t fbe_cms_lun_access_read_buffer_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t 							status;
    fbe_sep_payload_t *						payload;
    fbe_payload_block_operation_t * 		block_operation;
    fbe_payload_block_operation_status_t 	block_status;
	fbe_cms_lun_access_io_header_t *		read_header = (fbe_cms_lun_access_io_header_t *)context;
	fbe_cpu_id_t							cpu_id;
	fbe_u32_t								byte_count = read_header->user_data_size_in_bytes;
	fbe_u8_t *								user_data = read_header->user_data_ptr;
	fbe_u8_t *								buffer = read_header->io_buffer;
	fbe_cpu_id_t							current_cpu_id = fbe_get_cpu_id();

	fbe_transport_get_cpu_id(packet_p, &cpu_id);/*need to know on which core to return*/

	/*sanity check, need to handle in the future*/
	if (current_cpu_id != cpu_id) {
		cms_trace(FBE_TRACE_LEVEL_ERROR,"\n\n%s comp. cpu %d and sent cpu %d not the same\n\n", __FUNCTION__,current_cpu_id, cpu_id);
	}

    payload = fbe_transport_get_sep_payload(packet_p);
    if (payload == NULL) {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Can't get payload\n", __FUNCTION__);
        fbe_cms_lun_access_return_packet(packet_p, cpu_id);
		read_header->completion_func(read_header->context,
									  FBE_STATUS_GENERIC_FAILURE,
									  FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
        
        return FBE_STATUS_OK;
    }

    block_operation = fbe_sep_payload_get_block_operation(payload);
    if (block_operation == NULL) {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Can't get io block\n", __FUNCTION__);
        fbe_cms_lun_access_return_packet(packet_p, cpu_id);
		read_header->completion_func(read_header->context,
									  FBE_STATUS_GENERIC_FAILURE,
									  FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
        
        return FBE_STATUS_OK;
    }

    /* Verify that the request completed ok. */
    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK) {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s IO failed\n", __FUNCTION__);
        fbe_cms_lun_access_return_packet(packet_p, cpu_id);
		read_header->completion_func(read_header->context,
									  FBE_STATUS_GENERIC_FAILURE,
									  FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
        return FBE_STATUS_OK;
    }
    
    fbe_payload_block_get_status(block_operation, &block_status);
    if (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s bad status:x%llX\n", __FUNCTION__,
		  (unsigned long long)block_status);
        fbe_cms_lun_access_return_packet(packet_p, cpu_id);
		read_header->completion_func(read_header->context,
									  FBE_STATUS_GENERIC_FAILURE,
									  block_status);
        return FBE_STATUS_OK;
    }

    /* We are done with the operation. */
    fbe_sep_payload_release_block_operation(payload, block_operation);
    fbe_transport_reuse_packet(packet_p);

	/*now let's copy back the data from the read buffer to the user buffer*/
    while (byte_count != 0) {
		if (byte_count < FBE_BYTES_PER_BLOCK) {
			fbe_copy_memory(user_data, buffer, byte_count);
			byte_count = 0;
		}else{
			fbe_copy_memory(user_data, buffer, FBE_BYTES_PER_BLOCK);
			byte_count -= FBE_BYTES_PER_BLOCK;
		}

        /*get to the end of the block*/
		buffer+= FBE_BE_BYTES_PER_BLOCK;
		user_data+= FBE_BYTES_PER_BLOCK;
	}

	/*and call the user*/
	read_header->completion_func(read_header->context, FBE_STATUS_OK, block_status);

	/*let's return the resources we claimed to run the IO*/
    fbe_cms_lun_access_return_packet(packet_p, cpu_id);
	fbe_cms_return_io_memory(read_header, cpu_id);


   return FBE_STATUS_OK;
}

static fbe_status_t fbe_cms_lun_access_do_single_io(fbe_payload_block_operation_opcode_t operation,
													fbe_cms_lun_type_t lun_type,
													fbe_lba_t lba,
													fbe_u32_t data_size_in_bytes,
													fbe_u8_t * user_data_ptr,
													fbe_cms_lun_access_completion completion_func,
													void *context)

{
	fbe_u8_t *									io_buffer = NULL;
	fbe_cms_lun_access_small_io_memory_t	*	small_io_memory_ptr = NULL;
	fbe_cms_lun_access_big_io_memory_t *		big_io_memory_ptr = NULL;
	fbe_cpu_id_t								current_cpu_id = fbe_get_cpu_id();
	fbe_status_t								status;
	fbe_cms_lun_access_io_header_t *			io_header_p = NULL;
    
	/*let's see which pool to use*/
	if (data_size_in_bytes <= FBE_BYTES_PER_BLOCK) {
		/*let's get the memory for the write*/
		fbe_multicore_queue_lock(&fbe_cms_lun_access_data.small_io_memory_queue_head, current_cpu_id);
		small_io_memory_ptr = (fbe_cms_lun_access_small_io_memory_t *)fbe_multicore_queue_pop(&fbe_cms_lun_access_data.small_io_memory_queue_head, current_cpu_id);

		/*and put it inthe outstanding queue*/
		fbe_multicore_queue_push(&fbe_cms_lun_access_data.small_io_outstanding_memory_queue_head,
								 &small_io_memory_ptr->header.queue_element,
								 current_cpu_id);

		fbe_multicore_queue_unlock(&fbe_cms_lun_access_data.small_io_memory_queue_head, current_cpu_id);

		io_header_p = &small_io_memory_ptr->header;
		io_buffer = small_io_memory_ptr->memory.data;
		small_io_memory_ptr->header.data_type = FBE_CMS_LUN_ACCESS_1_BLOCK_IO;

	}else if ((data_size_in_bytes > FBE_BYTES_PER_BLOCK) && (data_size_in_bytes <= (FBE_BYTES_PER_BLOCK * FBE_CMS_LUN_ACCESS_MAX_BLOCKS_PER_TRANSACTION))) {
		/*let's get the memory for the write*/
		fbe_multicore_queue_lock(&fbe_cms_lun_access_data.big_io_memory_queue_head, current_cpu_id);
		big_io_memory_ptr = (fbe_cms_lun_access_big_io_memory_t *)fbe_multicore_queue_pop(&fbe_cms_lun_access_data.big_io_memory_queue_head, current_cpu_id);

		/*and put it inthe outstanding queue*/
		fbe_multicore_queue_push(&fbe_cms_lun_access_data.big_io_outstanding_memory_queue_head,
								 &big_io_memory_ptr->header.queue_element,
								 current_cpu_id);

		fbe_multicore_queue_unlock(&fbe_cms_lun_access_data.big_io_memory_queue_head, current_cpu_id);

		io_header_p = &big_io_memory_ptr->header;
		io_buffer = big_io_memory_ptr->memory.data;
		big_io_memory_ptr->header.data_type = FBE_CMS_LUN_ACCESS_MULTIPLE_BLOCK_IO;
		
	}else{
		cms_trace (FBE_TRACE_LEVEL_ERROR, "%s Illegal byte count 0x:%llX\n",
			    __FUNCTION__, (unsigned long long)data_size_in_bytes);
		status = FBE_STATUS_GENERIC_FAILURE;
		completion_func(context, FBE_STATUS_GENERIC_FAILURE, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
	}

	/*set up what we want*/
	io_header_p->completion_func = completion_func;
	io_header_p->context = context;
	io_header_p->io_buffer = io_buffer;
	io_header_p->lba = lba;
	io_header_p->user_data_size_in_bytes = data_size_in_bytes;
	io_header_p->user_data_ptr = user_data_ptr;

	/*and send the IO*/
	switch (operation) {
	case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:
		fbe_cms_lun_access_write_buffer(current_cpu_id,
										lun_type,
										io_header_p);
		break;
	case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:
		fbe_cms_lun_access_read_buffer(current_cpu_id,
										lun_type,
										io_header_p);
		break;
	default:
		cms_trace (FBE_TRACE_LEVEL_ERROR, "%s Illegal opcode 0x:%llX\n",
			   __FUNCTION__, (unsigned long long)operation);
		return FBE_STATUS_GENERIC_FAILURE;

	}

	return FBE_STATUS_PENDING;
}


/*how big is the buffer for doing vaulting should be around 1MB, a bit smaller*/
fbe_u32_t fbe_cms_lun_access_get_seq_buffer_block_count(void)
{
	return (sizeof (fbe_cms_lun_access_sequential_buffer_t) / FBE_BYTES_PER_BLOCK);

}

/*get a ~1MB buffer to presist multiple entries at once and then commit them*/
fbe_cms_lun_acces_buffer_id_t fbe_cms_lun_access_get_seq_buffer(void)
{
	fbe_cms_lun_access_sequential_memory_t *	seq_ptr;
    
	fbe_spinlock_lock(&fbe_cms_lun_access_data.sequential_buffer_spinlock);
	if (fbe_queue_is_empty(&fbe_cms_lun_access_data.sequential_buffer_memory_queue_head)) {
		fbe_spinlock_unlock(&fbe_cms_lun_access_data.sequential_buffer_spinlock);
		return FBE_BUFFER_ID_INVALID;
	}

	seq_ptr = (fbe_cms_lun_access_sequential_memory_t *)fbe_queue_pop(&fbe_cms_lun_access_data.sequential_buffer_memory_queue_head);

	seq_ptr->seq_header.buffer_id = fbe_cms_lun_access_data.buffer_id++;/*set and increment for next time*/
	seq_ptr->seq_header.current_block = 0;

	/*and put it in the outstanding queue*/
	fbe_queue_push(&fbe_cms_lun_access_data.sequential_buffer_outstanding_memory_queue_head, &seq_ptr->seq_header.header.queue_element);

	fbe_spinlock_unlock(&fbe_cms_lun_access_data.sequential_buffer_spinlock);

	return seq_ptr->seq_header.buffer_id;
}

fbe_status_t fbe_cms_lun_access_release_seq_buffer(fbe_cms_lun_acces_buffer_id_t buffer_id)
{

	fbe_cms_lun_access_sequential_memory_t *	seq_ptr = fbe_cms_lun_access_get_seq_buffer_ptr(buffer_id);

	if (seq_ptr == NULL) {
		cms_trace (FBE_TRACE_LEVEL_ERROR, "%s can't find buffer ID 0x:%llX\n",
			   __FUNCTION__, (unsigned long long)buffer_id);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_spinlock_lock(&fbe_cms_lun_access_data.sequential_buffer_spinlock);
	fbe_queue_remove(&seq_ptr->seq_header.header.queue_element);/*remove from outstanding*/
	seq_ptr->seq_header.header.completion_func = NULL;/*just in case someone forgot to set it and used this buffer, this is against a programming error*/
	fbe_queue_push(&fbe_cms_lun_access_data.sequential_buffer_memory_queue_head, &seq_ptr->seq_header.header.queue_element);
	fbe_spinlock_unlock(&fbe_cms_lun_access_data.sequential_buffer_spinlock);
    
	return FBE_STATUS_OK;
}

static fbe_cms_lun_access_sequential_memory_t * fbe_cms_lun_access_get_seq_buffer_ptr(fbe_cms_lun_acces_buffer_id_t buffer_id)
{
	fbe_cms_lun_access_sequential_memory_t *	seq_ptr;

	/*go over the queue and look for the buffer*/
	fbe_spinlock_lock(&fbe_cms_lun_access_data.sequential_buffer_spinlock);
	seq_ptr = (fbe_cms_lun_access_sequential_memory_t *)fbe_queue_front(&fbe_cms_lun_access_data.sequential_buffer_outstanding_memory_queue_head);
	while (seq_ptr != NULL) {
		if (seq_ptr->seq_header.buffer_id == buffer_id) {
			fbe_spinlock_unlock(&fbe_cms_lun_access_data.sequential_buffer_spinlock);
			return seq_ptr;
		}

		/*go to next one*/
		seq_ptr = (fbe_cms_lun_access_sequential_memory_t *)fbe_queue_next(&fbe_cms_lun_access_data.sequential_buffer_outstanding_memory_queue_head,
																		   &seq_ptr->seq_header.header.queue_element);
	}
    
	fbe_spinlock_unlock(&fbe_cms_lun_access_data.sequential_buffer_spinlock);

	return NULL;
}

/*we write the user data into the buffer in the next free block. this is not persisted to the disk yet*/
fbe_status_t fbe_cms_lun_access_seq_buffer_write(fbe_cms_lun_acces_buffer_id_t buffer_id,
												 fbe_u8_t *data_ptr,
												 fbe_u32_t data_lengh)
{
	fbe_cms_lun_access_sequential_memory_t * 	buffer_ptr = fbe_cms_lun_access_get_seq_buffer_ptr(buffer_id);
	fbe_u32_t 									byte_count = data_lengh;
	fbe_u8_t *									user_data = data_ptr;
	fbe_u8_t *									buffer = NULL;
	fbe_u32_t 									checksum;
    fbe_u32_t									block_count = 0;
	fbe_u32_t									seq_buf_size = fbe_cms_lun_access_get_seq_buffer_block_count();

	if (buffer_ptr == NULL) {
		cms_trace (FBE_TRACE_LEVEL_ERROR, "%s can't find buffer ID 0x:%llX\n",
			    __FUNCTION__, (unsigned long long)buffer_id);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	/*shift to the right location(start of the next data segment)*/
	buffer = (fbe_u8_t *)((buffer_ptr->memroy.data + (buffer_ptr->seq_header.current_block * FBE_BE_BYTES_PER_BLOCK)));

	/*sanity check*/
	if (buffer_ptr->seq_header.current_block >= seq_buf_size) {
		cms_trace (FBE_TRACE_LEVEL_ERROR, "%s user tries to write beyons block 0x:%llX\n",
			   __FUNCTION__, (unsigned long long)seq_buf_size);
		return FBE_STATUS_GENERIC_FAILURE;
	}
    
    /*let's copy the user data to the buffer while calculating checksums*/
	while (byte_count != 0) {
		if (byte_count < FBE_BYTES_PER_BLOCK) {
			fbe_copy_memory(buffer, user_data, byte_count);
			byte_count = 0;
		}else{
			fbe_copy_memory(buffer, user_data, FBE_BYTES_PER_BLOCK);
			byte_count -= FBE_BYTES_PER_BLOCK;
		}

		/*and generate CRC*/
		checksum = fbe_xor_lib_calculate_checksum((fbe_u32_t*)buffer);

		/*get to the end of the block*/
		buffer+= FBE_BYTES_PER_BLOCK;
		user_data+= FBE_BYTES_PER_BLOCK;

		/*copy CRC to end*/
        fbe_copy_memory(buffer, &checksum, sizeof(fbe_u32_t));

        buffer+= (FBE_BE_BYTES_PER_BLOCK - FBE_BYTES_PER_BLOCK);/*point to the next block to copy into, right after the checksum*/
		block_count++;
	}

	buffer_ptr->seq_header.current_block += block_count;/*point for next time*/

	return FBE_STATUS_OK;
}

/*we call this function after doing a few fbe_cms_lun_access_seq_buffer_write in a row*/
fbe_status_t fbe_cms_lun_access_seq_buffer_commit(fbe_cms_lun_type_t lun_type,
												  fbe_lba_t lba,
												  fbe_cms_lun_acces_buffer_id_t buffer_id,
												  fbe_cms_lun_access_completion completion_func,
												  void *context)
{
    fbe_sg_element_t *							temp_sg = NULL;
    fbe_packet_t *								packet_p = NULL;
	fbe_sep_payload_t * 						payload;
    fbe_payload_block_operation_t * 			block_operation;
    fbe_object_id_t								lun_object_id = FBE_OBJECT_ID_INVALID;
	fbe_cms_lun_access_sequential_memory_t * 	buffer_ptr = fbe_cms_lun_access_get_seq_buffer_ptr(buffer_id);
	fbe_cms_lun_access_io_header_t *			write_header = &buffer_ptr->seq_header.header;
	fbe_cpu_id_t								current_cpu_id = fbe_get_cpu_id();
    
	/*set up the write header*/
	write_header->lba = lba;
	write_header->completion_func = completion_func;
	write_header->context = context;
	write_header->data_type = FBE_CMS_LUN_ACCESS_1MB_CHUNK_IO;
	write_header->io_buffer = buffer_ptr->memroy.data;
    
    /*point the SG list to the right place*/
	temp_sg = write_header->sg_list;
	temp_sg->address = write_header->io_buffer;
	temp_sg->count = buffer_ptr->seq_header.current_block * FBE_BE_BYTES_PER_BLOCK;
	temp_sg++;
	temp_sg->address = NULL;
	temp_sg->count = 0;

    /*get a packet*/
	packet_p = fbe_cms_lun_access_get_packet(current_cpu_id);
    
	/*and set it up (it's already initialized)*/
	payload = fbe_transport_get_sep_payload(packet_p);
    if (payload == NULL) {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Can't get payload\n", __FUNCTION__);
        fbe_cms_lun_access_return_packet(packet_p, current_cpu_id);
		write_header->completion_func(write_header->context,
									  FBE_STATUS_GENERIC_FAILURE,
									  FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set up a block operation */
    block_operation = fbe_sep_payload_allocate_block_operation(payload);
    if (block_operation == NULL) {
        cms_trace(FBE_TRACE_LEVEL_ERROR,"%s Can't get block_operation\n", __FUNCTION__);
        fbe_cms_lun_access_return_packet(packet_p, current_cpu_id);
        write_header->completion_func(write_header->context,
									  FBE_STATUS_GENERIC_FAILURE,
									  FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                      write_header->lba,
                                      buffer_ptr->seq_header.current_block,
                                      FBE_BE_BYTES_PER_BLOCK,
                                      0,     /* optimum block size (not used) */
                                      NULL); /* preread descriptor (not used) */

	fbe_sep_payload_increment_block_operation_level(payload);

    /* Put an sgl pointer in the packet payload. */
    fbe_sep_payload_set_sg_list(payload, write_header->sg_list, 2);

    /* Send send down write. */
    fbe_transport_set_completion_function(packet_p, fbe_cms_lun_access_seq_buffer_commit_completion, (void *)buffer_ptr);

	fbe_cms_lun_access_get_lun_object_id(lun_type, &lun_object_id);
	if (lun_object_id == FBE_OBJECT_ID_INVALID) {
		cms_trace(FBE_TRACE_LEVEL_ERROR,"%s Can't map lun id with type:%d\n", __FUNCTION__, lun_type);
        fbe_cms_lun_access_return_packet(packet_p, current_cpu_id);
        write_header->completion_func(write_header->context,
									  FBE_STATUS_GENERIC_FAILURE,
									  FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
        return FBE_STATUS_GENERIC_FAILURE;

	}

    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              lun_object_id);

	fbe_transport_set_cpu_id(packet_p, current_cpu_id);

    return fbe_topology_send_io_packet(packet_p);
}

static fbe_status_t fbe_cms_lun_access_seq_buffer_commit_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
	fbe_status_t 							status;
    fbe_sep_payload_t *						payload;
    fbe_payload_block_operation_t * 		block_operation;
    fbe_payload_block_operation_status_t 	block_status;
	fbe_cms_lun_access_sequential_memory_t *buffer_ptr = (fbe_cms_lun_access_sequential_memory_t *)context;
	fbe_cms_lun_access_io_header_t *		write_header = &buffer_ptr->seq_header.header;
	fbe_cpu_id_t							cpu_id;
	fbe_cpu_id_t							current_cpu_id = fbe_get_cpu_id();

	fbe_transport_get_cpu_id(packet_p, &cpu_id);/*need to know on which core to return*/

	/*sanity check, need to handle in the future*/
	if (current_cpu_id != cpu_id) {
		cms_trace(FBE_TRACE_LEVEL_ERROR,"\n\n%s comp. cpu %d and sent cpu %d not the same\n\n", __FUNCTION__,current_cpu_id, cpu_id);
	}

    payload = fbe_transport_get_sep_payload(packet_p);
    if (payload == NULL) {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Can't get payload\n", __FUNCTION__);
        fbe_cms_lun_access_return_packet(packet_p, cpu_id);
		write_header->completion_func(write_header->context,
									  FBE_STATUS_GENERIC_FAILURE,
									  FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
        
        return FBE_STATUS_OK;
    }

    block_operation = fbe_sep_payload_get_block_operation(payload);
    if (block_operation == NULL) {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Can't get io block\n", __FUNCTION__);
        fbe_cms_lun_access_return_packet(packet_p, cpu_id);
		write_header->completion_func(write_header->context,
									  FBE_STATUS_GENERIC_FAILURE,
									  FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
        
        return FBE_STATUS_OK;
    }

    /* Verify that the request completed ok. */
    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK) {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s IO failed\n", __FUNCTION__);
        fbe_cms_lun_access_return_packet(packet_p, cpu_id);
		write_header->completion_func(write_header->context,
									  FBE_STATUS_GENERIC_FAILURE,
									  FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
        return FBE_STATUS_OK;
    }
    
    fbe_payload_block_get_status(block_operation, &block_status);
    if (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s bad status:x%llX\n", __FUNCTION__,
		  (unsigned long long)block_status);
        fbe_cms_lun_access_return_packet(packet_p, cpu_id);
		write_header->completion_func(write_header->context,
									  FBE_STATUS_GENERIC_FAILURE,
									  block_status);
        return FBE_STATUS_OK;
    }

    /* We are done with the operation. */
    fbe_sep_payload_release_block_operation(payload, block_operation);
    fbe_transport_reuse_packet(packet_p);
	write_header->completion_func(write_header->context, FBE_STATUS_OK, block_status);

	/*let's return the resources we claimed to run the IO*/
    fbe_cms_lun_access_return_packet(packet_p, cpu_id);

	/*the memory of the buffer is returned when the user releases the buffer upon completion, this is the propper transaction semantics*/
    
	return FBE_STATUS_OK;


}

/*
lun_type - which lun to read from,
lba - from where to start
data_ptr - where tor read to 
element_size_in_bytes - how big is each element you are reading
number_of_elements - how many (use fbe_cms_lun_access_get_num_of_elements_per_seq_buffer to figure this out)
*/
fbe_status_t fbe_cms_lun_access_seq_buffer_read(fbe_cms_lun_type_t lun_type,
												fbe_lba_t lba,
												fbe_u8_t *data_ptr,
												fbe_u32_t element_size_in_bytes,
												fbe_u32_t number_of_elements,
												fbe_cms_lun_access_completion completion_func,
												void *context)
{
    fbe_sg_element_t *						temp_sg = NULL;
    fbe_packet_t *							packet_p = NULL;
	fbe_sep_payload_t * 					payload;
    fbe_payload_block_operation_t * 		block_operation;
    fbe_object_id_t							lun_object_id = FBE_OBJECT_ID_INVALID;
	fbe_cms_lun_acces_buffer_id_t 			seq_buffer = fbe_cms_lun_access_get_seq_buffer();/*reserve a buffer from the pool*/
	fbe_cms_lun_access_sequential_memory_t *seq_memory = NULL;
	fbe_u32_t								block_per_element;
	fbe_cpu_id_t							cpu_id = fbe_get_cpu_id();


	if (element_size_in_bytes % FBE_BYTES_PER_BLOCK) {
		/*not round*/
		block_per_element = 1 + (element_size_in_bytes / FBE_BYTES_PER_BLOCK);
    }else{
		block_per_element = (element_size_in_bytes / FBE_BYTES_PER_BLOCK);
	}

	/*sanity check*/
	if (number_of_elements > fbe_cms_lun_access_get_num_of_elements_per_seq_buffer(element_size_in_bytes)){
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s too much data\n", __FUNCTION__);
		fbe_cms_lun_access_release_seq_buffer(seq_buffer);
		completion_func(context, FBE_STATUS_GENERIC_FAILURE, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
        return FBE_STATUS_GENERIC_FAILURE;
	}

    /*let's get a 1MB buffer first(TODO, handle if we have non, which should not happen on the read path)*/
	seq_memory = fbe_cms_lun_access_get_seq_buffer_ptr(seq_buffer);

	/*keep some context for later*/
	seq_memory->seq_header.header.user_data_ptr = data_ptr;
	seq_memory->seq_header.header.user_data_size_in_bytes = element_size_in_bytes;/*we'll use it later for copy back*/
	seq_memory->seq_header.header.completion_func = completion_func;
	seq_memory->seq_header.header.context = context;
    
	/*point the SG list to the right place*/
	temp_sg = seq_memory->seq_header.header.sg_list;
	temp_sg->address = seq_memory->memroy.data;
	temp_sg->count = block_per_element * number_of_elements * FBE_BE_BYTES_PER_BLOCK;/*we read the whole 1MB*/
	temp_sg++;
	temp_sg->address = NULL;
	temp_sg->count = 0;

	/*get a packet*/
	packet_p = fbe_cms_lun_access_get_packet(cpu_id);
    
	/*and set it up (it's already initialized)*/
	payload = fbe_transport_get_sep_payload(packet_p);
    if (payload == NULL) {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Can't get payload\n", __FUNCTION__);
        fbe_cms_lun_access_return_packet(packet_p, cpu_id);
		completion_func(context, FBE_STATUS_GENERIC_FAILURE, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set up a block operation  */
    block_operation = fbe_sep_payload_allocate_block_operation(payload);
    if (block_operation == NULL) {
        cms_trace(FBE_TRACE_LEVEL_ERROR,"%s Can't get block_operation\n", __FUNCTION__);
        fbe_cms_lun_access_return_packet(packet_p, cpu_id);
        completion_func(context, FBE_STATUS_GENERIC_FAILURE, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                      lba,
                                      block_per_element * number_of_elements,
                                      FBE_BE_BYTES_PER_BLOCK,
                                      0,     /* optimum block size (not used) */
                                      NULL); /* preread descriptor (not used) */

	fbe_sep_payload_increment_block_operation_level(payload);

    /* Put an sgl pointer in the packet payload. */
    fbe_sep_payload_set_sg_list(payload, seq_memory->seq_header.header.sg_list, 2);

    /* Send send down read. */
    fbe_transport_set_completion_function(packet_p, fbe_cms_lun_access_seq_buffer_read_completion, (void *)seq_memory);

	fbe_cms_lun_access_get_lun_object_id(lun_type, &lun_object_id);
	if (lun_object_id == FBE_OBJECT_ID_INVALID) {
		cms_trace(FBE_TRACE_LEVEL_ERROR,"%s Can't map lun id with type:%d\n", __FUNCTION__, lun_type);
        fbe_cms_lun_access_return_packet(packet_p, cpu_id);
        completion_func(context, FBE_STATUS_GENERIC_FAILURE, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
        return FBE_STATUS_GENERIC_FAILURE;

	}

    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              lun_object_id);

	fbe_transport_set_cpu_id(packet_p, cpu_id);

    return fbe_topology_send_io_packet(packet_p);

	

}

static fbe_status_t fbe_cms_lun_access_seq_buffer_read_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
	fbe_status_t 							status;
    fbe_sep_payload_t *						payload;
    fbe_payload_block_operation_t * 		block_operation;
    fbe_payload_block_operation_status_t 	block_status;
	fbe_cms_lun_access_sequential_memory_t *seq_memory = (fbe_cms_lun_access_sequential_memory_t *)context;
    fbe_cpu_id_t							cpu_id;
	fbe_u32_t								byte_count = seq_memory->seq_header.header.user_data_size_in_bytes;
	fbe_u8_t *								user_data = seq_memory->seq_header.header.user_data_ptr;
	fbe_u8_t *								buffer = seq_memory->memroy.data;
	fbe_cpu_id_t							current_cpu_id = fbe_get_cpu_id();
	fbe_u32_t								blocks_per_element = 0;	
	fbe_u32_t								number_of_elements = 0;
	fbe_u32_t								element;

	fbe_transport_get_cpu_id(packet_p, &cpu_id);/*need to know on which core to return*/

	/*sanity check, need to handle in the future*/
	if (current_cpu_id != cpu_id) {
		cms_trace(FBE_TRACE_LEVEL_ERROR,"\n\n%s comp. cpu %d and sent cpu %d not the same\n\n", __FUNCTION__,current_cpu_id, cpu_id);
	}

    payload = fbe_transport_get_sep_payload(packet_p);
    if (payload == NULL) {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Can't get payload\n", __FUNCTION__);
        fbe_cms_lun_access_return_packet(packet_p, cpu_id);
		seq_memory->seq_header.header.completion_func(seq_memory->seq_header.header.context, FBE_STATUS_GENERIC_FAILURE, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
        return FBE_STATUS_OK;
    }

    block_operation = fbe_sep_payload_get_block_operation(payload);
    if (block_operation == NULL) {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Can't get io block\n", __FUNCTION__);
        fbe_cms_lun_access_return_packet(packet_p, cpu_id);
		seq_memory->seq_header.header.completion_func(seq_memory->seq_header.header.context, FBE_STATUS_GENERIC_FAILURE, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
        return FBE_STATUS_OK;
    }

    /* Verify that the request completed ok. */
    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK) {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s IO failed\n", __FUNCTION__);
        fbe_cms_lun_access_return_packet(packet_p, cpu_id);
		seq_memory->seq_header.header.completion_func(seq_memory->seq_header.header.context, FBE_STATUS_GENERIC_FAILURE, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
        return FBE_STATUS_OK;
    }
    
    fbe_payload_block_get_status(block_operation, &block_status);
    if (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s bad status:x%llX\n", __FUNCTION__,
		  (unsigned long long)block_status);
        fbe_cms_lun_access_return_packet(packet_p, cpu_id);
		seq_memory->seq_header.header.completion_func(seq_memory->seq_header.header.context, FBE_STATUS_GENERIC_FAILURE, block_status);
        return FBE_STATUS_OK;
    }

	/*before relasing the block operaton, let's get how many block we did*/
	if (byte_count % FBE_BYTES_PER_BLOCK) {
		/*not round*/
		blocks_per_element = 1 + (byte_count / FBE_BYTES_PER_BLOCK);
    }else{
		blocks_per_element = (byte_count / FBE_BYTES_PER_BLOCK);
	}

	number_of_elements = (fbe_u32_t)(block_operation->block_count / blocks_per_element);

    /* We are done with the operation, let's copy back but we need to sweat a little for it. */
    fbe_sep_payload_release_block_operation(payload, block_operation);
    fbe_transport_reuse_packet(packet_p);

	/*now let's copy back the data from the read buffer to the user buffer*/
	for (element = 0; element < number_of_elements; element++) {
		byte_count = seq_memory->seq_header.header.user_data_size_in_bytes;
		while (byte_count != 0) {
			if (byte_count < FBE_BYTES_PER_BLOCK) {
				fbe_copy_memory(user_data, buffer, byte_count);
				user_data+= byte_count;
				byte_count = 0;
			}else{
				fbe_copy_memory(user_data, buffer, FBE_BYTES_PER_BLOCK);
				byte_count -= FBE_BYTES_PER_BLOCK;
				user_data+= FBE_BYTES_PER_BLOCK;
			}
	
			/*get to the end of the block*/
			buffer+= FBE_BE_BYTES_PER_BLOCK;
		}
	}

	/*and call the user*/
	seq_memory->seq_header.header.completion_func(seq_memory->seq_header.header.context, FBE_STATUS_OK, block_status);

	/*let's return the resources we claimed to run the IO*/
    fbe_cms_lun_access_return_packet(packet_p, cpu_id);
	fbe_cms_lun_access_release_seq_buffer(seq_memory->seq_header.buffer_id);


   return FBE_STATUS_OK;
}

/*returns how many elements of size element_size you can fit in the big seq buffer*/
fbe_u32_t fbe_cms_lun_access_get_num_of_elements_per_seq_buffer(fbe_u32_t element_size)
{
	fbe_u32_t seq_blocks =  fbe_cms_lun_access_get_seq_buffer_block_count();
	fbe_u32_t block_count = 0;

	if (element_size % FBE_BYTES_PER_BLOCK) {
		/*not round*/
		block_count = 1 + (element_size / FBE_BYTES_PER_BLOCK);
    }else{
		block_count = (element_size / FBE_BYTES_PER_BLOCK);
	}

	return (seq_blocks / block_count);

}

/***********************************************
* end file fbe_cms_lun_access_main.c
***********************************************/
