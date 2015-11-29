/***************************************************************************
 *  terminator_simulated_disk_main.c
 ***************************************************************************
 *
 *  Description
 *      APIs to emulate the disk drives
 *
 *
 *  History:
 *      08/12/09    guov    Created (Move Peter's memory_drive)
 *
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "terminator_simulated_disk_private.h"
#include "fbe/fbe_physical_drive.h"
#include "terminator_drive.h"
#include "fbe_terminator_common.h"
#include "fbe/fbe_xor_api.h"
#include "terminator_simulated_disk.h"

/* This functions implemented in fbe_zlib library 
    There is no public header file for this functionality
*/
fbe_status_t fbe_zlib_get_compressed_size(fbe_u32_t source_size, fbe_u32_t * destination_size);
fbe_status_t fbe_zlib_compress(fbe_u8_t * dest, fbe_u32_t * dest_size, fbe_u8_t * source, fbe_u32_t source_size);
fbe_status_t fbe_zlib_uncompress(fbe_u8_t * dest, fbe_u32_t * dest_size, fbe_u8_t * source, fbe_u32_t source_size);


/**********************************/
/*        local variables         */
/**********************************/

/******************************/
/*     local function         */
/*****************************/
static fbe_bool_t journal_record_is_intersected(journal_record_t * record, fbe_lba_t lba, fbe_block_count_t block_count);
static fbe_status_t terminator_simulated_disk_memory_fill_buffer_with_write_same(journal_record_t * write_same_record, 
                                                                                 fbe_lba_t lba,
                                                                                 fbe_block_count_t block_count,
                                                                                 fbe_block_size_t block_size,
                                                                                 fbe_u8_t * buffer_to_fill);
static fbe_status_t process_intersected_zero_record (terminator_drive_t *drive,
                                                     journal_record_t * record, 
                                                     journal_record_t * new_record, 
                                                     fbe_block_size_t block_size);
fbe_status_t terminator_drive_decompress_record_to_memory(fbe_u8_t * data_buffer_pointer, 
                                                         fbe_lba_t lba, 
                                                         fbe_block_count_t block_count, 
                                                         fbe_block_size_t block_size,
                                                         fbe_u8_t * record_data_pointer, 
                                                         fbe_block_size_t record_block_size);

fbe_atomic_t terminator_journal_memory_bytes = 0;
fbe_atomic_t terminator_journal_compressed_record_memory_bytes = 0;
fbe_atomic_t terminator_journal_zero_record_memory_bytes = 0;
static fbe_bool_t terminator_journal_verify_compressed_record = FBE_TRUE;

static journal_record_t *terminator_simulated_disk_memory_allocate_record(terminator_drive_t *drive,
                                                                          terminator_journal_record_type_t record_type,
                                                                          fbe_block_count_t block_count,
                                                                          fbe_block_size_t block_size)
{
    journal_record_t *new_record = NULL;
    fbe_u32_t key_size = FBE_ENCRYPTION_KEY_SIZE;
    
    /* Based on record type determine if we should track blocks allocated
     */
    switch (record_type)
    {
        case TERMINATOR_JOURNAL_RECORD_ZERO:
            /* Simply allocate the record
             */
            new_record = fbe_allocate_contiguous_memory( (fbe_u32_t)(sizeof(journal_record_t)+key_size));
            if (new_record == NULL)
            {
                terminator_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s Attempting to allocate journal(disk) of: 0x%llx bytes failed. Reduce drive size!!! \n", 
                             __FUNCTION__, (unsigned long long)sizeof(*new_record));
                return NULL;
            }
            fbe_atomic_add(&terminator_journal_memory_bytes, sizeof(journal_record_t)+key_size);
            fbe_atomic_add(&terminator_journal_zero_record_memory_bytes, sizeof(journal_record_t)+key_size);

#ifdef __SAFE__
            csx_p_memzero(new_record, sizeof(journal_record_t));
#endif
            drive->journal_num_records++;
            break;

        case TERMINATOR_JOURNAL_RECORD_WRITE:
            /* Allocate the request blocks
            */
            new_record = fbe_allocate_contiguous_memory( (fbe_u32_t)((block_count * block_size) + sizeof(journal_record_t)));
            if (new_record == NULL)
            {
                terminator_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s Attempting to allocate journal(disk) of: 0x%llx blocks failed. Reduce drive size!!! \n", 
                                 __FUNCTION__, (unsigned long long)block_count);
                return NULL;
            }
            fbe_atomic_add(&terminator_journal_memory_bytes, sizeof(journal_record_t) + (block_count * block_size));
            if (block_size < drive->block_size) {
                fbe_atomic_add(&terminator_journal_compressed_record_memory_bytes, sizeof(journal_record_t) + (block_count * block_size)); 
            }           
			
#ifndef ALAMOSA_WINDOWS_ENV
            /* I get the sense bugs are exposed if we let this memory have arbitrary contents, so
             *  set to unique pattern
             */
            {
                 void **pp = (void **)((char *)new_record + sizeof(journal_record_t));
                 int np = (int)((block_count * block_size) / sizeof(void *));
                 int idx;
                 for (idx = 0; idx < np; idx++)
                 {
                     pp[idx] = new_record;
                 }
            }

            csx_p_memzero(new_record, (block_count * block_size) + sizeof(journal_record_t));
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE */

            drive->journal_num_records++;
            drive->journal_blocks_allocated += block_count;
            break;

        default:
            terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s Unsupported record type: %d \n",
                             __FUNCTION__, record_type);
            return NULL;
    }

    return new_record;
}

