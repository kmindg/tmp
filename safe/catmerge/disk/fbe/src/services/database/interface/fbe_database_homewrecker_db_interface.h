#ifndef FBE_DATABASE_HOMEWRECKER_DB_INTERFACE_H
#define FBE_DATABASE_HOMEWRECKER_DB_INTERFACE_H
 /* All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_database_homewrecker_db_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains local definitions for the database homewrecker db interface.
 *
 * @version
 *   04/20/2012:  Created. 
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe_private_space_layout.h"
#include "fbe_database.h"

/* homewrecker db raw_mirror operation status return to caller via this */
typedef struct database_homewrecker_db_raw_mirror_operation_status_s {
    /*indicate if the read/write block operation successful*/
    fbe_bool_t          block_operation_successful;

    /*indicating which disk has magic number or sequence number error in this IO*/
    fbe_u32_t           error_disk_bitmap;

    /*error verify report structure passed to raid library to report errors*/
    fbe_raid_verify_raw_mirror_errors_t error_verify_report;

} database_homewrecker_db_raw_mirror_operation_status_t;

/*the priority of memory request for system db IO operations*/
#define DATABASE_HOMEWRECKER_DB_MEM_REQ_PRIORITY        1

/*define the valid sequence number region for Homewrecker raw-mirror blocks use.
  avoid use the pure 0 or pure 1*/
#define HOMEWRECKER_DB_RAW_MIRROR_START_SEQ              ((fbe_u64_t)0xbeefdaed)
#define HOMEWRECKER_DB_RAW_MIRROR_END_SEQ                ((fbe_u64_t)0xFFFFFFFFFFFFdead)


/* Offset and size of c4mirror cache in LBA */
#define HOMEWRECKER_DB_C4_MIRROR_OFFSET          (64 * 1024 / FBE_BYTES_PER_BLOCK)
#define HOMEWRECKER_DB_C4_MIRROR_SIZE            ((512 * 1024 / FBE_BYTES_PER_BLOCK) - HOMEWRECKER_DB_C4_MIRROR_OFFSET)
#define HOMEWRECKER_DB_C4_MIRROR_OBJ_SIZE        (4 * 1024 / FBE_BYTES_PER_BLOCK)

/*offet and size of c4mirror nonpaged metadata*/
#define HOMEWRECKER_DB_C4_MIRROR_NONPAGED_START_LBA       (512 * 1024 / FBE_BYTES_PER_BLOCK)
#define HOMEWRECKER_DB_C4_MIRROR_NONPAGED_BLOCK_COUNT     (512 * 1024 / FBE_BYTES_PER_BLOCK)

fbe_status_t database_homewrecker_db_interface_init(fbe_database_service_t* db_service);

fbe_status_t database_homewrecker_get_system_pvd_edge_state(fbe_u32_t pvd_index,
                                                                fbe_bool_t* path_enable);


fbe_status_t database_homewrecker_db_interface_init_raw_mirror_block_seq(void);

fbe_status_t database_homewrecker_db_interface_general_read
                                                        (fbe_u8_t *data_ptr, fbe_lba_t start_lba, 
                                                         fbe_u64_t count, fbe_u64_t *done_count, 
                                                         fbe_u16_t drive_mask,
                                                         database_homewrecker_db_raw_mirror_operation_status_t * operation_stauts);

static fbe_status_t database_homewrecker_db_raw_mirror_blocks_read_completion
                                                        (fbe_packet_t * packet, fbe_packet_completion_context_t context);

fbe_status_t database_homewrecker_db_interface_general_write
                                                        (fbe_u8_t *data_ptr, fbe_lba_t start_lba, fbe_u64_t count, 
                                                         fbe_u64_t *done_count, fbe_bool_t is_clear_write, 
                                                         database_homewrecker_db_raw_mirror_operation_status_t * operation_status);

static fbe_status_t
database_homewrecker_db_interface_mem_allocation_completion
                                                        (fbe_memory_request_t * memory_request, 
                                                         fbe_memory_completion_context_t context);

static fbe_status_t
database_homewrecker_db_interface_db_calculate_checksum
                                                        (void * block_p, fbe_bool_t seq_reset);

fbe_status_t fbe_homewrecker_db_util_raw_mirror_io_get_seq
                                                        (fbe_bool_t reset, fbe_u64_t *seq);

fbe_status_t fbe_homewrecker_db_util_get_raw_mirror_block_seq
                                                        (fbe_u8_t * block, fbe_u64_t * seq);

static fbe_status_t 
database_homewrecker_db_raw_mirror_blocks_write_completion
                                                        (fbe_packet_t * packet, fbe_packet_completion_context_t context);

void fbe_database_homewrecker_db_destroy(void);

fbe_status_t database_homewrecker_db_interface_clear(void);

#endif /* end FBE_DATABASE_HOMEWRECKER_DB_INTERFACE_H */





