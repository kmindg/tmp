#include "fbe/fbe_types.h"
#include "fbe_database_private.h"
#include "fbe_database_system_db_persist_transaction.h"
#include "fbe_database_system_db_interface.h"

/*support only one persist transaction of system db at the same time. 
  because the system only supports one transaction at the same time, so
  only one system db persist transaction needed either*/
static fbe_database_system_db_persist_transaction_t system_db_persist_transaction;


static fbe_status_t system_db_persist_transaction_get_free_entry(fbe_database_system_db_persist_transaction_t *transaction,
                                                                 fbe_u32_t *index);
static fbe_status_t system_db_persist_transaction_release_free_entry(fbe_database_system_db_persist_transaction_t *transaction,
                                                                 fbe_u32_t index);
static fbe_status_t system_db_persist_transaction_add_elem(fbe_database_system_db_persist_transaction_t *transaction,
                                                           fbe_u32_t index, fbe_lba_t lba,
                                                           fbe_u8_t *data, fbe_u32_t size);
static fbe_database_system_db_persist_transaction_t* system_db_persist_get_transaction(fbe_database_system_db_persist_handle_t handle);


/*Get a free entry in the transaction entry table to store the persist data*/
static fbe_status_t system_db_persist_transaction_get_free_entry(fbe_database_system_db_persist_transaction_t *transaction,
                                                                 fbe_u32_t *index)
{
    if (transaction == NULL || index == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (transaction->entry_num < MAX_DATABASE_SYSTEM_DB_PERSIST_ENTRY_NUM) {
        *index = transaction->entry_num;
        transaction->entry_num ++;
        return FBE_STATUS_OK;
    }

    return FBE_STATUS_GENERIC_FAILURE;
}

/*Get a free entry in the transaction entry table to store the persist data*/
static fbe_status_t system_db_persist_transaction_release_free_entry(fbe_database_system_db_persist_transaction_t *transaction,
                                                                 fbe_u32_t index)
{
    if (transaction == NULL || index >= MAX_DATABASE_SYSTEM_DB_PERSIST_ENTRY_NUM) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (transaction->entry_num > 0) {
        transaction->entry_num --;
        return FBE_STATUS_OK;
    }

    return FBE_STATUS_GENERIC_FAILURE;
}

/*add a persist entry to the transaction table*/
static fbe_status_t system_db_persist_transaction_add_elem(fbe_database_system_db_persist_transaction_t *transaction,
                                                           fbe_u32_t index, fbe_lba_t lba,
                                                           fbe_u8_t *data, fbe_u32_t size)
{

    if (index >= MAX_DATABASE_SYSTEM_DB_PERSIST_ENTRY_NUM ||
        data == NULL || size == 0 || transaction == NULL ||
        size > MAX_PERSIST_ELEM_DATA_SIZE) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*copy the data to a entry in the transaction table. 
      currently only use pre-allocated space to persist data with maximum size,
      may need allocate memory dynamically here if will support arbitrary sized
      persist data*/
    transaction->entry_table[index].size = size;
    transaction->entry_table[index].lba = lba;
    fbe_copy_memory(transaction->entry_table[index].data, data, size);

    return FBE_STATUS_OK;
}

/*Transform from the handle to the transaction, currently only one transaction in the system*/
static fbe_database_system_db_persist_transaction_t* system_db_persist_get_transaction(fbe_database_system_db_persist_handle_t handle)
{
    if (system_db_persist_transaction.current_handle == handle) {
        return &system_db_persist_transaction;
    }

    return NULL;
}

/*start a persist transaction for system db entries*/
fbe_status_t fbe_database_system_db_start_persist_transaction(fbe_database_system_db_persist_handle_t *handle)
{
    fbe_bool_t	active_side = database_common_cmi_is_active();

    /*only allowed to start a persist transaction on the active side*/
	if (!active_side) {
		return FBE_STATUS_OK;
	}


    if (handle == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_lock(&system_db_persist_transaction.lock);
    /*only allowed to start a transaction if no active transaction in the system*/
    if (system_db_persist_transaction.state != FBE_DATABASE_SYSTEM_DB_PERSIST_TRANSACTION_INVALID) {
        fbe_spinlock_unlock(&system_db_persist_transaction.lock);
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "system_db_persist_open_handle: failed, pending transaction in state %d \n",
                       system_db_persist_transaction.state);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*init the system db persist transaction*/
    system_db_persist_transaction.current_handle ++;
    system_db_persist_transaction.entry_num = 0;

    /*currently use the static pre-allocated memory and we will use dynamic allocated memory 
      for journal in the next step (maybe after the system db read/write facility support
      scatter-gather list)*/
    fbe_zero_memory(system_db_persist_transaction.entry_table, 
                    MAX_DATABASE_SYSTEM_DB_PERSIST_ENTRY_NUM * sizeof (fbe_database_system_db_persist_elem_t));
    system_db_persist_transaction.state = FBE_DATABASE_SYSTEM_DB_PERSIST_TRANSACTION_OPEN;
    *handle = system_db_persist_transaction.current_handle;
    fbe_spinlock_unlock(&system_db_persist_transaction.lock);

    return FBE_STATUS_OK;
}

/*add a element to the persist transaction*/
fbe_status_t fbe_database_system_db_add_persist_elem(fbe_database_system_db_persist_handle_t handle, 
                                                     fbe_lba_t lba, fbe_u8_t *data, fbe_u32_t size)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t current_elem_index;
    fbe_database_system_db_persist_transaction_t *transaction;

    /*check the handle validity*/
    if (data == NULL || size == 0) {
        return FBE_STATUS_GENERIC_FAILURE;
    }


    if ((transaction = system_db_persist_get_transaction(handle)) == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*although only allow one system db persist transaction in the system, many callers (threads) may 
      operate the same transaction simutaneously. spinlock acquired to ensure that each caller get
      different free entry slot to store the data*/
    fbe_spinlock_lock(&transaction->lock);
    if (transaction->state != FBE_DATABASE_SYSTEM_DB_PERSIST_TRANSACTION_OPEN) {
        fbe_spinlock_unlock(&transaction->lock);
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                       "only allow add element in open transaction, transaction state:%d", transaction->state);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*get a free entry in transaction table to store data*/
    status = system_db_persist_transaction_get_free_entry(transaction, &current_elem_index);
    if (status != FBE_STATUS_OK) {
        fbe_spinlock_unlock(&transaction->lock);
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "fbe_database_system_db_add_persist_elem: no free slot exist\n");
        return status;
    }

    /*copy the data to the transaction entry table*/
    status = system_db_persist_transaction_add_elem(transaction,
                                                    current_elem_index, lba, data, size);
    if (status != FBE_STATUS_OK) {
        /*will call system_db_persist_transaction_release_free_entry to release resource if 
          use dynamic memory allocation in the future version, current need not;*/
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "fbe_database_system_db_add_persist_elem: failed to add element\n");
    }
    fbe_spinlock_unlock(&transaction->lock);

    return status;
}

