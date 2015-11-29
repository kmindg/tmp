/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file lurker_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains paged metadata incomplete write test for degraded raid objects.
 *
 * @version
 *   12/09/2013 - Created. Jamin Kang
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <mut.h>
#include <sep_tests.h>
#include <fbe/fbe_api_sim_server.h>
#include <fbe/fbe_random.h>
#include <fbe_test_package_config.h>
#include <fbe_test_configurations.h>
#include <fbe_test_common_utils.h>
#include <fbe/fbe_api_trace_interface.h>
#include <fbe/fbe_api_discovery_interface.h>
#include <fbe_test.h>
#include <sep_rebuild_utils.h>
#include <sep_hook.h>
#include <fbe/fbe_api_database_interface.h>
#include <fbe/fbe_api_metadata_interface.h>
#include <fbe/fbe_api_raid_group_interface.h>
#include <fbe/fbe_api_rdgen_interface.h>
#include <fbe/fbe_api_raid_group_interface.h>
#include <fbe/fbe_api_utils.h>
#include <fbe/fbe_api_panic_sp_interface.h>
#include <fbe/fbe_api_event_log_interface.h>
#include <fbe/fbe_api_provision_drive_interface.h>
#include <pp_utils.h>
#include <sep_utils.h>

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
const char *lurker_short_desc = "Test paged metadata incomplete write for degrade raid group";
const char *lurker_long_desc ="\
The lurker_test senario tests paged metadata incomplete write in degraded raid group\n\
\n\
After bring up the initial topology for parity Raid Groups SPs (SPA == Active)\n\
The test follows steps:\n\
Step 1: Remove some disk(s) to create degraded RG.\n\
        -R6: remove 1st 2 non-parity drives\n\
Step 2: Wait until paged metadata was rebuilt, and some area on user data was rebuilt.\n\
Step 3: Inject incomplete write on paged metadata and these user data area.\n\
Step 4: Reboot SP.\n\
Step 5: Verify that we don't get uncorrectable errors.\n\
\n\
Description Last Updated: 12/09/2013\n\
\n";


/*!*******************************************************************
 * @def LURKER_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief luns per rg for the extended test.
 *
 *********************************************************************/
#define LURKER_LUNS_PER_RAID_GROUP 1

/*!*******************************************************************
 * @def LURKER_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define LURKER_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def LURKER_IW_PBA
 *********************************************************************
 * @brief The PBA we will inject incomplete write with Journal
 *
 *********************************************************************/
#define LURKER_IW_PBA (0)
#define LURKER_IW_PBA2 (0x800)

/*!*******************************************************************
 * @def LURKER_REBUILD_PAUSE_PBA
 *********************************************************************
 * @brief The PBA we will pause rebuild
 *
 *********************************************************************/
#define LURKER_REBUILD_PAUSE_PBA (0x1000)
#define LURKER_REBUILD_PAUSE_PBA_2 (LURKER_REBUILD_PAUSE_PBA + 0x800)
/*!*******************************************************************
 * @def LURKER_MARK_NR_PBA
 *********************************************************************
 * @brief Send IO on this PBA to mark NR in paged metadata
 *
 *********************************************************************/
#define LURKER_MARK_NR_PBA  (LURKER_REBUILD_PAUSE_PBA)
#define LURKER_MARK_NR_PBA2  (LURKER_REBUILD_PAUSE_PBA + 0x800)

