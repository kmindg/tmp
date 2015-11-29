/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file beibei_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains a database service reboot test.
 *
 * @version
 *   5/17/2011 - Created. guov
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/ 
#include "generics.h"
#include "mut.h"   
#include "sep_tests.h"
#include "pp_utils.h"
#include "fbe_test_common_utils.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe_private_space_layout.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_utils.h"
#include "dba_export_types.h"
#include "fbe/fbe_api_block_transport_interface.h"
#include "fbe/fbe_api_persist_interface.h"


char * beibei_test_short_desc = "database service reboot test.";
char * beibei_test_long_desc ="\
The Beibei scenario tests topology is recreated after reboot.\n\
\n\
Dependencies:\n\
        - The Persist service.\n\
\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] viper enclosure\n\
        [PP] SAS drives and SATA drives\n\
        [PP] logical drive\n\
        [SEP] provision drive\n\
\n\
STEP 0: Setup - Bring up the package.\n\
        - Create and verify the initial physical package config.\n\
        - Verify provision drives are created and in ready state.\n\
STEP 1: Config a toplogy \n\
        - Create raid groups.\n\
        - Create LUNs to each raid group.\n\
        - verify the topology.\n\
STEP 2: Reboot SEP and verify the topolgy.\n\
STEP 3: Modiy the toplogy.\n\
        - Delete raid groups.\n\
        - verify the topology.\n\
STEP 4: Reboot SEP and verify the topolgy.\n\
STEP 5: Modiy the toplogy.\n\
        - Create raid groups.\n\
        - Create LUNs to each raid group.\n\
STEP 6: Reboot SEP and verify the topolgy.\n\
STEP 7: Verify existance of private space luns.\n\
STEP 8: Update LUN info (WWN).Reboot SEP and verify that the change is persisted.\n\
STEP 9: Verify private space luns are created after reboot.\n\n\
arguments:   -fbe_default_trace_level using trace level\n\n\
Description last updated: 10/3/2011.\n";

/* The number of LUNs in our raid group.
 * When we want to verify the fixing of AR 671574, set the LUNs per RG as 128. 
 * Then there will be more than 400 object entries created. 
 * In addition, we need to enlarge the maximum supported num in persist:
typedef enum fbe_persist_db_layout_sim_e{
    FBE_PERSIST_MAX_SEP_OBJECTS_SIM = 640, 
	FBE_PERSIST_MAX_SEP_EDGES_SIM = 10240,            <<<<< enlarge the edges as 10240
	FBE_PERSIST_MAX_SEP_ADMIN_CONVERSION_SIM = 640,   <<<<< enlarge the objects as 640
    ...
    }
 *
 * Or set the value as 2.
 * The fbe_sim doesn't have enough space to store too more objects in persist service.
 */
#define BEIBEI_TEST_LUNS_PER_RAID_GROUP 2 

/*!*******************************************************************
 * @def BEIBEI_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define BEIBEI_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def BEIBEI_MAX_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Max number of raid groups we will test with at a time.
 *
 *********************************************************************/
#define BEIBEI_MAX_RAID_GROUP_COUNT 6

/*!*******************************************************************
 * @def BEIBEI_RG_COUNT_STEP_1
 *********************************************************************
 * @brief number of raid groups to be created at STEP_1.
 *
 *********************************************************************/
#define BEIBEI_RG_COUNT_STEP_1 2

/*!*******************************************************************
 * @def BEIBEI_RG_COUNT_STEP_5
 *********************************************************************
 * @brief number of raid groups to be created at STEP_5.
 *
 *********************************************************************/
#define BEIBEI_RG_COUNT_STEP_5 3

/*!*******************************************************************
 * @def BEIBEI_RG_COUNT_STEP_11
 *********************************************************************
 * @brief number of raid groups to be created at STEP_11.
 *
 *********************************************************************/
#define BEIBEI_RG_COUNT_STEP_11 4


/*!*******************************************************************
 * @def BEIBEI_WAIT_MSEC
 *********************************************************************
 * @brief Number of seconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define BEIBEI_WAIT_MSEC 30000

/*!*******************************************************************
 * @var beibei_raid_groups
 *********************************************************************
 * @brief Test config.
 *
 *********************************************************************/