void terminator_simulated_disk_memory_free_record(terminator_drive_t *drive, 
                                                  journal_record_t **record_pp)
{
    journal_record_t *record = *record_pp;
    fbe_u32_t key_size = FBE_ENCRYPTION_KEY_SIZE;

    /* Based on record type determine if we should track blocks allocated
     */
    switch (record->record_type)
    {
        case TERMINATOR_JOURNAL_RECORD_ZERO:
            /* Simply free the record
             */
            drive->journal_num_records--;
            fbe_atomic_add(&terminator_journal_memory_bytes, -(LONGLONG)(sizeof(journal_record_t)+key_size));
            fbe_atomic_add(&terminator_journal_zero_record_memory_bytes, -(LONGLONG)(sizeof(journal_record_t)+key_size));
            fbe_release_contiguous_memory(record);
            break;

        case TERMINATOR_JOURNAL_RECORD_WRITE:
            /* Free the memory allocated
             */
            drive->journal_blocks_allocated -= record->block_count;
            drive->journal_num_records--;
            fbe_atomic_add(&terminator_journal_memory_bytes, -(LONGLONG)(sizeof(journal_record_t) + record->data_size));
            if (record->is_compressed) {
                fbe_atomic_add(&terminator_journal_compressed_record_memory_bytes, -(LONGLONG)(sizeof(journal_record_t) + record->data_size)); 
            }
            fbe_release_contiguous_memory(record);
            break;

        default:
            terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s Unsupported record type: %d \n",
                             __FUNCTION__, record->record_type);
            return;
    }

    /* Now remove the reference to this memory
     */
    *record_pp = NULL;
    return;
}
fbe_bool_t fbe_terminator_disk_keys_equal(fbe_u8_t *key1, fbe_u8_t *key2)
{
    fbe_u32_t index;

    if ((key1 == NULL) && (key2 == NULL)) {
        return FBE_TRUE;
    }
    if ((key1 == NULL && key2 != NULL) ||
        (key1 != NULL && key2 == NULL)) {
        return FBE_FALSE;
    }
    for (index = 0; index < FBE_ENCRYPTION_KEY_SIZE; index++) {
        if (key1[index] != key2[index]) {
            return FBE_FALSE;
        }
    }
    return FBE_TRUE;
}

fbe_bool_t fbe_terminator_unmap_state_equal(terminator_journal_record_flags_t current_record_flags, terminator_journal_record_flags_t new_record_flags)
{
    if ((current_record_flags & TERMINATOR_JOURNAL_RECORD_FLAG_UNMAPPED) ==
        (new_record_flags & TERMINATOR_JOURNAL_RECORD_FLAG_UNMAPPED))
    {
        return FBE_TRUE;
    }
    return FBE_FALSE;
}


