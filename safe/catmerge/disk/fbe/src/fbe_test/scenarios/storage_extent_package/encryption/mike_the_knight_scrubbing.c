/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file mike_the_knight_scrubbing_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains an encryption I/O test.
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "sep_tests.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_dps_memory_interface.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "fbe_database.h"
#include "sep_utils.h"
#include "pp_utils.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_block_transport_interface.h"
#include "fbe_block_transport.h"
#include "fbe/fbe_random.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_encryption_interface.h"
#include "sep_hook.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "sep_zeroing_utils.h"
#include "fbe/fbe_api_system_time_threshold_interface.h"
#include "fbe_test.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * mike_the_knight_short_desc = "This scenario will test different scrubbing cases.";
char * mike_the_knight_long_desc ="\
The scenario tests different scrubbing cases.\n\
\n"\
"-raid_test_level 0 and 1\n\
     We test additional combinations of raid group widths and element sizes at -rtl 1.\n\
\n\
\n\
Description last updated: 02/07/2014.\n";

/*!*******************************************************************
 * @def MIKE_THE_KNIGHT_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Max number of LUNs for each raid group.
 *
 *********************************************************************/
#define MIKE_THE_KNIGHT_TEST_LUNS_PER_RAID_GROUP 0

/*!*******************************************************************
 * @def MIKE_THE_KNIGHT_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define MIKE_THE_KNIGHT_CHUNKS_PER_LUN 15

/*!*******************************************************************
 * @def MIKE_THE_KNIGHT_CHUNK_SIZE
 *********************************************************************
 * @brief chunk size.
 *
 *********************************************************************/
#define MIKE_THE_KNIGHT_CHUNK_SIZE                  0x800

/*!*******************************************************************
 * @def MIKE_THE_KNIGHT_TEST_WAIT_TIME
 *********************************************************************
 * @brief max wait time.
 *
 *********************************************************************/
#define MIKE_THE_KNIGHT_TEST_WAIT_TIME 200000

#define MIKE_THE_KNIGHT_EXTRA_DISK_BLOCKS  (0x800 * 10)

/*!*******************************************************************
 * @var mike_the_knight_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t mike_the_knight_raid_group_config_qual[] = 
{
    /* width,   capacity  raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {FBE_TEST_RG_CONFIG_RANDOM_TAG,       0xE000,     FBE_TEST_RG_CONFIG_RANDOM_TAG,  FBE_TEST_RG_CONFIG_RANDOM_TAG,   520,  7,  0,
        { {0,0,0}, {0,0,1}, {0,0,2}, {0,0,3} } }, 
    {2,       0xE000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            0,          0},
    {4,       0xE000,     FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER, 520,          1,          0},
    {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            2,          0},
    {6,       0xE000,     FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            3,          0},
    {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,   520,            4,          0},
    {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            5,          0},
    //{FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY,   520,  2,  FBE_TEST_RG_CONFIG_RANDOM_TAG},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

static fbe_block_count_t mike_the_knight_extra_capacity = MIKE_THE_KNIGHT_EXTRA_DISK_BLOCKS; 

/***************************************** 
 * FORWARD FUNCTION DECLARATIONS
 *****************************************/
void mike_the_knight_test_rg_config_scrubbing_consumed(fbe_test_rg_configuration_t *rg_config_p, void * context_p);
void mike_the_knight_test_rg_config_normal_scrubbing(fbe_test_rg_configuration_t *rg_config_p, void * context_p);
void mike_the_knight_test_rg_config_rg_destroy_before_encryption_complete(fbe_test_rg_configuration_t *rg_config_p, void * context_p);
void mike_the_knight_test_rg_config_garbage_collection(fbe_test_rg_configuration_t *rg_config_p, void * context_p);
void mike_the_knight_test_rg_config_normal_scrubbing_drive_pull(fbe_test_rg_configuration_t *rg_config_p, void * context_p);
void mike_the_knight_test_rg_config_reboot_sp_dualsp(fbe_test_rg_configuration_t *rg_config_p, void * context_p);

void mike_the_knight_reinsert_drives(fbe_test_rg_configuration_t *rg_config_p,
                            fbe_u32_t position);

void mike_the_knight_remove_drives(fbe_test_rg_configuration_t *rg_config_p, 
                            fbe_u32_t position);

void mike_the_knight_disable_zeroing(fbe_test_rg_configuration_t *rg_config_p);
void mike_the_knight_enable_zeroing(fbe_test_rg_configuration_t *rg_config_p);

void mike_the_knight_setup_rg_config(fbe_test_rg_configuration_t *rg_config_p)
{
    /* We no longer create the raid groups during the setup
     */
    fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
    fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
    fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

    if (rg_config_p[0].width > 4) {
        /* Limit our system RG to 4 drives so it does not exceed the system drives. 
         */
        rg_config_p[0].width = 4;
    }
    /* First config is on system drives, it uses a fixed disk config that we specified 
     * for the system drives. 
     */
    rg_config_p->b_use_fixed_disks = FBE_TRUE;

}
/*!**************************************************************
 * mike_the_knight_disable_zeroing()
 ****************************************************************
 * @brief
 *  disable zeroing on both SPs.
 *
 * @param  rg_config_p.           
 *
 * @return None.
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ****************************************************************/
void mike_the_knight_disable_zeroing(fbe_test_rg_configuration_t *current_rg_config_p)
{
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    fbe_test_sep_util_rg_config_disable_zeroing(current_rg_config_p);

    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_api_sim_transport_set_target_server(other_sp);
        fbe_test_sep_util_rg_config_disable_zeroing(current_rg_config_p);
        fbe_api_sim_transport_set_target_server(this_sp);
    }

    return;
}
/******************************************
 * end mike_the_knight_disable_zeroing()
 ******************************************/
/*!**************************************************************
 * mike_the_knight_enable_zeroing()
 ****************************************************************
 * @brief
 *  enable zeroing on both SPs.
 *
 * @param  rg_config_p.           
 *
 * @return None.
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ****************************************************************/
void mike_the_knight_enable_zeroing(fbe_test_rg_configuration_t *current_rg_config_p)
{
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    fbe_test_sep_util_rg_config_enable_zeroing(current_rg_config_p);

    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_api_sim_transport_set_target_server(other_sp);
        fbe_test_sep_util_rg_config_enable_zeroing(current_rg_config_p);
        fbe_api_sim_transport_set_target_server(this_sp);
    }

    return;
}
/******************************************
 * end mike_the_knight_enable_zeroing()
 ******************************************/
/*!**************************************************************
 * mike_the_knight_reinsert_drives()
 ****************************************************************
 * @brief
 *  remove drives from specific rg position.
 *
 * @param  rg_config_p.           
 * @param  postion.           
 *
 * @return None.
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ****************************************************************/
void mike_the_knight_reinsert_drives(fbe_test_rg_configuration_t *current_rg_config_p,
                            fbe_u32_t position_to_insert)
{
    fbe_test_raid_group_disk_set_t *drive_to_insert_p = NULL;
    fbe_status_t status;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    drive_to_insert_p = &current_rg_config_p->rg_disk_set[position_to_insert];

    mut_printf(MUT_LOG_TEST_STATUS, "== %s position %d_%d_%d. ==", 
               __FUNCTION__, 
               drive_to_insert_p->bus, drive_to_insert_p->enclosure, drive_to_insert_p->slot);


    /* inserting the same drive back */
    if(fbe_test_util_is_simulation())
    {
        status = fbe_test_pp_util_reinsert_drive(drive_to_insert_p->bus, 
                                             drive_to_insert_p->enclosure, 
                                             drive_to_insert_p->slot,
                                             current_rg_config_p->drive_handle[position_to_insert]);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            fbe_api_sim_transport_set_target_server(other_sp);
            status = fbe_test_pp_util_reinsert_drive(drive_to_insert_p->bus, 
                                                     drive_to_insert_p->enclosure, 
                                                     drive_to_insert_p->slot,
                                                     current_rg_config_p->peer_drive_handle[position_to_insert]);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            fbe_api_sim_transport_set_target_server(this_sp);
        }
    }
    else
    {
        status = fbe_test_pp_util_reinsert_drive_hw(drive_to_insert_p->bus, 
                                             drive_to_insert_p->enclosure, 
                                             drive_to_insert_p->slot);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            /* Set the target server to SPB and insert the drive there. */
            fbe_api_sim_transport_set_target_server(other_sp);
            status = fbe_test_pp_util_reinsert_drive_hw(drive_to_insert_p->bus, 
                                                        drive_to_insert_p->enclosure, 
                                                        drive_to_insert_p->slot);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Set the target server back to SPA. */
            fbe_api_sim_transport_set_target_server(this_sp);
        }
    }

    return;
}
/******************************************
 * end mike_the_knight_reinsert_drives()
 ******************************************/


