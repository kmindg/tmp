#include "fbe_persist_private.h"
#include "fbe_persist_debug.h"
#include "fbe/fbe_xor_api.h"
#include "fbe/fbe_transport.h"
#include "fbe_transport_memory.h"
#include "fbe_topology.h"

static fbe_persist_tran_header_t * persist_tran_header;
static fbe_u8_t * persist_tran_buffer[FBE_PERSIST_TRAN_BUF_MAX];
static fbe_persist_tran_elem_t * persist_tran_elem[FBE_PERSIST_TRAN_ENTRY_MAX];
static fbe_sg_element_t * persist_tran_journal_sgl;
static fbe_sg_element_t * persist_tran_commit_sgl;
static fbe_u8_t * persist_tran_buffer_memory = NULL; 

static fbe_persist_transaction_handle_t next_tran_handle = 1;
static fbe_spinlock_t persist_tran_lock;
static fbe_packet_t * transaction_subpacket;/*we need only one since we are serailizing our transactions*/

extern fbe_persist_db_header_t          persist_db_header;

/**********************/
/*local declerations*/
/**********************/

static fbe_status_t fbe_persist_transaction_clear_marked_entries(void);
static void fbe_persist_clear_deleted_entries(void);
static void persist_commit_write_transaction(fbe_packet_t * request_packet, fbe_persist_tran_elem_t *tran_elem, fbe_u32_t index);
static fbe_status_t persist_write_entry_completion(fbe_packet_t * subpacket, fbe_packet_completion_context_t notused);
static fbe_status_t persist_write_journal(fbe_packet_t * packet);
static fbe_status_t fbe_persist_transaction_read_and_replay_journal_completion(fbe_packet_t * subpacket, fbe_packet_completion_context_t notused);
static fbe_status_t persist_transaction_replay_journal(fbe_packet_t *packet);
static fbe_status_t fbe_persist_transaction_read_and_replay_journal(fbe_packet_t *packet, fbe_packet_completion_context_t notused);
static void fbe_persist_transaction_invalidate_transaction_lock(void);
static void fbe_persist_transaction_invalidate_transaction(void);
static fbe_status_t fbe_persist_rebuild_entry_bitmap(fbe_packet_t *packet, fbe_packet_completion_context_t notused);
static fbe_status_t
persist_tran_journal_invalidate_completion(fbe_packet_t * subpacket, fbe_packet_completion_context_t notused);
static fbe_status_t persist_tran_write_journal_header_completion(fbe_packet_t * subpacket, fbe_packet_completion_context_t notused);


/**************************************************************************************************************************************************/
/* This local function sets the block checksums in the transaction buffers. */
static void
persist_tran_buffer_set_checksums(void)
{
    fbe_u32_t ii, jj;
    fbe_u32_t checksum;
    fbe_u8_t * buffer;
    fbe_u32_t * trailer;

    for (ii = 0; ii < FBE_PERSIST_TRAN_BUF_MAX; ii++) {
        buffer = persist_tran_buffer[ii];
        if (buffer == NULL) {
            break;
        }
        for (jj = 0; jj < FBE_PERSIST_BLOCKS_PER_TRAN_BUF; jj++) {
            trailer = (fbe_u32_t *)(buffer + FBE_PERSIST_DATA_BYTES_PER_BLOCK);
            checksum = fbe_xor_lib_calculate_checksum((fbe_u32_t*)buffer);
            *(trailer)++ = checksum;
            *trailer = 0;
            buffer += FBE_PERSIST_BYTES_PER_BLOCK;
        }
    }
}

/* This local function finds the transaction buffer that contains a specific transaction element. */
static fbe_status_t
persist_tran_buffer_find_buffer(fbe_persist_tran_elem_t * tran_elem, fbe_u32_t * buffer_index, fbe_u32_t * byte_offset)
{
    fbe_u32_t ii;
    fbe_u8_t * address;
    
    address = (void*)tran_elem;
    for (ii = 0; ii < FBE_PERSIST_TRAN_BUF_MAX; ii++) {
        if ((address >= persist_tran_buffer[ii]) && (address < (persist_tran_buffer[ii] + FBE_PERSIST_TRAN_BUF_SIZE))) {
            *buffer_index = ii;
            *byte_offset = (fbe_u32_t)((address - persist_tran_buffer[ii]) + sizeof(fbe_persist_tran_elem_header_t));
            return FBE_STATUS_OK;
        }
    }
    return FBE_STATUS_GENERIC_FAILURE;
}

