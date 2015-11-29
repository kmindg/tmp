/***************************************************************************
 * Copyright (C) EMC Corporation 2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file daffy_duck_test.c
 ***************************************************************************
 *
 * @brief
 *  This test ensures that degraded I/O during encryption, which is journaled
 *  is read and written with the appropriate key.
 *
 * @version
 *   5/23/2014 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_encryption_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "sep_tests.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_random.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "sep_test_io.h"
#include "pp_utils.h"
#include "fbe/fbe_api_sep_io_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_test_common_utils.h"
#include "sep_test_region_io.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe/fbe_api_system_bg_service_interface.h"
#include "fbe/fbe_api_panic_sp_interface.h"
#include "sep_hook.h"
/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * daffy_duck_short_desc = "Test journaled I/O during encryption";
char * daffy_duck_long_desc = "\
This test has three parts, all of which run during encryption:  \n\
\n\
- Cause a journal verify to occur an ensure we use the correct key.\n\
- Cause a journal rebuild to occur an ensure we use the correct key.\n\
- Cause journaled I/O during encryption both above and below the\n\
encryption checkpoint.\n";

enum daffy_duck_defines_e {
    DAFFY_DUCK_MAX_RAID_GROUPS = 5,
    DAFFY_DUCK_TEST_LUNS_PER_RAID_GROUP = 3,

    DAFFY_DUCK_MAX_IO_SIZE_BLOCKS = (FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1) * FBE_RAID_MAX_BE_XFER_SIZE, //4*0x400*6

    DAFFY_DUCK_CHUNKS_PER_LUN = 5,
    
    DAFFY_DUCK_ENCRYPTION_HALT_LBA = (FBE_RAID_DEFAULT_CHUNK_SIZE * (DAFFY_DUCK_CHUNKS_PER_LUN + (DAFFY_DUCK_CHUNKS_PER_LUN/2))),
};
/*!*******************************************************************
 * @var daffy_duck_test_contexts
 *********************************************************************
 * @brief This contains our context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t daffy_duck_test_contexts[DAFFY_DUCK_TEST_LUNS_PER_RAID_GROUP * 2];

/*!*******************************************************************
 * @var daffy_duck_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t daffy_duck_raid_group_config[] = 
{
    /* width,   capacity    raid type,                  class,               block size     RAID-id.    bandwidth.*/
    {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            0,          0},
    {9,       0xE000,     FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,   520,            1,          0},
    {6,       0xE000,     FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            2,          0},

    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/**************************************
 * Local Static Functions.
 **************************************/


fbe_u32_t fbe_test_get_next_available_position(fbe_raid_position_bitmask_t used_bitmask,
                                               fbe_u32_t width)
{
    fbe_u32_t index;

    for (index = 0; index < width; index++) {
        if (((1 << index) & used_bitmask) == 0) {
            return index;
        }
    }
    return FBE_U32_MAX;
}
/*!*************************************************
 * @typedef daffy_duck_error_case_t
 ***************************************************
 * @brief Single test case with the error and
 *        the blocks to read for.
 *
 ***************************************************/
typedef struct daffy_duck_error_case_s
{
    fbe_lba_t lba;
    fbe_lba_t pba;
    fbe_raid_position_t error_pos;
    fbe_u32_t num_drives_to_remove;
    fbe_u32_t fault_pos[4]; /* 2 degraded drives for R6 plus one more to shutdown and terminator. */
    fbe_bool_t b_before_encryption_chkpt;
    fbe_api_logical_error_injection_type_t err_type;
}
daffy_duck_error_case_t;
/*!*******************************************************************
 * @var daffy_duck_timeout_error_template
 *********************************************************************
 * @brief This is the basis for our error record.
 *        All the error records we create start with this template.
 *
 *********************************************************************/
