#ifndef FBE_DATABASE_SYSTEM_DB_INTERFACE_H
#define FBE_DATABASE_SYSTEM_DB_INTERFACE_H
 /* All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_database_system_db_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains local definitions for the database system db interface.
 *
 * @version
 *   09/26/2011:  Created. 
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe_private_space_layout.h"
#include "fbe_database.h"

#define FBE_DATABASE_SYSTEM_ENTRY_RESERVE_BLOCKS                13 /* minus 2 blocks. These two blocks is used to keep recreation op flags */
#define DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY                  4 /* Use 4 blocks to save one entry */
#define FBE_DATABASE_SYSTEM_DB_LAYOUT_HEADER_SIZE               4
#define FBE_DATABASE_SYSTEM_DB_LAYOUT_SEMV_INFO_SIZE            2
#define FBE_DATABASE_SYSTEM_DB_LAYOUT_SYSTEM_OBJECT_OP_FLAGS_SIZE     2 /* Get 2 blocks from reserved region. The aim is to avoid ICA when promoting code*/
#define FBE_DATABASE_SYSTEM_DB_RAW_MIRROR_STATE_SIZE            1
#define FBE_DATABASE_SYSTEM_DB_LAYOUT_JOURNAL_SIZE              256
#define FBE_DATABASE_SYSTEM_DB_LAYOUT_SYSTEM_SPARE_TABLE_SIZE   48

#define MAX_DATABASE_SYSTEM_DB_PERSIST_ENTRY_NUM                  128

/*the priority of memory request for system db IO operations*/
#define DATABASE_SYSTEM_DB_MEM_REQ_PRIORITY        1
#define DATABASE_SYSTEM_RAW_MIRROR_ALL_DB_DRIVE_MASK          0x7

/*define the valid sequence number region database raw-mirror blocks use.
  avoid use the pure 0 or pure 1*/
#define DATABASE_RAW_MIRROR_START_SEQ              ((fbe_u64_t)0xbeefdaed)
#define DATABASE_RAW_MIRROR_END_SEQ                ((fbe_u64_t)0xFFFFFFFFFFFFdead)

typedef enum{
    FBE_DATABASE_SYSTEM_DB_INVALID_ENTRY,
    FBE_DATABASE_SYSTEM_DB_OBJECT_ENTRY,
    FBE_DATABASE_SYSTEM_DB_USER_ENTRY,
    FBE_DATABASE_SYSTEM_DB_EDGE_ENTRY,
    FBE_DATABASE_SYSTEM_DB_SYSTEM_SPARE_ENTRY
} fbe_db_system_entry_type_t;


/*!***************************************************************
 * define the data layout offset in system db region
 *****************************************************************
 * @Layout
  *  system db header: 4 blocks
  *  shared emv info: 2 blocks
  *  raw mirror state: 1 blocks
  *  recreate system obj flags: 2 blocks
  *  reserved regions: 13 blocks
  *  journal regions: 256 blocks
  *  system spare entry: 48 blocks
  *  object/user/edge entry: system_object_num * max_entry_num_per_object * 4 blocks 
  *  As show belows:
  *  -----------------------------------------------------------------------------------
  *  Header(4) | SEMV(2) | Rawmirror state (1)| recreate op flags (2) | reserved(13) | journal (256) | spare (48) | system entry
  *  -----------------------------------------------------------------------------------
  */
