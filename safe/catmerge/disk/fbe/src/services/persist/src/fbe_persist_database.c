#include "fbe_persist_private.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_transport.h"
#include "fbe_transport_memory.h"
#include "fbe_persist_transaction.h"
#include "fbe_persist_database_layout.h"


/*******************/
/*local definitions*/
/*******************/
/*each entry in the array holds a bitmap for 64 entries*/
#define ENTRY_ID_MAP_ENTRY_SIZE 64


/*to read 400 entried at once we'll allocate 100 buffer with 4 entries in each. The buffers have to be contiguos so we can't amke them too long
otherwise we might fail to allocate this memory*/
#define FBE_PERSIST_DB_READ_ENTRIES_PER_BUFFER 4
#define FBE_PERSIST_DB_ENTRIES_TO_READ 400
#define FBE_PERSIST_DB_TOTAL_READ_BUFFERS (FBE_PERSIST_DB_ENTRIES_TO_READ / FBE_PERSIST_DB_READ_ENTRIES_PER_BUFFER)
#define FBE_PERSIST_DB_TOTAL_READ_SG_ENTRIES (FBE_PERSIST_DB_TOTAL_READ_BUFFERS + 1)
#define FBE_PERSIST_DB_BYTES_PER_READ_ENTRY (FBE_PERSIST_BYTES_PER_BLOCK * FBE_PERSIST_BLOCKS_PER_TRAN_ENTRY)

typedef enum fbe_persist_db_read_state_e{
	FBE_PERSIST_DB_READY_TO_READ,
	FBE_PERSIST_DB_READ_IN_PROGRESS,
}fbe_persist_db_read_state_t;


/*******************/
/*local variables*/
/*******************/
static fbe_spinlock_t 					persist_read_lock;
static fbe_persist_db_read_state_t		persist_read_state = FBE_PERSIST_DB_READY_TO_READ;
static fbe_persist_tran_elem_t *		persist_read_buffer[FBE_PERSIST_DB_TOTAL_READ_BUFFERS];
static fbe_sg_element_t					persist_read_buffer_sg[FBE_PERSIST_DB_TOTAL_READ_SG_ENTRIES];
static fbe_u64_t * 						fbe_persist_entry_id_map;/*bitmaps for entries in use or free to be used*/
static fbe_u32_t 						fbe_persist_total_bitmaps_size = 0;
static fbe_packet_t *					persist_db_read_subpacket = NULL;/*we serialize eveything so we need only one*/
static fbe_s32_t						persist_number_of_entries_to_read[FBE_PERSIST_SECTOR_TYPE_LAST];
static fbe_s32_t						persist_total_number_of_entries_read[FBE_PERSIST_SECTOR_TYPE_LAST];
static fbe_u8_t * 						persist_db_header_read_write_buffer = NULL; //[FBE_PERSIST_BYTES_PER_BLOCK];
static fbe_sg_element_t *				persist_db_header_read_write_sgl = NULL; //[2];
static fbe_packet_t *                   persist_db_header_subpacket = NULL;/*used for header persist. As header can only be persisted in transaction and transaction is serialized, so only one unique packet is needed*/
fbe_persist_db_header_t                 persist_db_header;

/*******************/
/*local declerations*/
/*******************/
static void fbe_persist_database_sector_to_ptr(fbe_persist_sector_type_t sector_type, fbe_u64_t **ptr, fbe_u32_t *array_size);
static fbe_status_t fbe_persist_database_sector_start_lba(fbe_persist_sector_type_t sector_type, fbe_lba_t *lba);
static fbe_status_t fbe_persist_database_read_sector_completion(fbe_packet_t * subpacket, fbe_packet_completion_context_t context);
static fbe_status_t persist_database_return_read_data_to_user(fbe_packet_t * user_packet, fbe_persist_control_read_sector_t *read_sector);
static fbe_status_t fbe_persist_database_update_bitmap_completion(fbe_packet_t * subpacket, fbe_packet_completion_context_t context);
static fbe_status_t persist_database_fill_bitmap(fbe_packet_t * user_packet, fbe_persist_sector_type_t sector_type);
static fbe_status_t persist_database_return_single_read_data_to_user(fbe_packet_t * user_packet, fbe_persist_control_read_entry_t *read_entry);
static fbe_status_t fbe_persist_database_read_persist_db_header_completion(fbe_packet_t * subpacket, fbe_packet_completion_context_t context);
static fbe_status_t fbe_persist_database_write_persist_db_header_completion(fbe_packet_t *packe, fbe_packet_completion_context_t contextt);
static fbe_status_t persist_send_update_bitmap_io(fbe_packet_t *packet, fbe_persist_sector_type_t sector_type, fbe_persist_entry_id_t start_entry_id);
static fbe_status_t fbe_persist_database_update_bitmap_continue(fbe_packet_t *packet, fbe_persist_sector_type_t sector_type, fbe_persist_entry_id_t start_entry_id);
/*************************************************************************************************************************************************/
static fbe_status_t persist_destroy_entry_id_map(void)
{
	fbe_memory_ex_release(fbe_persist_entry_id_map);

	return FBE_STATUS_OK;
}

static void
persist_change_read_state(fbe_persist_db_read_state_t read_state)
{
    fbe_spinlock_lock(&persist_read_lock);
    /*we can read again. Tehnically, another user can try and read another sector, but we don't care,
    all the counters are seperated */
    persist_read_state = read_state;
    fbe_spinlock_unlock(&persist_read_lock);
}
/* This local function releases the memory allocated for read buffers. */
static void
persist_read_buffer_destroy(void)
{
    fbe_u32_t	buffer_count = 0;
    
	for (buffer_count = 0; buffer_count < FBE_PERSIST_DB_TOTAL_READ_BUFFERS; buffer_count++) {
		/*release the buffer for FBE_PERSIST_DB_READ_ENTRIES_PER_BUFFER entries*/
		fbe_memory_native_release(persist_read_buffer[buffer_count]);
		
	}

}

/* This local function allocates the memory for read buffers,
 * and initializes the associated tracking structures. */
static fbe_status_t
persist_read_buffer_init(void)
{
    fbe_u32_t	buffer_count = 0;
    
	for (buffer_count = 0; buffer_count < FBE_PERSIST_DB_TOTAL_READ_BUFFERS; buffer_count++) {

		/*allocate the buffer for FBE_PERSIST_DB_READ_ENTRIES_PER_BUFFER entries*/
		persist_read_buffer[buffer_count] = (fbe_persist_tran_elem_t *)fbe_memory_native_allocate(FBE_PERSIST_DB_READ_ENTRIES_PER_BUFFER * sizeof(fbe_persist_tran_elem_t));
		fbe_zero_memory(persist_read_buffer[buffer_count], FBE_PERSIST_DB_READ_ENTRIES_PER_BUFFER * sizeof(fbe_persist_tran_elem_t) );

		/*set sg list for this buffer and point it correctly*/
		fbe_sg_element_init(&persist_read_buffer_sg[buffer_count],
							FBE_PERSIST_DB_READ_ENTRIES_PER_BUFFER * sizeof(fbe_persist_tran_elem_t),
							persist_read_buffer[buffer_count]);
	}

	/*terminate the sg list for*/
	fbe_sg_element_terminate(&persist_read_buffer_sg[buffer_count]);

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_persist_database_init(void)
{
    fbe_status_t status;
	fbe_u32_t sector;

    persist_db_header_read_write_buffer = (fbe_u8_t *)fbe_memory_native_allocate(FBE_PERSIST_BYTES_PER_BLOCK);
    persist_db_header_read_write_sgl    = (fbe_sg_element_t *)fbe_memory_native_allocate(sizeof(fbe_sg_element_t) * 2);

    if((persist_db_header_read_write_buffer == NULL) || (persist_db_header_read_write_sgl == NULL)) {
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    status = persist_read_buffer_init();
    if (status != FBE_STATUS_OK) {
        return status;
    }

	status = persist_init_entry_id_map(&fbe_persist_entry_id_map, &fbe_persist_total_bitmaps_size);
	if (status != FBE_STATUS_OK) {
        return status;
    }

    fbe_spinlock_init(&persist_read_lock);

	persist_db_read_subpacket = fbe_transport_allocate_packet();
	if (persist_db_read_subpacket == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "%s Can't allocate packet\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	status  = fbe_transport_initialize_sep_packet(persist_db_read_subpacket);
	if (status != FBE_STATUS_OK) {
		fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "%s failed to init packet\n", __FUNCTION__);
        fbe_transport_release_packet(persist_db_read_subpacket);
        return FBE_STATUS_GENERIC_FAILURE;
	}

    persist_db_header_subpacket = fbe_transport_allocate_packet();
    if (persist_db_header_subpacket == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "%s Can't allocate packet\n", __FUNCTION__);
        fbe_transport_release_packet(persist_db_read_subpacket);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_transport_initialize_sep_packet(persist_db_header_subpacket);
    if (status != FBE_STATUS_OK) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "%s failed to init packet\n", __FUNCTION__);
        fbe_transport_release_packet(persist_db_read_subpacket);
        fbe_transport_release_packet(persist_db_header_subpacket);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*reaset the read progress counters per sector*/
    for (sector = FBE_PERSIST_SECTOR_TYPE_INVALID; sector < FBE_PERSIST_SECTOR_TYPE_LAST; sector ++) {
        persist_number_of_entries_to_read[sector] = 0;
        persist_total_number_of_entries_read[sector] = 0;
    }

    fbe_zero_memory(&persist_db_header, sizeof(fbe_persist_db_header_t));

    return FBE_STATUS_OK;
}

void
fbe_persist_database_destroy(void)
{
	fbe_persist_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "%s Entry\n", __FUNCTION__);

    while (persist_read_state != FBE_PERSIST_DB_READY_TO_READ) {
		fbe_persist_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "%s waiting for reads to finish\n", __FUNCTION__);
		fbe_thread_delay(100);/*let' wait for IOs on the drive to complete*/
	}

    fbe_spinlock_destroy(&persist_read_lock);
    persist_read_buffer_destroy();
	persist_destroy_entry_id_map();

    while(fbe_queue_is_element_on_queue(&persist_db_read_subpacket->queue_element)){
		fbe_thread_delay(100);/*let' wait for IOs on the drive to complete*/
	}

    if (persist_db_read_subpacket != NULL) {
		fbe_transport_release_packet(persist_db_read_subpacket);
		persist_db_read_subpacket = NULL;
	}

    while(fbe_queue_is_element_on_queue(&persist_db_header_subpacket->queue_element)){
        fbe_thread_delay(100);/*let' wait for IOs on the drive to complete*/
    }

    fbe_transport_release_packet(persist_db_header_subpacket);
    persist_db_header_subpacket = NULL;

    fbe_memory_native_release(persist_db_header_read_write_buffer);
    fbe_memory_native_release(persist_db_header_read_write_sgl);
    
	fbe_persist_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "%s Done\n", __FUNCTION__);

    return;
}