fbe_status_t terminator_simulated_disk_memory_write_zero_pattern(terminator_drive_t * drive,
                                                                 fbe_lba_t lba,
                                                                 fbe_block_count_t block_count,
                                                                 fbe_block_size_t block_size,
                                                                 fbe_u8_t * data_buffer,
																void * context)
{
    journal_record_t * record = NULL;
    journal_record_t * next_record = NULL;

    journal_record_t * new_record = NULL;
    fbe_queue_head_t tmp_queue_head;
    fbe_queue_element_t * queue_element = NULL;
    fbe_bool_t is_intersected;
    fbe_lba_t start_lba;
    fbe_lba_t last_lba;
    fbe_terminator_io_t * terminator_io = (fbe_terminator_io_t *)context;
    fbe_u8_t *key_p = NULL;
    terminator_journal_record_flags_t new_record_flags = 0;

    /* Handle unmap cmd*/
    if (terminator_io->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO) {
        if (terminator_io->u.zero.do_unmap){
            new_record_flags |= TERMINATOR_JOURNAL_RECORD_FLAG_UNMAPPED;
        }
    }

    if (terminator_io->b_key_valid) {
        key_p = &terminator_io->keys[0];
    }
    fbe_spinlock_lock(&drive->journal_lock);
    queue_element = fbe_queue_front(&drive->journal_queue_head);

    while(queue_element != NULL){
        record = (journal_record_t *)queue_element;

        /* All records are sorted */
        if((record->lba + record->block_count) < lba){ /* We write below this record */
            /* record  |----|
             * lba            |----| */
            queue_element = fbe_queue_next(&drive->journal_queue_head, queue_element);
            continue;
        }

        if((record->lba + record->block_count) == lba){ /* We are exactly on the border */
            /* record  |----|
             * lba          |----| */
			/* Check if record iz zero record as well */
			if((record->record_type == TERMINATOR_JOURNAL_RECORD_ZERO) &&
               fbe_terminator_disk_keys_equal(record->data_ptr, key_p) &&
               fbe_terminator_unmap_state_equal(record->flags, new_record_flags)){
				/* Check if we are not overlapping with next record */
				next_record = (journal_record_t *) fbe_queue_next(&drive->journal_queue_head, queue_element);
				if((next_record == NULL) || (next_record->lba > (lba + block_count))){
					/* We can just increase the size */
					record->block_count += block_count;
		            fbe_spinlock_unlock(&drive->journal_lock);
				    return FBE_STATUS_OK;
				}                

                /* record  |----|
                 * lba          |----|
                 * next_record     |--------------| */
                if ((next_record->record_type == TERMINATOR_JOURNAL_RECORD_ZERO) &&
                    ((lba + block_count) <= (next_record->lba + next_record->block_count))){
                    if (fbe_terminator_disk_keys_equal(next_record->data_ptr, key_p) &&
                        fbe_terminator_unmap_state_equal(next_record->flags, new_record_flags)) {
                        /* All keys are the same, combine 2 records */
                        record->block_count = (next_record->lba + next_record->block_count - record->lba);
                        /* Take next record out of the queue */
                        fbe_queue_remove(&next_record->queue_element);
                        terminator_simulated_disk_memory_free_record(drive, &next_record);
                        fbe_spinlock_unlock(&drive->journal_lock);
                        return FBE_STATUS_OK;
                    } else {
                        /* Increase the size of current record */
                        record->block_count += block_count;
                        /* Decrease the size of next record */
                        next_record->block_count -= (lba + block_count - next_record->lba);
                        next_record->lba = (lba + block_count);
                        if (next_record->block_count == 0) {
                            fbe_queue_remove(&next_record->queue_element);
                            terminator_simulated_disk_memory_free_record(drive, &next_record);
                        }
                        fbe_spinlock_unlock(&drive->journal_lock);
                        return FBE_STATUS_OK;
                    }
                }
			}
            queue_element = fbe_queue_next(&drive->journal_queue_head, queue_element);
            continue;
        }

        if(record->lba > (lba + block_count)){ /* We write above this record */
            /* record        |----|
             * lba    |----|         */
            /* for write same the data buffer size is always same as block_size */
            new_record = terminator_simulated_disk_memory_allocate_record(drive, TERMINATOR_JOURNAL_RECORD_ZERO, 0, 0);
            new_record->record_type = TERMINATOR_JOURNAL_RECORD_ZERO;
            new_record->lba = lba;
            new_record->block_count = block_count;
            if (terminator_io->b_key_valid) {
                new_record->data_ptr = (fbe_u8_t *)new_record + sizeof(journal_record_t);
                fbe_copy_memory(new_record->data_ptr, &terminator_io->keys[0], sizeof(fbe_u8_t) * FBE_ENCRYPTION_KEY_SIZE);
            } else {
                new_record->data_ptr = NULL;
            }
            new_record->data_size = 0;
            new_record->block_size = 0;
            new_record->is_compressed = FBE_FALSE;
            new_record->flags = new_record_flags;

            new_record->queue_element.next = record;
            new_record->queue_element.prev = record->queue_element.prev;
            ((fbe_queue_element_t *)(record->queue_element.prev))->next = new_record;
            record->queue_element.prev = new_record;
            fbe_spinlock_unlock(&drive->journal_lock);

            return FBE_STATUS_OK;
        }

        /* If we are here we have "intersections */
        /*
        One Place to rule them all, One Loop to find them,
        One Method to bring them all and in the darkness bind them
        */
        start_lba = lba;
        last_lba = lba + block_count;
        /* Initialize temporary queue */
        fbe_queue_init(&tmp_queue_head);
        /* Relocate all records with intersection to tmp_queue */
        is_intersected = journal_record_is_intersected(record, lba, block_count);
        while(is_intersected == FBE_TRUE) { /* One Loop to find them */
            if(start_lba > record->lba){
                /* record->lba |----- ...
                 * start_lba      |-- ... */
                if((record->record_type == TERMINATOR_JOURNAL_RECORD_ZERO) &&
                   fbe_terminator_disk_keys_equal(record->data_ptr, key_p) &&
                   fbe_terminator_unmap_state_equal(record->flags, new_record_flags)){
                    /* if the old record is zero pattern, merged with the new record */
                    /* record->lba |----- ...
                     * start_lba   |----- ... */
                    start_lba = record->lba;
                }
            }
            if(last_lba < (record->lba + record->block_count)){
                /* record       ...----|
                 * last_lba     ...--| */
                if((record->record_type == TERMINATOR_JOURNAL_RECORD_ZERO) &&
                   fbe_terminator_disk_keys_equal(record->data_ptr, key_p) &&
                   fbe_terminator_unmap_state_equal(record->flags, new_record_flags)){
                    /* if the old record is zero pattern, merged with the new record */
                    /* record       ...----|
                     * last_lba     ...----| */
                    last_lba = record->lba + record->block_count;
                }
            }
            /* Advance queue element to the next record */
            queue_element = fbe_queue_next(&drive->journal_queue_head, queue_element);

            /* Take record out of the queue */
            fbe_queue_remove(&record->queue_element);

            /* If we are in the new record range we can drop the record */
            if((record->lba >= lba) && ((record->lba + record->block_count) <= (lba + block_count))){
                /* Release record */
                terminator_simulated_disk_memory_free_record(drive, &record);
            } else { /* we are on the edge and not merged into the new record*/
                /* Insert record to temporary queue */
                fbe_queue_push(&tmp_queue_head, &record->queue_element);
            }
            is_intersected = FBE_FALSE;
            record = (journal_record_t *)queue_element;
            if(record != NULL){
                is_intersected = journal_record_is_intersected(record, lba, block_count);
            }
        }

        /* We found all records with intersection*/
        /* Allocate new write same */
        new_record = terminator_simulated_disk_memory_allocate_record(drive, TERMINATOR_JOURNAL_RECORD_ZERO, 0, 0);
        if (new_record == NULL)
        {
            fbe_spinlock_unlock(&drive->journal_lock);
            terminator_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s Attempting to allocate journal(disk) of: 0x%llx blocks failed. Reduce VD drive size!!! \n", 
                             __FUNCTION__,
			     (unsigned long long)(last_lba - start_lba));
            return FBE_STATUS_GENERIC_FAILURE;
        }

        new_record->record_type = TERMINATOR_JOURNAL_RECORD_ZERO;
        new_record->lba = start_lba;
        new_record->block_count = (fbe_block_count_t)(last_lba - start_lba);

        if (terminator_io->b_key_valid) {
            new_record->data_ptr = (fbe_u8_t *)new_record + sizeof(journal_record_t);
            fbe_copy_memory(new_record->data_ptr, &terminator_io->keys[0], sizeof(fbe_u8_t) * FBE_ENCRYPTION_KEY_SIZE);
        } else {
            new_record->data_ptr = NULL;
        }
        new_record->data_size = 0;
        new_record->block_size = 0;
        new_record->is_compressed = FBE_FALSE;
        new_record->flags = new_record_flags;

        /* Insert new record to the queue */
        if(record != NULL){
            new_record->queue_element.next = record;
            new_record->queue_element.prev = record->queue_element.prev;
            ((fbe_queue_element_t *)(record->queue_element.prev))->next = new_record;
            record->queue_element.prev = new_record;
        }else{
            fbe_queue_push(&drive->journal_queue_head, &new_record->queue_element);
        }

        /* create new records for the ones that are not merged to the write same */
        queue_element = fbe_queue_pop(&tmp_queue_head);
        while(queue_element != NULL){
            record = (journal_record_t *)queue_element;
            process_intersected_zero_record(drive, record, new_record, block_size);
            /* Release tmp_queue record */
            terminator_simulated_disk_memory_free_record(drive, &record);
            queue_element = fbe_queue_pop(&tmp_queue_head);
        }
        /* We are done with temporary queue */
        fbe_queue_destroy(&tmp_queue_head);
        fbe_spinlock_unlock(&drive->journal_lock);
        return FBE_STATUS_OK;
    }

    /* If we are here we did not create the record, let's do it now */
    new_record = terminator_simulated_disk_memory_allocate_record(drive, TERMINATOR_JOURNAL_RECORD_ZERO, 0, 0);
    new_record->record_type = TERMINATOR_JOURNAL_RECORD_ZERO;
    new_record->lba = lba;
    new_record->block_count = block_count;

    if (terminator_io->b_key_valid) {
        new_record->data_ptr = (fbe_u8_t *)new_record + sizeof(journal_record_t);
        fbe_copy_memory(new_record->data_ptr, &terminator_io->keys[0], sizeof(fbe_u8_t) * FBE_ENCRYPTION_KEY_SIZE);
    } else {
        new_record->data_ptr = NULL;
    }
    new_record->data_size = 0;
    new_record->block_size = 0;
    new_record->is_compressed = FBE_FALSE;
    new_record->flags = new_record_flags;

    fbe_queue_push(&drive->journal_queue_head, &new_record->queue_element);
    fbe_spinlock_unlock(&drive->journal_lock);

    if(new_record->block_count == 0){
        terminator_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "\n\n\n %s New record has block_count is 0!!! We are in throuble. \n\n\n",
                         __FUNCTION__);
    }

    return FBE_STATUS_OK;
}