fbe_api_logical_error_injection_record_t daffy_duck_timeout_error_template =
{ FBE_RAID_MAX_DISK_ARRAY_WIDTH,    /* pos to inject error on */
    0x10,    /* width */
    FBE_U32_MAX,    /* lba */
    FBE_U32_MAX,    /* max number of blocks */
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID,    /* error type */
    FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,    /* error mode */
    0x0,    /* error count */
    0x15,    /* error limit */
    0x0,    /* skip count */
    0x15,    /* skip limit */
    0x0,    /* error adjacency */
    0x0,    /* start bit */
    0x0,    /* number of bits */
    0x0,    /* erroneous bits */
    0x0,    /* crc detectable */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode */
};  
static daffy_duck_error_case_t daffy_duck_error_cases[DAFFY_DUCK_MAX_RAID_GROUPS];
/*!**************************************************************
 * daffy_duck_setup_error_record()
 ****************************************************************
 * @brief
 *  Enable injection across all RGs.
 *  Different RGs have different error records.
 *
 * @param rg_config_p
 * @param inject_rg_object_id_p
 *
 * @author
 *  5/23/2014 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t daffy_duck_setup_error_record(daffy_duck_error_case_t *error_record_p,
                                                  fbe_test_rg_configuration_t *rg_config_p,
                                                  fbe_bool_t b_encrypted)
{   
    fbe_status_t status;
    fbe_api_raid_group_get_info_t info;
    fbe_raid_group_map_info_t map_info;
    fbe_u32_t rg_index;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    fbe_object_id_t rg_object_id;
    fbe_raid_position_bitmask_t used_bitmask = 0;

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        fbe_lba_t offset;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }

        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Determine first paged lba.
         */
        status = fbe_api_raid_group_get_info(rg_object_id, &info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        fbe_zero_memory(&map_info, sizeof(fbe_raid_group_map_info_t));
        map_info.lba = 0;
        status = fbe_api_raid_group_map_lba(rg_object_id, &map_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        offset = map_info.offset;
        fbe_zero_memory(&map_info, sizeof(fbe_raid_group_map_info_t));
        if (b_encrypted) {
            map_info.pba = 0;
        } else {
            map_info.pba = info.rekey_checkpoint;  
        }
        map_info.pba += offset;
        status = fbe_api_raid_group_map_pba(rg_object_id, &map_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Take off the addr offset since we're injecting just below RAID.*/
        error_record_p->pba = map_info.pba - map_info.offset;
        error_record_p->error_pos = map_info.data_pos;
        error_record_p->lba = map_info.lba;
        error_record_p->err_type = FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIMEOUT_ERROR;

        /* The drives we fault cannot be 
         *   - the error position, where we will run I/O and inject an error to shutdown the RG.
         *   - the parity position.  If we do an I/O where we are only writing to the data position, we don't journal.
         */
        used_bitmask = (1 << error_record_p->error_pos);
        used_bitmask |= (1 << map_info.parity_pos);
        error_record_p->fault_pos[0] = fbe_test_get_next_available_position(used_bitmask, current_rg_config_p->width);
        used_bitmask |= (1 <<error_record_p->fault_pos[0]);

        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6) {
            error_record_p->fault_pos[1] = fbe_test_get_next_available_position(used_bitmask, current_rg_config_p->width);
            error_record_p->num_drives_to_remove = 2;
            error_record_p->fault_pos[2] = error_record_p->error_pos;
            error_record_p->fault_pos[3] = FBE_U32_MAX;
        } else {
            error_record_p->num_drives_to_remove = 1;
            error_record_p->fault_pos[1] = error_record_p->error_pos;
            error_record_p->fault_pos[2] = FBE_U32_MAX;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== rg object: 0x%x pba: 0x%llx lba: 0x%llx error_pos: %u fault 0/1: %u/%u parity: %u",
                   rg_object_id, 
                   error_record_p->pba, error_record_p->lba,
                   error_record_p->error_pos, error_record_p->fault_pos[0], error_record_p->fault_pos[1],
                   map_info.parity_pos);
        current_rg_config_p++;
        error_record_p++;
    }

    return status;
}
/******************************************
 * end daffy_duck_setup_error_record()
 ******************************************/
/*!**************************************************************
 * daffy_duck_enable_logical_error_injection()
 ****************************************************************
 * @brief
 *  Enable injection across all RGs.
 *  Different RGs have different error records.
 *
 * @param rg_config_p
 * @param inject_rg_object_id_p
 *
 * @author
 *  5/23/2014 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t daffy_duck_enable_logical_error_injection(daffy_duck_error_case_t *error_record_p,
                                                              fbe_test_rg_configuration_t *rg_config_p,
                                                              fbe_object_id_t *inject_rg_object_id_p)
{   
    fbe_status_t status;
    fbe_api_logical_error_injection_record_t record;
    fbe_u32_t rg_index;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    /* Since we purposefully injecting errors, only trace those sectors that 
     * result `critical' (i.e. for instance error injection mis-matches) errors.
     */
    fbe_test_sep_util_reduce_sector_trace_level();

    /* Disable all the records.
     */
    status = fbe_api_logical_error_injection_disable_records(0, FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    fbe_copy_memory(&record, &daffy_duck_timeout_error_template, sizeof(fbe_api_logical_error_injection_record_t));

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }

        record.pos_bitmap = (1 << error_record_p->error_pos);
        record.lba = error_record_p->pba;
        record.blocks = 1;
        record.err_type = error_record_p->err_type;
        record.err_adj = record.pos_bitmap;
        
        status = fbe_api_logical_error_injection_create_object_record(&record, inject_rg_object_id_p[rg_index]);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, "== create record for rg_object_id: 0x%x pos_bitmap: 0x%x lba: 0x%llx blocks: 0x%llx etype: 0x%x",
                   inject_rg_object_id_p[rg_index], record.pos_bitmap, record.lba, record.blocks, record.err_type);

        /* Enable injection on the RG.
         */
        status = fbe_api_logical_error_injection_enable_object(inject_rg_object_id_p[rg_index], FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        current_rg_config_p++;
        error_record_p++;
    }
    /* Enable injection overall.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Enable error injection ==");
    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return status;
}
/******************************************
 * end daffy_duck_enable_logical_error_injection()
 ******************************************/
/*!**************************************************************
 * daffy_duck_setup_error_record_for_media_error()
 ****************************************************************
 * @brief
 *  Enable injection across all RGs.
 *  Different RGs have different error records.
 *
 * @param error_record_p
 * @param rg_config_p
 *
 * @author
 *  5/23/2014 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t daffy_duck_setup_error_record_for_media_error(daffy_duck_error_case_t *error_record_p,
                                                                  fbe_test_rg_configuration_t *rg_config_p)
{   
    fbe_status_t status;
    fbe_api_raid_group_get_info_t info;
    fbe_u32_t rg_index;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    fbe_object_id_t rg_object_id;

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }

        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Determine first paged lba.
         */
        status = fbe_api_raid_group_get_info(rg_object_id, &info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Take off the addr offset since we're injecting just below RAID.*/
        error_record_p->pba = info.write_log_start_pba;
        error_record_p->error_pos = 0;
        error_record_p->lba = FBE_LBA_INVALID;
        error_record_p->err_type = FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SOFT_MEDIA_ERROR;

        mut_printf(MUT_LOG_TEST_STATUS, "== rg object: 0x%x pba: 0x%llx  error_pos: %u",
                   rg_object_id, error_record_p->pba, error_record_p->error_pos);
        current_rg_config_p++;
        error_record_p++;
    }

    return status;
}
/******************************************
 * end daffy_duck_setup_error_record_for_media_error()
 ******************************************/