/*!**************************************************************
 * mike_the_knight_remove_drives()
 ****************************************************************
 * @brief
 *  remove drives from specific rg position.
 *
 * @param  rg_config_p.           
 * @param  postion.           
 *
 * @return None.
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ****************************************************************/
void mike_the_knight_remove_drives(fbe_test_rg_configuration_t *current_rg_config_p, 
                                   fbe_u32_t position_to_remove)
{
    fbe_test_raid_group_disk_set_t *drive_to_remove_p = NULL;
    fbe_status_t status;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    drive_to_remove_p = &current_rg_config_p->rg_disk_set[position_to_remove];

    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s: Removing drive %d_%d_%d", 
               __FUNCTION__,  
               drive_to_remove_p->bus, drive_to_remove_p->enclosure, drive_to_remove_p->slot);


   /* We are going to pull a drive.
     */
    if(fbe_test_util_is_simulation())
    {
        status = fbe_test_pp_util_pull_drive(drive_to_remove_p->bus, 
                                             drive_to_remove_p->enclosure, 
                                             drive_to_remove_p->slot,
                                             &current_rg_config_p->drive_handle[position_to_remove]);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            fbe_api_sim_transport_set_target_server(other_sp);
            status = fbe_test_pp_util_pull_drive(drive_to_remove_p->bus, 
                                                 drive_to_remove_p->enclosure, 
                                                 drive_to_remove_p->slot,
                                                 &current_rg_config_p->peer_drive_handle[position_to_remove]);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            fbe_api_sim_transport_set_target_server(this_sp);
        }
    }
    else
    {
        status = fbe_test_pp_util_pull_drive_hw(drive_to_remove_p->bus, 
                                                drive_to_remove_p->enclosure, 
                                                drive_to_remove_p->slot);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            /* Set the target server to SPB and remove the drive there. */
            fbe_api_sim_transport_set_target_server(other_sp);
            status = fbe_test_pp_util_pull_drive_hw(drive_to_remove_p->bus, 
                                                    drive_to_remove_p->enclosure, 
                                                    drive_to_remove_p->slot);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Set the target server back to SPA. */
            fbe_api_sim_transport_set_target_server(this_sp);
        }
    }

    return;
}

/******************************************
 * end mike_the_knight_remove_drives()
 ******************************************/

/*!**************************************************************
 * mike_the_knight_test_rg_config_rg_destroy_before_encryption_complete()
 ****************************************************************
 * @brief
 *  destroy rg before encryption is turned on.
 *
 * @param  rg_config_p.           
 *
 * @return None.
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ****************************************************************/
void mike_the_knight_test_rg_config_rg_destroy_before_encryption_complete(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_status_t                status;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t                   rg_count = 0;
    fbe_u32_t                   rg_index = 0;
    fbe_database_system_scrub_progress_t  prev_scrub_progress;
    fbe_object_id_t             pvd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_base_config_encryption_mode_t encryption_mode;

    /* All pvds must be zeroed before enabling encryption
     */
    fbe_test_rg_wait_for_zeroing(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== Waiting for zeroing complete. (successful)==");

    rg_count = fbe_test_get_rg_array_length(rg_config_p);

    fbe_test_get_pvd_object_ids(rg_config_p, &pvd_object_ids[0], 0 /* position */);

    fbe_api_database_set_poll_interval(0);

    /* enable encryption */
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0, FBE_TEST_HOOK_ACTION_ADD);
    mut_printf(MUT_LOG_TEST_STATUS, "Starting Encryption.");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* make sure scrubbing is in progress */
    status = fbe_api_database_get_system_scrub_progress(&prev_scrub_progress);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_UINT64_NOT_EQUAL(prev_scrub_progress.blocks_already_scrubbed, prev_scrub_progress.total_capacity_in_blocks);

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for Encryption to hit chunk 0.");
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0, FBE_TEST_HOOK_ACTION_WAIT);

    /* destroy raid group */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++) {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            status = fbe_test_sep_util_destroy_raid_group(current_rg_config_p->raid_group_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        /* Goto next raid group
         */
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++) {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            status = fbe_api_base_config_get_encryption_mode(pvd_object_ids[rg_index], &encryption_mode);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            if (current_rg_config_p->b_use_fixed_disks == FBE_TRUE)
            {
                /* system drive encryption mode is based on vault */
                MUT_ASSERT_INT_NOT_EQUAL(encryption_mode, FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED);
            }
            else
            {
                MUT_ASSERT_INT_EQUAL(encryption_mode, FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED);
            }
        }
        /* Goto next raid group
         */
        current_rg_config_p++;
    }

    /* enable background zeroing */
    mut_printf(MUT_LOG_TEST_STATUS, "enable zeroing.");
    mike_the_knight_enable_zeroing(rg_config_p);

    /* make sure scrubbing is in progress */
    status = fbe_test_sep_util_wait_for_scrub_complete(MIKE_THE_KNIGHT_TEST_WAIT_TIME);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return ;

}
/******************************************
 * end mike_the_knight_test_rg_config_rg_destroy_before_encryption_complete()
 ******************************************/
/*!**************************************************************
 * mike_the_knight_run_rdgen_disk()
 ****************************************************************
 * @brief
 *  Run rdgen on a particular disk with a given pattern.
 *
 * @param rg_config_p
 * @param b_use_pvd
 * @param operation
 * @param pattern
 *
 * @return None.
 *
 * @author
 *  7/7/2014 - Created. Rob Foley
 *
 ****************************************************************/
void mike_the_knight_run_rdgen_disk(fbe_test_rg_configuration_t *rg_config_p,
                                    fbe_bool_t b_use_pvd,
                                    fbe_rdgen_operation_t operation,
                                    fbe_rdgen_pattern_t pattern)
{
    fbe_status_t status;
    fbe_object_id_t                object_id;
    fbe_test_rg_configuration_t    *current_rg_config_p = NULL;
    fbe_u32_t                      rg_count = 0;
    fbe_u32_t                      rg_index = 0;
    fbe_u32_t                      drive_position;
	fbe_api_provision_drive_info_t pvd_info;
    fbe_package_id_t               package_id;

    rg_count = fbe_test_get_rg_array_length(rg_config_p);

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++) {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            for (drive_position = 0; drive_position < current_rg_config_p->width; drive_position++) {
                /* Only do it for drives that we gave the larger capacity to.
                 */
                if (current_rg_config_p->rg_disk_set[drive_position].slot % 2) {
                    fbe_object_id_t pvd_object_id;
                    status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->rg_disk_set[drive_position].bus,
                                                                            current_rg_config_p->rg_disk_set[drive_position].enclosure,
                                                                            current_rg_config_p->rg_disk_set[drive_position].slot,
                                                                            &pvd_object_id);
                    status = fbe_api_get_physical_drive_object_id_by_location(current_rg_config_p->rg_disk_set[drive_position].bus,
                                                                            current_rg_config_p->rg_disk_set[drive_position].enclosure,
                                                                            current_rg_config_p->rg_disk_set[drive_position].slot,
                                                                            &object_id);
                    if (b_use_pvd) {
                        object_id = pvd_object_id;
                        package_id = FBE_PACKAGE_ID_SEP_0;
                    } else {
                        package_id = FBE_PACKAGE_ID_PHYSICAL;
                    }
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    status = fbe_api_provision_drive_get_info(pvd_object_id, &pvd_info);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

                    mut_printf(MUT_LOG_TEST_STATUS, "%s (0x%x) %d_%d_%d rdgen op: %s to lba: 0x%llx blocks: %d\n",
                               ((b_use_pvd) ? "pvd:" : "pdo:"), object_id,
                               current_rg_config_p->rg_disk_set[drive_position].bus,
                               current_rg_config_p->rg_disk_set[drive_position].enclosure,
                               current_rg_config_p->rg_disk_set[drive_position].slot,
                               ((operation == FBE_RDGEN_OPERATION_WRITE_ONLY) ? "write" : "read check"),
                               pvd_info.paged_metadata_lba - MIKE_THE_KNIGHT_EXTRA_DISK_BLOCKS, 
                               MIKE_THE_KNIGHT_EXTRA_DISK_BLOCKS);

                    /* Write a pattern to the PVD just before the paged.
                     */
                    fbe_test_sep_util_run_rdgen_for_pvd_ex(object_id, package_id, operation, pattern, 
                                                        pvd_info.paged_metadata_lba - MIKE_THE_KNIGHT_EXTRA_DISK_BLOCKS, 
                                                        MIKE_THE_KNIGHT_EXTRA_DISK_BLOCKS);
                }
            }
        }
        current_rg_config_p++;
    }
}
/******************************************
 * end mike_the_knight_run_rdgen_disk()
 ******************************************/