/*commit the persist transaction*/
fbe_status_t fbe_database_system_db_persist_commit(fbe_database_system_db_persist_handle_t handle)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_database_system_db_persist_transaction_t *transaction;
    fbe_database_system_db_journal_header_t *journal_header;
    fbe_u32_t index;
    fbe_status_t commit_data_status = FBE_STATUS_OK;
    fbe_u64_t io_bytes = 0;
    fbe_bool_t	active_side = database_common_cmi_is_active();

	if (!active_side) {
		return FBE_STATUS_OK;
	}


    if ((transaction = system_db_persist_get_transaction(handle)) == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_lock(&transaction->lock);
    /*we only commit open transaction*/
    if (transaction->state != FBE_DATABASE_SYSTEM_DB_PERSIST_TRANSACTION_OPEN) {
        fbe_spinlock_unlock(&transaction->lock);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*set transaction state to commit*/
    transaction->state = FBE_DATABASE_SYSTEM_DB_PERSIST_TRANSACTION_COMMIT;
    fbe_spinlock_unlock(&transaction->lock);

    /*if transaction->entry_num is zero, mean it is an empty transaction. 
      we support to commit an empty transaction sucessfully by doing nothing.
      commit a transaction by three phases:
      write journal to store the entries in a log;
      commit the data;
      invalidate the journal after data is committed sucessfully*/
    if (transaction->entry_num > 0) {

        /*write journal log first. 
          because writing journal is one IO, so assume it preserves the all-or-nothing
          property*/
        journal_header = &transaction->journal_header;
        journal_header->state = FBE_DATABASE_SYSTEM_DB_JOURNAL_WRITE;
        status = database_system_db_interface_write((fbe_u8_t *)transaction->entry_table, journal_header->journal_data_lba,
                                                sizeof (fbe_database_system_db_persist_elem_t) * transaction->entry_num, &io_bytes);
        if (status != FBE_STATUS_OK || io_bytes != sizeof (fbe_database_system_db_persist_elem_t) * (fbe_u64_t)transaction->entry_num) {
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "system_db_persist_commit: failed to read expected-size journal header\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* journal log wrote done, now write the journal header to disk;
           1) if system crash before writing journal header, no valid journal exist, data lost but
              the integrity is not damaged;
           2) if system crash after writing journal header, we can commit the data by replaying the
              journal log at the next boot time;
        */
        journal_header->state = FBE_DATABASE_SYSTEM_DB_JOURNAL_COMMIT;
        journal_header->journal_entry_num = transaction->entry_num;
        status = database_system_db_interface_write((fbe_u8_t *)journal_header, journal_header->journal_header_lba,
                                                    sizeof (fbe_database_system_db_journal_header_t), &io_bytes);
        if (status != FBE_STATUS_OK || io_bytes != sizeof (fbe_database_system_db_journal_header_t)) {
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "system_db_persist_commit: failed to write expected-size journal header\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /*starting commit the data now*/
        for (index = 0; index < transaction->entry_num; index ++) {
            commit_data_status = database_system_db_interface_write(transaction->entry_table[index].data,
                                                        transaction->entry_table[index].lba,
                                                        transaction->entry_table[index].size,
                                                        &io_bytes);
            if (commit_data_status != FBE_STATUS_OK ||
                io_bytes != transaction->entry_table[index].size) {
                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "system_db_persist_commit: failed to write expected-size journal entry\n");
                commit_data_status = FBE_STATUS_GENERIC_FAILURE;
                break;
            }
        }

        if (commit_data_status != FBE_STATUS_OK) {
            /*only part of data is committed to the disk, serious problem. 
              what to do here? currently we return the error code to the caller
              to handle the problem*/
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /*invalidate the journal data*/
        journal_header->state = FBE_DATABASE_SYSTEM_DB_JOURNAL_INVALIDATE;
        status = database_system_db_interface_write((fbe_u8_t *)journal_header, journal_header->journal_header_lba,
                                                        sizeof (fbe_database_system_db_journal_header_t), &io_bytes);
        if (status != FBE_STATUS_OK || io_bytes != sizeof (fbe_database_system_db_journal_header_t)) {
           /*data commit sucessfully but invalidate journal log failure. 
             don't know what to do here, return error code to the caller to
             handle it*/
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "system_db_persist_commit: failed to write expected-size journal header\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }

    }

    fbe_spinlock_lock(&transaction->lock);
    transaction->entry_num = 0;
    transaction->state = FBE_DATABASE_SYSTEM_DB_PERSIST_TRANSACTION_INVALID;
    fbe_spinlock_unlock(&transaction->lock);

    return FBE_STATUS_OK;
}

/*abort the system db persist transaction*/
fbe_status_t fbe_database_system_db_persist_abort(fbe_database_system_db_persist_handle_t handle)
{
    fbe_database_system_db_persist_transaction_t *transaction;
    fbe_bool_t	active_side = database_common_cmi_is_active();

    /*we should only operate the system db persist transaction in the active side*/
	if (!active_side) {
		return FBE_STATUS_OK;
	}


    if ((transaction = system_db_persist_get_transaction(handle)) == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_lock(&transaction->lock);
    /*we only abort the open transaction*/
    if (transaction->state != FBE_DATABASE_SYSTEM_DB_PERSIST_TRANSACTION_OPEN) {
        fbe_spinlock_unlock(&transaction->lock);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*current abort just cleans the transaction temporal state and data*/
    transaction->entry_num = 0;
    fbe_zero_memory(transaction->entry_table, 
                    MAX_DATABASE_SYSTEM_DB_PERSIST_ENTRY_NUM * sizeof (fbe_database_system_db_persist_elem_t));
    transaction->state = FBE_DATABASE_SYSTEM_DB_PERSIST_TRANSACTION_INVALID;
    fbe_spinlock_unlock(&transaction->lock);

    return FBE_STATUS_OK;
}

/*read, check and replay the journal state; will be called when system boots or restarted*/
fbe_status_t fbe_database_system_db_replay_journal(fbe_lba_t lba, 
                                                   fbe_database_system_db_journal_header_t *journal_header,
                                                   fbe_u32_t valid_db_drive_mask)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_status_t replay_status = FBE_STATUS_OK;
    fbe_database_system_db_persist_elem_t *persist_elem = NULL;
    fbe_u64_t io_bytes = 0;
    fbe_u32_t index;


    if (journal_header == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*we need replay the journal if journal log wrote but data not commited*/
    if (journal_header->state == FBE_DATABASE_SYSTEM_DB_JOURNAL_COMMIT) {
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                       "database system db start replay journal: lba: %llx, entry_num: %x \n", 
                       (unsigned long long)journal_header->journal_data_lba,
		       journal_header->journal_entry_num);
        if ((persist_elem = (fbe_database_system_db_persist_elem_t *)fbe_memory_native_allocate(
            journal_header->journal_entry_num * sizeof (fbe_database_system_db_persist_elem_t))) == NULL) {
            return FBE_STATUS_GENERIC_FAILURE;
        }
        /*read the journal log from the disk*/
        status = database_system_db_interface_read((fbe_u8_t *)persist_elem,
                                                   journal_header->journal_data_lba,
                                                   journal_header->journal_entry_num * sizeof (fbe_database_system_db_persist_elem_t),
                                                   valid_db_drive_mask,
                                                   &io_bytes);
        if (status != FBE_STATUS_OK ||
            io_bytes != (fbe_u64_t)journal_header->journal_entry_num * sizeof (fbe_database_system_db_persist_elem_t)) {
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "system_db_replay_journal: failed to read expected-size journal header\n");
            if (persist_elem != NULL) 
                fbe_memory_native_release(persist_elem);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /*commit the data according to the journal log read from the disk*/
        for (index = 0; index < journal_header->journal_entry_num; index ++) {
            replay_status = database_system_db_interface_write(persist_elem[index].data,
                                                               persist_elem[index].lba,
                                                               persist_elem[index].size, 
                                                               &io_bytes);
            if (replay_status != FBE_STATUS_OK || io_bytes != persist_elem[index].size) {
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "replay journal and commit data error");
                replay_status = FBE_STATUS_GENERIC_FAILURE;
                break;
            }
        }

        /*current we only invalidate the journal header after replaying sucessfully. 
          but what if replay failure? return an error code to the caller currently*/
        if (replay_status == FBE_STATUS_OK) {
            /*invalidate the journal header if replay the journal log sucessfully*/
            journal_header->state = FBE_DATABASE_SYSTEM_DB_JOURNAL_INVALIDATE;
            status = database_system_db_interface_write((fbe_u8_t *)journal_header,
                                                        lba, sizeof (fbe_database_system_db_journal_header_t), &io_bytes);
            if (status != FBE_STATUS_OK || io_bytes != sizeof (fbe_database_system_db_journal_header_t)) {
                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "system_db_replay_journal: failed to write expected-size journal header\n");
                if (persist_elem != NULL) 
                    fbe_memory_native_release(persist_elem);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        } else {
            if (persist_elem != NULL) 
                fbe_memory_native_release(persist_elem);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    if (persist_elem != NULL) 
        fbe_memory_native_release(persist_elem);

    return status;
}

/* SAFEBUG - added to make sure we properly manage the lifecycle of system_db_per_transation.lock */
fbe_status_t fbe_database_system_db_persist_world_initialize(void)
{
    fbe_spinlock_init(&system_db_persist_transaction.lock);
    return FBE_STATUS_OK;
}

/* SAFEBUG - added to make sure we properly manage the lifecycle of system_db_per_transation.lock */
fbe_status_t fbe_database_system_db_persist_world_teardown(void)
{
    fbe_spinlock_destroy(&system_db_persist_transaction.lock);
    return FBE_STATUS_OK;
}

/*initialize the system db persist transaction. init or replay the journal log if 
  necessary; the caller must give corrent LBA for journal header and journal log
  start LBA*/
fbe_status_t fbe_database_system_db_persist_initialize(fbe_lba_t journal_header_lba, 
                                                       fbe_lba_t journal_data_lba,
                                                       fbe_u32_t valid_db_drive_mask)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u64_t io_bytes = 0;

    system_db_persist_transaction.current_handle = 1;
    system_db_persist_transaction.entry_num = 0;
    system_db_persist_transaction.state = FBE_DATABASE_SYSTEM_DB_PERSIST_TRANSACTION_INVALID;
    fbe_zero_memory(system_db_persist_transaction.entry_table, 
                    sizeof (fbe_database_system_db_persist_elem_t) * MAX_DATABASE_SYSTEM_DB_PERSIST_ENTRY_NUM);

    /*read the journal header from disk and check*/
    status = database_system_db_interface_read((fbe_u8_t *)&system_db_persist_transaction.journal_header,
                                                journal_header_lba, sizeof (fbe_database_system_db_journal_header_t), 
                                                valid_db_drive_mask, &io_bytes);
    if (status != FBE_STATUS_OK || io_bytes != sizeof (fbe_database_system_db_journal_header_t)) {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, "database system db failed to read the journal header");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*check the journal header magic*/
    if (system_db_persist_transaction.journal_header.magic != FBE_DATABASE_SYSTEM_DB_JOURNAL_HEADER_MAGIC) {
        /*the journal header is invalid, means it is not inited yet, 
          we must initialize it here*/
        fbe_zero_memory(&system_db_persist_transaction.journal_header, 
                        sizeof (fbe_database_system_db_journal_header_t));
        system_db_persist_transaction.journal_header.version = FBE_DATABASE_SYSTEM_DB_JOURNAL_VERSION;
        system_db_persist_transaction.journal_header.magic = FBE_DATABASE_SYSTEM_DB_JOURNAL_HEADER_MAGIC;
        system_db_persist_transaction.journal_header.state = FBE_DATABASE_SYSTEM_DB_JOURNAL_INVALID;
        system_db_persist_transaction.journal_header.journal_data_lba = journal_data_lba;
        system_db_persist_transaction.journal_header.journal_header_lba = journal_header_lba;
        status = database_system_db_interface_write((fbe_u8_t *)&system_db_persist_transaction.journal_header,
                                                journal_header_lba, sizeof (fbe_database_system_db_journal_header_t), &io_bytes);
        if (status != FBE_STATUS_OK || io_bytes != sizeof (fbe_database_system_db_journal_header_t)) {
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                           "system_db_persist_init: failed to init journal header 1st time\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }

        return status;
    }
    
    /*initialize the journal header info and replay the journal log if necessary*/
    system_db_persist_transaction.journal_header.journal_header_lba = journal_header_lba;
    system_db_persist_transaction.journal_header.journal_data_lba = journal_data_lba;
    status = fbe_database_system_db_replay_journal(journal_header_lba, 
                                                   &system_db_persist_transaction.journal_header,
                                                   valid_db_drive_mask);
    if (status != FBE_STATUS_OK) {
        /*replay journal failure, we must do something here*/
        /*but how to handle this serious failure? just return failure directly at current*/

        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to replay journal\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
