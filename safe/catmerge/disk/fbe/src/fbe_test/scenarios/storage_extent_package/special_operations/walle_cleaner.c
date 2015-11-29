/***************************************************************************
 * Copyright (C) EMC Corporation 2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file walle_cleaner.c
 ***************************************************************************
 *
 * @brief
 *  This file has a test to increase code coverage.
 *  It will try to use as more untested fbe api as possible.
 *
 * @version
 *  10/24/2014 - Created. Jamin Kang
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"
#include "fbe/fbe_api_rdgen_interface.h"
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
#include "fbe/fbe_traffic_trace_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_random.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "sep_hook.h"
#include "sep_test_io.h"
#include "pp_utils.h"
#include "fbe/fbe_api_sep_io_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_test_common_utils.h"
#include "sep_test_region_io.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe_test.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe/fbe_api_trace_interface.h"
#include "fbe/fbe_raw_mirror_service_interface.h"
#include "fbe/fbe_api_metadata_interface.h"
#include "fbe/fbe_api_persist_interface.h"
#include "fbe/fbe_api_system_interface.h"
#include "fbe/fbe_api_dps_memory_interface.h"
#include "fbe/fbe_api_environment_limit_interface.h"
/*************************
 *   FUNCTION DEFINITIONS
 *************************/
char *walle_cleaner_short_desc = "Code coverage test.";
char *walle_cleaner_long_desc = "\
Code coverage test.\n\
It will use as more fbe api as possible to increase code coverage.\n\
If the system isn't panic during the test, this test will be treat as passed.\n\
\n";

#define WALLE_CLEANER_RAID_GROUP_COUNT  (6)

#define WALLE_CLEANER_CHUNKS_PER_LUN    (4)

#define WALLE_CLEANER_LUNS_PER_RG   (1)
/*!*******************************************************************
 * @var walle_cleaner_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *        Plus one on rg count for the terminator.
 *
 *********************************************************************/