/*!**************************************************************
 * mike_the_knight_zero_pvd_range()
 ****************************************************************
 * @brief
 *  Zero out a range on the PVD that is unconsumed.
 *
 * @param rg_config_p - current config.               
 *
 * @return none   
 *
 * @author
 *  7/7/2014 - Created. Rob Foley
 *
 ****************************************************************/

void mike_the_knight_zero_pvd_range(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;
    fbe_object_id_t                object_id;
    fbe_test_rg_configuration_t    *current_rg_config_p = NULL;
    fbe_u32_t                      rg_count = 0;
    fbe_u32_t                      rg_index = 0;
    fbe_u32_t                      drive_position;
	fbe_api_provision_drive_info_t pvd_info;
    fbe_api_rdgen_context_t        context;

    rg_count = fbe_test_get_rg_array_length(rg_config_p);

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++) {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            for (drive_position = 0; drive_position < current_rg_config_p->width; drive_position++) {
                /* Only do it for drives that we gave the larger capacity to.
                 */
                if (current_rg_config_p->rg_disk_set[drive_position].slot % 2) {
                    status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->rg_disk_set[drive_position].bus,
                                                                            current_rg_config_p->rg_disk_set[drive_position].enclosure,
                                                                            current_rg_config_p->rg_disk_set[drive_position].slot,
                                                                            &object_id);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    status = fbe_api_provision_drive_get_info(object_id, &pvd_info);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

                    mut_printf(MUT_LOG_TEST_STATUS, "pvd object: 0x%x %d_%d_%d ZERO to lba: 0x%llx blocks: %d\n",
                               object_id, 
                               current_rg_config_p->rg_disk_set[drive_position].bus,
                               current_rg_config_p->rg_disk_set[drive_position].enclosure,
                               current_rg_config_p->rg_disk_set[drive_position].slot,
                               pvd_info.paged_metadata_lba - MIKE_THE_KNIGHT_EXTRA_DISK_BLOCKS, 
                               MIKE_THE_KNIGHT_EXTRA_DISK_BLOCKS);

                    status = fbe_api_rdgen_send_one_io(&context, object_id, FBE_CLASS_ID_INVALID, FBE_PACKAGE_ID_SEP_0,
                                                       FBE_RDGEN_OPERATION_ZERO_ONLY, FBE_RDGEN_PATTERN_ZEROS,
                                                       pvd_info.paged_metadata_lba - MIKE_THE_KNIGHT_EXTRA_DISK_BLOCKS, 
                                                       MIKE_THE_KNIGHT_EXTRA_DISK_BLOCKS,
                                                       FBE_RDGEN_OPTIONS_INVALID, 0, 0, FBE_API_RDGEN_PEER_OPTIONS_INVALID);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                }
            }
        }
        current_rg_config_p++;
    }
}
/******************************************
 * end mike_the_knight_zero_pvd_range()
 ******************************************/
/*!**************************************************************
 * mike_the_knight_test_rg_config_normal_scrubbing()
 ****************************************************************
 * @brief
 *  mark rg as scrub complete after encryption is turned on.
 *
 * @param  rg_config_p.           
 *
 * @return None.
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ****************************************************************/
void mike_the_knight_test_rg_config_normal_scrubbing(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_status_t                status;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t                   rg_count = 0;
    fbe_u32_t                   rg_index = 0;
	fbe_api_provision_drive_info_t      pvd_info;
    fbe_object_id_t             pvd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_database_system_scrub_progress_t  scrub_progress;
    fbe_database_system_scrub_progress_t  prev_scrub_progress;
    fbe_object_id_t             pvd1_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
	fbe_u32_t old_hook_timeout;

    /* All pvds must be zeroed before enabling encryption
     */
    fbe_test_rg_wait_for_zeroing(rg_config_p);

    rg_count = fbe_test_get_rg_array_length(rg_config_p);

    fbe_test_get_pvd_object_ids(rg_config_p, &pvd_object_ids[0], 0 /* position */);
    fbe_test_get_pvd_object_ids(rg_config_p, &pvd1_object_ids[0], 1 /* position */);

    fbe_api_database_set_poll_interval(0);

    /* For drives that are larger write a pattern to them that we will eventually check to make sure they actually
     * scrubbed. Zero the range so we will be marked consumed eventually. 
     */
    mike_the_knight_zero_pvd_range(rg_config_p);
    /* We must use PVD since we marked for zeroing.  To make sure write occurs, go through PVD.
     */
    mike_the_knight_run_rdgen_disk(rg_config_p, FBE_TRUE, /* use pvd */ FBE_RDGEN_OPERATION_WRITE_ONLY, FBE_RDGEN_PATTERN_LBA_PASS);
    /* Use PVD since the seeded pattern was written through PVD.
     */
    mike_the_knight_run_rdgen_disk(rg_config_p, FBE_TRUE, /* use pvd */ FBE_RDGEN_OPERATION_READ_CHECK, FBE_RDGEN_PATTERN_LBA_PASS);

    /* pull one drive each rg, we need to test scrub_intent is properly persisted even the drive is not there.*/
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++) {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            mike_the_knight_remove_drives(current_rg_config_p, 1);
        }
        current_rg_config_p++;
    }

    /* enable encryption */
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* make sure scrubbing is in progress */
    status = fbe_api_database_get_system_scrub_progress(&prev_scrub_progress);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_UINT64_NOT_EQUAL(prev_scrub_progress.blocks_already_scrubbed, prev_scrub_progress.total_capacity_in_blocks);

    /* put drives back */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++) {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            mike_the_knight_reinsert_drives(current_rg_config_p, 1);
        }
        current_rg_config_p++;
    }

	/* Save old timeout */
	old_hook_timeout = fbe_test_get_timeout_ms();

	fbe_test_set_timeout_ms(180000); /* 3 min */

    /* This does all the logic of dealing with completing the raid group including 
     * the keys and the encryption mode. 
     */
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++) {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p)) {

            fbe_api_provision_drive_get_info(pvd_object_ids[rg_index], &pvd_info);
            MUT_ASSERT_INT_EQUAL(pvd_info.scrubbing_in_progress, FBE_FALSE);

            /* make sure in progress capacity do not go back */
            status = fbe_api_database_get_system_scrub_progress(&scrub_progress);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_TRUE_MSG((scrub_progress.blocks_already_scrubbed >= prev_scrub_progress.blocks_already_scrubbed),
                                "scrub in progress should always move forward!");
            prev_scrub_progress = scrub_progress;

        }
        /* Goto next raid group
         */
        current_rg_config_p++;
    }

    /* make sure scrubbing completes */
    status = fbe_test_sep_util_wait_for_scrub_complete(MIKE_THE_KNIGHT_TEST_WAIT_TIME);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++) {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            /* these drives should not have scrub in progress */
            fbe_api_provision_drive_get_info(pvd1_object_ids[rg_index], &pvd_info);
            MUT_ASSERT_INT_EQUAL(pvd_info.scrubbing_in_progress, FBE_FALSE);

        }
        /* Goto next raid group
         */
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Check that drives with extra capacity had extra capacity scrubbed");
    mike_the_knight_run_rdgen_disk(rg_config_p, FBE_FALSE, /* No need to use pvd */ FBE_RDGEN_OPERATION_READ_CHECK, FBE_RDGEN_PATTERN_ZEROS);

	fbe_test_set_timeout_ms(old_hook_timeout); /* Restore old timeout */


    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return ;

}
/******************************************
 * end mike_the_knight_test_rg_config_normal_scrubbing()
 ******************************************/