/* This local function copies data into the transaction buffers. */
static fbe_status_t
persist_tran_buffer_copy_in(fbe_u32_t buffer_index, fbe_u32_t byte_offset, fbe_u8_t * data, fbe_u32_t size)
{
    fbe_u32_t byte_count;
    fbe_u32_t block_byte_offset;
    fbe_u8_t * buffer;

    byte_count = 0;
    block_byte_offset = byte_offset % FBE_PERSIST_BYTES_PER_BLOCK;
    if (block_byte_offset == FBE_PERSIST_DATA_BYTES_PER_BLOCK) {
        byte_offset += FBE_PERSIST_HEADER_BYTES_PER_BLOCK;
        block_byte_offset = 0;
    }
    while ((buffer_index < FBE_PERSIST_TRAN_BUF_MAX) && (byte_count < size)) {
        buffer = (fbe_u8_t*)persist_tran_buffer[buffer_index];
        while ((byte_offset < FBE_PERSIST_TRAN_BUF_SIZE) && (byte_count < size)) {
            buffer[byte_offset++] = data[byte_count++];
            block_byte_offset++;
            if (block_byte_offset == FBE_PERSIST_DATA_BYTES_PER_BLOCK) {
                byte_offset += FBE_PERSIST_HEADER_BYTES_PER_BLOCK;
                block_byte_offset = 0;
            }
        }
        buffer_index++;
        byte_offset = block_byte_offset = 0;
    }
    if (byte_count != size) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: copy-in failed! count: 0x%x, expected: 0x%x\n", __FUNCTION__, byte_count, size);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/* This local function copies data into the transaction buffers. */
static fbe_status_t
persist_tran_zero_entry(fbe_u32_t buffer_index, fbe_u32_t byte_offset)
{
    fbe_u32_t byte_count;
    fbe_u32_t block_byte_offset;
    fbe_u8_t * buffer;

    byte_count = 0;
    block_byte_offset = byte_offset % FBE_PERSIST_BYTES_PER_BLOCK;
    if (block_byte_offset == FBE_PERSIST_DATA_BYTES_PER_BLOCK) {
        byte_offset += FBE_PERSIST_HEADER_BYTES_PER_BLOCK;
        block_byte_offset = 0;
    }
    
    while ((buffer_index < FBE_PERSIST_TRAN_BUF_MAX) && (byte_count < FBE_PERSIST_DATA_BYTES_PER_ENTRY)) {
        buffer = (fbe_u8_t*)persist_tran_buffer[buffer_index];
        while ((byte_offset < FBE_PERSIST_TRAN_BUF_SIZE) && (byte_count < FBE_PERSIST_DATA_BYTES_PER_ENTRY)) {
            buffer[byte_offset++] = 0;/*could be a bit more efficient because we do it one byte at a time, but for now it's not a big deal*/
			byte_count++;
            block_byte_offset++;
            if (block_byte_offset == FBE_PERSIST_DATA_BYTES_PER_BLOCK) {
                byte_offset += FBE_PERSIST_HEADER_BYTES_PER_BLOCK;
                block_byte_offset = 0;
            }
        }
        buffer_index++;
        byte_offset = block_byte_offset = 0;
    }
    if (byte_count != FBE_PERSIST_DATA_BYTES_PER_ENTRY) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: copy-in failed! count: 0x%x, expected: 0x%x\n", __FUNCTION__, byte_count, FBE_PERSIST_DATA_BYTES_PER_ENTRY);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

/* This local function zeros out the transaction buffers. */
static void
persist_tran_buffer_zero(void)
{
    fbe_u32_t ii;

    for (ii = 0; ii < FBE_PERSIST_TRAN_BUF_MAX; ii++) {
        if (persist_tran_buffer[ii] != NULL) {
            fbe_zero_memory(persist_tran_buffer[ii],  FBE_PERSIST_TRAN_BUF_SIZE);
        }
        else {
            break;
        }
    }
}

/* This local function releases the memory allocated for the transaction buffers. */
static void
persist_tran_buffer_destroy(void)
{
    fbe_u32_t ii;

    if (persist_tran_journal_sgl != NULL) {
        fbe_memory_native_release(persist_tran_journal_sgl);
        persist_tran_journal_sgl = NULL;
    }

	if (persist_tran_commit_sgl != NULL) {
        fbe_memory_native_release(persist_tran_commit_sgl);
        persist_tran_commit_sgl = NULL;
    }


    for (ii = 0; ii < FBE_PERSIST_TRAN_ENTRY_MAX; ii++) {
        persist_tran_elem[ii] = NULL;
    }

    if (persist_tran_buffer_memory != NULL) {
        fbe_memory_release_required(persist_tran_buffer_memory);
        persist_tran_buffer_memory = NULL;
    }

    for (ii = 0; ii < FBE_PERSIST_TRAN_BUF_MAX; ii++) {
        if (persist_tran_buffer[ii] != NULL) {
            persist_tran_buffer[ii] = NULL;
        }
    }

	fbe_memory_native_release(persist_tran_header);
}

/* This local function allocates the memory for the transaction buffers,
 * and initializes the associated tracking structures. */
static fbe_status_t
persist_tran_buffer_init(void)
{
    fbe_u32_t ii, jj, kk, pp;
    fbe_u32_t sgl_size;
    fbe_persist_tran_elem_t * tran_elem;
    fbe_u8_t *tran_buffer_ptr;

    /* Pre-initialize everything. */
    persist_tran_header = NULL;
    fbe_zero_memory(persist_tran_buffer,  sizeof(persist_tran_buffer));
    fbe_zero_memory(persist_tran_elem,  sizeof(persist_tran_elem));
    persist_tran_journal_sgl = NULL;

    /* Allocate memory for the transaction journal sgl. */
    sgl_size = FBE_PERSIST_SGL_ELEM_PER_TRAN_JOURNAL * sizeof(fbe_sg_element_t);
    persist_tran_journal_sgl = (fbe_sg_element_t*)fbe_memory_native_allocate(sgl_size);
    if (persist_tran_journal_sgl == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: transaction journal sgl memory allocation failed! \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	/*allocate memory for the commit sgl*/
	sgl_size = FBE_PERSIST_SGL_ELEM_PER_TRAN_COMMIT * sizeof(fbe_sg_element_t) * 2;/*we need twice as many entries because for each one we need a terminating one as well*/
    persist_tran_commit_sgl = (fbe_sg_element_t*)fbe_memory_native_allocate(sgl_size);
    if (persist_tran_commit_sgl == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: transaction commit sgl memory allocation failed! \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
	
    /* Allocate the transaction buffers memory. */
    persist_tran_buffer_memory = fbe_memory_allocate_required(FBE_PERSIST_TRAN_BUF_MAX * FBE_PERSIST_TRAN_BUF_SIZE);
    if (persist_tran_buffer_memory == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: transaction memory allocation failed! \n", __FUNCTION__);
        persist_tran_buffer_destroy();
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Allocate the transaction buffers. */
    tran_buffer_ptr = persist_tran_buffer_memory;
    for (ii = jj = pp= 0; ii < FBE_PERSIST_TRAN_BUF_MAX; ii++) {
        persist_tran_buffer[ii] = tran_buffer_ptr;
        tran_buffer_ptr += FBE_PERSIST_TRAN_BUF_SIZE;

        /* Create an sgl entry for this buffer. */
        fbe_sg_element_init(&persist_tran_journal_sgl[ii], FBE_PERSIST_TRAN_BUF_SIZE, (void*)persist_tran_buffer[ii]);
        /* Cycle across this buffer setting the tran element pointers. */
        tran_elem = (fbe_persist_tran_elem_t*)persist_tran_buffer[ii];
        for (kk = 0; kk < FBE_PERSIST_TRAN_ENTRY_PER_TRAN_BUF; kk++) {
			/* Create an sgl entry for this buffer. 
			the pp index just makes sure we keep filling the whole 64 entries of sgl with data in the even ones and termination in the odd ones*/
            fbe_sg_element_init(&persist_tran_commit_sgl[pp++], FBE_PERSIST_TRAN_BUF_SIZE / 2, (void*)(persist_tran_buffer[ii] + (kk * (FBE_PERSIST_TRAN_BUF_SIZE / 2))));
			/*and fill the actual table that points to each entry*/
            persist_tran_elem[jj++] = tran_elem++;
			fbe_sg_element_terminate(&persist_tran_commit_sgl[pp++]);/*will terminate the one just below the one we just set*/
			
			
        }
    }
    /* Null terminate the sgl. */
    fbe_sg_element_init(&persist_tran_journal_sgl[ii], 0, NULL);
    /* Clean the buffers so we don't think that undefined memory contains a transaction. */
    persist_tran_buffer_zero();

    /* The first transaction element is overlayed with the transaction header. */
    //persist_tran_header = (fbe_persist_tran_header_t*)persist_tran_elem[0];
	persist_tran_header = fbe_memory_native_allocate(sizeof(fbe_persist_tran_header_t));
    persist_tran_header->tran_handle = FBE_PERSIST_TRAN_HANDLE_INVALID;

    return FBE_STATUS_OK;
}


static void
persist_tran_elem_commit(fbe_packet_t * request_packet, fbe_persist_tran_elem_t * tran_elem, fbe_u32_t index)
{
    fbe_persist_tran_elem_header_t * tran_elem_header;

    tran_elem_header = &tran_elem->tran_elem_header;
    switch (tran_elem_header->header.tran_elem_op) {
		/*at the end of the day, everything is a write, even the FBE_PERSIST_TRAN_OP_DELETE_ENTRY already filled the buffer with zeros*/
		case FBE_PERSIST_TRAN_OP_WRITE_ENTRY:
		case FBE_PERSIST_TRAN_OP_MODIFY_ENTRY:
		case FBE_PERSIST_TRAN_OP_DELETE_ENTRY:
			persist_commit_write_transaction(request_packet, tran_elem, index);
            break;
        default:
            fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s commit failed, unexpected operation %llu\n",
                          __FUNCTION__, (unsigned long long)tran_elem_header->header.tran_elem_op);
			fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
			fbe_transport_complete_packet(request_packet);
        break;
    }
   
}

/*zero all entries in the transaction and write it as zeros again to utilize existing functions*/
static void persist_tran_journal_invalidate(fbe_packet_t * request_packet)
{
    fbe_persist_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "%s entry, caller:%d\n", __FUNCTION__, request_packet->package_id);
    

	/*zero everything first*/
    fbe_spinlock_lock(&persist_tran_lock);
	persist_tran_header->tran_state = FBE_PERSIST_TRAN_STATE_INVALIDATE_JOURNAL;
    fbe_spinlock_unlock(&persist_tran_lock);

    /*check hooks*/
    persist_check_and_run_hook(request_packet, FBE_PERSIST_HOOK_TYPE_SHRINK_LIVE_TRANSACTION_AND_WAIT, 
                               persist_tran_header, NULL, NULL);
    persist_check_and_run_hook(request_packet, FBE_PERSIST_HOOK_TYPE_PANIC_WHEN_WRITING_LIVE_DATA_REGION, 
                               persist_tran_header, NULL, NULL);
    persist_check_and_run_hook(request_packet, FBE_PERSIST_HOOK_TYPE_WAIT_BEFORE_INVALIDATE_JOURNAL_HEADER,
                               persist_tran_header, NULL, NULL);
    
    persist_db_header.signature = FBE_PERSIST_DB_SIGNATURE;
    persist_db_header.version = FBE_PERSIST_DB_VERSION;
    persist_db_header.journal_state = 0;
    persist_db_header.journal_size  = 0;

    /*set completion function here*/
    fbe_transport_set_completion_function(request_packet, persist_tran_journal_invalidate_completion, NULL);
    
    fbe_persist_database_write_persist_db_header(request_packet);
}

/* This local function cycles over the elements of a transaction, committing each element.
 * Once all of the elements are committed, it completes the original FBE_PERSIST_TRANSACTION_OP_COMMIT
 * request packet.  This function is called from the completion context of the journal write and also
 * from the completion context of each element commit.
 */
static void
persist_transaction_commit(fbe_packet_t * request_packet)
{
    fbe_persist_tran_elem_t * tran_elem;

    fbe_persist_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "%s entry\n", __FUNCTION__);

    fbe_spinlock_lock(&persist_tran_lock);
    /* Check to see if the transaction was aborted. */
    if (persist_tran_header->tran_state == FBE_PERSIST_TRAN_STATE_ABORT) {
        fbe_spinlock_unlock(&persist_tran_lock);
        fbe_transport_set_status(request_packet, FBE_STATUS_CANCELED, 0);
        fbe_transport_complete_packet(request_packet);
        fbe_persist_transaction_abort(persist_tran_header->tran_handle);
        return;
    }else if (persist_tran_header->tran_state == FBE_PERSIST_TRAN_STATE_JOURNAL) {
        /* Change the transaction state to indicate that the commit is ongoing. */
        persist_tran_header->tran_state = FBE_PERSIST_TRAN_STATE_COMMIT;
    }else if (persist_tran_header->tran_state != FBE_PERSIST_TRAN_STATE_COMMIT) {
        fbe_spinlock_unlock(&persist_tran_lock);
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s commit failed, unexpected state %x\n",
                          __FUNCTION__, persist_tran_header->tran_state);
        fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet);
        return;
    }
    fbe_spinlock_unlock(&persist_tran_lock);

    /* Commit the next transaction element. */
    if (persist_tran_header->tran_elem_committed_count < persist_tran_header->tran_elem_count) {
        tran_elem = persist_tran_elem[persist_tran_header->tran_elem_committed_count];
        persist_tran_elem_commit(request_packet, tran_elem, persist_tran_header->tran_elem_committed_count);
		persist_tran_header->tran_elem_committed_count++;
        /* Each transaction element commit is an asynchronous operation,
         * any additional commits will happen after this one completes. */
        return;
    }

    /* We are done with writing everything, let's just invalidate the journal.
	Upon completion, we are supposed to get to this function again and pick the FBE_PERSIST_TRAN_STATE_INVALIDATE_JOURNAL if statement. */
    persist_tran_journal_invalidate(request_packet);
}

