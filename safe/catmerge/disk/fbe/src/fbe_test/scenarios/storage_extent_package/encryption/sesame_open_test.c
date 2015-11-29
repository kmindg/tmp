/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file sesame_open_test.c
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

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * sesame_open_short_desc = "This scenario validates keys during encryption re-key and spares.";
char * sesame_open_long_desc ="\
This scenario validates keys during encryption re-key and spares.\n\
\n"\
"-raid_test_level 0 and 1\n\
     We test additional combinations of raid group widths and element sizes at -rtl 1.\n\
\n\
\n\
Description last updated: 02/05/2014.\n";


/*!*******************************************************************
 * @def SESAME_OPEN_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Max number of LUNs for each raid group.
 *
 *********************************************************************/
#define SESAME_OPEN_TEST_LUNS_PER_RAID_GROUP 2

/*!*******************************************************************
 * @def SESAME_OPEN_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define SESAME_OPEN_CHUNKS_PER_LUN 15

/*!*******************************************************************
 * @def SESAME_OPEN_SMALL_IO_SIZE_MAX_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define SESAME_OPEN_SMALL_IO_SIZE_MAX_BLOCKS 1024
/*!*******************************************************************
 * @def SESAME_OPEN_LIGHT_THREAD_COUNT
 *********************************************************************
 * @brief Lightly loaded test to allow rekey to proceed.
 *
 *********************************************************************/
#define SESAME_OPEN_LIGHT_THREAD_COUNT 1

/*!*******************************************************************
 * @def SESAME_OPEN_MAX_IO_SIZE_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define SESAME_OPEN_MAX_IO_SIZE_BLOCKS (FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1) * FBE_RAID_MAX_BE_XFER_SIZE //4*0x400*6

/*!*******************************************************************
 * @var sesame_open_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t sesame_open_raid_group_config_qual[] = 
{
    /* width,   capacity  raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {2,       0xE000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            0,          0},
    {4,       0xE000,     FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER, 520,            1,          0},
    {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            2,          0},
    {6,       0xE000,     FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            3,          0},
    {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,   520,            4,          0},
    //{FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY,   520,  2,  FBE_TEST_RG_CONFIG_RANDOM_TAG},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

extern void katrina_get_rg_object_ids(fbe_test_rg_configuration_t *rg_config_p, fbe_object_id_t *rg_object_ids_p);
extern void diabolicdiscdemon_wait_for_proactive_spare_swap_in(fbe_object_id_t  vd_object_id,
                                                               fbe_edge_index_t spare_edge_index,
                                                               fbe_test_raid_group_disk_set_t * spare_location_p);

/*!**************************************************************
 * sesame_open_test_validate_rg_keys()
 ****************************************************************
 * @brief
 *  Verify keys from KMS for a raid group.
 *
 * @param object_id - object id for a RG.               
 * @param raid_group_id - raid group number.               
 *
 * @return None.   
 *
 * @author
 *  02/05/2014 - Created. Lili Chen
 *
 ****************************************************************/
void sesame_open_test_validate_rg_keys(fbe_object_id_t object_id, fbe_raid_group_number_t raid_group_id)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_kms_control_get_keys_t get_keys;
	fbe_database_control_get_drive_sn_t drive_sn;
    fbe_u32_t i, num_keys = 0;
    fbe_bool_t equal;

	/* Get drive serial numbers for the RG */
	fbe_zero_memory(&drive_sn, sizeof(fbe_database_control_get_drive_sn_t));
	drive_sn.rg_id = object_id;
	status = fbe_api_database_get_drive_sn_for_raid(&drive_sn);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Get keys from KMS */
    get_keys.object_id = object_id;
    get_keys.control_number = raid_group_id;
    status = fbe_test_sep_util_kms_get_rg_keys(&get_keys);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    /* Verify keys has the same serial number */
    for (i = 0; i <= FBE_RAID_MAX_DISK_ARRAY_WIDTH; i++)
    {
        equal = fbe_equal_memory(get_keys.drive[i].serial_num, drive_sn.serial_number[i], FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, equal, "Keys for RG not correct\n");
        if (drive_sn.serial_number[i][0] != 0) 
        {
            num_keys++;
        }
    }

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s RG 0x%x has %d keys\n", __FUNCTION__, object_id, num_keys);

    return;
}
/******************************************
 * end sesame_open_test_validate_rg_keys()
 ******************************************/

/*!**************************************************************
 * sesame_open_test_start_verify_keys_for_rg_config()
 ****************************************************************
 * @brief
 *  Verify keys for all raid groups.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  02/05/2014 - Created. Lili Chen
 *
 ****************************************************************/