/*!**************************************************************
 * mike_the_knight_test_rg_config_scrubbing_consumed()
 ****************************************************************
 * @brief
 *  scrub consumed area.
 *
 * @param  rg_config_p.           
 *
 * @return None.
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ****************************************************************/
void mike_the_knight_test_rg_config_scrubbing_consumed(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_status_t                status;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t                   rg_count = 0;
    fbe_u32_t                   rg_index = 0;
    fbe_api_raid_group_get_info_t       rg_info;
    fbe_object_id_t             rg_object_id;
    fbe_object_id_t             pvd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t             pvd1_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_u64_t                           pba[FBE_TEST_MAX_RAID_GROUP_COUNT] = {NULL};
	fbe_api_provision_drive_info_t      pvd_info;
    fbe_provision_drive_paged_metadata_t         *pvd_metadata;
    fbe_api_base_config_metadata_paged_get_bits_t paged_get_bits;
    fbe_database_system_scrub_progress_t  scrub_progress;
    fbe_database_system_scrub_progress_t  prev_scrub_progress;
    fbe_api_provisional_drive_get_spare_drive_pool_t  spare_drive_pool;

    /* collect the unused PVDs */
    status = fbe_api_provision_drive_get_spare_drive_pool(&spare_drive_pool);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    rg_count = fbe_test_get_rg_array_length(rg_config_p);

    fbe_test_get_pvd_object_ids(rg_config_p, &pvd_object_ids[0], 0 /* position */);
    fbe_test_get_pvd_object_ids(rg_config_p, &pvd1_object_ids[0], 1 /* position */);

    fbe_api_database_set_poll_interval(0);

    /* destroy all raid groups */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++) {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            status = fbe_api_raid_group_get_info(rg_object_id, &rg_info,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            mut_printf(MUT_LOG_LOW, "RG %d paged metadata LBA: 0x%llX\n",
	                    current_rg_config_p->raid_group_id, (unsigned long long)rg_info.paged_metadata_start_lba);

            status = fbe_api_raid_group_get_physical_from_logical_lba(rg_object_id, rg_info.paged_metadata_start_lba, &pba[rg_index]);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            mut_printf(MUT_LOG_LOW, "RG paged metadata PBA: 0x%llX\n",
	                    (unsigned long long)pba[rg_index]);

            /* wait for disk zeroing to complete */
            status = sep_zeroing_utils_check_disk_zeroing_status(pvd_object_ids[rg_index], MIKE_THE_KNIGHT_TEST_WAIT_TIME, FBE_LBA_INVALID);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            status = fbe_test_sep_util_destroy_raid_group(current_rg_config_p->raid_group_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Read metadata */        
            mut_printf(MUT_LOG_LOW, "=== verify PVD obj id 0x%x paged metadata, chunck 0x%x, 0x%llx\n", 
                       pvd_object_ids[rg_index], (0x10000/MIKE_THE_KNIGHT_CHUNK_SIZE), (pba[rg_index]/MIKE_THE_KNIGHT_CHUNK_SIZE));
            paged_get_bits.metadata_offset = (fbe_u32_t )(pba[rg_index]/MIKE_THE_KNIGHT_CHUNK_SIZE) * sizeof(fbe_provision_drive_paged_metadata_t);
            paged_get_bits.metadata_record_data_size = sizeof(fbe_provision_drive_paged_metadata_t);
            * (fbe_u32_t *)paged_get_bits.metadata_record_data = 0;
            paged_get_bits.get_bits_flags = 0;
            status = fbe_api_base_config_metadata_paged_get_bits(pvd_object_ids[rg_index], &paged_get_bits);
            pvd_metadata = (fbe_provision_drive_paged_metadata_t *)&paged_get_bits.metadata_record_data[0];
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_EQUAL(pvd_metadata[0].need_zero_bit, 0);
            MUT_ASSERT_INT_EQUAL(pvd_metadata[0].consumed_user_data_bit, 1);

        }
        /* Goto next raid group
         */
        current_rg_config_p++;
    }

    /* disable background zeroing */
    mike_the_knight_disable_zeroing(rg_config_p);

    /* pull one drive each rg, we need to test scrub_intent is properly persisted even the drive is not there.*/
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++) {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            mike_the_knight_remove_drives(current_rg_config_p, 1);
        }
        current_rg_config_p++;
    }

    /* enable encryption */
    status = fbe_test_sep_util_enable_kms_encryption();
    mut_printf(MUT_LOG_TEST_STATUS, "Encryption enable status %d", status);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "enabling of encryption failed\n");

    /* make sure scrubbing is in progress */
    status = fbe_api_database_get_system_scrub_progress(&prev_scrub_progress);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_UINT64_NOT_EQUAL(prev_scrub_progress.blocks_already_scrubbed, prev_scrub_progress.total_capacity_in_blocks);

    /* unused PVD does not need scrubbing*/
    if (spare_drive_pool.number_of_spare > 4)
    {
        status = fbe_test_sep_util_wait_for_pvd_np_flags(spare_drive_pool.spare_object_list[4], FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_COMPLETE, MIKE_THE_KNIGHT_TEST_WAIT_TIME);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        fbe_api_provision_drive_get_info(spare_drive_pool.spare_object_list[4], &pvd_info);
        MUT_ASSERT_INT_EQUAL(pvd_info.scrubbing_in_progress, FBE_FALSE);
    }
    else
    {
        mut_printf(MUT_LOG_TEST_STATUS, "no unconsumed PVD to check\n");
    }
    /* enable background zeroing */
    mike_the_knight_enable_zeroing(rg_config_p);

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++) {
        if ((fbe_test_rg_config_is_enabled(current_rg_config_p)) &&
            (current_rg_config_p->b_use_fixed_disks != FBE_TRUE)) {  /* we can't guarantee system completes zeroing while another system drive is pulled */
            /* wait for disk zeroing to complete */
            status = sep_zeroing_utils_check_disk_zeroing_status(pvd_object_ids[rg_index], MIKE_THE_KNIGHT_TEST_WAIT_TIME, FBE_LBA_INVALID);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            status = fbe_test_sep_util_wait_for_pvd_np_flags(pvd_object_ids[rg_index], FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_COMPLETE, MIKE_THE_KNIGHT_TEST_WAIT_TIME);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            fbe_api_provision_drive_get_info(pvd_object_ids[rg_index], &pvd_info);
            MUT_ASSERT_INT_EQUAL(pvd_info.scrubbing_in_progress, FBE_FALSE);

            /* make sure in progress capacity do not go back */
            status = fbe_api_database_get_system_scrub_progress(&scrub_progress);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_TRUE_MSG((scrub_progress.blocks_already_scrubbed >= prev_scrub_progress.blocks_already_scrubbed),
                                "scrub in progress should always move forward!");
            /* we should not complete the scrub because there's still missing drives */
            MUT_ASSERT_UINT64_NOT_EQUAL(scrub_progress.blocks_already_scrubbed, scrub_progress.total_capacity_in_blocks);
            prev_scrub_progress = scrub_progress;

        }
        /* Goto next raid group
         */
        current_rg_config_p++;
    }

    /* put drives back */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++) {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            mike_the_knight_reinsert_drives(current_rg_config_p, 1);
        }
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++) {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            /* wait for disk zeroing to complete */
            status = sep_zeroing_utils_check_disk_zeroing_status(pvd1_object_ids[rg_index], MIKE_THE_KNIGHT_TEST_WAIT_TIME, FBE_LBA_INVALID);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            status = fbe_test_sep_util_wait_for_pvd_np_flags(pvd1_object_ids[rg_index], FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_COMPLETE, MIKE_THE_KNIGHT_TEST_WAIT_TIME);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            fbe_api_provision_drive_get_info(pvd1_object_ids[rg_index], &pvd_info);
            MUT_ASSERT_INT_EQUAL(pvd_info.scrubbing_in_progress, FBE_FALSE);

            /* make sure in progress capacity do not go back */
            status = fbe_api_database_get_system_scrub_progress(&scrub_progress);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_TRUE_MSG((scrub_progress.blocks_already_scrubbed >= prev_scrub_progress.blocks_already_scrubbed),
                                "scrub in progress should always move forward!");
            prev_scrub_progress = scrub_progress;
        }
        /* Goto next raid group
         */
        current_rg_config_p++;
    }

    /* make sure scrubbing is in progress */
    status = fbe_test_sep_util_wait_for_scrub_complete(MIKE_THE_KNIGHT_TEST_WAIT_TIME);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return ;

}
/******************************************
 * end mike_the_knight_test_rg_config_scrubbing_consumed()
 ******************************************/