fbe_test_rg_configuration_t beibei_raid_groups[BEIBEI_MAX_RAID_GROUP_COUNT] = 
{
    /* width,   capacity    raid type,                  class,              block size, RAID-id.    bandwidth.*/
    {4,       0x4B000000,      FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            10,         0},
    {3,       0x4B000000,      FBE_RAID_GROUP_TYPE_RAID5,   FBE_CLASS_ID_PARITY,    520,            5,          0},
    {4,       0x4B000000,      FBE_RAID_GROUP_TYPE_RAID6,   FBE_CLASS_ID_PARITY,    520,            6,          0},
    {5,       0x4B000000,      FBE_RAID_GROUP_TYPE_RAID3,   FBE_CLASS_ID_PARITY,    520,            3,          0},
    {2,       0x4B000000,      FBE_RAID_GROUP_TYPE_RAID1,   FBE_CLASS_ID_MIRROR,    520,            1,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
};

typedef struct beibei_lun_user_config_data_s{
    /*! World-Wide Name associated with the LUN
     */
    fbe_assigned_wwid_t      world_wide_name; 
    /*! User-Defined Name for the LUN
     */
    fbe_user_defined_lun_name_t user_defined_name;
}beibei_lun_user_config_data_t;

/*!*******************************************************************
 * @def BEIBEI_TEST_LUN_USER_CONFIG_DATA
 *********************************************************************
 * @brief  Lun wwn used by this test
 *
 *********************************************************************/
static beibei_lun_user_config_data_t beibei_lun_user_config_data_set[BEIBEI_TEST_LUNS_PER_RAID_GROUP] = 
{      
    {{01,01,01,01,01,01,01,01,01,01,01,01,01,01,01,01},{0xD3,0xE4,0xF0,0x00}}, 
    {{02,02,02,02,02,02,02,02,02,02,02,02,02,02,02,02},{0xD3,0xE4,0xF0,0x00}}      
};

fbe_status_t beibei_send_zero_block_operation_to_lun(fbe_object_id_t lun_object_id, fbe_lba_t lba, fbe_block_count_t block_count);

/*!**************************************************************
 * beibei_load_physical_config()
 ****************************************************************
 * @brief
 *  Configure the test's physical configuration based on the
 *  raid groups we intend to create.
 *
 * @param config_p - Current config.
 *
 * @return None.
 *
 * @author
 *  8/23/2010 - Created. guov
 *
 ****************************************************************/
void beibei_load_physical_config(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t                   raid_group_count;
    const fbe_char_t *raid_type_string_p = NULL;
    fbe_u32_t num_520_drives = 0;
    fbe_u32_t num_4160_drives = 0;

    fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    /* First get the count of drives.
     */
    fbe_test_sep_util_get_rg_num_enclosures_and_drives(rg_config_p, raid_group_count,
                                                       &num_520_drives,
                                                       &num_4160_drives);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: %s %d 520 drives and %d 4160 drives", 
               __FUNCTION__, raid_type_string_p, num_520_drives, num_4160_drives);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Creating %d 520 drives and %d 4160 drives", 
               __FUNCTION__, num_520_drives, num_4160_drives);

    /* Add a configuration to each raid group config.
     */
    fbe_test_sep_util_rg_fill_disk_sets(rg_config_p, raid_group_count, 
                                        num_520_drives, num_4160_drives);
    fbe_test_pp_util_create_physical_config_for_disk_counts(num_520_drives,
                                                            num_4160_drives,
                                                            0x4B000000);  // 600 GB disk
    return;
}
/******************************************
 * end beibei_load_physical_config()
 ******************************************/

void beibei_step1_create_topology(void)
{
    /* Setup the lun capacity with the given number of chunks for each lun.
     */
    fbe_test_sep_util_fill_lun_configurations_rounded(beibei_raid_groups, 
                                                      BEIBEI_RG_COUNT_STEP_1,
                                                      BEIBEI_CHUNKS_PER_LUN, 
                                                      BEIBEI_TEST_LUNS_PER_RAID_GROUP);

    /* Kick of the creation of all the raid group with Logical unit configuration.
     */
    fbe_test_sep_util_create_raid_group_configuration(beibei_raid_groups, BEIBEI_RG_COUNT_STEP_1);
}