void sesame_open_test_start_verify_keys_for_rg_config(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index = 0;
    fbe_object_id_t rg_object_id;

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            fbe_api_base_config_downstream_object_list_t downstream_object_list;
            fbe_u32_t i;
            
            fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);
            MUT_ASSERT_INT_NOT_EQUAL(downstream_object_list.number_of_downstream_objects, 0);
            for (i = 0; i < downstream_object_list.number_of_downstream_objects; i++)
			{
                sesame_open_test_validate_rg_keys(downstream_object_list.downstream_object_list[i], 
                                                  current_rg_config_p->raid_group_id);
			}
        }
        else
        {
            sesame_open_test_validate_rg_keys(rg_object_id, 
                                              current_rg_config_p->raid_group_id);
        }

        current_rg_config_p++;
    } 

    return;
}
/******************************************
 * end sesame_open_test_start_verify_keys_for_rg_config()
 ******************************************/

/*!**************************************************************
 * sesame_open_test_for_permanent_spare()
 ****************************************************************
 * @brief
 *  This function runs key validation before and after permanent spare.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  02/05/2014 - Created. Lili Chen
 *
 ****************************************************************/
void sesame_open_test_for_permanent_spare(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status = FBE_STATUS_OK;
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

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s starting ===", __FUNCTION__);

    /* Enable encryption now */
    // Add hook for encryption to complete and enable KMS
    mut_printf(MUT_LOG_TEST_STATUS, "== Add hooks for encryption completion and enable KMS started ==");
    status = fbe_test_sep_encryption_add_hook_and_enable_kms(rg_config_p, fbe_test_sep_util_get_dualsp_test_mode());
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== Add hooks for encryption completion and enable KMS completed; status: 0x%x. ==", status);
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    /* Verify Keys */
    sesame_open_test_start_verify_keys_for_rg_config(rg_config_p);

    katrina_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);
    fbe_test_get_pvd_object_ids(rg_config_p, &pvd_object_ids[0], 0 /* position */);
    fbe_test_get_vd_object_ids(rg_config_p, &vd_object_ids[0], 0);
    fbe_test_copy_determine_dest_index(&vd_object_ids[0], raid_group_count, &dest_index[0]);

    /* Start sparing */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index];
        pvd_object_id = pvd_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];

        /* Make sure our objects are in the ready state.*/
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(current_rg_config_p);

        /* Remove a drive */
        shaggy_remove_drive(current_rg_config_p,
                            0,
                            &current_rg_config_p->drive_handle[0]);

        current_rg_config_p++;
    }  

    /* Set timer */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(5);

    /* Wait spare to swap in */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index];
        pvd_object_id = pvd_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];

        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for sparing rg: 0x%x vd: 0x%x==", rg_object_id, vd_object_id);
        shaggy_test_wait_permanent_spare_swap_in(current_rg_config_p, 0);

        current_rg_config_p++;
    } 

    /* Verify Keys */
    sesame_open_test_start_verify_keys_for_rg_config(rg_config_p);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/******************************************
 * end sesame_open_test_for_permanent_spare()
 ******************************************/

/*!**************************************************************
 * sesame_open_test_start_encryption_when_rg_in_copy()
 ****************************************************************
 * @brief
 *  This function runs key validation before and after copy.
 *  Encryption enabled during copy.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  02/05/2014 - Created. Lili Chen
 *
 ****************************************************************/