/*!**************************************************************
 * mike_the_knight_test_rg_config_normal_scrubbing_drive_pull()
 ****************************************************************
 * @brief
 *  pull one drive off the first rg, scrubbing does not complete.
 *  mark rg as scrub complete after encryption is turned on.
 *
 * @param  rg_config_p.           
 *
 * @return None.
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ****************************************************************/
void mike_the_knight_test_rg_config_normal_scrubbing_drive_pull(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_status_t                status;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_test_rg_configuration_t *pull_drive_rg_config_p = NULL;
    fbe_u32_t                   rg_count = 0;
    fbe_u32_t                   rg_index = 0;
	fbe_api_provision_drive_info_t      pvd_info;
    fbe_object_id_t             pvd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_database_system_scrub_progress_t  scrub_progress;
    fbe_database_system_scrub_progress_t  prev_scrub_progress;
    fbe_object_id_t             pvd1_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};

    /* All pvds must be zeroed before enabling encryption
     */
    fbe_test_rg_wait_for_zeroing(rg_config_p);

    rg_count = fbe_test_get_rg_array_length(rg_config_p);

    fbe_test_get_pvd_object_ids(rg_config_p, &pvd_object_ids[0], 0 /* position */);
    fbe_test_get_pvd_object_ids(rg_config_p, &pvd1_object_ids[0], 1 /* position */);

    fbe_api_database_set_poll_interval(0);

    /* pull one drive.*/
    pull_drive_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++) {
        if ((fbe_test_rg_config_is_enabled(pull_drive_rg_config_p)) &&
            (pull_drive_rg_config_p->b_use_fixed_disks != FBE_TRUE))  // can't pull drive from system rg
        {
            mike_the_knight_remove_drives(pull_drive_rg_config_p, 1);
            pull_drive_rg_config_p->b_create_this_pass = FBE_FALSE;  /* take this rg out of regular loops */
            break;  /* only affect the first rg */
        }
        pull_drive_rg_config_p++;
    }

    /* enable encryption */
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* make sure scrubbing is in progress */
    status = fbe_api_database_get_system_scrub_progress(&prev_scrub_progress);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_UINT64_NOT_EQUAL(prev_scrub_progress.blocks_already_scrubbed, prev_scrub_progress.total_capacity_in_blocks);

    mut_printf(MUT_LOG_TEST_STATUS, "system scrubbing blocks_already_scrubbed: 0x%llx total_capacity_in_blocks: 0x%llx\n",
               prev_scrub_progress.blocks_already_scrubbed, prev_scrub_progress.total_capacity_in_blocks);

    /* This does all the logic of dealing with completing the raid group including 
     * the keys and the encryption mode. 
     */
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++) {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p)) {

            fbe_api_provision_drive_get_info(pvd_object_ids[rg_index], &pvd_info);
            MUT_ASSERT_INT_EQUAL(pvd_info.scrubbing_in_progress, FBE_FALSE);

            /* make sure in progress capacity do not go back */
            status = fbe_api_database_get_system_scrub_progress(&scrub_progress);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_TRUE_MSG((scrub_progress.blocks_already_scrubbed >= prev_scrub_progress.blocks_already_scrubbed),
                                "scrub in progress should always move forward!");
            prev_scrub_progress = scrub_progress;
            mut_printf(MUT_LOG_TEST_STATUS, "system scrubbing progress rg_id: 0x%x blocks_already_scrubbed: 0x%llx total_capacity_in_blocks: 0x%llx\n",
                       current_rg_config_p->raid_group_id,
                       prev_scrub_progress.blocks_already_scrubbed, 
                       prev_scrub_progress.total_capacity_in_blocks);
        }
        /* Goto next raid group
         */
        current_rg_config_p++;
    }

    /* scrubbing should not complete */
    mut_printf(MUT_LOG_TEST_STATUS, "system scrubbing before reinsert: blocks_already_scrubbed: 0x%llx total_capacity_in_blocks: 0x%llx\n",
               prev_scrub_progress.blocks_already_scrubbed, prev_scrub_progress.total_capacity_in_blocks);

    /* The below check is not valid.
     * If drives are powered down, they do not contribute to the total capacity in blocks since 
     * fbe_database_control_get_system_scrub_progress() does not get the capacity of drives that are not ready... 
     */
    //MUT_ASSERT_UINT64_NOT_EQUAL(prev_scrub_progress.blocks_already_scrubbed, prev_scrub_progress.total_capacity_in_blocks);

    mike_the_knight_reinsert_drives(pull_drive_rg_config_p, 1);

    /* make sure scrubbing completes */
    status = fbe_test_sep_util_wait_for_scrub_complete(MIKE_THE_KNIGHT_TEST_WAIT_TIME);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++) {
        if ((fbe_test_rg_config_is_enabled(current_rg_config_p)) ||
            (current_rg_config_p == pull_drive_rg_config_p)) {
            /* Wait for scrubbing to finish. */
            fbe_test_sep_util_wait_for_scrubbing_complete(pvd1_object_ids[rg_index], FBE_TEST_WAIT_TIMEOUT_MS);
        }
        /* Goto next raid group
         */
        current_rg_config_p++;
    }

    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return ;

}
/******************************************
 * end mike_the_knight_test_rg_config_normal_scrubbing_drive_pull()
 ******************************************/

/*!**************************************************************
 * mike_the_knight_test_rg_config_garbage_collection()
 ****************************************************************
 * @brief
 *  scrub consumed area.
 *
 * @param  rg_config_p.           
 *
 * @return None.
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ****************************************************************/
void mike_the_knight_test_rg_config_garbage_collection(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_status_t                status;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_test_rg_configuration_t *pull_drive_rg_config_p = NULL;
    fbe_u32_t                   rg_count = 0;
    fbe_u32_t                   rg_index = 0;
    fbe_object_id_t             pvd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT]  = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t             pvd1_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
	fbe_api_provision_drive_info_t      pvd_info;
    fbe_database_system_scrub_progress_t  scrub_progress;
    fbe_database_system_scrub_progress_t  prev_scrub_progress;
    fbe_system_time_threshold_info_t        out_time_threshold;

    rg_count = fbe_test_get_rg_array_length(rg_config_p);

    fbe_test_get_pvd_object_ids(rg_config_p, &pvd_object_ids[0], 0 /* position */);
    fbe_test_get_pvd_object_ids(rg_config_p, &pvd1_object_ids[0], 1 /* position */);

    fbe_api_database_set_poll_interval(0);

    /* pull one drive.*/
    pull_drive_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++) {
        if ((fbe_test_rg_config_is_enabled(pull_drive_rg_config_p)) &&
            (pull_drive_rg_config_p->b_use_fixed_disks != FBE_TRUE))  // system drive will not be garbage collected
        {
            mike_the_knight_remove_drives(pull_drive_rg_config_p, 1);
            break;  /* only affect the first rg */
        }
        pull_drive_rg_config_p++;
    }

    /* destroy all raid groups */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++) {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            status = fbe_test_sep_util_destroy_raid_group(current_rg_config_p->raid_group_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        /* Goto next raid group
         */
        current_rg_config_p++;
    }

    /* disable background zeroing */
    mike_the_knight_disable_zeroing(rg_config_p);

    /* enable encryption */
    status = fbe_test_sep_util_enable_kms_encryption();
    mut_printf(MUT_LOG_TEST_STATUS, "Encryption enable status %d", status);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "enabling of encryption failed\n");

    /* make sure scrubbing is in progress */
    status = fbe_api_database_get_system_scrub_progress(&prev_scrub_progress);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_UINT64_NOT_EQUAL(prev_scrub_progress.blocks_already_scrubbed, prev_scrub_progress.total_capacity_in_blocks);

    /* enable background zeroing */
    mike_the_knight_enable_zeroing(rg_config_p);

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++) {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            /* wait for disk zeroing to complete */
            status = sep_zeroing_utils_check_disk_zeroing_status(pvd_object_ids[rg_index], MIKE_THE_KNIGHT_TEST_WAIT_TIME, FBE_LBA_INVALID);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            status = fbe_test_sep_util_wait_for_pvd_np_flags(pvd_object_ids[rg_index], FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_COMPLETE, MIKE_THE_KNIGHT_TEST_WAIT_TIME);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            fbe_api_provision_drive_get_info(pvd_object_ids[rg_index], &pvd_info);
            MUT_ASSERT_INT_EQUAL(pvd_info.scrubbing_in_progress, FBE_FALSE);

            /* make sure in progress capacity do not go back */
            status = fbe_api_database_get_system_scrub_progress(&scrub_progress);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_TRUE_MSG((scrub_progress.blocks_already_scrubbed >= prev_scrub_progress.blocks_already_scrubbed),
                                "scrub in progress should always move forward!");

            prev_scrub_progress = scrub_progress;

        }
        /* Goto next raid group
         */
        current_rg_config_p++;
    }

    /* trigger garbage collection */
	out_time_threshold.time_threshold_in_minutes = 0;
    mut_printf(MUT_LOG_TEST_STATUS, "Set time threshold ...\n");
    status = fbe_api_set_system_time_threshold_info(&out_time_threshold);

    /* make sure scrubbing is in progress */
    status = fbe_test_sep_util_wait_for_scrub_complete(MIKE_THE_KNIGHT_TEST_WAIT_TIME);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return ;

}
/******************************************
 * end mike_the_knight_test_rg_config_garbage_collection()
 ******************************************/