void beibei_step11_create_topology(void)
{
    /* Setup the lun capacity with the given number of chunks for each lun.
     */
    fbe_test_sep_util_fill_lun_configurations_rounded(beibei_raid_groups, 
                                                      BEIBEI_RG_COUNT_STEP_11,
                                                      BEIBEI_CHUNKS_PER_LUN, 
                                                      BEIBEI_TEST_LUNS_PER_RAID_GROUP);

    /* Kick of the creation of all the raid group with Logical unit configuration.
     */
    fbe_test_sep_util_create_raid_group_configuration(beibei_raid_groups, BEIBEI_RG_COUNT_STEP_11);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: last lun_number = %d",
               __FUNCTION__, beibei_raid_groups[BEIBEI_RG_COUNT_STEP_11 - 1].logical_unit_configuration_list[127].lun_number);

}

void beibei_destroy_all_topology(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_test_sep_util_destroy_all_user_luns();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_sep_util_destroy_all_user_raid_groups();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

void beibei_step3_destroy_topology(void)
{
    beibei_destroy_all_topology();
}

void beibei_step10_destroy_topology(void)
{
    beibei_destroy_all_topology();
}

void beibei_step5_create_topology(void)
{
    /* Setup the lun capacity with the given number of chunks for each lun.
     */
    fbe_test_sep_util_fill_lun_configurations_rounded(&beibei_raid_groups[BEIBEI_RG_COUNT_STEP_1], 
                                                      BEIBEI_RG_COUNT_STEP_5,
                                                      BEIBEI_CHUNKS_PER_LUN, 
                                                      BEIBEI_TEST_LUNS_PER_RAID_GROUP);

    /* Kick of the creation of all the raid group with Logical unit configuration.
     */
    fbe_test_sep_util_create_raid_group_configuration(&beibei_raid_groups[BEIBEI_RG_COUNT_STEP_1], BEIBEI_RG_COUNT_STEP_5);
}


void beibei_reboot_and_verify(fbe_u32_t expected_rg_count, fbe_u32_t expected_lun_count)
{
    fbe_status_t status;
    fbe_u32_t    object_count;

    /*reboot sep*/
    fbe_test_sep_util_destroy_neit_sep();
    sep_config_load_sep_and_neit();
    status = fbe_test_sep_util_wait_for_database_service(BEIBEI_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    /*verify the objects*/
    status = fbe_test_sep_util_get_user_lun_count(&object_count);
    MUT_ASSERT_INTEGER_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INTEGER_EQUAL(expected_lun_count, object_count); 
    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s: Found expected %d number of LUNs. \n", 
               __FUNCTION__,
               expected_lun_count);

    status = fbe_test_sep_util_get_all_user_rg_count(&object_count);
    MUT_ASSERT_INTEGER_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INTEGER_EQUAL(expected_rg_count, object_count); 
    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s: Found expected %d number of RGs. \n", 
               __FUNCTION__,
               expected_rg_count);
}
/*tests binding of private luns*/
static void beibei_test_get_private_space_luns(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_lun_number_t lun_number;
    fbe_object_id_t index;
    fbe_private_space_layout_lun_info_t lun_info;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Test binding of private luns ... ===\n");
    for (index = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_FIRST; index <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_LAST ; index++)
    {
        status = fbe_private_space_layout_get_lun_by_object_id(index, &lun_info);
        if(status == FBE_STATUS_OK) {
            status = fbe_api_database_lookup_lun_by_object_id(index,&lun_number);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
    }
}
/*Tests lun update functionality */
static void beibei_test_verify_update_lun(void)
{
    fbe_status_t status;
    fbe_object_id_t lun_object_id;
    fbe_database_lun_info_t lun_info;
    fbe_assigned_wwid_t world_wide_name;
    fbe_user_defined_lun_name_t user_defined_name;
    fbe_u32_t           attributes = LU_ATTR_DUAL_ASSIGNABLE;
    fbe_job_service_error_type_t    job_error_type;

    status = fbe_api_database_lookup_lun_by_number(0, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);   

    /*Update LUN*/
    mut_printf(MUT_LOG_TEST_STATUS, "=== Issue LUN update! ===\n");

    fbe_copy_memory(&world_wide_name, 
                    &beibei_lun_user_config_data_set[1].world_wide_name,
                    sizeof(world_wide_name));
    status = fbe_api_update_lun_wwn(lun_object_id, &world_wide_name, &job_error_type);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "=== LUN updated WWN successfully ===\n");

    fbe_copy_memory(&user_defined_name, 
                    &beibei_lun_user_config_data_set[1].user_defined_name,
                    sizeof(user_defined_name));
    status = fbe_api_update_lun_udn(lun_object_id, &user_defined_name, &job_error_type);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "=== LUN updated User defined name successfully ===\n");

    status = fbe_api_update_lun_attributes(lun_object_id, attributes, &job_error_type);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "=== LUN updated attributes successfully ===\n");

    /*reboot sep to check if updated wwn is persisted properly*/
    fbe_test_sep_util_destroy_neit_sep();
    sep_config_load_sep_and_neit();
    status = fbe_test_sep_util_wait_for_database_service(BEIBEI_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Verify LUN get to ready state in reasonable time before we get lun_info. */
    status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, 
                                                     FBE_LIFECYCLE_STATE_READY, 20000,
                                                     FBE_PACKAGE_ID_SEP_0);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    lun_info.lun_object_id = lun_object_id;
    status = fbe_api_database_get_lun_info(&lun_info);

    MUT_ASSERT_BUFFER_EQUAL_MSG((char *)&world_wide_name, 
                                (char *)&lun_info.world_wide_name, 
                                sizeof(world_wide_name),
                                "lun_get_info return mismatch WWN!");
    MUT_ASSERT_BUFFER_EQUAL_MSG((char *)&user_defined_name, 
                                (char *)&lun_info.user_defined_name, 
                                sizeof(user_defined_name),
                                "lun_get_info return mismatch User Defined Name!");
    MUT_ASSERT_TRUE_MSG((attributes & lun_info.attributes), 
                         "lun_get_info return mismatch attributes!");
}