static fbe_status_t
terminator_simulated_disk_verify_compressed_record(terminator_drive_t * drive, fbe_bool_t is_compressed, fbe_u8_t *data_ptr, fbe_u32_t data_size)
{
    fbe_terminator_compressed_block_t * compressed_block = (fbe_terminator_compressed_block_t *)data_ptr;
    fbe_terminator_compressed_block_data_pattern_t * pattern_p;
    fbe_u32_t total_count;
    fbe_u32_t j;
    fbe_u32_t block_size;

    if (!terminator_journal_verify_compressed_record) {
        return FBE_STATUS_OK;
    }

    if (!is_compressed) {
        return FBE_STATUS_OK;
    }

    block_size = drive->block_size;
    if (block_size > FBE_BE_BYTES_PER_BLOCK) {
        block_size = FBE_BE_BYTES_PER_BLOCK;
    }

    while ((fbe_u8_t *)compressed_block < (data_ptr + data_size)) {
        total_count = 0;
        pattern_p = &compressed_block->data_pattern[0];
        for (j = 0; j < TERMINATOR_MAX_PATTERNS; j++) {
            total_count += pattern_p->count;
            if (total_count == (block_size/sizeof(fbe_u64_t))) {
                break;
            }
            if (total_count > (block_size/sizeof(fbe_u64_t))) {
                terminator_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, 
                                 "Total count not correct %d\n", total_count);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            pattern_p++;
        }
        
        compressed_block++;
    }

    return FBE_STATUS_OK;
}