/*!**************************************************************
 * mike_the_knight_test_rg_config_reboot_sp_dualsp()
 ****************************************************************
 * @brief
 *  scrub consumed area continues after reboot.
 *
 * @param  rg_config_p.           
 *
 * @return None.
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ****************************************************************/
void mike_the_knight_test_rg_config_reboot_sp_dualsp(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_status_t                status;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t                   rg_count = 0;
    fbe_u32_t                   rg_index = 0;
    fbe_api_raid_group_get_info_t       rg_info;
    fbe_object_id_t             rg_object_id;
    fbe_object_id_t             pvd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_u64_t                           pba[FBE_TEST_MAX_RAID_GROUP_COUNT] = {NULL};
	fbe_api_provision_drive_info_t      pvd_info;
    fbe_provision_drive_paged_metadata_t         *pvd_metadata;
    fbe_api_base_config_metadata_paged_get_bits_t paged_get_bits;
    fbe_database_system_scrub_progress_t  scrub_progress;
    fbe_database_system_scrub_progress_t  prev_scrub_progress;

    rg_count = fbe_test_get_rg_array_length(rg_config_p);

    fbe_test_get_pvd_object_ids(rg_config_p, &pvd_object_ids[0], 0 /* position */);

    fbe_api_database_set_poll_interval(0);

    /* destroy all raid groups */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++) {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            status = fbe_api_raid_group_get_info(rg_object_id, &rg_info,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            mut_printf(MUT_LOG_LOW, "RG %d paged metadata LBA: 0x%llX\n",
	                    current_rg_config_p->raid_group_id, (unsigned long long)rg_info.paged_metadata_start_lba);

            status = fbe_api_raid_group_get_physical_from_logical_lba(rg_object_id, rg_info.paged_metadata_start_lba, &pba[rg_index]);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            mut_printf(MUT_LOG_LOW, "RG paged metadata PBA: 0x%llX\n",
	                    (unsigned long long)pba[rg_index]);

            /* wait for disk zeroing to complete */
            status = sep_zeroing_utils_check_disk_zeroing_status(pvd_object_ids[rg_index], MIKE_THE_KNIGHT_TEST_WAIT_TIME, FBE_LBA_INVALID);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            status = fbe_test_sep_util_destroy_raid_group(current_rg_config_p->raid_group_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Read metadata */        
            mut_printf(MUT_LOG_LOW, "=== verify PVD obj id 0x%x paged metadata, chunck 0x%x, 0x%llx\n", 
                       pvd_object_ids[rg_index], (0x10000/MIKE_THE_KNIGHT_CHUNK_SIZE), (pba[rg_index]/MIKE_THE_KNIGHT_CHUNK_SIZE));
            paged_get_bits.metadata_offset = (fbe_u32_t )(pba[rg_index]/MIKE_THE_KNIGHT_CHUNK_SIZE) * sizeof(fbe_provision_drive_paged_metadata_t);
            paged_get_bits.metadata_record_data_size = sizeof(fbe_provision_drive_paged_metadata_t);
            * (fbe_u32_t *)paged_get_bits.metadata_record_data = 0;
            paged_get_bits.get_bits_flags = 0;
            status = fbe_api_base_config_metadata_paged_get_bits(pvd_object_ids[rg_index], &paged_get_bits);
            pvd_metadata = (fbe_provision_drive_paged_metadata_t *)&paged_get_bits.metadata_record_data[0];
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_EQUAL(pvd_metadata[0].need_zero_bit, 0);
            MUT_ASSERT_INT_EQUAL(pvd_metadata[0].consumed_user_data_bit, 1);
        }
        /* Goto next raid group
         */
        current_rg_config_p++;
    }

    /* disable background zeroing */
    mike_the_knight_disable_zeroing(rg_config_p);

    /* sets debug hook to stop processing scrub_intent bit */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++) {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            status = fbe_test_add_debug_hook_active(pvd_object_ids[rg_index], SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
                                                  FBE_PROVISION_DRIVE_SUBSTATE_SCRUB_INTENT, 
                                                  NULL,
                                                  NULL, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            /* this is to make sure peer lost will not trigger the processing of scrub intent bits */
            status = fbe_test_add_debug_hook_passive(pvd_object_ids[rg_index], SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
                                                  FBE_PROVISION_DRIVE_SUBSTATE_SCRUB_INTENT, 
                                                  NULL,
                                                  NULL, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        current_rg_config_p++;
    }

    /* initiate scrub, don't have to enable encryption right now, this simulates the array panic in the middle of enabling encyption */
    status = fbe_api_job_service_scrub_old_user_data();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for debug hook set */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++) {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            mut_printf(MUT_LOG_LOW, "=== wait for debug hook,  PVD obj id 0x%x \n", pvd_object_ids[rg_index]);
            status = fbe_test_wait_for_debug_hook_active(pvd_object_ids[rg_index], SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
                                                  FBE_PROVISION_DRIVE_SUBSTATE_SCRUB_INTENT, 
                                                  SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE, 
                                                  NULL, NULL);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        current_rg_config_p++;
    }

    /* reboot SPs */
    fbe_test_reboot_both_sp(rg_config_p);

    /* disable background zeroing */
    mike_the_knight_disable_zeroing(rg_config_p);

    fbe_test_wait_for_all_pvds_ready();

    /* load KMS */
    sep_config_load_kms_both_sps(NULL);
    
    mut_printf(MUT_LOG_LOW, " == checking scrub needed bits \n");
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++) {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            status = fbe_test_sep_util_wait_for_pvd_np_flags(pvd_object_ids[rg_index], FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_NEEDED, MIKE_THE_KNIGHT_TEST_WAIT_TIME);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            fbe_api_provision_drive_get_info(pvd_object_ids[rg_index], &pvd_info);
            MUT_ASSERT_INT_EQUAL(pvd_info.scrubbing_in_progress, FBE_TRUE);

            /* make sure scrubbing is in progress */
            status = fbe_api_database_get_system_scrub_progress(&scrub_progress);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_UINT64_NOT_EQUAL(scrub_progress.blocks_already_scrubbed, scrub_progress.total_capacity_in_blocks);

        }
        /* Goto next raid group
         */
        current_rg_config_p++;
    }

    prev_scrub_progress = scrub_progress;

    /* enable encryption */
    status = fbe_test_sep_util_enable_kms_encryption();
    mut_printf(MUT_LOG_TEST_STATUS, "Encryption enable status %d", status);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "enabling of encryption failed\n");

    /* enable background zeroing */
    mike_the_knight_enable_zeroing(rg_config_p);

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++) {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            /* wait for disk zeroing to complete */
            status = sep_zeroing_utils_check_disk_zeroing_status(pvd_object_ids[rg_index], MIKE_THE_KNIGHT_TEST_WAIT_TIME, FBE_LBA_INVALID);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            status = fbe_test_sep_util_wait_for_pvd_np_flags(pvd_object_ids[rg_index], FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_COMPLETE, MIKE_THE_KNIGHT_TEST_WAIT_TIME);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            fbe_api_provision_drive_get_info(pvd_object_ids[rg_index], &pvd_info);
            MUT_ASSERT_INT_EQUAL(pvd_info.scrubbing_in_progress, FBE_FALSE);

            /* make sure in progress capacity do not go back */
            status = fbe_api_database_get_system_scrub_progress(&scrub_progress);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_TRUE_MSG((scrub_progress.blocks_already_scrubbed >= prev_scrub_progress.blocks_already_scrubbed),
                                "scrub in progress should always move forward!");
            prev_scrub_progress = scrub_progress;

        }
        /* Goto next raid group
         */
        current_rg_config_p++;
    }

    /* make sure scrubbing is in progress */
    status = fbe_test_sep_util_wait_for_scrub_complete(MIKE_THE_KNIGHT_TEST_WAIT_TIME);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return ;

}
/******************************************
 * end mike_the_knight_test_rg_config_reboot_sp_dualsp()
 ******************************************/