/*!**************************************************************
 * daffy_duck_disable_error_injection()
 ****************************************************************
 * @brief
 *  Stop injecting errors across all RGs.
 *
 * @param rg_config_p
 * @param inject_rg_object_id_p
 * 
 * @return fbe_status_t
 *
 * @author
 *  5/23/2014 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t daffy_duck_disable_error_injection(fbe_test_rg_configuration_t *rg_config_p,
                                                       fbe_object_id_t *inject_rg_object_id_p)
{
    fbe_status_t status;
    fbe_api_logical_error_injection_get_stats_t stats, *stats_p = &stats;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t rg_index;

    /* Stop logical error injection.
     */
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_api_logical_error_injection_disable_object(inject_rg_object_id_p[rg_index], FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    status = fbe_api_logical_error_injection_get_stats(stats_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats_p->b_enabled, FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(stats_p->num_objects_enabled, 0);

    mut_printf(MUT_LOG_TEST_STATUS, "%s stats_p->num_objects %d, stats_p->num_objects_enabled %d", 
               __FUNCTION__, stats_p->num_objects, stats_p->num_objects_enabled);

    return status;
}
/******************************************
 * end daffy_duck_disable_error_injection()
 ******************************************/
static void fbe_test_sep_util_wait_rg_not_degraded(fbe_object_id_t object_id)
{
    fbe_status_t status;
    fbe_api_raid_group_get_info_t info;
    fbe_u32_t wait_time = 0;
    fbe_bool_t b_found;
    fbe_u32_t index;

    while (FBE_TRUE) {
        status = fbe_api_raid_group_get_info(object_id, &info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        b_found = FBE_FALSE;

        for (index = 0; index < info.width; index++) {
            if (info.rebuild_checkpoint[index] != FBE_LBA_INVALID) {
                b_found = FBE_TRUE;
                break;
            }
        }
        if (!b_found) {
            return;
        }
        if (wait_time > FBE_TEST_WAIT_TIMEOUT_MS) {
            mut_printf(MUT_LOG_TEST_STATUS, "== Timed out waiting for error injection after %u msec", wait_time);
            MUT_FAIL();
        }
        wait_time += 500;
        fbe_api_sleep(500);
    }
} 

static void daffy_duck_wait_error_injection(fbe_object_id_t object_id)
{
    fbe_status_t status;
    fbe_api_logical_error_injection_get_object_stats_t stats;
    fbe_u32_t wait_time = 0;

    while (FBE_TRUE) {
        status = fbe_api_logical_error_injection_get_object_stats(&stats, object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if (stats.num_errors_injected >= 1) {
            mut_printf(MUT_LOG_TEST_STATUS,"== object 0x%x found %llu errors injected ==",
                       object_id,
                       (unsigned long long)stats.num_errors_injected);
            break;
        }
        if (wait_time > FBE_TEST_WAIT_TIMEOUT_MS) {
            mut_printf(MUT_LOG_TEST_STATUS, "== Timed out waiting for error injection after %u msec", wait_time);
            MUT_FAIL();
        }
        wait_time += 500;
        fbe_api_sleep(500);
    }
} 
void daffy_duck_start_encryption(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, DAFFY_DUCK_ENCRYPTION_HALT_LBA, FBE_TEST_HOOK_ACTION_ADD);
    
    // Add hook for encryption to complete and enable KMS
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, DAFFY_DUCK_ENCRYPTION_HALT_LBA, FBE_TEST_HOOK_ACTION_WAIT);
    mut_printf(MUT_LOG_TEST_STATUS, "== Starting Encryption. ==");

}

void daffy_duck_complete_encryption(fbe_test_rg_configuration_t *rg_config_p)
{
    mut_printf(MUT_LOG_TEST_STATUS, "== Remove hooks so encryption can proceed. ==");

    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, DAFFY_DUCK_ENCRYPTION_HALT_LBA, FBE_TEST_HOOK_ACTION_WAIT);
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, DAFFY_DUCK_ENCRYPTION_HALT_LBA, FBE_TEST_HOOK_ACTION_DELETE);
    
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for Encryption. ==");
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE); /* KMS enabled */

}

/*!**************************************************************
 * daffy_duck_journal_rebuild_test_case()
 ****************************************************************
 * @brief
 *  Run a test where the journal rebuilds during encryption.
 *  Ensure the journal is rebuilt with correct key by faulting
 *  raid group while degraded on a different position and
 *  ensure there are no errors when we read the journal.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  5/27/2014 - Created. Rob Foley
 *
 ****************************************************************/