#define FBE_DATABASE_SYSTEM_DB_LAYOUT_HEADER_OFFSET   0
#define FBE_DATABASE_SYSTEM_DB_LAYOUT_SEMV_INFO_OFFSET   (FBE_DATABASE_SYSTEM_DB_LAYOUT_HEADER_OFFSET + FBE_DATABASE_SYSTEM_DB_LAYOUT_HEADER_SIZE)
#define FBE_DATABASE_SYSTEM_DB_LAYOUT_RAW_MIRROR_STATE_OFFSET (FBE_DATABASE_SYSTEM_DB_LAYOUT_SEMV_INFO_OFFSET + FBE_DATABASE_SYSTEM_DB_LAYOUT_SEMV_INFO_SIZE)
#define FBE_DATABASE_SYSTEM_DB_LAYOUT_SYSTEM_OBJECT_OP_FLAGS_OFFSET (FBE_DATABASE_SYSTEM_DB_LAYOUT_RAW_MIRROR_STATE_OFFSET + FBE_DATABASE_SYSTEM_DB_RAW_MIRROR_STATE_SIZE)
#define FBE_DATABASE_SYSTEM_DB_LAYOUT_JOURNAL_OFFSET   (FBE_DATABASE_SYSTEM_DB_LAYOUT_SYSTEM_OBJECT_OP_FLAGS_OFFSET + FBE_DATABASE_SYSTEM_DB_LAYOUT_SYSTEM_OBJECT_OP_FLAGS_SIZE + FBE_DATABASE_SYSTEM_ENTRY_RESERVE_BLOCKS)
#define FBE_DATABASE_SYSTEM_DB_LAYOUT_SYSTEM_SPARE_TABLE_OFFSET   (FBE_DATABASE_SYSTEM_DB_LAYOUT_JOURNAL_OFFSET + FBE_DATABASE_SYSTEM_DB_LAYOUT_JOURNAL_SIZE)
#define FBE_DATABASE_SYSTEM_DB_LAYOUT_SYSTEM_ENTRY_OFFSET (FBE_DATABASE_SYSTEM_DB_LAYOUT_SYSTEM_SPARE_TABLE_OFFSET + FBE_DATABASE_SYSTEM_DB_LAYOUT_SYSTEM_SPARE_TABLE_SIZE)

typedef enum fbe_database_system_db_layout_region_id_e {
    FBE_DATABASE_SYSTEM_DB_LAYOUT_HEADER,
    FBE_DATABASE_SYSTEM_DB_LAYOUT_SEMV_INFO,
    FBE_DATABASE_SYSTEM_DB_RAW_MIRROR_STATE,
    FBE_DATABASE_SYSTEM_DB_LAYOUT_SYSTEM_OBJECT_OP_FLAGS,
    FBE_DATABASE_SYSTEM_DB_LAYOUT_SYSTEM_SPARE_TABLE,
    FBE_DATABASE_SYSTEM_DB_LAYOUT_ENTRY,
    FBE_DATABASE_SYSTEM_DB_LAYOUT_JOURNAL,
    FBE_DATABASE_SYSTEM_DB_LAYOUT_MAX_REGIONS
}fbe_database_system_db_layout_region_id_t;


/*! @struct fbe_database_system_db_space_layout_region_t 
 * @brief Descripes the system db space layout region
 */
#define FBE_DATABASE_SYSTEM_DB_SL_MAX_REGION_NAME 64
typedef struct fbe_database_system_db_layout_region_s {
    fbe_database_system_db_layout_region_id_t region_id;
    fbe_char_t  region_name[FBE_DATABASE_SYSTEM_DB_SL_MAX_REGION_NAME];
    fbe_lba_t   starting_block_address;
    fbe_lba_t   size_in_blocks;
}fbe_database_system_db_layout_region_t;

/*! @struct fbe_database_system_db_space_layout_t 
 * @brief Descripes the system db space layout
 */
struct fbe_database_system_db_space_layout_s {
    fbe_database_system_db_layout_region_t regions[FBE_DATABASE_SYSTEM_DB_LAYOUT_MAX_REGIONS];
};

typedef fbe_u64_t fbe_database_system_db_persist_handle_t;

/*control block to handle raw-way-mirror IO errors*/
#define DATABASE_RAW_MIRROR_DRIVE_NUM                    3
#define DATABASE_RAW_MIRROR_BITMAP_BYTE_SIZE             8
#define DATABASE_RAW_MIRROR_READ_RETRY_COUNT             3