/* This local function is called when the write of the transaction journal completes (in the packet
 * completion context).  If the write was successful, this function will start the process of committing
 * the transaction. */
static fbe_status_t
persist_write_journal_completion(fbe_packet_t * subpacket, fbe_packet_completion_context_t notused)
{
    fbe_packet_t * request_packet;
    fbe_status_t status;
    fbe_payload_ex_t *payload;
    fbe_payload_block_operation_t * block_operation;
    fbe_payload_block_operation_status_t block_status;

    /* Get the commit request packet. */
    request_packet = (fbe_packet_t *)fbe_transport_get_master_packet(subpacket);
    fbe_transport_remove_subpacket(subpacket);

    /* Verify that the write request subpacket completed ok. */
    status = fbe_transport_get_status_code(subpacket);
    if (status != FBE_STATUS_OK) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Journal write failed\n", __FUNCTION__);
        fbe_transport_reuse_packet(subpacket);
		fbe_persist_transaction_invalidate_transaction();
        fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    payload = fbe_transport_get_payload_ex(subpacket);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Can't get payload\n", __FUNCTION__);
        fbe_transport_reuse_packet(subpacket);
		fbe_persist_transaction_invalidate_transaction();
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
		fbe_persist_transaction_invalidate_transaction();
        fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    fbe_payload_block_get_status(block_operation, &block_status);
    if (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Block_operation error, status: 0x%X\n", __FUNCTION__, block_status);
        fbe_transport_reuse_packet(subpacket);
		fbe_persist_transaction_invalidate_transaction();
        fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    fbe_persist_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                      "%s journal write successfully\n", __FUNCTION__);

    /* We are done with the journal write operation. */
    fbe_payload_ex_release_block_operation(payload, block_operation);
    fbe_transport_reuse_packet(subpacket);

    persist_check_and_run_hook(request_packet, FBE_PERSIST_HOOK_TYPE_WAIT_BEFORE_MARK_JOURNAL_HEADER_VALID, 
                               persist_tran_header, NULL, NULL);

    /* Now write journal header*/
    fbe_transport_set_completion_function(request_packet, persist_tran_write_journal_header_completion, NULL);
    fbe_spinlock_lock(&persist_tran_lock);
    persist_db_header.version = FBE_PERSIST_DB_VERSION;
    persist_db_header.signature = FBE_PERSIST_DB_SIGNATURE;
    persist_db_header.journal_state = FBE_PERSIST_DB_JOURNAL_VALID_STATE;
    persist_db_header.journal_size = persist_tran_header->tran_elem_count;
    persist_tran_header->tran_state = FBE_PERSIST_TRAN_STATE_WRITE_HEADER;
    fbe_spinlock_unlock(&persist_tran_lock);

    /*check hooks*/
    persist_check_and_run_hook(request_packet, FBE_PERSIST_HOOK_TYPE_SHRINK_JOURNAL_AND_WAIT, 
                                persist_tran_header, NULL, NULL);
    persist_check_and_run_hook(request_packet, FBE_PERSIST_HOOK_TYPE_PANIC_WHEN_WRITING_JOURNAL_REGION,
                              persist_tran_header, NULL, NULL);
    
    fbe_persist_database_write_persist_db_header(request_packet);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

static fbe_packet_t *persist_get_subpacket(void)
{
    fbe_packet_t *packet = transaction_subpacket;
    fbe_payload_ex_t *payload = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_allocate_memory_operation(payload);

    return packet;
}

/* This local function writes the transaction journal prior to committing the transaction. */
static fbe_status_t persist_write_journal(fbe_packet_t * packet)
{
    fbe_packet_t * subpacket;
    fbe_payload_ex_t * payload;
    fbe_payload_block_operation_t * block_operation;
    fbe_lba_t tran_journal_lba;

    /* Create a subpacket for an I/O that writes the transaction to the journal. */
    subpacket = persist_get_subpacket();

    fbe_persist_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(subpacket);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Can't get payload\n", __FUNCTION__);
        fbe_transport_reuse_packet(subpacket);
		fbe_persist_transaction_invalidate_transaction();
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set up a block operation to write the entire transaction to the journal. */
    block_operation = fbe_payload_ex_allocate_block_operation(payload);
    if (block_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Can't get block_operation\n", __FUNCTION__);
        fbe_transport_reuse_packet(subpacket);
		fbe_persist_transaction_invalidate_transaction();
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*check hooks*/
    persist_check_and_run_hook(packet, FBE_PERSIST_HOOK_TYPE_SHRINK_JOURNAL_AND_WAIT,
                               persist_tran_header, NULL, NULL);
    persist_check_and_run_hook(packet, FBE_PERSIST_HOOK_TYPE_PANIC_WHEN_WRITING_JOURNAL_REGION, 
                               persist_tran_header, NULL , NULL);

    tran_journal_lba = fbe_persist_database_get_tran_journal_lba();
    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                      tran_journal_lba,
                                      FBE_PERSIST_BLOCKS_PER_TRAN,
                                      FBE_PERSIST_BYTES_PER_BLOCK,
                                      0,     /* optimum block size (not used) */
                                      NULL); /* preread descriptor (not used) */
    fbe_payload_ex_increment_block_operation_level(payload);

    /* Do the block-level checksums across the transaction buffers. */
    persist_tran_buffer_set_checksums();
    /* Put an sgl pointer in the packet payload. */
    fbe_payload_ex_set_sg_list(payload, persist_tran_journal_sgl, FBE_PERSIST_SGL_ELEM_PER_TRAN_JOURNAL);
    /* Link the subpacket into the caller's request packet. */
    fbe_transport_add_subpacket(packet, subpacket);

    /* Send send down the journal write. */
    fbe_transport_set_completion_function(subpacket, persist_write_journal_completion, NULL);
    fbe_transport_set_address(subpacket,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              fbe_persist_service.database_lun_object_id);
    fbe_topology_send_io_packet(subpacket);
    /* If the status is FBE_STATUS_OK the original commit request is asynchronous from here on. */
    return FBE_STATUS_OK;
}

/* This external function is called when the service is first initialized. */
fbe_status_t
fbe_persist_transaction_init(void)
{
    fbe_status_t status;

    status = persist_tran_buffer_init();
    if (status != FBE_STATUS_OK) {
        return status;
    }
    fbe_spinlock_init(&persist_tran_lock);

	transaction_subpacket = fbe_transport_allocate_packet();
	if (transaction_subpacket == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "%s Can't allocate packet\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	status = fbe_transport_initialize_sep_packet(transaction_subpacket);
	if (status != FBE_STATUS_OK) {
		fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "%s failed to init packet\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
	}

    return FBE_STATUS_OK;
}

/* This external function is called when the service is terminated. */
void
fbe_persist_transaction_destroy(void)
{
	fbe_persist_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "%s Entry\n", __FUNCTION__);
    fbe_spinlock_lock(&persist_tran_lock);
    /* Is there a outstanding transaction? If so, wait of it. */
    while (persist_tran_header->tran_state != FBE_PERSIST_TRAN_HANDLE_INVALID) {
        fbe_persist_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: there is a outstanding transaction! State is 0x%x\n", __FUNCTION__, persist_tran_header->tran_state);
        fbe_spinlock_unlock(&persist_tran_lock);
        fbe_thread_delay(100);
        fbe_spinlock_lock(&persist_tran_lock);
    }

    if (persist_tran_header->tran_handle != FBE_PERSIST_TRAN_HANDLE_INVALID) {
        fbe_spinlock_unlock(&persist_tran_lock);
	fbe_persist_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
			  "%s persist_tran_header->tran_handle unexpect:%llu\n",
			  __FUNCTION__,
			  (unsigned long long)persist_tran_header->tran_handle);
        (void)fbe_persist_transaction_abort(persist_tran_header->tran_handle);
    }
    else {
        fbe_spinlock_unlock(&persist_tran_lock);
    }

    fbe_spinlock_destroy(&persist_tran_lock);
    persist_tran_buffer_destroy();

	fbe_transport_release_packet(transaction_subpacket);
	fbe_persist_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "%s Done\n", __FUNCTION__);
    return;
}

/* This external function is called when a FBE_PERSIST_CONTROL_CODE_WRITE_ENTRY request is received.
 * It copies the request into a new transaction element in the transaction buffer. it will also return 
 * the  entry id it will assign.
 */