static fbe_test_rg_configuration_t lurker_raid_group_config[] = {
    /* width, capacity     raid type,                  class,                  block size      RAID-id.   bandwidth.*/
    {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            0,         0},
    {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            1,         0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

static fbe_u32_t lurker_object_debug_flags =
    FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING |
    FBE_RAID_GROUP_DEBUG_FLAG_VERIFY_TRACING |
    FBE_RAID_GROUP_DEBUG_FLAGS_ERROR_TRACE_MONITOR_IOTS_ERROR |
    FBE_RAID_GROUP_DEBUG_FLAGS_IOTS_TRACING;

fbe_u32_t lurker_rebuild_pause_pba;
fbe_u32_t lurker_rebuild_pause_pba_2;
fbe_u32_t lurker_mark_nr_pba;
fbe_u32_t lurker_mark_nr_pba_2;

static void lurker_setup_variables(void)
{
    lurker_rebuild_pause_pba = (fbe_test_sep_util_get_chunks_per_rebuild()*2)*0x800;
    lurker_rebuild_pause_pba_2 = (fbe_test_sep_util_get_chunks_per_rebuild()*3)*0x800;
    lurker_mark_nr_pba = lurker_rebuild_pause_pba;
    lurker_mark_nr_pba_2 = lurker_rebuild_pause_pba_2;
}

/*!**************************************************************
 * lurker_get_removed_physical_objects_number()
 ****************************************************************
 * @brief
 *  Get number of physical objects that will be removed when we
 *  remove a drive.
 *  We use sizeof raid_group nonpaged metadata to distinguish SP3
 *  and MR1
 *
 * @param rg_config_p - the raid group configuration to test
 *
 * @return None.
 *
 * @author
 *  12/09/2013 - Created. Jamin Kang
 *
 ****************************************************************/
static fbe_u32_t lurker_get_removed_physical_objects_number(void)
{
    fbe_u32_t rg_np_size;
    const fbe_u32_t SP3_rg_np_size = 167;

    rg_np_size = sizeof(fbe_raid_group_nonpaged_metadata_t);
    mut_printf(MUT_LOG_TEST_STATUS,
               "%s: rg_np_size: %u\n", __FUNCTION__, rg_np_size);
    /**
     * In SP3, nonpaged_size is 167.
     * We changed raid group metadata size in MR1.
     */
    if (rg_np_size > SP3_rg_np_size) {
        return 1;
    } else {
        return 2;
    }
}

/*!**************************************************************
 * lurker_get_removed_drives_number()
 ****************************************************************
 * @brief
 *  Get number of drives we remove in test
 *
 * @param rg_config_p - the raid group configuration to test
 *
 * @return None.
 *
 * @author
 *  12/09/2013 - Created. Jamin Kang
 *
 ****************************************************************/
static fbe_u32_t lurker_get_removed_drives_number(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t drives_to_remove = 0;

    switch (rg_config_p->raid_type) {
    case FBE_RAID_GROUP_TYPE_RAID3:
    case FBE_RAID_GROUP_TYPE_RAID5:
        drives_to_remove = 1;
        break;

    case FBE_RAID_GROUP_TYPE_RAID6:
        drives_to_remove = 2;
        break;
    default:
        MUT_FAIL();
        break;
    }
    return drives_to_remove;
}

/*!**************************************************************
 * lurker_determine_drives_to_remove()
 ****************************************************************
 * @brief
 *  Setup the position we will remove in test.
 *  We choose these positions that we don't inject incomplete write
 *  on them.
 *
 * @param rg_config_p - the raid group configuration to test
 * @num_to_remove - the number of drive to remove
 *
 * @return None.
 *
 * @author
 *  12/09/2013 - Created. Jamin Kang
 *
 ****************************************************************/
static void lurker_determine_drives_to_remove(fbe_test_rg_configuration_t *rg_config_p,
                                              fbe_u32_t *num_to_remove)
{
    fbe_u32_t drives_to_remove = 0;

    switch (rg_config_p->raid_type) {
    case FBE_RAID_GROUP_TYPE_RAID3:
    case FBE_RAID_GROUP_TYPE_RAID5:
        drives_to_remove = 1;
        /* Parity pos on first chunk: 0, on metadata chunk: 2.
         * We inject iwr on: first chunk: 5, metadata chunk: 1
         */
        rg_config_p->specific_drives_to_remove[0] = 3;
        break;

    case FBE_RAID_GROUP_TYPE_RAID6:
        drives_to_remove = 2;
        /* Parity pos on first chunk: 0, 1, on metadata chunk: 4, 5.
         * We inject iwr on: first chunk: 5, metadata chunk: 3
         */
        rg_config_p->specific_drives_to_remove[0] = 1;
        rg_config_p->specific_drives_to_remove[1] = 2;
        break;

    default:
        MUT_FAIL();
        break;
    }
    *num_to_remove = drives_to_remove;
}

/*!**************************************************************
 * lurker_get_rebuild_position()
 ****************************************************************
 * @brief
 *  Return a position that will be rebuilt
 *
 * @param rg_config_p - the raid group configuration to test
 *
 * @return A position will be rebuilt
 *
 * @author
 *  12/11/2013 - Created. Jamin Kang
 *
 ****************************************************************/
static fbe_u32_t lurker_get_rebuild_position(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t position = 0;

    switch (rg_config_p->raid_type) {
    case FBE_RAID_GROUP_TYPE_RAID3:
    case FBE_RAID_GROUP_TYPE_RAID5:
        position = 3;
        break;

    case FBE_RAID_GROUP_TYPE_RAID6:
        position = 1;
        break;

    default:
        MUT_FAIL();
        break;
    }
    return position;
}


/*!**************************************************************
 * lurker_remove_all_disks()
 ****************************************************************
 * @brief
 *  Removed all disks to make the raid group fail and abort all IOs.
 *
 * @param rg_config - the raid group
 *
 * @return None
 *
 * @author
 *  12/18/2013 - Created. Jamin Kang
 *
 ****************************************************************/
static void lurker_remove_all_disks(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;
    fbe_u32_t i;
    fbe_object_id_t rg_object_id;
    fbe_object_id_t pvd_object_id;

    /* Get object-id of the RAID group. */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    for (i = 0; i < 3; i++) {
        fbe_u32_t in_position;

        in_position = i;

        /* Pull drive quickly!!!. Make sure the RAID group has no chance to mark NR */
        status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[in_position].bus,
                                                                rg_config_p->rg_disk_set[in_position].enclosure,
                                                                rg_config_p->rg_disk_set[in_position].slot,
                                                                &pvd_object_id);
        status = fbe_test_pp_util_pull_drive(rg_config_p->rg_disk_set[in_position].bus,
                                             rg_config_p->rg_disk_set[in_position].enclosure,
                                             rg_config_p->rg_disk_set[in_position].slot,
                                             &rg_config_p->drive_handle[i]);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    mut_printf(MUT_LOG_TEST_STATUS, "%s Succeed\n", __FUNCTION__);
}

/*!**************************************************************
 * lurker_reinsert_all_disks()
 ****************************************************************
 * @brief
 *  Reinsert the drives we removed
 *
 * @param rg_config_p - the raid group configuration to test
 *
 * @return None.
 *
 * @author
 *  12/18/2013 - Created. Jamin Kang
 *
 ****************************************************************/
static void lurker_reinsert_all_disks(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t i;
    fbe_status_t status;
    fbe_object_id_t rg_object_id;
    fbe_object_id_t pvd_object_id;

    /* Get object-id of the RAID group. */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    for (i = 0; i < 3; i++) {
        fbe_u32_t in_position;

        in_position = i;
        status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[in_position].bus,
                                                                rg_config_p->rg_disk_set[in_position].enclosure,
                                                                rg_config_p->rg_disk_set[in_position].slot,
                                                                &pvd_object_id);
        status = fbe_test_pp_util_reinsert_drive(rg_config_p->rg_disk_set[in_position].bus,
                                                 rg_config_p->rg_disk_set[in_position].enclosure,
                                                 rg_config_p->rg_disk_set[in_position].slot,
                                                 rg_config_p->drive_handle[i]);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
	status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY,
                                                     30000, FBE_PACKAGE_ID_SEP_0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "%s Succeed\n", __FUNCTION__);
}

/*!**************************************************************
 * lurker_reinsert_drives()
 ****************************************************************
 * @brief
 *  Reinsert the drives we removed
 *
 * @param rg_config_p - the raid group configuration to test
 *
 * @return None.
 *
 * @author
 *  12/09/2013 - Created. Jamin Kang
 *
 ****************************************************************/
static void lurker_reinsert_drives(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t drives_removed;
    fbe_u32_t i;
    fbe_u32_t physical_objects;
    fbe_status_t status;
    fbe_u32_t objects_to_add;

    objects_to_add = lurker_get_removed_physical_objects_number();

    status = fbe_api_get_total_objects(&physical_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Error getting object number");
    drives_removed = lurker_get_removed_drives_number(rg_config_p);
    for (i = 0; i < drives_removed; i++) {
        physical_objects += objects_to_add;
        sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p,
                                                    rg_config_p->specific_drives_to_remove[i],
                                                    physical_objects,
                                                    &rg_config_p->drive_handle[i]);

    }
}

static void lurker_del_debug_hook(fbe_object_id_t rg_object_id, fbe_lba_t lba)
{
    fbe_status_t status = FBE_STATUS_OK;

    status = fbe_test_del_debug_hook_active(rg_object_id,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                            FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                            lba,
                                            0,
                                            SCHEDULER_CHECK_VALS_GT,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}

static void lurker_add_debug_hook(fbe_object_id_t rg_object_id, fbe_lba_t lba)
{
    fbe_status_t status = FBE_STATUS_OK;

    status = fbe_test_add_debug_hook_active(rg_object_id,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                            FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                            lba,
                                            0,
                                            SCHEDULER_CHECK_VALS_GT,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}

static void lurker_wait_debug_hook(fbe_object_id_t rg_object_id, fbe_lba_t lba)
{
    fbe_status_t status;

    status = fbe_test_wait_for_debug_hook(rg_object_id,
                                          SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                          FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                          SCHEDULER_CHECK_VALS_GT,
                                          SCHEDULER_DEBUG_ACTION_PAUSE,
                                          lba,
                                          0);
}

/*!**************************************************************
 * lurker_reinsert_and_wait_rebuilt()
 ****************************************************************
 * @brief
 *  Reinsert drive and wait rebuilt to specific checkpoint
 *
 * @param rg_config_p - the raid group configuration to test
 * @param rebuld_checkpoint - return the checkpoint we pause at
 *
 * @return None
 *
 * @author
 *  12/09/2013 - Created. Jamin Kang
 *
 ****************************************************************/
static void lurker_reinsert_and_wait_rebuilt(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t rg_object_id;
    fbe_lba_t       checkpoint_to_pause_at =  lurker_rebuild_pause_pba;

    /* Lookup RG object ID and save them locally */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    lurker_add_debug_hook(rg_object_id, checkpoint_to_pause_at);
    lurker_reinsert_drives(rg_config_p);
    lurker_wait_debug_hook(rg_object_id, checkpoint_to_pause_at);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: rebuild checkpoint: %llx\n",
               __FUNCTION__, (unsigned long long)checkpoint_to_pause_at);
}

/*!**************************************************************
 * lurker_get_pvd_offset()
 ****************************************************************
 * @brief
 *  Get downstream edge offset of raid group
 *
 * @param rg_object_id - the raid group object id
 *
 * @return pvd offset lba
 *
 * @author
 *  12/09/2013 - Created. Jamin Kang
 *
 ****************************************************************/
static fbe_lba_t lurker_get_pvd_offset(fbe_object_id_t rg_object_id)
{
    fbe_raid_group_map_info_t rg_map;
    fbe_status_t status;

    memset(&rg_map, 0, sizeof(rg_map));
    rg_map.lba = 0;
    status = fbe_api_raid_group_map_lba(rg_object_id, &rg_map);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Map (%u %llx) to %llx, offset: %llx\n",
               __FUNCTION__, rg_map.position, rg_map.pba, rg_map.lba, rg_map.offset);
    return rg_map.offset;
}

/*!**************************************************************
 * lurker_is_parity_position()
 ****************************************************************
 * @brief
 *  Test if a position is parity position on specified PBA
 *
 * @param rg_object_id - the raid group object id
 * @param position - the position to test
 * @param pba - the pba to test
 *
 * @return True if it is parity position; else False
 *
 * @author
 *  12/09/2013 - Created. Jamin Kang
 *
 ****************************************************************/
static fbe_bool_t lurker_is_parity_position(fbe_object_id_t rg_object_id,
                                            fbe_u32_t position, fbe_lba_t pba)
{
    fbe_raid_group_map_info_t rg_map;
    fbe_status_t status;

    memset(&rg_map, 0, sizeof(rg_map));
    rg_map.pba = pba;
    rg_map.position = position;
    status = fbe_api_raid_group_map_pba(rg_object_id, &rg_map);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (rg_map.data_pos != rg_map.position) {
        return FBE_TRUE;
    } else {
        return FBE_FALSE;
    }
}

/*!**************************************************************
 * lurker_find_data_position()
 ****************************************************************
 * @brief
 *  Find a data position according to pba
 *
 * @param rg_object_id - the raid group object id
 * @param pba - the pba to test
 *
 * @return A data position
 *
 * @author
 *  12/09/2013 - Created. Jamin Kang
 *
 ****************************************************************/
static fbe_u32_t lurker_find_data_position(fbe_object_id_t rg_object_id,
                                           fbe_lba_t pba)
{
    fbe_u32_t pos;

    for (pos = 0; pos < FBE_RAID_MAX_DISK_ARRAY_WIDTH; pos++) {
        if (!lurker_is_parity_position(rg_object_id, pos, pba)) {
            return pos;
        }
    }
    /* Can't find any valid position. Fail here */
    MUT_FAIL_MSG("Can't find data position");
    return 0;
}

/*!**************************************************************
 * lurker_map_pba_to_lba()
 ****************************************************************
 * @brief
 *  Map pba to lba. We choose a data position to do the mapping.
 *
 * @param rg_object_id - the raid group object id
 * @param pba - the pba to test
 * @param out_position - the data position we choose
 * @param out_lba - the mapped lba
 *
 * @return None
 *
 * @author
 *  12/09/2013 - Created. Jamin Kang
 *
 ****************************************************************/
static void lurker_map_pba_to_lba(fbe_object_id_t rg_object_id,
                                  fbe_lba_t pba,
                                  fbe_u32_t *out_position,
                                  fbe_lba_t *out_lba)
{
    fbe_raid_group_map_info_t rg_map;
    fbe_u32_t position;
    fbe_status_t status;

    position = lurker_find_data_position(rg_object_id, pba);
    memset(&rg_map, 0, sizeof(rg_map));
    rg_map.pba = pba;
    rg_map.position = position;
    status = fbe_api_raid_group_map_pba(rg_object_id, &rg_map);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Map (%u %llx) to %llx, offset: %llx\n",
               __FUNCTION__, rg_map.position, (unsigned long long)rg_map.pba,
               (unsigned long long)rg_map.lba,
               (unsigned long long)rg_map.offset);
    if (out_position) {
        *out_position = position;
    }
    if (out_lba) {
        *out_lba = rg_map.lba;
    }
}