/*booktracking structures to maintain database raw-mirror rebuild state 
  in memory and disk.  this data structure will persist to disk.*/
#pragma pack(1)
typedef struct fbe_database_raw_mirror_drive_state_s{
    fbe_u32_t   raw_mirror_drive_nr_bitmap;         /*bitmap to identify which raw-mirror drive need rebuild*/
} fbe_database_raw_mirror_drive_state_t;
#pragma pack()

typedef struct fbe_database_raw_mirror_io_cb_s{

    /*total block numbers in database raw-3way-mirror*/
    fbe_block_count_t block_count;       
	
#if 1
    /*how many blocks flushed per IO*/
    fbe_u32_t           flush_unit;                                    

    /*statistics for error handling; after a read or write 
      the error_count == flushed_error_count + failed_flush_count*/
    fbe_u64_t           error_count;
    fbe_u64_t           flushed_error_count;
    fbe_u64_t           failed_flush_count;

    /*a new disk ready notification will increase this counter; it determines how 
      many rounds the background flush thread need scan the database raw-mirror and
      rebuild it*/
    fbe_u32_t           background_scan_round;

    /*define the maximum DB drive number in the platform*/
    fbe_u32_t  max_db_drive_num;

    /*it indicates which raw-mirror drive is detected error but flushed failed; 
      need further actions to fix the error*/
    fbe_u32_t  degradation_bitmap;

    /*the bitmap indicates which raw-mirror drive is down now; disk fail 
      and ready notification will set it*/
    fbe_u32_t  down_disk_bitmap;

    /*the bitmap indicates which disk needs to be rebuilt 
      if it is ready*/
    fbe_u32_t  nr_disk_bitmap;

    /*flag indicate if physical package notification registered*/
    fbe_bool_t   notification_registered;
#endif
    /*it is an array to store the sequence number of each block of database raw-mirror*/
    fbe_u64_t* seq_numbers;

} fbe_database_raw_mirror_io_cb_t;

/* use for read/write interface as the arguments */
typedef struct database_raw_mirror_operation_cb_s {
    /*The raw-3way-mirror LBA and count need read/write*/
    fbe_lba_t           lba; 
    fbe_block_count_t   block_count;  

    /*drive flags for raw-mirror IO*/
    fbe_u32_t           read_drive_mask;
    fbe_u32_t           write_drive_mask;

    /*the sg list IO is for*/
    fbe_sg_element_t    *sg_list;
    fbe_u32_t           sg_list_count;

    /*this semaphore is used to sync with read/write and complete function; 
      will be re-used in the read-write/verify or write-write/verify cases*/
    fbe_semaphore_t     semaphore;

    /*control fields or flags for IO error handling*/

    /*set this flag be TRUE if we need flush it when detecting an error*/
    fbe_bool_t          flush_error;

    /*indicating if flush operation is done successfully*/
    fbe_bool_t          flush_done;

    /*indicate an error detected in the read or write, need flush it*/
    fbe_bool_t          need_flush;

    /*flush the raw-mirror no matter if inconsistency detected*/
    fbe_bool_t          force_flush;

    /*indicate if the read/write block operation successful*/
    fbe_bool_t          block_operation_successful;
    /*indicate if the read/write block operation need retry*/
    fbe_bool_t          operation_need_retry;


    /*indicating which disk has magic number or sequence number error in this IO*/
    fbe_u32_t           error_disk_bitmap;

    /*how many times we need retry the flush if one flush operation failed, 
      always set to 2*/
    fbe_u32_t           flush_retry_count;

    /*error verify report structure passed to raid library to report errors*/
    fbe_raid_verify_raw_mirror_errors_t error_verify_report;

} database_raw_mirror_operation_cb_t;

typedef enum{
    FBE_DATABASE_RAW_MIRROR_DRIVE_INVALID_EVENT = 0,
    FBE_DATABASE_RAW_MIRROR_DRIVE_FAIL,          /*the drive failed*/
    FBE_DATABASE_RAW_MIRROR_DRIVE_READY,         /*the drive ready*/
    FBE_DATABASE_RAW_MIRROR_DRIVE_DEGRADATION
} fbe_database_raw_mirror_drive_events_t;

