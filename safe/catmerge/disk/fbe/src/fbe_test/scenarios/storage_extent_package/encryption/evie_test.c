/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file evie_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains an encryption I/O test.
 *
 * @author
 *  10/25/2013 - Created. Lili Chen
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "sep_tests.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "fbe_database.h"
#include "sep_utils.h"
#include "sep_test_io.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_block_transport_interface.h"
#include "fbe_block_transport.h"
#include "fbe/fbe_random.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_data_pattern.h"
#include "fbe_test_common_utils.h"
#include "sep_test_region_io.h"
#include "fbe/fbe_api_encryption_interface.h"
#include "sep_hook.h"
#include "fbe_test.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_random.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "sep_rebuild_utils.h"
#include "pp_utils.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_persist_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "sep_test_background_ops.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * evie_short_desc = "This scenario tests spare and copy for raid groups with system drives.";
char * evie_long_desc ="\
This scenario tests spare and copy for raid groups with system drives.\n\
\n"\
"-raid_test_level 0 and 1\n\
     We test additional combinations of raid group widths and element sizes at -rtl 1.\n\
\n\
\n\
Description last updated: 02/05/2014.\n";


/*!*******************************************************************
 * @def EVIE_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Max number of LUNs for each raid group.
 *
 *********************************************************************/
#define EVIE_TEST_LUNS_PER_RAID_GROUP 1

/*!*******************************************************************
 * @def EVIE_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define EVIE_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def EVIE_SMALL_IO_SIZE_MAX_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define EVIE_SMALL_IO_SIZE_MAX_BLOCKS 1024
/*!*******************************************************************
 * @def EVIE_LIGHT_THREAD_COUNT
 *********************************************************************
 * @brief Lightly loaded test to allow rekey to proceed.
 *
 *********************************************************************/
#define EVIE_LIGHT_THREAD_COUNT 1

/*!*******************************************************************
 * @def EVIE_MAX_IO_SIZE_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define EVIE_MAX_IO_SIZE_BLOCKS (FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1) * FBE_RAID_MAX_BE_XFER_SIZE //4*0x400*6

/*!*******************************************************************
 * @var evie_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t evie_raid_group_config[] = 
{
       
        /* width,   capacity    raid type,                  class,                 block size      RAID-id.    bandwidth.*/
        /* [1] RAID-5 4 + 1 Bound only on non-system drives */
        /*{5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            0,          0,
            {{0, 2, 10}, {0, 2, 11}, {0, 2, 12}, {0, 2, 13}, {0, 2, 14}}},*/
        /* [0] RAID-5 4 + 1 Bound including (2) system drives */
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            1,          0,
            {{0, 0, 0}, {1, 0, 3}, {1, 0, 0}, {1, 0, 1}, {2, 0, 0}}},
        {2,       0xE000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            2,          0,
            {{0, 0, 1}, {1, 0, 2}}},
        {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,  520,             3,          0,
            {{0, 0, 2}, {0, 0, 4}, {0, 0, 5}, {0, 0, 6}}},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    
};

fbe_test_rg_configuration_t evie_raid_group_config_vault[] = 
{
    /*! @note Although all fields must be valid only the system id is used to populate the test config+
     *                                                                                                |
     *                                                                                                v    */
    /*width capacity    raid type                   class               block size  system id                               bandwidth.*/
    {5,     0xF004800,  FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY, 520,       FBE_PRIVATE_SPACE_LAYOUT_RG_ID_VAULT,   0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

#define RG_CAPACITY_CONFIG1 0x100000
fbe_test_rg_configuration_t evie_raid_group_config1[] = 
{
       
        /* width,   capacity    raid type,                  class,                 block size      RAID-id.    bandwidth.*/
        /* [0] RAID-5 4 + 1 Bound  */
        /* {5,       0x20000000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            1,          0,
            {{1, 0, 0}, {1, 0, 1}, {1, 0, 2}, {1, 0, 3}, {2, 0, 0}}},*/
        {5,       0x100000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            1,          0,
            {{2, 0, 5}, {2, 0, 6}, {2, 0, 7}, {2, 0, 8}, {2, 0, 10}}},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    
};

static fbe_bool_t enable_encryption_in_test = FBE_FALSE;

extern void shaggy_test_fill_removed_disk_set(fbe_test_rg_configuration_t *rg_config_p,
                                       fbe_test_raid_group_disk_set_t*removed_disk_set_p,
                                       fbe_u32_t * position_p,
                                       fbe_u32_t number_of_drives);
extern void shaggy_test_reinsert_removed_disks(fbe_test_rg_configuration_t *rg_config_p,
                                        fbe_test_raid_group_disk_set_t*removed_disk_set_p,
                                        fbe_u32_t * position_p,
                                        fbe_u32_t    number_of_drives);
extern void diabolicdiscdemon_wait_for_proactive_spare_swap_in(fbe_object_id_t  vd_object_id,
                                                               fbe_edge_index_t spare_edge_index,
                                                               fbe_test_raid_group_disk_set_t * spare_location_p);
extern void diabolicdiscdemon_get_source_destination_edge_index(fbe_object_id_t  vd_object_id,
                                                                fbe_edge_index_t * source_edge_index_p,
                                                                fbe_edge_index_t * dest_edge_index_p);
extern void diabolicdiscdemon_wait_for_proactive_copy_completion(fbe_object_id_t vd_object_id);
extern void diabolicdiscdemon_wait_for_source_drive_swap_out(fbe_object_id_t vd_object_id,
                                                             fbe_edge_index_t source_edge_index);
extern void scoobert_run_test_on_rg_config_with_extra_disk(fbe_test_rg_configuration_t *rg_config_p,
                                void * context_p,
                                fbe_test_rg_config_test test_fn,
                                fbe_u32_t luns_per_rg,
                                fbe_u32_t chunks_per_lun);
static void evie_test_wait_system_rg_rebuild (void);

static fbe_lba_t evie_set_rebuild_hook(fbe_object_id_t vd_object_id, fbe_u32_t percent_copied_before_pause)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_lba_t                       checkpoint_to_pause_at = FBE_LBA_INVALID;

    /* If we want to stop right away just set the return value to 0.
     */
    if (percent_copied_before_pause == 0)
    {
        checkpoint_to_pause_at = 0;
        return checkpoint_to_pause_at;
    }

    /* Use common API to determine the `pause at' lba.
     */
    status = fbe_test_sep_background_pause_get_rebuild_checkpoint_to_pause_at(vd_object_id,
                                                                              percent_copied_before_pause,
                                                                              &checkpoint_to_pause_at);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Display the values we generated.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s: vd obj: 0x%x pause percent: %d lba: 0x%llxx",
               __FUNCTION__, vd_object_id, percent_copied_before_pause, checkpoint_to_pause_at);

    /* Set the debug hook
     */
    status = fbe_api_scheduler_add_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                              FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                              checkpoint_to_pause_at,
                                              0,
                                              SCHEDULER_CHECK_VALS_GT,
                                              SCHEDULER_DEBUG_ACTION_PAUSE); 
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return checkpoint_to_pause_at;
}

/*!**************************************************************
 * evie_insert_blank_new_drive()
 ****************************************************************
 * @brief
 *  Insert a new drive to the slot.
 *
 * @param port_number - port number.               
 * @param enclosure_number - enclosure number.               
 * @param slot_number - slot number.               
 *
 * @return None.   
 *
 * @author
 *  02/10/2014 - Created. Lili Chen
 *
 ****************************************************************/
static void evie_insert_blank_new_drive (fbe_u32_t port_number, 
                                         fbe_u32_t enclosure_number, 
                                         fbe_u32_t slot_number,
										 fbe_lba_t drive_capacity,
                                         fbe_api_terminator_device_handle_t *drive_handle_p_spa,
                                         fbe_api_terminator_device_handle_t *drive_handle_p_spb)
{
    fbe_status_t    status;
    fbe_sim_transport_connection_target_t   original_target;
    fbe_sas_address_t   blank_new_sas_address;
    
    original_target = fbe_api_sim_transport_get_target_server();
    blank_new_sas_address = fbe_test_pp_util_get_unique_sas_address();

    if (fbe_test_sep_util_get_dualsp_test_mode()) {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        status = fbe_test_pp_util_insert_sas_drive_extend(port_number, enclosure_number, slot_number, 
                                                          520, drive_capacity, 
                                                          blank_new_sas_address, FBE_SAS_DRIVE_SIM_520, drive_handle_p_spb);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "evie_insert_blank_new_drive: fail to insert drive from SPB");
    }

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_test_pp_util_insert_sas_drive_extend(port_number, enclosure_number, slot_number, 
                                                      520, drive_capacity, 
                                                      blank_new_sas_address, FBE_SAS_DRIVE_SIM_520, drive_handle_p_spa);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "evie_insert_blank_new_drive: fail to insert drive from SPA");


    fbe_api_sim_transport_set_target_server(original_target);
}
/******************************************
 * end evie_insert_blank_new_drive()
 ******************************************/

/*!**************************************************************
 * evie_test_wait_system_rg_rebuild()
 ****************************************************************
 * @brief
 *  Insert a new drive to the slot.
 *
 * @param port_number - port number.               
 * @param enclosure_number - enclosure number.               
 * @param slot_number - slot number.               
 *
 * @return None.   
 *
 * @author
 *  02/10/2014 - Created. Lili Chen
 *
 ****************************************************************/