fbe_status_t terminator_simulated_disk_memory_write(terminator_drive_t * drive,
                                                    fbe_lba_t lba,
                                                    fbe_block_count_t block_count,
                                                    fbe_block_size_t block_size,
                                                    fbe_u8_t * data_buffer,
												   void * context)
{
    journal_record_t * record = NULL;
    journal_record_t * new_record = NULL;
    fbe_queue_head_t tmp_queue_head;
    fbe_queue_element_t * queue_element = NULL;
    fbe_bool_t is_intersected;
    fbe_lba_t start_lba;
    fbe_lba_t last_lba;
    fbe_u32_t data_offset;
    fbe_bool_t is_compressed = FBE_FALSE;

    if ((block_size % sizeof(fbe_terminator_compressed_block_t)) == 0) {
        is_compressed = FBE_TRUE;
    }
    /* Take the lock on the journal
     */
    fbe_spinlock_lock(&drive->journal_lock);

    /* Check if we can coalesce new data.
     */
    queue_element = fbe_queue_front(&drive->journal_queue_head);
    while(queue_element != NULL){
        record = (journal_record_t *)queue_element;

        /* All records are sorted */
        if((record->lba + record->block_count) <= lba){ /* We write below this record */
            /* record  |----|
             * lba            |----| */
            queue_element = fbe_queue_next(&drive->journal_queue_head, queue_element);
            continue;
        }

        if(record->lba > (lba + block_count)){ /* We write above this record */
            /* record        |----|
             * lba    |----|         */
            new_record = terminator_simulated_disk_memory_allocate_record(drive, TERMINATOR_JOURNAL_RECORD_WRITE,
                                                                          block_count,
                                                                          block_size),
            new_record->record_type = TERMINATOR_JOURNAL_RECORD_WRITE;
            new_record->lba = lba;
            new_record->block_count = block_count;
            new_record->data_ptr = (fbe_u8_t *)new_record + sizeof(journal_record_t);
            new_record->data_size = (fbe_u32_t)(block_count * block_size);
            new_record->block_size = block_size;
            new_record->is_compressed = is_compressed;
            new_record->flags = 0;
            fbe_copy_memory(new_record->data_ptr, data_buffer, new_record->data_size);
            terminator_simulated_disk_verify_compressed_record(drive, new_record->is_compressed, new_record->data_ptr, new_record->data_size);

            new_record->queue_element.next = record;
            new_record->queue_element.prev = record->queue_element.prev;
            ((fbe_queue_element_t *)(record->queue_element.prev))->next = new_record;
            record->queue_element.prev = new_record;
            fbe_spinlock_unlock(&drive->journal_lock);

            return FBE_STATUS_OK;
        }

		if((record->record_type != TERMINATOR_JOURNAL_RECORD_ZERO) && (record->block_size == block_size)){
			if((record->lba <= lba) && ((record->lba + record->block_count) >= (lba + block_count))){ /* Write inside this record */
				/* record |------------|
				 * lba        |----|         */
				    data_offset = (fbe_u32_t)((lba - record->lba)* block_size);
				    fbe_copy_memory(record->data_ptr + data_offset, data_buffer, (fbe_u32_t)(block_count * block_size));
                    terminator_simulated_disk_verify_compressed_record(drive, record->is_compressed, record->data_ptr, record->data_size);
				    fbe_spinlock_unlock(&drive->journal_lock);
				    return FBE_STATUS_OK;
			}
		}

        /* If we are here we have "intersections */
        /*
        One Place to rule them all, One Loop to find them,
        One Method to bring them all and in the darkness bind them
        */
        start_lba = lba;
        last_lba = lba + block_count;
        /* Initialize temporary queue */
        fbe_queue_init(&tmp_queue_head);
        /* Relocate all records with intersection to tmp_queue */
        is_intersected = journal_record_is_intersected(record, lba, block_count);
        while(is_intersected == FBE_TRUE) { /* One Loop to find them */
            if(start_lba > record->lba){
                /* record->lba |----- ...
                 * start_lba      |-- ... */
                if((record->record_type != TERMINATOR_JOURNAL_RECORD_ZERO) && (record->block_size == block_size)){
                    /* if the old record is not zero pattern, merged with the new record */
                    /* record->lba |----- ...
                     * start_lba   |----- ... */
                    start_lba = record->lba;
                }
            }
            if(last_lba < (record->lba + record->block_count)){
                /* record       ...----|
                 * last_lba     ...--| */
               if((record->record_type != TERMINATOR_JOURNAL_RECORD_ZERO) && (record->block_size == block_size)){
                    /* if the old record is not zero pattern, merged with the new record */
                    /* record       ...----|
                     * last_lba     ...----| */
                    last_lba = record->lba + record->block_count;
                }
            }
            /* Advance queue element to the next record */
            queue_element = fbe_queue_next(&drive->journal_queue_head, queue_element);

            /* Take record out of the queue */
            fbe_queue_remove(&record->queue_element);

            /* If we are not on the edge we can drop the record */
            if((record->lba >= lba) && ((record->lba + record->block_count) <= (lba + block_count))){
                /* Release record */
                terminator_simulated_disk_memory_free_record(drive, &record);
            } else { /* we are on the edge */
                /* Insert record to temporary queue */
                fbe_queue_push(&tmp_queue_head, &record->queue_element);
            }

            is_intersected = FBE_FALSE;
            record = (journal_record_t *)queue_element;
            if(record != NULL){
                is_intersected = journal_record_is_intersected(record, lba, block_count);
            }
        }

        /* We found  all records with intersection*/
        /* Allocate one big record to bind them all */
        new_record = terminator_simulated_disk_memory_allocate_record(drive, TERMINATOR_JOURNAL_RECORD_WRITE,
                                                                      (last_lba - start_lba),
                                                                      block_size);
        if (new_record == NULL)
        {
            fbe_spinlock_unlock(&drive->journal_lock);
            terminator_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s Attempting to allocate journal(disk) of: 0x%llx blocks failed. Reduce drive size!!! \n", 
                             __FUNCTION__,
			     (unsigned long long)(last_lba - start_lba));
            return FBE_STATUS_GENERIC_FAILURE;
        }

        new_record->record_type = TERMINATOR_JOURNAL_RECORD_WRITE;
        new_record->lba = start_lba;
        new_record->block_count = (fbe_block_count_t)(last_lba - start_lba);
        new_record->data_ptr = (fbe_u8_t *)new_record + sizeof(journal_record_t);
        new_record->data_size = (fbe_u32_t)(new_record->block_count * block_size);
        new_record->block_size = block_size;
        new_record->is_compressed = is_compressed;
        new_record->flags = 0;

        /* Insert new record to the queue */
        if(record != NULL){
            new_record->queue_element.next = record;
            new_record->queue_element.prev = record->queue_element.prev;
            ((fbe_queue_element_t *)(record->queue_element.prev))->next = new_record;
            record->queue_element.prev = new_record;
        }else{
            fbe_queue_push(&drive->journal_queue_head, &new_record->queue_element);
        }

        /* Copy data from tmp_queue records to our new record */
        queue_element = fbe_queue_pop(&tmp_queue_head);
        while(queue_element != NULL){
            record = (journal_record_t *)queue_element;
            if((record->record_type != TERMINATOR_JOURNAL_RECORD_ZERO) && (record->block_size == block_size)) {
                /* fill in the old data first */
                data_offset = (fbe_u32_t)((record->lba - new_record->lba)* block_size);
                fbe_copy_memory(new_record->data_ptr + data_offset, record->data_ptr, record->data_size);
            }
            else{
                /* Zero pattern record.  Keep the unwritten space as Zero pattern. */
                process_intersected_zero_record(drive, record, new_record, block_size);
            }
            /* Release tmp_queue record */
            terminator_simulated_disk_memory_free_record(drive, &record);
            /* get the next one */
            queue_element = fbe_queue_pop(&tmp_queue_head);
        }
        /* We are done with temporary queue */
        fbe_queue_destroy(&tmp_queue_head);

        /* Perform actual write operation */
        data_offset = (fbe_u32_t)((lba - start_lba)* block_size);
        /* Sanity check */
        if(data_offset + block_count * block_size > new_record->data_size){
            fbe_spinlock_unlock(&drive->journal_lock);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        fbe_copy_memory(new_record->data_ptr + data_offset, data_buffer, (fbe_u32_t)(block_count * block_size));
        terminator_simulated_disk_verify_compressed_record(drive, new_record->is_compressed, new_record->data_ptr, new_record->data_size);

        fbe_spinlock_unlock(&drive->journal_lock);

        return FBE_STATUS_OK;
    }

    /* If we are here we did not create the record, let's do it now */
    new_record = terminator_simulated_disk_memory_allocate_record(drive, TERMINATOR_JOURNAL_RECORD_WRITE,
                                                                  block_count,
                                                                  block_size);
    new_record->record_type = TERMINATOR_JOURNAL_RECORD_WRITE;
    new_record->lba = lba;
    new_record->block_count = block_count;
    new_record->data_ptr = (fbe_u8_t *)new_record + sizeof(journal_record_t);
    new_record->data_size = (fbe_u32_t)(block_count * block_size);
    new_record->block_size = block_size;
    new_record->is_compressed = is_compressed;
    new_record->flags = 0;
    fbe_copy_memory(new_record->data_ptr, data_buffer, new_record->data_size);
    terminator_simulated_disk_verify_compressed_record(drive, new_record->is_compressed, new_record->data_ptr, new_record->data_size);
    fbe_queue_push(&drive->journal_queue_head, &new_record->queue_element);
    fbe_spinlock_unlock(&drive->journal_lock);

    if(new_record->block_count == 0){
        terminator_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "\n\n\n %s New record has block_count is 0!!! We are in throuble. \n\n\n",
                         __FUNCTION__);
    }

    return FBE_STATUS_OK;
}