fbe_status_t
fbe_persist_transaction_write_entry(fbe_persist_transaction_handle_t callers_tran_handle,
									fbe_persist_sector_type_t target_sector,
                                    fbe_persist_entry_id_t *entry_id,
                                    fbe_u8_t * data,
                                    fbe_u32_t byte_count,
									fbe_bool_t auto_insert_entry_id)
{
    fbe_persist_tran_elem_t * tran_elem;
    fbe_u32_t buffer_index;
    fbe_u32_t byte_offset;
    fbe_status_t status;

    fbe_spinlock_lock(&persist_tran_lock);
    /* Is there a current transaction? */
    if (persist_tran_header->tran_handle == FBE_PERSIST_TRAN_HANDLE_INVALID) {
        fbe_spinlock_unlock(&persist_tran_lock);
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: there is no current transaction!\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Is callers transaction handle for the current transaction? */
    if (callers_tran_handle != persist_tran_header->tran_handle) {
        fbe_spinlock_unlock(&persist_tran_lock);
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: transaction requested is not for the current transaction, got 0x%llX, expected 0x%llX!\n",
                      __FUNCTION__, (unsigned long long)callers_tran_handle,
		      (unsigned long long)persist_tran_header->tran_handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Is the current transaction pending, waiting for more transaction elements? */
    if (persist_tran_header->tran_state != FBE_PERSIST_TRAN_STATE_PENDING) {
        fbe_spinlock_unlock(&persist_tran_lock);
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: current transaction state is invalid, found 0x%X, expected 0x%X!\n",
                      __FUNCTION__, persist_tran_header->tran_state, FBE_PERSIST_TRAN_STATE_PENDING);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Do we have more transaction elements available in the transaction buffer? */
    if (persist_tran_header->tran_elem_count == FBE_PERSIST_TRAN_ENTRY_MAX) {
        fbe_spinlock_unlock(&persist_tran_lock);
        fbe_persist_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: 0x%x exceeds max 0x%x. Transaction element overflow!\n", 
                          __FUNCTION__, persist_tran_header->tran_elem_count, FBE_PERSIST_TRAN_ENTRY_MAX);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Find the buffer that contains the current transaction element. */
    tran_elem = persist_tran_elem[persist_tran_header->tran_elem_count];
    status = persist_tran_buffer_find_buffer(tran_elem, &buffer_index, &byte_offset);
    if (status != FBE_STATUS_OK) {
        fbe_spinlock_unlock(&persist_tran_lock);
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: transaction element corruption!\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	/*if the first 64 bits of data the user sent are reserved for an entry ID and the user wants to save the time of
	re-writing the data again after getting the entry ID from this function, we can do it for him*/
	if (auto_insert_entry_id) {
        /*generate entry id for the next free space in the selected sector*/
        status = fbe_persist_database_get_entry_id(entry_id, target_sector);
        if (status != FBE_STATUS_OK) {
             fbe_spinlock_unlock(&persist_tran_lock);
             fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: failed to get a free entry id\n", __FUNCTION__);
             return FBE_STATUS_GENERIC_FAILURE;
        }        
		fbe_copy_memory(data, entry_id, sizeof(fbe_persist_entry_id_t));
	}

	/* Populate the transaction element. */
	tran_elem->tran_elem_header.header.tran_elem_op = FBE_PERSIST_TRAN_OP_WRITE_ENTRY;
	persist_tran_header->tran_elem_count++; /*for next time*/
    tran_elem->tran_elem_header.header.data_length = byte_count;
	tran_elem->tran_elem_header.header.valid = FBE_TRUE;
	tran_elem->tran_elem_header.header.entry_id = *entry_id;
    status = persist_tran_buffer_copy_in(buffer_index, byte_offset, data, byte_count);
    if (status != FBE_STATUS_OK) {
        fbe_spinlock_unlock(&persist_tran_lock);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_spinlock_unlock(&persist_tran_lock);
    return FBE_STATUS_OK;
}

/* This external function is the starting function for handling a FBE_PERSIST_TRANSACTION_OP_START request. */
fbe_status_t
fbe_persist_transaction_start(fbe_packet_t * packet, 
                              fbe_persist_transaction_handle_t * return_tran_handle)
{
    fbe_persist_transaction_handle_t tran_handle;
    fbe_persist_transaction_handle_t current_tran_handle;
    fbe_bool_t  sep_hook_is_set = FBE_FALSE;

    *return_tran_handle = tran_handle = FBE_PERSIST_TRAN_HANDLE_INVALID;

    fbe_persist_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                  "%s entry.\n", __FUNCTION__);

    persist_check_and_run_hook(packet, FBE_PERSIST_HOOK_TYPE_RETURN_FAILED_TRANSACTION,
                               NULL, NULL, &sep_hook_is_set);
    if(FBE_TRUE == sep_hook_is_set)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* We only allow one transaction at a time, so there cannot be a current transaction in progress. */
    fbe_spinlock_lock(&persist_tran_lock);
    current_tran_handle = persist_tran_header->tran_handle;
    if (persist_tran_header->tran_state == FBE_PERSIST_TRAN_STATE_INVALID) {
		if(current_tran_handle != FBE_PERSIST_TRAN_HANDLE_INVALID)
	    {
			/*This should never happen. We trace this unexpected state here for tracing potential risk*/
			fbe_spinlock_unlock(&persist_tran_lock);
			fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
						  "%s: tran_state is invalid but tran handle is 0x%llx.\n",
						  __FUNCTION__,
						  current_tran_handle);
			return FBE_STATUS_GENERIC_FAILURE;
		}
		
        tran_handle = next_tran_handle++;
        if (next_tran_handle == FBE_PERSIST_TRAN_HANDLE_INVALID) {
            next_tran_handle++;
        }
        /* Zero out the previous (completed) transaction. */
        persist_tran_buffer_zero();
        /* Initialize the transaction header. */
        persist_tran_header->tran_handle = tran_handle;
		persist_tran_header->tran_state = FBE_PERSIST_TRAN_STATE_PENDING;
		persist_tran_header->tran_elem_count = 0;
		persist_tran_header->tran_elem_committed_count = 0;
        /* Return the transaction handle. */
       *return_tran_handle = tran_handle;
    }
    fbe_spinlock_unlock(&persist_tran_lock);

    /* There is a current transaction, so this request for a new transaction is an error. */
    if (tran_handle == FBE_PERSIST_TRAN_HANDLE_INVALID) {
        fbe_persist_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                      "%s: there is a tran 0x%llx in progress!\n", __FUNCTION__, (unsigned long long)current_tran_handle);
        return FBE_STATUS_BUSY;
    }
    else
    {
        fbe_persist_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                      "%s: successfully start new tran 0x%llx.\n", __FUNCTION__, (unsigned long long)tran_handle);
    }

    return FBE_STATUS_OK;
}

/* This external function is the starting function for handling a FBE_PERSIST_TRANSACTION_OP_ABORT request. */
fbe_status_t
fbe_persist_transaction_abort(fbe_persist_transaction_handle_t callers_tran_handle)
{
	fbe_status_t	status;

    fbe_spinlock_lock(&persist_tran_lock);
    if (persist_tran_header->tran_handle == FBE_PERSIST_TRAN_HANDLE_INVALID) {
        /* Do nothing if there is no current transaction to abort. */
        fbe_spinlock_unlock(&persist_tran_lock);
		fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: Can't abort while there is no transaction open\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (callers_tran_handle != persist_tran_header->tran_handle) {
        /* Caller wants to abort a transaction that is not the current transaction, that's an error. */
        fbe_spinlock_unlock(&persist_tran_lock);
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: transaction abort requested, but not for the current transaction, got 0x%llX, expected 0x%llX!\n",
                      __FUNCTION__, (unsigned long long)callers_tran_handle,
		      (unsigned long long)persist_tran_header->tran_handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* We only allow to abort the transaction when it is in pending state.
     * In other state, we are in the middle of journal operation and it is not safe to
     * abort it.*/
    if (persist_tran_header->tran_state != FBE_PERSIST_TRAN_STATE_PENDING) {
        fbe_persist_tran_state_t cur_state = persist_tran_header->tran_state;
        fbe_spinlock_unlock(&persist_tran_lock);
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: transaction abort requested, but not current state is: %u\n",
                          __FUNCTION__, (fbe_u32_t)cur_state);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	/*we need to clear all the bits of the entries we marked as consumed*/
	status = fbe_persist_transaction_clear_marked_entries();
	if (status != FBE_STATUS_OK) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: clear marked entries failed, continue to abort the transaction\n",
                          __FUNCTION__);
        /* continue */
	}
    fbe_persist_transaction_invalidate_transaction_lock();
    fbe_spinlock_unlock(&persist_tran_lock);
    return FBE_STATUS_OK;
}