/* system_db interfaces */
fbe_status_t database_system_db_interface_init(void);
fbe_status_t database_system_db_interface_clear(void);
fbe_status_t database_system_db_persist_init(fbe_u32_t valid_db_drive_mask);
fbe_status_t database_system_db_get_lba_by_region(fbe_database_system_db_layout_region_id_t region_id, fbe_lba_t *lba);
fbe_status_t database_system_db_interface_read(fbe_u8_t *data_ptr, fbe_lba_t start_lba, fbe_u64_t size, fbe_u32_t read_drive_mask, fbe_u64_t *done_size);
fbe_status_t database_system_db_interface_general_read(fbe_u8_t *data_ptr, fbe_lba_t start_lba, fbe_u64_t count, 
                                             fbe_u32_t data_block_size, fbe_u64_t *done_count,
                                             fbe_bool_t is_read_flush,
                                             fbe_u32_t  read_mask, fbe_u32_t write_mask);
fbe_status_t database_system_db_interface_write(fbe_u8_t *data_ptr, fbe_lba_t start_lba, fbe_u64_t size, fbe_u64_t *done_size);
fbe_status_t database_system_db_interface_general_write(fbe_u8_t *data_ptr, fbe_lba_t start_lba, fbe_u64_t count, 
                                                        fbe_u64_t *done_count,
                                                        fbe_bool_t is_clear,
                                                        fbe_u32_t  write_mask);
fbe_status_t fbe_database_system_db_read_header(database_system_db_header_t *header, fbe_u32_t read_drive_mask);
fbe_status_t fbe_database_system_db_persist_header(database_system_db_header_t *header);
fbe_status_t database_system_db_interface_debug(void);
fbe_status_t database_system_db_get_entry_lba(fbe_db_system_entry_type_t type, fbe_object_id_t object_id, 
                                              fbe_edge_index_t index, fbe_lba_t *lba);

/*initialize the system db persist transaction and replay the journal if necessary*/
fbe_status_t fbe_database_system_db_persist_initialize(fbe_lba_t journal_header_lba, 
                                                       fbe_lba_t journal_data_lba,
                                                       fbe_u32_t  valid_db_drive_mask);

/* SAFEBUG - added to make sure we properly manage the lifecycle of system_db_per_transation.lock */
fbe_status_t fbe_database_system_db_persist_world_initialize(void);
fbe_status_t fbe_database_system_db_persist_world_teardown(void);

/*start a system db persist transaction*/
fbe_status_t fbe_database_system_db_start_persist_transaction(fbe_database_system_db_persist_handle_t *handle);
/*commit a system db persist transaction*/
fbe_status_t fbe_database_system_db_persist_commit(fbe_database_system_db_persist_handle_t handle);
/*abort a system db persist transaction*/
fbe_status_t fbe_database_system_db_persist_abort(fbe_database_system_db_persist_handle_t handle);
/*persist an object entry inside a transaction*/
fbe_status_t fbe_database_system_db_persist_object_entry(fbe_database_system_db_persist_handle_t handle,
                                                         fbe_database_system_db_persist_type_t type, database_object_entry_t *entry);
/*persist a user entry inside a transaction*/
fbe_status_t fbe_database_system_db_persist_user_entry(fbe_database_system_db_persist_handle_t handle,
                                                       fbe_database_system_db_persist_type_t type, database_user_entry_t *entry);
/*persist an edge entry inside a transaction*/
fbe_status_t fbe_database_system_db_persist_edge_entry(fbe_database_system_db_persist_handle_t handle,
                                                       fbe_database_system_db_persist_type_t type, database_edge_entry_t *entry);

