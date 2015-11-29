#ifndef FBE_PERSIST_DATABASE_H
#define FBE_PERSIST_DATABASE_H

#include "fbe/fbe_persist_interface.h"


#define FBE_PERSIST_DB_READ_BUF_MAX 5

#define FBE_PERSIST_DB_SIGNATURE 0xFBEFAA1111AAFEBF
#define FBE_PERSIST_DB_VERSION 10
#define FBE_PERSIST_DB_JOURNAL_VALID_STATE  0xFBEDB888


typedef struct fbe_persist_sector_order_s{
	fbe_persist_sector_type_t	sector_type;
	fbe_u32_t					entries;
}fbe_persist_sector_order_t;

#pragma pack(1)
typedef struct fbe_persist_db_header_s{
    fbe_u64_t   signature;  /*identify whether the header is valid*/
    fbe_u64_t   version;    /*record the current version of persist service*/
    fbe_u32_t   journal_state;  /*if it is FBE_PERSIST_DB_JOURNAL_VALID_STATE, then the journal is valid*/
    fbe_u32_t   journal_size;   /*the number of valid entries in the journal if it is valid*/
}fbe_persist_db_header_t;
#pragma pack()


/* These are the prototypes for invoking persist database operations. */
fbe_status_t fbe_persist_database_init(void); 
void fbe_persist_database_destroy(void);
fbe_lba_t fbe_persist_database_get_tran_journal_lba(void);
fbe_status_t fbe_persist_database_read_single_entry(fbe_packet_t * packet_p, fbe_persist_control_read_entry_t* control_read);
fbe_status_t fbe_persist_database_get_entry_id(fbe_persist_entry_id_t *entry_id, fbe_persist_sector_type_t setctor_type);
fbe_status_t fbe_persist_entry_exists(fbe_persist_entry_id_t entry_id, fbe_bool_t *exists);
fbe_status_t fbe_persist_clear_entry_bit(fbe_persist_entry_id_t entry_id);
fbe_status_t fbe_persist_database_get_entry_lba(fbe_persist_entry_id_t entry_id, fbe_lba_t *lba);
fbe_status_t fbe_persist_database_read_sector(fbe_packet_t *packet, fbe_persist_control_read_sector_t *read_sector);
fbe_status_t fbe_persist_database_update_bitmap(fbe_packet_t *packet, fbe_persist_sector_type_t setctor_type, fbe_persist_entry_id_t entry_id);
fbe_status_t fbe_persist_set_entry_bit(fbe_persist_entry_id_t entry_id);
fbe_status_t fbe_persist_database_get_lba_layout_info( fbe_persist_control_get_layout_info_t *get_layout);
fbe_status_t fbe_persist_database_read_persist_db_header(fbe_packet_t *packet);
fbe_status_t fbe_persist_database_write_persist_db_header(fbe_packet_t *packet);

#endif /* FBE_PERSIST_DATABASE_H */