/*!**************************************************************
 * lurker_send_io_async()
 ****************************************************************
 * @brief
 *  Run IO on lba
 *
 * @param rg_object_id - the raid group object id
 * @param lba - the lba to run IO
 * @param in_rdgen_op - Rdgen operation
 *
 * @return None
 *
 * @author
 *  12/09/2013 - Created. Jamin Kang
 *
 ****************************************************************/
static void lurker_send_io_async(fbe_api_rdgen_context_t *context,
                                 fbe_object_id_t rg_object_id,
                                 fbe_lba_t lba,
                                 fbe_rdgen_operation_t in_rdgen_op)
{
    fbe_status_t status;

    status = fbe_api_rdgen_send_one_io_asynch(context,
                                              rg_object_id,
                                              FBE_CLASS_ID_INVALID,
                                              FBE_PACKAGE_ID_SEP_0,
                                              in_rdgen_op,
                                              FBE_RDGEN_LBA_SPEC_FIXED,
                                              lba,
                                              2,
                                              FBE_RDGEN_OPTIONS_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

/*!**************************************************************
 * lurker_send_io()
 ****************************************************************
 * @brief
 *  Run IO on lba
 *
 * @param rg_object_id - the raid group object id
 * @param lba - the lba to run IO
 * @param in_rdgen_op - Rdgen operation
 *
 * @return None
 *
 * @author
 *  12/09/2013 - Created. Jamin Kang
 *
 ****************************************************************/
static void lurker_send_io(fbe_object_id_t rg_object_id,
                           fbe_lba_t lba,
                           fbe_rdgen_operation_t in_rdgen_op)
{
    fbe_api_rdgen_context_t context;
    fbe_status_t status;
    fbe_api_rdgen_send_one_io_params_t params;

    fbe_api_rdgen_io_params_init(&params);
    params.object_id = rg_object_id;
    params.class_id = FBE_CLASS_ID_PARITY;
    params.package_id = FBE_PACKAGE_ID_SEP_0;
    params.msecs_to_abort = 0;
    params.msecs_to_expiration = 0;
    params.block_spec = FBE_RDGEN_BLOCK_SPEC_CONSTANT;
    params.rdgen_operation = FBE_RDGEN_OPERATION_WRITE_ONLY;
    params.pattern = FBE_RDGEN_PATTERN_LBA_PASS;
    params.lba = lba;
    params.blocks = 1;
    params.b_async = FBE_FALSE;     /* We only do sync IO in lurker */
    params.options = FBE_RDGEN_OPTIONS_INVALID;
    status = fbe_api_rdgen_send_one_io_params(&context, &params);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

/*!**************************************************************
 * lurker_mark_nr_on_pba()
 ****************************************************************
 * @brief
 *  Mark need rebuild according to PBA.
 *  We write a block to one of the data disk to mark NR on this chunk.
 *
 * @param rg_object_id - the raid group object id
 * @param pba - the pba to mark NR
 *
 * @return None
 *
 * @author
 *  12/09/2013 - Created. Jamin Kang
 *
 ****************************************************************/
static void lurker_mark_nr_on_pba(fbe_object_id_t rg_object_id, fbe_lba_t pba)
{
    fbe_lba_t pvd_offset;
    fbe_lba_t io_lba;

    mut_printf(MUT_LOG_TEST_STATUS, "%s enttry\n", __FUNCTION__);
    pvd_offset = lurker_get_pvd_offset(rg_object_id);
    lurker_map_pba_to_lba(rg_object_id, pba + pvd_offset, NULL, &io_lba);
    mut_printf(MUT_LOG_TEST_STATUS,
               "%s: Send io on 0x%llx\n",
               __FUNCTION__, (unsigned long long)io_lba);
    lurker_send_io(rg_object_id, io_lba, FBE_RDGEN_OPERATION_WRITE_ONLY);
}

/*!**************************************************************
 * lurker_degraded_raid_group()
 ****************************************************************
 * @brief
 *  Removed 1/2 disks to make the raid group degraded.
 *  We also run IO on some chunks to mark NR.
 *
 * @param rg_config - the raid group
 *
 * @return None
 *
 * @author
 *  12/09/2013 - Created. Jamin Kang
 *
 ****************************************************************/
static void lurker_degraded_raid_group(fbe_test_rg_configuration_t *rg_config)
{
    fbe_status_t status;
    fbe_u32_t physical_objects;
    fbe_u32_t i;
    fbe_u32_t drives_to_remove;
    fbe_u32_t objects_to_remove;
    fbe_object_id_t rg_object_id;

    mut_printf(MUT_LOG_TEST_STATUS, "%s Entry\n", __FUNCTION__);
    /* Lookup RG object ID and save them locally */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_raid_group_set_group_debug_flags(rg_object_id, lurker_object_debug_flags);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    objects_to_remove = lurker_get_removed_physical_objects_number();
    lurker_determine_drives_to_remove(rg_config, &drives_to_remove);
    status = fbe_api_get_total_objects(&physical_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Error getting object number");

    for (i = 0; i < drives_to_remove; i++) {
        fbe_u32_t degraded_position;

        physical_objects -= objects_to_remove;
        degraded_position = rg_config->specific_drives_to_remove[i];
        sep_rebuild_utils_remove_drive_and_verify(rg_config, degraded_position,
                                                  physical_objects,
                                                  &rg_config->drive_handle[i]);
        sep_rebuild_utils_verify_rb_logging(rg_config, degraded_position);
        sep_rebuild_utils_check_for_reb_restart(rg_config, degraded_position);
        mut_printf(MUT_LOG_TEST_STATUS, "Drive removed: %d_%d_%d\n",
                   rg_config->rg_disk_set[degraded_position].bus,
                   rg_config->rg_disk_set[degraded_position].enclosure,
                   rg_config->rg_disk_set[degraded_position].slot);

    }

	status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY,
                                                     30000, FBE_PACKAGE_ID_SEP_0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Send some IO to mark NR in paged metadata */
    lurker_mark_nr_on_pba(rg_object_id, LURKER_IW_PBA);
    lurker_mark_nr_on_pba(rg_object_id, LURKER_IW_PBA2);
    lurker_mark_nr_on_pba(rg_object_id, lurker_mark_nr_pba);
    //lurker_mark_nr_on_pba(rg_object_id, LURKER_MARK_NR_PBA2);
    lurker_mark_nr_on_pba(rg_object_id, lurker_mark_nr_pba_2);//LURKER_MARK_NR_PBA+0x800*fbe_test_sep_util_get_chunks_per_rebuild() + 0x800);
    mut_printf(MUT_LOG_TEST_STATUS, "%s Succeed\n", __FUNCTION__);
}

/*!**************************************************************
 * lurker_wait_iw_io_match()
 ****************************************************************
 * @brief
 *  Wait unit expected incomplete write errors hit.
 *
 * @param expected_errors - the number of injected errors.
 *
 * @return None
 *
 * @author
 *  12/09/2013 - Created. Jamin Kang
 *
 ****************************************************************/
static void lurker_wait_iw_io_match(fbe_u32_t expected_errors)
{
    fbe_u32_t max_wait_time = 10000;
    fbe_u32_t wait_time = 200;
    fbe_u32_t wait_count = max_wait_time / wait_time;

    for (; wait_count; wait_count -= 1) {
        fbe_api_logical_error_injection_get_stats_t stats;
        fbe_status_t status;

        status = fbe_api_logical_error_injection_get_stats(&stats);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK );
        if (stats.num_errors_injected >= expected_errors) {
            mut_printf(MUT_LOG_TEST_STATUS, "%s: en: %u, num injected: %llu, num object: %u, num records: %u\n",
                       __FUNCTION__,
                       stats.b_enabled, (unsigned long long)stats.num_errors_injected,
                       stats.num_objects_enabled, stats.num_records);
            return;
        }
        fbe_api_sleep(wait_time);
    }
    MUT_FAIL_MSG("Wait LEI failed");
}

/*!**************************************************************
 * lurker_inject_iw_io_on_lba()
 ****************************************************************
 * @brief
 *  Inject incomplete write on specificed LBA.
 *  We use SILENT_DROP to simulate the incomplete write.
 *
 * @param rg_object_id - the raid group object id
 * @param lba - the lba to inject
 *
 * @return None
 *
 * @author
 *  12/09/2013 - Created. Jamin Kang
 *
 ****************************************************************/
static void lurker_inject_iw_io_on_lba(fbe_object_id_t rg_object_id,
                                       fbe_lba_t lba)
{
    fbe_status_t status;
    fbe_raid_group_map_info_t rg_map;
    fbe_lba_t pba;
    fbe_api_logical_error_injection_record_t error_record = {
        0x1,  /* pos_bitmap */
        0x10, /* width */
        0x0,  /* lba */
        0x80,  /* blocks */
        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INCOMPLETE_WRITE,
        FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS, /* error mode */
        0x0,  /* error count */
        0x100,
        0x0,  /* skip count */
        0x0 , /* skip limit */
        0x1,  /* error adjcency */
        0x0,  /* start bit */
        0x0,  /* number of bits */
        0x0,  /* erroneous bits */
        0x0,  /* crc detectable */
    };

    memset(&rg_map, 0, sizeof(rg_map));
    rg_map.lba = lba;
    status = fbe_api_raid_group_map_lba(rg_object_id, &rg_map);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    pba = rg_map.pba - rg_map.offset;
    mut_printf(MUT_LOG_TEST_STATUS,
               "%s: Inject iw IO on %u offset: 0x%llx, ppos: %u\n",
               __FUNCTION__, rg_map.data_pos, (unsigned long long)pba,
               rg_map.parity_pos);
    error_record.pos_bitmap = 1 << rg_map.data_pos;
    error_record.lba = pba;
    status = fbe_api_logical_error_injection_create_record(&error_record);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

/*!**************************************************************
 * lurker_inject_iw_io()
 ****************************************************************
 * @brief
 *  Inject incomplete write on raid group
 *  We inject 2 incomplete write here:
 *      1. lba 0 on position 0.
 *      2. metadata of lba that beyond the rebuild checkpoint
 *
 * @param rg_config_p - the raid group
 *
 * @return None
 *
 * @author
 *  12/09/2013 - Created. Jamin Kang
 *
 ****************************************************************/
static void lurker_inject_iw_io(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_lba_t user_iw_lba = 0;
    fbe_status_t status;
    fbe_object_id_t rg_object_id;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_api_rdgen_context_t context;

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Disable LEI and clear all records */
    status = fbe_api_logical_error_injection_disable_object(rg_object_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_disable_records(0, 255);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Inject iw on metadata */
    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info,
                                         FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Inject iw error on metadata */
    lurker_inject_iw_io_on_lba(rg_object_id, rg_info.paged_metadata_start_lba);
    /* Inject iw error on first chunk */
    lurker_inject_iw_io_on_lba(rg_object_id, user_iw_lba);

    /* Enable LEI on this raid group */
    status = fbe_api_logical_error_injection_enable_object(rg_object_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Let background service rebuild on a chunk, so we can get an IW here */
    lurker_add_debug_hook(rg_object_id, lurker_rebuild_pause_pba_2);
    lurker_del_debug_hook(rg_object_id, lurker_rebuild_pause_pba);

    lurker_send_io_async(&context, rg_object_id, user_iw_lba, FBE_RDGEN_OPERATION_WRITE_ONLY);
    lurker_wait_iw_io_match(2);

    lurker_wait_debug_hook(rg_object_id, lurker_rebuild_pause_pba_2);
    status = fbe_api_scheduler_add_debug_hook(rg_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_PENDING,
                                              FBE_RAID_GROUP_SUBSTATE_PENDING,
                                              0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
    lurker_remove_all_disks(rg_config_p);
    status = fbe_api_logical_error_injection_disable_object(rg_object_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );
    lurker_del_debug_hook(rg_object_id, lurker_rebuild_pause_pba_2);

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for rg obj: 0x%x to Fail", rg_object_id);
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_PENDING_FAIL, FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_scheduler_del_debug_hook(rg_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_PENDING,
                                              FBE_RAID_GROUP_SUBSTATE_PENDING,
                                              0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_FAIL, FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for async issued I/O to fail");
    fbe_api_rdgen_wait_for_ios(&context, FBE_PACKAGE_ID_SEP_0, 1);
    status = fbe_api_rdgen_test_context_destroy(&context);

    mut_printf(MUT_LOG_TEST_STATUS, "Wait done");
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

/*!**************************************************************
 * lurker_reboot()
 ****************************************************************
 * @brief
 *   Reboot the SP
 *
 * @param None
 *
 * @return None
 *
 * @author
 *  12/09/2013 - Created. Jamin Kang
 *
 ****************************************************************/
static void lurker_reboot(void)
{

    fbe_status_t status;

    fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
    fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_NEIT);
    /*reboot sep*/
    mut_printf(MUT_LOG_TEST_STATUS, "%s: destroy sep\n",
               __FUNCTION__);
    fbe_test_sep_util_destroy_sep();
    mut_printf(MUT_LOG_TEST_STATUS, "%s: reload sep\n",
               __FUNCTION__);
    sep_config_load_sep();
    mut_printf(MUT_LOG_TEST_STATUS, "%s: wait for database\n",
               __FUNCTION__);
    status = fbe_test_sep_util_wait_for_database_service(60000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

/*!**************************************************************
 * lurker_wait_for_rebuild_complete()
 ****************************************************************
 * @brief
 *   Wait until rebuild complete
 *
 * @param rg_config_p - the raid group to wait
 *
 * @return None
 *
 * @author
 *  12/09/2013 - Created. Jamin Kang
 *
 ****************************************************************/
static void lurker_wait_for_rebuild_complete(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_object_id_t rg_object_id;
    fbe_status_t status;
    fbe_u32_t position;

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY,
                                                     60000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    position = lurker_get_rebuild_position(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: wait for %u rebuild\n",
               __FUNCTION__, position);
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, position);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: wait done\n",
               __FUNCTION__);
}

/*!**************************************************************
 * lurker_verify()
 ****************************************************************
 * @brief
 *   Seach event log to make sure we don't get Uncorrectable errors.
 *
 * @param None
 *
 * @return None
 *
 * @author
 *  12/09/2013 - Created. Jamin Kang
 *
 ****************************************************************/
static void lurker_verify(void)
{
    fbe_status_t status;
    fbe_u32_t msg_count;

    /* We only inject 1 incomplete write, so shouldn't get uncorrectable errors */
    status = fbe_test_utils_get_event_log_error_count(SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, &msg_count);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL_MSG(msg_count, 0, "Unexpected Uncorrectable Sector Message found\n");
}

/*!**************************************************************
 * lurker_md_incomplete_write_test()
 ****************************************************************
 * @brief
 *  Entry of the lurker single SP test
 *
 * @param rg_config_p - the raid group configuration to test
 *
 * @return None.
 *
 * @author
 *  12/09/2013 - Created. Jamin Kang
 *
 ****************************************************************/
static void lurker_md_incomplete_write_test(fbe_test_rg_configuration_t *rg_config_p, void *contex)
{
    fbe_u32_t raid_group_count = 0;
    fbe_u32_t i;

    lurker_setup_variables();
    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "===== %s Entry =====\n", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "~~~~~ Step 1: Degrade Raid Group. ~~~~~\n");
    for (i = 0; i < raid_group_count; i++) {
        lurker_degraded_raid_group(&rg_config_p[i]);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "~~~~~ Step 2: Wait first chunk to be rebuilt\n");
    for (i = 0; i < raid_group_count; i++) {
        lurker_reinsert_and_wait_rebuilt(&rg_config_p[i]);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "~~~~~ Step 3: Inject incomplete write on first chunk and metadata\n");
    for (i = 0; i < raid_group_count; i++) {
        lurker_inject_iw_io(&rg_config_p[i]);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "~~~~~ Step 4: Reboot SP\n");
    /* Reboot the sp to let RG go to specilize state and do verify on metadata. */
    lurker_reboot();
    for (i = 0; i < raid_group_count; i++) {
        lurker_reinsert_all_disks(&rg_config_p[i]);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "~~~~~ Step 5: Wait rebuild done\n");
    /* Wait rebuild done */
    for (i = 0; i < raid_group_count; i++) {
        lurker_wait_for_rebuild_complete(&rg_config_p[i]);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "~~~~~ Step 6: Verify Result\n");
    lurker_verify();
    mut_printf(MUT_LOG_TEST_STATUS, "%s done\n", __FUNCTION__);

    return;
}
/******************************************
 * End lurker_md_incomplete_write_test()
 ******************************************/

/*!**************************************************************
 * lurker_singlesp_test()
 ****************************************************************
 * @brief
 *  Run single SP paged metadata incomplete write test.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  12/09/2013 - Created. Jamin Kang
 *
 ****************************************************************/
void lurker_singlesp_test(void)
{
    fbe_test_rg_configuration_t *rg_config;
    /* Disable the recovery queue so that a spare cannot swap-in */
    fbe_test_sep_util_disable_recovery_queue(FBE_OBJECT_ID_INVALID);
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_DEBUG_LOW);

    rg_config = &lurker_raid_group_config[0];
    fbe_test_run_test_on_rg_config_with_time_save(rg_config,
                                                  NULL,
                                                  lurker_md_incomplete_write_test,
                                                  LURKER_LUNS_PER_RAID_GROUP,
                                                  LURKER_CHUNKS_PER_LUN,
                                                  FBE_FALSE);
    /* Enable the recovery queue */
    fbe_test_sep_util_enable_recovery_queue(FBE_OBJECT_ID_INVALID);
    mut_printf(MUT_LOG_TEST_STATUS, "%s PASS. ===\n", __FUNCTION__);
    return;
}
/******************************************
 * end lurker_singlesp_test()
 ******************************************/

/*!**************************************************************
 * lurker_singlesp_setup()
 ****************************************************************
 * @brief
 *  Setup for a single SP paged metadata incomplete write test.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  12/09/2013 - Created. Jamin Kang
 *
 ****************************************************************/
void lurker_singlesp_setup(void)
{
    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation()) {
        fbe_test_rg_configuration_t *rg_config = NULL;
        fbe_u32_t num_raid_groups;

        mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for lurker test ===\n");

        rg_config = &lurker_raid_group_config[0];
        fbe_test_sep_util_init_rg_configuration_array(rg_config);

        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config);

        num_raid_groups = fbe_test_get_rg_array_length(rg_config);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        elmo_create_physical_config_for_rg(rg_config, num_raid_groups);
        elmo_load_sep_and_neit();
    }

    fbe_test_common_util_test_setup_init();
    fbe_test_sep_util_disable_system_drive_zeroing();
    fbe_test_sep_util_set_dualsp_test_mode(FALSE);
    fbe_test_sep_util_set_chunks_per_rebuild(1); 

    return;
}
/******************************************
 * End lurker_singlesp_setup()
 ******************************************/

/*!**************************************************************
 * lurker_singlesp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the lurker single SP test.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  12/09/2013 - Created. Jamin Kang
 *
 ****************************************************************/
void lurker_singlesp_cleanup(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for lurker_singlesp_test ===\n");
    if (fbe_test_util_is_simulation()) {
        fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * End lurker_singlesp_cleanup()
 ******************************************/

/*******************************
 * End file lurker_test.c
 *******************************/