static fbe_status_t terminator_simulated_disk_memory_populate_uninitialized_data(terminator_drive_t * drive,
                                                                                 fbe_lba_t in_lba,
                                                                                 fbe_block_count_t block_count,
                                                                                 fbe_block_size_t block_size,
                                                                                 fbe_u8_t * data_buffer,
                                                                                 fbe_bool_t b_sends_zeros)
{
    fbe_u32_t   data_offset;
    fbe_u32_t   data_block_count;
    fbe_u64_t   uninitialized_quadword = 0;
    fbe_u64_t  *data_ptr = NULL;
    fbe_u32_t   quadword_per_block = (block_size / sizeof(fbe_u64_t));
    fbe_lba_t   lba = in_lba;

    data_ptr = (fbe_u64_t *)data_buffer;
    for (data_block_count = 0; data_block_count < block_count; data_block_count++)
    {
        /*! @todo Currently we allow all 0s to be sent.  Need to confirm this can 
         *        be guaranteed.
         */
        if (b_sends_zeros == FBE_TRUE)
        {
            uninitialized_quadword = 0;
        }
        else
        {
            uninitialized_quadword = ((drive->backend_number & 0x00000000000000FFULL) << 56) |
                                     ((drive->encl_number    & 0x00000000000000FFULL) << 48) |
                                     ((drive->slot_number    & 0x00000000000000FFULL) << 40) |
                                     (lba                    & 0x000000FFFFFFFFFFULL);
        }
        for (data_offset = 0; data_offset < quadword_per_block; data_offset++)
        {
            *data_ptr++ = uninitialized_quadword;
        }
        lba++;
    }

    return FBE_STATUS_OK;
}

fbe_status_t terminator_simulated_disk_memory_read(terminator_drive_t * drive,
                                                   fbe_lba_t lba,
                                                   fbe_block_count_t block_count,
                                                   fbe_block_size_t block_size,
                                                   fbe_u8_t * data_buffer,
												   void * context)
{
    journal_record_t * record = NULL;
    fbe_queue_element_t * queue_element = NULL;
    fbe_bool_t is_intersected = FALSE;
    fbe_u32_t data_offset;
    fbe_u32_t data_block_count;
	fbe_bool_t is_record_found = FBE_FALSE;
    fbe_u64_t * data_ptr = NULL;
    fbe_terminator_io_t * terminator_io = (fbe_terminator_io_t *)context;
    fbe_bool_t need_decompress = FBE_FALSE;
    fbe_bool_t compressed_record_exist = FBE_FALSE;
    fbe_u32_t record_block_size = 0;
    fbe_u32_t total_blocks = 0;
    fbe_bool_t is_any_record_unmapped = FBE_FALSE;

    /* Get the data buffer.
     */
    data_ptr = (fbe_u64_t *)data_buffer;

/* This is performance killer
    for(;(fbe_u8_t *)data_ptr < data_buffer + block_count * block_size; data_ptr ++){
                                            *data_ptr = 0xBAD0BAD0BAD0BAD0;
                                                           }
*/
    fbe_spinlock_lock(&drive->journal_lock);
    queue_element = fbe_queue_front(&drive->journal_queue_head);

    while(queue_element != NULL){
        record = (journal_record_t *)queue_element;

        /* The records are sorted */
        if(lba + block_count < record->lba ){
            break;
        }

        is_intersected = journal_record_is_intersected(record, lba, block_count);
        if(is_intersected == FBE_TRUE){
            is_record_found = FBE_TRUE;


            /* is record unmapped */
            if (record->flags & TERMINATOR_JOURNAL_RECORD_FLAG_UNMAPPED)
            {
                is_any_record_unmapped = FBE_TRUE;
            }

            /* We could read from 2 records with different block size */
            record_block_size = record->block_size;
            if (!record->is_compressed) {
                 need_decompress = FBE_TRUE;
                 if (compressed_record_exist) {
                    compressed_record_exist = FBE_FALSE;
                    total_blocks = 0;
                    queue_element = fbe_queue_front(&drive->journal_queue_head);
                    continue;
                }
            }

            if(lba >= record->lba){
                /* record->lba |----- ...
                 * lba            |-- ... */
                data_offset = (fbe_u32_t)((lba - record->lba)* block_size);
                data_block_count = (fbe_u32_t) FBE_MIN(block_count, (record->block_count - (lba - record->lba)));
                total_blocks += data_block_count;
                if(record->record_type == TERMINATOR_JOURNAL_RECORD_ZERO){
                    fbe_bool_t do_valid_metadata = (record->flags & TERMINATOR_JOURNAL_RECORD_FLAG_UNMAPPED)? FBE_FALSE : FBE_TRUE;   /* zero the metadata if blocks are unmapped */
                    terminator_simulated_disk_generate_zero_buffer(data_buffer, lba, data_block_count, block_size, do_valid_metadata);
                    if (record->data_ptr != NULL) {
                        /* There is a key.  The caller expects encrypted data, so encrypt it.   */
                        terminator_simulated_disk_encrypt_data(data_buffer, 
                                               (fbe_u32_t)(data_block_count * block_size),
                                               record->data_ptr /* key */, FBE_ENCRYPTION_KEY_SIZE);
                    }
                }
                else if (record->is_compressed){
                    data_offset = (fbe_u32_t)((lba - record->lba)* record->block_size);
                    if (need_decompress) {
                        terminator_drive_decompress_record_to_memory(data_buffer, lba, data_block_count, block_size, record->data_ptr + data_offset, record->block_size);
                    } else {
						compressed_record_exist = FBE_TRUE;
                        fbe_copy_memory(data_buffer , record->data_ptr + data_offset, data_block_count * record->block_size);
                    }
                }
                else{
                    fbe_copy_memory(data_buffer , record->data_ptr + data_offset, data_block_count * block_size);
                }
            }else {
                /* record->lba    |-- ...
                 * lba          |---- ... */
                data_offset = (fbe_u32_t)((record->lba - lba)* block_size);
                data_block_count = (fbe_u32_t) FBE_MIN(record->block_count, (block_count - (record->lba - lba)));
                total_blocks += data_block_count;
                if(record->record_type == TERMINATOR_JOURNAL_RECORD_ZERO) {
                    fbe_bool_t do_valid_metadata = (record->flags & TERMINATOR_JOURNAL_RECORD_FLAG_UNMAPPED)? FBE_FALSE : FBE_TRUE;   /* zero the metadata if blocks are unmapped */
                    terminator_simulated_disk_generate_zero_buffer(data_buffer + data_offset, record->lba, data_block_count, block_size, do_valid_metadata);
                    if (record->data_ptr != NULL) {
                        /* There is a key.  The caller expects encrypted data, so encrypt it.   */
                        terminator_simulated_disk_encrypt_data(data_buffer + data_offset, 
                                               (fbe_u32_t)(data_block_count * block_size),
                                               record->data_ptr /* key */, FBE_ENCRYPTION_KEY_SIZE);
                    }
                }
                else if (record->is_compressed){
                    if (need_decompress) {
                        terminator_drive_decompress_record_to_memory(data_buffer + data_offset, record->lba, data_block_count, block_size, record->data_ptr, record->block_size);
                    } else {
						compressed_record_exist = FBE_TRUE;
                        data_offset = (fbe_u32_t)((record->lba - lba)* record->block_size);
                        fbe_copy_memory(data_buffer + data_offset, record->data_ptr, data_block_count * record->block_size);
                    }
                }
                else{
                    fbe_copy_memory(data_buffer + data_offset, record->data_ptr , data_block_count * block_size);
                }
            }
        }

        queue_element = fbe_queue_next(&drive->journal_queue_head, queue_element);
    }

    fbe_spinlock_unlock(&drive->journal_lock);

    /* If no record was found log a critical error.
     */
    if (is_record_found == FBE_FALSE)
    {
        /*! @note The database service will read database headers before they 
         *        have been written.  We must allow these reads.
         */
        if (((drive->backend_number <= TERMINATOR_SIMULATED_DISK_MAX_DATABASE_DRIVE_BUS)  &&
             (drive->encl_number    <= TERMINATOR_SIMULATED_DISK_MAX_DATABASE_DRIVE_ENCL) &&
             (drive->slot_number    <= TERMINATOR_SIMULATED_DISK_MAX_DATABASE_DRIVE_SLOT)    ) ||
            ((lba + block_count - 1) < TERMINATOR_SIMULATED_DISK_DEFAULT_BIND_OFFSET)          ||
            ((block_size == 520) /*!@todo HACK*/                               &&
             (block_count == TERMINATOR_SIMULATED_DISK_520_BPS_OPTIMAL_BLOCKS)     )           ||
            ((block_size == 512) /*!@todo HACK*/                               &&
             (block_count == TERMINATOR_SIMULATED_DISK_512_BPS_OPTIMAL_BLOCKS)     )              ) 

        {
            /* Populate the buffers with a known `uninitialized' pattern.
             */
            terminator_simulated_disk_memory_populate_uninitialized_data(drive,
                                                                         lba,
                                                                         block_count,
                                                                         block_size,
                                                                         data_buffer,
                                                                         FBE_FALSE);
        }
        else
        {
            /* Else we should not be accessing the user area unless it has been
             * written.
             */
            terminator_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "sim disk mem read: %d_%d_%d no read data for lba: 0x%016llx blks: 0x%08llx blk_size: %d\n", 
                         drive->backend_number, drive->encl_number,
			 drive->slot_number, (unsigned long long)lba,
			 (unsigned long long)block_count, block_size);
        }
    }

    if (is_any_record_unmapped && !terminator_io->u.read.is_unmapped_allowed){
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, 
                         "%s attempted to read unmapped area\n", __FUNCTION__);
    }


    if (terminator_io) {
        if (is_any_record_unmapped){
            terminator_io->u.read.is_unmapped = FBE_TRUE;
        }
        if (compressed_record_exist && !need_decompress) {
            terminator_io->return_size = (fbe_u32_t)(block_count * record_block_size);
            terminator_simulated_disk_verify_compressed_record(drive, FBE_TRUE, data_buffer, terminator_io->return_size);
        } else {
            terminator_io->return_size = (fbe_u32_t)(block_count * block_size);
        }
    } 