static void evie_test_wait_system_rg_rebuild (void)
{
    fbe_object_id_t object_id;
    fbe_raid_group_number_t rg_id;
    fbe_api_base_config_get_width_t get_width;
    fbe_u8_t position;

    for (rg_id = FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR; rg_id <= FBE_PRIVATE_SPACE_LAYOUT_RG_ID_VAULT; rg_id++ ) 
    {
        fbe_api_database_lookup_raid_group_by_number(rg_id, &object_id);
        fbe_api_base_config_get_width(object_id, &get_width);
        for(position = 0; position < get_width.width; position++)
        {
            sep_rebuild_utils_wait_for_rb_comp_by_obj_id(object_id, position);
        }
    }
    return;
}
/******************************************
 * end evie_test_wait_system_rg_rebuild()
 ******************************************/

/*!**************************************************************
 * evie_test_check_pvd_key_info()
 ****************************************************************
 * @brief
 *  This function validates the keys in PVD.
 *
 * @param pvd_id - pvd object id.               
 *
 * @return None.   
 *
 * @author
 *  02/20/2014 - Created. Lili Chen
 *
 ****************************************************************/
static void evie_test_check_pvd_key_info(fbe_object_id_t pvd_id, fbe_edge_index_t server_index)
{
    fbe_status_t status;
    fbe_provision_drive_get_key_info_t get_key_t;
    fbe_encryption_key_mask_t mask0, mask1;
    fbe_u32_t bus, enclosure, slot;
    fbe_object_id_t port_object_id;
    fbe_u32_t num_keys = 0;

    get_key_t.server_index = server_index;
    status = fbe_api_get_pvd_key_info(pvd_id, &get_key_t);
    MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);

    if (get_key_t.key_mask == 0)
    {
        return;
    }

    /* Validate key mask */
    fbe_encryption_key_mask_get(&get_key_t.key_mask, 0, &mask0);
    fbe_encryption_key_mask_get(&get_key_t.key_mask, 1, &mask1);
    if (mask0 & FBE_ENCRYPTION_KEY_MASK_VALID)
    {
        MUT_ASSERT_INT_EQUAL (get_key_t.key1_valid, FBE_TRUE);
        num_keys++;
    }
    else
    {
        MUT_ASSERT_INT_EQUAL (get_key_t.key1_valid, FBE_FALSE);
    }
    if (mask1 & FBE_ENCRYPTION_KEY_MASK_VALID)
    {
        MUT_ASSERT_INT_EQUAL (get_key_t.key2_valid, FBE_TRUE);
        num_keys++;
    }
    else
    {
        MUT_ASSERT_INT_EQUAL (get_key_t.key2_valid, FBE_FALSE);
    }
    MUT_ASSERT_INT_NOT_EQUAL(num_keys, 0);

    status = fbe_api_provision_drive_get_location(pvd_id, &bus, &enclosure, &slot);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_get_port_object_id_by_location(bus, &port_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(port_object_id, FBE_OBJECT_ID_INVALID);
    /* Check the port_object_id for the keys are correct */
    MUT_ASSERT_INT_EQUAL(get_key_t.port_object_id, port_object_id);

    return;
}
/******************************************
 * end evie_test_check_pvd_key_info()
 ******************************************/

/*!**************************************************************
 * evie_test_lockbox_backup_restore()
 ****************************************************************
 * @brief
 *  Insert a new drive to the slot.
 *
 * @param port_number - port number.               
 * @param enclosure_number - enclosure number.               
 * @param slot_number - slot number.               
 *
 * @return None.   
 *
 * @author
 *  02/27/2014 - Created. Lili Chen
 *
 ****************************************************************/
void evie_test_lockbox_backup_restore(void)
{
    fbe_u8_t fname[129];
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_database_backup_info_t backup_info;
    fbe_u32_t i;

    status = fbe_api_database_get_backup_info(&backup_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(backup_info.encryption_backup_state, FBE_ENCRYPTION_BACKUP_REQUIERED);

    for (i = 0; i <= 90; i++)
    {
        fbe_api_sleep(3000);
        status = fbe_api_database_get_backup_info(&backup_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (backup_info.encryption_backup_state == FBE_ENCRYPTION_BACKUP_REQUIERED) {
            break;
        }
    }
    MUT_ASSERT_INT_EQUAL(backup_info.encryption_backup_state, FBE_ENCRYPTION_BACKUP_REQUIERED);

    fbe_zero_memory(fname, 129);
    mut_printf(MUT_LOG_TEST_STATUS, "== Starting backup ==");
    status = fbe_test_sep_util_kms_start_backup(fname);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_PENDING, status);

    fbe_api_sleep(6000);

    mut_printf(MUT_LOG_TEST_STATUS, "== Starting backup ==");
    status = fbe_test_sep_util_kms_start_backup(fname);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_api_sleep(2000);
    mut_printf(MUT_LOG_TEST_STATUS, "== Completing backup ==");
    status = fbe_test_sep_util_kms_complete_backup(fname);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_api_sleep(1000);
	status = fbe_api_database_get_backup_info(&backup_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(backup_info.encryption_backup_state, FBE_ENCRYPTION_BACKUP_COMPLETED);

    fbe_api_sleep(1000);
    mut_printf(MUT_LOG_TEST_STATUS, "== Starting restore ==");
    status = fbe_test_sep_util_kms_restore(fname);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return;
}
/******************************************
 * end evie_test_lockbox_backup_restore()
 ******************************************/

static void evie_test_wait_pvd_config_type(fbe_object_id_t pvd_id, fbe_provision_drive_config_type_t expected_type)
{
    fbe_status_t status;
    fbe_u32_t wait_seconds = 10;
	fbe_api_provision_drive_info_t provision_drive_info_p;

    while (wait_seconds--) {
        status = fbe_api_provision_drive_get_info(pvd_id, &provision_drive_info_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
 
        if (provision_drive_info_p.config_type != expected_type)
        {
            fbe_api_sleep(1000);
        }
        else
        {
            return;
        }
    }
    MUT_ASSERT_INT_EQUAL(provision_drive_info_p.config_type, expected_type);
}

/*!**************************************************************
 * evie_test_wait_pvd_key_unregister()
 ****************************************************************
 * @brief
 *  This function validates the keys in PVD.
 *
 * @param pvd_id - pvd object id.               
 *
 * @return None.   
 *
 * @author
 *  03/05/2014 - Created. Lili Chen
 *
 ****************************************************************/
static void evie_test_wait_pvd_key_unregister(fbe_object_id_t pvd_id)
{
    fbe_status_t status;
    fbe_provision_drive_get_key_info_t get_key;
    fbe_bool_t key_destroyed = FBE_FALSE;
    fbe_u32_t wait_seconds = 30;
    fbe_u32_t num_upstream_objects;

    fbe_test_sep_util_get_upstream_object_count(pvd_id, &num_upstream_objects);

    /* For system drives, the keys for user RG is not in the first location */
    get_key.server_index = num_upstream_objects;

    while (wait_seconds--) {
        status = fbe_api_get_pvd_key_info(pvd_id, &get_key);
        MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);

        /* Validate key mask */
        if (get_key.key_mask != 0 ||
            get_key.key1_valid || get_key.key2_valid ||
            get_key.port_object_id != FBE_OBJECT_ID_INVALID)
        {
            fbe_api_sleep(1000);
        }
        else
        {
            key_destroyed = FBE_TRUE;
            break;
        }
    }
    MUT_ASSERT_INT_EQUAL (key_destroyed, FBE_TRUE);

    return;
}
/******************************************
 * end evie_test_wait_pvd_key_unregister()
 ******************************************/

/*!**************************************************************
 * evie_test_check_pvd_encryption_mode_unconsumed()
 ****************************************************************
 * @brief
 *  This function validates the encryption mode in PVD right after 
 *  the PVD is unconsumed.
 *
 * @param pvd_id - pvd object id.               
 *
 * @return None.   
 *
 * @author
 *  03/05/2014 - Created. Lili Chen
 *
 ****************************************************************/
static void evie_test_check_pvd_encryption_mode_unconsumed(fbe_object_id_t pvd_id)
{
    fbe_status_t status;
    fbe_base_config_encryption_mode_t   encryption_mode;
    fbe_base_config_encryption_mode_t   vault_encryption_mode;

    evie_test_wait_pvd_config_type(pvd_id, FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED);

    status = fbe_api_base_config_get_encryption_mode(pvd_id, &encryption_mode);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Getting encryption mode failed\n");

    if (fbe_private_space_layout_object_id_is_system_pvd(pvd_id))
    {
        status = fbe_api_base_config_get_encryption_mode(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG, &vault_encryption_mode);
        MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Getting vault encryption mode failed\n");
        if (vault_encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_INVALID)
        {
            vault_encryption_mode = FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED;
        }
        MUT_ASSERT_INT_EQUAL_MSG(encryption_mode, vault_encryption_mode, "Encryption mode not expected\n");
    }
    else
    {
        MUT_ASSERT_INT_EQUAL_MSG(encryption_mode, FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED, "Encryption mode not expected\n");
    }

    return;
}
/******************************************
 * end evie_test_check_pvd_encryption_mode_unconsumed()
 ******************************************/


/*!**************************************************************
 * evie_test_check_pvd_encryption_mode_consumed()
 ****************************************************************
 * @brief
 *  This function validates the encryption mode in PVD right after 
 *  the PVD is consumed.
 *
 * @param pvd_id - pvd object id.               
 *
 * @return None.   
 *
 * @author
 *  03/05/2014 - Created. Lili Chen
 *
 ****************************************************************/
static void evie_test_check_pvd_encryption_mode_consumed(fbe_object_id_t pvd_id, fbe_object_id_t vd_id)
{
    fbe_status_t status;
    fbe_base_config_encryption_mode_t   encryption_mode;
    fbe_base_config_encryption_mode_t   vd_encryption_mode;
    fbe_base_config_encryption_mode_t   vault_encryption_mode;

    evie_test_wait_pvd_config_type(pvd_id, FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID);

    status = fbe_api_base_config_get_encryption_mode(pvd_id, &encryption_mode);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Getting encryption mode failed\n");

    if (vd_id != FBE_OBJECT_ID_INVALID)
    {
        status = fbe_api_base_config_get_encryption_mode(vd_id, &vd_encryption_mode);
        MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Getting vd encryption mode failed\n");
        if (vd_encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_INVALID)
        {
            vd_encryption_mode = FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED;
        }
    }
    else
    {
        vd_encryption_mode = FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED;	
    }

    if (fbe_private_space_layout_object_id_is_system_pvd(pvd_id))
    {
        status = fbe_api_base_config_get_encryption_mode(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG, &vault_encryption_mode);
        MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Getting vault encryption mode failed\n");
        if (vault_encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_INVALID)
        {
            vault_encryption_mode = FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED;
        }

        if (vault_encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS || 
			vd_encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS) {
            MUT_ASSERT_INT_EQUAL_MSG(encryption_mode, FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS, "Encryption mode not expected\n");
        } else if (vault_encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS || 
			vd_encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS) {
            MUT_ASSERT_INT_EQUAL_MSG(encryption_mode, FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS, "Encryption mode not expected\n");
        } else if (vault_encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED && 
			vd_encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED) {
            MUT_ASSERT_INT_EQUAL_MSG(encryption_mode, FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED, "Encryption mode not expected\n");
        } else {
            MUT_ASSERT_INT_EQUAL_MSG(encryption_mode, FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED, "Encryption mode not expected\n");
        }
    }
    else
    {
        MUT_ASSERT_INT_EQUAL_MSG(encryption_mode, FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED, "Encryption mode not expected\n");
    }

    return;
}
/******************************************
 * end evie_test_check_pvd_encryption_mode_consumed()
 ******************************************/

/*!**************************************************************
 * evie_test_check_key_info_after_rg_destroy()
 ****************************************************************
 * @brief
 *  This function validates the keys in PVD.
 *
 * @param pvd_id - pvd object id.               
 *
 * @return None.   
 *
 * @author
 *  03/05/2014 - Created. Lili Chen
 *
 ****************************************************************/
static void evie_test_check_key_info_after_rg_destroy(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t rg_index = 0, i;
    fbe_object_id_t pvd_object_id;
    fbe_status_t status;

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        for (i = 0; i < current_rg_config_p->width; i++)
        {
            status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->rg_disk_set[i].bus,
                                                                    current_rg_config_p->rg_disk_set[i].enclosure,
                                                                    current_rg_config_p->rg_disk_set[i].slot,
                                                                    &pvd_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            evie_test_wait_pvd_key_unregister(pvd_object_id);

            evie_test_check_pvd_encryption_mode_unconsumed(pvd_object_id);
        }

        current_rg_config_p++;
    }
}
/******************************************
 * end evie_test_check_key_info_after_rg_destroy()
 ******************************************/

/*!**************************************************************
 * evie_test_check_key_info_after_rg_create()
 ****************************************************************
 * @brief
 *  This function validates the keys in PVD.
 *
 * @param pvd_id - pvd object id.               
 *
 * @return None.   
 *
 * @author
 *  03/05/2014 - Created. Lili Chen
 *
 ****************************************************************/
static void evie_test_check_key_info_after_rg_create(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t rg_index = 0, i;
    fbe_object_id_t pvd_object_id;
    fbe_status_t status;
    fbe_u32_t num_upstream_objects;

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* Make sure our objects are in the ready state.*/
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(current_rg_config_p);

        for (i = 0; i < current_rg_config_p->width; i++)
        {
            status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->rg_disk_set[i].bus,
                                                                    current_rg_config_p->rg_disk_set[i].enclosure,
                                                                    current_rg_config_p->rg_disk_set[i].slot,
                                                                    &pvd_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            fbe_test_sep_util_get_upstream_object_count(pvd_object_id, &num_upstream_objects);
            evie_test_check_pvd_key_info(pvd_object_id, num_upstream_objects - 1);

            evie_test_check_pvd_encryption_mode_consumed(pvd_object_id, FBE_OBJECT_ID_INVALID); /* This is bind case */
        }

        current_rg_config_p++;
    }
}
/******************************************
 * end evie_test_check_key_info_after_rg_create()
 ******************************************/