/*!**************************************************************
 * mike_the_knight_scrubbing_consumed_test()
 ****************************************************************
 * @brief
 *  scrub consumed area.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ****************************************************************/
void mike_the_knight_scrubbing_consumed_test(void)
{
    fbe_test_create_raid_group_params_t params;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    char *test_cases_option_p = mut_get_user_option_value("-start_index");
    fbe_u32_t index = 0;

    rg_config_p = &mike_the_knight_raid_group_config_qual[0];

    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p)){
        fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                             NULL, mike_the_knight_test_rg_config_scrubbing_consumed,
                                                             MIKE_THE_KNIGHT_TEST_LUNS_PER_RAID_GROUP,
                                                             MIKE_THE_KNIGHT_CHUNKS_PER_LUN,
                                                             FBE_TRUE);
        params.b_encrypted = FBE_FALSE;
        fbe_test_run_test_on_rg_config_params(rg_config_p, &params); 
    }
    return;
}
/******************************************
 * end mike_the_knight_scrubbing_consumed_test()
 ******************************************/

/*!**************************************************************
 * mike_the_knight_dualsp_scrubbing_consumed_test()
 ****************************************************************
 * @brief
 *  dual sp scrubbing
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ****************************************************************/

void mike_the_knight_dualsp_scrubbing_consumed_test(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_test_create_raid_group_params_t params;
    char *test_cases_option_p = mut_get_user_option_value("-start_index");
    fbe_u32_t index = 0;

    if (test_level > 0)
    {
        rg_config_p = &mike_the_knight_raid_group_config_qual[0];  //extended[0];
    }
    else
    {
        rg_config_p = &mike_the_knight_raid_group_config_qual[0];
    }

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p)){
        fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                             NULL, mike_the_knight_test_rg_config_scrubbing_consumed,
                                                             MIKE_THE_KNIGHT_TEST_LUNS_PER_RAID_GROUP,
                                                             MIKE_THE_KNIGHT_CHUNKS_PER_LUN,
                                                             FBE_TRUE);
        params.b_encrypted = FBE_FALSE;
        fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    }

    /* Always disable dualsp I/O after running the test
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    return;
}
/******************************************
 * end mike_the_knight_dualsp_scrubbing_consumed_test()
 ******************************************/


/*!**************************************************************
 * mike_the_knight_destroy_rg_before_encryption()
 ****************************************************************
 * @brief
 *  destroy rg before encryption completes.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ****************************************************************/
void mike_the_knight_destroy_rg_before_encryption(void)
{
    fbe_test_create_raid_group_params_t params;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    char *test_cases_option_p = mut_get_user_option_value("-start_index");
    fbe_u32_t index = 0;

    rg_config_p = &mike_the_knight_raid_group_config_qual[0];

    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p)){
        fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                             NULL, mike_the_knight_test_rg_config_rg_destroy_before_encryption_complete,
                                                             MIKE_THE_KNIGHT_TEST_LUNS_PER_RAID_GROUP,
                                                             MIKE_THE_KNIGHT_CHUNKS_PER_LUN,
                                                             FBE_TRUE);
        params.b_encrypted = FBE_FALSE;
        fbe_test_run_test_on_rg_config_params(rg_config_p, &params); 
    }
    return;
}
/******************************************
 * end mike_the_knight_destroy_rg_before_encryption()
 ******************************************/

/*!**************************************************************
 * mike_the_knight_dualsp_destroy_rg_before_encryption()
 ****************************************************************
 * @brief
 *  destroy raid group before encryption completes for dual sp
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ****************************************************************/

void mike_the_knight_dualsp_destroy_rg_before_encryption(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_test_create_raid_group_params_t params;
    char *test_cases_option_p = mut_get_user_option_value("-start_index");
    fbe_u32_t index = 0;

    if (test_level > 0)
    {
        rg_config_p = &mike_the_knight_raid_group_config_qual[0];  //extended[0];
    }
    else
    {
        rg_config_p = &mike_the_knight_raid_group_config_qual[0];
    }

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p)){
        fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                             NULL, mike_the_knight_test_rg_config_rg_destroy_before_encryption_complete,
                                                             MIKE_THE_KNIGHT_TEST_LUNS_PER_RAID_GROUP,
                                                             MIKE_THE_KNIGHT_CHUNKS_PER_LUN,
                                                             FBE_TRUE);
        params.b_encrypted = FBE_FALSE;
        fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    }

    /* Always disable dualsp I/O after running the test
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    return;
}
/******************************************
 * end mike_the_knight_dualsp_destroy_rg_before_encryption()
 ******************************************/

/*!**************************************************************
 * mike_the_knight_normal_scrubbing_test()
 ****************************************************************
 * @brief
 *  normal scrub.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ****************************************************************/
void mike_the_knight_normal_scrubbing_test(void)
{
    fbe_test_create_raid_group_params_t params;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    char *test_cases_option_p = mut_get_user_option_value("-start_index");
    fbe_u32_t index = 0;

    rg_config_p = &mike_the_knight_raid_group_config_qual[0];

    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p)){
        fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                             NULL, mike_the_knight_test_rg_config_normal_scrubbing,
                                                             MIKE_THE_KNIGHT_TEST_LUNS_PER_RAID_GROUP,
                                                             MIKE_THE_KNIGHT_CHUNKS_PER_LUN,
                                                             FBE_TRUE);
        params.b_encrypted = FBE_FALSE;
        fbe_test_run_test_on_rg_config_params(rg_config_p, &params); 
    }
    return;
}
/******************************************
 * end mike_the_knight_normal_scrubbing_test()
 ******************************************/

/*!**************************************************************
 * mike_the_knight_dualsp_normal_scrubbing_test()
 ****************************************************************
 * @brief
 *  normal scrubbing for dual sp
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ****************************************************************/

void mike_the_knight_dualsp_normal_scrubbing_test(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_test_create_raid_group_params_t params;
    char *test_cases_option_p = mut_get_user_option_value("-start_index");
    fbe_u32_t index = 0;

    if (test_level > 0)
    {
        rg_config_p = &mike_the_knight_raid_group_config_qual[0];  //extended[0];
    }
    else
    {
        rg_config_p = &mike_the_knight_raid_group_config_qual[0];
    }

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p)){
        fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                             NULL, mike_the_knight_test_rg_config_normal_scrubbing,
                                                             MIKE_THE_KNIGHT_TEST_LUNS_PER_RAID_GROUP,
                                                             MIKE_THE_KNIGHT_CHUNKS_PER_LUN,
                                                             FBE_TRUE);
        params.b_encrypted = FBE_FALSE;
        fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    }

    /* Always disable dualsp I/O after running the test
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    return;
}
/******************************************
 * end mike_the_knight_dualsp_normal_scrubbing_test()
 ******************************************/


/*!**************************************************************
 * mike_the_knight_normal_scrubbing_drive_pull_test()
 ****************************************************************
 * @brief
 *  normal scrub with drive pull.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ****************************************************************/
void mike_the_knight_normal_scrubbing_drive_pull_test(void)
{
    fbe_test_create_raid_group_params_t params;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    char *test_cases_option_p = mut_get_user_option_value("-start_index");
    fbe_u32_t index = 0;

    rg_config_p = &mike_the_knight_raid_group_config_qual[0];

    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p)){
        fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                             NULL, mike_the_knight_test_rg_config_normal_scrubbing_drive_pull,
                                                             MIKE_THE_KNIGHT_TEST_LUNS_PER_RAID_GROUP,
                                                             MIKE_THE_KNIGHT_CHUNKS_PER_LUN,
                                                             FBE_TRUE);
        params.b_encrypted = FBE_FALSE;
        fbe_test_run_test_on_rg_config_params(rg_config_p, &params); 
    }
    return;
}
/******************************************
 * end mike_the_knight_normal_scrubbing_drive_pull_test()
 ******************************************/

/*!**************************************************************
 * mike_the_knight_dualsp_normal_scrubbing_drive_pull_test()
 ****************************************************************
 * @brief
 *  normal scrubbing with drive pulled for dual sp
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ****************************************************************/

