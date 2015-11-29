#ifndef FBE_DATABASE_SYSTEM_DB_PERSIST_TRANSACTION_H__
#define FBE_DATABASE_SYSTEM_DB_PERSIST_TRANSACTION_H__
#include "fbe_database_system_db_interface.h"


#define MAX_PERSIST_ELEM_DATA_SIZE                                384            
#define FBE_DATABASE_SYSTEM_DB_JOURNAL_HEADER_MAGIC               0xabbadeaddeadcddc
#define FBE_DATABASE_SYSTEM_DB_JOURNAL_VERSION                    1

/*journal related data structures*/
#pragma pack(1)
typedef enum{
    FBE_DATABASE_SYSTEM_DB_JOURNAL_INVALID,                         /*journal state is invalid*/
    FBE_DATABASE_SYSTEM_DB_JOURNAL_WRITE,                           /*system is writing journal*/
    FBE_DATABASE_SYSTEM_DB_JOURNAL_COMMIT,                          /*journal write done, is committing data;
                                                                      may need replay journal if booting in this state*/
    FBE_DATABASE_SYSTEM_DB_JOURNAL_INVALIDATE                       /*data IO done, journal committed*/
} fbe_database_system_db_journal_state_t;
#pragma pack()

/*journal header, including metadata required to read and replay journal*/
#pragma pack(1)
typedef struct fbe_database_system_db_journal_header_s{
    fbe_u64_t magic;                                     /*magic for check journal validity*/
    fbe_u64_t version;                                   /*version of the journal header*/
    fbe_database_system_db_journal_state_t state;
    fbe_u32_t journal_entry_num;                         /*the number of journaled entries*/
    fbe_lba_t journal_header_lba;                        /*LBA to write journal header*/
    fbe_lba_t journal_data_lba;                          /*start LBA to write journal log*/
} fbe_database_system_db_journal_header_t;
#pragma pack()

/*journal record, including data will be committed and its LBA*/
#pragma pack(1)
typedef struct fbe_database_system_db_persist_elem_s{
    fbe_lba_t lba;                                        /*LBA the data will be wrote*/
    fbe_u32_t size;                                       /*data size*/
    fbe_u8_t  data[MAX_PERSIST_ELEM_DATA_SIZE];           /*the data will be committed*/
} fbe_database_system_db_persist_elem_t;
#pragma pack()

/*transaction state 
  FBE_DATABASE_SYSTEM_DB_PERSIST_TRANSACTION_INVALID: no active transaction this time, need start
                                                      it first
  FBE_DATABASE_SYSTEM_DB_PERSIST_TRANSACTION_OPEN:   transaction active, can add persist
                                                     entries to it;
  FBE_DATABASE_SYSTEM_DB_PERSIST_TRANSACTION_COMMIT: is commiting the persist transaction now,
                                                     3 steps: write journal, commit data, invalidate
                                                     journal
  FBE_DATABASE_SYSTEM_DB_PERSIST_TRANSACTION_ROLLBACK: rollback the transaction, currently noting
                                                       need to be done here
 
*/
typedef enum fbe_database_system_db_persist_transaction_state_s{
    FBE_DATABASE_SYSTEM_DB_PERSIST_TRANSACTION_INVALID,
    FBE_DATABASE_SYSTEM_DB_PERSIST_TRANSACTION_OPEN,
    FBE_DATABASE_SYSTEM_DB_PERSIST_TRANSACTION_COMMIT,
    FBE_DATABASE_SYSTEM_DB_PERSIST_TRANSACTION_ROLLBACK
} fbe_database_system_db_persist_transaction_state_t;

/*system db persist transaction*/
/*only support limited transaction support for system db persist, we assume 
  no more than MAX_DATABASE_SYSTEM_DB_PERSIST_ENTRY_NUM will be persist and
  each entry size is under MAX_PERSIST_ELEM_DATA_SIZE; currently we only need
  persist database_object_entry_t, database_user_entry_t, database_edge_entry_t,
  and system spare pvd info here, so it is enough*/
typedef struct fbe_database_system_db_persist_transaction_s{
    fbe_spinlock_t lock;                                           /*spinlock to protect the transaction table*/
    fbe_u64_t current_handle;                                      /*handle to identify a system db persist transaction*/
    fbe_database_system_db_persist_transaction_state_t state;
    fbe_u32_t entry_num;                                          /*persist entry number in the transaction*/
    fbe_database_system_db_journal_header_t journal_header;       /*journal header of the persist transaction*/

    fbe_database_system_db_persist_elem_t entry_table[MAX_DATABASE_SYSTEM_DB_PERSIST_ENTRY_NUM];
} fbe_database_system_db_persist_transaction_t;


/*add a element to the persist transaction*/
fbe_status_t fbe_database_system_db_add_persist_elem(fbe_database_system_db_persist_handle_t handle, 
                                                     fbe_lba_t lba, fbe_u8_t *data, fbe_u32_t size);

/*read, check and replay the journal state*/
fbe_status_t fbe_database_system_db_replay_journal(fbe_lba_t lba, fbe_database_system_db_journal_header_t *journal_header,
                                                   fbe_u32_t valid_drive_mask);

#endif
