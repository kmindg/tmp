#ifndef FBE_DISK_BLOCK_CORRECT_H
#define FBE_DISK_BLOCK_CORRECT_H

#include "fbe/fbe_api_common.h"
#include "fbe/fbe_package.h"
#include "fbe_trace.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_time.h"
#include <time.h>
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "EmcUTIL_Device.h"
#include "k10defs.h"

#define NUM_VOL_PARAM_SPARES 8
#define FBE_BAD_BLOCKS_COMMAND_REPORT   0x01    /* command will create a report from the event log */
#define FBE_BAD_BLOCKS_COMMAND_CLEAN    0x02    /* command will zero uncorrectable sectors */

#define FBE_BAD_BLOCKS_CONSOLE_MAX_CHAR 128

#define FBE_BAD_BLOCKS_DEVICE_PREFIX "CLARiiONdisk"
#define MAX_BUFFER_SIZEOF 4096
#define FBE_DISK_BLOCK_CORRECT_BYTES_PER_BLOCK 512
#define FBE_DISK_BLOCK_CORRECT_RECORDS_MAX 100

#define FBE_BAD_BLOCKS_EVENT_MSG "Data Sector Invalidated RAID Group: 0x%x Position: 0x%x LBA: 0x%llx Blocks: 0x%x Error info: 0x%x Extra info: 0x%x"
#define FBE_BAD_BLOCKS_EVENT_MSG_NUM_FIELDS 6

#define SYM_LINK_NAME  "DiskBlkCorrectDev"
#define DOS_DEV_NAME   "\\\\.\\DiskBlkCorrectDev"

typedef struct fbe_bad_blocks_cmd_s {
    fbe_u32_t command;
    fbe_bool_t is_sim;
    fbe_char_t * file;
    time_t startdate;
    time_t enddate;
    fbe_char_t * description;
} fbe_bad_blocks_cmd_t;

typedef struct fbe_disk_block_correct_event_record_s {
    fbe_object_id_t raid_group_id;
    fbe_u16_t position;
    fbe_lba_t disk_pba;
    fbe_u32_t blocks;
} fbe_disk_block_correct_event_record_t;

typedef struct fbe_disk_block_correct_record_s {
    fbe_object_id_t             lun_object_id;
    fbe_lba_t                   lun_lba;
    fbe_lun_number_t 			lun_number;
    fbe_assigned_wwid_t         world_wide_name; 
    fbe_lba_t                   offset;
    fbe_object_id_t             rg_object_id;
    fbe_raid_group_number_t 	rg_number;
    fbe_u16_t                   position;
    fbe_u32_t                   blocks;
} fbe_disk_block_correct_record_t;

/* General functions for disk block correct
 */
fbe_status_t fbe_disk_block_correct_initialize(fbe_bad_blocks_cmd_t * bb_command);
void __cdecl fbe_disk_block_correct_destroy_fbe_api(fbe_bad_blocks_cmd_t * bb_command);
fbe_status_t fbe_get_package_id(fbe_package_id_t * package_id);

fbe_status_t fbe_disk_block_correct_execute(fbe_bad_blocks_cmd_t * bb_command);
fbe_status_t fbe_disk_block_correct_report(fbe_bad_blocks_cmd_t * bb_command);
fbe_status_t fbe_disk_block_correct_add_record(fbe_object_id_t rg_object_id,
                                               fbe_u16_t position,
                                               fbe_lba_t pba, 
                                               fbe_u32_t blocks);
void fbe_disk_block_correct_add_lun_record(fbe_database_lun_info_t * lun_info,
                                           fbe_lba_t lba,
                                           fbe_u16_t position,
                                           fbe_u32_t blocks); 
fbe_status_t fbe_disk_block_correct_get_upstream_objects(fbe_object_id_t * rg_object_id,
                                                         fbe_api_base_config_upstream_object_list_t * upstream_object_list,
                                                         fbe_u16_t * position);
void fbe_disk_block_correct_clean_lun_record(fbe_disk_block_correct_record_t * record);
void fbe_disk_block_correct_clean_record(fbe_disk_block_correct_record_t * record);
void fbe_disk_block_correct_clean(void);

/* Helper functions */
void fbe_disk_block_correct_get_wwn_string(fbe_assigned_wwid_t world_wide_name, fbe_char_t * wwn);
fbe_status_t fbe_disk_block_correct_get_arraysn(fbe_char_t * sn);
fbe_bool_t fbe_disk_block_correct_is_within_dates(fbe_bad_blocks_cmd_t * bb_command, fbe_system_time_t * timestamp);
time_t fbe_disk_block_correct_string_to_date(fbe_char_t * date);

/* Report file functions */
void fbe_disk_block_correct_print_output(fbe_char_t * filename);
void fbe_disk_block_correct_read_records_from_file(fbe_char_t * filename);
fbe_status_t fbe_disk_block_correct_get_write_file_handle(fbe_char_t * filename, FILE ** filehandle);
fbe_status_t fbe_disk_block_correct_get_read_file_handle(fbe_char_t * filename, FILE ** filehandle);
fbe_status_t fbe_disk_block_correct_close_file(FILE * fp); 

/* Lun Device functions */
fbe_status_t fbe_disk_block_correct_open_lun_device(fbe_char_t * lunDeviceName, EMCUTIL_RDEVICE_REFERENCE * lunDeviceHandle);
void fbe_disk_block_correct_close_lun_device(EMCUTIL_RDEVICE_REFERENCE lunDeviceHandle);
fbe_status_t fbe_disk_block_correct_read_block(EMCUTIL_RDEVICE_REFERENCE lunDeviceHandle, fbe_lba_t lun_lba);
void fbe_disk_block_correct_write_block(EMCUTIL_RDEVICE_REFERENCE lunDeviceHandle, fbe_lba_t lun_lba);


/* Functions for simulation
 */
fbe_status_t fbe_disk_block_correct_initialize_sim(fbe_bad_blocks_cmd_t * bb_command);
static fbe_u32_t fbe_disk_block_correct_get_target_sp(char spstr);
void __cdecl fbe_disk_block_correct_destroy_fbe_api_sim(fbe_u32_t in);


/* Functions for hardware
 */
fbe_status_t fbe_disk_block_correct_initialize_user(void);
void __cdecl fbe_disk_block_correct_destroy_fbe_api_user(void);

#endif /* FBE_CLI_H */