/* This is performance killer
    invalid_data = 0xcdcdcdcdcdcdcdcd;
    if(fbe_equal_memory(data_buffer, &invalid_data, sizeof(fbe_u64_t))) {
        terminator_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "\n\n\n %s got invalid data at lba 0x%llx. We are in trouble. \n\n\n",
                         __FUNCTION__, (unsigned long long)lba);
    }
*/
    return FBE_STATUS_OK;
}

static fbe_bool_t journal_record_is_intersected(journal_record_t * record, fbe_lba_t lba, fbe_block_count_t block_count)
{
    if((record->lba <= lba) && (record->lba + record->block_count > lba)) {
        return FBE_TRUE;
    }
    if((lba <= record->lba) && (lba + block_count > record->lba)){
        return FBE_TRUE;
    }
    return FBE_FALSE;
}

static fbe_status_t process_intersected_zero_record (terminator_drive_t *drive,
                                                     journal_record_t * record, 
                                                     journal_record_t * new_record, 
                                                     fbe_block_size_t block_size)
{
    journal_record_t * split_record = NULL;
    fbe_u32_t data_offset;

    if(record->lba < new_record->lba) {
        /* new_record                   |------|
         * record                  |--------...
         * split_record            |----|       */
        if(record->record_type == TERMINATOR_JOURNAL_RECORD_WRITE) {
            split_record = terminator_simulated_disk_memory_allocate_record(drive, record->record_type,
                                                                            (new_record->lba - record->lba),
                                                                            record->block_size);
            split_record->data_ptr = (fbe_u8_t *)split_record + sizeof(journal_record_t);
            split_record->data_size = (fbe_u32_t)((new_record->lba - record->lba)*record->block_size);
            fbe_copy_memory(split_record->data_ptr, record->data_ptr, split_record->data_size);
        }else{
            split_record = terminator_simulated_disk_memory_allocate_record(drive, record->record_type, 0, 0);

            if (record->data_ptr != NULL) {
                split_record->data_ptr = (fbe_u8_t *)split_record + sizeof(journal_record_t);
                fbe_copy_memory(split_record->data_ptr, record->data_ptr, sizeof(fbe_u8_t) * FBE_ENCRYPTION_KEY_SIZE);
            } else {
                split_record->data_ptr = NULL;
            }
            split_record->data_size = 0;
        }
        split_record->block_size = record->block_size;
        split_record->is_compressed = record->is_compressed;
        split_record->flags = record->flags;
        split_record->record_type = record->record_type;
        split_record->lba = record->lba;
        split_record->block_count = (fbe_block_count_t) (new_record->lba - record->lba);
        terminator_simulated_disk_verify_compressed_record(drive, split_record->is_compressed, split_record->data_ptr, split_record->data_size);
        /* add before the new record */
        ((fbe_queue_element_t *)(new_record->queue_element.prev))->next = split_record;
        split_record->queue_element.prev = new_record->queue_element.prev;
        new_record->queue_element.prev = split_record;
        split_record->queue_element.next = new_record;
    }
    if((record->lba + record->block_count) > (new_record->lba + new_record->block_count)) {
        /* new_record              |------|
         * record                   ...--------|
         * split_record                   |----| */
        if(record->record_type == TERMINATOR_JOURNAL_RECORD_WRITE) {
            split_record = terminator_simulated_disk_memory_allocate_record(drive, record->record_type,
                              ((record->lba + record->block_count) - (new_record->lba + new_record->block_count)),
                              record->block_size);
            split_record->data_ptr = (fbe_u8_t *)split_record + sizeof(journal_record_t);
            split_record->data_size = (fbe_u32_t)(((record->lba + record->block_count) - (new_record->lba + new_record->block_count))*record->block_size);
            /* fill in the old data */
            data_offset = (fbe_u32_t)(((new_record->lba + new_record->block_count) - record->lba)* record->block_size);
            fbe_copy_memory(split_record->data_ptr, record->data_ptr + data_offset, split_record->data_size);

        }else{
            split_record = terminator_simulated_disk_memory_allocate_record(drive, record->record_type, 0, 0);
            if (record->data_ptr != NULL) {
                split_record->data_ptr = (fbe_u8_t *)split_record + sizeof(journal_record_t);
                fbe_copy_memory(split_record->data_ptr, record->data_ptr, sizeof(fbe_u8_t) * FBE_ENCRYPTION_KEY_SIZE);
            } else {
                split_record->data_ptr = NULL;
            }
            split_record->data_size = 0;
        }
        split_record->block_size = record->block_size;
        split_record->is_compressed = record->is_compressed;
        split_record->flags = record->flags;
        split_record->record_type = record->record_type;
        split_record->lba = new_record->lba + new_record->block_count;
        split_record->block_count = (fbe_block_count_t)((record->lba + record->block_count) - split_record->lba);
        terminator_simulated_disk_verify_compressed_record(drive, split_record->is_compressed, split_record->data_ptr, split_record->data_size);
        /* add after the new record */
        ((fbe_queue_element_t *)(new_record->queue_element.next))->prev = split_record;
        split_record->queue_element.next = new_record->queue_element.next;
        new_record->queue_element.next = split_record;
        split_record->queue_element.prev = new_record;
    }
    return FBE_STATUS_OK;
}