static void daffy_duck_journal_rebuild_test_case(fbe_test_rg_configuration_t * rg_config_p)
{
    fbe_status_t status;
    fbe_u32_t num_luns;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t num_raid_groups;
    fbe_u32_t rg_index;
    fbe_object_id_t rg_object_id;
    fbe_api_base_config_downstream_object_list_t downstream_object_list;
    fbe_sim_transport_connection_target_t peer_sp;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_object_id_t inject_rg_object_id[DAFFY_DUCK_MAX_RAID_GROUPS];
    fbe_u32_t num_drives_to_degrade;

    fbe_test_sep_get_active_target_id(&active_sp);
    peer_sp = fbe_test_sep_get_peer_target_id(active_sp);

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }

        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        /*MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

                status = fbe_api_raid_group_set_library_debug_flags(rg_object_id, 
                                                        (FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_TRACING | 
                                                         FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_TRACING |
                                                         FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_DATA_TRACING |
                                                         FBE_RAID_LIBRARY_DEBUG_FLAG_REBUILD_TRACING));

        fbe_test_sep_util_set_rg_debug_flags_both_sps(current_rg_config_p, 
                                                      FBE_RAID_GROUP_DEBUG_FLAG_IO_TRACING |
                                                      FBE_RAID_GROUP_DEBUG_FLAGS_IOTS_TRACING |
                                                      FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING |
                                                      FBE_RAID_GROUP_DEBUG_FLAGS_STRIPE_LOCK_TRACING |
                                                      FBE_RAID_GROUP_DEBUG_FLAG_QUIESCE_TRACING);*/

        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
            fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);
            if (downstream_object_list.number_of_downstream_objects == 0) {
                MUT_FAIL_MSG("number of ds objects is 0");
            }
            inject_rg_object_id[rg_index] = downstream_object_list.downstream_object_list[0];
        } else {
            inject_rg_object_id[rg_index] = rg_object_id;
        }
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "\n\n");
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==\n\n", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "== Remove drives ");
    big_bird_remove_all_drives(rg_config_p, num_raid_groups, /*rg count */ 1, /* Drive count */
                               FBE_FALSE, /* Yes we are pulling and reinserting the same drive*/
                               0, /* do not wait between removals */
                               FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for rebuild logging set on intentionally removed drives.==");
    big_bird_wait_for_rgs_rb_logging(rg_config_p, num_raid_groups, /* num rgs */
                                     1 /* Drives to wait for */);

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for swap in of permanent spare. ");
    big_bird_insert_all_drives(rg_config_p, num_raid_groups, 1, /* Number of drives to insert */
                               FBE_FALSE /* Yes we are pulling and reinserting the same drive*/);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for object: 0x%x to become ready. ", inject_rg_object_id[rg_index]);
        status = fbe_api_wait_for_object_lifecycle_state(inject_rg_object_id[rg_index], 
                                                         FBE_LIFECYCLE_STATE_READY, FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for %d rebuilds on RG obj %x", 1, inject_rg_object_id[rg_index]);
        big_bird_wait_for_rebuilds(current_rg_config_p, 1, /* Num RGs */ 1 /* Drives to rebuild */);

        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Make sure there were no raid errors.");
    fbe_test_sep_util_validate_no_raid_errors();

    mut_printf(MUT_LOG_TEST_STATUS, "== Remove drives to become degraded");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {

        fbe_u32_t drives_to_remove[3] = {1, 2, FBE_U32_MAX};
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        num_drives_to_degrade = (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6) ? 2 : 1;
        
        fbe_test_set_specific_drives_to_remove(current_rg_config_p, &drives_to_remove[0]);
        big_bird_remove_all_drives(current_rg_config_p, 1, /*rg count */ num_drives_to_degrade, /* Drive count */
                                   FBE_TRUE, /* Yes we are pulling and reinserting the same drive */
                                   0, /* do not wait between removals */
                                   FBE_DRIVE_REMOVAL_MODE_SPECIFIC);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for rebuild logging");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {

        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        num_drives_to_degrade = (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6) ? 2 : 1;
        big_bird_wait_for_rgs_rb_logging(current_rg_config_p, 1, /* num rgs */
                                         num_drives_to_degrade /* Drives to wait for */);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Remove drives to become broken");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {

        fbe_u32_t drives_to_remove[3] = {0, FBE_U32_MAX};
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        /* We already pulled N drive to make us degraded, now pull more to shutdown.
         */
        fbe_test_set_specific_drives_to_remove(current_rg_config_p, &drives_to_remove[0]);
        big_bird_remove_all_drives(current_rg_config_p, 1, /*rg count */ 1, /* Drive count */
                                   FBE_TRUE, /* Yes we are pulling and reinserting the same drive */
                                   0, /* do not wait between removals */
                                   FBE_DRIVE_REMOVAL_MODE_SPECIFIC);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for FAIL.==");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for object: 0x%x to become FAIL. ", inject_rg_object_id[rg_index]);
        status = fbe_api_wait_for_object_lifecycle_state(inject_rg_object_id[rg_index], 
                                                         FBE_LIFECYCLE_STATE_FAIL, FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Reinsert drives");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        num_drives_to_degrade = (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6) ? 2 : 1;
        big_bird_insert_all_drives(current_rg_config_p, 1, num_drives_to_degrade + 1, /* Number of drives to insert */
                                   FBE_TRUE /* Yes we are pulling and reinserting the same drive*/);

        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for object: 0x%x to become ready. ", inject_rg_object_id[rg_index]);
        status = fbe_api_wait_for_object_lifecycle_state(inject_rg_object_id[rg_index], 
                                                         FBE_LIFECYCLE_STATE_READY, FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for %d rebuilds on RG obj %x", 1, inject_rg_object_id[rg_index]);
        big_bird_wait_for_rebuilds(current_rg_config_p, 1, /* Num RGs */ 1 /* Drives to rebuild */);

        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== Make sure there were no raid errors.");
    fbe_test_sep_util_validate_no_raid_errors();

    /* We could have marked degraded on another position due to coming back from shutdown with a degraded drive.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for RG object %u not degraded", inject_rg_object_id[rg_index]);
        fbe_test_sep_util_wait_rg_not_degraded(inject_rg_object_id[rg_index]);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);

    return;
}
/******************************************
 * end daffy_duck_journal_rebuild_test_case()
 ******************************************/

/*!**************************************************************
 * daffy_duck_journal_verify_test_case()
 ****************************************************************
 * @brief
 *  Run a test where the journal verify runs during encryption.
 *  - Inject an error during journal read to cause journal verify.
 *  - Ensure the journal is rebuilt with correct key by faulting
 *  raid group while degraded on a different position and ensure
 *  there are no errors when we read the journal.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  5/27/2014 - Created. Rob Foley
 *
 ****************************************************************/

static void daffy_duck_journal_verify_test_case(fbe_test_rg_configuration_t * rg_config_p)
{
    daffy_duck_error_case_t *error_case_p = &daffy_duck_error_cases[0];
    fbe_status_t status;
    fbe_u32_t num_luns;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t num_raid_groups;
    fbe_u32_t rg_index;
    fbe_object_id_t rg_object_id;
    fbe_api_base_config_downstream_object_list_t downstream_object_list;
    fbe_sim_transport_connection_target_t peer_sp;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_object_id_t inject_rg_object_id[DAFFY_DUCK_MAX_RAID_GROUPS];
    fbe_u32_t num_drives_to_degrade;

    fbe_test_sep_get_active_target_id(&active_sp);
    peer_sp = fbe_test_sep_get_peer_target_id(active_sp);

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
            fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);
            if (downstream_object_list.number_of_downstream_objects == 0) {
                MUT_FAIL_MSG("number of ds objects is 0");
            }
            inject_rg_object_id[rg_index] = downstream_object_list.downstream_object_list[0];
        } else {
            inject_rg_object_id[rg_index] = rg_object_id;
        }
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "\n\n");
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==\n\n", __FUNCTION__);
    daffy_duck_setup_error_record_for_media_error(error_case_p, rg_config_p);

    mut_printf(MUT_LOG_TEST_STATUS, "== Remove drives to become degraded");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {

        fbe_u32_t drives_to_remove[3] = {1, 2, FBE_U32_MAX};
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        num_drives_to_degrade = (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6) ? 2 : 1;
        
        fbe_test_set_specific_drives_to_remove(current_rg_config_p, &drives_to_remove[0]);
        big_bird_remove_all_drives(current_rg_config_p, 1, /*rg count */ num_drives_to_degrade, /* Drive count */
                                   FBE_TRUE, /* Yes we are pulling and reinserting the same drive */
                                   0, /* do not wait between removals */
                                   FBE_DRIVE_REMOVAL_MODE_SPECIFIC);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for rebuild logging");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {

        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        num_drives_to_degrade = (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6) ? 2 : 1;
        big_bird_wait_for_rgs_rb_logging(current_rg_config_p, 1, /* num rgs */
                                         num_drives_to_degrade /* Drives to wait for */);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Remove drives to become broken");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {

        fbe_u32_t drives_to_remove[3] = {0, FBE_U32_MAX};
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        /* We already pulled N drive to make us degraded, now pull more to shutdown.
         */
        fbe_test_set_specific_drives_to_remove(current_rg_config_p, &drives_to_remove[0]);
        big_bird_remove_all_drives(current_rg_config_p, 1, /*rg count */ 1, /* Drive count */
                                   FBE_TRUE, /* Yes we are pulling and reinserting the same drive */
                                   0, /* do not wait between removals */
                                   FBE_DRIVE_REMOVAL_MODE_SPECIFIC);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for FAIL.==");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for object: 0x%x to become FAIL. ", inject_rg_object_id[rg_index]);
        status = fbe_api_wait_for_object_lifecycle_state(inject_rg_object_id[rg_index], 
                                                         FBE_LIFECYCLE_STATE_FAIL, FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Add error injection on journal.");
    daffy_duck_enable_logical_error_injection(error_case_p, rg_config_p, &inject_rg_object_id[0]);

    mut_printf(MUT_LOG_TEST_STATUS, "== Reinsert drives");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {

        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_REMAP, 
                                       FBE_RAID_GROUP_SUBSTATE_JOURNAL_REMAP_SLOTS_ALLOCATED,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_ADD);

        num_drives_to_degrade = (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6) ? 2 : 1;
        big_bird_insert_all_drives(current_rg_config_p, 1, num_drives_to_degrade + 1, /* Number of drives to insert */
                                   FBE_TRUE /* Yes we are pulling and reinserting the same drive*/);

        current_rg_config_p++;
    } 

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for hit of errors.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        daffy_duck_wait_error_injection(inject_rg_object_id[rg_index]);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Disable error injection.");
    daffy_duck_disable_error_injection(rg_config_p, &inject_rg_object_id[0]);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for object 0x%x to verify journal", inject_rg_object_id[rg_index]);
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_REMAP, 
                                       FBE_RAID_GROUP_SUBSTATE_JOURNAL_REMAP_SLOTS_ALLOCATED,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_WAIT_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_REMAP, 
                                       FBE_RAID_GROUP_SUBSTATE_JOURNAL_REMAP_SLOTS_ALLOCATED,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_DELETE_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for object: 0x%x to become ready. ", inject_rg_object_id[rg_index]);
        status = fbe_api_wait_for_object_lifecycle_state(inject_rg_object_id[rg_index], 
                                                         FBE_LIFECYCLE_STATE_READY, FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for %d rebuilds on RG obj %x", 1, inject_rg_object_id[rg_index]);
        big_bird_wait_for_rebuilds(current_rg_config_p, 1, /* Num RGs */ 1 /* Drives to rebuild */);

        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== Make sure there were no raid errors.");
    fbe_test_sep_util_validate_no_raid_errors();

    mut_printf(MUT_LOG_TEST_STATUS, "== Remove drives to become degraded again");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {

        fbe_u32_t drives_to_remove[3] = {1, 2, FBE_U32_MAX};
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        num_drives_to_degrade = (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6) ? 2 : 1;
        
        fbe_test_set_specific_drives_to_remove(current_rg_config_p, &drives_to_remove[0]);
        big_bird_remove_all_drives(current_rg_config_p, 1, /*rg count */ num_drives_to_degrade, /* Drive count */
                                   FBE_TRUE, /* Yes we are pulling and reinserting the same drive */
                                   0, /* do not wait between removals */
                                   FBE_DRIVE_REMOVAL_MODE_SPECIFIC);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for rebuild logging again");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {

        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        num_drives_to_degrade = (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6) ? 2 : 1;
        big_bird_wait_for_rgs_rb_logging(current_rg_config_p, 1, /* num rgs */
                                         num_drives_to_degrade /* Drives to wait for */);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Remove drives to become broken again");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {

        fbe_u32_t drives_to_remove[3] = {0, FBE_U32_MAX};
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        /* We already pulled N drive to make us degraded, now pull more to shutdown.
         */
        fbe_test_set_specific_drives_to_remove(current_rg_config_p, &drives_to_remove[0]);
        big_bird_remove_all_drives(current_rg_config_p, 1, /*rg count */ 1, /* Drive count */
                                   FBE_TRUE, /* Yes we are pulling and reinserting the same drive */
                                   0, /* do not wait between removals */
                                   FBE_DRIVE_REMOVAL_MODE_SPECIFIC);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for FAIL. again==");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for object: 0x%x to become FAIL. ", inject_rg_object_id[rg_index]);
        status = fbe_api_wait_for_object_lifecycle_state(inject_rg_object_id[rg_index], 
                                                         FBE_LIFECYCLE_STATE_FAIL, FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Reinsert drives");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {

        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        big_bird_insert_all_drives(current_rg_config_p, 1, num_drives_to_degrade + 1, /* Number of drives to insert */
                                   FBE_TRUE /* Yes we are pulling and reinserting the same drive*/);

        current_rg_config_p++;
    } 

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for object: 0x%x to become ready. ", inject_rg_object_id[rg_index]);
        status = fbe_api_wait_for_object_lifecycle_state(inject_rg_object_id[rg_index], 
                                                         FBE_LIFECYCLE_STATE_READY, FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for %d rebuilds on RG obj %x", 1, inject_rg_object_id[rg_index]);
        big_bird_wait_for_rebuilds(current_rg_config_p, 1, /* Num RGs */ 1 /* Drives to rebuild */);

        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Make sure there were no raid errors.");
    fbe_test_sep_util_validate_no_raid_errors();

    /* We could have marked degraded on another position due to coming back from shutdown with a degraded drive.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for RG object %u not degraded", inject_rg_object_id[rg_index]);
        fbe_test_sep_util_wait_rg_not_degraded(inject_rg_object_id[rg_index]);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);

    return;
}
/******************************************
 * end daffy_duck_journal_verify_test_case()
 ******************************************/
/*!**************************************************************
 * daffy_duck_journal_flush_test()
 ****************************************************************
 * @brief
 *  Put data into the journal either before or after encryption
 *  checkpoint.  Make sure we successfully flush this information.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  5/23/2014 - Created. Rob Foley
 *
 ****************************************************************/

static void daffy_duck_journal_flush_test(fbe_test_rg_configuration_t * rg_config_p,
                                     fbe_bool_t b_encrypted_io)
{
    fbe_api_rdgen_context_t *context_p = &daffy_duck_test_contexts[0];
    daffy_duck_error_case_t *error_case_p = &daffy_duck_error_cases[0];
    daffy_duck_error_case_t *current_error_case_p = NULL;
    fbe_status_t status;
    fbe_u32_t num_luns;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t num_raid_groups;
    fbe_u32_t rg_index;
    fbe_object_id_t rg_object_id;
    fbe_api_base_config_downstream_object_list_t downstream_object_list;
    fbe_sim_transport_connection_target_t peer_sp;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_object_id_t inject_rg_object_id[DAFFY_DUCK_MAX_RAID_GROUPS];
    fbe_api_rdgen_send_one_io_params_t params;

    fbe_test_sep_get_active_target_id(&active_sp);
    peer_sp = fbe_test_sep_get_peer_target_id(active_sp);

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
            fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);
            if (downstream_object_list.number_of_downstream_objects == 0) {
                MUT_FAIL_MSG("number of ds objects is 0");
            }
            inject_rg_object_id[rg_index] = downstream_object_list.downstream_object_list[0];
        } else {
            inject_rg_object_id[rg_index] = rg_object_id;
        }
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "\n\n");
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting io is %s==\n\n", __FUNCTION__, b_encrypted_io ? "encrypted" : "not encrypted");
    daffy_duck_setup_error_record(error_case_p, rg_config_p, b_encrypted_io);

    mut_printf(MUT_LOG_TEST_STATUS, "== Remove drives ");
    current_error_case_p = error_case_p;
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {

        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        
        fbe_test_set_specific_drives_to_remove(current_rg_config_p, &current_error_case_p->fault_pos[0]);
        big_bird_remove_all_drives(current_rg_config_p, 1, /*rg count */ current_error_case_p->num_drives_to_remove, /* Drive count */
                                   FBE_TRUE, /* Yes we are pulling and reinserting the same drive*/
                                   0, /* do not wait between removals */
                                   FBE_DRIVE_REMOVAL_MODE_SPECIFIC);
        current_rg_config_p++;
        current_error_case_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for rebuild logging set on intentionally removed drives.==");
    current_error_case_p = error_case_p;
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {

        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        big_bird_wait_for_rgs_rb_logging(current_rg_config_p, 1, /* num rgs */
                                         current_error_case_p->num_drives_to_remove /* Drives to wait for */);
        current_rg_config_p++;
        current_error_case_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Add error injection.");
    daffy_duck_enable_logical_error_injection(error_case_p, rg_config_p, &inject_rg_object_id[0]);

    current_error_case_p = error_case_p;
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
         mut_printf(MUT_LOG_TEST_STATUS, "== Send one I/O so the raid group hits an error. ");

        fbe_api_rdgen_io_params_init(&params);
        params.object_id = inject_rg_object_id[rg_index];
        params.class_id = FBE_CLASS_ID_INVALID;
        params.package_id = FBE_PACKAGE_ID_SEP_0;
        params.msecs_to_abort = 0;
        params.msecs_to_expiration = 0;
        params.block_spec = FBE_RDGEN_BLOCK_SPEC_CONSTANT;
        params.b_async = FBE_TRUE;
        params.rdgen_operation = FBE_RDGEN_OPERATION_WRITE_ONLY;
        params.pattern = FBE_RDGEN_PATTERN_LBA_PASS;
        params.lba = current_error_case_p->lba;
        params.blocks = 1;
        status = fbe_api_rdgen_send_one_io_params(&context_p[rg_index], &params);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_EQUAL(context_p[rg_index].start_io.statistics.error_count, 0);

        current_rg_config_p++;
        current_error_case_p++;
    }

   
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for hit of errors.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        daffy_duck_wait_error_injection(inject_rg_object_id[rg_index]);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Remove last drive to shutdown raid group. ");
    current_error_case_p = error_case_p;
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        
        big_bird_remove_all_drives(current_rg_config_p, 1, /*rg count */ 1, /* Drive count */
                                   FBE_TRUE, /* Yes we are pulling and reinserting the same drive*/
                                   0, /* do not wait between removals */
                                   FBE_DRIVE_REMOVAL_MODE_SPECIFIC);
        current_rg_config_p++;
        current_error_case_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for I/Os to finish");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_api_rdgen_wait_for_ios(&context_p[rg_index], FBE_PACKAGE_ID_SEP_0, 1);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_rdgen_test_context_destroy(&context_p[rg_index]);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        current_rg_config_p++;
    }

    /* Leave error injection enabled during paged verify so the errors are detected and we are 
     * forced to reconstruct the paged. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Disable error injection.");
    daffy_duck_disable_error_injection(rg_config_p, &inject_rg_object_id[0]);

    mut_printf(MUT_LOG_TEST_STATUS, "== Reinsert drives");
    current_error_case_p = error_case_p;
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        big_bird_insert_all_drives(current_rg_config_p, 1, current_error_case_p->num_drives_to_remove + 1, /* Number of drives to insert */
                                   FBE_TRUE /* Yes we are pulling and reinserting the same drive*/);

        current_rg_config_p++;
        current_error_case_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for object: 0x%x to become ready. ", inject_rg_object_id[rg_index]);
        status = fbe_api_wait_for_object_lifecycle_state(inject_rg_object_id[rg_index], 
                                                         FBE_LIFECYCLE_STATE_READY, FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== Make sure there were no raid errors.");
    fbe_test_sep_util_validate_no_raid_errors();

    mut_printf(MUT_LOG_TEST_STATUS, "== Check the pattern.");
    current_error_case_p = error_case_p;
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }

        fbe_api_rdgen_io_params_init(&params);
        params.object_id = inject_rg_object_id[rg_index];
        params.class_id = FBE_CLASS_ID_INVALID;
        params.package_id = FBE_PACKAGE_ID_SEP_0;
        params.msecs_to_abort = 0;
        params.msecs_to_expiration = 0;
        params.block_spec = FBE_RDGEN_BLOCK_SPEC_CONSTANT;
        params.b_async = FBE_TRUE;
        params.rdgen_operation = FBE_RDGEN_OPERATION_READ_CHECK;
        params.pattern = FBE_RDGEN_PATTERN_LBA_PASS;
        params.lba = current_error_case_p->lba;
        params.blocks = 1;
        status = fbe_api_rdgen_send_one_io_params(&context_p[rg_index], &params);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_EQUAL(context_p[rg_index].start_io.statistics.error_count, 0);

        current_rg_config_p++;
        current_error_case_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for Check Pattern I/Os to finish");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_api_rdgen_wait_for_ios(&context_p[rg_index], FBE_PACKAGE_ID_SEP_0, 1);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_rdgen_test_context_destroy(&context_p[rg_index]);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        current_rg_config_p++;
    }

    current_error_case_p = error_case_p;
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for %d rebuilds on RG obj %x",
                   current_error_case_p->num_drives_to_remove, inject_rg_object_id[rg_index]);
        big_bird_wait_for_rebuilds(current_rg_config_p, 1, current_error_case_p->num_drives_to_remove + 1);

        current_rg_config_p++;
        current_error_case_p++;
    }

    /* We could have marked degraded on another position due to coming back from shutdown with a degraded drive.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for RG object %u not degraded", inject_rg_object_id[rg_index]);
        fbe_test_sep_util_wait_rg_not_degraded(inject_rg_object_id[rg_index]);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);

    return;
}
/******************************************
 * end daffy_duck_journal_flush_test()
 ******************************************/