void evie_test_is_unregistering_during_reboot_and_proactive_copy(fbe_test_rg_configuration_t *rg_config_p, void *context)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_sim_transport_connection_target_t active_target;
    fbe_sim_transport_connection_target_t passive_target;
    fbe_object_id_t vd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t pvd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index = 0;
    fbe_object_id_t vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    //fbe_edge_index_t source_edge_index;
    //fbe_edge_index_t dest_edge_index;
    fbe_edge_index_t initial_source_edge_index;
    fbe_edge_index_t initial_dest_edge_index;
    fbe_lba_t checkpoint_to_pause_at = FBE_LBA_INVALID;
    fbe_api_provision_drive_info_t source_drive_pvd_info;
    fbe_test_raid_group_disk_set_t spare_drive_location;
    fbe_object_id_t dest_pvd_object_id;
    //fbe_provision_drive_copy_to_status_t copy_status;

    mut_printf(MUT_LOG_TEST_STATUS, "==%s entry==", __FUNCTION__);

    //fbe_api_sleep(50000);

    fbe_test_sep_get_active_target_id(&active_target);
    passive_target = (active_target == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    /* Enable automatic VD hot spare during testing */
    status = fbe_api_control_automatic_hot_spare(FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get RG, PVD and VD object ids and check if the "chosen" one is READY*/
    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);
    fbe_test_get_pvd_object_ids(rg_config_p, &pvd_object_ids[0], 0);
    fbe_test_get_vd_object_ids(rg_config_p, &vd_object_ids[0], 0);
    rg_index = fbe_random()%raid_group_count;
    current_rg_config_p = rg_config_p + rg_index;
    rg_object_id = rg_object_ids[rg_index];
    pvd_object_id = pvd_object_ids[rg_index];
    vd_object_id = vd_object_ids[rg_index];
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(current_rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "Chosen PVD %d", pvd_object_id);

    /* Wait for encryption to complete */
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for Encryption to finish successfully.");
    fbe_api_sim_transport_set_target_server(passive_target); 
    status = fbe_test_sep_util_wait_all_rg_encryption_state(rg_config_p, FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED,
                                                            FBE_TEST_WAIT_TIMEOUT_MS,
                                                            FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_sep_util_wait_rg_encryption_state((fbe_object_id_t)FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG, 
                                                        FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED, 
                                                        FBE_TEST_WAIT_TIMEOUT_MS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "Encryption rekey finished successfully on passive.");
    fbe_api_sim_transport_set_target_server(active_target);
    status = fbe_test_sep_util_wait_all_rg_encryption_state(rg_config_p, FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED,
                                                            FBE_TEST_WAIT_TIMEOUT_MS,
                                                            FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_sep_util_wait_rg_encryption_state((fbe_object_id_t)FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG, 
                                                        FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED, 
                                                        FBE_TEST_WAIT_TIMEOUT_MS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "Encryption rekey finished successfully on active.");

    mut_printf(MUT_LOG_TEST_STATUS, "==%s setup complete==", __FUNCTION__);

    /*Populate initial source and destination edge indices*/
    diabolicdiscdemon_get_source_destination_edge_index(vd_object_id, &initial_source_edge_index, &initial_dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, initial_source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, initial_dest_edge_index);

    /*Reboot SPs: Shutdown SPs together before bringing them up*/
    mut_printf(MUT_LOG_TEST_STATUS, "Reboot both SPs");
    status = fbe_test_common_reboot_both_sps_neit_sep_kms(active_target, NULL, NULL, NULL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_wait_for_object_lifecycle_state(vd_object_id, 
                                                     FBE_LIFECYCLE_STATE_READY, 
                                                     60000, 
                                                     FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*Add Unregister debug hook*/
    status = fbe_test_add_debug_hook_passive(pvd_object_id,
                                          SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ENCRYPTION,
                                          FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_UNREGISTER_KEYS,
                                          NULL,
                                          NULL,
                                          SCHEDULER_CHECK_VALS_EQUAL,
                                          SCHEDULER_DEBUG_ACTION_CONTINUE); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    /* Add debug hook to pause at 50% rebuilt before proceeding */
    checkpoint_to_pause_at = evie_set_rebuild_hook(vd_object_id, 50);
    mut_printf(MUT_LOG_TEST_STATUS, "Added rebuild hook vd obj: 0x%x lba: 0x%llx ==", 
               vd_object_id, checkpoint_to_pause_at);

    /*Set EOL on source PVD*/
    status = fbe_api_provision_drive_get_info(pvd_object_id, &source_drive_pvd_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "Set EOL on source pvd: 0x%x source bes: %d_%d_%d", 
               pvd_object_id,
               source_drive_pvd_info.port_number,
               source_drive_pvd_info.enclosure_number,
               source_drive_pvd_info.slot_number);
    status = fbe_api_provision_drive_set_eol_state(pvd_object_id, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Wait for the proactive spare to swap-in and hit 50% */
    diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, initial_dest_edge_index, &spare_drive_location); 
    status = fbe_test_wait_for_debug_hook(vd_object_id, 
                                         SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                         FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                         SCHEDULER_CHECK_VALS_GT, 
                                         SCHEDULER_DEBUG_ACTION_PAUSE, 
                                         checkpoint_to_pause_at,
                                         0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_provision_drive_get_obj_id_by_location(spare_drive_location.bus,
                                                            spare_drive_location.enclosure,
                                                            spare_drive_location.slot,    
                                                            &dest_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "Proactive copy started to dest pvd: 0x%x dest bes: %d_%d_%d", 
               dest_pvd_object_id, 
               spare_drive_location.bus,
               spare_drive_location.enclosure,
               spare_drive_location.slot);

    /* Get the counter for number of times the unregister key was sent. */
    passive_target = (active_target == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    status = fbe_api_sim_transport_set_target_server(passive_target);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_wait_for_hook_counter(pvd_object_id, 
                                         SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ENCRYPTION, 
                                         FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_UNREGISTER_KEYS,
                                         SCHEDULER_CHECK_VALS_EQUAL, SCHEDULER_DEBUG_ACTION_CONTINUE,
                                         0, 0, 0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Unexpected hook counter value. Expected:0\n");
    status = fbe_api_sim_transport_set_target_server(active_target); 
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*Delete Unregister debug hook*/
    status = fbe_test_del_debug_hook_passive(pvd_object_id,
                                              SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ENCRYPTION,
                                              FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_UNREGISTER_KEYS,
                                              NULL,
                                              NULL,
                                              SCHEDULER_CHECK_VALS_EQUAL,
                                              SCHEDULER_DEBUG_ACTION_CONTINUE);  
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*Clear pause at 50% and wait for proactive copy completion*/
    status = fbe_api_scheduler_del_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                              FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                              checkpoint_to_pause_at, 
                                              0, 
                                              SCHEDULER_CHECK_VALS_GT, 
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    diabolicdiscdemon_wait_for_proactive_copy_completion(vd_object_id);
    mut_printf(MUT_LOG_TEST_STATUS, "Proactive copy completed vd obj: 0x%x", vd_object_id);
    
#if 0
    /*Populate the after swap source and destination edge indices*/
    diabolicdiscdemon_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

    /*Clear the EOL state in the source PVD after source swapped out*/
    mut_printf(MUT_LOG_TEST_STATUS, "Clear EOL on source pvd: 0x%x source bes: %d_%d_%d", 
               pvd_object_id,
               source_drive_pvd_info.port_number,
               source_drive_pvd_info.enclosure_number,
               source_drive_pvd_info.slot_number);
    status = fbe_api_provision_drive_clear_eol_state(pvd_object_id, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*Wait for PVD READY state after EOL clear*/
    status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id, FBE_LIFECYCLE_STATE_READY, 10000 /*10 sec*/, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*Initiate user copy from the spare to original drive*/
    mut_printf(MUT_LOG_TEST_STATUS, "Initiate user copy from dest PVD 0x%x to source PVD 0x%x", dest_pvd_object_id, pvd_object_id);
    status = fbe_api_copy_to_replacement_disk(dest_pvd_object_id, pvd_object_id, &copy_status);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Wait copy to complete */
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for copy to complete vd: 0x%x", vd_object_id);
    status = fbe_test_copy_wait_for_completion(vd_object_id, dest_edge_index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*Populate the source and destination edge indices which should be initial state*/
    diabolicdiscdemon_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);
    MUT_ASSERT_INT_EQUAL(initial_source_edge_index, source_edge_index);
    MUT_ASSERT_INT_EQUAL(initial_dest_edge_index, dest_edge_index);

    /*Verify rebuild is complete on the PSL Triple mirror*/
    status = fbe_test_sep_util_wait_rg_pos_rebuild(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR, 
                                                   0, 
                                                   FBE_TEST_SEP_UTIL_RB_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_sep_util_wait_rg_pos_rebuild(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR, 
                                                   1, 
                                                   FBE_TEST_SEP_UTIL_RB_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_sep_util_wait_rg_pos_rebuild(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR, 
                                                   2, 
                                                   FBE_TEST_SEP_UTIL_RB_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
#endif

    mut_printf(MUT_LOG_TEST_STATUS, "======Function Exit: %s======", __FUNCTION__);
}

/*!**************************************************************
 * evie_test_rg_config()
 ****************************************************************
 * @brief
 *  Run test with steps:
 *  1) create a user raid group with system drives;
 *  2) remove a system drive;
 *  3) verify permanent spare to another drive;
 *  4) insert a new drive to the system drive location;
 *  5) copy to the new system drive.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  02/10/2014 - Created. Lili Chen
 *
 ****************************************************************/
void evie_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index = 0;
    fbe_object_id_t vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t vd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t pvd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_u32_t dest_index[FBE_TEST_MAX_RAID_GROUP_COUNT] = {0};
    fbe_test_raid_group_disk_set_t  removed_disk_set[FBE_TEST_MAX_RAID_GROUP_COUNT] = {0};
    fbe_u32_t position = 0;
    fbe_api_base_config_downstream_object_list_t downstream_object_list;
    fbe_provision_drive_copy_to_status_t copy_status;
    fbe_test_raid_group_disk_set_t spare_drive_location;
    fbe_api_rdgen_context_t *context_p = NULL;
    fbe_u32_t num_luns;
    fbe_api_rdgen_peer_options_t peer_options = FBE_API_RDGEN_PEER_OPTIONS_INVALID;
    fbe_api_terminator_device_handle_t drive_handle_p_spa, drive_handle_p_spb;
    fbe_u32_t num_upstream_objects = 0;
    fbe_bool_t b_dualsp = fbe_test_sep_util_get_dualsp_test_mode();
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_provision_drive_control_get_mp_key_handle_t mp_key_handle;
    fbe_api_terminator_device_handle_t port_handle;
    fbe_u8_t            terminator_key[FBE_ENCRYPTION_KEY_SIZE];
    fbe_u8_t            terminator_key1[FBE_ENCRYPTION_KEY_SIZE];
    fbe_u8_t            current_terminator_key[FBE_ENCRYPTION_KEY_SIZE];
    fbe_bool_t          key_match = 1;
    fbe_api_provision_drive_info_t pvd_info;

    #define IO_DURING_ENC_INCREMENT_CHECKPOINT 0x1800
    #define IO_DURING_ENC_MAX_TEST_POINTS 2
    #define IO_DURING_ENC_WAIT_IO_COUNT 5

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_control_automatic_hot_spare(FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    /* Write a zero pattern to all luns.
     */
    big_bird_write_zero_background_pattern();

    /* Start I/O (w/r/c)
     */
    if (fbe_test_sep_util_get_dualsp_test_mode()) {
        peer_options = FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting I/O ==", __FUNCTION__);
    big_bird_start_io(rg_config_p, &context_p, FBE_U32_MAX, EVIE_MAX_IO_SIZE_BLOCKS, 
                      FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                      FBE_TEST_RANDOM_ABORT_TIME_INVALID, peer_options);
    fbe_test_io_wait_for_io_count(context_p, num_luns, IO_DURING_ENC_WAIT_IO_COUNT, FBE_TEST_WAIT_TIMEOUT_MS);


    /* Speed up VD hot spare during testing */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(5); /* 5 second hotspare timeout */

    if (enable_encryption_in_test)
    {
        status = fbe_test_sep_util_enable_kms_encryption();
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

//    evie_test_lockbox_backup_restore();

    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);
    fbe_test_get_pvd_object_ids(rg_config_p, &pvd_object_ids[0], 0 /* position */);
    fbe_test_get_vd_object_ids(rg_config_p, &vd_object_ids[0], 0);
    fbe_test_copy_determine_dest_index(&vd_object_ids[0], raid_group_count, &dest_index[0]);
            
    /* Start sparing */
    rg_index = fbe_random()%raid_group_count;
    current_rg_config_p = rg_config_p + rg_index;

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for Encryption to finish successfully.");
    if (b_dualsp) {
        fbe_api_sim_transport_set_target_server(other_sp); 
        status = fbe_test_sep_util_wait_all_rg_encryption_state(rg_config_p, FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED,
                                                                FBE_TEST_WAIT_TIMEOUT_MS,
                                                                FBE_FALSE /* do not check top level */);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "Encryption rekey finished successfully.");
        fbe_api_sim_transport_set_target_server(this_sp);
    }
    status = fbe_test_sep_util_wait_all_rg_encryption_state(rg_config_p, FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED,
                                                            FBE_TEST_WAIT_TIMEOUT_MS,
                                                            FBE_FALSE /* do not check top level */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "Encryption rekey finished successfully.");

    mut_printf(MUT_LOG_TEST_STATUS, "== Starting on rg %d ==", rg_index);
    //current_rg_config_p = rg_config_p;
    //for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        rg_object_id = rg_object_ids[rg_index];
        pvd_object_id = pvd_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];

        /* Make sure our objects are in the ready state.*/
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(current_rg_config_p);

        /* Save the old Key data */
        mp_key_handle.index = 3; /* Vault Edge Index */
        status = fbe_api_provision_drive_get_miniport_key_handle(pvd_object_id, &mp_key_handle);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_terminator_get_port_handle(0, &port_handle);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        fbe_api_terminator_get_encryption_keys(port_handle, mp_key_handle.mp_handle_2, terminator_key);

        /* Remove a drive */
        shaggy_test_fill_removed_disk_set(current_rg_config_p, &removed_disk_set[rg_index], &position, 1);
        shaggy_remove_drive(current_rg_config_p,
                            0,
                            &current_rg_config_p->drive_handle[0]);

        /* Wait spare to swap in */
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for sparing rg: 0x%x vd: 0x%x==", rg_object_id, vd_object_id);
        shaggy_test_wait_permanent_spare_swap_in(current_rg_config_p, 0);

        fbe_test_sep_util_get_ds_object_list(vd_object_id, &downstream_object_list);
        MUT_ASSERT_INT_EQUAL(downstream_object_list.number_of_downstream_objects, 1);

        /* Check swapped in drive key info */
        fbe_test_sep_util_get_upstream_object_count(downstream_object_list.downstream_object_list[0], &num_upstream_objects);
        evie_test_check_pvd_key_info(downstream_object_list.downstream_object_list[0], num_upstream_objects - 1);
        evie_test_check_pvd_encryption_mode_consumed(downstream_object_list.downstream_object_list[0], vd_object_id);

        /* Check swapped out drive key info */
        evie_test_wait_pvd_key_unregister(pvd_object_id);
        evie_test_check_pvd_encryption_mode_unconsumed(pvd_object_id);

        /* Wait rebuild to complete */
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for rebuild rg: 0x%x vd: 0x%x==", rg_object_id, vd_object_id);
        sep_rebuild_utils_wait_for_rb_comp(current_rg_config_p, 0);
        sep_rebuild_utils_check_bits(rg_object_id); 

        /*Reinsert the removed drive*/
#if 0
        shaggy_test_reinsert_removed_disks(current_rg_config_p, &removed_disk_set[rg_index], &position, 1);
#else
        evie_insert_blank_new_drive(removed_disk_set[rg_index].bus, removed_disk_set[rg_index].enclosure, removed_disk_set[rg_index].slot,
                                    (fbe_lba_t)(1.5*TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY), &drive_handle_p_spa, &drive_handle_p_spb);

        fbe_api_sleep(5000); /*wait for database to process drives*/
        status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id, FBE_LIFECYCLE_STATE_READY, 120000, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
#endif

        /* Get the new key data and validate that they keys are different which means KMS has pushed the new key */
        mp_key_handle.index = 3; /* Vault Edge Index */
        status = fbe_api_provision_drive_get_miniport_key_handle(pvd_object_id, &mp_key_handle);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        fbe_api_terminator_get_encryption_keys(port_handle, mp_key_handle.mp_handle_2, current_terminator_key);
        key_match = fbe_equal_memory(current_terminator_key, terminator_key, FBE_ENCRYPTION_KEY_SIZE);

        MUT_ASSERT_TRUE_MSG((key_match != 1), " Vault Key values does matches\n");

        /* Start user copy */
        mut_printf(MUT_LOG_TEST_STATUS, "== start copy to source 0x%x dst 0x%x==", downstream_object_list.downstream_object_list[0], pvd_object_id);
        status = fbe_api_copy_to_replacement_disk(downstream_object_list.downstream_object_list[0], pvd_object_id, &copy_status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* wait for the proactive spare to swap-in. */
        diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_index[rg_index], &spare_drive_location);

        /* Check swapped in drive key info */
        fbe_test_sep_util_get_upstream_object_count(pvd_object_id, &num_upstream_objects);
        evie_test_check_pvd_key_info(pvd_object_id, num_upstream_objects - 1);
        evie_test_check_pvd_encryption_mode_consumed(pvd_object_id, vd_object_id);

        /* Wait copy to complete */
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for copy rg: 0x%x vd: 0x%x==", rg_object_id, vd_object_id);
        status = fbe_test_copy_wait_for_completion(vd_object_id, dest_index[rg_index]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Check swapped out drive key info */
        evie_test_wait_pvd_key_unregister(downstream_object_list.downstream_object_list[0]);
        evie_test_check_pvd_encryption_mode_unconsumed(downstream_object_list.downstream_object_list[0]);

        //evie_test_wait_system_rg_rebuild(0);

        current_rg_config_p++;
    } 

    /* We want to make sure that if HS is disabled and a new drive is inserted into system slot, make sure that new key is 
     * provided for that drive 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Disable Automatic HS...");
    status = fbe_api_control_automatic_hot_spare(FBE_FALSE);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Unable to turn off HS\n");

    mut_printf(MUT_LOG_TEST_STATUS, "Remove drive and save the old keys information ...");
    /* Save the old Key data */
    mp_key_handle.index = 3; /* Vault Edge Index */
    status = fbe_api_provision_drive_get_miniport_key_handle(pvd_object_id, &mp_key_handle);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_terminator_get_port_handle(0, &port_handle);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_api_terminator_get_encryption_keys(port_handle, mp_key_handle.mp_handle_2, terminator_key);

    mp_key_handle.index = 4; /* User RG */
    status = fbe_api_provision_drive_get_miniport_key_handle(pvd_object_id, &mp_key_handle);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_api_terminator_get_encryption_keys(port_handle, mp_key_handle.mp_handle_1, terminator_key1);

    status = fbe_api_provision_drive_get_info(pvd_object_id, &pvd_info);
    /* Remove the drive */
    sep_rebuild_utils_remove_drive_by_location(pvd_info.port_number, pvd_info.enclosure_number, pvd_info.slot_number, 
                                               &current_rg_config_p->drive_handle[0]);

    mut_printf(MUT_LOG_TEST_STATUS, "Insert a new drive ...");
    /* Insert the new drive */
    evie_insert_blank_new_drive(pvd_info.port_number, pvd_info.enclosure_number, pvd_info.slot_number,
                                    (fbe_lba_t)(1.5*TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY), &drive_handle_p_spa, &drive_handle_p_spb);

    fbe_api_sleep(5000); /*wait for database to process drives*/
    status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id, FBE_LIFECYCLE_STATE_READY, 120000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Get the new key data and validate that they keys are different which means KMS has pushed the new key */
    mp_key_handle.index = 3; /* Vault Edge Index */
    status = fbe_api_provision_drive_get_miniport_key_handle(pvd_object_id, &mp_key_handle);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_api_terminator_get_encryption_keys(port_handle, mp_key_handle.mp_handle_2, current_terminator_key);
    key_match = fbe_equal_memory(current_terminator_key, terminator_key, FBE_ENCRYPTION_KEY_SIZE);

    MUT_ASSERT_TRUE_MSG((key_match != 1), " Vault Key values does matches\n");

    mp_key_handle.index = 4; /* User RG */
    status = fbe_api_provision_drive_get_miniport_key_handle(pvd_object_id, &mp_key_handle);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_api_terminator_get_encryption_keys(port_handle, mp_key_handle.mp_handle_1, current_terminator_key);
    key_match = fbe_equal_memory(current_terminator_key, terminator_key1, FBE_ENCRYPTION_KEY_SIZE);

    MUT_ASSERT_TRUE_MSG((key_match != 1), " RG Key values does matches\n");

    status = fbe_api_control_automatic_hot_spare(FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Stop I/O and check for errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s halting I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s  I/O halted ==", __FUNCTION__);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);

    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&context_p, num_luns);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);

    /* Make sure no errors occurred. */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end evie_test_rg_config()
 ******************************************/

/*!**************************************************************
 * evie_run_test_vault_encryption_in_progress()
 ****************************************************************
 * @brief
 *  Run test with steps:
 *  1) create a user raid group with system drives;
 *  2) remove a system drive;
 *  3) verify permanent spare to another drive;
 *  4) insert a new drive to the system drive location;
 *  5) copy to the new system drive.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  02/10/2014 - Created. Lili Chen
 *
 ****************************************************************/
void evie_run_test_vault_encryption_in_progress(fbe_test_rg_configuration_t *rg_config_p, void * context)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t *vault_config_p = NULL;
    fbe_u32_t rg_index;
    fbe_u32_t num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_bool_t add_hooks = FBE_FALSE;

    mut_printf(MUT_LOG_TEST_STATUS, "== Adding hooks to stop vault encryption. ===");
    vault_config_p = &evie_raid_group_config_vault[0];
    /* Populate the raid groups test information
     */
    status = fbe_test_populate_system_rg_config(vault_config_p,
                                                1, /* LUNs per RG. */
                                                3 /* Chunks. */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_sep_util_raid_group_refresh_disk_info(vault_config_p, 1);

    /* Add the debug hooks.
     */
    status = fbe_test_use_rg_hooks(vault_config_p,
                                   FBE_U32_MAX, /* only top object */
                                   SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                   FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_VAULT_ENTRY,
                                   0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                   FBE_TEST_HOOK_ACTION_ADD);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	/* Confirmed with Lili - this use case is obsolete - we are not starting encryption untill Vault is encrypted */
#if 0 /* The test is failing randomly when Hooks are set for user RG's, need to revisit test case */
    if (fbe_random()%2)
    {
        add_hooks = FBE_TRUE;
        current_rg_config_p = rg_config_p;
        for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           2, /* only top object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_BEFORE_INITIAL_KEY,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_ADD);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            current_rg_config_p++;
        }
    }
#endif

    /* kick off a rekey.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Starting Encryption. ===");
    //status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    status = fbe_test_sep_util_enable_kms_encryption();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (add_hooks)
    {
        current_rg_config_p = rg_config_p;
        for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           2, /* only top object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_BEFORE_INITIAL_KEY,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_WAIT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            current_rg_config_p++;
        }
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Waiting hooks to stop vault encryption. ===");
    status = fbe_test_use_rg_hooks(vault_config_p,
                                   FBE_U32_MAX, /* only top object */
                                   SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                   FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_VAULT_ENTRY,
                                   0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                   FBE_TEST_HOOK_ACTION_WAIT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_api_sleep(1000);

    evie_test_rg_config(rg_config_p, context);

    if (add_hooks)
    {
        current_rg_config_p = rg_config_p;
        for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           2, /* only top object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_BEFORE_INITIAL_KEY,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_DELETE);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            current_rg_config_p++;
        }
    }
    fbe_api_sleep(1000);

}
/******************************************
 * end evie_run_test_vault_encryption_in_progress()
 ******************************************/

/*!**************************************************************
 * evie_vault_encryption_in_progress_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 5 objects.  The setting of sep dualsp
 *  mode determines if the test will run I/Os on both SPs or not.
 *  sep dualsp has been configured in the setup routine.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  02/18/2014 - Created. Lili Chen
 *
 ****************************************************************/
void evie_vault_encryption_in_progress_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    rg_config_p = &evie_raid_group_config[0];
    
    scoobert_run_test_on_rg_config_with_extra_disk(rg_config_p,NULL,evie_run_test_vault_encryption_in_progress,
                                   EVIE_TEST_LUNS_PER_RAID_GROUP,
                                   EVIE_CHUNKS_PER_LUN);

    evie_test_check_key_info_after_rg_destroy(rg_config_p);

    return;
}
/******************************************
 * end evie_vault_encryption_in_progress_test()
 ******************************************/

/*!**************************************************************
 * evie_run_test_drive_relocation()
 ****************************************************************
 * @brief
 *  Run test with steps:
 *  1) create a user raid group with system drives;
 *  2) remove a user drive;
 *  3) relocate to another slot (different port);
 *  4) verify that the pvd re-registers keys with new port.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  02/19/2014 - Created. Lili Chen
 *
 ****************************************************************/
void evie_run_test_drive_relocation(fbe_test_rg_configuration_t *rg_config_p, void * context)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index = 0;
    fbe_u32_t drive_index = 0;
    fbe_test_raid_group_disk_set_t *drive_to_remove;
    fbe_test_raid_group_disk_set_t slot_to_insert = {2, 0, 3};
    fbe_object_id_t pvd_id, new_pvd_id;
    fbe_api_terminator_device_handle_t drive_in_new_slot;

    /* Remove the drive originally in the slot */
    status = fbe_test_pp_util_pull_drive(slot_to_insert.bus, slot_to_insert.enclosure, slot_to_insert.slot, &drive_in_new_slot); 
    MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        status = fbe_test_pp_util_pull_drive(slot_to_insert.bus, slot_to_insert.enclosure, slot_to_insert.slot, &drive_in_new_slot); 
        MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }
    status = fbe_test_sep_util_wait_for_pvd_state(slot_to_insert.bus, slot_to_insert.enclosure, slot_to_insert.slot,
                                                  FBE_LIFECYCLE_STATE_FAIL, 20000);
    MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        status = fbe_test_sep_util_wait_for_pvd_state(slot_to_insert.bus, slot_to_insert.enclosure, slot_to_insert.slot,
                                                      FBE_LIFECYCLE_STATE_FAIL, 20000);
        MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }

    /* Start test on a random raid group */
    rg_index = fbe_random()%raid_group_count;
    current_rg_config_p = rg_config_p + rg_index;
    mut_printf(MUT_LOG_TEST_STATUS, "== Starting on rg %d ==", rg_index);

    /* Remove a random drive, except the first one (system drive) */
    drive_index = fbe_random()%(current_rg_config_p->width - 1) + 1;
    drive_to_remove = &(current_rg_config_p->rg_disk_set[drive_index]);

    status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_remove->bus, drive_to_remove->enclosure, drive_to_remove->slot, &pvd_id);
    MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== Pulling the Drive %d_%d_%d ===\n", drive_to_remove->bus, drive_to_remove->enclosure, drive_to_remove->slot);
    shaggy_remove_drive(current_rg_config_p,
                        drive_index,
                        &current_rg_config_p->drive_handle[drive_index]);
    fbe_api_sleep(2000);
	
    mut_printf(MUT_LOG_LOW, "=== Re-inserting the Drive %d_%d_%d ===", slot_to_insert.bus, slot_to_insert.enclosure, slot_to_insert.slot);
    status = fbe_test_pp_util_reinsert_drive(slot_to_insert.bus, slot_to_insert.enclosure, slot_to_insert.slot, 
                                             current_rg_config_p->drive_handle[drive_index]);
    MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        status = fbe_test_pp_util_reinsert_drive(slot_to_insert.bus, slot_to_insert.enclosure, slot_to_insert.slot, 
                                                 current_rg_config_p->peer_drive_handle[drive_index]);
        MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }

    /* Wait for the pvd object to be in ready state */
    status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                          pvd_id, FBE_LIFECYCLE_STATE_READY,
                                                                          30000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(slot_to_insert.bus, slot_to_insert.enclosure, slot_to_insert.slot, &new_pvd_id);
    MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL (new_pvd_id, pvd_id);

    evie_test_check_pvd_key_info(pvd_id, 0);
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        evie_test_check_pvd_key_info(pvd_id, 0);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }

    return;
}
/******************************************
 * end evie_run_test_drive_relocation()
 ******************************************/

/*!**************************************************************
 * evie_drive_relocation_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 5 objects.  The setting of sep dualsp
 *  mode determines if the test will run I/Os on both SPs or not.
 *  sep dualsp has been configured in the setup routine.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  02/19/2014 - Created. Lili Chen
 *
 ****************************************************************/
void evie_drive_relocation_test(void)
{
    fbe_status_t status;
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    status = fbe_test_sep_util_enable_kms_encryption();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    rg_config_p = &evie_raid_group_config[0];
    
    scoobert_run_test_on_rg_config_with_extra_disk(rg_config_p,NULL,evie_run_test_drive_relocation,
                                   EVIE_TEST_LUNS_PER_RAID_GROUP,
                                   EVIE_CHUNKS_PER_LUN);

    return;
}
/******************************************
 * end evie_drive_relocation_test()
 ******************************************/

/*!**************************************************************
 * evie_drive_relocation_dualsp_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 5 objects.  The setting of sep dualsp
 *  mode determines if the test will run I/Os on both SPs or not.
 *  sep dualsp has been configured in the setup routine.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  02/19/2014 - Created. Lili Chen
 *
 ****************************************************************/
void evie_drive_relocation_dualsp_test(void)
{
    fbe_status_t status;
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    status = fbe_test_sep_util_enable_kms_encryption();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    rg_config_p = &evie_raid_group_config[0];
    
    scoobert_run_test_on_rg_config_with_extra_disk(rg_config_p,NULL,evie_run_test_drive_relocation,
                                   EVIE_TEST_LUNS_PER_RAID_GROUP,
                                   EVIE_CHUNKS_PER_LUN);

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/******************************************
 * end evie_drive_relocation_dualsp_test()
 ******************************************/

/*!**************************************************************
 * evie_reboot_and_proactive_dualsp_test()
 ****************************************************************
 * @brief
 *  Test is unregistration is happening AR706368
 *  1. Shutdown both SPs and bring them up.
 *  2. Set EOL on one of the PVDs
 *  3. Verify if unregister is not happening.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  03/13/2015 - Created. Saranyadevi Ganesan
 *
 ****************************************************************/
void evie_reboot_and_proactive_dualsp_test(void)
{

    fbe_status_t status;
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    status = fbe_test_sep_util_enable_kms_encryption();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    rg_config_p = &evie_raid_group_config[0];
    
    scoobert_run_test_on_rg_config_with_extra_disk(rg_config_p,NULL,
                                   evie_test_is_unregistering_during_reboot_and_proactive_copy,
                                   EVIE_TEST_LUNS_PER_RAID_GROUP,
                                   EVIE_CHUNKS_PER_LUN);

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/*********************************************
 * end evie_reboot_and_proactive_dualsp_test()
 *********************************************/

/*!**************************************************************
 * evie_run_test_check_encryption_mode()
 ****************************************************************
 * @brief
 *  Check encryption mode after RGs are created.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  02/18/2014 - Created. Lili Chen
 *
 ****************************************************************/
void evie_run_test_check_encryption_mode(fbe_test_rg_configuration_t *rg_config_p, void * context)
{
    evie_test_check_key_info_after_rg_create(rg_config_p);

    return;
}
/******************************************
 * end evie_run_test_check_encryption_mode()
 ******************************************/

/*!**************************************************************
 * evie_encryption_mode_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 5 objects.  The setting of sep dualsp
 *  mode determines if the test will run I/Os on both SPs or not.
 *  sep dualsp has been configured in the setup routine.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  02/18/2014 - Created. Lili Chen
 *
 ****************************************************************/
void evie_encryption_mode_test(void)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t *vault_config_p = NULL;
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    mut_printf(MUT_LOG_TEST_STATUS, "== Adding hooks to stop vault encryption. ===");
    vault_config_p = &evie_raid_group_config_vault[0];
    /* Populate the raid groups test information
     */
    status = fbe_test_populate_system_rg_config(vault_config_p,
                                                1, /* LUNs per RG. */
                                                3 /* Chunks. */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_sep_util_raid_group_refresh_disk_info(vault_config_p, 1);

    /* Add the debug hooks.
     */
    status = fbe_test_use_rg_hooks(vault_config_p,
                                   FBE_U32_MAX, /* only top object */
                                   SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                   FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_VAULT_ENTRY,
                                   0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                   FBE_TEST_HOOK_ACTION_ADD);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* kick off a rekey.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Starting Encryption. ===");
    //status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    status = fbe_test_sep_util_enable_kms_encryption();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== Waiting hooks to stop vault encryption. ===");
    status = fbe_test_use_rg_hooks(vault_config_p,
                                   FBE_U32_MAX, /* only top object */
                                   SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                   FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_VAULT_ENTRY,
                                   0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                   FBE_TEST_HOOK_ACTION_WAIT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_api_sleep(1000);

    rg_config_p = &evie_raid_group_config[0];
    scoobert_run_test_on_rg_config_with_extra_disk(rg_config_p,NULL,evie_run_test_check_encryption_mode,
                                   EVIE_TEST_LUNS_PER_RAID_GROUP,
                                   EVIE_CHUNKS_PER_LUN);

    evie_test_check_key_info_after_rg_destroy(rg_config_p);
}
/******************************************
 * end evie_encryption_mode_test()
 ******************************************/


/*!**************************************************************
 * evie_run_test_bind_unbind()
 ****************************************************************
 * @brief
 *  bind/unbind LUN during RG encryption.
 *
 * @param  rg_config_p           
 *
 * @return None.
 *
 * @author
 *  03/13/2014 - Created. Lili Chen
 *
 ****************************************************************/
void evie_run_test_bind_unbind(fbe_test_rg_configuration_t *rg_config_p, void * context)
{
    fbe_status_t status;
    fbe_lba_t                       lun_exported_capacity;
    fbe_api_lun_create_t            fbe_lun_create_req;
    fbe_object_id_t                 lu_id; 
    fbe_object_id_t                 rg_object_id; 
    fbe_job_service_error_type_t    job_error_type;
    fbe_api_lun_destroy_t           fbe_lun_destroy_req;
    fbe_test_rg_configuration_t * current_rg_config_p = rg_config_p;
    fbe_object_id_t     pvd_id[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_u32_t disk_position, i;
    fbe_api_raid_group_get_info_t rg_info;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for Zeroing to complete ==", __FUNCTION__);
    for (disk_position = 0; disk_position < current_rg_config_p->width; disk_position++)
    {
        status = fbe_api_provision_drive_get_obj_id_by_location( current_rg_config_p->rg_disk_set[disk_position].bus,
                                                                 current_rg_config_p->rg_disk_set[disk_position].enclosure,
                                                                 current_rg_config_p->rg_disk_set[disk_position].slot,
                                                                 &pvd_id[disk_position]);
        status = fbe_test_zero_wait_for_disk_zeroing_complete(pvd_id[disk_position], 1000000);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Make sure our objects are in the ready state.*/
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(current_rg_config_p);

    /* Set a pause hook to stop the raid group request encryption start */
    status = fbe_test_add_debug_hook_active(rg_object_id,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                            FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START,
                                            0, 0, 
                                            SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* kick off a rekey.
     */
    status = fbe_test_sep_util_enable_kms_encryption();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_test_wait_for_debug_hook_active(rg_object_id, 
                                                 SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION, 
                                                 FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START,
                                                 SCHEDULER_CHECK_STATES,
                                                 SCHEDULER_DEBUG_ACTION_PAUSE,
                                                 0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_test_del_debug_hook_active(rg_object_id,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                            FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START,
                                            0, 0, 
                                            SCHEDULER_CHECK_STATES, 
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // Calculate the lun exported size based on rg and number of luns to bind.
    status = fbe_api_lun_calculate_max_lun_size(rg_config_p[0].raid_group_id, RG_CAPACITY_CONFIG1, 
                                          1, &lun_exported_capacity);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    for (i = 0; i < 6; i++) {
        // Destroy a LUN
        fbe_lun_destroy_req.lun_number = 0;
        status = fbe_api_destroy_lun(&fbe_lun_destroy_req, FBE_TRUE, 300000, &job_error_type);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, " LUN destroyed successfully! \n");
        fbe_api_sleep(1000);

        // Create a LUN
        fbe_zero_memory(&fbe_lun_create_req, sizeof(fbe_api_lun_create_t));
        fbe_lun_create_req.raid_type = rg_config_p[0].raid_type;
        fbe_lun_create_req.raid_group_id = rg_config_p[0].raid_group_id; /*sep_standard_logical_config_one_r5 is 5 so this is what we use*/
        fbe_lun_create_req.lun_number = 0;
        fbe_lun_create_req.capacity = (0x28 * 0x800 * 4); /*lun_exported_capacity;*/
        fbe_lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
        fbe_lun_create_req.ndb_b = FBE_FALSE;
        fbe_lun_create_req.noinitialverify_b = FBE_FALSE;
        fbe_lun_create_req.addroffset = FBE_LBA_INVALID; /* set to a valid offset for NDB */
        fbe_lun_create_req.world_wide_name.bytes[0] = (fbe_u8_t)fbe_random() & 0xf;
        fbe_lun_create_req.world_wide_name.bytes[1] = (fbe_u8_t)fbe_random() & 0xf;

        status = fbe_api_create_lun(&fbe_lun_create_req, FBE_TRUE, 300000, &lu_id, &job_error_type);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);   
        mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X created successfully! \n", __FUNCTION__, lu_id);
        fbe_api_sleep(1000);

        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);   
        mut_printf(MUT_LOG_TEST_STATUS, " %s: rekey checkpoint 0x%llx\n", __FUNCTION__, rg_info.rekey_checkpoint);
	}

	return;
}
/******************************************
 * end evie_run_test_bind_unbind()
 ******************************************/

/*!**************************************************************
 * evie_bind_unbind_test()
 ****************************************************************
 * @brief
 *  bind/unbind LUN during RG encryption.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  03/13/2014 - Created. Lili Chen
 *
 ****************************************************************/
void evie_bind_unbind_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    rg_config_p = &evie_raid_group_config1[0];

    scoobert_run_test_on_rg_config_with_extra_disk(rg_config_p,NULL,evie_run_test_bind_unbind,
                                   3,
                                   0x28);

    evie_test_check_key_info_after_rg_destroy(rg_config_p);

    return;
}
/******************************************
 * end evie_bind_unbind_test()
 ******************************************/

/*!**************************************************************
 * evie_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 5 objects.  The setting of sep dualsp
 *  mode determines if the test will run I/Os on both SPs or not.
 *  sep dualsp has been configured in the setup routine.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  02/10/2014 - Created. Lili Chen
 *
 ****************************************************************/
void evie_test(void)
{
    fbe_status_t status;
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_random()%2) {
        status = fbe_test_sep_util_enable_kms_encryption();
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    } else {
        enable_encryption_in_test = FBE_TRUE;
    }

    rg_config_p = &evie_raid_group_config[0];
    
    scoobert_run_test_on_rg_config_with_extra_disk(rg_config_p,NULL,evie_test_rg_config,
                                   EVIE_TEST_LUNS_PER_RAID_GROUP,
                                   EVIE_CHUNKS_PER_LUN);

    evie_test_check_key_info_after_rg_destroy(rg_config_p);

    return;
}
/******************************************
 * end evie_test()
 ******************************************/

/*!**************************************************************
 * evie_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 5 I/O test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  02/10/2014 - Created. Lili Chen
 *
 ****************************************************************/
void evie_setup(void)
{
    //fbe_status_t status;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Initialize encryption.  Ensures that the test is in sync with 
     * the raid groups which have no key yet. 
     */
    fbe_test_encryption_init();

    if (fbe_test_util_is_simulation())
    {
        
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups = 0;
        
        rg_config_p = &evie_raid_group_config[0];
       
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* initialize the number of extra drive required by each rg 
          */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        scoobert_physical_config(520);

        elmo_load_sep_and_neit();

        /* Set the trace level to info. */
        elmo_set_trace_level(FBE_TRACE_LEVEL_INFO);
        
        /* load KMS */
        sep_config_load_kms(NULL);

        //status = fbe_test_sep_util_enable_kms_encryption();
        //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Initialize any required fields 
     */
    fbe_test_common_util_test_setup_init();
    
    //fbe_test_sep_drive_disable_background_zeroing_for_all_pvds();
    //fbe_test_sep_drive_disable_sniff_verify_for_all_pvds();


    return;
}
/**************************************
 * end evie_setup()
 **************************************/

/*!**************************************************************
 * evie_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  02/10/2014 - Created. Lili Chen
 *
 ****************************************************************/

void evie_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Re-enable automatic sparing*/
    fbe_api_control_automatic_hot_spare(FBE_TRUE);

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
 * end evie_cleanup()
 ******************************************/

/*!**************************************************************
 * evie_dualsp_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 5 objects.  The setting of sep dualsp
 *  mode determines if the test will run I/Os on both SPs or not.
 *  sep dualsp has been configured in the setup routine.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  02/10/2014 - Created. Lili Chen
 *
 ****************************************************************/
void evie_dualsp_test(void)
{
    fbe_status_t status;
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    status = fbe_test_sep_util_enable_kms_encryption();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    rg_config_p = &evie_raid_group_config[0];
    
    scoobert_run_test_on_rg_config_with_extra_disk(rg_config_p,NULL,evie_test_rg_config,
                                   EVIE_TEST_LUNS_PER_RAID_GROUP,
                                   EVIE_CHUNKS_PER_LUN);

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/******************************************
 * end evie_dualsp_test()
 ******************************************/

/*!**************************************************************
 * evie_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 5 I/O test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  02/10/2014 - Created. Lili Chen
 *
 ****************************************************************/
void evie_dualsp_setup(void)
{
    //fbe_status_t status;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Initialize encryption.  Ensures that the test is in sync with 
     * the raid groups which have no key yet. 
     */
    fbe_test_encryption_init();

    if (fbe_test_util_is_simulation())
    {
        
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups = 0;
        
        rg_config_p = &evie_raid_group_config[0];
       
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* initialize the number of extra drive required by each rg 
          */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        /* Instantiate the drives on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        scoobert_physical_config(520);
       
        /*! Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        scoobert_physical_config(520);

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_sep_and_neit_both_sps();
    
        /* After sep is loaded setup notifications
         */
        fbe_test_common_setup_notifications(FBE_TRUE /* This is a dual SP test*/);   
        
        /* load KMS */
        sep_config_load_kms_both_sps(NULL);

        //status = fbe_test_sep_util_enable_kms_encryption();
        //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Initialize any required fields 
     */
    fbe_test_common_util_test_setup_init();
    
    //fbe_test_sep_drive_disable_background_zeroing_for_all_pvds();
    //fbe_test_sep_drive_disable_sniff_verify_for_all_pvds();


    return;
}
/**************************************
 * end evie_dualsp_setup()
 **************************************/

/*!**************************************************************
 * evie_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  02/10/2014 - Created. Lili Chen
 *
 ****************************************************************/

void evie_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    fbe_test_common_cleanup_notifications(FBE_TRUE /* This is a dualsp test */);

    /* Re-enable automatic sparing*/
    fbe_api_control_automatic_hot_spare(FBE_TRUE);

    /* Restore the sector trace level to it's default.
     */ 
    fbe_test_sep_util_restore_sector_trace_level();

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_kms_both_sps();

        /* First execute teardown on SP B then on SP A
        */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_destroy_neit_sep_physical();

        /* First execute teardown on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end evie_dualsp_cleanup()
 ******************************************/

void evie_run_quiesce_test(fbe_test_rg_configuration_t *rg_config_p, void * context)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index = 0, test_count;
    fbe_object_id_t vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t vd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t pvd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    //fbe_api_rdgen_context_t *context_p = NULL;
    //fbe_api_rdgen_peer_options_t peer_options = FBE_API_RDGEN_PEER_OPTIONS_INVALID;
    //fbe_api_lun_create_t            fbe_lun_create_req;
    fbe_api_lun_destroy_t           fbe_lun_destroy_req;
    fbe_job_service_error_type_t    job_error_type;
    fbe_u32_t lun_number;
    fbe_object_id_t lu_id;
    #define IO_DURING_ENC_WAIT_IO_COUNT 5

#if 0
    /* Write a zero pattern to all luns.
     */
    big_bird_write_zero_background_pattern();

    /* Start I/O (w/r/c)
     */
    if (fbe_test_sep_util_get_dualsp_test_mode()) {
        peer_options = FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting I/O ==", __FUNCTION__);
    big_bird_start_io(rg_config_p, &context_p, FBE_U32_MAX, EVIE_MAX_IO_SIZE_BLOCKS, 
                      FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                      FBE_TEST_RANDOM_ABORT_TIME_INVALID, peer_options);
    fbe_test_io_wait_for_io_count(context_p, num_luns, IO_DURING_ENC_WAIT_IO_COUNT, FBE_TEST_WAIT_TIMEOUT_MS);
#endif

    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);
    fbe_test_get_pvd_object_ids(rg_config_p, &pvd_object_ids[0], 0 /* position */);
    fbe_test_get_vd_object_ids(rg_config_p, &vd_object_ids[0], 0);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        rg_object_id = rg_object_ids[rg_index];
        pvd_object_id = pvd_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];
        lun_number = current_rg_config_p->logical_unit_configuration_list[0].lun_number;

        status = fbe_api_database_lookup_lun_by_number(lun_number, &lu_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Make sure our objects are in the ready state.*/
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(current_rg_config_p);

        for ( test_count = 0; test_count < 3; test_count++)
        {
            fbe_u32_t sleep_time = 3000 + (fbe_random() % 200);

            /* Destroy a LUN */
            fbe_lun_destroy_req.lun_number = lun_number;
            status = fbe_api_destroy_lun(&fbe_lun_destroy_req, FBE_TRUE, 300000, &job_error_type);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            mut_printf(MUT_LOG_TEST_STATUS, " LUN destroyed successfully! \n");
            fbe_api_sleep(1000);

            /* Add hook to DB servcie */
            status = fbe_api_database_add_hook(FBE_DATABASE_HOOK_TYPE_WAIT_IN_UPDATE_TRANSACTION);
            MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to add hook!");
            mut_printf(MUT_LOG_TEST_STATUS, "%s: Debug hook is now added\n", __FUNCTION__);

            /* Create a LUN */
            status = fbe_test_sep_util_create_logical_unit(&current_rg_config_p->logical_unit_configuration_list[0]);
            MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Creation of Logical Unit failed.");

            /* Wait for the DB hook to be triggered */
            status = fbe_api_database_wait_hook(FBE_DATABASE_HOOK_TYPE_WAIT_IN_UPDATE_TRANSACTION, 30000);
            MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), 
                        "Wait to trigger hook timed out!");
            MUT_ASSERT_TRUE_MSG((status == FBE_STATUS_OK), "FAIL to trigger hook!");
            mut_printf(MUT_LOG_TEST_STATUS, "%s: hook is now triggered\n", __FUNCTION__);

            /* Add hook in LUN */
            status = fbe_api_scheduler_add_debug_hook(lu_id, SCHEDULER_MONITOR_STATE_LUN_ZERO,
                                                      FBE_LUN_SUBSTATE_ZERO_START, 
                                                      NULL, NULL, 
                                                      SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Delete DB hook */
            status = fbe_api_database_remove_hook(FBE_DATABASE_HOOK_TYPE_WAIT_IN_UPDATE_TRANSACTION);
            MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to remove hook!");


            /* Wait for LUN hook */
            status = fbe_test_wait_for_debug_hook(lu_id, SCHEDULER_MONITOR_STATE_LUN_ZERO,
                                                      FBE_LUN_SUBSTATE_ZERO_START, 
                                                      SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE, 
                                                      NULL, NULL);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Delete LUN hook */
            status = fbe_api_scheduler_del_debug_hook(lu_id, SCHEDULER_MONITOR_STATE_LUN_ZERO,
                                                      FBE_LUN_SUBSTATE_ZERO_START, 
                                                      NULL, NULL, 
                                                      SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            mut_printf(MUT_LOG_TEST_STATUS, "=== starting %s iteration %d sleep_time: %d msec===", 
                       __FUNCTION__, test_count, sleep_time);
            fbe_api_sleep(sleep_time);

            status = fbe_api_provision_drive_quiesce(pvd_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            status = fbe_test_sep_utils_wait_pvd_for_base_config_clustered_flags(pvd_object_id,
                                                                             30, 
                                                                             FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED,
                                                                             FBE_FALSE);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            status = fbe_api_provision_drive_unquiesce(pvd_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            status = fbe_test_sep_utils_wait_pvd_for_base_config_clustered_flags(pvd_object_id,
                                                                             30, 
                                                                             FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED,
                                                                             FBE_TRUE);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Wait for the pvd object to be in ready state */
            status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                          lu_id, FBE_LIFECYCLE_STATE_READY,
                                                                          30000, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        }

        current_rg_config_p++;
    } 

#if 0
    /* Stop I/O and check for errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s halting I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s  I/O halted ==", __FUNCTION__);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);

    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&context_p, num_luns);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
#endif

    /* Make sure no errors occurred. */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}

/*!**************************************************************
 * evie_quiesce_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 5 objects.  The setting of sep dualsp
 *  mode determines if the test will run I/Os on both SPs or not.
 *  sep dualsp has been configured in the setup routine.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  03/17/2014 - Created. Lili Chen
 *
 ****************************************************************/
void evie_quiesce_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    rg_config_p = &evie_raid_group_config[0];
    
    scoobert_run_test_on_rg_config_with_extra_disk(rg_config_p,NULL,evie_run_quiesce_test,
                                   EVIE_TEST_LUNS_PER_RAID_GROUP,
                                   EVIE_CHUNKS_PER_LUN);

    return;
}
/******************************************
 * end evie_quiesce_test()
 ******************************************/

/*!**************************************************************
 * evie_quiesce_test_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 5 I/O test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  03/17/2014 - Created. Lili Chen
 *
 ****************************************************************/
void evie_quiesce_test_setup(void)
{
    //fbe_status_t status;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    if (fbe_test_util_is_simulation())
    {
        
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups = 0;
        
        rg_config_p = &evie_raid_group_config[0];
       
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* initialize the number of extra drive required by each rg 
          */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        scoobert_physical_config(520);

        elmo_load_sep_and_neit();

        /* Set the trace level to info. */
        elmo_set_trace_level(FBE_TRACE_LEVEL_INFO);
    }

    /* Initialize any required fields 
     */
    fbe_test_common_util_test_setup_init();
    
    //fbe_test_sep_drive_disable_background_zeroing_for_all_pvds();
    //fbe_test_sep_drive_disable_sniff_verify_for_all_pvds();

    return;
}
/**************************************
 * end evie_quiesce_test_setup()
 **************************************/

/*!**************************************************************
 * evie_quiesce_test_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  03/17/2014 - Created. Lili Chen
 *
 ****************************************************************/

void evie_quiesce_test_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Re-enable automatic sparing*/
    fbe_api_control_automatic_hot_spare(FBE_TRUE);

    /* Restore the sector trace level to it's default.
     */ 
    fbe_test_sep_util_restore_sector_trace_level();

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end evie_quiesce_test_cleanup()
 ******************************************/


/*************************
 * end file evie_test.c
 *************************/