static fbe_test_rg_configuration_t walle_cleaner_raid_group_config[WALLE_CLEANER_RAID_GROUP_COUNT + 1] = {
    /* width,                       capacity    raid type,                  class,       block size RAID-id. bandwidth. */
    { 6 /*FBE_TEST_RG_CONFIG_RANDOM_TAG*/, 0xE000, FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER, 520, 0, FBE_TEST_RG_CONFIG_RANDOM_TAG},
    {12 /*FBE_TEST_RG_CONFIG_RANDOM_TAG*/, 0xE000, FBE_RAID_GROUP_TYPE_RAID6,   FBE_CLASS_ID_PARITY,  520, 1, FBE_TEST_RG_CONFIG_RANDOM_TAG},
    {6 /*FBE_TEST_RG_CONFIG_RANDOM_TAG*/, 0xE000, FBE_RAID_GROUP_TYPE_RAID5,   FBE_CLASS_ID_PARITY,  520, 2, FBE_TEST_RG_CONFIG_RANDOM_TAG},
    {4 /*FBE_TEST_RG_CONFIG_RANDOM_TAG*/, 0xE000, FBE_RAID_GROUP_TYPE_RAID3,   FBE_CLASS_ID_PARITY,  520, 3, FBE_TEST_RG_CONFIG_RANDOM_TAG},
    {4 /*FBE_TEST_RG_CONFIG_RANDOM_TAG*/, 0xE000, FBE_RAID_GROUP_TYPE_RAID0,   FBE_CLASS_ID_STRIPER, 520, 5, FBE_TEST_RG_CONFIG_RANDOM_TAG},
    {3 /* FBE_TEST_RG_CONFIG_RANDOM_TAG*/, 0xE000, FBE_RAID_GROUP_TYPE_RAID1,   FBE_CLASS_ID_MIRROR,  520, 4, 0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

#define ARRAY_LEN(a)    (sizeof(a) / sizeof(a[0]))

/*********************************************************************
 * Maks opcodes: the opcode have side effect and will fail the test.
 * We won't send these opcodes
 ********************************************************************/
static fbe_u32_t lun_mask_opcodes[] = {
    FBE_LUN_CONTROL_CODE_PREPARE_TO_DESTROY_LUN,
    FBE_LUN_CONTROL_CODE_EXPORT_DEVICE,
    FBE_LUN_CONTROL_CODE_PREPARE_FOR_POWER_SHUTDOWN,
};

static fbe_u32_t rg_mask_opcodes[] = {
    FBE_RAID_GROUP_CONTROL_CODE_QUIESCE,
    FBE_RAID_GROUP_CONTROL_CODE_UNQUIESCE,
};

static fbe_u32_t vd_mask_opcodes[] = {
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_SET_NONPAGED_PERM_SPARE_BIT,
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_MARK_NEEDS_REBUILD_DONE, //20
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_ENCRYPTION_START,
    FBE_VIRTUAL_DRIVE_CONTROL_CODE_SEND_SWAP_COMMAND_COMPLETE,
};

static fbe_u32_t pvd_mask_opcodes[] = {
    FBE_PROVISION_DRIVE_CONTROL_CODE_QUIESCE,
    FBE_PROVISION_DRIVE_CONTROL_CODE_UNQUIESCE,
    FBE_PROVISION_DRIVE_CONTROL_CODE_DOWNLOAD_ACK,
    FBE_PROVISION_DRIVE_CONTROL_CODE_UNREGISTER_KEYS,
    FBE_PROVISION_DRIVE_CONTROL_CODE_RECONSTRUCT_PAGED,
    FBE_PROVISION_DRIVE_CONTROL_CODE_QUIESCE,
    FBE_PROVISION_DRIVE_CONTROL_CODE_UNQUIESCE,
    FBE_PROVISION_DRIVE_CONTROL_CODE_INITIATE_DISK_ZEROING,
    FBE_PROVISION_DRIVE_CONTROL_CODE_INITIATE_CONSUMED_AREA_ZEROING,
    FBE_PROVISION_DRIVE_CONTROL_CODE_DISABLE_PAGED_CACHE,
    FBE_PROVISION_DRIVE_CONTROL_CLEAR_SWAP_PENDING,
    FBE_PROVISION_DRIVE_CONTROL_SET_EAS_START, //62
};

static fbe_u32_t lun_class_mask_opcodes[] = {
    FBE_LUN_CONTROL_CODE_CLASS_PREPARE_FOR_POWER_SHUTDOWN,  //30
};

static fbe_u32_t raid_class_mask_opcodes[] = {
    FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_RAID_LIBRARY_ERROR_TESTING,
    FBE_RAID_GROUP_CONTROL_CODE_CLASS_RESET_RAID_LIBRARY_ERROR_TESTING_STATS,
};

static fbe_u32_t base_service_mask_opcodes[] = {
    FBE_BASE_SERVICE_CONTROL_CODE_INIT,
    FBE_BASE_SERVICE_CONTROL_CODE_DESTROY,
    FBE_BASE_SERVICE_CONTROL_CODE_USER_INDUCED_PANIC,
    FBE_MEMORY_SERVICE_CONTROL_CODE_DPS_REDUCE_SIZE,

    FBE_METADATA_CONTROL_CODE_NONPAGED_SYSTEM_CLEAR,
    FBE_METADATA_CONTROL_CODE_NONPAGED_SYSTEM_LOAD,
    FBE_METADATA_CONTROL_CODE_NONPAGED_SYSTEM_PERSIST,
    FBE_METADATA_CONTROL_CODE_NONPAGED_SYSTEM_ZERO_AND_PERSIST,
    FBE_METADATA_CONTROL_CODE_NONPAGED_LOAD,
    FBE_METADATA_CONTROL_CODE_NONPAGED_PERSIST,

    FBE_PERSIST_CONTROL_CODE_UNSET_LUN,

    FBE_DATABASE_CONTROL_CODE_UPDATE_PEER_CONFIG, //44
    FBE_DATABASE_CONTROL_CODE_COMMIT_DATABASE_TABLES,
    FBE_DATABASE_CONTROL_CODE_CLEANUP_RE_INIT_PERSIST_SERVICE,
    FBE_DATABASE_CONTROL_CODE_GENERATE_ALL_SYSTEM_OBJECTS_CONFIG_AND_PERSIST,

    FBE_CMI_CONTROL_CODE_DISABLE_TRAFFIC, /*disable all incoming traffic*/
	FBE_CMI_CONTROL_CODE_ENABLE_TRAFFIC, /*enable all incoming traffic*/

    FBE_DATABASE_CONTROL_CODE_SETUP_ENCRYPTION_KEY,
    FBE_DATABASE_CONTROL_CODE_SETUP_ENCRYPTION_REKEY,
    FBE_DATABASE_CONTROL_CODE_REMOVE_ENCRYPTION_KEYS,
    FBE_DATABASE_CONTROL_CODE_UPDATE_DRIVE_KEYS,
    FBE_DATABASE_CONTROL_CODE_COMMIT,
    FBE_DATABASE_CONTROL_CODE_SCRUB_OLD_USER_DATA,
};

/****************************************************************************
 * White list: These opcodes are expected to return OK.
 ****************************************************************************/
static fbe_u32_t object_white_list[] = {
    FBE_BVD_INTERFACE_CONTROL_CODE_ENABLE_PERFORMANCE_STATS,
    FBE_BVD_INTERFACE_CONTROL_CODE_DISABLE_PERFORMANCE_STATS,
    FBE_BVD_INTERFACE_CONTROL_CODE_CLEAR_PERFORMANCE_STATS,
    FBE_BVD_INTERFACE_CONTROL_CODE_ENABLE_ASYNC_IO,
    FBE_BVD_INTERFACE_CONTROL_CODE_DISABLE_ASYNC_IO,
    FBE_BVD_INTERFACE_CONTROL_CODE_ENABLE_ASYNC_IO_COMPL,
    FBE_BVD_INTERFACE_CONTROL_CODE_DISABLE_ASYNC_IO_COMPL,
    FBE_BVD_INTERFACE_CONTROL_CODE_ENABLE_GROUP_PRIORITY,
    FBE_BVD_INTERFACE_CONTROL_CODE_DISABLE_GROUP_PRIORITY,
    FBE_BVD_INTERFACE_CONTROL_CODE_ENABLE_PP_GROUP_PRIORITY,
    FBE_BVD_INTERFACE_CONTROL_CODE_DISABLE_PP_GROUP_PRIORITY,

    FBE_LUN_CONTROL_CODE_CLEAR_VERIFY_REPORT,
    FBE_LUN_CONTROL_CODE_TRESPASS_OP,
    FBE_LUN_CONTROL_CODE_ENABLE_PERFORMANCE_STATS,
    FBE_LUN_CONTROL_CODE_DISABLE_PERFORMANCE_STATS,
    FBE_LUN_CONTROL_CODE_ENABLE_WRITE_BYPASS,
	FBE_LUN_CONTROL_CODE_DISABLE_WRITE_BYPASS,
    FBE_LUN_CONTROL_CODE_CLEAR_UNEXPECTED_ERROR_INFO,
    FBE_LUN_CONTROL_CODE_ENABLE_FAIL_ON_UNEXPECTED_ERROR,
    FBE_LUN_CONTROL_CODE_DISABLE_FAIL_ON_UNEXPECTED_ERROR,

    FBE_RAID_GROUP_CONTROL_CODE_CLEAR_FLUSH_ERROR_COUNTERS,

    FBE_PROVISION_DRIVE_CONTROL_CODE_INITIATE_DISK_ZEROING,
    FBE_PROVISION_DRIVE_CONTROL_CODE_CLEAR_VERIFY_REPORT,
    FBE_PROVISION_DRIVE_CONTROL_CODE_SET_EOL_STATE,
    FBE_PROVISION_DRIVE_CONTROL_CODE_CLEAR_EOL_STATE,
    FBE_PROVISION_DRIVE_CONTROL_CODE_CLEAR_DRIVE_FAULT,
    FBE_PROVISION_DRIVE_CONTROL_SET_EAS_COMPLETE,
};
static fbe_u32_t class_white_list[] = {
    FBE_LUN_CONTROL_CODE_CLASS_PREFSTATS_SET_ENABLED,
    FBE_LUN_CONTROL_CODE_CLASS_PREFSTATS_SET_DISABLED,
    FBE_RAID_GROUP_CONTROL_CODE_CLASS_RESET_RAID_MEMORY_STATS,
};
static fbe_u32_t service_white_list[] = {
    FBE_MEMORY_SERVICE_CONTROL_CODE_GET_DPS_STATS,
    FBE_MEMORY_SERVICE_CONTROL_CODE_GET_ENV_LIMITS,
    FBE_MEMORY_SERVICE_CONTROL_CODE_PERSIST_SET_PARAMS,
    FBE_SCHEDULER_CONTROL_CODE_SET_CREDITS,
    FBE_SCHEDULER_CONTROL_CODE_GET_CREDITS,
    FBE_SCHEDULER_CONTROL_CODE_ADD_CPU_CREDITS_PER_CORE,
    FBE_SCHEDULER_CONTROL_CODE_REMOVE_CPU_CREDITS_PER_CORE,
    FBE_SCHEDULER_CONTROL_CODE_REMOVE_CPU_CREDITS_ALL_CORES,
    FBE_SCHEDULER_CONTROL_CODE_ADD_CPU_CREDITS_ALL_CORES,
    FBE_SCHEDULER_CONTROL_CODE_ADD_MEMORY_CREDITS,
    FBE_SCHEDULER_CONTROL_CODE_REMOVE_MEMORY_CREDITS,
    FBE_SCHEDULER_CONTROL_CODE_SET_SCALE,
    FBE_SCHEDULER_CONTROL_CODE_GET_SCALE,
    FBE_SCHEDULER_CONTROL_CODE_SET_DEBUG_HOOK,
    FBE_SCHEDULER_CONTROL_CODE_GET_DEBUG_HOOK,
    FBE_SCHEDULER_CONTROL_CODE_DELETE_DEBUG_HOOK,
    FBE_SCHEDULER_CONTROL_CODE_CLEAR_DEBUG_HOOKS,

    FBE_EVENT_LOG_CONTROL_CODE_CLEAR_LOG,
    FBE_EVENT_LOG_CONTROL_CODE_CLEAR_STATISTICS,
    FBE_EVENT_LOG_CONTROL_CODE_DISABLE_TRACE,
    FBE_EVENT_LOG_CONTROL_CODE_ENABLE_TRACE,

    FBE_TRACE_CONTROL_CODE_SET_ERROR_LIMIT,
    FBE_TRACE_CONTROL_CODE_GET_ERROR_LIMIT,
    FBE_TRACE_CONTROL_CODE_RESET_ERROR_COUNTERS,
    FBE_TRACE_CONTROL_CODE_ENABLE_BACKTRACE,
	FBE_TRACE_CONTROL_CODE_DISABLE_BACKTRACE,
    FBE_TRACE_CONTROL_CODE_CLEAR_ERROR_COUNTERS,

    FBE_TRAFFIC_TRACE_CONTROL_CODE_ENABLE_RBA_TRACE,
    FBE_TRAFFIC_TRACE_CONTROL_CODE_DISABLE_RBA_TRACE,

	FBE_METADATA_CONTROL_CODE_SET_NDU_IN_PROGRESS,
	FBE_METADATA_CONTROL_CODE_CLEAR_NDU_IN_PROGRESS,

    FBE_CMI_CONTROL_CODE_CLEAR_IO_STATISTICS,

    FBE_DATABASE_CONTROL_CODE_SET_NDU_STATE,
    FBE_DATABASE_CONTROL_CODE_RESTART_ALL_BACKGROUND_SERVICES,
    FBE_DATABASE_CONTROL_ENABLE_LOAD_BALANCE,
    FBE_DATABASE_CONTROL_DISABLE_LOAD_BALANCE,

    FBE_JOB_CONTROL_CODE_ADD_DEBUG_HOOK,
    FBE_JOB_CONTROL_CODE_GET_DEBUG_HOOK,
    FBE_JOB_CONTROL_CODE_REMOVE_DEBUG_HOOK,
};


static fbe_bool_t walle_cleaner_is_in_array(fbe_u32_t op, fbe_u32_t *ops, fbe_u32_t ops_len)
{
    fbe_u32_t i;

    if (ops == NULL) {
        return FBE_FALSE;
    }

    for (i = 0; i < ops_len; i += 1) {
        if (op == ops[i]) {
            return FBE_TRUE;
        }
    }
    return FBE_FALSE;
}

static fbe_bool_t walle_cleaner_is_op_masked(fbe_u32_t op, fbe_u32_t *mask_ops, fbe_u32_t mask_op_len)
{
    return walle_cleaner_is_in_array(op, mask_ops, mask_op_len);
}

#define walle_cleaner_is_in_white_list(op, list)        \
    walle_cleaner_is_in_array(op, list, ARRAY_LEN(list))


static void walle_cleaner_test_sleep(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "Sleeping...");
    /* Just sleep 5 milliseconds ...*/
    fbe_api_sleep(5);
}

/*!**************************************************************
 * walle_cleaner_send_usurpers_to_object()
 ****************************************************************
 * @brief
 *   Call usurper functions for object
 *
 * @param None
 *
 * @return None.
 *
 * @author
 *  10/24/2014 - Created. Jamin Kang
 *
 ****************************************************************/
static void walle_cleaner_send_usurpers_to_object(fbe_object_id_t obj_id, fbe_u32_t start, fbe_u32_t end,
                                                  fbe_u32_t *mask_ops, fbe_u32_t mask_op_len)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                                       opcode;
    fbe_api_control_operation_status_info_t         status_info;

    for (opcode = start; opcode < end; opcode += 1) {
        if (walle_cleaner_is_op_masked(opcode, mask_ops, mask_op_len)) {
            mut_printf(MUT_LOG_TEST_STATUS, "Skip usurpers 0x%x to object 0x%x",
                       opcode, obj_id);
            continue;
        }

        mut_printf(MUT_LOG_TEST_STATUS, "Send usurpers to object 0x%x, opcode: 0x%x",
                   obj_id, opcode);
        status = fbe_api_common_send_control_packet(opcode, NULL, 0,
                                                    obj_id, FBE_PACKET_FLAG_NO_ATTRIB, &status_info,
                                                    FBE_PACKAGE_ID_SEP_0);
        if (status == FBE_STATUS_OK) {
            if (!walle_cleaner_is_in_white_list(opcode, object_white_list)){
                mut_printf(MUT_LOG_TEST_STATUS,
                           "Send usurpers to object 0x%x, opcode: 0x%x unexpected SUCCESSED!!!",
                   obj_id, opcode);
            }
        }
    }
}