/* This external function is the starting function for handling a FBE_PERSIST_TRANSACTION_OP_COMMIT request. */
fbe_status_t
fbe_persist_transaction_commit(fbe_packet_t * packet, fbe_persist_transaction_handle_t callers_tran_handle)
{
    fbe_status_t status;
	fbe_payload_ex_t * payload;
    fbe_payload_control_operation_t * control_operation;

    fbe_persist_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "%s: entry\n", __FUNCTION__);
    
	payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: get_sep_payload failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: get_control_operation failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_lock(&persist_tran_lock);
    if (callers_tran_handle != persist_tran_header->tran_handle) {
        /* Caller wants to commit a transaction but its not the current transaction. */
        fbe_spinlock_unlock(&persist_tran_lock);
        fbe_persist_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: transaction commit requested, but the current transaction is corrupt, found 0x%llX, expected 0x%llX!\n",
                      __FUNCTION__, callers_tran_handle, persist_tran_header->tran_handle);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (persist_tran_header->tran_state != FBE_PERSIST_TRAN_STATE_PENDING) {
        /* Caller wants to commit a transaction but the current transaction is not commitable. */
        fbe_spinlock_unlock(&persist_tran_lock);
        fbe_persist_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: transaction commit requested, but the current transaction state is invalid, found 0x%X, expected 0x%X!\n",
                      __FUNCTION__, persist_tran_header->tran_state, FBE_PERSIST_TRAN_STATE_PENDING);
		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*we now need support commit an empty transaction by doing nothing. 
      let the transaction commit request go as normal*/
    if (persist_tran_header->tran_elem_count == 0) {
        /* Caller wants to commit a transaction but there are no sub-transactions. */
        persist_tran_header->tran_handle = FBE_PERSIST_TRAN_HANDLE_INVALID;
        persist_tran_header->tran_state = FBE_PERSIST_TRAN_STATE_INVALID;
        fbe_spinlock_unlock(&persist_tran_lock);
        fbe_persist_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: transaction commit requested, but the current transaction is empty!\n", __FUNCTION__);        
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    /* Change the transaction state to prevent further operations, but release the lock so we can fail any
     * future transaction requests that arrive while we are commiting. */
    persist_tran_header->tran_state = FBE_PERSIST_TRAN_STATE_JOURNAL;
    fbe_spinlock_unlock(&persist_tran_lock);

    /* Write the transaction journal. If an OK status is returned then an asynchronous write was issued
     * and the caller's packet will get completed when the commit finishes. */
    status = persist_write_journal(packet);
    return status;
}

/* This external function is called when a FBE_PERSIST_CONTROL_CODE_MODIFY_ENTRY request is received.
 * It copies the request into an existing transaction element in the transaction buffer.
 */