/*!**************************************************************
 * daffy_duck_test_rg()
 ****************************************************************
 * @brief
 *  Run a shutdown encryption test.
 *
 * @param rg_config_p - Config to create.
 * @param drive_to_remove_p - Num drives to remove. 
 *
 * @return none
 *
 * @author
 *  5/22/2014 - Created. Rob Foley
 *
 ****************************************************************/
void daffy_duck_test_rg(fbe_test_rg_configuration_t *rg_config_p, void * unused_p)
{
    fbe_api_rdgen_peer_options_t peer_options = FBE_API_RDGEN_PEER_OPTIONS_INVALID;

    if (fbe_test_sep_util_get_dualsp_test_mode()) {
        fbe_u32_t random_number = fbe_random() % 2;
        if (random_number == 0)
        {
            peer_options = FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER;
        }
        else
        {
            peer_options = FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing peer_options %d", __FUNCTION__, peer_options);
    }

    fbe_test_rg_wait_for_zeroing(rg_config_p);

    daffy_duck_start_encryption(rg_config_p);

    mut_printf(MUT_LOG_TEST_STATUS, "== Run journal verify test case. ");
    daffy_duck_journal_verify_test_case(rg_config_p);

    mut_printf(MUT_LOG_TEST_STATUS, "== Run journal rebuild test case. ");
    daffy_duck_journal_rebuild_test_case(rg_config_p);

    mut_printf(MUT_LOG_TEST_STATUS, "== Run test in encrypted area");
    daffy_duck_journal_flush_test(rg_config_p, FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "== Run test in unencrypted area");
    daffy_duck_journal_flush_test(rg_config_p, FBE_FALSE);
    daffy_duck_complete_encryption(rg_config_p);
}
/******************************************
 * end daffy_duck_test_rg()
 ******************************************/
/*!**************************************************************
 * daffy_duck_shutdown_encryption_test()
 ****************************************************************
 * @brief
 *  Run a shutdown test with I/O to a set of raid 5 objects.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  5/22/2014 - Created. Rob Foley
 *
 ****************************************************************/

void daffy_duck_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    rg_config_p = &daffy_duck_raid_group_config[0];

    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p, (void*)0,
                                                   daffy_duck_test_rg,
                                                   DAFFY_DUCK_TEST_LUNS_PER_RAID_GROUP,
                                                   DAFFY_DUCK_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end daffy_duck_test_single_degraded_test()
 ******************************************/
/*!**************************************************************
 * daffy_duck_setup()
 ****************************************************************
 * @brief
 *  Setup for a journal encryption test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  5/23/2014 - Created. Rob Foley
 *
 ****************************************************************/
void daffy_duck_setup(void)
{
    fbe_test_sep_util_set_encryption_test_mode(FBE_TRUE);
    /* Only load the physical config in simulation.
    */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups;

        /* Based on the test level determine which configuration to use.
        */
        rg_config_p = &daffy_duck_raid_group_config[0];

        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* initialize the number of extra drive required by each rg 
          */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);

        elmo_load_sep_and_neit();
        if (fbe_test_sep_util_get_encryption_test_mode()) {
            sep_config_load_kms(NULL);
        }
    }

    /* This test will pull a drive out and insert a new drive with configure extra drive as spare
     * Set the spare trigger timer 1 second to swap in immediately.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1/* 1 second */);

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    fbe_test_sep_util_disable_system_drive_zeroing();
    fbe_test_set_vault_wait_time();
    return;
}
/**************************************
 * end daffy_duck_setup()
 **************************************/
/*!**************************************************************
 * daffy_duck_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup for the daffy duck test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  5/27/2014 - Created. Rob Foley
 *
 ****************************************************************/

void daffy_duck_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        if (fbe_test_sep_util_get_encryption_test_mode()) {
            fbe_test_sep_util_destroy_kms();
        }
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    fbe_test_sep_util_set_encryption_test_mode(FBE_FALSE);
    return;
}
/******************************************
 * end daffy_duck_cleanup()
 ******************************************/

/*************************
 * end file daffy_duck_test.c
 *************************/