/*!**************************************************************
 * walle_cleaner_send_usurpers_to_class()
 ****************************************************************
 * @brief
 *   Call usurper functions for object
 *
 * @param None
 *
 * @return None.
 *
 * @author
 *  10/24/2014 - Created. Jamin Kang
 *
 ****************************************************************/
static void walle_cleaner_send_usurpers_to_class(fbe_class_id_t class_id, fbe_u32_t start, fbe_u32_t end,
                                                 fbe_u32_t *mask_ops, fbe_u32_t mask_op_len)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                                       opcode;
    fbe_api_control_operation_status_info_t         status_info;

    for (opcode = start; opcode < end; opcode += 1) {
        if (walle_cleaner_is_op_masked(opcode, mask_ops, mask_op_len)) {
            mut_printf(MUT_LOG_TEST_STATUS, "Skip usurpers 0x%x for class 0x%x",
                       opcode, class_id);
            continue;
        }
        status = fbe_api_common_send_control_packet_to_class(opcode,
                                                             NULL, 0,
                                                             class_id,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_SEP_0);

        if (status == FBE_STATUS_OK) {
            if (!walle_cleaner_is_in_white_list(opcode, class_white_list)) {
                mut_printf(MUT_LOG_TEST_STATUS,
                           "Send usurpers to class 0x%x, opcode: 0x%x unexpected SUCCESSED!!!",
                           class_id, opcode);
            }
        }
        mut_printf(MUT_LOG_TEST_STATUS, "Send usurpers to class 0x%x, opcode: 0x%x",
                   class_id, opcode);
    }
}

/*!**************************************************************
 * walle_cleaner_send_usurpers_to_service()
 ****************************************************************
 * @brief
 *   Call usurper functions for services
 *
 * @param None
 *
 * @return None.
 *
 * @author
 *  10/24/2014 - Created. Jamin Kang
 *
 ****************************************************************/
static void walle_cleaner_send_usurpers_to_service(fbe_service_id_t service_id, fbe_u32_t start, fbe_u32_t end,
                                                   fbe_u32_t *mask_ops, fbe_u32_t mask_op_len)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                                       opcode;
    fbe_api_control_operation_status_info_t         status_info;

    for (opcode = start; opcode < end; opcode += 1) {
        if (walle_cleaner_is_op_masked(opcode, mask_ops, mask_op_len)) {
            mut_printf(MUT_LOG_TEST_STATUS, "Skip usurpers 0x%x for service 0x%x",
                       opcode, service_id);
            continue;
        }

        mut_printf(MUT_LOG_TEST_STATUS, "Send usurpers to service 0x%x, opcode: 0x%x",
                   service_id, opcode);
        status = fbe_api_common_send_control_packet_to_service(opcode,
                                                               NULL, 0,
                                                               service_id,
                                                               FBE_PACKET_FLAG_NO_ATTRIB,
                                                               &status_info,
                                                               FBE_PACKAGE_ID_SEP_0);
        if (status == FBE_STATUS_OK) {
            if (!walle_cleaner_is_in_white_list(opcode, service_white_list)) {
                mut_printf(MUT_LOG_TEST_STATUS,
                           "Send usurpers to service 0x%x, opcode: 0x%x unexpected SUCCESSED!!!",
                           service_id, opcode);
            }
        }
        mut_printf(MUT_LOG_TEST_STATUS, "Send usurpers to service 0x%x, opcode: 0x%x done",
                   service_id, opcode);
    }
}

static void walle_cleaner_test_common_api(void)
{
    fbe_status_t status;
    fbe_object_id_t lun_obj;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_trace_level_control_t trace_flags;
    fbe_api_trace_get_error_limit_t error_limits;


    status = fbe_api_database_lookup_lun_by_number(0, &lun_obj);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get LUN 0 object id");
    rg_config_p = &walle_cleaner_raid_group_config[0];
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p[0].raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get raid object id");
    status = fbe_api_wait_for_rg_peer_state_ready(rg_object_id, 30000);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to wait raid status");
    status = fbe_api_wait_for_lun_peer_state_ready(lun_obj, 30000);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to wait lun status");
    status = fbe_test_sep_util_set_lifecycle_trace_control_flag(FBE_LIFECYCLE_DEBUG_FLAG_LUN_CLASS_TRACING);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to set lifecycle trace");
    status = fbe_api_trace_get_flags(&trace_flags, FBE_PACKAGE_ID_SEP_0);
    status = fbe_api_trace_set_flags(&trace_flags, FBE_PACKAGE_ID_SEP_0);
    status = fbe_api_trace_get_error_limit(FBE_PACKAGE_ID_SEP_0, &error_limits);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get error limit");
    fbe_api_traffic_priority_to_string(FBE_TRAFFIC_PRIORITY_NORMAL);
}