void beibei_validate_database_and_reboot(void)
{
    fbe_status_t status;
    fbe_database_validate_request_type_t    request_type = FBE_DATABASE_VALIDATE_REQUEST_TYPE_INVALID;
    fbe_database_validate_failure_action_t  failure_action = FBE_DATABASE_VALIDATE_FAILURE_ACTION_NONE;
    fbe_database_state_t   state = FBE_DATABASE_STATE_READY;

    // start to validate database after reboot
    mut_printf(MUT_LOG_TEST_STATUS, "%s: start to validate the database", __FUNCTION__);
    request_type = FBE_DATABASE_VALIDATE_REQUEST_TYPE_USER;
    failure_action = FBE_DATABASE_VALIDATE_FAILURE_ACTION_ERROR_TRACE;
    status = fbe_api_database_validate_database(request_type, failure_action);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_database_get_state(&state);
    MUT_ASSERT_INT_EQUAL(state, FBE_DATABASE_STATE_READY);

    /*reboot sep. NOTE reboot function will verify if there is ERROR trace.
     * If there is, the mut_assert will be triggered. */
    fbe_test_sep_util_destroy_neit_sep();
    sep_config_load_sep_and_neit();
    status = fbe_test_sep_util_wait_for_database_service(BEIBEI_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_database_get_state(&state);
    MUT_ASSERT_INT_EQUAL(state, FBE_DATABASE_STATE_READY);
}


void beibei_step12_destroy_the_last_lun_but_one(void)
{
    fbe_status_t status;
    fbe_object_id_t lun_object_id;
    fbe_lun_number_t lun_number;
    fbe_api_lun_destroy_t lun_destroy_req;
    fbe_job_service_error_type_t      job_error_type = FBE_JOB_SERVICE_ERROR_INVALID;

    if ((BEIBEI_RG_COUNT_STEP_11 * BEIBEI_TEST_LUNS_PER_RAID_GROUP - 2) > 0)
    {
        lun_number = BEIBEI_RG_COUNT_STEP_11 * BEIBEI_TEST_LUNS_PER_RAID_GROUP - 2;
    } else {
        lun_number = 0;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "%s:The lun_number: %d is going to be delete",
               __FUNCTION__, lun_number);
    status = fbe_api_database_lookup_lun_by_number(lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);   

    lun_destroy_req.lun_number = lun_number;
    lun_destroy_req.allow_destroy_broken_lun = FBE_TRUE;
    status= fbe_api_destroy_lun(&lun_destroy_req, FBE_TRUE,LU_DESTROY_TIMEOUT, &job_error_type);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);   
    MUT_ASSERT_INT_EQUAL(FBE_JOB_SERVICE_ERROR_NO_ERROR, job_error_type);
}