/*interfaces to read/write system database entry from/to the disk directly*/
fbe_status_t fbe_database_system_db_read_object_entry(fbe_object_id_t object_id, database_object_entry_t *obj_entry);
fbe_status_t fbe_database_system_db_read_user_entry(fbe_object_id_t object_id, database_user_entry_t *user_entry);
fbe_status_t fbe_database_system_db_read_edge_entry(fbe_object_id_t object_id, fbe_edge_index_t client_index, database_edge_entry_t *edge_entry);
fbe_status_t fbe_database_system_db_read_system_spare_entry(fbe_object_id_t object_id, database_system_spare_entry_t *entry);
fbe_status_t fbe_database_system_db_persist_system_spare_entry(fbe_database_system_db_persist_type_t type, database_system_spare_entry_t *entry);

fbe_status_t fbe_database_system_db_raw_persist_object_entry(fbe_database_system_db_persist_type_t type, database_object_entry_t *entry);
fbe_status_t fbe_database_system_db_raw_persist_user_entry(fbe_database_system_db_persist_type_t type, database_user_entry_t *entry);
fbe_status_t fbe_database_system_db_raw_persist_edge_entry(fbe_database_system_db_persist_type_t type, database_edge_entry_t *entry);

fbe_status_t system_db_rw_verify(void);

fbe_status_t database_util_raw_mirror_write_verify_blocks(database_raw_mirror_operation_cb_t *raw_mirr_op_cb);
fbe_status_t database_util_raw_mirror_read_blocks(fbe_packet_t *packet, 
                                                  database_raw_mirror_operation_cb_t *raw_mirr_op_cb);
fbe_status_t fbe_database_raw_mirror_io_cb_init(fbe_database_raw_mirror_io_cb_t *rw_mirr_cb);

fbe_status_t database_utils_raw_mirror_general_read(fbe_u8_t *data_ptr, fbe_lba_t start_lba, fbe_u32_t count, fbe_u32_t valid_db_drive_mask);

fbe_status_t fbe_database_raw_mirror_io_cb_destroy(fbe_database_raw_mirror_io_cb_t *rw_mirr_cb);

void fbe_database_system_db_destroy(void);

void fbe_database_metadata_raw_mirror_destroy(void);

fbe_status_t database_util_raw_mirror_write_blocks(fbe_packet_t *packet, 
                                                  database_raw_mirror_operation_cb_t *raw_mirr_op_cb);

fbe_status_t fbe_database_util_raw_mirror_io_get_seq(fbe_lba_t lba, fbe_bool_t inc,
                                                  fbe_u64_t *seq);

fbe_status_t fbe_database_util_raw_mirror_get_error_report(fbe_database_control_raw_mirror_err_report_t *report);

fbe_status_t database_util_raw_mirror_check_and_flush(fbe_database_raw_mirror_io_cb_t *raw_mirr_io_cb);

fbe_u32_t fbe_database_system_get_all_raw_mirror_drive_mask(void);

fbe_status_t fbe_database_system_db_read_semv_info(fbe_database_emv_t* semv_info, fbe_u32_t read_drive_mask);
fbe_status_t fbe_database_system_db_persist_semv_info(fbe_database_emv_t *semv_info);

fbe_status_t fbe_database_raw_mirror_drive_state_initialize(void);

fbe_status_t fbe_database_raw_mirror_set_new_drive(fbe_u32_t new_drives);

fbe_status_t fbe_database_raw_mirror_get_valid_drives(fbe_u32_t *valid_drives);

fbe_status_t fbe_database_read_system_object_recreate_flags(fbe_database_system_object_recreate_flags_t *system_object_op_flags, fbe_u32_t read_drive_mask);
fbe_status_t fbe_database_persist_system_object_recreate_flags(fbe_database_system_object_recreate_flags_t *system_object_op_flags);

fbe_status_t fbe_database_raw_mirror_init_block_seq_number(fbe_u32_t valid_drives);

#endif /* end FBE_DATABASE_SYSTEM_DB_INTERFACE_H */