void mike_the_knight_dualsp_normal_scrubbing_drive_pull_test(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_test_create_raid_group_params_t params;
    char *test_cases_option_p = mut_get_user_option_value("-start_index");
    fbe_u32_t index = 0;

    if (test_level > 0)
    {
        rg_config_p = &mike_the_knight_raid_group_config_qual[0];  //extended[0];
    }
    else
    {
        rg_config_p = &mike_the_knight_raid_group_config_qual[0];
    }

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p)){
        fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                             NULL, mike_the_knight_test_rg_config_normal_scrubbing_drive_pull,
                                                             MIKE_THE_KNIGHT_TEST_LUNS_PER_RAID_GROUP,
                                                             MIKE_THE_KNIGHT_CHUNKS_PER_LUN,
                                                             FBE_TRUE);
        params.b_encrypted = FBE_FALSE;
        fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    }

    /* Always disable dualsp I/O after running the test
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    return;
}
/******************************************
 * end mike_the_knight_dualsp_normal_scrubbing_drive_pull_test()
 ******************************************/

/*!**************************************************************
 * mike_the_knight_scrubbing_garbage_collection_test()
 ****************************************************************
 * @brief
 *  drive pulled triggers garbadge collection.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ****************************************************************/
void mike_the_knight_scrubbing_garbage_collection_test(void)
{
    fbe_test_create_raid_group_params_t params;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    char *test_cases_option_p = mut_get_user_option_value("-start_index");
    fbe_u32_t index = 0;

    rg_config_p = &mike_the_knight_raid_group_config_qual[0];

    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p)){
        fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                             NULL, mike_the_knight_test_rg_config_garbage_collection,
                                                             MIKE_THE_KNIGHT_TEST_LUNS_PER_RAID_GROUP,
                                                             MIKE_THE_KNIGHT_CHUNKS_PER_LUN,
                                                             FBE_TRUE);
        params.b_encrypted = FBE_FALSE;
        fbe_test_run_test_on_rg_config_params(rg_config_p, &params); 
    }
    return;
}
/******************************************
 * end mike_the_knight_scrubbing_garbage_collection_test()
 ******************************************/

/*!**************************************************************
 * mike_the_knight_dualsp_scrubbing_garbage_collection_test()
 ****************************************************************
 * @brief
 *  drive pulled triggers garbadge collection for dual sp
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ****************************************************************/

void mike_the_knight_dualsp_scrubbing_garbage_collection_test(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_test_create_raid_group_params_t params;
    char *test_cases_option_p = mut_get_user_option_value("-start_index");
    fbe_u32_t index = 0;

    if (test_level > 0)
    {
        rg_config_p = &mike_the_knight_raid_group_config_qual[0];  //extended[0];
    }
    else
    {
        rg_config_p = &mike_the_knight_raid_group_config_qual[0];
    }

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p)){
        fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                             NULL, mike_the_knight_test_rg_config_garbage_collection,
                                                             MIKE_THE_KNIGHT_TEST_LUNS_PER_RAID_GROUP,
                                                             MIKE_THE_KNIGHT_CHUNKS_PER_LUN,
                                                             FBE_TRUE);
        params.b_encrypted = FBE_FALSE;
        fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    }

    /* Always disable dualsp I/O after running the test
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    return;
}
/******************************************
 * end mike_the_knight_dualsp_scrubbing_garbage_collection_test()
 ******************************************/

/*!**************************************************************
 * mike_the_knight_dualsp_reboot_test()
 ****************************************************************
 * @brief
 *  normal scrubbing reboot for dual sp
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ****************************************************************/

void mike_the_knight_dualsp_reboot_test(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_test_create_raid_group_params_t params;
    char *test_cases_option_p = mut_get_user_option_value("-start_index");
    fbe_u32_t index = 0;

    if (test_level > 0)
    {
        rg_config_p = &mike_the_knight_raid_group_config_qual[0];  //extended[0];
    }
    else
    {
        rg_config_p = &mike_the_knight_raid_group_config_qual[0];
    }

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p)){
        fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                             NULL, mike_the_knight_test_rg_config_reboot_sp_dualsp,
                                                             MIKE_THE_KNIGHT_TEST_LUNS_PER_RAID_GROUP,
                                                             MIKE_THE_KNIGHT_CHUNKS_PER_LUN,
                                                             FBE_TRUE);
        params.b_encrypted = FBE_FALSE;
        fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    }

    /* Always disable dualsp I/O after running the test
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    return;
}
/******************************************
 * end mike_the_knight_dualsp_reboot_test()
 ******************************************/


/*!**************************************************************
 * mike_the_knight_setup()
 ****************************************************************
 * @brief
 *  Setup for raid.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ****************************************************************/
void mike_the_knight_setup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Initialize encryption.  Ensures that the test is in sync with 
     * the raid groups which have no key yet. 
     */
    fbe_test_encryption_init();

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t  raid_group_count = 0;

        if (test_level > 0)
        {
            rg_config_p = &mike_the_knight_raid_group_config_qual[0]; //extended[0];
        }
        else
        {
            rg_config_p = &mike_the_knight_raid_group_config_qual[0];
        }
        raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

        /* We no longer create the raid groups during the setup
         */
        mike_the_knight_setup_rg_config(rg_config_p);

        /* Use simulator PSL so that it does not take forever and a day to zero the system drives.
         */
        fbe_test_set_use_fbe_sim_psl(FBE_TRUE);

        fbe_test_create_physical_config_with_capacity(rg_config_p, raid_group_count, MIKE_THE_KNIGHT_EXTRA_DISK_BLOCKS);
        
        elmo_load_sep_and_neit();
    }

    fbe_test_wait_for_all_pvds_ready();
  
    /* load KMS */
    sep_config_load_kms(NULL);
    fbe_test_set_rg_vault_encrypt_wait_time(1000);

    return;
}
/**************************************
 * end mike_the_knight_setup()
 **************************************/

/*!**************************************************************
 * mike_the_knight_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup raid on both sps.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ****************************************************************/
void mike_the_knight_dualsp_setup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Initialize encryption.  Ensures that the test is in sync with 
     * the raid groups which have no key yet. 
     */
    fbe_test_encryption_init();
    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups;

        if (test_level > 0)
        {
            rg_config_p = &mike_the_knight_raid_group_config_qual[0]; //extended[0];
        }
        else
        {
            rg_config_p = &mike_the_knight_raid_group_config_qual[0];
        }

        /* We no longer create the raid groups during the setup
         */
        mike_the_knight_setup_rg_config(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* Use simulator PSL so that it does not take forever and a day to zero the system drives.
         */
        fbe_test_set_use_fbe_sim_psl(FBE_TRUE);

        /* Instantiate the drives on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

        fbe_test_create_physical_config_with_capacity(rg_config_p, num_raid_groups, mike_the_knight_extra_capacity);
        
        /*! Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_create_physical_config_with_capacity(rg_config_p, num_raid_groups, mike_the_knight_extra_capacity);

        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during 
         * the setup.
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_sep_and_neit_both_sps();
        
    }

    /* load KMS */
    sep_config_load_kms_both_sps(NULL);
    
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_set_rg_vault_encrypt_wait_time(1000);
    
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_test_set_rg_vault_encrypt_wait_time(1000);
    return;
}
/**************************************
 * end mike_the_knight_dualsp_setup()
 **************************************/
/*!**************************************************************
 * mike_the_knight_dualsp_reboot_setup()
 ****************************************************************
 * @brief
 *  Reboot both SPs with no extra capacity on the drives we use.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  7/9/2014 - Created. Rob Foley
 *
 ****************************************************************/
void mike_the_knight_dualsp_reboot_setup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    mike_the_knight_extra_capacity = 0;
    mike_the_knight_dualsp_setup();
    return;
}
/**************************************
 * end mike_the_knight_dualsp_reboot_setup()
 **************************************/

/*!**************************************************************
 * mike_the_knight_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the mike_the_knight_scrubbing test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ****************************************************************/

void mike_the_knight_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Restore the sector trace level to it's default.
     */ 
    fbe_test_sep_util_restore_sector_trace_level();

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {		
        fbe_test_sep_util_destroy_kms();
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end mike_the_knight_cleanup()
 ******************************************/

/*!**************************************************************
 * mike_the_knight_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the mike_the_knight_scrubbing test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ****************************************************************/

void mike_the_knight_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Restore the sector trace level to it's default.
     */ 
    fbe_test_sep_util_restore_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_restore_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {		
        fbe_test_sep_util_destroy_kms_both_sps();
        fbe_test_sep_util_destroy_neit_sep_physical_both_sps();
    }

    return;
}
/******************************************
 * end mike_the_knight_dualsp_cleanup()
 ******************************************/

/*************************
 * end file mike_the_knight_scrubbing.c
 *************************/