/* Verify database and persist service code.
 * When there is all zeroed entry in the middle of the used entries,
 * make sure database and persist service code won't panic the system.
 * */
void beibei_test_verify_persist_service_after_zero_entry(void)
{
    fbe_lba_t   lba;
    fbe_block_count_t   block_count;
    fbe_status_t    status;
    fbe_persist_control_get_layout_info_t    get_info;
    fbe_database_state_t   state = FBE_DATABASE_STATE_READY;
    
    status = fbe_api_persist_get_layout_info(&get_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // set lba as the user entry address
    lba = get_info.sep_admin_conversion_start_lba;
    block_count = 5;
    beibei_send_zero_block_operation_to_lun(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MCR_DATABASE, lba, block_count);

    lba = get_info.sep_admin_conversion_start_lba + 5;
    block_count = 1;
    beibei_send_zero_block_operation_to_lun(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MCR_DATABASE, lba, block_count);
   
    /* Reboot sp */ 
    fbe_test_sep_util_destroy_neit_sep();
    sep_config_load_sep_and_neit();
    status = fbe_api_database_get_state(&state);
    
    status = fbe_test_sep_util_wait_for_database_service(BEIBEI_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_database_get_state(&state);
    MUT_ASSERT_INT_EQUAL(state, FBE_DATABASE_STATE_CORRUPT);

    /* Reset the error counter. The above steps will introduce Error logs intentionally*/
    fbe_test_sep_util_sep_neit_physical_reset_error_counters();
}

fbe_status_t beibei_send_zero_block_operation_to_lun(fbe_object_id_t lun_object_id, fbe_lba_t lba, fbe_block_count_t block_count)
{
    fbe_payload_block_operation_t   block_operation = {0};
    fbe_payload_block_operation_opcode_t rdt_block_op = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID; 
    fbe_block_size_t    block_size = FBE_BE_BYTES_PER_BLOCK;  // raid always reports 520 bytes per block
    fbe_block_size_t    optimal_block_size;
    fbe_block_transport_block_operation_info_t  block_operation_info = {0};
    fbe_sg_element_t  *sg_ptr = NULL;
    
    fbe_object_id_t   object_id_for_block_op = FBE_OBJECT_ID_INVALID;
    fbe_package_id_t  package_id_for_block_op = FBE_PACKAGE_ID_SEP_0;
    fbe_database_lun_info_t lun_info;
    fbe_status_t    status;
    
    object_id_for_block_op = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MCR_DATABASE;
    lun_info.lun_object_id = lun_object_id;
    status = fbe_api_database_get_lun_info(&lun_info);
    if (status != FBE_STATUS_OK)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "ERROR: Lun obj_id: %d details are not available\n ",
                       object_id_for_block_op);
        return status;
    }

    optimal_block_size = lun_info.raid_info.optimal_block_size;
    rdt_block_op = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO;

    status = fbe_payload_block_build_operation(&block_operation,
                                               rdt_block_op,
                                               lba,
                                               block_count,
                                               block_size,
                                               optimal_block_size,
                                               NULL);
    
    block_operation_info.block_operation = block_operation;
    status = fbe_api_block_transport_send_block_operation(
                                                          object_id_for_block_op,
                                                          package_id_for_block_op,
                                                          &block_operation_info,
                                                          (fbe_sg_element_t*)sg_ptr);
    
    if (status != FBE_STATUS_OK)
    {
        mut_printf(MUT_LOG_TEST_STATUS,"ERROR: send block operation to %d failed. status = %d\n ",
                       object_id_for_block_op, status);
        return status;
    }

    return status;
}