fbe_status_t
fbe_persist_transaction_modify_entry(fbe_persist_transaction_handle_t callers_tran_handle,
                                    fbe_persist_entry_id_t entry_id,
                                    fbe_u8_t * data,
                                    fbe_u32_t byte_count)
{
    fbe_persist_tran_elem_t * 	tran_elem;
    fbe_u32_t 					buffer_index;
    fbe_u32_t 					byte_offset;
    fbe_status_t 				status;
	fbe_bool_t 					entry_exists = FBE_FALSE;
    
    fbe_spinlock_lock(&persist_tran_lock);
    /* Is there a current transaction? */
    if (persist_tran_header->tran_handle == FBE_PERSIST_TRAN_HANDLE_INVALID) {
        fbe_spinlock_unlock(&persist_tran_lock);
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: there is no current transaction!\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Is callers transaction handle for the current transaction? */
    if (callers_tran_handle != persist_tran_header->tran_handle) {
        fbe_spinlock_unlock(&persist_tran_lock);
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: transaction requested is not for the current transaction, got 0x%llX, expected 0x%llX!\n",
                      __FUNCTION__, (unsigned long long)callers_tran_handle,
		      (unsigned long long)persist_tran_header->tran_handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Is the current transaction pending, waiting for more transaction elements? */
    if (persist_tran_header->tran_state != FBE_PERSIST_TRAN_STATE_PENDING) {
        fbe_spinlock_unlock(&persist_tran_lock);
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: current transaction state is invalid, found 0x%X, expected 0x%X!\n",
                      __FUNCTION__, persist_tran_header->tran_state, FBE_PERSIST_TRAN_STATE_PENDING);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	/*do we even have an existing transaction on the disk that we can modify?*/
	status = fbe_persist_entry_exists(entry_id, &entry_exists);
	if ((!entry_exists) || (status != FBE_STATUS_OK)) {
		fbe_spinlock_unlock(&persist_tran_lock);
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: can't modify non existing entry:0x%llX!\n",
                      __FUNCTION__, (unsigned long long)entry_id);
        return FBE_STATUS_GENERIC_FAILURE;
	}

    /* Do we have more transaction elements available in the transaction buffer? */
    if (persist_tran_header->tran_elem_count == FBE_PERSIST_TRAN_ENTRY_MAX) {
        fbe_spinlock_unlock(&persist_tran_lock);
        fbe_persist_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: transaction element overflow!\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	/* Find the buffer that contains the current transaction element. */
    tran_elem = persist_tran_elem[persist_tran_header->tran_elem_count];
    status = persist_tran_buffer_find_buffer(tran_elem, &buffer_index, &byte_offset);
    if (status != FBE_STATUS_OK) {
        fbe_spinlock_unlock(&persist_tran_lock);
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: transaction element corruption!\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	persist_tran_header->tran_elem_count++;/*for next time*/

    /* Populate the transaction element. */
    tran_elem->tran_elem_header.header.tran_elem_op = FBE_PERSIST_TRAN_OP_MODIFY_ENTRY;
    tran_elem->tran_elem_header.header.entry_id = entry_id;
    tran_elem->tran_elem_header.header.data_length = byte_count;
	tran_elem->tran_elem_header.header.valid = FBE_TRUE;
    status = persist_tran_buffer_copy_in(buffer_index, byte_offset, data, byte_count);
    if (status != FBE_STATUS_OK) {
        fbe_spinlock_unlock(&persist_tran_lock);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_spinlock_unlock(&persist_tran_lock);
    return FBE_STATUS_OK;
}

/*clear the write entries when we abort so we can use them for new writes*/
static fbe_status_t fbe_persist_transaction_clear_marked_entries(void)
{
	fbe_u32_t		max_entry = persist_tran_header->tran_elem_count;
	fbe_u32_t		entry;
	fbe_status_t	status, return_status = FBE_STATUS_OK;
	

	for (entry = 0; entry < max_entry; entry ++) {
		/*clear only the ones that were new writes, the rest should stay there to indicate */
		if (FBE_PERSIST_TRAN_OP_WRITE_ENTRY == persist_tran_elem[entry]->tran_elem_header.header.tran_elem_op) {
			status = fbe_persist_clear_entry_bit(persist_tran_elem[entry]->tran_elem_header.header.entry_id);
			if (status != FBE_STATUS_OK) {
				fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: failed to clear entry no:%d!\n", __FUNCTION__, entry);

				return_status = FBE_STATUS_GENERIC_FAILURE;
				
			}
		}
	}

	return return_status;
}

/* This external function is called when a FBE_PERSIST_CONTROL_CODE_DELETE_ENTRY request is received.
 * It marks the entry for deletion once we commit
 */
fbe_status_t
fbe_persist_transaction_delete_entry(fbe_persist_transaction_handle_t callers_tran_handle,
                                    fbe_persist_entry_id_t entry_id)
{
    fbe_persist_tran_elem_t * tran_elem;
    fbe_u32_t buffer_index;
    fbe_u32_t byte_offset;
    fbe_status_t status;
	fbe_bool_t 	entry_exists = FBE_FALSE;
    
    fbe_spinlock_lock(&persist_tran_lock);
    /* Is there a current transaction? */
    if (persist_tran_header->tran_handle == FBE_PERSIST_TRAN_HANDLE_INVALID) {
        fbe_spinlock_unlock(&persist_tran_lock);
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: there is no current transaction!\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Is callers transaction handle for the current transaction? */
    if (callers_tran_handle != persist_tran_header->tran_handle) {
        fbe_spinlock_unlock(&persist_tran_lock);
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: transaction requested is not for the current transaction, got 0x%llX, expected 0x%llX!\n",
                      __FUNCTION__, (unsigned long long)callers_tran_handle,
		      (unsigned long long)persist_tran_header->tran_handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Is the current transaction pending, waiting for more transaction elements? */
    if (persist_tran_header->tran_state != FBE_PERSIST_TRAN_STATE_PENDING) {
        fbe_spinlock_unlock(&persist_tran_lock);
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: current transaction state is invalid, found 0x%X, expected 0x%X!\n",
                      __FUNCTION__, persist_tran_header->tran_state, FBE_PERSIST_TRAN_STATE_PENDING);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	/*do we even have an existing transaction on the disk that we can deleted?*/
	status = fbe_persist_entry_exists(entry_id, &entry_exists);
	if ((!entry_exists) || (status != FBE_STATUS_OK)) {
		fbe_spinlock_unlock(&persist_tran_lock);
        fbe_persist_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: can't delete non existing entry:0x%llX !\n",
                      __FUNCTION__, (unsigned long long)entry_id);
        return FBE_STATUS_GENERIC_FAILURE;
	}

    /* Do we have more transaction elements available in the transaction buffer? */
    if (persist_tran_header->tran_elem_count == FBE_PERSIST_TRAN_ENTRY_MAX) {
        fbe_spinlock_unlock(&persist_tran_lock);
        fbe_persist_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: transaction element overflow!\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	/* Find the buffer that contains the current transaction element. */
    tran_elem = persist_tran_elem[persist_tran_header->tran_elem_count];
    status = persist_tran_buffer_find_buffer(tran_elem, &buffer_index, &byte_offset);
    if (status != FBE_STATUS_OK) {
        fbe_spinlock_unlock(&persist_tran_lock);
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: transaction element corruption!\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	persist_tran_header->tran_elem_count++;/*for next time*/

    /* Populate the transaction element. */
    tran_elem->tran_elem_header.header.tran_elem_op = FBE_PERSIST_TRAN_OP_DELETE_ENTRY;
    tran_elem->tran_elem_header.header.entry_id = entry_id;
    tran_elem->tran_elem_header.header.data_length = FBE_PERSIST_DATA_BYTES_PER_ENTRY;/*always the maximum*/
	tran_elem->tran_elem_header.header.valid = FBE_TRUE;
    status = persist_tran_zero_entry(buffer_index, byte_offset);
    if (status != FBE_STATUS_OK) {
        fbe_spinlock_unlock(&persist_tran_lock);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_spinlock_unlock(&persist_tran_lock);
    return FBE_STATUS_OK;
}

/*check if an entry was deleted*/
fbe_status_t fbe_persist_transaction_is_entry_deleted(fbe_persist_entry_id_t entry_id, fbe_bool_t *deleted)
{
	fbe_u32_t	element = 0;
	fbe_u32_t	max_entry = persist_tran_header->tran_elem_count;

	for (element = 0; element < max_entry; element++) {
		if ((persist_tran_elem[element]->tran_elem_header.header.tran_elem_op == FBE_PERSIST_TRAN_OP_DELETE_ENTRY) &&
			(persist_tran_elem[element]->tran_elem_header.header.entry_id == entry_id)){

			*deleted = FBE_TRUE;
			return FBE_STATUS_OK;
		}
	}

	*deleted = FBE_FALSE;
	return FBE_STATUS_OK;
}

/*go over entries that were a FBE_PERSIST_TRAN_OP_DELETE_ENTRY and clear their bits to mark that we can re-use them*/
static void fbe_persist_clear_deleted_entries(void)
{
	fbe_u32_t		max_entry = persist_tran_header->tran_elem_count;
	fbe_u32_t		entry;
	fbe_status_t	status;
	

	for (entry = 0; entry < max_entry; entry ++) {
		/*clear only the ones that were new writes, the rest should stay there to indicate */
		if (FBE_PERSIST_TRAN_OP_DELETE_ENTRY == persist_tran_elem[entry]->tran_elem_header.header.tran_elem_op) {
			status = fbe_persist_clear_entry_bit(persist_tran_elem[entry]->tran_elem_header.header.entry_id);
			if (status != FBE_STATUS_OK) {
				fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: failed to clear entry no:%d!\n", __FUNCTION__, entry);
			}
		}
	}

	return ;
}

/*write to disk a transaction that created or changed a record*/
static void persist_commit_write_transaction(fbe_packet_t * packet, fbe_persist_tran_elem_t *tran_elem, fbe_u32_t index)
{
	fbe_packet_t * 					subpacket;
    fbe_payload_ex_t * 			payload;
    fbe_payload_block_operation_t * block_operation;
    fbe_lba_t 						tran_write_lba;
    fbe_status_t					status;

    /* Create a subpacket for an I/O that writes the transaction to the journal. */
    subpacket = persist_get_subpacket();

    payload = fbe_transport_get_payload_ex(subpacket);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Can't get payload\n", __FUNCTION__);

		fbe_transport_reuse_packet(subpacket);
		fbe_persist_transaction_invalidate_transaction();
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return ;
    }

    /* Set up a block operation to write the entire transaction to the journal. */
    block_operation = fbe_payload_ex_allocate_block_operation(payload);
    if (block_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Can't get block_operation\n", __FUNCTION__);
        fbe_transport_reuse_packet(subpacket);
		fbe_persist_transaction_invalidate_transaction();
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return ;
    }

    status = fbe_persist_database_get_entry_lba(tran_elem->tran_elem_header.header.entry_id, &tran_write_lba);
	if (status != FBE_STATUS_OK) {
		fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Can't get lba\n", __FUNCTION__);
        fbe_transport_reuse_packet(subpacket);
		fbe_persist_transaction_invalidate_transaction();
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
	}

    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                      tran_write_lba,
                                      FBE_PERSIST_BLOCKS_PER_TRAN_ENTRY,
                                      FBE_PERSIST_BYTES_PER_BLOCK,
                                      0,     /* optimum block size (not used) */
                                      NULL); /* preread descriptor (not used) */

    fbe_payload_ex_increment_block_operation_level(payload);
    
    /* Put an sgl pointer in the packet payload. */
    fbe_payload_ex_set_sg_list(payload, &persist_tran_commit_sgl[index * 2], 2);/*we multiply by two because every other element is the terminating one*/

	/* Link the subpacket into the caller's request packet. */
    fbe_transport_add_subpacket(packet, subpacket);

    /* Send send down the journal write. */
    fbe_transport_set_completion_function(subpacket, persist_write_entry_completion, NULL);
    fbe_transport_set_address(subpacket,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              fbe_persist_service.database_lun_object_id);

    fbe_topology_send_io_packet(subpacket);
   
    return ;


}

/*completion of the entry is done*/
static fbe_status_t persist_write_entry_completion(fbe_packet_t * subpacket, fbe_packet_completion_context_t notused)
{
    fbe_packet_t * 					request_packet;
    fbe_status_t 					status;
    fbe_payload_ex_t *				payload;
    fbe_payload_block_operation_t * block_operation;
    fbe_payload_block_operation_status_t 	block_status;

    /* Get the commit request packet. */
    request_packet = (fbe_packet_t *)fbe_transport_get_master_packet(subpacket);
    fbe_transport_remove_subpacket(subpacket);

	payload = fbe_transport_get_payload_ex(subpacket);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Can't get payload\n", __FUNCTION__);
        fbe_transport_reuse_packet(subpacket);
		fbe_persist_transaction_invalidate_transaction();
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
		fbe_persist_transaction_invalidate_transaction();
        fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Verify that the write request subpacket completed ok. */
    status = fbe_transport_get_status_code(subpacket);
    if (status != FBE_STATUS_OK) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s write failed\n", __FUNCTION__);
		fbe_payload_ex_release_block_operation(payload, block_operation);
        fbe_transport_reuse_packet(subpacket);
		fbe_persist_transaction_invalidate_transaction();
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
		fbe_persist_transaction_invalidate_transaction();
        fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* We are done with the entry write operation. */
    fbe_payload_ex_release_block_operation(payload, block_operation);
    fbe_transport_reuse_packet(subpacket);

    /* Now we can continue the commit to the next entry (or get out if we are done) */
    persist_transaction_commit(request_packet);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*while setting the LUN (repond to FBE_PERSIST_CONTROL_CODE_SET_LUN) we also read all the sectors and update the bitmap*/
fbe_status_t fbe_persist_transaction_read_sectors_and_update_bitmap(fbe_packet_t *packet)
{
    fbe_persist_sector_type_t 		sector_type = FBE_PERSIST_SECTOR_TYPE_INVALID + 1;/*start with the first sector*/

    fbe_persist_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "%s entry\n", __FUNCTION__);

    /*check we can do that now*/
	fbe_spinlock_lock(&persist_tran_lock);
	if (persist_tran_header->tran_state != FBE_PERSIST_TRAN_STATE_INVALID
        && persist_tran_header->tran_state != FBE_PERSIST_TRAN_STATE_READ_HEADER)
    {
		fbe_spinlock_unlock(&persist_tran_lock);
		fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
						  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
						  "%s illegal state: %d\n",
						  __FUNCTION__, persist_tran_header->tran_state);

        fbe_persist_transaction_invalidate_transaction();
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	persist_tran_header->tran_state = FBE_PERSIST_TRAN_STATE_REBUILD_BITMAP;
	fbe_spinlock_unlock(&persist_tran_lock);

	return fbe_persist_database_update_bitmap(packet, sector_type, FBE_PERSIST_SECTOR_START_ENTRY_ID);
}

/*!*******************************************************************************************
 * @fn fbe_persist_transaction_read_and_replay_journal
 *********************************************************************************************
* @brief
*  After setting the LUN (repond to FBE_PERSIST_CONTROL_CODE_SET_LUN) we need to replay the journal
*  if the journal is valid
*
* @param packet - request packet
* @param notused - always NULL
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    11/19/2012: Zhipeng Hu - created
 *
 ********************************************************************************************/ 
static fbe_status_t fbe_persist_transaction_read_and_replay_journal(fbe_packet_t *packet, fbe_packet_completion_context_t notused)
{
    fbe_packet_t * 					subpacket;
    fbe_payload_ex_t * 			payload;
    fbe_payload_block_operation_t * block_operation;
    fbe_lba_t 						tran_journal_lba;
    fbe_status_t					status = fbe_transport_get_status_code(packet);

    fbe_persist_trace(FBE_TRACE_LEVEL_INFO,
        FBE_TRACE_MESSAGE_ID_INFO,
        "%s: Entry \n", __FUNCTION__);

    if (status != FBE_STATUS_OK) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s previous state failed, cannot proceed\n",
                        __FUNCTION__);
        fbe_persist_transaction_invalidate_transaction();
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; /*we are in compl context, so return MORE_PROCESSING*/
    }

    /*check we can do that now*/
    fbe_spinlock_lock(&persist_tran_lock);
    if (persist_tran_header->tran_state != FBE_PERSIST_TRAN_STATE_READ_HEADER) {
        fbe_spinlock_unlock(&persist_tran_lock);
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s illegal state: %d\n",
            __FUNCTION__, persist_tran_header->tran_state);

        fbe_persist_transaction_invalidate_transaction();

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;  /*we are in compl context, so return MORE_PROCESSING*/
    }

    if(FBE_PERSIST_DB_JOURNAL_VALID_STATE != persist_db_header.journal_state)
    {
        /*invalid journal, so just transit to next state: rebuild bitmap*/
        fbe_spinlock_unlock(&persist_tran_lock);
        
        fbe_persist_trace(FBE_TRACE_LEVEL_INFO,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s journal invalid, so go to start build bitmap\n",
            __FUNCTION__);
                
        fbe_persist_transaction_read_sectors_and_update_bitmap(packet);

        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    else
    {
        /*valid journal, so continue current state: replay journal*/
        persist_tran_header->tran_state = FBE_PERSIST_TRAN_STATE_REPLAY_JOURNAL;
        fbe_spinlock_unlock(&persist_tran_lock);

        fbe_persist_trace(FBE_TRACE_LEVEL_INFO,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s journal valid, so first let us replay journal\n",
            __FUNCTION__);
    }

    /* Create a subpacket for an I/O that reads the transaction to the journal. */
    subpacket = persist_get_subpacket();
    payload = fbe_transport_get_payload_ex(subpacket);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s Can't get payload\n", __FUNCTION__);

        fbe_persist_transaction_invalidate_transaction();
        fbe_transport_reuse_packet(subpacket);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Set up a block operation to write the entire transaction to the journal. */
    block_operation = fbe_payload_ex_allocate_block_operation(payload);
    if (block_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s Can't get block_operation\n", __FUNCTION__);

        fbe_persist_transaction_invalidate_transaction();
        fbe_transport_reuse_packet(subpacket);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    tran_journal_lba = fbe_persist_database_get_tran_journal_lba();
    fbe_payload_block_build_operation(block_operation,
        FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
        tran_journal_lba,
        FBE_PERSIST_BLOCKS_PER_TRAN,
        FBE_PERSIST_BYTES_PER_BLOCK,
        0,     /* optimum block size (not used) */
        NULL); /* preread descriptor (not used) */

    fbe_payload_ex_increment_block_operation_level(payload);

    /* Put an sgl pointer in the packet payload. */
    fbe_payload_ex_set_sg_list(payload, persist_tran_journal_sgl, FBE_PERSIST_SGL_ELEM_PER_TRAN_JOURNAL);
    /* Link the subpacket into the caller's request packet. */
    fbe_transport_add_subpacket(packet, subpacket);

    /* After replaying journal, rebuild the entry bitmap*/
    fbe_transport_set_completion_function(packet, fbe_persist_rebuild_entry_bitmap, NULL);

    /* Send down the journal read. */
    fbe_transport_set_completion_function(subpacket, fbe_persist_transaction_read_and_replay_journal_completion, NULL);
    fbe_transport_set_address(subpacket,
        FBE_PACKAGE_ID_SEP_0,
        FBE_SERVICE_ID_TOPOLOGY,
        FBE_CLASS_ID_INVALID,
        fbe_persist_service.database_lun_object_id);

    fbe_persist_trace(FBE_TRACE_LEVEL_INFO,
        FBE_TRACE_MESSAGE_ID_INFO,
        "%s: Sending Read IO to Lun.\n", __FUNCTION__);

    fbe_topology_send_io_packet(subpacket);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;/*we are in completion context here so don't return the PENDING otherwise we will complete too soon*/
}

/*!*******************************************************************************************
 * @fn fbe_persist_transaction_read_and_replay_journal_completion
 *********************************************************************************************
* @brief
*  Completion routine of journal region data read, where the journal would be replayed
*
* @param subpacket - IO packet which is completed
* @param notused - always NULL
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    11/19/2012: Zhipeng Hu - created
 *
 ********************************************************************************************/ 
static fbe_status_t fbe_persist_transaction_read_and_replay_journal_completion(fbe_packet_t * subpacket, fbe_packet_completion_context_t notused)
{
	fbe_packet_t * 					request_packet;
    fbe_status_t 					status;
    fbe_payload_ex_t *				payload;
    fbe_payload_block_operation_t * block_operation;
    fbe_payload_block_operation_status_t 	block_status;

    /* Get the original request packet (the one the user sent to set the LUN). */
    request_packet = (fbe_packet_t *)fbe_transport_get_master_packet(subpacket);
    fbe_transport_remove_subpacket(subpacket);

	payload = fbe_transport_get_payload_ex(subpacket);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Can't get payload\n", __FUNCTION__);
		fbe_persist_transaction_invalidate_transaction();
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
		fbe_persist_transaction_invalidate_transaction();
        fbe_transport_reuse_packet(subpacket);
        fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Verify that the read request subpacket completed ok. */
    status = fbe_transport_get_status_code(subpacket);
    if (status != FBE_STATUS_OK) {
        fbe_persist_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s journal read failed\n", __FUNCTION__);
		fbe_persist_transaction_invalidate_transaction();
		 fbe_payload_ex_release_block_operation(payload, block_operation);
        fbe_transport_reuse_packet(subpacket);
        fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    
    fbe_payload_block_get_status(block_operation, &block_status);
    if (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Block_operation error, status: 0x%X\n", __FUNCTION__, block_status);
		fbe_persist_transaction_invalidate_transaction();
		 fbe_payload_ex_release_block_operation(payload, block_operation);
        fbe_transport_reuse_packet(subpacket);
        fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    fbe_persist_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                      "%s read journal data successfully\n", __FUNCTION__);

    /* We are done with the entry read operation. */
    fbe_payload_ex_release_block_operation(payload, block_operation);
    fbe_transport_reuse_packet(subpacket);

    /* Now we can check if there are entries in the journal and if so, we need to re-play the journal by repeating
	the process of the commit */
    persist_transaction_replay_journal(request_packet);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    
}

/*!*******************************************************************************************
 * @fn persist_transaction_replay_journal
 *********************************************************************************************
* @brief
*  Trigger replay of journal in this function
*
* @param packet - request packet
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    11/19/2012: Zhipeng Hu - created
 *
 ********************************************************************************************/
static fbe_status_t persist_transaction_replay_journal(fbe_packet_t *packet)
{
    fbe_payload_ex_t *payload;
    fbe_payload_control_operation_t *control_operation;
    fbe_persist_control_set_lun_t *set_lun;
    fbe_u32_t buffer_length;

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s fbe_transport_get_payload_ex() failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s fbe_payload_ex_get_control_operation() failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    set_lun = NULL;
    fbe_payload_control_get_buffer(control_operation, &set_lun);
    if (set_lun == NULL){
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    buffer_length = 0;
    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if(buffer_length != sizeof(fbe_persist_control_set_lun_t)){
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Invalid control buffer length %d, expected %llu\n",
                          __FUNCTION__, buffer_length, (unsigned long long)sizeof(fbe_persist_control_set_lun_t));
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	fbe_persist_trace(FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "%s: Entry.\n", __FUNCTION__);

	persist_tran_header->tran_elem_count = persist_db_header.journal_size;

	fbe_persist_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: start to replay journal\n", __FUNCTION__);

	/*change our state and start a commit */
	fbe_spinlock_lock(&persist_tran_lock);
	persist_tran_header->tran_elem_committed_count = 0;
    persist_tran_header->tran_state = FBE_PERSIST_TRAN_STATE_JOURNAL;
	fbe_spinlock_unlock(&persist_tran_lock);

	persist_transaction_commit(packet);

    set_lun->journal_replayed = FBE_TRI_STATE_TRUE;

	return FBE_STATUS_MORE_PROCESSING_REQUIRED;

}

static void fbe_persist_transaction_invalidate_transaction_lock(void)
{
	persist_tran_header->tran_handle = FBE_PERSIST_TRAN_HANDLE_INVALID;
	persist_tran_header->tran_state = FBE_PERSIST_TRAN_STATE_INVALID;
	persist_tran_header->tran_elem_count = 0;
	persist_tran_header->tran_elem_committed_count = 0;
}

static void fbe_persist_transaction_invalidate_transaction(void)
{
	fbe_spinlock_lock(&persist_tran_lock);
    fbe_persist_transaction_invalidate_transaction_lock();
	fbe_spinlock_unlock(&persist_tran_lock);
}

/*while unsetting the LUN (repond to FBE_PERSIST_CONTROL_CODE_UNSET_LUN) we also reset the transaction*/
fbe_status_t fbe_persist_transaction_reset(void)
{
    fbe_persist_transaction_invalidate_transaction();
    return FBE_STATUS_OK;
}

/*!*******************************************************************************************
 * @fn fbe_persist_read_persist_db_header_and_replay_journal
 *********************************************************************************************
* @brief
*  This function starts reading the persist db header from the header region of DB LUN
*
* @param packet - request packet
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    11/20/2012: Zhipeng Hu - created
 *
 ********************************************************************************************/
fbe_status_t fbe_persist_read_persist_db_header_and_replay_journal(fbe_packet_t *packet)
{
    if (fbe_persist_service.database_lun_object_id == FBE_OBJECT_ID_INVALID) {
        fbe_persist_trace(FBE_TRACE_LEVEL_WARNING,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s LUN is not set, can't proceed\n",
            __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*check we can do that now*/
    fbe_spinlock_lock(&persist_tran_lock);
    if (persist_tran_header->tran_state != FBE_PERSIST_TRAN_STATE_INVALID) {
        fbe_spinlock_unlock(&persist_tran_lock);
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s illegal state: %d\n",
            __FUNCTION__, persist_tran_header->tran_state);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    persist_tran_header->tran_state = FBE_PERSIST_TRAN_STATE_READ_HEADER;
    fbe_spinlock_unlock(&persist_tran_lock);

    /*After reading the persist db header, we would replay the journal if the header says there is valid journal*/
    fbe_transport_set_completion_function(packet, fbe_persist_transaction_read_and_replay_journal, NULL);

    return fbe_persist_database_read_persist_db_header(packet);
}

/*!*******************************************************************************************
 * @fn fbe_persist_rebuild_entry_bitmap
 *********************************************************************************************
* @brief
*  This function is called after journal replay, where all the entries in live data region would
*  be read and based on these data rebuild the entry bitmap.
*
* @param packet - request packet
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    11/20/2012: Zhipeng Hu - created
 *
 ********************************************************************************************/
static fbe_status_t fbe_persist_rebuild_entry_bitmap(fbe_packet_t *packet, fbe_packet_completion_context_t notused)
{
    fbe_status_t    status = fbe_transport_get_status_code(packet);

    fbe_persist_trace(FBE_TRACE_LEVEL_INFO,
        FBE_TRACE_MESSAGE_ID_INFO,
        "%s: Entry \n", __FUNCTION__);

    if (status != FBE_STATUS_OK) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s previous state failed. cannot proceed\n",
            __FUNCTION__);
        
        fbe_persist_transaction_invalidate_transaction();
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /*check we can do that now*/
	fbe_spinlock_lock(&persist_tran_lock);
	if (persist_tran_header->tran_state != FBE_PERSIST_TRAN_STATE_INVALID
        && persist_tran_header->tran_state != FBE_PERSIST_TRAN_STATE_READ_HEADER)
    {
		fbe_spinlock_unlock(&persist_tran_lock);
		fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
						  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
						  "%s illegal state: %d\n",
						  __FUNCTION__, persist_tran_header->tran_state);

        fbe_persist_transaction_invalidate_transaction();
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_MORE_PROCESSING_REQUIRED;
	}

	fbe_spinlock_unlock(&persist_tran_lock);

    fbe_persist_transaction_read_sectors_and_update_bitmap(packet);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;/*we are in completion context here so don't return the PENDING otherwise we will complete too soon*/
}

/*!*******************************************************************************************
 * @fn persist_tran_journal_invalidate_completion
 *********************************************************************************************
* @brief
*  Completion routine of invalidating journal, which is called after writing transaction data
*  is persisted onto the live data region on DB LUN.
*
* @param packet - request packet
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    11/20/2012: Zhipeng Hu - created
 *
 ********************************************************************************************/
static fbe_status_t
persist_tran_journal_invalidate_completion(fbe_packet_t * request_packet, fbe_packet_completion_context_t notused)
{
    fbe_status_t    status = fbe_transport_get_status_code(request_packet);

    fbe_persist_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "%s entry\n", __FUNCTION__);

    if(status != FBE_STATUS_OK)
    {
        fbe_persist_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s invalidate journal header failed, cannot proceed\n", __FUNCTION__);
        fbe_persist_transaction_invalidate_transaction();
        fbe_transport_set_status(request_packet, status, 0);
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    fbe_persist_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                      "%s invalidate journal header successfully\n", __FUNCTION__);

    /*now that we wrote all elements, we can mark all the elements that are of type FBE_PERSIST_TRAN_OP_DELETE_ENTRY,
    as free entries. We waited until now in case the user aborted the transaction */
    fbe_persist_clear_deleted_entries();

    /* We are done with the transaction. */
    fbe_persist_transaction_invalidate_transaction();
    persist_db_header.journal_state = 0;
    persist_db_header.journal_size = 0;

    /* We are done with the commit request. */
    fbe_transport_set_status(request_packet, status, 0);
    fbe_transport_complete_packet(request_packet);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*!*******************************************************************************************
 * @fn persist_tran_write_journal_header_completion
 *********************************************************************************************
* @brief
*  Completion routine of writing journal header, which is called after writing transaction data
*  onto journal region on DB LUN
*
* @param packet - request packet
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    11/20/2012: Zhipeng Hu - created
 *
 ********************************************************************************************/
static fbe_status_t persist_tran_write_journal_header_completion(fbe_packet_t * request_packet, fbe_packet_completion_context_t notused)
{    
    fbe_status_t    status = fbe_transport_get_status_code(request_packet);

    /*now that we have written the journal header, we can start writing entries to live data region now */
    if (status != FBE_STATUS_OK) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s write journal header failed. cannot proceed\n",
            __FUNCTION__);
        
        fbe_persist_transaction_invalidate_transaction();
        fbe_transport_set_status(request_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(request_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    fbe_spinlock_lock(&persist_tran_lock);
    persist_tran_header->tran_state = FBE_PERSIST_TRAN_STATE_COMMIT;
    fbe_spinlock_unlock(&persist_tran_lock);

    fbe_persist_trace(FBE_TRACE_LEVEL_INFO,
        FBE_TRACE_MESSAGE_ID_INFO,
        "%s write journal header successfully.\n",
        __FUNCTION__);

    /*check hooks*/
    persist_check_and_run_hook(request_packet, FBE_PERSIST_HOOK_TYPE_SHRINK_LIVE_TRANSACTION_AND_WAIT,
                               persist_tran_header, NULL, NULL);
    persist_check_and_run_hook(request_packet, FBE_PERSIST_HOOK_TYPE_PANIC_WHEN_WRITING_LIVE_DATA_REGION,
                               persist_tran_header, NULL, NULL);

    /* We are done with the journal header write*/
    persist_transaction_commit(request_packet);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*!*******************************************************************************************
 * @fn persist_set_database_lun_completion
 *********************************************************************************************
* @brief
*  Completion routine of setting db lun, where the persist transaction would be invalidated.
*
* @param packet - request packet
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    11/20/2012: Zhipeng Hu - created
 *
 ********************************************************************************************/
fbe_status_t persist_set_database_lun_completion(fbe_packet_t * request_packet, fbe_packet_completion_context_t notused)
{  
    fbe_persist_trace(FBE_TRACE_LEVEL_INFO,
        FBE_TRACE_MESSAGE_ID_INFO,
        "%s set db lun complete. Invalidate the transaction.\n",
        __FUNCTION__);

    fbe_persist_transaction_invalidate_transaction();

    return FBE_STATUS_OK;
}

/* This external function is called when a FBE_PERSIST_CONTROL_CODE_VALIDATE_ENTRY request is received.
 * It copies the request into an existing transaction element in the transaction buffer.
 */
fbe_status_t
fbe_persist_transaction_validate_entry(fbe_persist_transaction_handle_t callers_tran_handle,
                                    fbe_persist_entry_id_t entry_id)
{
    fbe_status_t 				status;
	fbe_bool_t 					entry_exists = FBE_FALSE;
    
    fbe_spinlock_lock(&persist_tran_lock);
    /* Is there a current transaction? */
    if (persist_tran_header->tran_handle == FBE_PERSIST_TRAN_HANDLE_INVALID) {
        fbe_spinlock_unlock(&persist_tran_lock);
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: there is no current transaction!\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Is callers transaction handle for the current transaction? */
    if (callers_tran_handle != persist_tran_header->tran_handle) {
        fbe_spinlock_unlock(&persist_tran_lock);
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: transaction requested is not for the current transaction, got 0x%llX, expected 0x%llX!\n",
                      __FUNCTION__, (unsigned long long)callers_tran_handle,
		      (unsigned long long)persist_tran_header->tran_handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Is the current transaction pending, waiting for more transaction elements? */
    if (persist_tran_header->tran_state != FBE_PERSIST_TRAN_STATE_PENDING) {
        fbe_spinlock_unlock(&persist_tran_lock);
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: current transaction state is invalid, found 0x%X, expected 0x%X!\n",
                      __FUNCTION__, persist_tran_header->tran_state, FBE_PERSIST_TRAN_STATE_PENDING);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	/*do we even have an existing transaction on the disk that we can modify?*/
	status = fbe_persist_entry_exists(entry_id, &entry_exists);
	if ((!entry_exists) || (status != FBE_STATUS_OK)) {
		fbe_spinlock_unlock(&persist_tran_lock);
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: non existing entry:0x%llX!\n",
                      __FUNCTION__, (unsigned long long)entry_id);
        return FBE_STATUS_GENERIC_FAILURE;
	}

    fbe_spinlock_unlock(&persist_tran_lock);
    return FBE_STATUS_OK;
}