void sesame_open_test_start_encryption_when_rg_in_copy(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status = FBE_STATUS_OK;
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
    fbe_provision_drive_copy_to_status_t copy_status;
    fbe_test_raid_group_disk_set_t spare_drive_location;

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s starting ===", __FUNCTION__);

    /* Start copy */
    katrina_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);
    fbe_test_get_pvd_object_ids(rg_config_p, &pvd_object_ids[0], 0 /* position */);
    fbe_test_get_vd_object_ids(rg_config_p, &vd_object_ids[0], 0);
    fbe_test_copy_determine_dest_index(&vd_object_ids[0], raid_group_count, &dest_index[0]);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index];
        pvd_object_id = pvd_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];

        /* Make sure our objects are in the ready state.*/
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(current_rg_config_p);


        mut_printf(MUT_LOG_TEST_STATUS, "%s: vd obj: 0x%x \n",
                   __FUNCTION__, vd_object_id);

        /* Set the debug hook
         */
        status = fbe_api_scheduler_add_debug_hook(vd_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                  0,
                                                  0,
                                                  SCHEDULER_CHECK_VALS_GT,
                                                  SCHEDULER_DEBUG_ACTION_PAUSE); 
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    }       

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0) ||
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index];
        pvd_object_id = pvd_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];

        /* Step  7: Start a user copy */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Start user copy rg: 0x%x dest pvd: 0x%x==", __FUNCTION__, rg_object_id, pvd_object_id);
        status = fbe_api_provision_drive_user_copy(pvd_object_id, &copy_status);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(FBE_PROVISION_DRIVE_COPY_TO_STATUS_NO_ERROR, copy_status);

        diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_index[rg_index], &spare_drive_location);

        status = fbe_test_wait_for_debug_hook(vd_object_id, 
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                              FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                              SCHEDULER_CHECK_VALS_GT, 
                                              SCHEDULER_DEBUG_ACTION_PAUSE, 
                                              0,
                                              0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    }  

    /* Enable encryption now */
    status = fbe_test_sep_encryption_add_hook_and_enable_kms(rg_config_p, fbe_test_sep_util_get_dualsp_test_mode());
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Verify Keys */
    sesame_open_test_start_verify_keys_for_rg_config(rg_config_p);

    /* Remove hooks */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0) ||
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index];
        pvd_object_id = pvd_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];

        /* Remove rebuild hooks */
        status = fbe_api_scheduler_del_debug_hook(vd_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                  0, 
                                                  0, 
                                                  SCHEDULER_CHECK_VALS_GT, 
                                                  SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for copy rg: 0x%x vd: 0x%x==", rg_object_id, vd_object_id);
        status = fbe_test_copy_wait_for_completion(vd_object_id, dest_index[rg_index]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    } 

    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);
    /* Verify Keys */
    sesame_open_test_start_verify_keys_for_rg_config(rg_config_p);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/******************************************
 * end sesame_open_test_start_encryption_when_rg_in_copy()
 ******************************************/

/*!**************************************************************
 * sesame_open_test_rg_config()
 ****************************************************************
 * @brief
 *  Run sesame key validation test.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  02/05/2014 - Created. Lili Chen
 *
 ****************************************************************/
void sesame_open_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_u32_t test_index = (fbe_random()%2);

    if (test_index == 0) {
        sesame_open_test_for_permanent_spare(rg_config_p);
    } else {
        sesame_open_test_start_encryption_when_rg_in_copy(rg_config_p);
    }

    /* Make sure no errors occurred. */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end sesame_open_test_rg_config()
 ******************************************/

/*!**************************************************************
 * sesame_open_test()
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
 *  02/05/2014 - Created. Lili Chen
 *
 ****************************************************************/
void sesame_open_test(void)
{
    fbe_test_create_raid_group_params_t params;
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    if (test_level > 0)
    {
        rg_config_p = &sesame_open_raid_group_config_qual[0]; //extended[0];
    }
    else
    {
        rg_config_p = &sesame_open_raid_group_config_qual[0];
    }

    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    //status = fbe_api_base_config_disable_all_bg_services();
    //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                         NULL, sesame_open_test_rg_config,
                                                         SESAME_OPEN_TEST_LUNS_PER_RAID_GROUP,
                                                         SESAME_OPEN_CHUNKS_PER_LUN,
                                                         FBE_FALSE /* don't save config destroy config */);
    params.b_encrypted = FBE_FALSE;
    fbe_test_run_test_on_rg_config_params(rg_config_p, &params);

    return;
}
/******************************************
 * end sesame_open_test()
 ******************************************/

/*!**************************************************************
 * sesame_open_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 5 I/O test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  02/05/2014 - Created. Lili Chen
 *
 ****************************************************************/
void sesame_open_setup(void)
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

        if (test_level > 0)
        {
            rg_config_p = &sesame_open_raid_group_config_qual[0]; //extended[0];
        }
        else
        {
            rg_config_p = &sesame_open_raid_group_config_qual[0];
        }

        /* We no longer create the raid groups during the setup
         */
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);

        /* initialize the number of extra drive required by each rg 
          */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);
        fbe_test_rg_populate_extra_drives(rg_config_p, 7);

        elmo_load_config(rg_config_p, 
                           SESAME_OPEN_TEST_LUNS_PER_RAID_GROUP, 
                           SESAME_OPEN_CHUNKS_PER_LUN);
    }

    /* Initialize any required fields 
     */
    fbe_test_common_util_test_setup_init();

    /*! @todo Currently we reduce the sector trace error level to critical
     *        to prevent dumping errored secoter tracing logs while running
     *	      the corrupt crc and corrupt data tests. We will remove this code 
     *        once fix the issue of unexpected errored sector tracing logs.
     */
    fbe_test_sep_util_reduce_sector_trace_level();

	sep_config_load_kms(NULL);
    return;
}
/**************************************
 * end sesame_open_setup()
 **************************************/

/*!**************************************************************
 * sesame_open_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  02/05/2014 - Created. Lili Chen
 *
 ****************************************************************/

void sesame_open_cleanup(void)
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
 * end sesame_open_cleanup()
 ******************************************/

/*************************
 * end file sesame_open_test.c
 *************************/