fbe_status_t drive_journal_destroy(terminator_drive_t * drive)
{
    journal_record_t * record = NULL;
    fbe_queue_element_t * queue_element;

    /* Walk trou the queue and release all journal_records */
    fbe_spinlock_lock(&drive->journal_lock);
    queue_element = fbe_queue_pop(&drive->journal_queue_head);
    /* printf("\n\n"); */
    while(queue_element != NULL){
        record = (journal_record_t *)queue_element;

        /* printf("free lba %d block_count %d \n", (int)(record->lba), (int)(record->block_count)); */
        terminator_simulated_disk_memory_free_record(drive, &record);
        queue_element = fbe_queue_pop(&drive->journal_queue_head);
    }
    fbe_spinlock_unlock(&drive->journal_lock);

    fbe_spinlock_destroy(&drive->journal_lock);
    fbe_queue_destroy(&drive->journal_queue_head);
    return FBE_STATUS_OK;
}


static fbe_status_t 
terminator_drive_decompress_one_block(fbe_u8_t * data_buffer_pointer, 
                                      fbe_block_size_t block_size,
                                      fbe_u8_t * record_data_pointer)
{
    fbe_u64_t * data_p = (fbe_u64_t *)data_buffer_pointer;
    fbe_terminator_compressed_block_data_pattern_t * current_pattern;
    fbe_u32_t i, pattern_index;
    fbe_u32_t total_count = 0;

    current_pattern = (fbe_terminator_compressed_block_data_pattern_t *)record_data_pointer;
    for (pattern_index = 0; pattern_index < TERMINATOR_MAX_PATTERNS; pattern_index++) {
        for (i = 0; i < current_pattern->count; i++) {
            *data_p = current_pattern->pattern;
			data_p++;
        }
        total_count += current_pattern->count;
        if (total_count >= (block_size/sizeof(fbe_u64_t))) {
            break;
        }
        current_pattern++;
    }

    if (total_count != (block_size/sizeof(fbe_u64_t))) {
        terminator_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, 
                         "Total count not correct %d\n", total_count);
    }

    return FBE_STATUS_OK;
}

fbe_status_t terminator_drive_decompress_record_to_memory(fbe_u8_t * data_buffer_pointer, 
                                                         fbe_lba_t lba, 
                                                         fbe_block_count_t block_count, 
                                                         fbe_block_size_t block_size,
                                                         fbe_u8_t * record_data_pointer, 
                                                         fbe_block_size_t record_block_size)
{
    fbe_u8_t * data_p = data_buffer_pointer;
    fbe_u8_t * record_data_p = record_data_pointer;
    fbe_u32_t i;

    if (block_size > FBE_BE_BYTES_PER_BLOCK) {
        block_count *= (block_size/FBE_BE_BYTES_PER_BLOCK);
		record_block_size /= (block_size/FBE_BE_BYTES_PER_BLOCK);
        block_size = FBE_BE_BYTES_PER_BLOCK;
    }

    if (record_block_size != sizeof(fbe_terminator_compressed_block_t)) {
        terminator_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, 
                         "record block size 0x%x not correct\n", record_block_size);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for (i = 0; i < block_count; i++) {
        terminator_drive_decompress_one_block(data_p, block_size, record_data_p);
        data_p += block_size;
        record_data_p += record_block_size;
    }

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_INFO,
                     "%s LBA 0x%llx block_count 0x%llx\n",
                     __FUNCTION__, lba, block_count);

    return FBE_STATUS_OK;
}