fbe_lba_t
fbe_persist_database_get_tran_journal_lba(void)
{
	/*we leave one block for persist version, signature and so on*/
    return (fbe_lba_t)(FBE_PERSIST_START_LBA + 1);

}

static void
persist_read_buffer_entry_copy_out(fbe_u8_t * src_buffer_p, fbe_u8_t * dst_buffer_p)
{
    fbe_u32_t block_count;
    fbe_u32_t byte_count;

    for (block_count = 0; block_count < FBE_PERSIST_BLOCKS_PER_ENTRY; block_count++) {
        for (byte_count = 0; byte_count < FBE_PERSIST_DATA_BYTES_PER_BLOCK; byte_count++) {
            *dst_buffer_p++ = *src_buffer_p++;
        }
        src_buffer_p += FBE_PERSIST_HEADER_BYTES_PER_BLOCK;
    }
}

/* This local function is called when a read entry completes (in the packet completion context).
 * If the read was successful, this function will strip out the block headers and return the entry
 * in the caller's original request packet. */
static fbe_status_t
fbe_persist_database_read_single_entry_completion(fbe_packet_t * subpacket, fbe_packet_completion_context_t context)
{
    fbe_packet_t * 							request_packet;
    fbe_status_t 							status;
    fbe_payload_ex_t *						payload;
    fbe_payload_block_operation_t * 		block_operation;
    fbe_payload_block_operation_status_t 	block_status;
    fbe_persist_control_read_entry_t *      read_entry = (fbe_persist_control_read_entry_t *)context;
    fbe_u32_t							    total_sector_entries;
    fbe_u64_t							    sector_id_shifted = read_entry->persist_sector_type;

	sector_id_shifted <<= 32;/*prepare it for calculating the next entry id*/
        
    /* Get the original request packet (the one the user sent to set the LUN). */
    request_packet = (fbe_packet_t *)fbe_transport_get_master_packet(subpacket);
    fbe_transport_remove_subpacket(subpacket);

	payload = fbe_transport_get_payload_ex(subpacket);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Can't get payload\n", __FUNCTION__);
        fbe_transport_reuse_packet(subpacket);
        persist_change_read_state(FBE_PERSIST_DB_READY_TO_READ);
        fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

	/* Verify that the read completed ok. */
    block_operation = fbe_payload_ex_get_block_operation(payload);
    if (block_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Can't get block_operation\n", __FUNCTION__);
        fbe_transport_reuse_packet(subpacket);
        persist_change_read_state(FBE_PERSIST_DB_READY_TO_READ);
        fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Verify that the read request subpacket completed ok. */
    status = fbe_transport_get_status_code(subpacket);
    if (status != FBE_STATUS_OK) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s journal read failed\n", __FUNCTION__);
		fbe_payload_ex_release_block_operation(payload, block_operation);
        fbe_transport_reuse_packet(subpacket);
        persist_change_read_state(FBE_PERSIST_DB_READY_TO_READ);
        fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    fbe_payload_block_get_status(block_operation, &block_status);
    if (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {
        fbe_persist_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Block_operation error, status: 0x%X\n", __FUNCTION__, block_status);
		fbe_payload_ex_release_block_operation(payload, block_operation);
        fbe_transport_reuse_packet(subpacket);
        persist_change_read_state(FBE_PERSIST_DB_READY_TO_READ);
        if (block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR) {
            fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_BAD_BLOCK);
        }
        else{ 
            fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0); 
        }
        /*check what we need to do next*/
        status = fbe_persist_database_layout_get_sector_total_entries(read_entry->persist_sector_type, &total_sector_entries);
        if (persist_number_of_entries_to_read[read_entry->persist_sector_type] == total_sector_entries) {
            /*reset everything for next time*/
            persist_number_of_entries_to_read[read_entry->persist_sector_type] = 0;
            /*update user portion*/
            read_entry->next_entry_id = FBE_PERSIST_NO_MORE_ENTRIES_TO_READ; 
        }else{
            /*we need to know what was the last entry id we read in case we need to keep on reading.
		    However, we can't just read it from disk like the one we send to the user because we need
		    to generate the next read LBA off this entry id and it might be invalid since there is no entry there*/
            persist_number_of_entries_to_read[read_entry->persist_sector_type]++;
            read_entry->next_entry_id = persist_number_of_entries_to_read[read_entry->persist_sector_type] | sector_id_shifted;
	}
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* We are done with the entry read operation. */
    fbe_payload_ex_release_block_operation(payload, block_operation);
    fbe_transport_reuse_packet(subpacket);

    persist_number_of_entries_to_read[read_entry->persist_sector_type]++;

    /*we can now complete the read to the user by copying everything from our buffer to the user's buffer*/
    persist_database_return_single_read_data_to_user(request_packet, read_entry);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/* This external function is the starting function for handling a FBE_PERSIST_CONTROL_CODE_READ_ENTRY request.
it is reading a single entry into the user buffer */
fbe_status_t
fbe_persist_database_read_single_entry(fbe_packet_t * request_packet, fbe_persist_control_read_entry_t * control_read)
{
    fbe_packet_t * 						subpacket;
    fbe_payload_ex_t * 				    payload;
    fbe_payload_block_operation_t * 	block_operation;
    fbe_lba_t 							entry_lba;
    fbe_status_t 						status;

	fbe_spinlock_lock(&persist_read_lock); 
	if (persist_read_state != FBE_PERSIST_DB_READY_TO_READ) {
		fbe_spinlock_unlock(&persist_read_lock);
        
        fbe_transport_set_status(request_packet, FBE_STATUS_BUSY, 0);
        fbe_transport_complete_packet(request_packet);
        
		return FBE_STATUS_BUSY;/*we are probably doing a read for another client now*/
	}

	/*good for us, we can read*/
	persist_read_state = FBE_PERSIST_DB_READ_IN_PROGRESS;
	fbe_spinlock_unlock(&persist_read_lock);

	/* Create a subpacket for an I/O that read the entries into the buffers. */
    subpacket = persist_db_read_subpacket;
    
    payload = fbe_transport_get_payload_ex(subpacket);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Can't get payload\n", __FUNCTION__);
        fbe_transport_reuse_packet(subpacket);
        persist_change_read_state(FBE_PERSIST_DB_READY_TO_READ);
        fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set up a block operation */
    block_operation = fbe_payload_ex_allocate_block_operation(payload);
    if (block_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Can't get block_operation\n", __FUNCTION__);
        fbe_transport_reuse_packet(subpacket);
        persist_change_read_state(FBE_PERSIST_DB_READY_TO_READ);
        fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*do we read for a specific entry if or from the start of the sector ?*/
    if (control_read->entry_id == FBE_PERSIST_SECTOR_START_ENTRY_ID) {
        persist_number_of_entries_to_read[control_read->persist_sector_type] = 0;/*we use it just like the read sector, but a bit different*/
        status = fbe_persist_database_sector_start_lba(control_read->persist_sector_type, &entry_lba);
    }else{
        status = fbe_persist_database_get_entry_lba(control_read->entry_id, &entry_lba);
    }

    if (status != FBE_STATUS_OK) {
		fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Can't find start lba\n", __FUNCTION__);
        fbe_transport_reuse_packet(subpacket);
        persist_change_read_state(FBE_PERSIST_DB_READY_TO_READ);
        fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_GENERIC_FAILURE;
	}

    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                      entry_lba,
                                      FBE_PERSIST_BLOCKS_PER_TRAN_ENTRY,
                                      FBE_PERSIST_BYTES_PER_BLOCK,
                                      0,     /* optimum block size (not used) */
                                      NULL); /* preread descriptor (not used) */

	fbe_payload_ex_increment_block_operation_level(payload);

    /* Put an sgl pointer in the packet payload. We use the same sgls we allocate for the sector read,
	but we want to read only one entry to we use the last one that has the terminating one at the end*/
    fbe_payload_ex_set_sg_list(payload, &persist_read_buffer_sg[FBE_PERSIST_DB_TOTAL_READ_SG_ENTRIES - 2], 2);

    /* Link the subpacket into the caller's request packet. */
    fbe_transport_add_subpacket(request_packet, subpacket);

    /* Send send down the journal write. */
    fbe_transport_set_completion_function(subpacket, fbe_persist_database_read_single_entry_completion, control_read);
    fbe_transport_set_address(subpacket,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              fbe_persist_service.database_lun_object_id);

    
    fbe_topology_send_io_packet(subpacket);

    return FBE_STATUS_OK;

}

/*this function finds the first available entry in the sector, represented by 0 in the bitmap*/
static fbe_status_t fbe_persist_find_next_available_entry_offset(fbe_u64_t *sector_ptr, fbe_u32_t array_size, fbe_u32_t *offest)
{
	fbe_u32_t	idx;
	fbe_u64_t	bit_search = 0x1;
	fbe_u32_t	bit_offset = 0;

	for (idx = 0; idx < array_size; idx++) {
		/*does the entry itself has any free bits*/
		if (*sector_ptr == 0xFFFFFFFFFFFFFFFF) {
			sector_ptr++;
			continue;/*let's try the next one*/
		}

		/*we have at least one free entry, let's calculate it'f offset*/
		while((bit_search & (*sector_ptr)) != 0) {
			bit_search<<=1;
			bit_offset++;
		}

		*offest = (idx * ENTRY_ID_MAP_ENTRY_SIZE) + bit_offset;
		/*mark it as used for next time*/
		(*sector_ptr) |= bit_search;

		return FBE_STATUS_OK;
	}

    return FBE_STATUS_INSUFFICIENT_RESOURCES;

}

/*return a valid entry id in a given sector*/
fbe_status_t fbe_persist_database_get_entry_id(fbe_persist_entry_id_t *entry_id, fbe_persist_sector_type_t sector_type)
{
	fbe_u64_t *		sector_ptr = NULL;
	fbe_u32_t 		array_size = 0;
	fbe_u32_t 		offest;
	fbe_status_t	status;
	fbe_u64_t		temp_sector_type = sector_type;

	/*first get the correct pointer*/
	fbe_persist_database_sector_to_ptr(sector_type, &sector_ptr, &array_size);
	if (sector_ptr == NULL) {
		return FBE_STATUS_GENERIC_FAILURE;
	}

	/*and run a generic function that finds the correct offset of the first non 1 bit*/
	status = fbe_persist_find_next_available_entry_offset(sector_ptr, array_size, &offest);
	if (status != FBE_STATUS_OK) {
		fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s no more free entries in sector:%d \n", __FUNCTION__, sector_type);

		return status;
	}

	/*now that we have the offset inside the sector we are all set, just put the sector itself in the upper 32 bits*/
	*entry_id = offest;
	(*entry_id) |= (temp_sector_type << 32);

	return FBE_STATUS_OK;

}

/*check if the entry exists before we try to*/
fbe_status_t fbe_persist_entry_exists(fbe_persist_entry_id_t entry_id, fbe_bool_t *exists)
{
	fbe_u64_t *					sector_ptr = NULL;
    fbe_u32_t 					offset;
    fbe_persist_sector_type_t 	sector_type;
	fbe_u32_t					array_location;
	fbe_u64_t					bit_offset = 0x1;

	offset = entry_id & 0x00000000FFFFFFFF;

	/*extract the sector type off the entry ID*/
	entry_id &= 0xFFFFFFFF00000000;
	entry_id >>= 32;
	
    sector_type = entry_id;

	/*first get the correct pointer*/
	fbe_persist_database_sector_to_ptr(sector_type, &sector_ptr, NULL);
	if (sector_ptr == NULL) {
		return FBE_STATUS_GENERIC_FAILURE;
	}
    
	array_location = offset / ENTRY_ID_MAP_ENTRY_SIZE;
	offset %= ENTRY_ID_MAP_ENTRY_SIZE;
	bit_offset <<= offset;

	*exists = ((sector_ptr[array_location] & bit_offset) ? FBE_TRUE : FBE_FALSE);
    
	return FBE_STATUS_OK;
}

/*clear the bit the repeasets the entry to mark it as free*/
fbe_status_t fbe_persist_clear_entry_bit(fbe_persist_entry_id_t entry_id)
{
	fbe_u64_t *					sector_ptr = NULL;
    fbe_u32_t 					offset;
    fbe_persist_sector_type_t 	sector_type;
	fbe_u32_t					array_location;
	fbe_u64_t					bit_offset = 0x1;

	offset = entry_id & 0x00000000FFFFFFFF;

	/*extract the sector type off the entry ID*/
	entry_id &= 0xFFFFFFFF00000000;
	entry_id >>= 32;
	
    sector_type = entry_id;

	/*first get the correct pointer*/
	fbe_persist_database_sector_to_ptr(sector_type, &sector_ptr, NULL);
	if (sector_ptr == NULL) {
		return FBE_STATUS_GENERIC_FAILURE;
	}
    
	array_location = offset / ENTRY_ID_MAP_ENTRY_SIZE;
	offset %= ENTRY_ID_MAP_ENTRY_SIZE;
	bit_offset <<= offset;

	sector_ptr[array_location] &= ~bit_offset;

	return FBE_STATUS_OK;

}

static void fbe_persist_database_sector_to_ptr(fbe_persist_sector_type_t sector_type, fbe_u64_t **ptr, fbe_u32_t *array_size)
{
	fbe_u32_t		sector_offset_in_entries;
	fbe_status_t	status;

	/*check how many entries are there before our sector starts*/
	status = fbe_persist_database_layout_get_sector_offset(sector_type, &sector_offset_in_entries);
	if (status != FBE_STATUS_OK) {
		fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s failed to get offset for sector:%d\n", __FUNCTION__, sector_type);

		*ptr = NULL;
		return;
	}

	*ptr = fbe_persist_entry_id_map;/*this is where the map start*/

	/*the entries are representsed by 64 bits so we have to get the right offset*/
	sector_offset_in_entries /= ENTRY_ID_MAP_ENTRY_SIZE;
	(*ptr)+= sector_offset_in_entries;

	/*if the user wants that give it to him as well*/
	if (array_size != NULL) {
		fbe_persist_database_layout_get_sector_total_entries(sector_type, array_size);
		(*array_size) /= 64;/*the array has 64 bit entries so it's size is what we get, divided by 64*/
	}
	
}

/*calculate the start lba of the entry*/
fbe_status_t fbe_persist_database_get_entry_lba(fbe_persist_entry_id_t entry_id, fbe_lba_t *lba)
{
    fbe_u32_t 					offset;
    fbe_persist_sector_type_t 	sector_type;
	fbe_lba_t					start_lba;
	fbe_status_t 				status;

	/*at the beggining of the LU we have the journal, so the first entry of the first sector starts after that*/
	/*The sectors are alligned this way*/
	/*|--Journal--|---Objects---|---Edges---|...and so on*/

	offset = entry_id & 0x00000000FFFFFFFF;

	/*extract the sector type off the entry ID*/
	entry_id &= 0xFFFFFFFF00000000;
	entry_id >>= 32;
    sector_type = entry_id;

	/*add the offset to the begginig of the sector type*/
	status = fbe_persist_database_sector_start_lba(sector_type, &start_lba);
	if (status != FBE_STATUS_OK) {
		return status;
	}

	/*to the start lba we add the offset which is the index of the element in this sector*/
	*lba = start_lba + (offset * FBE_PERSIST_BLOCKS_PER_TRAN_ENTRY);
    return FBE_STATUS_OK;
}

/*check at which lba our sector starts*/
static fbe_status_t fbe_persist_database_sector_start_lba(fbe_persist_sector_type_t sector_type, fbe_lba_t *lba)
{
	fbe_status_t status;
	fbe_u32_t sector_offset_in_entries;

#ifndef SIMMODE_ENV
    /* Verify we don't change journal size in compile time */
    (void)sizeof(char[(FBE_PERSIST_BLOCKS_PER_TRAN * FBE_PERSIST_TRANSACTION_COUNT_IN_JOURNAL_AREA) == FBE_JOURNAL_AREA_SIZE ? 1 : -1]);
#endif
    /*right after the journal, we leave 3 extra journals for future use and we use only the first one
	hence we have FBE_PERSIST_TRANSACTION_COUNT_IN_JOURNAL_AREA of these*/
	*lba = fbe_persist_database_get_tran_journal_lba() + (FBE_PERSIST_BLOCKS_PER_TRAN * FBE_PERSIST_TRANSACTION_COUNT_IN_JOURNAL_AREA);

	/*check how many entries are there before our sector starts*/
	status = fbe_persist_database_layout_get_sector_offset(sector_type, &sector_offset_in_entries);
	if (status != FBE_STATUS_OK) {
		fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s failed to get offset for sector:%d\n", __FUNCTION__, sector_type);

        return status;
	}

	(*lba)+= FBE_PERSIST_BLOCKS_PER_TRAN_ENTRY * sector_offset_in_entries;

	return FBE_STATUS_OK;
}



/*continue to handle FBE_PERSIST_CONTROL_CODE_READ_SECTOR*/
fbe_status_t fbe_persist_database_read_sector(fbe_packet_t *packet, fbe_persist_control_read_sector_t *read_sector)
{
	fbe_packet_t * 					subpacket;
    fbe_payload_ex_t * 			payload;
    fbe_payload_block_operation_t * block_operation;
    fbe_lba_t 						read_lba;
    fbe_status_t 					status;
	fbe_s32_t						total_sector_entries;
	fbe_s32_t						entries_left_to_read;

	fbe_spinlock_lock(&persist_read_lock);
	if (persist_read_state != FBE_PERSIST_DB_READY_TO_READ) {
		fbe_spinlock_unlock(&persist_read_lock);
		fbe_transport_set_status(packet, FBE_STATUS_BUSY, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_BUSY;/*we are probably doing a read for another client now*/
	}

	/*good for us, we can read*/
	persist_read_state = FBE_PERSIST_DB_READ_IN_PROGRESS;

	/*let's do some math regarding where we are as far as the read goes and how much do we have left*/
	status = fbe_persist_database_layout_get_sector_total_entries(read_sector->sector_type, &total_sector_entries); 
	entries_left_to_read = total_sector_entries - persist_total_number_of_entries_read[read_sector->sector_type];

	if (entries_left_to_read > FBE_PERSIST_NUMBER_OF_SECTOR_READ_ENTRIES) {
		persist_number_of_entries_to_read[read_sector->sector_type] = FBE_PERSIST_NUMBER_OF_SECTOR_READ_ENTRIES;
	}else{
		persist_number_of_entries_to_read[read_sector->sector_type] = entries_left_to_read;
	}
	
	fbe_spinlock_unlock(&persist_read_lock);
    
    /* Create a subpacket for an I/O that read the entries into the bufers. */
    subpacket = persist_db_read_subpacket;
    
    payload = fbe_transport_get_payload_ex(subpacket);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Can't get payload\n", __FUNCTION__);
        fbe_transport_reuse_packet(subpacket);
        persist_change_read_state(FBE_PERSIST_DB_READY_TO_READ);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	fbe_payload_ex_allocate_memory_operation(payload);

    /* Set up a block operation to write the read */
    block_operation = fbe_payload_ex_allocate_block_operation(payload);
    if (block_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Can't get block_operation\n", __FUNCTION__);
        fbe_transport_reuse_packet(subpacket);
        persist_change_read_state(FBE_PERSIST_DB_READY_TO_READ);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	/*do we read for a specific entry if or from the stat of the sector ?*/
	if (read_sector->start_entry_id == FBE_PERSIST_SECTOR_START_ENTRY_ID) {
		status = fbe_persist_database_sector_start_lba(read_sector->sector_type, &read_lba);
	}else{
		status = fbe_persist_database_get_entry_lba(read_sector->start_entry_id, &read_lba);
	}

	if (status != FBE_STATUS_OK) {
		fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Can't find start lba\n", __FUNCTION__);
        fbe_transport_reuse_packet(subpacket);
        persist_change_read_state(FBE_PERSIST_DB_READY_TO_READ);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
	}

    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                      read_lba,
                                      FBE_PERSIST_BLOCKS_PER_TRAN_ENTRY * persist_number_of_entries_to_read[read_sector->sector_type],
                                      FBE_PERSIST_BYTES_PER_BLOCK,
                                      0,     /* optimum block size (not used) */
                                      NULL); /* preread descriptor (not used) */

	fbe_payload_ex_increment_block_operation_level(payload);

    /* Put an sgl pointer in the packet payload. */
    fbe_payload_ex_set_sg_list(payload, persist_read_buffer_sg, FBE_PERSIST_DB_TOTAL_READ_SG_ENTRIES);
    /* Link the subpacket into the caller's request packet. */
    fbe_transport_add_subpacket(packet, subpacket);

    /* Send send down the journal write. */
    fbe_transport_set_completion_function(subpacket, fbe_persist_database_read_sector_completion, read_sector);
    fbe_transport_set_address(subpacket,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              fbe_persist_service.database_lun_object_id);

    fbe_topology_send_io_packet(subpacket);
    
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_persist_database_read_sector_completion(fbe_packet_t * subpacket, fbe_packet_completion_context_t context)
{
	fbe_packet_t * 							request_packet;
    fbe_status_t 							status;
    fbe_payload_ex_t *						payload;
    fbe_payload_block_operation_t * 		block_operation;
    fbe_payload_block_operation_status_t	block_status;
	fbe_persist_control_read_sector_t *read_sector = (fbe_persist_control_read_sector_t *)context;

    /* Get the original request packet (the one the user sent to set the LUN). */
    request_packet = (fbe_packet_t *)fbe_transport_get_master_packet(subpacket);
    fbe_transport_remove_subpacket(subpacket);

	payload = fbe_transport_get_payload_ex(subpacket);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Can't get payload\n", __FUNCTION__);
        fbe_transport_reuse_packet(subpacket);
        persist_change_read_state(FBE_PERSIST_DB_READY_TO_READ);
        fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

	/* Verify that the read completed ok. */
    block_operation = fbe_payload_ex_get_block_operation(payload);
    if (block_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Can't get block_operation\n", __FUNCTION__);
        fbe_transport_reuse_packet(subpacket);
        persist_change_read_state(FBE_PERSIST_DB_READY_TO_READ);
        fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Verify that the read request subpacket completed ok. */
    status = fbe_transport_get_status_code(subpacket);
    if (status != FBE_STATUS_OK) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s journal read failed\n", __FUNCTION__);
		fbe_payload_ex_release_block_operation(payload, block_operation);
        fbe_transport_reuse_packet(subpacket);
        persist_change_read_state(FBE_PERSIST_DB_READY_TO_READ);
        fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet); 
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    fbe_payload_block_get_status(block_operation, &block_status);
    if (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {
        fbe_persist_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Block_operation error, status: 0x%X\n", __FUNCTION__, block_status);
		fbe_payload_ex_release_block_operation(payload, block_operation);
        fbe_transport_reuse_packet(subpacket);
        persist_change_read_state(FBE_PERSIST_DB_READY_TO_READ);
        if (block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR) {
            fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_BAD_BLOCK);
        }
        else{
            fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        }
        
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* We are done with the entry read operation. */
    fbe_payload_ex_release_block_operation(payload, block_operation);
    fbe_transport_reuse_packet(subpacket);

	/*remember how many entries we read so far*/
	persist_total_number_of_entries_read[read_sector->sector_type] += persist_number_of_entries_to_read[read_sector->sector_type];

	/*we can now complete the read to the user by copying everything from our buffer to the user's buffer*/
    persist_database_return_read_data_to_user(request_packet, read_sector);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*when we are done reading, we'll porcess the data and return it to the user*/
fbe_status_t persist_database_return_read_data_to_user(fbe_packet_t * user_packet, fbe_persist_control_read_sector_t *read_sector)
{
	fbe_sg_element_t *					user_sg_element = NULL;
	fbe_u32_t							buffer_count = 0;
	fbe_u32_t							entry = 0;
	fbe_u32_t							block = 0;
	fbe_u8_t *							read_data = NULL;
	fbe_payload_ex_t * 				request_payload;
	fbe_u32_t							sg_count = 0;
	fbe_u8_t *							user_data_ptr = NULL;
    fbe_persist_user_header_t *			user_header;
	fbe_persist_tran_elem_header_t *	persist_header;
	fbe_u32_t							total_sector_entries;
	fbe_status_t						status;
    fbe_u64_t							sector_id_shifted = read_sector->sector_type;
	fbe_u32_t							entries_copied = 0;
	fbe_bool_t							all_entries_copied = FBE_FALSE;

	sector_id_shifted <<= 32;/*prepare it for calculating the next entry id*/

	/*get a poiner to the user's buffer*/
	request_payload = fbe_transport_get_payload_ex(user_packet);
	fbe_payload_ex_get_sg_list(request_payload, &user_sg_element, &sg_count);
	user_data_ptr = user_sg_element->address;
	
	/*now that we are done reading, we can copy all the data from the read buffer to the user buffer represented by the sg list*/
    for (buffer_count = 0; buffer_count < FBE_PERSIST_DB_TOTAL_READ_BUFFERS; buffer_count++) {
		read_data = (fbe_u8_t *)persist_read_buffer[buffer_count];

		for (entry = 0; entry < FBE_PERSIST_DB_READ_ENTRIES_PER_BUFFER; entry ++) {
			
			user_header = (fbe_persist_user_header_t *)user_data_ptr;
			persist_header = (fbe_persist_tran_elem_header_t *)read_data;

            if (persist_header->header.tran_elem_op == FBE_PERSIST_TRAN_OP_DELETE_ENTRY) {
                /*this entry was previously delted so don't even bother reporting it*/
                fbe_persist_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s persist_header->header.tran_elem_op is delete, jump it\n", __FUNCTION__);
                read_data+= ((FBE_PERSIST_BYTES_PER_BLOCK * FBE_PERSIST_BLOCKS_PER_ENTRY) + (FBE_PERSIST_BLOCKS_PER_HEADER * FBE_PERSIST_BYTES_PER_BLOCK));
                entries_copied++;  // For the entry which we skip, also need to count it.
                /* If all entries are copied, break out */
                if (entries_copied == persist_number_of_entries_to_read[read_sector->sector_type]) {
                    all_entries_copied = FBE_TRUE;
                    break;
                }
                continue;
            }

			/*Before the first user data block, we copy any information which is in our header to the user header
			 so he can learn about this entry, this is part of the return data*/
			user_header->entry_id = persist_header->header.entry_id;
			user_data_ptr += sizeof(fbe_persist_user_header_t);/*jump in a size of fbe_persist_user_header_t to the start of the data*/

			read_data += FBE_PERSIST_BYTES_PER_BLOCK;/*skip the first block, it's our own header*/
			for (block = 0; block < FBE_PERSIST_BLOCKS_PER_ENTRY; block ++) {
				/*now we can copy the data to the user buffer but carefully, we want to skip every FBE_PERSIST_DATA_BYTES_PER_BLOCK
				to make sure we don't copy the CRC to the user.*/
				fbe_copy_memory(user_data_ptr, read_data, FBE_PERSIST_DATA_BYTES_PER_BLOCK);
                
				read_data+= FBE_PERSIST_BYTES_PER_BLOCK;/*go to next block of this entry*/
				user_data_ptr += FBE_PERSIST_DATA_BYTES_PER_BLOCK;/*go to the next user block*/
				
			}

			/*did we copy everything we read ? No need to copy more garbage*/
			entries_copied++;
			if (entries_copied == persist_number_of_entries_to_read[read_sector->sector_type]) {
				all_entries_copied = FBE_TRUE;
				break;
			}
		}

		if (FBE_IS_TRUE(all_entries_copied)) {
			break;
		}
	}
    
	/*tell the user how much we read so he can tell how many entries to go over in the buffer he receives*/
	read_sector->entries_read = persist_number_of_entries_to_read[read_sector->sector_type];

	/*check what we need to do next*/
	status = fbe_persist_database_layout_get_sector_total_entries(read_sector->sector_type, &total_sector_entries);
	if (persist_total_number_of_entries_read[read_sector->sector_type] == total_sector_entries) {
		/*reset everything for next time*/
        persist_number_of_entries_to_read[read_sector->sector_type] = 0;
		persist_total_number_of_entries_read[read_sector->sector_type] = 0;
        /*update user portion*/
		read_sector->next_entry_id = FBE_PERSIST_NO_MORE_ENTRIES_TO_READ;
	}else{
		/*we need to know what was the last entry id we read in case we need to keep on reading.
		However, we can't just read it from disk like the one we send to the user because we need
		to generate the next read LBA off this entry id and it might be invalid since there is no entry there*/
		read_sector->next_entry_id = persist_total_number_of_entries_read[read_sector->sector_type] | sector_id_shifted;
	}

	fbe_spinlock_lock(&persist_read_lock);
	/*we can read again. Tehnically, another user can try and read another sector, but we don't care,
	all the counters are seperated */
	persist_read_state = FBE_PERSIST_DB_READY_TO_READ;
	fbe_spinlock_unlock(&persist_read_lock);

	/*we are done let's return data back to user*/
	fbe_transport_set_status(user_packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(user_packet);
	return FBE_STATUS_OK;
}

/*read all sectors and update the bitmap*/
fbe_status_t fbe_persist_database_update_bitmap(fbe_packet_t *packet, fbe_persist_sector_type_t sector_type, fbe_persist_entry_id_t start_entry_id)
{
    fbe_status_t    status;
    fbe_s32_t       total_sector_entries;
    fbe_s32_t       entries_left_to_read;

    fbe_spinlock_lock(&persist_read_lock); 
    if (persist_read_state != FBE_PERSIST_DB_READY_TO_READ) {
        fbe_spinlock_unlock(&persist_read_lock);

        fbe_persist_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s persist db is already in progress\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_BUSY, 0);
        fbe_transport_complete_packet(packet);
        
        return FBE_STATUS_BUSY;/*we are probably doing a read for another client now*/
    }

    /*good for us, we can read*/
    persist_read_state = FBE_PERSIST_DB_READ_IN_PROGRESS;    
    
	/*let's do some math regarding where we are as far as the read goes and how much do we have left*/
	status = fbe_persist_database_layout_get_sector_total_entries(sector_type, &total_sector_entries); 
	entries_left_to_read = total_sector_entries - persist_total_number_of_entries_read[sector_type];

	if (entries_left_to_read > FBE_PERSIST_NUMBER_OF_SECTOR_READ_ENTRIES) {
		persist_number_of_entries_to_read[sector_type] = FBE_PERSIST_NUMBER_OF_SECTOR_READ_ENTRIES;
	}else{
		persist_number_of_entries_to_read[sector_type] = entries_left_to_read;
	}
	
	fbe_spinlock_unlock(&persist_read_lock);

    status = persist_send_update_bitmap_io(packet, sector_type, start_entry_id);
    if(FBE_STATUS_OK != status)
    {
        persist_change_read_state(FBE_PERSIST_DB_READY_TO_READ);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*when we are done reading the sector for the bitmap update, we'll porcess the data and fill the bitmap*/
static fbe_status_t fbe_persist_database_update_bitmap_completion(fbe_packet_t * subpacket, fbe_packet_completion_context_t context)
{
	fbe_packet_t * 							request_packet;
    fbe_status_t 							status;
    fbe_payload_ex_t *						payload;
    fbe_payload_block_operation_t * 		block_operation;
    fbe_payload_block_operation_status_t	block_status;
	fbe_persist_sector_type_t				read_sector = (fbe_persist_sector_type_t)context;

    /* Get the original request packet (the one the user sent to set the LUN). */
    request_packet = (fbe_packet_t *)fbe_transport_get_master_packet(subpacket);
    fbe_transport_remove_subpacket(subpacket);

    /* Verify that the read request subpacket completed ok. */
    status = fbe_transport_get_status_code(subpacket);
    if (status != FBE_STATUS_OK) {
        fbe_persist_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s sector data read failed\n", __FUNCTION__);
        fbe_transport_reuse_packet(subpacket);
        persist_change_read_state(FBE_PERSIST_DB_READY_TO_READ);
        fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    payload = fbe_transport_get_payload_ex(subpacket);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Can't get payload\n", __FUNCTION__);
        fbe_transport_reuse_packet(subpacket);
        persist_change_read_state(FBE_PERSIST_DB_READY_TO_READ);
        fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Verify that the read completed ok. */
    block_operation = fbe_payload_ex_get_block_operation(payload);
    if (block_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Can't get block_operation\n", __FUNCTION__);
        fbe_transport_reuse_packet(subpacket);
        persist_change_read_state(FBE_PERSIST_DB_READY_TO_READ);
        fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    fbe_payload_block_get_status(block_operation, &block_status);
    if (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {
        fbe_persist_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Block_operation error, status: 0x%X\n", __FUNCTION__, block_status);
		fbe_payload_ex_release_block_operation(payload, block_operation);
        fbe_transport_reuse_packet(subpacket);
         if (block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR) {
            fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_BAD_BLOCK);
        }
        else{ 
            fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0); 
        }

        persist_change_read_state(FBE_PERSIST_DB_READY_TO_READ);
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* We are done with the entry read operation. */
    fbe_payload_ex_release_block_operation(payload, block_operation);
    fbe_transport_reuse_packet(subpacket);

	/*remember how many entries we read so far*/
	persist_total_number_of_entries_read[read_sector] += persist_number_of_entries_to_read[read_sector];

	persist_database_fill_bitmap(request_packet, read_sector);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*fill the bitmap based no the read data*/
static fbe_status_t persist_database_fill_bitmap(fbe_packet_t * user_packet, fbe_persist_sector_type_t sector_type)
{
    fbe_u32_t							buffer_count = 0;
	fbe_u32_t							entry = 0;
	fbe_u8_t *							read_data = NULL;
    fbe_persist_tran_elem_header_t *	persist_header;
	fbe_u32_t							total_sector_entries;
	fbe_status_t						status;
    fbe_u64_t							sector_id_shifted = sector_type;
	fbe_u32_t							entries_read = 0;
	fbe_bool_t							all_entries_read = FBE_FALSE;
	fbe_persist_entry_id_t				next_entry_id;

	sector_id_shifted <<= 32;/*prepare it for calculating the next entry id*/

    /*now that we are done reading, we can go over and fill the bitmap*/
    for (buffer_count = 0; buffer_count < FBE_PERSIST_DB_TOTAL_READ_BUFFERS; buffer_count++) {
		read_data = (fbe_u8_t *)persist_read_buffer[buffer_count];
		for (entry = 0; entry < FBE_PERSIST_DB_READ_ENTRIES_PER_BUFFER; entry ++) {
            persist_header = (fbe_persist_tran_elem_header_t *)read_data;
			/*do we have a valid entry there ? It is valid if the entry id is not zero or if it's a previouslt deleted entry
			The reason why we don't write a 0 in a deleted entry is because when we set up the write, we must still leave
			the entry id in case the user wants to abort the transaction. We end up with the entry id on the disk, but this is
			whay we have the FBE_PERSIST_TRAN_OP_DELETE_ENTRY marker which means this is a free entry*/
			if ((persist_header->header.entry_id != 0) && (persist_header->header.tran_elem_op != FBE_PERSIST_TRAN_OP_DELETE_ENTRY)) {
				fbe_persist_set_entry_bit(persist_header->header.entry_id);
			}

			/*skip the first block, it's our own header and then the entry itself*/
			read_data+= (FBE_PERSIST_BYTES_PER_BLOCK + FBE_PERSIST_BYTES_PER_ENTRY);
            
			/*did we read everything we read ? No need to copy more garbage*/
			entries_read++;
			if (entries_read == persist_number_of_entries_to_read[sector_type]) {
				all_entries_read = FBE_TRUE;
				break;
			}
		}

		if (FBE_IS_TRUE(all_entries_read)) {
			break;
		}
	}

    /*check what we need to do next, if we are done reading this sector, we check if we can go to the next one, if not we keep reading*/
	status = fbe_persist_database_layout_get_sector_total_entries(sector_type, &total_sector_entries);
	if (persist_total_number_of_entries_read[sector_type] == total_sector_entries) {
		/*reset everything for next time*/
        persist_number_of_entries_to_read[sector_type] = 0;
		persist_total_number_of_entries_read[sector_type] = 0;

		/*let's go to the next sector (if any)*/
		sector_type++;
		if (sector_type == FBE_PERSIST_SECTOR_TYPE_LAST) {
			fbe_spinlock_lock(&persist_read_lock);
			persist_read_state = FBE_PERSIST_DB_READY_TO_READ;/*we can read again, each sector has it's own entry*/
			fbe_spinlock_unlock(&persist_read_lock);

            fbe_persist_trace(FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: all sectors read finished.\n", __FUNCTION__);

			/*we are don'e let's complete the packet, this will read the journal in*/
			fbe_transport_set_status(user_packet, FBE_STATUS_OK, 0);
			fbe_transport_complete_packet(user_packet);
		}else{
			fbe_persist_database_update_bitmap_continue(user_packet, sector_type, FBE_PERSIST_SECTOR_START_ENTRY_ID);
		}

	}else{

		//fbe_spinlock_lock(&persist_read_lock);
		//persist_read_state = FBE_PERSIST_DB_READY_TO_READ;/*we can read again, each sector has it's own entry*/
		//fbe_spinlock_unlock(&persist_read_lock);

		/*we need to know what was the last entry id we read in case we need to keep on reading.
		However, we can't just read it from disk like the one we send to the user because we need
		to generate the next read LBA off this entry id and it might be invalid since there is no entry there*/
        
		next_entry_id = persist_total_number_of_entries_read[sector_type] | sector_id_shifted;
		fbe_persist_database_update_bitmap_continue(user_packet, sector_type, next_entry_id);
		
	}

	return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*continue to read sectors and update the bitmap*/
static fbe_status_t fbe_persist_database_update_bitmap_continue(fbe_packet_t *packet, fbe_persist_sector_type_t sector_type, fbe_persist_entry_id_t start_entry_id)
{
    fbe_status_t 					status;
	fbe_s32_t						total_sector_entries;
	fbe_s32_t						entries_left_to_read;

    fbe_spinlock_lock(&persist_read_lock); 
    
	/*let's do some math regarding where we are as far as the read goes and how much do we have left*/
	status = fbe_persist_database_layout_get_sector_total_entries(sector_type, &total_sector_entries); 
	entries_left_to_read = total_sector_entries - persist_total_number_of_entries_read[sector_type];

	if (entries_left_to_read > FBE_PERSIST_NUMBER_OF_SECTOR_READ_ENTRIES) {
		persist_number_of_entries_to_read[sector_type] = FBE_PERSIST_NUMBER_OF_SECTOR_READ_ENTRIES;
	}else{
		persist_number_of_entries_to_read[sector_type] = entries_left_to_read;
	}
	
	fbe_spinlock_unlock(&persist_read_lock);

    status = persist_send_update_bitmap_io(packet, sector_type, start_entry_id);
    if(FBE_STATUS_OK != status)
    {
        persist_change_read_state(FBE_PERSIST_DB_READY_TO_READ);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    return status;
}

static fbe_status_t persist_send_update_bitmap_io(fbe_packet_t *packet, fbe_persist_sector_type_t sector_type, fbe_persist_entry_id_t start_entry_id)
{
    fbe_packet_t *                  subpacket;
    fbe_payload_ex_t *              payload;
    fbe_payload_block_operation_t * block_operation;
    fbe_lba_t                       read_lba;
    fbe_status_t                    status;

    /* Create a subpacket for an I/O that read the entries into the bufers. */
    subpacket = persist_db_read_subpacket;
    
    payload = fbe_transport_get_payload_ex(subpacket);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Can't get payload\n", __FUNCTION__);
        fbe_transport_reuse_packet(subpacket);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set up a block operation to write the read */
    block_operation = fbe_payload_ex_allocate_block_operation(payload);
    if (block_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Can't get block_operation\n", __FUNCTION__);
        fbe_transport_reuse_packet(subpacket);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	/*do we read for a specific entry if or from the stat of the sector ?*/
	if (start_entry_id == FBE_PERSIST_SECTOR_START_ENTRY_ID) {
		status = fbe_persist_database_sector_start_lba(sector_type, &read_lba);
	}else{
		status = fbe_persist_database_get_entry_lba(start_entry_id, &read_lba);
	}

	if (status != FBE_STATUS_OK) {
		fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Can't find start lba\n", __FUNCTION__);
        fbe_transport_reuse_packet(subpacket);
        return FBE_STATUS_GENERIC_FAILURE;
	}

    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                      read_lba,
                                      FBE_PERSIST_BLOCKS_PER_TRAN_ENTRY * persist_number_of_entries_to_read[sector_type],
                                      FBE_PERSIST_BYTES_PER_BLOCK,
                                      0,     /* optimum block size (not used) */
                                      NULL); /* preread descriptor (not used) */

	fbe_payload_ex_increment_block_operation_level(payload);

    /* Put an sgl pointer in the packet payload. */
    fbe_payload_ex_set_sg_list(payload, persist_read_buffer_sg, FBE_PERSIST_DB_TOTAL_READ_SG_ENTRIES);
    /* Link the subpacket into the caller's request packet. */
    fbe_transport_add_subpacket(packet, subpacket);

    /* Send send down the journal write. */
    fbe_transport_set_completion_function(subpacket, fbe_persist_database_update_bitmap_completion, (void *)sector_type);
    fbe_transport_set_address(subpacket,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              fbe_persist_service.database_lun_object_id);

	fbe_persist_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "%s: Sending Read packet to LUN.\n", __FUNCTION__);

    fbe_topology_send_io_packet(subpacket);

    return FBE_STATUS_OK;
}

fbe_status_t fbe_persist_set_entry_bit(fbe_persist_entry_id_t entry_id)
{
	fbe_u64_t *					sector_ptr = NULL;
    fbe_u32_t 					offset;
    fbe_persist_sector_type_t 	sector_type;
	fbe_u32_t					array_location;
	fbe_u64_t					bit_offset = 0x1;

	offset = entry_id & 0x00000000FFFFFFFF;

	/*extract the sector type off the entry ID*/
	entry_id &= 0xFFFFFFFF00000000;
	entry_id >>= 32;
	
    sector_type = entry_id;

	/*first get the correct pointer*/
	fbe_persist_database_sector_to_ptr(sector_type, &sector_ptr, NULL);
	if (sector_ptr == NULL) {
		return FBE_STATUS_GENERIC_FAILURE;
	}
    
	array_location = offset / ENTRY_ID_MAP_ENTRY_SIZE;
	offset %= ENTRY_ID_MAP_ENTRY_SIZE;
	bit_offset <<= offset;

	sector_ptr[array_location] |= bit_offset;

	return FBE_STATUS_OK;

}

/*copy data from our read buffer to the user's buffer for a single entry read we did*/
static fbe_status_t persist_database_return_single_read_data_to_user(fbe_packet_t * user_packet, fbe_persist_control_read_entry_t * read_entry)
{
	fbe_sg_element_t *					user_sg_element = NULL;
    fbe_u32_t							block = 0;
	fbe_u8_t *							read_data = NULL;
	fbe_payload_ex_t * 				    request_payload;
	fbe_u32_t							sg_count = 0;
	fbe_u8_t *							user_data_ptr = NULL;
	fbe_status_t						status;
    fbe_u32_t							total_sector_entries;
    fbe_u64_t							sector_id_shifted = read_entry->persist_sector_type;

	sector_id_shifted <<= 32;/*prepare it for calculating the next entry id*/

    /*get a poiner to the user's buffer*/
	request_payload = fbe_transport_get_payload_ex(user_packet);
	fbe_payload_ex_get_sg_list(request_payload, &user_sg_element, &sg_count);
	user_data_ptr = user_sg_element->address;
	
	/*start from the last buffer which contains the entry we read in it's first out of 4 entries it has*/
	read_data = (fbe_u8_t *)persist_read_buffer[FBE_PERSIST_DB_TOTAL_READ_BUFFERS - 1];
    read_data += FBE_PERSIST_BYTES_PER_BLOCK;/*skip the first block, it's our own header*/

	for (block = 0; block < FBE_PERSIST_BLOCKS_PER_ENTRY; block ++) {
		/*now we can copy the data to the user buffer but carefully, we want to skip every FBE_PERSIST_DATA_BYTES_PER_BLOCK
		to make sure we don't copy the CRC to the user.*/
		fbe_copy_memory(user_data_ptr, read_data, FBE_PERSIST_DATA_BYTES_PER_BLOCK);
		read_data+= FBE_PERSIST_BYTES_PER_BLOCK;/*go to next block of this entry*/
		user_data_ptr += FBE_PERSIST_DATA_BYTES_PER_BLOCK;/*go to the next user block*/
	}

	/*check what we need to do next*/
	status = fbe_persist_database_layout_get_sector_total_entries(read_entry->persist_sector_type, &total_sector_entries);
	if (persist_number_of_entries_to_read[read_entry->persist_sector_type] == total_sector_entries) {
		/*reset everything for next time*/
        persist_number_of_entries_to_read[read_entry->persist_sector_type] = 0;
        /*update user portion*/
		read_entry->next_entry_id = FBE_PERSIST_NO_MORE_ENTRIES_TO_READ;
	}else{
		/*we need to know what was the last entry id we read in case we need to keep on reading.
		However, we can't just read it from disk like the one we send to the user because we need
		to generate the next read LBA off this entry id and it might be invalid since there is no entry there*/
		read_entry->next_entry_id = persist_number_of_entries_to_read[read_entry->persist_sector_type] | sector_id_shifted;
	}
		
    fbe_spinlock_lock(&persist_read_lock);
	/*we can read again. Tehnically, another user can try and read another sector, but we don't care,
	all the counters are seperated */
	persist_read_state = FBE_PERSIST_DB_READY_TO_READ;
	fbe_spinlock_unlock(&persist_read_lock);

	/*we are done let's return data back to user*/
	fbe_transport_set_status(user_packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(user_packet);
	return FBE_STATUS_OK;

}

/*give the user the information on how the lbas are set up*/
fbe_status_t fbe_persist_database_get_lba_layout_info( fbe_persist_control_get_layout_info_t *get_layout)
{
	fbe_status_t	status;

	/*fill out all the information*/
	get_layout->journal_start_lba= fbe_persist_database_get_tran_journal_lba();
    
    status = fbe_persist_database_sector_start_lba(FBE_PERSIST_SECTOR_TYPE_SEP_OBJECTS, &get_layout->sep_object_start_lba);
	if(status != FBE_STATUS_OK){
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s failed to get start of sector: %d\n",
                          __FUNCTION__, FBE_PERSIST_SECTOR_TYPE_SEP_OBJECTS);
       
        return FBE_STATUS_GENERIC_FAILURE;
    }

	status = fbe_persist_database_sector_start_lba(FBE_PERSIST_SECTOR_TYPE_SEP_EDGES, &get_layout->sep_edges_start_lba);
	if(status != FBE_STATUS_OK){
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s failed to get start of sector: %d\n",
                          __FUNCTION__, FBE_PERSIST_SECTOR_TYPE_SEP_EDGES);
       
        return FBE_STATUS_GENERIC_FAILURE;
    }

	status = fbe_persist_database_sector_start_lba(FBE_PERSIST_SECTOR_TYPE_SEP_ADMIN_CONVERSION, &get_layout->sep_admin_conversion_start_lba);
	if(status != FBE_STATUS_OK){
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s failed to get start of sector: %d\n",
                          __FUNCTION__, FBE_PERSIST_SECTOR_TYPE_SEP_EDGES);
       
        return FBE_STATUS_GENERIC_FAILURE;
    }

	status = fbe_persist_database_sector_start_lba(FBE_PERSIST_SECTOR_TYPE_ESP_OBJECTS, &get_layout->esp_objects_start_lba);
	if(status != FBE_STATUS_OK){
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s failed to get start of sector: %d\n",
                          __FUNCTION__, FBE_PERSIST_SECTOR_TYPE_SEP_EDGES);
       
        return FBE_STATUS_GENERIC_FAILURE;
    }

	status = fbe_persist_database_sector_start_lba(FBE_PERSIST_SECTOR_TYPE_SYSTEM_GLOBAL_DATA, &get_layout->system_data_start_lba);
	if(status != FBE_STATUS_OK){
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s failed to get start of sector: %d\n",
                          __FUNCTION__, FBE_PERSIST_SECTOR_TYPE_SYSTEM_GLOBAL_DATA);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

	return FBE_STATUS_OK;
}

/*!**************************************************************************
 * @file fbe_persist_database_read_persist_db_header
 ***************************************************************************
 *
 * @brief
 *  This function reads persist db header from the header region (the 1st block
 *  of DATABASE LUN).
 *
 * @author
 *  11/17/2012 - Created. Zhipeng Hu
 *
 ***************************************************************************/
fbe_status_t fbe_persist_database_read_persist_db_header(fbe_packet_t *packet)
{
    fbe_packet_t *                     subpacket;
    fbe_payload_ex_t *             payload;
    fbe_payload_block_operation_t * block_operation;
    fbe_lba_t                         read_lba;
    fbe_persist_db_header_t *        in_persist_db_header = NULL;

    /*set up the read*/
    in_persist_db_header = (fbe_persist_db_header_t *)persist_db_header_read_write_buffer;

    fbe_zero_memory(persist_db_header_read_write_buffer, FBE_PERSIST_BYTES_PER_BLOCK);
    persist_db_header_read_write_sgl[0].address = persist_db_header_read_write_buffer;
    persist_db_header_read_write_sgl[0].count = FBE_PERSIST_BYTES_PER_BLOCK;
    fbe_sg_element_terminate(&persist_db_header_read_write_sgl[1]);

    /* Create a subpacket for an I/O that read the first block */
    subpacket = persist_db_header_subpacket;

    payload = fbe_transport_get_payload_ex(subpacket);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s Can't get payload\n", __FUNCTION__);
        fbe_transport_reuse_packet(subpacket);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set up a block operation to write the read */
    block_operation = fbe_payload_ex_allocate_block_operation(payload);
    if (block_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s Can't get block_operation\n", __FUNCTION__);
        fbe_transport_reuse_packet(subpacket);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    read_lba = FBE_PERSIST_START_LBA;/*the persist db header is always at the 1st blk of persist start*/

    fbe_payload_block_build_operation(block_operation,
                                    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                    read_lba,
                                    1,
                                    FBE_PERSIST_BYTES_PER_BLOCK,
                                    0,     /* optimum block size (not used) */
                                    NULL); /* preread descriptor (not used) */

    fbe_payload_ex_increment_block_operation_level(payload);

    /* Put an sgl pointer in the packet payload. */
    fbe_payload_ex_set_sg_list(payload, persist_db_header_read_write_sgl, 2);

    /* Link the subpacket into the caller's request packet. */
    fbe_transport_add_subpacket(packet, subpacket);

    /* Send send down the journal write. */
    fbe_transport_set_completion_function(subpacket, fbe_persist_database_read_persist_db_header_completion, NULL);
    fbe_transport_set_address(subpacket,
                                FBE_PACKAGE_ID_SEP_0,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                fbe_persist_service.database_lun_object_id);

    fbe_topology_send_io_packet(subpacket);

    return FBE_STATUS_OK;
}

/*!**************************************************************************
 * @file fbe_persist_database_read_persist_db_header_completion
 ***************************************************************************
 *
 * @brief
 *  Completion routine of reading persist db header from DB LUN.
 *  If it is found the db header is not initialized, it will write the initialized
 *  db header to the header region on DB LUN.
 *
 * @author
 *  11/17/2012 - Created. Zhipeng Hu
 *
 ***************************************************************************/
static fbe_status_t fbe_persist_database_read_persist_db_header_completion(fbe_packet_t * subpacket, fbe_packet_completion_context_t context)
{
    fbe_packet_t *                             request_packet;
    fbe_status_t                             status;
    fbe_payload_ex_t *                        payload;
    fbe_payload_block_operation_t *         block_operation;
    fbe_payload_block_operation_status_t    block_status;
    fbe_persist_db_header_t *                in_persist_db_header = NULL;

    /* Get the original request packet (the one the user sent to set the LUN). */
    request_packet = (fbe_packet_t *)fbe_transport_get_master_packet(subpacket);
    fbe_transport_remove_subpacket(subpacket);

    /* Verify that the read request subpacket completed ok. */
    status = fbe_transport_get_status_code(subpacket);
    if (status != FBE_STATUS_OK) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s read failed\n", __FUNCTION__);
        fbe_transport_reuse_packet(subpacket);
        fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    
    payload = fbe_transport_get_payload_ex(subpacket);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s Can't get payload\n", __FUNCTION__);
        fbe_transport_reuse_packet(subpacket);
        fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Verify that the read completed ok. */
    block_operation = fbe_payload_ex_get_block_operation(payload);
    if (block_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Can't get block_operation\n", __FUNCTION__);
        fbe_transport_reuse_packet(subpacket);
        fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    
    fbe_payload_block_get_status(block_operation, &block_status);
    if (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s Block_operation error, status: 0x%X\n", __FUNCTION__, block_status);
        fbe_payload_ex_release_block_operation(payload, block_operation);
        fbe_transport_reuse_packet(subpacket);
        fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    fbe_persist_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
        "%s read persist db header successfully\n", __FUNCTION__);   

    /* We are done with the entry read operation. */
    fbe_payload_ex_release_block_operation(payload, block_operation);
    fbe_transport_reuse_packet(subpacket);    

    in_persist_db_header = (fbe_persist_db_header_t *)persist_db_header_read_write_buffer;

    /*do we have a fresh DB*/
    if (in_persist_db_header->signature !=  FBE_PERSIST_DB_SIGNATURE) {
        fbe_persist_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "Persist db header mismatch:0x%llX expected:0x%llX, stamping new header\n",
            (unsigned long long)in_persist_db_header->signature, (unsigned long long)FBE_PERSIST_DB_SIGNATURE);

        persist_db_header.signature = FBE_PERSIST_DB_SIGNATURE;
        persist_db_header.version = FBE_PERSIST_DB_VERSION;
        persist_db_header.journal_state = 0;
        persist_db_header.journal_size = 0;

        fbe_persist_database_write_persist_db_header(request_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    
    fbe_copy_memory(&persist_db_header, in_persist_db_header, sizeof(fbe_persist_db_header_t));

    fbe_transport_set_status(request_packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(request_packet);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*!**************************************************************************
 * @file fbe_persist_database_write_persist_db_header
 ***************************************************************************
 *
 * @brief
 *  Write persist db header into header region on the DB LUN
 *
 * @author
 *  11/17/2012 - Created. Zhipeng Hu
 *
 ***************************************************************************/
fbe_status_t fbe_persist_database_write_persist_db_header(fbe_packet_t *packet)
{
    fbe_packet_t *                     subpacket;
    fbe_payload_ex_t *             payload;
    fbe_payload_block_operation_t * block_operation;
    fbe_u32_t                         checksum;

    fbe_persist_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
        "%s entry\n", __FUNCTION__);   

    fbe_zero_memory(persist_db_header_read_write_buffer, FBE_PERSIST_BYTES_PER_BLOCK);
    fbe_copy_memory(persist_db_header_read_write_buffer, &persist_db_header, sizeof(fbe_persist_db_header_t));
    
    checksum = fbe_xor_lib_calculate_checksum((fbe_u32_t*)persist_db_header_read_write_buffer);
    fbe_copy_memory(&persist_db_header_read_write_buffer[FBE_PERSIST_DATA_BYTES_PER_BLOCK], &checksum, sizeof(fbe_u32_t));

    persist_db_header_read_write_sgl[0].address = persist_db_header_read_write_buffer;
    persist_db_header_read_write_sgl[0].count = FBE_PERSIST_BYTES_PER_BLOCK;
    fbe_sg_element_terminate(&persist_db_header_read_write_sgl[1]);

    /* Create a subpacket for an I/O that write the first block */
    subpacket = persist_db_header_subpacket;

    payload = fbe_transport_get_payload_ex(subpacket);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s Can't get payload\n", __FUNCTION__);
        fbe_transport_reuse_packet(subpacket);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set up a block operation to write the read */
    block_operation = fbe_payload_ex_allocate_block_operation(payload);
    if (block_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s Can't get block_operation\n", __FUNCTION__);
        fbe_transport_reuse_packet(subpacket);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_block_build_operation(block_operation,
                                    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                    FBE_PERSIST_START_LBA,
                                    1,
                                    FBE_PERSIST_BYTES_PER_BLOCK,
                                    0,     /* optimum block size (not used) */
                                    NULL); /* preread descriptor (not used) */

    fbe_payload_ex_increment_block_operation_level(payload);

    /* Put an sgl pointer in the packet payload. */
    fbe_payload_ex_set_sg_list(payload, persist_db_header_read_write_sgl, 2);
    /* Link the subpacket into the caller's request packet. */
    fbe_transport_add_subpacket(packet, subpacket);

    /* Send send down the journal write. */
    fbe_transport_set_completion_function(subpacket, fbe_persist_database_write_persist_db_header_completion, NULL);
    fbe_transport_set_address(subpacket,
                            FBE_PACKAGE_ID_SEP_0,
                            FBE_SERVICE_ID_TOPOLOGY,
                            FBE_CLASS_ID_INVALID,
                            fbe_persist_service.database_lun_object_id);

    fbe_topology_send_io_packet(subpacket);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*!***************************************************************************************
 * @file fbe_persist_database_write_persist_db_header_completion
 *****************************************************************************************
 *
 * @brief
 *  Completion routine of writing persist db header into header region on the DB LUN
 *
 * @author
 *  11/17/2012 - Created. Zhipeng Hu
 *
 ****************************************************************************************/
static fbe_status_t fbe_persist_database_write_persist_db_header_completion(fbe_packet_t * subpacket, fbe_packet_completion_context_t context)
{
    fbe_packet_t *                             request_packet;
    fbe_status_t                             status;
    fbe_payload_ex_t *                        payload;
    fbe_payload_block_operation_t *         block_operation;
    fbe_payload_block_operation_status_t    block_status;

    /* Get the original request packet (the one the user sent to set the LUN). */
    request_packet = (fbe_packet_t *)fbe_transport_get_master_packet(subpacket);
    fbe_transport_remove_subpacket(subpacket);

    /* Verify that the write request subpacket completed ok. */
    status = fbe_transport_get_status_code(subpacket);
    if (status != FBE_STATUS_OK) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s write failed, status %d\n", __FUNCTION__, status);
        fbe_transport_reuse_packet(subpacket);
        fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    
    payload = fbe_transport_get_payload_ex(subpacket);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s Can't get payload\n", __FUNCTION__);
        fbe_transport_reuse_packet(subpacket);
        fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Verify that the write completed ok. */
    block_operation = fbe_payload_ex_get_block_operation(payload);
    if (block_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s Can't get block_operation\n", __FUNCTION__);
        fbe_transport_reuse_packet(subpacket);
        fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    fbe_payload_block_get_status(block_operation, &block_status);
    if (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s Block_operation error, status: 0x%X\n", __FUNCTION__, block_status);
        fbe_payload_ex_release_block_operation(payload, block_operation);
        fbe_transport_reuse_packet(subpacket);
        fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    fbe_persist_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
        "%s write persist db header successfully\n", __FUNCTION__);

    /* We are done with the header write operation. */
    fbe_payload_ex_release_block_operation(payload, block_operation);
    fbe_transport_reuse_packet(subpacket);

    fbe_transport_set_status(request_packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(request_packet);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}