/*!**************************************************************
 * beibei_simulation_setup()
 ****************************************************************
 * @brief
 *  Setup the simulation configuration.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  3/24/2011 - Created. guov
 *
 ****************************************************************/
void beibei_simulation_setup(fbe_test_rg_configuration_t *raid_group_config_p)
{
    /* Only load the physical config in simulation.
     */
    if (!fbe_test_util_is_simulation())
    {
        MUT_FAIL_MSG("Should only be called in simulation mode");
        return;
    }
    fbe_test_sep_util_init_rg_configuration_array(raid_group_config_p);
    beibei_load_physical_config(raid_group_config_p);
    sep_config_load_sep_and_neit();
    return;
}
/**************************************
 * end beibei_simulation_setup()
 **************************************/
/*********************************************************************************
 * BEGIN Test, Setup, Cleanup.
 *********************************************************************************/
/*!**************************************************************
 * beibei_test()
 ****************************************************************
 * @brief
 *  Run error injection test to several LUNs.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. guov
 *
 ****************************************************************/
void beibei_test(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "Step 1: Create the topology");
    beibei_step1_create_topology();
    mut_printf(MUT_LOG_TEST_STATUS, "Step 2: reboot and verify the topology");
    beibei_reboot_and_verify(BEIBEI_RG_COUNT_STEP_1, BEIBEI_RG_COUNT_STEP_1*BEIBEI_TEST_LUNS_PER_RAID_GROUP);
    mut_printf(MUT_LOG_TEST_STATUS, "Step 3: destroy the topology");
    beibei_step3_destroy_topology();
    mut_printf(MUT_LOG_TEST_STATUS, "Step 4: reboot and verify if the topology is empty");
    beibei_reboot_and_verify(0, 0);
    mut_printf(MUT_LOG_TEST_STATUS, "Step 5: create the topology for more tests");
    beibei_step5_create_topology();
    mut_printf(MUT_LOG_TEST_STATUS, "Step 6: reboot and verify the topology");
    beibei_reboot_and_verify(BEIBEI_RG_COUNT_STEP_5, BEIBEI_RG_COUNT_STEP_5*BEIBEI_TEST_LUNS_PER_RAID_GROUP);
    mut_printf(MUT_LOG_TEST_STATUS, "Step 7: reboot and verify the topology again");
    beibei_reboot_and_verify(BEIBEI_RG_COUNT_STEP_5, BEIBEI_RG_COUNT_STEP_5*BEIBEI_TEST_LUNS_PER_RAID_GROUP);
    mut_printf(MUT_LOG_TEST_STATUS, "Step 8: verify the private space LUNs");
    beibei_test_get_private_space_luns();
    mut_printf(MUT_LOG_TEST_STATUS, "Step 9: verify the LUN updating operation");
    beibei_test_verify_update_lun();

    mut_printf(MUT_LOG_TEST_STATUS, "Step 10: destroy the topology");
    beibei_step10_destroy_topology();
    mut_printf(MUT_LOG_TEST_STATUS, "Step 11: create the topology");
    beibei_step11_create_topology();
    mut_printf(MUT_LOG_TEST_STATUS, "Step 12: delete the last lun but one");
    beibei_step12_destroy_the_last_lun_but_one();
    mut_printf(MUT_LOG_TEST_STATUS, "Step 13: validate the database and reboot sp");
    beibei_validate_database_and_reboot();

    mut_printf(MUT_LOG_TEST_STATUS, "Step 14: verify persist service after zero the entry");
    beibei_test_verify_persist_service_after_zero_entry();

    return;
}
/******************************************
 * end beibei_test()
 ******************************************/
/*!**************************************************************
 * beibei_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 5 I/O test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  9/1/2009 - Created. guov
 *
 ****************************************************************/
void beibei_setup(void)
{
    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        beibei_simulation_setup(&beibei_raid_groups[0]);
    }
    return;
}
/**************************************
 * end beibei_setup()
 **************************************/

/*!**************************************************************
 * beibei_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the beibei test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. guov
 *
 ****************************************************************/

void beibei_cleanup(void)
{
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end beibei_cleanup()
 ******************************************/

/*************************
 * end file beibei_test.c
 *************************/