extern fbe_status_t FBE_API_CALL
fbe_api_database_update_virtual_drive(fbe_database_control_update_vd_t *virtual_drive);

static void walle_cleaner_test_database_api(void)
{
    fbe_status_t status;
    fbe_u8_t buffer[10 * 4096];
    fbe_u32_t edge_count;
    fbe_u32_t obj_count;
    fbe_u32_t actual_count;
    fbe_u32_t user_count;
    fbe_object_id_t lun_obj;
    fbe_database_capacity_limit_t get_limit;
    fbe_compatibility_mode_t mode;
    fbe_database_debug_flags_t  debug_flags;
    fbe_fru_signature_t disk_signature = {0};
    fbe_database_encryption_t encryption_info;
    fbe_database_get_sep_shim_raid_info_t raid_info;
    fbe_database_power_save_t power_save;
    fbe_database_system_encryption_info_t sys_encryption_info;
    fbe_database_system_encryption_progress_t sys_encryption_progress;
    fbe_u32_t max_sep_objects;
    fbe_object_id_t system_raid_group_id;
    fbe_database_time_threshold_t time_threshold_info;
    fbe_set_ndu_state_t ndu_state;
    fbe_database_lun_info_t lun_list[3];


    edge_count = sizeof(buffer) / (sizeof(database_edge_entry_t) * DATABASE_MAX_EDGE_PER_OBJECT);
    status = fbe_api_database_get_edge_tables_in_range(buffer, sizeof(buffer), FBE_RESERVED_OBJECT_IDS,
                                                       FBE_RESERVED_OBJECT_IDS + edge_count - 1, &actual_count);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get edge table");
    obj_count = sizeof(buffer) / sizeof(database_object_entry_t);
    status = fbe_api_database_get_object_tables_in_range(buffer, sizeof(buffer), FBE_RESERVED_OBJECT_IDS,
                                                         FBE_RESERVED_OBJECT_IDS + obj_count - 1, &actual_count);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get object table");
    user_count = sizeof(buffer) / sizeof(database_user_entry_t);
    fbe_api_database_get_user_tables_in_range(buffer, sizeof(buffer), FBE_RESERVED_OBJECT_IDS,
                                              FBE_RESERVED_OBJECT_IDS + user_count - 1, &actual_count);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get user table");
    status = fbe_api_database_persist_system_db(NULL, 0, 0, NULL);
    MUT_ASSERT_INT_NOT_EQUAL_MSG(FBE_STATUS_OK, status, "Persist return OK!");


    status = fbe_api_database_lookup_lun_by_number(0, &lun_obj);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get LUN 0 object id");

    status = fbe_api_database_lun_operation_ktrace_warning(lun_obj, FBE_DATABASE_LUN_CREATE_FBE_CLI, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to send warning");
    fbe_api_database_clear_disk_signature_info(NULL);
    fbe_api_database_commit_transaction(0);
    fbe_api_database_generate_system_object_config_and_persist(FBE_OBJECT_ID_INVALID);
    status = fbe_api_database_get_capacity_limit(&get_limit);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get capacity limit");
    status = fbe_api_database_get_compat_mode(&mode);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get compat mode");
    status = fbe_api_database_get_debug_flags(&debug_flags);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get DB debug flags");
    status = fbe_api_database_get_disk_signature_info(&disk_signature);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get fru signature");
    status = fbe_api_database_get_encryption(&encryption_info);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get DB encryption");
    status = fbe_api_database_get_lun_raid_info(lun_obj, &raid_info);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get lun raid info");
    status = fbe_api_database_get_power_save(&power_save);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get power save");
    status = fbe_api_database_get_system_encryption_info(&sys_encryption_info);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get sys encryption info");
    status = fbe_api_database_get_system_encryption_progress(&sys_encryption_progress);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get sys encryption progress");
    fbe_api_database_init_system_db_header(NULL);
    fbe_api_database_restore_user_configuration(NULL);
    status = fbe_api_database_set_disk_signature_info(&disk_signature);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to set disk signature");
    fbe_api_database_system_send_clone_cmd(NULL);
    fbe_api_database_update_virtual_drive(NULL);
    status = fbe_api_database_get_max_configurable_objects(&max_sep_objects);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get max sep object number");
    status = fbe_api_database_get_system_object_id(&system_raid_group_id);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get system raid group id");
    status = fbe_api_database_set_capacity_limit(&get_limit);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to set capacity limit");
    status = fbe_api_database_set_debug_flags(debug_flags);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to set debug flags");
    fbe_api_database_set_encryption(encryption_info);
    status = fbe_api_database_get_time_threshold(&time_threshold_info);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get time threshold");
    status = fbe_api_database_set_time_threshold(time_threshold_info);
    fbe_zero_memory(&ndu_state, sizeof(ndu_state));
    fbe_api_set_ndu_state(ndu_state);
    status = fbe_api_database_get_all_luns(&lun_list[0], ARRAY_LEN(lun_list), &actual_count);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get all luns");
}

static void walle_cleaner_test_pvd_api(void)
{
    fbe_status_t status;
    fbe_api_provision_drive_get_paged_info_t paged_info;
    fbe_object_id_t pvd_id = 1;
    fbe_provision_drive_paged_metadata_t paged_md[1];
    fbe_provision_drive_set_priorities_t priorities = {
        FBE_TRAFFIC_PRIORITY_NORMAL, FBE_TRAFFIC_PRIORITY_NORMAL, FBE_TRAFFIC_PRIORITY_NORMAL,
    };
    fbe_api_provision_drive_calculate_capacity_info_t provision_drive_capacity_info = {
        TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY, FBE_BLOCK_EDGE_GEOMETRY_520_520,
    };
    fbe_provision_drive_control_get_bg_op_speed_t get_bg_op;
    fbe_bool_t eas_is_started, eas_is_complete;
    fbe_provision_drive_control_get_metadata_memory_t md_memory;
    fbe_bool_t slf_is_enabled;
    fbe_provision_drive_map_info_t pvd_map = {
        0, 0, 0,
    };
    fbe_api_provision_drive_update_config_type_t update_config_type = {
        0x100, FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID,
    };
    fbe_api_provision_drive_set_swap_pending_t mark_offline_reason = {
        FBE_PROVISION_DRIVE_DRIVE_SWAP_PENDING_REASON_USER_COPY
    };
    fbe_provision_drive_get_paged_cache_info_t paged_cache_info;
    fbe_bool_t b_is_disable_spare_select_unconsumed_set;
    fbe_object_id_t used_pvd_obj, vd_obj;
    fbe_u32_t max_chunk;


    status = fbe_api_provision_drive_clear_all_pvds_drive_states();
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to clear pvd states");
    status = fbe_api_provision_drive_get_paged_bits(pvd_id, &paged_info, FBE_FALSE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get paged info");
    status = fbe_api_provision_drive_get_paged_metadata(pvd_id, 0, ARRAY_LEN(paged_md), &paged_md[0], FBE_FALSE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get paged metdatad");
    fbe_api_provision_drive_set_swap_pending(0x200);
    status = fbe_api_provision_drive_set_background_priorities(pvd_id, &priorities);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to set bg priorities");
    fbe_api_provision_drive_abort_download(pvd_id);
    status = fbe_api_provision_drive_calculate_capacity(&provision_drive_capacity_info);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to calculate capacity");
    status = fbe_api_provision_drive_get_background_operation_speed(&get_bg_op);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get bg op speed");
    status = fbe_api_provision_drive_get_eas_state(pvd_id, &eas_is_started, &eas_is_complete);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get eas state");
    status = fbe_api_provision_drive_get_metadata_memory(pvd_id, &md_memory);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get md memory");
    status = fbe_api_provision_drive_get_slf_enabled(&slf_is_enabled);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get SLF state");
    status = fbe_api_provision_drive_map_lba_to_chunk(pvd_id, &pvd_map);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to map lba");
    status = fbe_api_provision_drive_set_background_operation_speed(FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF, 10);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to set bgop speed");
    fbe_api_provision_drive_set_config_type(&update_config_type);
    fbe_api_provision_drive_set_eas_complete(pvd_id);
    fbe_api_provision_drive_set_eas_start(pvd_id);
    fbe_api_provision_drive_set_set_pvd_swap_pending_with_reason(0x200, &mark_offline_reason);
    fbe_api_provision_drive_set_verify_invalidate_checkpoint(pvd_id, 0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to set invalidate checkpoint");
    status = fbe_api_provision_drive_test_slf(0x100, 0);
    status = fbe_api_provision_drive_disable_paged_cache(0x100);
    status = fbe_api_provision_drive_get_paged_cache_info(0x100, &paged_cache_info);
    status = fbe_api_provision_drive_send_logical_error(0x100, FBE_BLOCK_TRANSPORT_ERROR_TYPE_INVALID);
    status = fbe_api_provision_drive_get_disable_spare_select_unconsumed(&b_is_disable_spare_select_unconsumed_set);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get disable spare select unconsumed");
    status = fbe_api_provision_drive_get_obj_id_by_location(
        walle_cleaner_raid_group_config[0].rg_disk_set[0].bus,
        walle_cleaner_raid_group_config[0].rg_disk_set[0].enclosure,
        walle_cleaner_raid_group_config[0].rg_disk_set[0].slot,
        &used_pvd_obj);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get pvd object");
    status = fbe_api_provision_drive_get_vd_object_id(used_pvd_obj, &vd_obj);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get vd object");
    status = fbe_api_provision_drive_get_max_read_chunks(&max_chunk);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get max chunk");
}

static void walle_cleaner_test_rg_api(void)
{
    fbe_status_t status;
    fbe_raid_group_get_default_bts_params_t bts_param;
    fbe_raid_group_set_default_bts_params_t set_bts_param;
    fbe_object_id_t rg_object_id;
    fbe_raid_group_get_statistics_t rg_static;
    fbe_raid_group_control_get_bg_op_speed_t get_bg_op_speed;
    fbe_raid_flush_error_counts_t flush_error_counts;
    fbe_block_transport_control_get_max_unused_extent_size_t get_max_unused_extent_size;
    fbe_api_raid_group_calculate_capacity_info_t calculate_capacity = {
        0x10000000, FBE_BLOCK_EDGE_GEOMETRY_520_520,

    };
    fbe_raid_group_default_debug_flag_payload_t debug_flag;
    fbe_raid_group_default_library_flag_payload_t debug_lib_flags;


    status = fbe_api_database_lookup_raid_group_by_number(walle_cleaner_raid_group_config[0].raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_raid_group_get_bts_params(&bts_param);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get bts param");
    status = fbe_api_raid_group_get_stats(rg_object_id, &rg_static);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get rg statics");
    set_bts_param.user_queue_depth = bts_param.user_queue_depth;
    set_bts_param.system_queue_depth = bts_param.system_queue_depth;
    set_bts_param.b_throttle_enabled = bts_param.b_throttle_enabled;
    set_bts_param.b_reload = FBE_FALSE;
    set_bts_param.b_persist = FBE_FALSE;
    status = fbe_api_raid_group_set_bts_params(&set_bts_param);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to set bts param");
    status = fbe_api_raid_group_get_background_operation_speed(&get_bg_op_speed);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get bg op speed");
    status = fbe_api_raid_group_get_flush_error_counters(rg_object_id, FBE_PACKET_FLAG_NO_ATTRIB, &flush_error_counts);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get flush counter");
    status = fbe_api_raid_group_get_max_unused_extent_size(rg_object_id, &get_max_unused_extent_size);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get max unused extent size");
    status = fbe_api_raid_group_set_background_operation_speed(FBE_RAID_GROUP_BACKGROUND_OP_RW_VERIFY, 10);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to set bgop speed");
    status = fbe_api_raid_group_set_raid_attributes(rg_object_id, FBE_RAID_ATTRIBUTE_NON_OPTIMAL);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to set rg attribute");
    status = fbe_api_raid_group_calculate_capacity(&calculate_capacity);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to calculate capacity");
    status = fbe_api_raid_group_clear_flush_error_counters(rg_object_id, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to clear flush error");
    status = fbe_api_raid_group_get_default_debug_flags(FBE_CLASS_ID_MIRROR, &debug_flag);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get debug flags");
    status = fbe_api_raid_group_get_default_library_flags(FBE_CLASS_ID_MIRROR, &debug_lib_flags);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get library debug flags");
    status = fbe_api_raid_group_set_default_debug_flags(FBE_CLASS_ID_MIRROR, debug_flag.user_debug_flags, debug_flag.system_debug_flags);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to set debug flags");
    status = fbe_api_raid_group_set_default_library_flags(FBE_CLASS_ID_MIRROR,
                                                          debug_lib_flags.user_debug_flags, debug_lib_flags.system_debug_flags);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to set debug library flags");
    status = fbe_api_raid_group_set_class_group_debug_flags(FBE_CLASS_ID_MIRROR, FBE_RAID_GROUP_DEBUG_FLAG_NONE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to set class debug library flags");
    status = fbe_api_raid_group_set_class_library_debug_flags(FBE_CLASS_ID_MIRROR, FBE_RAID_LIBRARY_DEBUG_FLAG_NONE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to set class library debug library flags");
}

static void walle_cleaner_test_bvd_api(void)
{
    fbe_status_t status;
    fbe_sep_shim_get_perf_stat_t get_perf_stat;

    status = fbe_api_bvd_interface_disable_group_priority(FBE_FALSE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to disable group priority");
    status = fbe_api_bvd_interface_enable_group_priority(FBE_FALSE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to enable group priority");
    status = fbe_api_bvd_interface_get_peformance_statistics(&get_perf_stat);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get performance statistics");
    status = fbe_api_bvd_interface_clear_peformance_statistics();
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to clear performance statistics");
    status = fbe_api_bvd_interface_disable_async_io();
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to diable async io");
    status = fbe_api_bvd_interface_disable_async_io_compl();
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to diable async io compl");
    status = fbe_api_bvd_interface_disable_peformance_statistics();
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to diable performance statistics");
    status = fbe_api_bvd_interface_enable_async_io();
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to enable async io");
    status = fbe_api_bvd_interface_enable_async_io_compl();
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to enable async io compl");
    status = fbe_api_bvd_interface_enable_peformance_statistics();
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to enable performance statistics");
    status = fbe_api_bvd_interface_set_alert_time(0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to set alert time");
    status = fbe_api_bvd_interface_set_rq_method(FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to set rq method");
}

static void walle_cleaner_test_base_config_api(void)
{
    fbe_status_t status;
    fbe_object_id_t rg_object_id;

    status = fbe_api_database_lookup_raid_group_by_number(walle_cleaner_raid_group_config[0].raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_base_config_passive_request(rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_api_base_config_set_encryption_mode(rg_object_id, FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED);
    fbe_api_base_config_set_encryption_state(rg_object_id, FBE_BASE_CONFIG_ENCRYPTION_STATE_UNENCRYPTED);
}

static void walle_cleaner_test_metadata_api(void)
{
    fbe_status_t status;
    fbe_object_id_t rg_object_id;
    fbe_api_nonpaged_metadata_version_info_t version_info;
    fbe_nonpaged_metadata_backup_restore_t backup;
    fbe_api_metadata_get_sl_info_t sl_info;

    status = fbe_api_database_lookup_raid_group_by_number(walle_cleaner_raid_group_config[0].raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_metadata_nonpaged_get_version_info(rg_object_id, &version_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    backup.start_object_id = rg_object_id;
    backup.object_number = 1;
    status = fbe_api_metadata_nonpaged_backup_objects(&backup);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    sl_info.object_id = rg_object_id;
    status = fbe_api_metadata_get_sl_info(&sl_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}

static void walle_cleaner_test_persist_api(void)
{
    fbe_status_t status;
    fbe_lba_t lba;

    status = fbe_api_persist_get_total_blocks_needed(&lba);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}

static void walle_cleaner_test_lun_api(void)
{
    fbe_status_t status;
    fbe_object_id_t lun_obj;
    fbe_lun_get_verify_status_t verify_status;
    fbe_lun_trespass_op_t trespass_op = {
        FBE_LUN_TRESPASS_EXECUTE,
    };
    fbe_object_id_t rg_object_id;
    fbe_api_lun_calculate_cache_zero_bitmap_blocks_t zero_bitmap_blocks = {
        0x10000000, 0
    };

    status = fbe_api_database_lookup_raid_group_by_number(walle_cleaner_raid_group_config[0].raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_database_lookup_lun_by_number(0, &lun_obj);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get LUN 0 object id");
    status = fbe_api_lun_initiate_verify_on_all_existing_luns(FBE_LUN_VERIFY_TYPE_USER_RO);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_lun_clear_verify_reports_on_all_existing_luns();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_lun_initiate_verify_on_all_user_and_system_luns(FBE_LUN_VERIFY_TYPE_USER_RO);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_lun_get_verify_status(lun_obj, FBE_PACKET_FLAG_NO_ATTRIB, &verify_status);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_lun_set_write_bypass_mode(lun_obj, FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_lun_trespass_op(lun_obj, FBE_PACKET_FLAG_NO_ATTRIB, &trespass_op);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_lun_clear_verify_reports_on_rg(rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_lun_calculate_cache_zero_bit_map_size_to_remove(&zero_bitmap_blocks);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}

static void walle_cleaner_test_service_api(void)
{
    fbe_status_t status;
    fbe_environment_limits_t env_limit;
    fbe_api_system_get_failure_info_t failure_info;

    status = fbe_api_dps_memory_display_statistics(FBE_FALSE, FBE_API_DPS_DISPLAY_DEFAULT, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_memory_get_env_limits(&env_limit);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_api_dps_memory_add_more_memory(FBE_PACKAGE_ID_SEP_0);
    status = fbe_api_get_environment_limits(&env_limit, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_api_set_environment_limits(&env_limit, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_trace_err_set_notify_level(FBE_TRACE_LEVEL_INVALID, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_event_log_enable(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_system_get_failure_info(&failure_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}

/*!**************************************************************
 * walle_cleaner_test_fbe_api()
 ****************************************************************
 * @brief
 *   Call fbe api functions to increase code coverage
 *
 * @param None
 *
 * @return None.
 *
 * @author
 *  10/24/2014 - Created. Jamin Kang
 *
 ****************************************************************/
static void walle_cleaner_test_fbe_api(void)
{
    walle_cleaner_test_common_api();
    walle_cleaner_test_database_api();
    walle_cleaner_test_pvd_api();
    walle_cleaner_test_rg_api();
    walle_cleaner_test_bvd_api();
    walle_cleaner_test_base_config_api();
    walle_cleaner_test_metadata_api();
    walle_cleaner_test_persist_api();
    walle_cleaner_test_lun_api();
    walle_cleaner_test_service_api();
}

/*!**************************************************************
 * walle_cleaner_test_usurper()
 ****************************************************************
 * @brief
 *   Call usurper functions to objects to increase code coverage
 *
 * @param None
 *
 * @return None.
 *
 * @author
 *  10/24/2014 - Created. Jamin Kang
 *
 ****************************************************************/
static void walle_cleaner_test_obj_usurper(void)
{
    fbe_object_id_t bvd_obj;
    fbe_status_t status;
    fbe_u32_t lun_number = 0;
    fbe_object_id_t lun_obj;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t num_raid_groups;
    fbe_u32_t rg_index;
    fbe_u32_t position = 0;
    fbe_object_id_t vd_obj;
    fbe_object_id_t pvd_obj;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;

    status = fbe_api_bvd_interface_get_bvd_object_id(&bvd_obj);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get bvd object id");
    mut_printf(MUT_LOG_TEST_STATUS, "Send usurpers to BVD\n");
    walle_cleaner_send_usurpers_to_object(bvd_obj,
                                          FBE_BVD_INTERFACE_CONTROL_CODE_INVALID, FBE_BVD_INTERFACE_CONTROL_CODE_LAST,
                                          NULL, 0);
    status = fbe_api_database_lookup_lun_by_number(lun_number, &lun_obj);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get LUN 0 object id");
    mut_printf(MUT_LOG_TEST_STATUS, "Send usurpers to LUN\n");
    walle_cleaner_send_usurpers_to_object(lun_obj,
                                          FBE_LUN_CONTROL_CODE_INVALID, FBE_LUN_CONTROL_CODE_LAST,
                                          &lun_mask_opcodes[0], ARRAY_LEN(lun_mask_opcodes));

    rg_config_p = &walle_cleaner_raid_group_config[0];
    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    for (rg_index = 0; rg_index < num_raid_groups; rg_index += 1) {

        status = fbe_api_database_lookup_raid_group_by_number(rg_config_p[rg_index].raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "Send usurpers to RG[%u]\n", rg_index);
        walle_cleaner_send_usurpers_to_object(rg_object_id,
                                              FBE_RAID_GROUP_CONTROL_CODE_INVALID, FBE_RAID_GROUP_CONTROL_CODE_LAST,
                                              &rg_mask_opcodes[0], ARRAY_LEN(rg_mask_opcodes));
    }

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p[0].raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position, &vd_obj);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "Send usurpers to VD\n");
    walle_cleaner_send_usurpers_to_object(vd_obj,
                                          FBE_VIRTUAL_DRIVE_CONTROL_CODE_INVALID, FBE_VIRTUAL_DRIVE_CONTROL_CODE_LAST,
                                          &vd_mask_opcodes[0], ARRAY_LEN(vd_mask_opcodes));

    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p[0].rg_disk_set[position].bus,
                                                            rg_config_p[0].rg_disk_set[position].enclosure,
                                                            rg_config_p[0].rg_disk_set[position].slot,
                                                            &pvd_obj);
    mut_printf(MUT_LOG_TEST_STATUS, "Send usurpers to PVD 0x%x\n", pvd_obj);
    walle_cleaner_send_usurpers_to_object(pvd_obj,
                                          FBE_PROVISION_DRIVE_CONTROL_CODE_INVALID,
                                          FBE_PROVISION_DRIVE_CONTROL_CODE_LAST,
                                          &pvd_mask_opcodes[0], ARRAY_LEN(pvd_mask_opcodes));
}

/*!**************************************************************
 * walle_cleaner_test_class_usurper()
 ****************************************************************
 * @brief
 *   Call usurper functions to classes to increase code coverage
 *
 * @param None
 *
 * @return None.
 *
 * @author
 *  10/24/2014 - Created. Jamin Kang
 *
 ****************************************************************/
static void walle_cleaner_test_class_usurper(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "Send usurpers to class LUN\n");
    walle_cleaner_send_usurpers_to_class(FBE_CLASS_ID_LUN,
                                         FBE_LUN_CONTROL_CODE_CLASS_CALCULATE_IMPORTED_CAPACITY,
                                         FBE_LUN_CONTROL_CODE_LAST,
                                         &lun_class_mask_opcodes[0], ARRAY_LEN(lun_class_mask_opcodes));
    mut_printf(MUT_LOG_TEST_STATUS, "Send usurpers to class RAID GROUP\n");
    walle_cleaner_send_usurpers_to_class(FBE_CLASS_ID_RAID_GROUP,
                                         FBE_RAID_GROUP_CONTROL_CODE_INVALID,
                                         FBE_RAID_GROUP_CONTROL_CODE_LAST,
                                         &raid_class_mask_opcodes[0], ARRAY_LEN(raid_class_mask_opcodes));
    mut_printf(MUT_LOG_TEST_STATUS, "Send usurpers to class MIRROR\n");
    walle_cleaner_send_usurpers_to_class(FBE_CLASS_ID_MIRROR,
                                         FBE_RAID_GROUP_CONTROL_CODE_INVALID,
                                         FBE_RAID_GROUP_CONTROL_CODE_LAST,
                                         &raid_class_mask_opcodes[0], ARRAY_LEN(raid_class_mask_opcodes));
    mut_printf(MUT_LOG_TEST_STATUS, "Send usurpers to class STRIPER\n");
    walle_cleaner_send_usurpers_to_class(FBE_CLASS_ID_STRIPER,
                                         FBE_RAID_GROUP_CONTROL_CODE_INVALID,
                                         FBE_RAID_GROUP_CONTROL_CODE_LAST,
                                         &raid_class_mask_opcodes[0], ARRAY_LEN(raid_class_mask_opcodes));
    mut_printf(MUT_LOG_TEST_STATUS, "Send usurpers to class PARITY\n");
    walle_cleaner_send_usurpers_to_class(FBE_CLASS_ID_PARITY,
                                         FBE_RAID_GROUP_CONTROL_CODE_INVALID,
                                         FBE_RAID_GROUP_CONTROL_CODE_LAST,
                                         &raid_class_mask_opcodes[0], ARRAY_LEN(raid_class_mask_opcodes));
    mut_printf(MUT_LOG_TEST_STATUS, "Send usurpers to class VD\n");
    walle_cleaner_send_usurpers_to_class(FBE_CLASS_ID_VIRTUAL_DRIVE,
                                         FBE_VIRTUAL_DRIVE_CONTROL_CODE_INVALID,
                                         FBE_VIRTUAL_DRIVE_CONTROL_CODE_LAST, NULL, 0);
    mut_printf(MUT_LOG_TEST_STATUS, "Send usurpers to class PVD\n");
    walle_cleaner_send_usurpers_to_class(FBE_CLASS_ID_PROVISION_DRIVE,
                                         FBE_PROVISION_DRIVE_CONTROL_CODE_INVALID,
                                         FBE_PROVISION_DRIVE_CONTROL_CODE_LAST, NULL, 0);
}

/*!**************************************************************
 * walle_cleaner_test_service_usurper()
 ****************************************************************
 * @brief
 *   Call usurper functions to classes to increase code coverage
 *
 * @param None
 *
 * @return None.
 *
 * @author
 *  10/24/2014 - Created. Jamin Kang
 *
 ****************************************************************/
static void walle_cleaner_test_service_usurper(void)
{
    walle_cleaner_send_usurpers_to_service(FBE_SERVICE_ID_BASE_SERVICE,
                                           FBE_BASE_SERVICE_CONTROL_CODE_INVALID, FBE_BASE_SERVICE_CONTROL_CODE_LAST,
                                           &base_service_mask_opcodes[0], ARRAY_LEN(base_service_mask_opcodes));

    walle_cleaner_test_sleep();
    walle_cleaner_send_usurpers_to_service(FBE_SERVICE_ID_MEMORY,
                                           FBE_MEMORY_SERVICE_CONTROL_CODE_INVALID, FBE_MEMORY_SERVICE_CONTROL_CODE_LAST,
                                           &base_service_mask_opcodes[0], ARRAY_LEN(base_service_mask_opcodes));
    walle_cleaner_test_sleep();

    walle_cleaner_send_usurpers_to_service(FBE_SERVICE_ID_TOPOLOGY,
                                           FBE_TOPOLOGY_CONTROL_CODE_INVALID, FBE_TOPOLOGY_CONTROL_CODE_LAST,
                                           &base_service_mask_opcodes[0], ARRAY_LEN(base_service_mask_opcodes));
    walle_cleaner_test_sleep();

    walle_cleaner_send_usurpers_to_service(FBE_SERVICE_ID_NOTIFICATION,
                                           FBE_NOTIFICATION_CONTROL_CODE_INVALID, FBE_NOTIFICATION_CONTROL_CODE_LAST,
                                           &base_service_mask_opcodes[0], ARRAY_LEN(base_service_mask_opcodes));
    walle_cleaner_test_sleep();


    walle_cleaner_send_usurpers_to_service(FBE_SERVICE_ID_SERVICE_MANAGER,
                                           FBE_SERVICE_MANAGER_CONTROL_CODE_INVALID, FBE_SERVICE_MANAGER_CONTROL_CODE_LAST,
                                           &base_service_mask_opcodes[0], ARRAY_LEN(base_service_mask_opcodes));
    walle_cleaner_test_sleep();


    walle_cleaner_send_usurpers_to_service(FBE_SERVICE_ID_SCHEDULER,
                                           FBE_SCHEDULER_CONTROL_CODE_INVALID, FBE_SCHEDULER_CONTROL_CODE_LAST,
                                           &base_service_mask_opcodes[0], ARRAY_LEN(base_service_mask_opcodes));
    walle_cleaner_test_sleep();


    // No control entry for FBE_SERVICE_ID_TRANSPORT

    walle_cleaner_send_usurpers_to_service(FBE_SERVICE_ID_EVENT_LOG,
                                           FBE_EVENT_LOG_CONTROL_CODE_INVALID, FBE_EVENT_LOG_CONTROL_CODE_LAST,
                                           &base_service_mask_opcodes[0], ARRAY_LEN(base_service_mask_opcodes));
    walle_cleaner_test_sleep();


    walle_cleaner_send_usurpers_to_service(FBE_SERVICE_ID_ENVIRONMENT_LIMIT,
                                           FBE_ENVIRONMENT_LIMIT_CONTROL_CODE_INVALID, FBE_ENVIRONMENT_LIMIT_CONTROL_CODE_SET_LIMITS + 1,
                                           &base_service_mask_opcodes[0], ARRAY_LEN(base_service_mask_opcodes));
    walle_cleaner_test_sleep();


    walle_cleaner_send_usurpers_to_service(FBE_SERVICE_ID_TRACE,
                                           FBE_TRACE_CONTROL_CODE_INVALID, FBE_TRACE_CONTROL_CODE_LAST,
                                           &base_service_mask_opcodes[0], ARRAY_LEN(base_service_mask_opcodes));
    walle_cleaner_test_sleep();


    walle_cleaner_send_usurpers_to_service(FBE_SERVICE_ID_TRAFFIC_TRACE,
                                           FBE_TRAFFIC_TRACE_CONTROL_CODE_INVALID, FBE_TRAFFIC_TRACE_CONTROL_CODE_LAST,
                                           &base_service_mask_opcodes[0], ARRAY_LEN(base_service_mask_opcodes));
    walle_cleaner_test_sleep();


    walle_cleaner_send_usurpers_to_service(FBE_SERVICE_ID_SECTOR_TRACE,
                                           FBE_SECTOR_TRACE_CONTROL_CODE_INVALID, FBE_SECTOR_TRACE_CONTROL_CODE_LAST,
                                           &base_service_mask_opcodes[0], ARRAY_LEN(base_service_mask_opcodes));
    walle_cleaner_test_sleep();


    walle_cleaner_send_usurpers_to_service(FBE_SERVICE_ID_METADATA,
                                           FBE_METADATA_CONTROL_CODE_INVALID, FBE_METADATA_CONTROL_CODE_LAST,
                                           &base_service_mask_opcodes[0], ARRAY_LEN(base_service_mask_opcodes));
    walle_cleaner_test_sleep();


    walle_cleaner_send_usurpers_to_service(FBE_SERVICE_ID_JOB_SERVICE,
                                           FBE_JOB_CONTROL_CODE_INVALID, FBE_JOB_CONTROL_CODE_LAST,
                                           &base_service_mask_opcodes[0], ARRAY_LEN(base_service_mask_opcodes));
    walle_cleaner_test_sleep();


    walle_cleaner_send_usurpers_to_service(FBE_SERVICE_ID_EVENT,
                                           FBE_BASE_SERVICE_CONTROL_CODE_INVALID, FBE_BASE_SERVICE_CONTROL_CODE_LAST,
                                           &base_service_mask_opcodes[0], ARRAY_LEN(base_service_mask_opcodes));
    walle_cleaner_test_sleep();


    walle_cleaner_send_usurpers_to_service(FBE_SERVICE_ID_CMI,
                                           FBE_CMI_CONTROL_CODE_INVALID, FBE_CMI_CONTROL_CODE_LAST,
                                           &base_service_mask_opcodes[0], ARRAY_LEN(base_service_mask_opcodes));
    walle_cleaner_test_sleep();


    walle_cleaner_send_usurpers_to_service(FBE_SERVICE_ID_PERSIST,
                                           FBE_PERSIST_CONTROL_CODE_INVALID, FBE_PERSIST_CONTROL_CODE_LAST,
                                           &base_service_mask_opcodes[0], ARRAY_LEN(base_service_mask_opcodes));
    walle_cleaner_test_sleep();


    walle_cleaner_send_usurpers_to_service(FBE_SERVICE_ID_DATABASE,
                                           FBE_DATABASE_CONTROL_CODE_INVALID, FBE_DATABASE_CONTROL_CODE_LAST,
                                           &base_service_mask_opcodes[0], ARRAY_LEN(base_service_mask_opcodes));
    walle_cleaner_test_sleep();


    walle_cleaner_send_usurpers_to_service(FBE_SERVICE_ID_RAW_MIRROR,
                                           FBE_RAW_MIRROR_SERVICE_CONTROL_CODE_INVALID, FBE_RAW_MIRROR_SERVICE_CONTROL_CODE_LAST,
                                           &base_service_mask_opcodes[0], ARRAY_LEN(base_service_mask_opcodes));
    walle_cleaner_test_sleep();
}

/*!**************************************************************
 * walle_cleaner_test_usurper()
 ****************************************************************
 * @brief
 *   Call usurper functions to increase code coverage
 *
 * @param None
 *
 * @return None.
 *
 * @author
 *  10/24/2014 - Created. Jamin Kang
 *
 ****************************************************************/
static void walle_cleaner_test_usurper(void)
{
    walle_cleaner_test_obj_usurper();
    walle_cleaner_test_class_usurper();
    walle_cleaner_test_service_usurper();
}


/*!**************************************************************
 * walle_cleaner_create_raid_group()
 ****************************************************************
 * @brief
 *   Create a raid group
 *
 * @param
 *   rg_config_p: the configuration of Raid to be created.
 *
 * @return None.
 *
 * @author
 *  10/24/2014 - Created. Jamin Kang
 *
 ****************************************************************/
static void walle_cleaner_create_raid_group(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                        status;
    fbe_object_id_t                     rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_job_service_error_type_t        job_error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    fbe_status_t                        job_status = FBE_STATUS_OK;
	fbe_object_id_t                     rg_object_id_from_job = FBE_OBJECT_ID_INVALID;
    fbe_sim_transport_connection_target_t orig_target;

    orig_target = fbe_api_sim_transport_get_target_server();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Create a RAID Group ===\n");
    fbe_test_wait_for_rg_pvds_ready(rg_config_p);
    status = fbe_test_sep_util_create_raid_group(rg_config_p);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "RG creation failed");

    /* wait for notification from job service. */
    status = fbe_api_common_wait_for_job(rg_config_p->job_number,
                                         120000,
                                         &job_error_code,
                                         &job_status,
                                         &rg_object_id_from_job);
    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to created RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    /* Verify the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
	MUT_ASSERT_INT_EQUAL(rg_object_id, rg_object_id_from_job);

    /* Verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id,
                                                     FBE_LIFECYCLE_STATE_READY, 20000,
                                                     FBE_PACKAGE_ID_SEP_0);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Created Raid Group object %d\n", rg_object_id);
    /* Wait RAID object to be ready on SPB */
    if (fbe_test_sep_util_get_dualsp_test_mode()) {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
		/* Verify the object id of the raid group */
		status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
		MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
		MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
		/* Verify the raid group get to ready state in reasonable time */
		status = fbe_api_wait_for_object_lifecycle_state(rg_object_id,
														 FBE_LIFECYCLE_STATE_READY, 20000,
														 FBE_PACKAGE_ID_SEP_0);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		mut_printf(MUT_LOG_TEST_STATUS, "Created Raid Group object on SPB %d\n", rg_object_id);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }
    fbe_api_sim_transport_set_target_server(orig_target);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: END\n", __FUNCTION__);
}

/*!**************************************************************
 * walle_cleaner_create_all_raid_groups()
 ****************************************************************
 * @brief
 *   Create a raid group
 *
 * @param None
 *
 * @return None.
 *
 * @author
 *  10/24/2014 - Created. Jamin Kang
 *
 ****************************************************************/
static void walle_cleaner_create_all_raid_groups(void)
{
    fbe_test_sep_util_fill_lun_configurations_rounded(walle_cleaner_raid_group_config,
                                                      WALLE_CLEANER_RAID_GROUP_COUNT,
                                                      WALLE_CLEANER_CHUNKS_PER_LUN,
                                                      WALLE_CLEANER_LUNS_PER_RG);
    fbe_test_sep_util_create_raid_group_configuration(walle_cleaner_raid_group_config,
                                                      WALLE_CLEANER_RAID_GROUP_COUNT);
}


void walle_cleaner_test(void)
{
    fbe_status_t status;

    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: wait for database\n", __FUNCTION__);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_test_sep_util_wait_for_database_service(60000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_test_sep_util_wait_for_database_service(60000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_test_wait_for_all_pvds_ready();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    walle_cleaner_create_all_raid_groups();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    walle_cleaner_test_fbe_api();
    walle_cleaner_test_usurper();
}

/*!**************************************************************
 * walle_cleaner_setup()
 ****************************************************************
 * @brief
 *  Setup for a pippin test.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  10/24/2014 - Created. Jamin Kang
 *
 ****************************************************************/
void walle_cleaner_setup(void)
{
    /* Only load the physical config in simulation.
    */
    if (fbe_test_util_is_simulation()) {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups;

        rg_config_p = &walle_cleaner_raid_group_config[0];
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
        fbe_test_sep_util_update_extra_chunk_size(rg_config_p, WALLE_CLEANER_CHUNKS_PER_LUN * 2);
        /* initialize the number of extra drive required by each rg
          */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_sep_and_neit_both_sps();
    }

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();

    return;
}

/*!**************************************************************
 * walle_cleaner_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the walle_cleaner test.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  10/24/2014 - Created. Jamin Kang
 *
 ****************************************************************/
void walle_cleaner_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    if (fbe_test_util_is_simulation()) {
        /* First execute teardown on SP A then on SP B
        */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        /* clear the error counter */
        fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
        fbe_test_sep_util_destroy_neit_sep_physical();

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        /* clear the error counter */
        fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
        fbe_test_sep_util_destroy_neit_sep_physical();

    }
    return;
}
/******************************************
 * end walle_cleaner_cleanup()
 ******************************************/
/*************************
 * end file walle_cleaner_test.c
 *************************/


