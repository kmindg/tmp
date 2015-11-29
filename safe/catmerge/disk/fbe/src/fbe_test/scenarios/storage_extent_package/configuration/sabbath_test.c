/*********************************************************************************************
 * Copyright (C) EMC Corporation 2011-2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 *********************************************************************************************/

/*!*******************************************************************************************
 * @file sabbath_test.c
 *********************************************************************************************
 *
 * @brief
 *  
 *
 * @version
 *  12/08/2011 - Created. Jian Gao
 *  07/24/2012 - Modified to dualsp test, change some expected system behavior and add new tests. Zhipeng Hu
 *********************************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "sep_tests.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe_private_space_layout.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_system_interface.h"
#include "sep_utils.h"
#include "fbe_database.h"
#include "pp_utils.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe/fbe_event_log_utils.h"                    //  for message codes
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_trace_interface.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_esp_board_mgmt_interface.h"
#include "fbe/fbe_api_esp_common_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_api_persist_interface.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe/fbe_api_system_time_threshold_interface.h"
#include "sep_hook.h"
#include "sep_test_io.h"

extern void fbe_test_sep_util_set_object_trace_flags(fbe_trace_type_t trace_type, 
                                                     fbe_object_id_t object_id, 
                                                     fbe_package_id_t package_id,
                                                     fbe_trace_level_t level);

/*-----------------------------------------------------------------------------
    DEFINITIONS:
*/

/*!*******************************************************************
 * @def SABBATH_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects to create
 *        (1 board + 1 port + 4 encl + 60 pdo) 
 *
 *********************************************************************/
#define SABBATH_TEST_NUMBER_OF_PHYSICAL_OBJECTS 66
/*#define SABBATH_TEST_NUMBER_OF_PHYSICAL_OBJECTS 33*/

#define SABBATH_TEST_REMOVED_DB_DRIVE_BUS_0 0
#define SABBATH_TEST_REMOVED_DB_DRIVE_ENCL_0 0
#define SABBATH_TEST_REMOVED_DB_DRIVE_SLOT_0 0
#define SABBATH_TEST_REMOVED_DB_DRIVE_SLOT_1 1
#define SABBATH_TEST_REMOVED_DB_DRIVE_SLOT_2 2
#define SABBATH_DB_STATE_RETRY_MAX_COUNT        5
#define SABBATH_DB_STATE_RETRY_INTERNAL_SECOND  1

/*!*******************************************************************
 * @def SABBATH_TEST_DRIVE_CAPACITY
 *********************************************************************
 * @brief Number of blocks in the physical drive
 *
 *********************************************************************/
#define SABBATH_TEST_DRIVE_CAPACITY ((fbe_lba_t)(1.5*TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY))


/************************************************************************************
 * Components to support Table driven test methodology.
 * Following this approach, the parameters for the test will be stored
 * as rows in a table.
 * The test will then be executed for each row(i.e. set of parameters)
 * in the table.
 ***********************************************************************************/

/* The different test numbers.
 */
typedef enum sabbath_test_enclosure_num_e
{
    SABBATH_TEST_ENCL1_WITH_SAS_DRIVES = 0,
    SABBATH_TEST_ENCL2_WITH_SAS_DRIVES,
    SABBATH_TEST_ENCL3_WITH_SAS_DRIVES,
    SABBATH_TEST_ENCL4_WITH_SAS_DRIVES
} sabbath_test_enclosure_num_t;

/* The different drive types.
 */
typedef enum sabbath_test_drive_type_e
{
    SAS_DRIVE
} sabbath_test_drive_type_t;

/* This is a set of parameters for the Scoobert spare drive selection tests.
 */
typedef struct enclosure_drive_details_s
{
    sabbath_test_drive_type_t drive_type; /* Type of the drives to be inserted in this enclosure */
    fbe_u32_t port_number;                    /* The port on which current configuration exists */
    sabbath_test_enclosure_num_t  encl_number;      /* The unique enclosure number */
    fbe_block_size_t block_size;
} enclosure_drive_details_t;

/* Table containing actual enclosure and drive details to be inserted.
 */
static enclosure_drive_details_t encl_table[] = 
{
    {   SAS_DRIVE,
        0,
        SABBATH_TEST_ENCL1_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        SABBATH_TEST_ENCL2_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        SABBATH_TEST_ENCL3_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        SABBATH_TEST_ENCL4_WITH_SAS_DRIVES,
        520
    },
};
static fbe_api_job_service_raid_group_create_t raid_group_create_cmd = 
{
    10,
    3,
    FBE_LUN_DEFAULT_POWER_SAVE_IDLE_TIME,
    8000,
    FBE_RAID_GROUP_TYPE_RAID5,
    FBE_TRI_FALSE,
    FBE_TRUE,
    5000,
    0,
    {
            {0, 0, 5},
            {0, 0, 6},
            {0, 0, 8},
            {0, 0, 10}
    },
    FBE_FALSE,
    FBE_FALSE,
    30000, 
    FBE_FALSE,
    FBE_OBJECT_ID_INVALID
};

static fbe_api_job_service_raid_group_create_t raid_group_create_on_system_drive_cmd = 
{
    12,
    4,
    FBE_LUN_DEFAULT_POWER_SAVE_IDLE_TIME,
    8000,
    FBE_RAID_GROUP_TYPE_RAID5,
    FBE_TRI_FALSE,
    FBE_FALSE,
    0x9000,
    0,
    {
            {0, 0, 0},
            {0, 0, 1},
            {0, 0, 2},
            {0, 0, 3}
    },
    FBE_FALSE,
    FBE_TRUE,
    30000, 
    FBE_FALSE,
    FBE_OBJECT_ID_INVALID
};



/* Count of rows in the table.
 */
#define SABBATH_TEST_ENCL_MAX sizeof(encl_table)/sizeof(encl_table[0])

#define SABBATH_TEST_WAIT_MSEC                  30000
#define SABBATH_TEST_WAIT_SEC                   30
#define SABBATH_TEST_WAIT_OBJ_READY             90
#define SABBATH_TEST_SLEEP_MESC                 10000
char * sabbath_test_short_desc = "Database Homewrecker test";
char * sabbath_test_long_desc =
    "Database Homewrecker test\n"
    "    - Homewrecker is a method to handle db drives moved around\n"
    "    - and the chassis replacement(planned or unplanned)\n"
    "    - As the phase 1 Homewrecker implementation, it makes a\n"
    "    - simple policy to handle situations. If any db drives' wwn_seed\n"
    "    - not match the system's and no planned movement flag found\n"
    "    - SEP enter maintenance mode. Or if any db drives' wwn_seed do\n"
    "    - not match to other db drives', SEP enter maintenance mode also.\n"
    ;

char * broken_Rose_test_short_desc = "Nonpaged Metadata Initialization test";
char * broken_Rose_test_long_desc = 
    "Nonpaged Metadata Initialization test\n"
    "   - Test against AR601723.\n"
    "   - One PVD gets destroyed, then another PVD gets created.\n"
    "   - Then Passive SP reboot, then Active SP reboot.\n"
    "   - Above steps would lead in-memory NP of newly created PVD\n"
    "   - gets overwritten by on-disk NP(which is out of date)\n"
    "   - Test against AR537044.\n"
    "   - To ensure no regression involved by future check-in\n"
    ;

char * dark_archon_test_short_desc = "PVD pull and reinsert test";
char * dark_archon_test_long_desc =
    "PVD pull and reinsert test\n"
    "   - Test against AR642445.\n"
    "   1. Drive removed and PVDs on both sides go into fail rotary\n"
    "   2. Passive side reboot.\n"
    "   3. reinsert the drive in another slot.\n"
    "   4. Passive tries to join with active, but active is still in fail rotary\n"
    "   5. Passive will goto fail rotary after join.\n"
    "   6. Verify both PVDs can goto READY rotary.\n"
    "Test for system pvd reinit while setting drive fault\n"
    "   - Test against AR700491.\n"
    "   1. Remove drive 0_0_3 on acitve sp\n"
    "   2. Set drive fault on 0_0_3 on passive sp\n"
    "   3. remove drive 0_0_3 on passive sp\n"
    "   4. Insert a new drive on 0_0_3\n"
    "   5. Wait this PVD on both SP to be ready\n"
    ;

char * zealot_test_short_desc = "PVD pull and reinsert test on creating";
char * zealot_test_long_desc =
    "PVD pull and reinsert test on creating\n"
    "   - Test against AR656681.\n"
    "   1. Drive removed on active side when creating\n"
    "   2. PVD on passive side enter ready rotary.\n"
    "   3. reinsert the drive on active side.\n"
    "   4. Verify PVD has valid B_E_S information.\n"
    ;

static void sabbath_test_inject_error_record(
									fbe_object_id_t							raid_group_object_id,
                                    fbe_u16_t                               position_bitmap,
                                    fbe_lba_t                               lba,
                                    fbe_block_count_t                       num_blocks,
                                    fbe_api_logical_error_injection_type_t  error_type,
                                    fbe_u16_t                               err_adj_bitmask,
                                    fbe_u32_t								opcode);

static void sabbath_test_disable_error_injection(fbe_object_id_t	raid_group_object_id,
                                                 fbe_u16_t       position_bitmap);


/*!**************************************************************
 * sabbath_test_load_physical_config()
 ****************************************************************
 * @brief
 *  
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  12/08/2011 - Created. Jian Gao
 ****************************************************************/
static void sabbath_test_load_physical_config(void)
{
    fbe_status_t                            status;
    fbe_u32_t                               object_handle;
    fbe_u32_t                               port_idx;
    fbe_u32_t                               enclosure_idx;
    fbe_u32_t                               drive_idx;
    fbe_u32_t                               total_objects;
    fbe_class_id_t                          class_id;
    fbe_api_terminator_device_handle_t      port0_handle;
    fbe_api_terminator_device_handle_t      port0_encl_handle[SABBATH_TEST_ENCL_MAX];
    fbe_api_terminator_device_handle_t      drive_handle;
    fbe_u32_t                               slot;
    fbe_u32_t                               enclosure;

    /* Initialize local variables */
    status          = FBE_STATUS_GENERIC_FAILURE;
    object_handle   = 0;
    port_idx        = 0;
    enclosure_idx   = 0;
    drive_idx       = 0;
    total_objects   = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== configuring terminator ===\n");
    /* before loading the physical package we initialize the terminator */
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Insert a board
     */
    status = fbe_test_pp_util_insert_armada_board();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Insert a port
     */
    fbe_test_pp_util_insert_sas_pmc_port(   1, /* io port */
                                            2, /* portal */
                                            0, /* backend number */ 
                                            &port0_handle);

    /* Insert enclosures to port 0
     */
    for ( enclosure = 0; enclosure < SABBATH_TEST_ENCL_MAX; enclosure++)
    { 
        fbe_test_pp_util_insert_viper_enclosure(0,
                                                enclosure,
                                                port0_handle,
                                                &port0_encl_handle[enclosure]);
    }

    /* Insert drives for enclosures.
     */
    for ( enclosure = 0; enclosure < SABBATH_TEST_ENCL_MAX; enclosure++)
    {
        for ( slot = 0; slot < 15; slot++)
        {
            if(SAS_DRIVE == encl_table[enclosure].drive_type)
            {
                fbe_test_pp_util_insert_sas_drive(encl_table[enclosure].port_number,
                                                encl_table[enclosure].encl_number,
                                                slot,
                                                encl_table[enclosure].block_size,
                                                SABBATH_TEST_DRIVE_CAPACITY,
                                                &drive_handle);
            }
        }
    }

    mut_printf(MUT_LOG_TEST_STATUS, "=== starting physical package===\n");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "=== set physical package entries ===\n");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Init fbe api on server */
    mut_printf(MUT_LOG_TEST_STATUS, "=== starting Server FBE_API ===\n");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(SABBATH_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 20000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== verifying configuration ===\n");

    /* Inserted a armada board so we check for it;
     * board is always assumed to be object id 0
     */
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying board type....Passed");

    /* Make sure we have the expected number of objects.
     */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == SABBATH_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");

    return;

}

/*!**************************************************************
 * sabbath_hot_spare_control()
 ****************************************************************
 * @brief   This method sets up the system to enable or disbale the hot spare feature
 *
 * @param   enable - if FBE_TRUE is passed, the system will automatically swap in HS based no policy
 *                   if FBE_FALSE is passed, the system will need drives to be marked as hot spare to swap them
 *
 * @return fbe_status_t.   
 *
 * @author
 *  05/31/2013 - Created. Wei He
 ****************************************************************/
static fbe_status_t sabbath_hot_spare_control(fbe_bool_t enable)
{
    fbe_status_t        status;
    fbe_u32_t           virtual_drive_count;
    fbe_object_id_t*    object_array = NULL;

    fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s: Need to turn AHS %s\n", __FUNCTION__, (enable == FBE_TRUE) ? "On" : "Off" );

    /*let's tell the topology service what to do based on the flag*/
    status  = fbe_api_provision_drive_set_config_hs_flag(!enable);
    if (status != FBE_STATUS_OK) {
         mut_printf(MUT_LOG_TEST_STATUS,"%s, failed to change toplogy HS selection flag\n", __FUNCTION__);
         return status;
    }

    /*now, let's turn off all future VDs that will be created by controlling the class level*/
    status = fbe_api_virtual_drive_class_set_unused_as_spare_flag(enable);
    if (status != FBE_STATUS_OK) {
         mut_printf(MUT_LOG_TEST_STATUS, "%s, failed to change VD class level HS selection flag\n", __FUNCTION__);
         return status;
    }

    /*and now let's do all the existing VD, we have to call them explicitly*/
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_VIRTUAL_DRIVE, FBE_PACKAGE_ID_SEP_0, &virtual_drive_count );
    if (status != FBE_STATUS_OK) {
        mut_printf(MUT_LOG_TEST_STATUS, "%s, failed to get total VDs in the system\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (virtual_drive_count)
    {
        status = fbe_api_enumerate_class(FBE_CLASS_ID_VIRTUAL_DRIVE, FBE_PACKAGE_ID_SEP_0, &object_array, &virtual_drive_count);
        if (status != FBE_STATUS_OK) {
            mut_printf(MUT_LOG_TEST_STATUS,"%s, failed to get a list of VDs\n", __FUNCTION__);
            if(object_array != NULL)
            {
                fbe_api_free_memory(object_array);
            }
            return FBE_STATUS_GENERIC_FAILURE;
        }
    
        for (;virtual_drive_count > 0; virtual_drive_count--) {
            status = fbe_api_virtual_drive_set_unused_drive_as_spare_flag(object_array[virtual_drive_count - 1], enable);
            if (status != FBE_STATUS_OK) {
                mut_printf(MUT_LOG_TEST_STATUS, "%s, Try set VD 0x%X again\n", 
                           __FUNCTION__, object_array[virtual_drive_count - 1]);
                status = fbe_api_wait_for_object_lifecycle_state(object_array[virtual_drive_count - 1], 
                                                        FBE_LIFECYCLE_STATE_READY, 3000, FBE_PACKAGE_ID_SEP_0);
                status = fbe_api_virtual_drive_set_unused_drive_as_spare_flag(object_array[virtual_drive_count - 1], enable);
                mut_printf(MUT_LOG_TEST_STATUS, "%s, failed to set vd ID 0x%X unused_drive_as_spare to:%s\n", 
                          __FUNCTION__, object_array[virtual_drive_count - 1], enable ? "True" : "False");
            }
        }

        fbe_api_free_memory(object_array);
    }
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * sabbath_disable_background_service()
 ****************************************************************
 * @brief
 *  disable backgroud service to stop huge memory consumption
 *
 * @return status.   
 *
 * @author
 *  Created. Wei He
 ****************************************************************/
static fbe_status_t sabbath_disable_background_service(void)
{
    
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t       count = 0;
    do{
        status = fbe_api_base_config_disable_all_bg_services();

        if (status != FBE_STATUS_OK)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "retry disable background service\n");
            fbe_api_sleep(SABBATH_DB_STATE_RETRY_INTERNAL_SECOND);
        }
        
        count++;
    }while(count < SABBATH_DB_STATE_RETRY_MAX_COUNT && status != FBE_STATUS_OK);

    return status;
}

/*!**************************************************************
 * sabbath_check_database_states()
 ****************************************************************
 * @brief
 *  Check whether the current database state is matched with the expected one.
 *  Check on both SPs
 *
 * @param SPA_expected_state - expected db state on SPA.               
 * @param SPB_expected_state - expected db state on SPB.
 *
 * @return None.   
 *
 * @author
 *  07/24/2012 - Created. Zhipeng Hu
 ****************************************************************/
static void sabbath_check_database_states(fbe_database_state_t  SPA_expected_state, fbe_database_state_t SPB_expected_state)
{
    fbe_database_state_t    state;
    fbe_status_t    status;
    fbe_sim_transport_connection_target_t   original_target;
    
    original_target = fbe_api_sim_transport_get_target_server();

    fbe_api_sleep(5000);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_database_get_state(&state);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK,status,"sabbath_test: fail to get db state of SPA");
    MUT_ASSERT_INT_EQUAL_MSG(SPA_expected_state,state,"sabbath_test: SPA's state is not expected one");
    
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_database_get_state(&state);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK,status,"sabbath_test: fail to get db state of SPB");
    MUT_ASSERT_INT_EQUAL_MSG(SPB_expected_state,state,"sabbath_test: SPB's state is not expected one");

    fbe_api_sim_transport_set_target_server(original_target);
}

/*!**************************************************************
 * sabbath_shutdown_systems()
 ****************************************************************
 * @brief
 *  Shutdown the array.
 *
 * @param shutdown_spa - whether to shutdown SPA.
 * @param shutdown_spb - whether to shutdown SPB.
 *
 * @return None.   
 *
 * @author
 *  07/24/2012 - Created. Zhipeng Hu
 ****************************************************************/
static void sabbath_shutdown_systems(fbe_bool_t shutdown_spa, fbe_bool_t shutdown_spb)
{
    fbe_sim_transport_connection_target_t   original_target;
    
    original_target = fbe_api_sim_transport_get_target_server();

    if(FBE_TRUE == shutdown_spb)
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
        fbe_test_sep_util_destroy_esp_neit_sep();
    }

    if(FBE_TRUE == shutdown_spa)
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
        fbe_test_sep_util_destroy_esp_neit_sep();
    }

    fbe_api_sim_transport_set_target_server(original_target);
}
/*!**************************************************************
 * sabbath_poweron_systems()
 ****************************************************************
 * @brief
 *  Poweron the array.
 *
 * @param shutdown_spa - whether to poweron SPA.
 * @param shutdown_spb - whether to poweron SPB.
 *
 * @return None.   
 *
 * @author
 *  07/24/2012 - Created. Zhipeng Hu
 ****************************************************************/
static void sabbath_poweron_systems(fbe_bool_t poweron_spa, fbe_bool_t poweron_spb)
{
    fbe_sim_transport_connection_target_t   original_target;
    
    original_target = fbe_api_sim_transport_get_target_server();

    if(FBE_TRUE == poweron_spa)
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_esp_sep_and_neit();
        sabbath_hot_spare_control(FBE_FALSE);  /*disable automatic hotspare activity*/
    }

    if(FBE_TRUE == poweron_spb)
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        sep_config_load_esp_sep_and_neit();
        sabbath_hot_spare_control(FBE_FALSE);  /*disable automatic hotspare activity*/
    }


    fbe_api_sim_transport_set_target_server(original_target);
}

static void sabbath_poweron_systems_with_bootflash_mode(fbe_bool_t poweron_spa, fbe_bool_t poweron_spb)
{
    fbe_sim_transport_connection_target_t   original_target;
	fbe_sep_package_load_params_t sep_params;
	fbe_neit_package_load_params_t neit_params;

	sep_config_fill_load_params(&sep_params);
	neit_config_fill_load_params(&neit_params);
	sep_params.flags |= FBE_SEP_LOAD_FLAG_BOOTFLASH_MODE;

    original_target = fbe_api_sim_transport_get_target_server();

    if(FBE_TRUE == poweron_spa)
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_esp_sep_and_neit_params(&sep_params, &neit_params);
        sabbath_hot_spare_control(FBE_FALSE);  /*disable automatic hotspare activity*/
    }

    if(FBE_TRUE == poweron_spb)
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        sep_config_load_esp_sep_and_neit_params(&sep_params, &neit_params);
        sabbath_hot_spare_control(FBE_FALSE);  /*disable automatic hotspare activity*/
    }


    fbe_api_sim_transport_set_target_server(original_target);
}

/*!**************************************************************
 * sabbath_poweron_systems_disable_backgroud_serivce()
 ****************************************************************
 * @brief
 *  1. Poweron the array
 *  2. Check the db state. If it is ready, disable background service ASAP
 *
 * @param shutdown_spa - whether to poweron SPA.
 * @param shutdown_spb - whether to poweron SPB.
 *
 * @return None.   
 *
 * @author
 *  12/15/2012 - He, Wei
 ****************************************************************/
static void sabbath_poweron_systems_disable_backgroud_serivce(fbe_bool_t poweron_spa, fbe_bool_t poweron_spb)
{
    fbe_sim_transport_connection_target_t   original_target;
    
    fbe_database_state_t    spa_state;
    fbe_database_state_t    spb_state;
    fbe_status_t            spa_status;
    fbe_status_t            spb_status;
    fbe_bool_t              spa_ready = FBE_FALSE;
    fbe_bool_t              spb_ready = FBE_FALSE;
    fbe_u32_t               retry_count = 0;
    
    original_target = fbe_api_sim_transport_get_target_server();

    if(FBE_TRUE == poweron_spa)
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_esp_sep_and_neit();
        sabbath_hot_spare_control(FBE_FALSE);  /*disable automatic hotspare activity*/
        retry_count = 0;
        do{
            mut_printf(MUT_LOG_TEST_STATUS, "Get DB state ,%d\n", retry_count);
            spa_status = fbe_api_database_get_state(&spa_state);
            if (spa_status == FBE_STATUS_OK &&
                FBE_DATABASE_STATE_READY== spa_state)
            {
                spa_ready = FBE_TRUE;
                
                spa_status = sabbath_disable_background_service();
                MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK,spa_status,"sabbath_test: fail to disable backgroud service in SPA");
                
                mut_printf(MUT_LOG_TEST_STATUS, "Disable SPA backgroud service successfully\n");
            }
            retry_count++;        
        
        }while(retry_count < SABBATH_DB_STATE_RETRY_MAX_COUNT && 
               spa_ready == FBE_FALSE);
    }

    if(FBE_TRUE == poweron_spb)
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        sep_config_load_esp_sep_and_neit();
        sabbath_hot_spare_control(FBE_FALSE);  /*disable automatic hotspare activity*/
        retry_count = 0;
        do{
            mut_printf(MUT_LOG_TEST_STATUS, "Get DB state ,%d\n", retry_count);
            spb_status = fbe_api_database_get_state(&spb_state);
            if (spb_status == FBE_STATUS_OK &&
                FBE_DATABASE_STATE_READY== spb_state)
            {
                spb_ready = FBE_TRUE;
                
                spb_status = sabbath_disable_background_service();
                MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK,spb_status,"sabbath_test: fail to disable backgroud service in SPB");
                
                mut_printf(MUT_LOG_TEST_STATUS, "Disable SPB backgroud service successfully\n");
            }
            retry_count++;        
        
        }while(retry_count < SABBATH_DB_STATE_RETRY_MAX_COUNT && 
               spb_ready == FBE_FALSE);
    }


    fbe_api_sim_transport_set_target_server(original_target);
}

/*!**************************************************************
 * sabbath_pull_drive()
 ****************************************************************
 * @brief
 *  Pull a specified drive from array
 *
 * @param port_number - drive location.
 * @param enclosure_number - drive location.
 * @param slot_number - drive location.
 * @param drive_handle_p_spa - the handle of the drive on SPA
 * @param drive_handle_p_spb - the handle of the drive on SPB
 *
 * @return None.   
 *
 * @author
 *  07/25/2012 - Created. Zhipeng Hu
 ****************************************************************/
void sabbath_pull_drive(fbe_u32_t port_number, 
                                         fbe_u32_t enclosure_number, 
                                         fbe_u32_t slot_number,
                                         fbe_api_terminator_device_handle_t *drive_handle_p_spa,
                                         fbe_api_terminator_device_handle_t *drive_handle_p_spb)
{
    fbe_status_t    status;
    fbe_sim_transport_connection_target_t   original_target;
    
    original_target = fbe_api_sim_transport_get_target_server();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_test_pp_util_pull_drive(port_number, enclosure_number, slot_number, drive_handle_p_spa);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_pull_drive: fail to pull drive from SPA");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_test_pp_util_pull_drive(port_number, enclosure_number, slot_number, drive_handle_p_spb);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_pull_drive: fail to pull drive from SPB");

    fbe_api_sim_transport_set_target_server(original_target);
}

/*!**************************************************************
 * sabbath_reinsert_drive()
 ****************************************************************
 * @brief
 *  Reinsert a specified drive into array
 *
 * @param port_number - drive location where to insert.
 * @param enclosure_number - drive location where to insert.
 * @param slot_number - drive location where to insert.
 * @param drive_handle_p_spa - the handle of the drive on SPA
 * @param drive_handle_p_spb - the handle of the drive on SPB
 *
 * @return None.   
 *
 * @author
 *  07/25/2012 - Created. Zhipeng Hu
 ****************************************************************/
void sabbath_reinsert_drive(fbe_u32_t port_number, 
                                         fbe_u32_t enclosure_number, 
                                         fbe_u32_t slot_number,
                                         fbe_api_terminator_device_handle_t drive_handle_p_spa,
                                         fbe_api_terminator_device_handle_t drive_handle_p_spb)
{
    fbe_status_t    status;
    fbe_sim_transport_connection_target_t   original_target;
    
    original_target = fbe_api_sim_transport_get_target_server();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_test_pp_util_reinsert_drive(port_number, enclosure_number, slot_number, drive_handle_p_spa);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_reinsert_drive: fail to reinsert drive from SPA");
    fbe_api_sleep(2000);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_test_pp_util_reinsert_drive(port_number, enclosure_number, slot_number, drive_handle_p_spb);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_reinsert_drive: fail to reinsert drive from SPB");

    fbe_api_sim_transport_set_target_server(original_target);
}

/*!**************************************************************
 * sabbath_insert_blank_new_drive()
 ****************************************************************
 * @brief
 *  Insert a blank drive into array
 *
 * @param port_number - drive location where to insert.
 * @param enclosure_number - drive location where to insert.
 * @param slot_number - drive location where to insert.
 * @param drive_handle_p_spa - the handle of the newly inserted drive on SPA
 * @param drive_handle_p_spb - the handle of the newly inserted drive on SPB
 *
 * @return None.   
 *
 * @author
 *  07/25/2012 - Created. Zhipeng Hu
 ****************************************************************/
void sabbath_insert_blank_new_drive(fbe_u32_t port_number, 
                                    fbe_u32_t enclosure_number, 
                                    fbe_u32_t slot_number,
                                    fbe_api_terminator_device_handle_t *drive_handle_p_spa,
                                    fbe_api_terminator_device_handle_t *drive_handle_p_spb)
{
    fbe_status_t    status;
    fbe_sim_transport_connection_target_t   original_target;
    fbe_sas_address_t   blank_new_sas_address;
    
    original_target = fbe_api_sim_transport_get_target_server();
    blank_new_sas_address = fbe_test_pp_util_get_unique_sas_address();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_test_pp_util_insert_sas_drive_extend(port_number, enclosure_number, slot_number, 
                                                                                                        520, SABBATH_TEST_DRIVE_CAPACITY, 
                                                                                                        blank_new_sas_address, FBE_SAS_DRIVE_SIM_520, drive_handle_p_spb);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_insert_blank_new_drive: fail to insert drive from SPB");

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_test_pp_util_insert_sas_drive_extend(port_number, enclosure_number, slot_number, 
                                                                                                        520, SABBATH_TEST_DRIVE_CAPACITY, 
                                                                                                        blank_new_sas_address, FBE_SAS_DRIVE_SIM_520, drive_handle_p_spa);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_insert_blank_new_drive: fail to insert drive from SPA");


    fbe_api_sim_transport_set_target_server(original_target);
}

/*!**************************************************************
 * sabbath_insert_blank_new_drive_extended()
 ****************************************************************
 * @brief
 *  Insert a blank new drive into array
 *
 * @param port_number - drive location where to insert.
 * @param enclosure_number - drive location where to insert.
 * @param slot_number - drive location where to insert.
 * @param capacity - the capacity of the new drive
 * @param drive_type - the type of the new drive
 * @param block_size - block size of the drive
 * @param drive_handle_p_spa - the handle of the newly inserted drive on SPA
 * @param drive_handle_p_spb - the handle of the newly inserted drive on SPB
 *
 * @return None.   
 *
 * @author
 *  02/28/2013 - Created. Zhipeng Hu
 ****************************************************************/
static void sabbath_insert_blank_new_drive_extended(fbe_u32_t port_number, 
                                         fbe_u32_t enclosure_number, 
                                         fbe_u32_t slot_number,
                                         fbe_lba_t capacity,
                                         fbe_sas_drive_type_t drive_type,
                                         fbe_block_size_t block_size,
                                         fbe_api_terminator_device_handle_t *drive_handle_p_spa,
                                         fbe_api_terminator_device_handle_t *drive_handle_p_spb)
{
    fbe_status_t    status;
    fbe_sim_transport_connection_target_t   original_target;
    fbe_sas_address_t   blank_new_sas_address;
    
    original_target = fbe_api_sim_transport_get_target_server();
    blank_new_sas_address = fbe_test_pp_util_get_unique_sas_address();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_test_pp_util_insert_sas_drive_extend(port_number, enclosure_number, slot_number, 
                                                      block_size, capacity, 
                                                      blank_new_sas_address, drive_type, drive_handle_p_spb);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_insert_blank_new_drive_extended: fail to insert drive from SPB");

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_test_pp_util_insert_sas_drive_extend(port_number, enclosure_number, slot_number, 
                                                      block_size, capacity, 
                                                      blank_new_sas_address, drive_type, drive_handle_p_spa);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_insert_blank_new_drive_extended: fail to insert drive from SPA");


    fbe_api_sim_transport_set_target_server(original_target);
}


/*!**************************************************************
 * sabbath_wait_for_sep_object_state()
 ****************************************************************
 * @brief
 *  Wait for a SEP object to become the specified lifecycle state
 *
 * @param object_id - SEP object id.
 * @param expected_lifecycle_state - the specified lifecycle state.
 * @param wait_spa - whether to wait on SPA.
 * @param wait_spb - whether to wait on SPB.
 * @param wait_sec - how many seconds to wait
 *
 * @return whether waited the expected state.   
 *
 * @author
 *  07/26/2012 - Created. Zhipeng Hu
 ****************************************************************/
fbe_bool_t  sabbath_wait_for_sep_object_state(fbe_object_id_t object_id, fbe_lifecycle_state_t  expected_lifecycle_state, 
                                              fbe_bool_t wait_spa, fbe_bool_t wait_spb, fbe_u32_t wait_sec)
{
    fbe_status_t    status;
    fbe_lifecycle_state_t   current_state;
    fbe_u32_t   wait_sec_count = wait_sec;
    fbe_u32_t   remain_wait_count = 0;
    fbe_bool_t  spa_wait_success = FBE_FALSE;
    fbe_bool_t  spb_wait_success = FBE_FALSE;
    fbe_sim_transport_connection_target_t   original_target;
    
    original_target = fbe_api_sim_transport_get_target_server();    

    if(FBE_TRUE == wait_spa && FBE_TRUE == wait_spb)
    {
        remain_wait_count = 2;
    }
    else if(FBE_TRUE == wait_spa || FBE_TRUE == wait_spb)
    {
        remain_wait_count = 1;
    }
    else
    {
        return FBE_TRUE;
    }

    while(wait_sec_count--)
    {
        if(FBE_FALSE == spa_wait_success && wait_spa)
        {
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
            status = fbe_api_get_object_lifecycle_state (object_id, &current_state, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_wait_for_sep_object_state: fail to get object lifecycle state in SPA");
            if(expected_lifecycle_state == current_state)
            {
                spa_wait_success = FBE_TRUE;
                remain_wait_count--;
            }
        }

        if(FBE_FALSE == spb_wait_success && wait_spb)
        {
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
            status = fbe_api_get_object_lifecycle_state (object_id, &current_state, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_wait_for_sep_object_state: fail to get object lifecycle state in SPB");
            if(expected_lifecycle_state == current_state)
            {
                spb_wait_success = FBE_TRUE;
                remain_wait_count--;
            }
        }

        if(0 == remain_wait_count)
            break;
        fbe_api_sleep(1000);
    }

    fbe_api_sim_transport_set_target_server(original_target);

    if(remain_wait_count > 0)
    {
        return FBE_FALSE;
    }

    return FBE_TRUE;   
    
}

/*!**************************************************************
 * sabbath_check_pvd_downstream_connection()
 ****************************************************************
 * @brief
 *  Check whether specified PVD has downstream connection. If have, whether it
 *  connects to the expected drive
 *
 * @param object_id - SEP object id.
 * @param expected_bus - the expected drive location.
 * @param expected_encl - the expected drive location.
 * @param expected_slot - the expected drive location.
 * @param wait_spa - whether to wait on SPA.
 * @param wait_spb - whether to wait on SPB.
 *
 * @return whether PVD has downstream connection.   
 *
 * @author
 *  07/26/2012 - Created. Zhipeng Hu
 ****************************************************************/
static fbe_bool_t   sabbath_check_pvd_downstream_connection(fbe_object_id_t object_id, fbe_u32_t expected_bus, 
                                                                            fbe_u32_t expected_encl, fbe_u32_t expected_slot, fbe_bool_t check_spa, fbe_bool_t check_spb)
{
    fbe_status_t    status;
    fbe_bool_t  downstream_exist_spa;
    fbe_bool_t  downstream_exist_spb;
    fbe_u32_t   location_spa_bus;
    fbe_u32_t   location_spa_encl;    fbe_u32_t   location_spa_slot;
    fbe_u32_t   location_spb_bus;
    fbe_u32_t   location_spb_encl;    fbe_u32_t   location_spb_slot;
    fbe_api_base_config_downstream_object_list_t downstream_list;
    fbe_sim_transport_connection_target_t   original_target;
    fbe_api_provision_drive_info_t  pvd_info = {0};

    if(FBE_FALSE == check_spa && FBE_FALSE == check_spb)
    {
        MUT_ASSERT_INT_EQUAL_MSG(0, 1 , "sabbath_check_sep_downstream_connection: Are you kidding? Both input parameters check_spa and check_spb are FBE_FALSE!!!!");
        return FBE_FALSE;
    }
    
    original_target = fbe_api_sim_transport_get_target_server();    

    if(FBE_TRUE == check_spa)
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        status = fbe_api_base_config_get_downstream_object_list(object_id, &downstream_list);
        if(FBE_STATUS_BUSY == status)  /*if object is in SPECIALIZE state, return false*/
        {
            fbe_api_sim_transport_set_target_server(original_target);
            return FBE_FALSE;
        }
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_check_sep_downstream_connection: fail to get down list in SPA");
        downstream_exist_spa = downstream_list.number_of_downstream_objects > 0?FBE_TRUE:FBE_FALSE;
        if(FBE_TRUE == downstream_exist_spa)
        {
            status = fbe_api_provision_drive_get_info(object_id, &pvd_info);
            MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_check_sep_downstream_connection: fail to get pvd location in SPA");
            
            location_spa_bus = pvd_info.port_number;
            location_spa_encl = pvd_info.enclosure_number;
            location_spa_slot = pvd_info.slot_number;
        }
    }

    if(FBE_TRUE == check_spb)
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        status = fbe_api_base_config_get_downstream_object_list(object_id, &downstream_list);
        if(FBE_STATUS_BUSY == status)  /*if object is in SPECIALIZE state, return false*/
        {
            fbe_api_sim_transport_set_target_server(original_target);
            return FBE_FALSE;
        }
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_check_sep_downstream_connection: fail to get down list in SPB");
        downstream_exist_spb = downstream_list.number_of_downstream_objects > 0?FBE_TRUE:FBE_FALSE;
        if(FBE_TRUE == check_spa && (downstream_exist_spb != downstream_exist_spa))
        {
            MUT_ASSERT_INT_EQUAL_MSG(0, 1 , "sabbath_check_sep_downstream_connection: downstream connection states are not consistent between two SPs");
            fbe_api_sim_transport_set_target_server(original_target);
            return FBE_FALSE;
        }
        if(FBE_TRUE == downstream_exist_spb)
        {
            status = fbe_api_provision_drive_get_info(object_id, &pvd_info);
            MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_check_sep_downstream_connection: fail to get pvd location in SPB");
            
            location_spb_bus = pvd_info.port_number;
            location_spb_encl = pvd_info.enclosure_number;
            location_spb_slot = pvd_info.slot_number;
            
            if(FBE_TRUE == check_spa && (location_spb_bus != location_spa_bus || location_spb_encl != location_spa_encl ||location_spb_slot != location_spa_slot))
            {
                MUT_ASSERT_INT_EQUAL_MSG(0, 1 , "sabbath_check_sep_downstream_connection: PVDs connect to different drives on two SPs");
                fbe_api_sim_transport_set_target_server(original_target);
                return FBE_FALSE;
            }
        }
    }
    
    fbe_api_sim_transport_set_target_server(original_target);
    if(FBE_TRUE == check_spa)
    {
        if(downstream_exist_spa && (location_spa_bus != expected_bus || location_spa_encl != expected_encl || location_spa_slot != expected_slot))
        {
            MUT_ASSERT_INT_EQUAL_MSG(0, 1 , "sabbath_check_sep_downstream_connection: SPA PVD is not connected to expected drive");
        }
        return downstream_exist_spa;
    }

    if(FBE_TRUE == check_spb)
    {
        if(downstream_exist_spb && (location_spb_bus != expected_bus || location_spb_encl != expected_encl || location_spb_slot != expected_slot))
        {
            MUT_ASSERT_INT_EQUAL_MSG(0, 1 , "sabbath_check_sep_downstream_connection: SPB PVD is not connected to expected drive");
        }
        return downstream_exist_spb;
    }

    MUT_ASSERT_INT_EQUAL_MSG(0, 1 , "sabbath_check_sep_downstream_connection: Are you kidding? Both input parameters check_spa and check_spb are FBE_FALSE!!!!");
    return FBE_FALSE;
    
}

/*!**************************************************************
 * sabbath_check_drive_upstream_connection()
 ****************************************************************
 * @brief
 *  Check whether specified drive has connected to a SEP PVD. If have, output
 *  the PVD's object id.
 *
 * @param upstream_object_id - output PVD's object id.
 * @param bus - drive location.
 * @param encl - drive location.
 * @param slot - drive location.
 * @param wait_spa - whether to wait on SPA.
 * @param wait_spb - whether to wait on SPB.
 *
 * @return whether drive connects to a PVD
 *
 * @author
 *  07/26/2012 - Created. Zhipeng Hu
 ****************************************************************/
static fbe_bool_t   sabbath_check_drive_upstream_connection(fbe_u32_t   bus, fbe_u32_t  encl, fbe_u32_t slot, 
                                                                                                                        fbe_object_id_t *upstream_object_id, fbe_bool_t check_spa, fbe_bool_t check_spb)
{
    fbe_object_id_t pvd_id_spa = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t pvd_id_spb = FBE_OBJECT_ID_INVALID;
    fbe_sim_transport_connection_target_t   original_target;

    if(FBE_FALSE == check_spa && FBE_FALSE == check_spb)
    {
        MUT_ASSERT_INT_EQUAL_MSG(0, 1 , "sabbath_check_drive_upstream_connection: Are you kidding? Both input parameters check_spa and check_spb are FBE_FALSE!!!!");
        return FBE_FALSE;
    }

    original_target = fbe_api_sim_transport_get_target_server();

    if(check_spa)
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_api_provision_drive_get_obj_id_by_location_from_topology(bus, encl, slot, &pvd_id_spa);
    }

    if(check_spb)
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_api_provision_drive_get_obj_id_by_location_from_topology(bus, encl, slot, &pvd_id_spb);
        if(check_spa)
        {
            if(pvd_id_spa != pvd_id_spb)
            {
                MUT_ASSERT_INT_EQUAL_MSG(pvd_id_spa, pvd_id_spb, "sabbath_check_drive_upstream_connection: the drive connects to different PVDs on different SPs");
                fbe_api_sim_transport_set_target_server(original_target);
                return FBE_FALSE;
            }
        }
    }

    fbe_api_sim_transport_set_target_server(original_target);
    if(FBE_OBJECT_ID_INVALID != pvd_id_spa || FBE_OBJECT_ID_INVALID != pvd_id_spb)
    {
        if(NULL != upstream_object_id)
        {
            *upstream_object_id = pvd_id_spa == FBE_OBJECT_ID_INVALID?pvd_id_spb:pvd_id_spa;
        }
        return FBE_TRUE;
    }

    return FBE_FALSE;
    
}



/*!****************************************************************************
 *  sabbath_check_event_log_msg
 ******************************************************************************
 *
 * @brief
 *   This is the test function to check event messages for LUN zeroing operation.
 *
 * @param   pvd_object_id       - provision drive object id
 * @param   lun_number          - LUN number
 * @param   is_lun_zero_start  - 
 *                  FBE_TRUE  - check message for lun zeroing start
 *                  FBE_FALSE - check message for lun zeroing complete
 *
 * @return  is_msg_present 
 *                  FBE_TRUE  - if expected event message found
 *                  FBE_FALSE - if expected event message not found
 *
 * @author
 *
 *
 *****************************************************************************/
static fbe_bool_t sabbath_check_event_log_msg(fbe_u32_t message_code, int *disk_seq)
{

    fbe_status_t                        status;               
    fbe_bool_t                          is_msg_present = FBE_FALSE;    

    if(disk_seq == NULL)
    {
        MUT_ASSERT_TRUE(message_code != SEP_ERROR_CODE_DB_DRIVES_DISORDERED_ERROR);
        status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_SEP_0, &is_msg_present, message_code);
        
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    else
    {
        MUT_ASSERT_TRUE(message_code == SEP_ERROR_CODE_DB_DRIVES_DISORDERED_ERROR);
        /* check that given event message is logged correctly */
        status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_SEP_0, &is_msg_present, 
                        message_code, disk_seq[0], disk_seq[1], disk_seq[2]);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    return is_msg_present; 
}

/*!****************************************************************************
 *  sabbath_get_active_sp
 ******************************************************************************
 *
 * @brief
 *   Get active target
 *
 * @param   target     -  the active SP
 *
 * @return  status of operation
 *
 * @author
 *
 *
 *****************************************************************************/
static fbe_status_t sabbath_get_active_sp(fbe_sim_transport_connection_target_t *target)
{
    fbe_sim_transport_connection_target_t   original_target;
	fbe_cmi_service_get_info_t spa_cmi_info;
	fbe_cmi_service_get_info_t spb_cmi_info;
	fbe_sim_transport_connection_target_t active_connection_target = FBE_SIM_INVALID_SERVER;
	fbe_sim_transport_connection_target_t passive_connection_target = FBE_SIM_INVALID_SERVER;

    original_target = fbe_api_sim_transport_get_target_server();

	do {
		fbe_api_sleep(100);
		/* We have to figure out whitch SP is ACTIVE */
		fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
		fbe_api_cmi_service_get_info(&spa_cmi_info);
		fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
		fbe_api_cmi_service_get_info(&spb_cmi_info);

		if(spa_cmi_info.sp_state == FBE_CMI_STATE_ACTIVE){
			active_connection_target = FBE_SIM_SP_A;
			passive_connection_target = FBE_SIM_SP_B;
		}
		if(spb_cmi_info.sp_state == FBE_CMI_STATE_ACTIVE){
			active_connection_target = FBE_SIM_SP_B;
			passive_connection_target = FBE_SIM_SP_A;
		}
	} while(active_connection_target != FBE_SIM_SP_A && active_connection_target != FBE_SIM_SP_B);
    fbe_api_sim_transport_set_target_server(original_target);
    *target = active_connection_target;

    return FBE_STATUS_OK;
}

/* Reboot the system
  * Validate the fru descriptor
  */
static void sabbath_test_case_2_normal_case(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_provision_drive_info_t  drive_info;
    fbe_homewrecker_fru_descriptor_t    descriptor;
    fbe_bool_t  warning;
    fbe_u32_t   count;
    fbe_u32_t   system_drive_count = fbe_private_space_layout_get_number_of_system_drives();
    fbe_u32_t   array_wwn_seed;

    mut_printf(MUT_LOG_TEST_STATUS, "*J* Homewrecker Test: sabbath_test_case_2_normal_case *L*\n");
    /* reboot */
    mut_printf(MUT_LOG_TEST_STATUS, "Sabbath_test_case_2_normal_case: reboot the system ...\n");
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);

    /*validate the FRU descriptors*/
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_database_get_fru_descriptor_info(&descriptor, FBE_FRU_DISK_ALL, &warning);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_test_case_2_normal_case: Fail to get fru descriptor from raw-mirror");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, warning, "sabbath_test_case_2_normal_case: There is problem when reading raw-mirror");

    if(fbe_compare_string(descriptor.magic_string, FBE_FRU_DESCRIPTOR_MAGIC_STRING_LENGTH, FBE_FRU_DESCRIPTOR_MAGIC_STRING, FBE_FRU_DESCRIPTOR_MAGIC_STRING_LENGTH, FBE_TRUE))
    {
        MUT_ASSERT_INT_EQUAL_MSG(0, 1, "sabbath_test_case_2_normal_case: Invalid magic number of descriptor");
        return;
    }
    
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_database_system_obtain_prom_wwnseed_cmd(&array_wwn_seed);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_test_case_2_normal_case: Fail to get prom wwn seed.");
    MUT_ASSERT_INT_EQUAL_MSG(array_wwn_seed, descriptor.wwn_seed, "sabbath_test_case_2_normal_case: The wwn seed in the descriptor is not equal to the array's");
    MUT_ASSERT_INT_EQUAL_MSG(0, descriptor.chassis_replacement_movement, "sabbath_test_case_2_normal_case: The chassis replacement flag is set, which is not expected");
    
    /* get the serial number of each system drive and do validation */
    for(count = 0; count < system_drive_count; count++)
    {
        status = fbe_api_provision_drive_get_info(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + count, &drive_info);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_test_case_2_normal_case: Fail to get provision drive info");
        if(fbe_compare_string(descriptor.system_drive_serial_number[count], FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE, drive_info.serial_num, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE, FBE_TRUE))
        {
            MUT_ASSERT_INT_EQUAL_MSG(0, 1, "sabbath_test_case_2_normal_case: Invalid serial number");
            return;
        }
    }

    mut_printf(MUT_LOG_TEST_STATUS, "Sabbath_test_case_2_normal_case: 2nd case PASSed, end\n");
}

/*Test:
  *1. Remove system drive 0_0_1
  *2. Reboot system
  *3. Check system is in READY state
  *4. Restore the system to its original state*/
static void sabbath_test_case_3_remove_single_db_drive(void)
{
    fbe_api_terminator_device_handle_t drive_handle_SPA;
    fbe_api_terminator_device_handle_t drive_handle_SPB;
    fbe_bool_t  downstream_connection_status;
    fbe_bool_t  wait_state;

    mut_printf(MUT_LOG_TEST_STATUS, "*J* Homewrecker Test: sabbath_test_case_3_remove_single_db_drive *L*\n");
    mut_printf(MUT_LOG_TEST_STATUS, "Sabbath_test_case_3_normal_case: pull 0_0_1 out...\n");
      
    /* In this case, one db drive need to be pulled out from enclosure.*/
    /* Remove the 0_0_1 drive */
    sabbath_pull_drive(0, 0, 1, &drive_handle_SPA, &drive_handle_SPB);

    /* reboot */
    mut_printf(MUT_LOG_TEST_STATUS, "Sabbath_test_case_3_remove_single_db_drive: reboot the system ...\n");
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "Sabbath_test_case_3_remove_single_db_drive: reinsert db drive back...\n");
    
    /* reinsert the removed db drive restore the system to normal status */
    sabbath_reinsert_drive(0, 0, 1, drive_handle_SPA, drive_handle_SPB);
    fbe_api_sleep(5000);
    /*check PVD state*/
    downstream_connection_status = sabbath_check_pvd_downstream_connection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 1,
                                                                                                                        0, 0, 1, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, downstream_connection_status, "Sabbath_test_case_3_remove_single_db_drive: system PVD1 is not connected to drive 0_0_1, not expected.");
    wait_state = sabbath_wait_for_sep_object_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 1, FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_state, "Sabbath_test_case_3_remove_single_db_drive: system PVD1 is not in READY state, not expected.");

    mut_printf(MUT_LOG_TEST_STATUS, "Sabbath_test_case_3_remove_single_db_drive: 3rd case PASSed, end\n");
}


/*Test:
  *1. Remove system drive 0_0_1 and 0_0_0
  *2. Reboot system
  *3. Check system is NOT in SERVICE_MODE
  *4. Restore the system to its original state*/
static void sabbath_test_case_4_remove_two_db_drives(void)
{
    fbe_bool_t  downstream_connection_status;
    fbe_bool_t  wait_state;
    fbe_api_terminator_device_handle_t drive_handle0_spa;
    fbe_api_terminator_device_handle_t drive_handle1_spa;
    fbe_api_terminator_device_handle_t drive_handle0_spb;
    fbe_api_terminator_device_handle_t drive_handle1_spb;

    mut_printf(MUT_LOG_TEST_STATUS, "*J* Homewrecker Test: sabbath_test_case_4_remove_two_db_drives *L*\n");
    mut_printf(MUT_LOG_TEST_STATUS, "sabbath_test_case_4_remove_two_db_drives: pull 0_0_0 and 0_0_1..\n");
      
    /* In this case, one db drive need to be pulled out from enclosure.*/
    /* Remove the 0_0_0 and 0_0_1 drives */
    sabbath_pull_drive(0, 0, 0, &drive_handle0_spa, &drive_handle0_spb);
    sabbath_pull_drive(0, 0, 1, &drive_handle1_spa, &drive_handle1_spb);

    /* reboot */
    mut_printf(MUT_LOG_TEST_STATUS, "Sabbath_test_case_4_remove_two_db_drives: reboot the system ...\n");
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "Sabbath_test_case_4_remove_two_db_drives: reinsert db drives back...\n");
    
    /* reinsert the removed db drive restore the system to normal status */
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    sabbath_reinsert_drive(0, 0, 0, drive_handle0_spa, drive_handle0_spb);
    sabbath_reinsert_drive(0, 0, 1, drive_handle1_spa, drive_handle1_spb);

    mut_printf(MUT_LOG_TEST_STATUS, "Sabbath_test_case_4_remove_two_db_drives: Reboot system\n");
    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);

    /*check PVD state*/
    downstream_connection_status = sabbath_check_pvd_downstream_connection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 1,
                                                                                                                        0, 0, 1, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, downstream_connection_status, "Sabbath_test_case_3_remove_single_db_drive: system PVD1 is not connected to drive 0_0_1, not expected.");
    wait_state = sabbath_wait_for_sep_object_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 1, FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_state, "Sabbath_test_case_3_remove_single_db_drive: system PVD1 is not in READY state, not expected.");
    downstream_connection_status = sabbath_check_pvd_downstream_connection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 0,
                                                                                                                        0, 0, 0, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, downstream_connection_status, "Sabbath_test_case_3_remove_single_db_drive: system PVD0 is not connected to drive 0_0_1, not expected.");
    wait_state = sabbath_wait_for_sep_object_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 0, FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_state, "Sabbath_test_case_3_remove_single_db_drive: system PVD0 is not in READY state, not expected.");


    mut_printf(MUT_LOG_TEST_STATUS, "Sabbath_test_case_4_remove_two_db_drives: 4th case PASSed, end\n");
}

/*Test:
  *1. Modify the midplane's wwn seed to a new one
  *2. Reboot system
  *3. Check system is in SERVICE_MODE
  *4. Restore the system to its original state*/
static void sabbath_test_case_5_unplanned_chassis_replacement(void)
{
    fbe_status_t               status;
    fbe_u32_t                   original_wwn_seed = 0;
    fbe_u32_t                   test_data_wwn_seed = 233391;

    mut_printf(MUT_LOG_TEST_STATUS, "*J* Homewrecker Test: sabbath_test_case_5_unplanned_chassis_replacement *L*\n");
    mut_printf(MUT_LOG_TEST_STATUS, "sabbath_test_case_5_unplanned_chassis_replacement: store system original wwn_seed\n");
    /* In fbe_sim, the wwn_seed is 0 */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);    
    status = fbe_api_database_system_obtain_prom_wwnseed_cmd(&original_wwn_seed);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);    
    
    mut_printf(MUT_LOG_TEST_STATUS, "sabbath_test_case_5_unplanned_chassis_replacement: persist a mismatch wwn_seed into PROM\n");
    /* persist the another wwn_seed into PROM. On simulator, we have to persist it on both SP simulators */
    status = fbe_api_database_system_persist_prom_wwnseed_cmd(&test_data_wwn_seed);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_database_system_persist_prom_wwnseed_cmd(&test_data_wwn_seed);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* reboot */
    mut_printf(MUT_LOG_TEST_STATUS, "sabbath_test_case_5_unplanned_chassis_replacement: reboot the system ......\n");
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    sabbath_poweron_systems(FBE_TRUE, FBE_TRUE);

    sabbath_check_database_states(FBE_DATABASE_STATE_SERVICE_MODE, FBE_DATABASE_STATE_SERVICE_MODE);

    mut_printf(MUT_LOG_TEST_STATUS, "Sabbath_test_case_5_unplanned_chassis_replacement: database service mode:)\n");
    
    /* restore the system's original wwn_seed */
    mut_printf(MUT_LOG_TEST_STATUS, "sabbath_test_case_5_unplanned_chassis_replacement: restore the system's original wwn_seed\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);    
     status = fbe_api_database_system_persist_prom_wwnseed_cmd(&original_wwn_seed);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_database_system_persist_prom_wwnseed_cmd(&original_wwn_seed);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Sabbath_test_case_5_unplanned_chassis_replacement: Reboot system\n");
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "Sabbath_test_case_5_unplanned_chassis_replacement: 5th case PASSed\n");

}

/*Test:
  *1. Modify the midplane's wwn seed to a new one
  *2. Set the chassis replacement flag and then Reboot system
  *3. Check system is in READY state
  */
static void sabbath_test_case_6_planned_chassis_replacement(void)
{
    fbe_status_t               status;
    fbe_homewrecker_fru_descriptor_t    descriptor;
    fbe_u32_t                   array_wwn_seed;
    fbe_u32_t                   system_drive_count = fbe_private_space_layout_get_number_of_system_drives();
    fbe_api_provision_drive_info_t  drive_info;
    fbe_u32_t                   count;
    fbe_bool_t                  warning;
    fbe_u32_t                   original_wwn_seed = 0;
    fbe_u32_t                   test_data_wwn_seed = 233391;

    mut_printf(MUT_LOG_TEST_STATUS, "*J* Homewrecker Test: sabbath_test_case_6_planned_chassis_replacement *L*\n");
    mut_printf(MUT_LOG_TEST_STATUS, "sabbath_test_case_6_planned_chassis_replacement: store system original wwn_seed\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);    
    status = fbe_api_database_system_obtain_prom_wwnseed_cmd(&original_wwn_seed);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);    
    
    mut_printf(MUT_LOG_TEST_STATUS, "sabbath_test_case_6_planned_chassis_replacement: persist a mismatch wwn_seed into PROM\n");
    /* persist another wwn_seed into PROM */
    status = fbe_api_database_system_persist_prom_wwnseed_cmd(&test_data_wwn_seed);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_database_system_persist_prom_wwnseed_cmd(&test_data_wwn_seed);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    /* set the chassis replacement flag */
    mut_printf(MUT_LOG_TEST_STATUS, "sabbath_test_case_6_planned_chassis_replacement: set the chassis replacement flag\n");
    status = fbe_api_database_set_chassis_replacement_flag(1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);   
    
    /* reboot */
    mut_printf(MUT_LOG_TEST_STATUS, "sabbath_test_case_6_planned_chassis_replacement: reboot the system ......\n");
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);

    /*validate the FRU descriptors*/
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_database_get_fru_descriptor_info(&descriptor, FBE_FRU_DISK_ALL, &warning);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_test_case_6_planned_chassis_replacement: Fail to get fru descriptor from raw-mirror");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, warning, "sabbath_test_case_6_planned_chassis_replacement: There is problem when reading raw-mirror");

    if(fbe_compare_string(descriptor.magic_string, FBE_FRU_DESCRIPTOR_MAGIC_STRING_LENGTH, FBE_FRU_DESCRIPTOR_MAGIC_STRING, FBE_FRU_DESCRIPTOR_MAGIC_STRING_LENGTH, FBE_TRUE))
    {
        MUT_ASSERT_INT_EQUAL_MSG(0, 1, "sabbath_test_case_6_planned_chassis_replacement: Invalid magic number of descriptor");
        return;
    }
    
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_database_system_obtain_prom_wwnseed_cmd(&array_wwn_seed);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_test_case_6_planned_chassis_replacement: Fail to get prom wwn seed.");
    MUT_ASSERT_INT_EQUAL_MSG(array_wwn_seed, descriptor.wwn_seed, "sabbath_test_case_6_planned_chassis_replacement: The wwn seed in the descriptor is not equal to the array's");
    MUT_ASSERT_INT_EQUAL_MSG(0, descriptor.chassis_replacement_movement, "sabbath_test_case_6_planned_chassis_replacement: The chassis replacement flag is set, which is not expected");
    
    /* get the serial number of each system drive and do validation */
    for(count = 0; count < system_drive_count; count++)
    {
        status = fbe_api_provision_drive_get_info(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + count, &drive_info);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_test_case_6_planned_chassis_replacement: Fail to get provision drive info");
        if(fbe_compare_string(descriptor.system_drive_serial_number[count], FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE, drive_info.serial_num, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE, FBE_TRUE))
        {
            MUT_ASSERT_INT_EQUAL_MSG(0, 1, "sabbath_test_case_6_planned_chassis_replacement: Invalid serial number");
            return;
        }
    }
    
  
    mut_printf(MUT_LOG_TEST_STATUS, "sabbath_test_case_6_planned_chassis_replacement: 6th case PASSed, END\n");
}

/*Test:
    1. modify PROM wwn seed and set usermodifiedwwnseed flag.
    2. reboot array check if usermodifiedwwnseed flag is cleared and database is ready
    3. check if wwn seed in chassis PROM is the same as the one in fru descriptor
*/
static void sabbath_test_case_7_user_modified_wwn_seed(void)
{
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_homewrecker_fru_descriptor_t    descriptor;
    fbe_u32_t                   system_drive_count = fbe_private_space_layout_get_number_of_system_drives();
    fbe_api_provision_drive_info_t  drive_info;
    fbe_u32_t                   count;
    fbe_bool_t                  warning;
    fbe_u32_t                   original_wwn_seed = 0;
    fbe_u32_t                   test_data_wwn_seed = 754457;
    fbe_bool_t                  user_modify_wwn_seed = FBE_FALSE;

    mut_printf(MUT_LOG_TEST_STATUS, "*J* Homewrecker Test: sabbath_test_case_7_user_modified_wwn_seed *L*\n");
    mut_printf(MUT_LOG_TEST_STATUS, "sabbath_test_case_7_user_modified_wwn_seed: store system original wwn_seed\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);    
    status = fbe_api_database_system_obtain_prom_wwnseed_cmd(&original_wwn_seed);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "sabbath_test_case_7_user_modified_wwn_seed: original wwn_seed %d\n", original_wwn_seed);

    
    status = fbe_api_esp_encl_mgmt_clear_user_modified_wwn_seed_flag();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    status = fbe_api_esp_encl_mgmt_get_user_modified_wwn_seed_flag(&user_modify_wwn_seed);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(user_modify_wwn_seed == FALSE);
    
    status = fbe_api_esp_encl_mgmt_user_modify_wwn_seed(&test_data_wwn_seed);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    /*wait resume prom info update*/
    mut_printf(MUT_LOG_LOW, "sabbath_test_case_7_user_modified_wwn_seed: waiting 10 seconds for resume prom write\n");
    fbe_api_sleep (10000);

    status = fbe_api_esp_encl_mgmt_get_user_modified_wwn_seed_flag(&user_modify_wwn_seed);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(user_modify_wwn_seed == TRUE);

    /* reboot */
    mut_printf(MUT_LOG_TEST_STATUS, "sabbath_test_case_7_user_modified_wwn_seed: reboot the system ......\n");
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);

    fbe_api_sleep (5000);
    status = fbe_api_database_get_fru_descriptor_info(&descriptor, FBE_FRU_DISK_ALL, &warning);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_test_case_7_user_modified_wwn_seed: Fail to get fru descriptor from raw-mirror");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, warning, "sabbath_test_case_7_user_modified_wwn_seed: There is problem when reading raw-mirror");
    
    if(fbe_compare_string(descriptor.magic_string, FBE_FRU_DESCRIPTOR_MAGIC_STRING_LENGTH, FBE_FRU_DESCRIPTOR_MAGIC_STRING, FBE_FRU_DESCRIPTOR_MAGIC_STRING_LENGTH, FBE_TRUE))
    {
        MUT_ASSERT_INT_EQUAL_MSG(0, 1, "sabbath_test_case_7_user_modified_wwn_seed: Invalid magic number of descriptor");
        return;
    }


    MUT_ASSERT_INT_EQUAL_MSG(test_data_wwn_seed, descriptor.wwn_seed, "sabbath_test_case_7_user_modified_wwn_seed: The wwn seed in the descriptor is not equal to the array");
    MUT_ASSERT_INT_EQUAL_MSG(0, descriptor.chassis_replacement_movement, "sabbath_test_case_7_user_modified_wwn_seed: The chassis replacement flag is set, which is not expected");
    
    /* get the serial number of each system drive and do validation */
    for(count = 0; count < system_drive_count; count++)
    {
        status = fbe_api_provision_drive_get_info(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + count, &drive_info);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_test_case_7_user_modified_wwn_seed: Fail to get provision drive info");
        if(fbe_compare_string(descriptor.system_drive_serial_number[count], FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE, drive_info.serial_num, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE, FBE_TRUE))
        {
            MUT_ASSERT_INT_EQUAL_MSG(0, 1, "sabbath_test_case_7_user_modified_wwn_seed: Invalid serial number");
            return;
        }
    }

    status = fbe_api_esp_encl_mgmt_get_user_modified_wwn_seed_flag(&user_modify_wwn_seed);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(user_modify_wwn_seed == FALSE);

    mut_printf(MUT_LOG_TEST_STATUS, "sabbath_test_case_7_user_modified_wwn_seed: PASSed\n");

}

/*
Test:
1. modify PROM wwn seed, set both usermodifiedwwnseed flag and chassis replacement flag.
2. reboot array, array should enter degraded mode.
3. recovery array.
*/
static void sabbath_test_case_8_set_both_wwn_seed_flag(void)
{
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                   test_data_wwn_seed = 120685;

    mut_printf(MUT_LOG_TEST_STATUS, "*J* Homewrecker Test: sabbath_test_case_8_set_both_wwn_seed_flag *L*\n");
    mut_printf(MUT_LOG_TEST_STATUS, "sabbath_test_case_8_set_both_wwn_seed_flag: store system original wwn_seed\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_esp_encl_mgmt_user_modify_wwn_seed(&test_data_wwn_seed);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_esp_encl_mgmt_user_modify_wwn_seed(&test_data_wwn_seed);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    /*wait resume prom info update*/
    mut_printf(MUT_LOG_LOW, "sabbath_test_case_8_set_both_wwn_seed_flag: waiting 10 seconds for resume prom write\n");
    fbe_api_sleep (10000);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    /* set the chassis replacement flag */
    mut_printf(MUT_LOG_TEST_STATUS, "sabbath_test_case_8_set_both_wwn_seed_flag: set the chassis replacement flag\n");
    status = fbe_api_database_set_chassis_replacement_flag(FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 

    mut_printf(MUT_LOG_TEST_STATUS, "sabbath_test_case_8_set_both_wwn_seed_flag: Reboot system\n");
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    sabbath_poweron_systems(FBE_TRUE, FBE_TRUE);

    sabbath_check_database_states(FBE_DATABASE_STATE_SERVICE_MODE, FBE_DATABASE_STATE_SERVICE_MODE);
    
    /* set the chassis replacement flag */
    mut_printf(MUT_LOG_TEST_STATUS, "sabbath_test_case_8_set_both_wwn_seed_flag: clear the chassis replacement flag\n");
    status = fbe_api_database_set_chassis_replacement_flag(FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);   

    mut_printf(MUT_LOG_TEST_STATUS, "sabbath_test_case_8_set_both_wwn_seed_flag: Reboot system\n");
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "sabbath_test_case_8_set_both_wwn_seed_flag: PASSed\n");
}


/*Test:
  *1. Remove system drive 0_0_1
  *2. Insert the drive back
  *3. Check system PVD 1 becomes READY
  */
static void sabbath_test_case_11_array_online_reinsert_a_db_drive_to_its_original_slot(void)
{
    fbe_bool_t  wait_lifecycle;
    fbe_api_terminator_device_handle_t drive_handler_spa;
    fbe_api_terminator_device_handle_t drive_handler_spb;

    /* system online cases, totally 8 cases */

    /* case a. Reinsert a DB drive to its original slot */
    mut_printf(MUT_LOG_TEST_STATUS, "****Case a. Array online reinsert a DB drive to its original slot\n");
    mut_printf(MUT_LOG_TEST_STATUS, "Pull system drive 0_0_1...");
    sabbath_pull_drive(0, 
                                            0, 
                                            1, 
                                            &drive_handler_spa, 
                                            &drive_handler_spb);

    mut_printf(MUT_LOG_TEST_STATUS, "Insert it back...");
    fbe_api_sleep(2000);
    /* reinsert the removed db drive restore the system to normal status */
    sabbath_reinsert_drive(0, 
                                            0, 
                                            1, 
                                            drive_handler_spa, 
                                            drive_handler_spb);

    /* Wait for PVD run its lifecycle or modify PVD<-->LDO connection */
    wait_lifecycle = sabbath_wait_for_sep_object_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 1,
                                                                            FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait system PVD 0x2 READY");

    mut_printf(MUT_LOG_TEST_STATUS, "****Case a. Array online reinsert a DB drive to its original slot end!!!\n");
    
}

/*Test:
  *1. Remove user drive 0_0_7
  *2. Insert the drive back
  *3. Check user PVD becomes READY and connects to its original PVD
  */
static void sabbath_test_case_12_array_online_reinsert_a_user_drive_to_its_original_slot(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t pvd_id;
    fbe_object_id_t pvd_id2;
    fbe_bool_t  wait_lifecycle;
    fbe_api_terminator_device_handle_t drive_handler_spa;
    fbe_api_terminator_device_handle_t drive_handler_spb;

    status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, 7, &pvd_id);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "fail to get pvd id by location");

    /* case b. Reinsert a user drive to its original slot */
    mut_printf(MUT_LOG_TEST_STATUS, "****Case b. Array online reinsert a user drive to its original slot\n");
    mut_printf(MUT_LOG_TEST_STATUS, "Pull user drive 0_0_7...");
    status = sabbath_hot_spare_control(FBE_FALSE);  /*disable automatic hotspare activity*/
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "fail to disable automatic hotspare activity");
    sabbath_pull_drive(0,  0, 7, &drive_handler_spa, &drive_handler_spb);

    mut_printf(MUT_LOG_TEST_STATUS, "Insert it back...");
    fbe_api_sleep(2000);
    /* reinsert the removed db drive restore the system to normal status */
    sabbath_reinsert_drive(0, 0, 7, drive_handler_spa, drive_handler_spb);

    /* Wait for PVD run its lifecycle or modify PVD<-->LDO connection */
    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id,
                                                                            FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait 0_0_7's PVD READY");
    wait_lifecycle = sabbath_check_drive_upstream_connection(0, 0, 7, &pvd_id2, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Drive 0_0_7 is not connected to any PVD.");
    MUT_ASSERT_INT_EQUAL_MSG(pvd_id, pvd_id2, "drive 0_0_7 is not connected to its original PVD.");
    
    mut_printf(MUT_LOG_TEST_STATUS, "****Case b. Array online reinsert a user drive to its original slot end!!!\n");
}

/*Test:
  *1. Remove unconsumed user drive 0_0_7 and system drive 0_0_1
  *2. Insert the system drive to 0_0_7 and then insert the user drive to db slot
  *3. Check system PVD connects to 0_0_7 and becomes READY; check the system drive does not connect to any PVD
  *4. Use fbe_api_database_online_planned_drive to make the drive in location 0_0_7 online
  *5. Reboot system
  *6. Remove consumed user drive 0_0_5 and system drive 0_0_1
  *7. Insert the user drive to 0_0_1 slot
  *8. Check the system PVD does not connect to the user drive and the original user PVD does not connect to the user drive, either.
  *9. Restore the system (swap 0_0_1 and 0_0_5)
  */
static void sabbath_test_case_13_array_online_move_a_user_drive_to_db_slot(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t pvd_id;
    fbe_bool_t  wait_lifecycle;
    fbe_bool_t  downstream_connect_state;
    fbe_bool_t  drive_upstream_connect_state;
    fbe_bool_t  drive_upstream_connection;
    fbe_database_control_online_planned_drive_t online_drive;
    fbe_api_terminator_device_handle_t drive_handler_1_spa;
    fbe_api_terminator_device_handle_t drive_handler_1_spb;
    fbe_api_terminator_device_handle_t drive_handler_7_spa;
    fbe_api_terminator_device_handle_t drive_handler_7_spb;
    fbe_api_terminator_device_handle_t drive_handler_5_spa;
    fbe_api_terminator_device_handle_t drive_handler_5_spb;

    /* case c. Move a user drive to DB slot*/
    /* case d. Move a DB drive to user slot */
    mut_printf(MUT_LOG_TEST_STATUS, "****Case c. Array online move an unconsumed user drive to DB slot.");
    mut_printf(MUT_LOG_TEST_STATUS, "****Case d. Array online move a DB drive to user slot.\n");
    
    status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, 7, &pvd_id);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "fail to get pvd id by location");
    
    mut_printf(MUT_LOG_TEST_STATUS, "Pull DB drive 0_0_1...");
    sabbath_pull_drive(0, 0, 1, &drive_handler_1_spa, &drive_handler_1_spb);    
    mut_printf(MUT_LOG_TEST_STATUS, "Pull user drive 0_0_7...");
    sabbath_pull_drive(0, 0, 7, &drive_handler_7_spa, &drive_handler_7_spb);
    fbe_api_sleep(2000);

    mut_printf(MUT_LOG_TEST_STATUS, "Insert DB drive 0_0_1 to slot 0_0_7...");
    sabbath_reinsert_drive(0, 0, 7, drive_handler_1_spa, drive_handler_1_spb);
    fbe_api_sleep(3000);
    mut_printf(MUT_LOG_TEST_STATUS, "Insert user drive 0_0_7 to slot 0_0_1...");
    sabbath_reinsert_drive(0, 0, 1, drive_handler_7_spa, drive_handler_7_spb);

    /* Wait for PVD run its lifecycle or modify PVD<-->LDO connection */
    fbe_api_sleep(10000); /*wait for 10secs for db processing*/
    /*0_0_7 pvd should have been destroyed*/
    status = fbe_api_wait_till_object_is_destroyed(pvd_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_test_case_13: user 0_0_7 PVD is not destroyed. Not Expected.");
    downstream_connect_state = sabbath_check_pvd_downstream_connection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 1, 0, 0, 1, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, downstream_connect_state, "sabbath_test_case_13: checked 0_0_1 PVD not connects to a drive, which is not expected");
    drive_upstream_connection = sabbath_check_drive_upstream_connection(0, 0, 7, NULL, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, drive_upstream_connection, "sabbath_test_case_13: checked drive 0_0_7 connects to a PVD, which is not expected");

    wait_lifecycle = sabbath_wait_for_sep_object_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 1,
                                                                            FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait system PVD 0x2 READY");

    /*check the user cli online_planned_drive, no matter use it on which SP*/
    online_drive.port_number = 0;
    online_drive.enclosure_number = 0;
    online_drive.slot_number = 7;
    online_drive.pdo_object_id = FBE_OBJECT_ID_INVALID;
    status = fbe_api_database_online_planned_drive(&online_drive);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_test_case_13: fail to make planned drive 0_0_7 online");
    fbe_api_sleep(10000);  /*wait database to process*/
    drive_upstream_connect_state = sabbath_check_drive_upstream_connection(0, 0, 7, &pvd_id, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, drive_upstream_connect_state, "sabbath_test_case_13: drive 0_0_7 is not connect to an upstream PVD, which is not expected");
    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id,
                                                                            FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait 0_0_7's PVD READY");

    /*no need to restore system because user drive 0_0_7 has been promoted to system drive
    *and the original system drive 0_0_1 has been downgraded to a user drive*/    

    mut_printf(MUT_LOG_TEST_STATUS, "reboot system.");
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "****Case c. Array online move an unconsumed user drive to DB slot end.");
    mut_printf(MUT_LOG_TEST_STATUS, "****Case d. Array online move a DB drive to user slot end.\n");
    
    mut_printf(MUT_LOG_TEST_STATUS, "****Case c-1. Array online move a consumed user drive to DB slot end.");

    status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, 5, &pvd_id);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "fail to get pvd id by location");
    
    mut_printf(MUT_LOG_TEST_STATUS, "Pull DB drive 0_0_1...");
    sabbath_pull_drive(0, 0, 1, &drive_handler_1_spa, &drive_handler_1_spb);    
    mut_printf(MUT_LOG_TEST_STATUS, "Pull user drive 0_0_5 which is in raid group...");
    sabbath_pull_drive(0, 0, 5, &drive_handler_5_spa, &drive_handler_5_spb);
    fbe_api_sleep(2000);

    mut_printf(MUT_LOG_TEST_STATUS, "Insert 0_0_5 to slot 0_0_1...");
    sabbath_reinsert_drive(0, 0, 1, drive_handler_5_spa, drive_handler_5_spb);
    fbe_api_sleep(5000); /*wait for database to process drives*/
    downstream_connect_state = sabbath_check_pvd_downstream_connection(pvd_id, 0, 0, 1, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, downstream_connect_state, "sabbath_test_case_13: checked 0_0_5 PVD connects to a drive, which is not expected");
    downstream_connect_state = sabbath_check_pvd_downstream_connection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 1, 0, 0, 1, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, downstream_connect_state, "sabbath_test_case_13: checked 0_0_1 PVD connects to a drive, which is not expected");
    drive_upstream_connection = sabbath_check_drive_upstream_connection(0, 0, 1, NULL, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, drive_upstream_connection, "sabbath_test_case_13: checked drive 0_0_1 connects to a PVD, which is not expected");

    /*reinsert drives*/
    mut_printf(MUT_LOG_TEST_STATUS, "Pull user drive from  0_0_1 slot...");
    sabbath_pull_drive(0, 0, 1, &drive_handler_5_spa, &drive_handler_5_spb);
    fbe_api_sleep(2000);
    mut_printf(MUT_LOG_TEST_STATUS, "Reinsert user drive 0_0_5 and db drive 0_0_1 back to their original slots...");
    sabbath_reinsert_drive(0, 0, 1, drive_handler_1_spa, drive_handler_1_spb);
    sabbath_reinsert_drive(0, 0, 5, drive_handler_5_spa, drive_handler_5_spb);
    fbe_api_sleep(10000);  /*wait for database to process drives*/ 

    mut_printf(MUT_LOG_TEST_STATUS, "****Case c-1. Array online move a consumed user drive to DB slot end.");

}

/*Test:
  *1. Remove unconsumed user drive 0_0_7 and consumed user drive 0_0_5
  *2. Swap the two drives
  *3. Check the original drive 0_0_7's PVD connects to drive 0_0_5 while the original drive 0_0_5's PVD connects to 0_0_7
  *4. Wait for the two PVDs' READY
  *5. Restore the system (swap 0_0_7 and 0_0_5)
  */
static void sabbath_test_case_14_array_online_move_a_user_drive_to_another_user_slot(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t  downstream_connect_state;
    fbe_object_id_t pvd_id_5;
    fbe_object_id_t pvd_id_7;
    fbe_bool_t  wait_lifecycle;
    fbe_api_terminator_device_handle_t drive_handler_5_spa;
    fbe_api_terminator_device_handle_t drive_handler_5_spb;
    fbe_api_terminator_device_handle_t drive_handler_7_spa;
    fbe_api_terminator_device_handle_t drive_handler_7_spb;


    /* case e. Move a user drive to another user slot */
    mut_printf(MUT_LOG_TEST_STATUS, "****Case e. Array online move a user drive to another user slot.\n");

    status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, 5, &pvd_id_5);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "fail to get pvd id by location");
    status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, 7, &pvd_id_7);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "fail to get pvd id by location");
    
    mut_printf(MUT_LOG_TEST_STATUS, "Pull consumed user drive 0_0_5 and unconsumed user drive 0_0_7...");
    sabbath_pull_drive(0, 0, 5, &drive_handler_5_spa, &drive_handler_5_spb);
    sabbath_pull_drive(0, 0, 7, &drive_handler_7_spa, &drive_handler_7_spb);
    fbe_api_sleep(2000);

    mut_printf(MUT_LOG_TEST_STATUS, "Insert consumed user drive 0_0_5 to slot 0_0_7...");
    sabbath_reinsert_drive(0, 0, 7, drive_handler_5_spa, drive_handler_5_spb);
    mut_printf(MUT_LOG_TEST_STATUS, "Insert unconsumed user drive 0_0_7 to slot 0_0_5...");
    sabbath_reinsert_drive(0, 0, 5, drive_handler_7_spa, drive_handler_7_spb);
    fbe_api_sleep(10000); /*wait for database to process drives*/
    downstream_connect_state = sabbath_check_pvd_downstream_connection(pvd_id_5, 0, 0, 7, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, downstream_connect_state, "sabbath_test_case_14: checked 0_0_5 PVD not connects to a drive, which is not expected");
    downstream_connect_state = sabbath_check_pvd_downstream_connection(pvd_id_7, 0, 0, 5, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, downstream_connect_state, "sabbath_test_case_14: checked 0_0_7 PVD not connects to a drive, which is not expected");

    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id_5,
                                                                            FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait pvd_id_5 READY");
    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id_7,
                                                                            FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait pvd_id_7 READY");

    /*resume original topology*/
    mut_printf(MUT_LOG_TEST_STATUS, "Resume original topology...");
    sabbath_pull_drive(0, 0, 5, &drive_handler_5_spa, &drive_handler_5_spb);
    sabbath_pull_drive(0, 0, 7, &drive_handler_7_spa, &drive_handler_7_spb);
    fbe_api_sleep(2000);
    sabbath_reinsert_drive(0, 0, 7, drive_handler_5_spa, drive_handler_5_spb);
    sabbath_reinsert_drive(0, 0, 5, drive_handler_7_spa, drive_handler_7_spb);
    fbe_api_sleep(5000); /*wait for database to process drives*/
    downstream_connect_state = sabbath_check_pvd_downstream_connection(pvd_id_5, 0, 0, 5, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, downstream_connect_state, "sabbath_test_case_14: checked 0_0_5 PVD not connects to a drive, which is not expected");
    downstream_connect_state = sabbath_check_pvd_downstream_connection(pvd_id_7, 0, 0, 7, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, downstream_connect_state, "sabbath_test_case_14: checked 0_0_7 PVD not connects to a drive, which is not expected");

    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id_5,
                                                                            FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait pvd_id_5 READY");
    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id_7,
                                                                            FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait pvd_id_7 READY");
    
    mut_printf(MUT_LOG_TEST_STATUS, "****Case e. Array online move a user drive to another user slot end.\n");

}

/*Test:
  *1. Remove system drive 0_0_1 and 0_0_2
  *2. Swap the two drives
  *3. Check the original drive 0_0_1's PVD not connects to drive 0_0_2 and the original drive 0_0_2's PVD connects to 0_0_1
  *4. Check no connection between PVDs and drives
  *5. Restore the system (swap 0_0_1 and 0_0_2)
  */
static void sabbath_test_case_15_array_online_move_a_db_drive_to_another_db_slot(void)
{
    fbe_bool_t  downstream_connect_state;
    fbe_bool_t  wait_lifecycle;
    fbe_api_terminator_device_handle_t drive_handler_1_spa;
    fbe_api_terminator_device_handle_t drive_handler_2_spa;
    fbe_api_terminator_device_handle_t drive_handler_1_spb;
    fbe_api_terminator_device_handle_t drive_handler_2_spb;

    /* case f. Move a DB drive to another DB slot */
    mut_printf(MUT_LOG_TEST_STATUS, "Case f. Array online move a DB drive to another DB slot.\n");
    mut_printf(MUT_LOG_TEST_STATUS, "Pull DB drive 0_0_1 and DB drive 0_0_2...");    
    sabbath_pull_drive(0, 0, 1, &drive_handler_1_spa, &drive_handler_1_spb);
    sabbath_pull_drive(0, 0, 2, &drive_handler_2_spa, &drive_handler_2_spb);
    fbe_api_sleep(2000);
 
    mut_printf(MUT_LOG_TEST_STATUS, "Insert DB drive 0_0_2 into slot 0_0_1...");
    sabbath_reinsert_drive(0, 0, 1, drive_handler_2_spa, drive_handler_2_spb);
    mut_printf(MUT_LOG_TEST_STATUS, "Insert DB drive 0_0_1 into slot 0_0_2...");
    sabbath_reinsert_drive(0, 0, 2, drive_handler_1_spa, drive_handler_1_spb);
    fbe_api_sleep(10000);  /*wait for database to process*/

    downstream_connect_state = sabbath_check_pvd_downstream_connection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 1, 0, 0, 2, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, downstream_connect_state, "sabbath_test_case_15: checked 0_0_1 PVD connects to a drive, which is not expected");
    downstream_connect_state = sabbath_check_pvd_downstream_connection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 2, 0, 0, 1, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, downstream_connect_state, "sabbath_test_case_15: checked 0_0_2 PVD connects to a drive, which is not expected");

    /*no need to test reboot system because this case would be tested in offline test*/
    mut_printf(MUT_LOG_TEST_STATUS, "Now restore system state...");    

    mut_printf(MUT_LOG_TEST_STATUS, "2. swap drives between 0_0_1 and 0_0_2...");    
    sabbath_pull_drive(0, 0, 1, &drive_handler_1_spa, &drive_handler_1_spb);
    sabbath_pull_drive(0, 0, 2, &drive_handler_2_spa, &drive_handler_2_spb); 
    fbe_api_sleep(2000);
    sabbath_reinsert_drive(0, 0, 1, drive_handler_2_spa, drive_handler_2_spb);
    sabbath_reinsert_drive(0, 0, 2, drive_handler_1_spa, drive_handler_1_spb);

    wait_lifecycle = sabbath_wait_for_sep_object_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 1,
                                                                            FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait system PVD 1 READY");
    wait_lifecycle = sabbath_wait_for_sep_object_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 2,
                                                                            FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait system PVD 7 READY");

    /*wait for all system RGs to do the necessary stuff*/
    fbe_api_sleep(20000);
    
    mut_printf(MUT_LOG_TEST_STATUS, "****Case f. Array online move a DB drive to another DB slot end.\n");    

}

/*Test:
  *1. Remove system drive 0_0_1
  *2. Insert a blank new drive into 0_0_1 location
  *3. Check the system PVD 1 connects to this new drive and its state becomes READY
  *4. Restart system
  *5. Check system state is READY
  */
static void sabbath_test_case_16_array_online_insert_a_new_drive_into_db_slot(void)
{
    fbe_bool_t  downstream_connect_state;
    fbe_bool_t  wait_lifecycle;
    fbe_api_terminator_device_handle_t drive_handler_spa;
    fbe_api_terminator_device_handle_t drive_handler_spb;
    fbe_api_terminator_device_handle_t drive_handler_new_drive_spa;
    fbe_api_terminator_device_handle_t drive_handler_new_drive_spb;

    /* case g. Insert  a new drive into DB slot */
    mut_printf(MUT_LOG_TEST_STATUS, "****Case g. Array online insert  a new drive into DB slot...\n");
    sabbath_pull_drive(0, 0, 1, &drive_handler_spa, &drive_handler_spb);    
    mut_printf(MUT_LOG_TEST_STATUS, "Insert  a new drive into DB slot...");
    fbe_api_sleep(2000);
    sabbath_insert_blank_new_drive(0, 0, 1, &drive_handler_new_drive_spa, &drive_handler_new_drive_spb);
    fbe_api_sleep(10000); /*wait for database to process*/
    downstream_connect_state = sabbath_check_pvd_downstream_connection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 1, 0, 0, 1, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, downstream_connect_state, "sabbath_test_case_16: checked 0_0_1 PVD not connects to a drive, which is not expected");

    wait_lifecycle = sabbath_wait_for_sep_object_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 1,
                                                                            FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait system PVD 0x1 READY");
    
    mut_printf(MUT_LOG_TEST_STATUS, "Reboot system......");
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);
    mut_printf(MUT_LOG_TEST_STATUS, "****Case g. Array online insert  a new drive into DB slot end.\n");    
}

/*Test:
  *1. Remove unconsumed drive 0_0_7
  *2. Insert a blank new drive into 0_0_7 location
  *3. Check the inserted drive connects to a newly created PVD and the PVD is in READY state
  */
static void sabbath_test_case_17_array_online_insert_a_new_drive_into_user_slot(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t  upstream_connect_state;
    fbe_bool_t  wait_lifecycle;
    fbe_object_id_t pvd_id;
    fbe_api_terminator_device_handle_t drive_handler_spa;
    fbe_api_terminator_device_handle_t drive_handler_spb;
    fbe_api_terminator_device_handle_t drive_handler_new_drive_spa;
    fbe_api_terminator_device_handle_t drive_handler_new_drive_spb;


    /* case h. Insert  a new drive into user slot */
    status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, 7, &pvd_id);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_test_case_17: fail to get pvd object id by location");

    mut_printf(MUT_LOG_TEST_STATUS, "****Case h. Array online insert  a new drive into user slot...\n");
    mut_printf(MUT_LOG_TEST_STATUS, "Pull user drive 0_0_7...");    
    sabbath_pull_drive(0, 0, 7, &drive_handler_spa, &drive_handler_spb);    
    mut_printf(MUT_LOG_TEST_STATUS, "Insert  a new drive into 0_0_7...");
    fbe_api_sleep(2000);
    sabbath_insert_blank_new_drive(0, 0, 7, &drive_handler_new_drive_spa, &drive_handler_new_drive_spb);
    fbe_api_sleep(10000); /*wait for database to process*/
    
    upstream_connect_state = sabbath_check_drive_upstream_connection(0, 0, 7, &pvd_id, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, upstream_connect_state, "sabbath_test_case_17: checked drive 0_0_7  not connects to a PVD, which is not expected");

    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id,
                                                                            FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait system 0)0_7's PVD READY");
    
    mut_printf(MUT_LOG_TEST_STATUS, "****Case g. Array online insert  a new drive into user slot end.\n");        
}

/*Test:
  *1. Shutdowns system
  *2. Remove unconsumed drive 0_0_7 and system drive 0_0_1
  *3. Insert drive 0_0_7 to system slot 1
  *4. Poweron system and check database is READY
  *5. Check user drive 0_0_7 has been promoted to a system drive
  *6. Insert the original system drive 0_0_1 into user location 0_0_7
  *7. Check the inserted drive becomes online and is a normal user drive
  *8. Shutdowns sytem
  *9. Remove consumed drive 0_0_5 and system drive 0_0_1
  *10. Insert drive 0_0_5 to system slot 1
  *11. Poweron system and check database is READY
  *12. Check user drive 0_0_5 is not connected to the system PVD
  *13. Restore the system
  */
static void sabbath_test_case_23_array_offline_move_a_user_drive_to_db_slot(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t pvd_id;
     fbe_object_id_t pvd_id2;
    fbe_bool_t  wait_lifecycle;
    fbe_bool_t  downstream_connect_state;
    fbe_bool_t  drive_upstream_connect_state;
    fbe_api_terminator_device_handle_t drive_handler_1_spa;
    fbe_api_terminator_device_handle_t drive_handler_1_spb;
    fbe_api_terminator_device_handle_t drive_handler_7_spa;
    fbe_api_terminator_device_handle_t drive_handler_7_spb;
    fbe_api_terminator_device_handle_t drive_handler_5_spa;
    fbe_api_terminator_device_handle_t drive_handler_5_spb;


    /* case c. Move a user drive to DB slot*/
    /* case d. Move a DB drive to user slot */
    status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, 7, &pvd_id);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_test_case_23: fail to get the 0_0_7's PVD obj id by location");
    mut_printf(MUT_LOG_TEST_STATUS, "****Case j. Array offline move an unconsumed user drive to DB slot.");
    mut_printf(MUT_LOG_TEST_STATUS, "****Case k. Array offline move a DB drive to user slot.\n");
    mut_printf(MUT_LOG_TEST_STATUS, "shutdown system");
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    
    mut_printf(MUT_LOG_TEST_STATUS, "Pull DB drive 0_0_1 and user drive 0_0_7...");
    sabbath_pull_drive(0, 0, 1, &drive_handler_1_spa, &drive_handler_1_spb);
    sabbath_pull_drive(0, 0, 7, &drive_handler_7_spa, &drive_handler_7_spb);
    mut_printf(MUT_LOG_TEST_STATUS, "Insert DB drive 0_0_1 to slot 0_0_7 and insert drive 0_0_7 to db slot 0_0_1...");
    fbe_api_sleep(2000);
    /*do not insert drive 0_0_1 to slot 7 offline because the system behavior is not certain after system power on.
    *This is becasue database would process the drive in system slot 1 first and then process drive in user slot 7.
    *When processing drive in system slot 1, it is found that the drive is an unconsumed user drive so the system
    *PVD is reinitialized through a job and after the job finished, the database would connect the PVD and drive and
    *persist new descriptors and signatures. However, the process of drive in user slot 7 may happed before the job
    *finished or after the job finished. In before finished case, the drive is determined as system drive and would not
    *be allowed online; in after finished case, it is not determined as system drive and then becomes online.
    *In order to avoid such uncertainty, we insert system drive 0_0_7 after system powerup and the user drive has
    *been promoted to a system drive*/
    /*sabbath_reinsert_drive(0, 0, 7, drive_handler_1_spa, drive_handler_1_spb);*/
    sabbath_reinsert_drive(0, 0, 1, drive_handler_7_spa, drive_handler_7_spb);
 
    mut_printf(MUT_LOG_TEST_STATUS, "Poweron system");
    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);

    fbe_api_sleep(10000); /*wait for database to process drives*/

    /*check connection status*/
    downstream_connect_state = sabbath_check_pvd_downstream_connection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 1, 0, 0, 1, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, downstream_connect_state, "sabbath_test_case_23: checked 0_0_1 PVD not connects to a drive, which is not expected");
    wait_lifecycle = sabbath_wait_for_sep_object_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 1,
                                                                            FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait system 0_0_1 PVD READY");
    /*0_0_7 PVD should be destroyed*/
    status = fbe_api_wait_till_object_is_destroyed(pvd_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_test_case_13: user 0_0_7 PVD is not destroyed. Not Expected.");

    sabbath_reinsert_drive(0, 0, 7, drive_handler_1_spa, drive_handler_1_spb);
    fbe_api_sleep(5000);/*wait for database to process*/
    drive_upstream_connect_state = sabbath_check_drive_upstream_connection(0, 0, 7, &pvd_id, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, drive_upstream_connect_state, "sabbath_test_case_23: drive 0_0_7 is not connect to an upstream PVD, which is not expected");
    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id,
                                                                            FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait system 0_0_1 PVD READY");
       
    /* do not need to restore the array disk order as the user drive has been promoted to a system drive and the original
    * db drive is downgraded to a user drive now*/
    mut_printf(MUT_LOG_TEST_STATUS, "Reboot system now.");
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    mut_printf(MUT_LOG_TEST_STATUS, "***Case j. Array offline move a user drive to DB slot end.");
    mut_printf(MUT_LOG_TEST_STATUS, "***Case k. Array offline move a DB drive to user slot end.\n");
    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);

    /*how about insert a consumed user drive to the db slot?*/
    status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, 5, &pvd_id);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_test_case_23: fail to get the 0_0_5's PVD obj id by location");
    mut_printf(MUT_LOG_TEST_STATUS, "****Case j-1. Array offline move a consumed user drive to DB slot.");
    mut_printf(MUT_LOG_TEST_STATUS, "shutdown system");
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    
    mut_printf(MUT_LOG_TEST_STATUS, "Pull DB drive 0_0_1 and user drive 0_0_5...");
    sabbath_pull_drive(0, 0, 1, &drive_handler_1_spa, &drive_handler_1_spb);
    sabbath_pull_drive(0, 0, 5, &drive_handler_5_spa, &drive_handler_5_spb);
    mut_printf(MUT_LOG_TEST_STATUS, "insert drive 0_0_5 to db slot 0_0_1...");
    fbe_api_sleep(2000);
    sabbath_reinsert_drive(0, 0, 1, drive_handler_5_spa, drive_handler_5_spb);
 
    mut_printf(MUT_LOG_TEST_STATUS, "Poweron system");
    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);
    fbe_api_sleep(5000); /*wait for database to process drives*/

    /*check connection status*/
    downstream_connect_state = sabbath_check_pvd_downstream_connection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 1, 0, 0, 1, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, downstream_connect_state, "sabbath_test_case_23: checked 0_0_1 PVD connects to a drive, which is not expected");
    downstream_connect_state = sabbath_check_pvd_downstream_connection(pvd_id, 0, 0, 1, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, downstream_connect_state, "sabbath_test_case_23: checked 0_0_5 PVD connects to a drive, which is not expected");
    drive_upstream_connect_state = sabbath_check_drive_upstream_connection(0, 0, 1, NULL, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, drive_upstream_connect_state, "sabbath_test_case_23: drive 0_0_1 is connect to an upstream PVD, which is not expected");

    /*restore drive order*/
    mut_printf(MUT_LOG_TEST_STATUS, "restore drive order...");
    sabbath_pull_drive(0, 0, 1, &drive_handler_5_spa, &drive_handler_5_spb);
    fbe_api_sleep(2000);
    sabbath_reinsert_drive(0, 0, 1, drive_handler_1_spa, drive_handler_1_spb);
    sabbath_reinsert_drive(0, 0, 5, drive_handler_5_spa, drive_handler_5_spb);
    fbe_api_sleep(8000);  /*wait for database to process*/

    drive_upstream_connect_state = sabbath_check_drive_upstream_connection(0, 0, 1, &pvd_id2, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, drive_upstream_connect_state, "sabbath_test_case_23: drive 0_0_1 is not connect to an upstream PVD, which is not expected");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 1, pvd_id2, "sabbath_test_case_23: drive 0_0_1 is not connect to system PVD1, which is not expected");
    drive_upstream_connect_state = sabbath_check_drive_upstream_connection(0, 0, 5, &pvd_id2, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, drive_upstream_connect_state, "sabbath_test_case_23: drive 0_0_5 is not connect to an upstream PVD, which is not expected");
    MUT_ASSERT_INT_EQUAL_MSG(pvd_id, pvd_id2, "sabbath_test_case_23: drive 0_0_5 is not connect to its original PVD, which is not expected");

    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id2,
                                                                            FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait 0_0_5's PVD READY");
    wait_lifecycle = sabbath_wait_for_sep_object_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 1,
                                                                            FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait 0_0_1's PVD READY");
    mut_printf(MUT_LOG_TEST_STATUS, "****Case j-1. Array offline move a consumed user drive to DB slot END.");    
    
}

/*Test:
  *1. Shutdowns system
  *2. Remove unconsumed drive 0_0_7 and system drive 0_0_1
  *3. Insert drive 0_0_7 to system slot 1
  *4. Poweron system and check database is READY
  *5. Check user drive 0_0_7 has been promoted to a system drive
  *6. Insert the original system drive 0_0_1 into user location 0_0_7
  *7. Check the inserted drive becomes online and is a normal user drive
  *8. Shutdowns sytem
  *9. Remove consumed drive 0_0_5 and system drive 0_0_1
  *10. Insert drive 0_0_5 to system slot 1
  *11. Poweron system and check database is READY
  *12. Check user drive 0_0_5 is not connected to the system PVD
  *13. Restore the system
  */
static void sabbath_test_case_24_array_offline_move_a_user_drive_to_another_user_slot(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t pvd_id_5;
    fbe_object_id_t pvd_id_7;
    fbe_bool_t  wait_lifecycle;
    fbe_bool_t  downstream_connect_state;
    fbe_api_terminator_device_handle_t drive_handler_7_spa;
    fbe_api_terminator_device_handle_t drive_handler_7_spb;
    fbe_api_terminator_device_handle_t drive_handler_5_spa;
    fbe_api_terminator_device_handle_t drive_handler_5_spb;

    status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, 5, &pvd_id_5);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_test_case_24: fail to get pvd object id of drive 0_0_5");
    status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, 7, &pvd_id_7);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_test_case_24: fail to get pvd object id of drive 0_0_7");

    /* case L. Move a user drive to another user slot */
    mut_printf(MUT_LOG_TEST_STATUS, "****Case L. Array offline move a user drive to another user slot.\n");
    mut_printf(MUT_LOG_TEST_STATUS, "Shutdown system");
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    mut_printf(MUT_LOG_TEST_STATUS, "Pull consumed user drive 0_0_5 and unconsumed user drive 0_0_7");
    sabbath_pull_drive(0, 0, 7, &drive_handler_7_spa, &drive_handler_7_spb);
    sabbath_pull_drive(0, 0, 5, &drive_handler_5_spa, &drive_handler_5_spb);
    fbe_api_sleep(2000);   

    mut_printf(MUT_LOG_TEST_STATUS, "swap the two drives...");
    sabbath_reinsert_drive(0, 0, 7, drive_handler_5_spa, drive_handler_5_spb);
    sabbath_reinsert_drive(0, 0, 5, drive_handler_7_spa, drive_handler_7_spb);

    mut_printf(MUT_LOG_TEST_STATUS, "poweron system.");
    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);
    fbe_api_sleep(10000);  /*wait for database to process physical drives*/

    /*now check the status of each drive*/
    downstream_connect_state = sabbath_check_pvd_downstream_connection(pvd_id_5, 0, 0, 7, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, downstream_connect_state, "sabbath_test_case_24: checked 0_0_5 PVD not connects to a drive, which is not expected");
    downstream_connect_state = sabbath_check_pvd_downstream_connection(pvd_id_7, 0, 0, 5, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, downstream_connect_state, "sabbath_test_case_24: checked 0_0_5 PVD not connects to a drive, which is not expected");
    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id_5,
                                                                            FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait 0_0_5's PVD READY");
    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id_7,
                                                                            FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait 0_0_7's PVD READY");

    /*restore the original disk order*/
    sabbath_pull_drive(0, 0, 7, &drive_handler_7_spa, &drive_handler_7_spb);
    sabbath_pull_drive(0, 0, 5, &drive_handler_5_spa, &drive_handler_5_spb);
    fbe_api_sleep(2000);   
    sabbath_reinsert_drive(0, 0, 7, drive_handler_5_spa, drive_handler_5_spb);
    sabbath_reinsert_drive(0, 0, 5, drive_handler_7_spa, drive_handler_7_spb);

    fbe_api_sleep(8000);  /*wait for database to process physical drives*/
    downstream_connect_state = sabbath_check_pvd_downstream_connection(pvd_id_5, 0, 0, 5, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, downstream_connect_state, "sabbath_test_case_24: checked 0_0_5 PVD not connects to a drive, which is not expected");
    downstream_connect_state = sabbath_check_pvd_downstream_connection(pvd_id_7, 0, 0, 7, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, downstream_connect_state, "sabbath_test_case_24: checked 0_0_5 PVD not connects to a drive, which is not expected");
    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id_5,
                                                                            FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait 0_0_5's PVD READY");
    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id_7,
                                                                            FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait 0_0_7's PVD READY");
    
    mut_printf(MUT_LOG_TEST_STATUS, "****case L. Array offline move a user drive to another user slot end.\n");

}

/*Test:
  *1. Shutdowns system
  *2. Remove system drive 0_0_1 and drive 0_0_2
  *3. Swap the two system drives
  *4. Poweron system and check database is in service mode
  *5. Check system PVD 1 and 2 does not have downstream edges
  *7. Shutdown sytem and restore the original system state
  *8. Poweron system and check database is READY
  *9. Check system PVD 1 and 2 have downstream edges and connnect to the right drives
  */
static void sabbath_test_case_25_array_offline_move_a_db_drive_to_another_db_slot(void)
{
    fbe_bool_t  downstream_connect_state;
    fbe_api_terminator_device_handle_t drive_handler_1_spa;
    fbe_api_terminator_device_handle_t drive_handler_1_spb;
    fbe_api_terminator_device_handle_t drive_handler_2_spa;
    fbe_api_terminator_device_handle_t drive_handler_2_spb;


    /* case m. Move a DB drive to another DB slot */
    mut_printf(MUT_LOG_TEST_STATUS, "****Case m. Array offline move a DB drive to another DB slot.\n");
    mut_printf(MUT_LOG_TEST_STATUS, "Shutdown system");
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    mut_printf(MUT_LOG_TEST_STATUS, "Pull system drives 0_0_1 and 0_0_2");
    sabbath_pull_drive(0, 0, 1, &drive_handler_1_spa, &drive_handler_1_spb);
    sabbath_pull_drive(0, 0, 2, &drive_handler_2_spa, &drive_handler_2_spb);
    fbe_api_sleep(2000);

    mut_printf(MUT_LOG_TEST_STATUS, "swap the two drives...");
    sabbath_reinsert_drive(0, 0, 1, drive_handler_2_spa, drive_handler_2_spb);
    sabbath_reinsert_drive(0, 0, 2, drive_handler_1_spa, drive_handler_1_spb);

    mut_printf(MUT_LOG_TEST_STATUS, "poweron system.");
    
    /* Homewrecker will meet system drives disorder situation when arrya get rebooted,
      * then Homewrecker would let database enter service mode and block the SEP on the
      * the boot path, it is by design array behavior.
      */
    sabbath_poweron_systems(FBE_TRUE, FBE_TRUE);
    sabbath_check_database_states(FBE_DATABASE_STATE_SERVICE_MODE, FBE_DATABASE_STATE_SERVICE_MODE);

    /*check connection status*/
    downstream_connect_state = sabbath_check_pvd_downstream_connection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 1, 0, 0, 1, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, downstream_connect_state, "sabbath_test_case_25: checked 0_0_1 PVD connects to a drive, which is not expected");
    downstream_connect_state = sabbath_check_pvd_downstream_connection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 2, 0, 0, 2, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, downstream_connect_state, "sabbath_test_case_25: checked 0_0_2 PVD connects to a drive, which is not expected");


    /* We need to resotore the array disk order to let followed cases could start
      * with clean environment.
      */
    mut_printf(MUT_LOG_TEST_STATUS, "Restore the array disk order. start...");
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    
    sabbath_pull_drive(0, 0, 1, &drive_handler_1_spa, &drive_handler_1_spb);
    sabbath_pull_drive(0, 0, 2, &drive_handler_2_spa, &drive_handler_2_spb);
    fbe_api_sleep(2000);

    mut_printf(MUT_LOG_TEST_STATUS, "swap the two drives...");
    sabbath_reinsert_drive(0, 0, 1, drive_handler_2_spa, drive_handler_2_spb);
    sabbath_reinsert_drive(0, 0, 2, drive_handler_1_spa, drive_handler_1_spb);
    
    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);

    /*check connection status*/
    downstream_connect_state = sabbath_check_pvd_downstream_connection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 1, 0, 0, 1, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, downstream_connect_state, "sabbath_test_case_25: checked 0_0_1 PVD not connects to a drive, which is not expected");
    downstream_connect_state = sabbath_check_pvd_downstream_connection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 2, 0, 0, 2, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, downstream_connect_state, "sabbath_test_case_25: checked 0_0_2 PVD not connects to a drive, which is not expected");
    
    mut_printf(MUT_LOG_TEST_STATUS, "****case m. Array offline move a DB drive to another DB slot END.\n");    
}

/*Test:
  *1. Shutdown system
  *2. Remove system drive 0_0_1and insert a blank new drive into the slot
  *4. Poweron system and check database is in READY mode
  *5. Check system PVD 1 has connected to the newly inserted blank drive
  *6. Wait for system PVD 1 becomes READY state
  */
static void sabbath_test_case_26_array_offline_insert_a_new_drive_into_db_slot(void)
{
    fbe_bool_t  wait_lifecycle;
    fbe_bool_t  downstream_connect_state;
    fbe_api_terminator_device_handle_t drive_handler_1_spa;
    fbe_api_terminator_device_handle_t drive_handler_1_spb;
    fbe_api_terminator_device_handle_t drive_handler_1_new_drive_spa;
    fbe_api_terminator_device_handle_t drive_handler_1_new_drive_spb;

    /* case n. Insert  a new drive into DB slot */
    mut_printf(MUT_LOG_TEST_STATUS, "****Case n. Array offline insert  a new drive into DB slot...\n");
    mut_printf(MUT_LOG_TEST_STATUS, "Shutdown system");    
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    
    mut_printf(MUT_LOG_TEST_STATUS, "Pull DB drive 0_0_1...");    
    sabbath_pull_drive(0, 0, 1, &drive_handler_1_spa, &drive_handler_1_spb);
    fbe_api_sleep(2000);
    mut_printf(MUT_LOG_TEST_STATUS, "Insert  a new drive into DB slot...");
    sabbath_insert_blank_new_drive(0, 0, 1, &drive_handler_1_new_drive_spa, &drive_handler_1_new_drive_spb);
    
    mut_printf(MUT_LOG_TEST_STATUS, "Power on system.");
    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);

    fbe_api_sleep(5000); /*wait for database to process drives*/
    

    /*check PVD state*/
    downstream_connect_state = sabbath_check_pvd_downstream_connection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 1, 0, 0, 1, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, downstream_connect_state, "sabbath_test_case_25: checked 0_0_1 PVD not connects to a drive, which is not expected");
    wait_lifecycle = sabbath_wait_for_sep_object_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 1,
                                                                            FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait 0_0_1's PVD READY");
    
    mut_printf(MUT_LOG_TEST_STATUS, "****case n. Array offline insert  a new drive into DB slot end.\n");    
}

/*Test:
  *1. Shutdown system
  *2. Remove the unconsumed user  drive 0_0_7 and insert a blank new drive into the slot 0_0_7
  *4. Poweron system and check database is in READY mode
  *5. Check the newly inserted blank drive connects to a newly created PVD and the PVD becomes READY state
  */
static void sabbath_test_case_27_array_offline_insert_a_new_drive_into_user_slot(void)
{
    fbe_bool_t  wait_lifecycle;
    fbe_object_id_t pvd_id;
    fbe_bool_t  upstream_connect_state;
    fbe_api_terminator_device_handle_t drive_handler_7_spa;
    fbe_api_terminator_device_handle_t drive_handler_7_spb;
    fbe_api_terminator_device_handle_t drive_handler_7_new_drive_spa;
    fbe_api_terminator_device_handle_t drive_handler_7_new_drive_spb;

    /* case o. Insert  a new drive into user slot */
    mut_printf(MUT_LOG_TEST_STATUS, "****Case o. Array offline insert  a new drive into user slot start.\n");        
    mut_printf(MUT_LOG_TEST_STATUS, "Shutdown system");    
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    
    mut_printf(MUT_LOG_TEST_STATUS, "Pull user drive 0_0_7...");    
    sabbath_pull_drive(0, 0, 7, &drive_handler_7_spa, &drive_handler_7_spb);
    fbe_api_sleep(2000);
    mut_printf(MUT_LOG_TEST_STATUS, "Insert  a new drive into user slot...");
    sabbath_insert_blank_new_drive(0, 0, 7, &drive_handler_7_new_drive_spa, &drive_handler_7_new_drive_spb);
    
    mut_printf(MUT_LOG_TEST_STATUS, "Power on system.");
    
    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);

    /*check PVD state*/
    upstream_connect_state = sabbath_check_drive_upstream_connection(0, 0, 7, &pvd_id, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, upstream_connect_state, "sabbath_test_case_27: drive 0_0_7 is not connected to a PVD, which is not expected");
    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id,
                                                                            FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait 0_0_7's PVD READY");
    
    mut_printf(MUT_LOG_TEST_STATUS, "****Case o. Array offline insert  a new drive into user slot end.\n");
}

 /*Test:
  *1. Shutdown system
  *2. Remove system drive 0_0_1 and insert a blank new drive into the slot
  *3. Remove system drive 0_0_2
  *4. Poweron system and check database is not in SERVICE MODE
  *5. Check the inserted blank drive is not connected to any PVD
  *6. Restore system (shutdown, reinsert 0_0_2 and poweron system)
  */
static void sabbath_test_case_28_array_offline_insert_a_new_drive_into_db_slot_and_remove_one_db_drive(void)
{
    fbe_object_id_t pvd_id;
    fbe_bool_t  upstream_connect_state;
    fbe_api_terminator_device_handle_t drive_handler_1_spa;
    fbe_api_terminator_device_handle_t drive_handler_1_spb;
    fbe_api_terminator_device_handle_t drive_handler_2_spa;
    fbe_api_terminator_device_handle_t drive_handler_2_spb;
    fbe_api_terminator_device_handle_t drive_handler_1_new_drive_spa;
    fbe_api_terminator_device_handle_t drive_handler_1_new_drive_spb;

    /* case n. Insert  a new drive into DB slot */
    mut_printf(MUT_LOG_TEST_STATUS, "****Case n. Array offline insert  a new drive into DB slot and remove another db drive...\n");
    mut_printf(MUT_LOG_TEST_STATUS, "Shutdown system");    
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    
    mut_printf(MUT_LOG_TEST_STATUS, "Pull DB drive 0_0_1 and 0_0_2...");    
    sabbath_pull_drive(0, 0, 1, &drive_handler_1_spa, &drive_handler_1_spb);
    sabbath_pull_drive(0, 0, 2, &drive_handler_2_spa, &drive_handler_2_spb);
    fbe_api_sleep(2000);
    mut_printf(MUT_LOG_TEST_STATUS, "Insert  a new drive into DB slot...");
    sabbath_insert_blank_new_drive(0, 0, 1, &drive_handler_1_new_drive_spa, &drive_handler_1_new_drive_spb);
    
    mut_printf(MUT_LOG_TEST_STATUS, "Power on system.");
    
    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);

    /*check PVD state*/
    upstream_connect_state = sabbath_check_drive_upstream_connection(0, 0, 1, &pvd_id, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, upstream_connect_state, "0_0_1 drive is connected to an upstream PVD, which is not expected.");

    /*now restore system*/
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    sabbath_reinsert_drive(0, 0, 2, drive_handler_2_spa, drive_handler_2_spb);
    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);

    fbe_api_sleep(10000);
    
    upstream_connect_state = sabbath_check_drive_upstream_connection(0, 0, 1, &pvd_id, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, upstream_connect_state, "0_0_1 drive is not connected to an upstream PVD, which is not expected.");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 1, pvd_id, "0_0_1 drive is not connected to system PVD 1, which is not expected.");
    upstream_connect_state = sabbath_check_drive_upstream_connection(0, 0, 2, &pvd_id, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, upstream_connect_state, "0_0_2 drive is not connected to an upstream PVD, which is not expected.");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 2, pvd_id, "0_0_2 drive is not connected to system PVD 2, which is not expected.");

    mut_printf(MUT_LOG_TEST_STATUS, "****case n. Array offline insert  a new drive into DB slot end.\n");    
}


 /*Test:
  *1. Shutdown system
  *2. Remove system drive 0_0_1 
  *3. Remove system drive 0_0_2 to a user slot
  *4. Poweron system and check database is in SERVICE MODE
  *5. Return the system drive 0_0_2
  *6. Restore system (shutdown, reinsert 0_0_2 and poweron system)
  */
static void sabbath_test_case_29_array_offline_remove_a_db_drive_and_remove_another_db_drive_to_user_slot(void)
{
    fbe_api_terminator_device_handle_t drive_handler_1_spa;
    fbe_api_terminator_device_handle_t drive_handler_1_spb;
    fbe_api_terminator_device_handle_t drive_handler_2_spa;
    fbe_api_terminator_device_handle_t drive_handler_2_spb;
    fbe_api_terminator_device_handle_t drive_handler_9_spa;
    fbe_api_terminator_device_handle_t drive_handler_9_spb;


    /* case n. Insert  a new drive into DB slot */
    mut_printf(MUT_LOG_TEST_STATUS, "****Case n. Array offline remove a DB drive and remove another db drive to user slot...\n");
    mut_printf(MUT_LOG_TEST_STATUS, "Shutdown system");    
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    
    mut_printf(MUT_LOG_TEST_STATUS, "Pull DB drive 0_0_1 and 0_0_2... and 0_0_9");    
    sabbath_pull_drive(0, 0, 1, &drive_handler_1_spa, &drive_handler_1_spb);
    sabbath_pull_drive(0, 0, 2, &drive_handler_2_spa, &drive_handler_2_spb);
    sabbath_pull_drive(0, 0, 9, &drive_handler_9_spa, &drive_handler_9_spb);
    sabbath_reinsert_drive(0, 0, 9, drive_handler_2_spa, drive_handler_2_spb);

    fbe_api_sleep(2000);
    
    mut_printf(MUT_LOG_TEST_STATUS, "Power on system.");
    sabbath_poweron_systems(FBE_TRUE, FBE_TRUE);
    
    sabbath_check_database_states(FBE_DATABASE_STATE_SERVICE_MODE, FBE_DATABASE_STATE_SERVICE_MODE);
    

    /*now restore system*/
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    sabbath_pull_drive(0, 0, 9, &drive_handler_2_spa, &drive_handler_2_spb);
    sabbath_reinsert_drive(0, 0, 1, drive_handler_1_spa, drive_handler_1_spb);
    sabbath_reinsert_drive(0, 0, 2, drive_handler_2_spa, drive_handler_2_spb);
    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);
    
    mut_printf(MUT_LOG_TEST_STATUS, "****Case n. Array offline remove a DB drive and remove another db drive to user slot... end.\n");    
}

 /*Test:
  *1. Shutdown system
  *2. Remove system drive 0_0_1 to user slot 0_0_5
  *3. Remove system drive 0_0_2 to user slot 0_0_7
  *4. Poweron system and check database is in SERVICE MODE
  *5. Return the system drive 0_0_1 and 0_0_2
  *6. Restore system (shutdown, reinsert 0_0_2 and poweron system)
  */
static void sabbath_test_case_30_array_offline_remove_two_db_drives_to_user_slot(void)
{
    fbe_api_terminator_device_handle_t drive_handler_1_spa;
    fbe_api_terminator_device_handle_t drive_handler_1_spb;
    fbe_api_terminator_device_handle_t drive_handler_2_spa;
    fbe_api_terminator_device_handle_t drive_handler_2_spb;
    fbe_api_terminator_device_handle_t drive_handler_7_spa;
    fbe_api_terminator_device_handle_t drive_handler_7_spb;
    fbe_api_terminator_device_handle_t drive_handler_5_spa;
    fbe_api_terminator_device_handle_t drive_handler_5_spb;


    /* case n. Insert  a new drive into DB slot */
    mut_printf(MUT_LOG_TEST_STATUS, "****Case n. Array offline remove two DB drives to user slot...\n");
    mut_printf(MUT_LOG_TEST_STATUS, "Shutdown system");    
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    
    mut_printf(MUT_LOG_TEST_STATUS, "Pull DB drive 0_0_1 and 0_0_2... and 0_0_5 and 0_0_7");    
    sabbath_pull_drive(0, 0, 1, &drive_handler_1_spa, &drive_handler_1_spb);
    sabbath_pull_drive(0, 0, 5, &drive_handler_5_spa, &drive_handler_5_spb);
    sabbath_reinsert_drive(0, 0, 5, drive_handler_1_spa, drive_handler_1_spb);

    mut_printf(MUT_LOG_TEST_STATUS, "Pull DB drive 0_0_1 and 0_0_5... and insert drive from 0_0_1 to slot 5 done ");  

    sabbath_pull_drive(0, 0, 2, &drive_handler_2_spa, &drive_handler_2_spb);
    sabbath_pull_drive(0, 0, 7, &drive_handler_7_spa, &drive_handler_7_spb);
    sabbath_reinsert_drive(0, 0, 7, drive_handler_2_spa, drive_handler_2_spb);
    mut_printf(MUT_LOG_TEST_STATUS, "Pull DB drive 0_0_2 and 0_0_7... and insert drive from 0_0_2 to slot 7 done ");  

    fbe_api_sleep(2000);
    
    mut_printf(MUT_LOG_TEST_STATUS, "Power on system.");
    sabbath_poweron_systems(FBE_TRUE, FBE_TRUE);
    
    sabbath_check_database_states(FBE_DATABASE_STATE_SERVICE_MODE, FBE_DATABASE_STATE_SERVICE_MODE);

    /*now restore system*/
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    sabbath_pull_drive(0, 0, 7, &drive_handler_7_spa, &drive_handler_7_spb);
    sabbath_pull_drive(0, 0, 5, &drive_handler_5_spa, &drive_handler_5_spb);
    sabbath_reinsert_drive(0, 0, 1, drive_handler_5_spa, drive_handler_5_spb);
    sabbath_reinsert_drive(0, 0, 2, drive_handler_7_spa, drive_handler_7_spb);
    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "****Case n. Array offline remove two DB drives to user slot... end.\n");    
}

/*Test: This is to verify the fix for AR606032
  *0.  0_0_5 is a consumed drive
  *1.  Shutdown system
  *2.  Insert user drive 0_0_5 to system slot 0_0_2
  *3.  Insert system drive 0_0_2 to user slot 0_0_5
  *4.  Poweron system and check 0_0_2's pvd and 0_0_5's pvd is not connected to downstream
  *5.  Check 0_0_2's pvd and 0_0_5's pvd is stuck in SPECIALIZE
  *6.  Restore drive 0_0_2 and 0_0_5 online
  *7.  Check 0_0_2 and 0_0_5 has upstream PVD 
  *8.  Check 0_0_2's pvd and 0_0_5's pvd is READY 
  *9.  Reboot system
  *10. Check 0_0_2 and 0_0_5 has upstream PVD 
  *11. Check 0_0_2's pvd and 0_0_5's pvd is READY 
  *Created 12/12/2013 by Song, Yingying
  */
void sabbath_test_move_drive_when_array_is_offline_test3(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t pvd_id;
    fbe_bool_t  wait_lifecycle;
    fbe_bool_t  downstream_connect_state;
    fbe_bool_t  drive_upstream_connect_state;
    fbe_api_terminator_device_handle_t drive_handler_2_spa;
    fbe_api_terminator_device_handle_t drive_handler_2_spb;
    fbe_api_terminator_device_handle_t drive_handler_5_spa;
    fbe_api_terminator_device_handle_t drive_handler_5_spb;
    fbe_object_id_t pvd_id2;

    fbe_database_state_t state = FBE_DATABASE_STATE_INVALID;
    fbe_job_service_error_type_t    job_error_code;
    fbe_status_t    job_status;

    /* Wait for database ready */
    status = fbe_test_sep_util_wait_for_database_service(SABBATH_TEST_WAIT_MSEC);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_database_get_state(&state);
    if(status == FBE_STATUS_OK && state != FBE_DATABASE_STATE_READY)
        status = FBE_STATUS_GENERIC_FAILURE;
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Start sabbath_test_move_drive_when_array_is_offline3...\n");
    mut_printf(MUT_LOG_TEST_STATUS, "Start array offline swap a consumed user drive with a db drive...\n");

    /* Wait for database ready */
    sabbath_check_database_states(FBE_DATABASE_STATE_READY, FBE_DATABASE_STATE_READY);

    /*create RGs for following test*/
    status = fbe_api_job_service_raid_group_create(&raid_group_create_cmd);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to create raid group");
    status = fbe_api_common_wait_for_job(raid_group_create_cmd.job_number,
                                         SABBATH_TEST_WAIT_MSEC,
                                         &job_error_code,
                                         &job_status,
										 NULL);
    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to create RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    /*how about swap a consumed user drive with a db drive*/
    status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, 5, &pvd_id);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_swap_test: fail to get the 0_0_5's PVD obj id by location");
    mut_printf(MUT_LOG_TEST_STATUS, "****Case o. Array offline swap a consumed user drive with a db drive");

    /*1. Shutdown system */
    mut_printf(MUT_LOG_TEST_STATUS, "shutdown system");
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);

    /*2. Insert user drive 0_0_5 to system slot 0_0_2 */
    mut_printf(MUT_LOG_TEST_STATUS, "Pull DB drive 0_0_2 and user drive 0_0_5...");
    sabbath_pull_drive(0, 0, 2, &drive_handler_2_spa, &drive_handler_2_spb);
    sabbath_pull_drive(0, 0, 5, &drive_handler_5_spa, &drive_handler_5_spb);
    mut_printf(MUT_LOG_TEST_STATUS, "Insert drive 0_0_5 to db slot 0_0_2...");
    fbe_api_sleep(2000);
    sabbath_reinsert_drive(0, 0, 2, drive_handler_5_spa, drive_handler_5_spb);

    /*3. Insert system drive 0_0_2 to user slot 0_0_5 */
    mut_printf(MUT_LOG_TEST_STATUS, "insert db drive 0_0_2 to user slot 0_0_5...");
    fbe_api_sleep(2000);
    sabbath_reinsert_drive(0, 0, 5, drive_handler_2_spa, drive_handler_2_spb);

    /*4. Poweron system and check 0_0_2's pvd and 0_0_5's pvd is not connected to downstream */
    /*5. Check 0_0_2's pvd and 0_0_5's pvd is stuck in SPECIALIZE */
    mut_printf(MUT_LOG_TEST_STATUS, "Poweron system");
    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);
    fbe_api_sleep(10000); /*wait for database to process drives*/

    /*check connection status*/
    downstream_connect_state = sabbath_check_pvd_downstream_connection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 2, 0, 0, 2, FBE_TRUE, FBE_FALSE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, downstream_connect_state, "sabbath_swap_test: SPA checked 0_0_2 PVD connects to a drive, which is not expected");
    mut_printf(MUT_LOG_TEST_STATUS, "0_0_2's PVD didn't connect on any drive on SPA, this is right.");

    downstream_connect_state = sabbath_check_pvd_downstream_connection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 2, 0, 0, 2, FBE_FALSE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, downstream_connect_state, "sabbath_swap_test: SPB checked 0_0_2 PVD connects to a drive, which is not expected");
    mut_printf(MUT_LOG_TEST_STATUS, "0_0_2's PVD didn't connect on any drive on SPB, this is right.");

    wait_lifecycle = sabbath_wait_for_sep_object_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 2,
                                                       FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, wait_lifecycle, "0_0_2's PVD become READY, which is not expected.");
    mut_printf(MUT_LOG_TEST_STATUS, "0_0_2's PVD stuck in SPECIALIZE, this is right.");

    downstream_connect_state = sabbath_check_pvd_downstream_connection(pvd_id, 0, 0, 5, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, downstream_connect_state, "sabbath_swap_test: checked 0_0_5 PVD connects to a drive, which is not expected");
    mut_printf(MUT_LOG_TEST_STATUS, "0_0_5 PVD didn't connect on any drive on both SPs, this is right.");

    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id,
                                                       FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, wait_lifecycle, "0_0_5's PVD become READY, which is not expected.");
    mut_printf(MUT_LOG_TEST_STATUS, "0_0_5's PVD stuck in SPECIALIZE, this is right.");

    drive_upstream_connect_state = sabbath_check_drive_upstream_connection(0, 0, 2, NULL, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, drive_upstream_connect_state, "sabbath_swap_test: drive 0_0_2 is connect to an upstream PVD, which is not expected");

    /*6. Restore drive 0_0_2 and 0_0_5 online */
    mut_printf(MUT_LOG_TEST_STATUS, "restore drive order...");
    sabbath_pull_drive(0, 0, 2, &drive_handler_5_spa, &drive_handler_5_spb);
    sabbath_pull_drive(0, 0, 5, &drive_handler_2_spa, &drive_handler_2_spb);
    fbe_api_sleep(3000);
    sabbath_reinsert_drive(0, 0, 2, drive_handler_2_spa, drive_handler_2_spb);
    sabbath_reinsert_drive(0, 0, 5, drive_handler_5_spa, drive_handler_5_spb);
    fbe_api_sleep(10000);  /*wait for database to process*/

    /*7. Check 0_0_2 and 0_0_5 has upstream PVD */ 
    drive_upstream_connect_state = sabbath_check_drive_upstream_connection(0, 0, 2, &pvd_id2, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, drive_upstream_connect_state, "sabbath_swap_test: drive 0_0_2 is not connect to an upstream PVD, which is not expected");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 2, pvd_id2, "sabbath_swap_test: drive 0_0_2 is not connect to system PVD0x3, which is not expected");
    drive_upstream_connect_state = sabbath_check_drive_upstream_connection(0, 0, 5, &pvd_id2, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, drive_upstream_connect_state, "sabbath_swap_test: drive 0_0_5 is not connect to an upstream PVD, which is not expected");
    MUT_ASSERT_INT_EQUAL_MSG(pvd_id, pvd_id2, "sabbath_swap_test: drive 0_0_5 is not connect to its original PVD, which is not expected");
    mut_printf(MUT_LOG_TEST_STATUS, "Drive 0_0_2 and 0_0_5 connected to right PVD, this is right.");

    
    /*8. Check 0_0_2's pvd and 0_0_5's pvd is READY */ 
    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id2,
                                                       FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fail to wait 0_0_5's PVD READY");
    wait_lifecycle = sabbath_wait_for_sep_object_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 2,
                                                       FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fail to wait 0_0_2's PVD READY");
    mut_printf(MUT_LOG_TEST_STATUS, "0_0_2'PVD and 0_0_5'PVD are READY now, this is right.");


    /*9. Reboot system */
    mut_printf(MUT_LOG_TEST_STATUS, "Shutdown system");
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    mut_printf(MUT_LOG_TEST_STATUS, "Poweron system");
    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);
    fbe_api_sleep(10000); /*wait for database to process drives*/

    /*10. Check 0_0_2 and 0_0_5 has upstream PVD */ 
    drive_upstream_connect_state = sabbath_check_drive_upstream_connection(0, 0, 2, &pvd_id2, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, drive_upstream_connect_state, "sabbath_swap_test: drive 0_0_2 is not connect to an upstream PVD, which is not expected");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 2, pvd_id2, "sabbath_swap_test: drive 0_0_2 is not connect to system PVD0x3, which is not expected");
    drive_upstream_connect_state = sabbath_check_drive_upstream_connection(0, 0, 5, &pvd_id2, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, drive_upstream_connect_state, "sabbath_swap_test: drive 0_0_5 is not connect to an upstream PVD, which is not expected");
    MUT_ASSERT_INT_EQUAL_MSG(pvd_id, pvd_id2, "sabbath_swap_test: drive 0_0_5 is not connect to its original PVD, which is not expected");
    mut_printf(MUT_LOG_TEST_STATUS, "Drive 0_0_2 and 0_0_5 connected to right PVD, this is right.");

    /*11. Check 0_0_2's pvd and 0_0_5's pvd is READY */ 
    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id2,
                                                       FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fail to wait 0_0_5's PVD READY");
    wait_lifecycle = sabbath_wait_for_sep_object_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 2,
                                                       FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fail to wait 0_0_2's PVD READY");
    mut_printf(MUT_LOG_TEST_STATUS, "0_0_2'PVD and 0_0_5'PVD are READY now, this is right.");

    mut_printf(MUT_LOG_TEST_STATUS, "****Case o. Array offline swap a consumed user drive with a DB drive END.");    

    mut_printf(MUT_LOG_TEST_STATUS, "END sabbath_test_move_drive_when_array_is_offline3...\n");

}

/*Test:
  *1. Shutdowns system
  *2. Remove unconsumed drive 0_0_7 , 0_0_8 and system drive 0_0_0, 0_0_1
  *3. Insert drive 0_0_7 to system slot 0, 0_0_8 to system slot 1
  *4. Poweron system and check database is READY
  *5. Return two drives back to the user slot.
  */
static void sabbath_test_case_31_array_offline_remove_two_user_drives_to_system_slot_different_mirror(void)
{
	fbe_api_terminator_device_handle_t drive_handler_0_spa;
	fbe_api_terminator_device_handle_t drive_handler_0_spb;
	fbe_api_terminator_device_handle_t drive_handler_1_spa;
	fbe_api_terminator_device_handle_t drive_handler_1_spb;
	fbe_api_terminator_device_handle_t drive_handler_7_spa;
	fbe_api_terminator_device_handle_t drive_handler_7_spb;
    fbe_api_terminator_device_handle_t drive_handler_8_spa;
    fbe_api_terminator_device_handle_t drive_handler_8_spb;
	fbe_bool_t	downstream_connect_state;
	fbe_status_t status;

    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    mut_printf(MUT_LOG_TEST_STATUS, "Pull DB drive 0_0_0, 0_0_1 and user drive 0_0_7, 0_0_8...");
    sabbath_pull_drive(0, 0, 0, &drive_handler_0_spa, &drive_handler_0_spb);
    sabbath_pull_drive(0, 0, 1, &drive_handler_1_spa, &drive_handler_1_spb);
    sabbath_pull_drive(0, 0, 7, &drive_handler_7_spa, &drive_handler_7_spb);
    sabbath_pull_drive(0, 0, 8, &drive_handler_8_spa, &drive_handler_8_spb);
    mut_printf(MUT_LOG_TEST_STATUS, "Insert drive 0_0_7 to slot 0_0_0 and insert drive 0_0_8 to db slot 0_0_1...");
    fbe_api_sleep(2000);

    sabbath_reinsert_drive(0, 0, 0, drive_handler_7_spa, drive_handler_7_spb);
    sabbath_reinsert_drive(0, 0, 1, drive_handler_8_spa, drive_handler_8_spb);
 
    mut_printf(MUT_LOG_TEST_STATUS, "Poweron system");
	sabbath_poweron_systems_with_bootflash_mode(FBE_TRUE, FBE_TRUE);

    fbe_api_sleep(10000); /*wait for database to process drives*/

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_test_sep_util_wait_for_database_service(SABBATH_TEST_WAIT_MSEC);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_test_sep_util_wait_for_database_service(SABBATH_TEST_WAIT_MSEC);

    sabbath_check_database_states(FBE_DATABASE_STATE_READY, FBE_DATABASE_STATE_READY);


    /*check connection status*/
	mut_printf(MUT_LOG_TEST_STATUS, "Check drive connection status ...");
    downstream_connect_state = sabbath_check_pvd_downstream_connection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 0, 0, 0, 0, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, downstream_connect_state, " checked 0_0_0 PVD connects to a drive, which is not expected");
    downstream_connect_state = sabbath_check_pvd_downstream_connection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 1, 0, 0, 1, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, downstream_connect_state, " checked 0_0_1 PVD connects to a drive, which is not expected");

	/* restore the system */
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    mut_printf(MUT_LOG_TEST_STATUS, "Pull drive 0_0_7, 0_0_8 from system slot ");
    sabbath_pull_drive(0, 0, 0, &drive_handler_7_spa, &drive_handler_7_spb);
    sabbath_pull_drive(0, 0, 1, &drive_handler_8_spa, &drive_handler_8_spb);

    mut_printf(MUT_LOG_TEST_STATUS, "Insert drive 0_0_7, 0_0_8 back to user slot and 0_0_0, 0_0_1 back to system slot ");
    sabbath_reinsert_drive(0, 0, 0, drive_handler_0_spa, drive_handler_0_spb);
    sabbath_reinsert_drive(0, 0, 1, drive_handler_1_spa, drive_handler_1_spb);

    sabbath_reinsert_drive(0, 0, 7, drive_handler_7_spa, drive_handler_7_spb);
    sabbath_reinsert_drive(0, 0, 8, drive_handler_8_spa, drive_handler_8_spb);

    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);
}

/*Test:
  *1. Shutdowns sytem
  *2. Remove unconsumed drive 0_0_7 , 0_0_8 and system drive 0_0_0, 0_0_2
  *3. Insert drive 0_0_7 to system slot 0, 0_0_8 to system slot 2
  *4. Poweron system and check database is not READY
  *5. Return two drives back to the user slot.
  *6. Restore the system
  */
static void sabbath_test_case_32_array_offline_remove_two_user_drives_to_system_slot_same_mirror(void)
{
	fbe_api_terminator_device_handle_t drive_handler_0_spa;
	fbe_api_terminator_device_handle_t drive_handler_0_spb;
	fbe_api_terminator_device_handle_t drive_handler_2_spa;
	fbe_api_terminator_device_handle_t drive_handler_2_spb;
	fbe_api_terminator_device_handle_t drive_handler_7_spa;
	fbe_api_terminator_device_handle_t drive_handler_7_spb;
    fbe_api_terminator_device_handle_t drive_handler_8_spa;
    fbe_api_terminator_device_handle_t drive_handler_8_spb;
	fbe_bool_t	downstream_connect_state;
	fbe_status_t status;

    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    mut_printf(MUT_LOG_TEST_STATUS, "Pull DB drive 0_0_0, 0_0_2 and user drive 0_0_7, 0_0_8...");
    sabbath_pull_drive(0, 0, 0, &drive_handler_0_spa, &drive_handler_0_spb);
    sabbath_pull_drive(0, 0, 2, &drive_handler_2_spa, &drive_handler_2_spb);
    sabbath_pull_drive(0, 0, 7, &drive_handler_7_spa, &drive_handler_7_spb);
    sabbath_pull_drive(0, 0, 8, &drive_handler_8_spa, &drive_handler_8_spb);
    mut_printf(MUT_LOG_TEST_STATUS, "Insert drive 0_0_7 to slot 0_0_0 and insert drive 0_0_8 to db slot 0_0_2...");
    fbe_api_sleep(2000);

    sabbath_reinsert_drive(0, 0, 0, drive_handler_7_spa, drive_handler_7_spb);
    sabbath_reinsert_drive(0, 0, 2, drive_handler_8_spa, drive_handler_8_spb);
 
    mut_printf(MUT_LOG_TEST_STATUS, "Poweron system");
	sabbath_poweron_systems_with_bootflash_mode(FBE_TRUE, FBE_TRUE);

    fbe_api_sleep(10000); /*wait for database to process drives*/

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_test_sep_util_wait_for_database_service(SABBATH_TEST_WAIT_MSEC);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_test_sep_util_wait_for_database_service(SABBATH_TEST_WAIT_MSEC);

    sabbath_check_database_states(FBE_DATABASE_STATE_BOOTFLASH_SERVICE_MODE, FBE_DATABASE_STATE_BOOTFLASH_SERVICE_MODE);

    /*check connection status*/
	mut_printf(MUT_LOG_TEST_STATUS, "Check drive connection status ...");
    downstream_connect_state = sabbath_check_pvd_downstream_connection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 0, 0, 0, 0, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, downstream_connect_state, " checked 0_0_0 PVD connects to a drive, which is not expected");
    downstream_connect_state = sabbath_check_pvd_downstream_connection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 2, 0, 0, 2, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, downstream_connect_state, " checked 0_0_2 PVD connects to a drive, which is not expected");

	/* restore the system */
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    mut_printf(MUT_LOG_TEST_STATUS, "Pull drive 0_0_7, 0_0_8 from system slot ");
    sabbath_pull_drive(0, 0, 0, &drive_handler_7_spa, &drive_handler_7_spb);
    sabbath_pull_drive(0, 0, 2, &drive_handler_8_spa, &drive_handler_8_spb);

    mut_printf(MUT_LOG_TEST_STATUS, "Insert drive 0_0_7, 0_0_8 back to user slot and 0_0_0, 0_0_2 back to system slot ");
    sabbath_reinsert_drive(0, 0, 0, drive_handler_0_spa, drive_handler_0_spb);
    sabbath_reinsert_drive(0, 0, 2, drive_handler_2_spa, drive_handler_2_spb);

    sabbath_reinsert_drive(0, 0, 7, drive_handler_7_spa, drive_handler_7_spb);
    sabbath_reinsert_drive(0, 0, 8, drive_handler_8_spa, drive_handler_8_spb);

    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);
}

 /*Test:
  *1. Shutdown system
  *2. Remove system drive 0_0_1 and insert a blank new drive into the slot
  *3. Remove system drive 0_0_2
  *4. Wait for a period that the re-init system PVD job for 0_0_1 failed.
  *5. Return system drive 0_0_2
  *6. Wait for 0_0_1 is re-initlized by a new job.
  */
static void sabbath_test_fix_626807_start_new_job_when_reinit_system_pvd_job_fail_because_of_upstream_RG_not_ready(void)
{
    fbe_object_id_t pvd_id;
    fbe_bool_t  upstream_connect_state;
    fbe_api_terminator_device_handle_t drive_handler_1_spa;
    fbe_api_terminator_device_handle_t drive_handler_1_spb;
    fbe_api_terminator_device_handle_t drive_handler_2_spa;
    fbe_api_terminator_device_handle_t drive_handler_2_spb;
    fbe_api_terminator_device_handle_t drive_handler_1_new_drive_spa;
    fbe_api_terminator_device_handle_t drive_handler_1_new_drive_spb;

    /* case n. Insert  a new drive into DB slot */
    mut_printf(MUT_LOG_TEST_STATUS, "****Case n. Array offline insert  a new drive into DB slot and remove another db drive...\n");
    mut_printf(MUT_LOG_TEST_STATUS, "Shutdown system");    
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    
    mut_printf(MUT_LOG_TEST_STATUS, "Pull DB drive 0_0_1 and 0_0_2...");    
    sabbath_pull_drive(0, 0, 1, &drive_handler_1_spa, &drive_handler_1_spb);
    sabbath_pull_drive(0, 0, 2, &drive_handler_2_spa, &drive_handler_2_spb);
    fbe_api_sleep(2000);
    mut_printf(MUT_LOG_TEST_STATUS, "Insert  a new drive into DB slot...");
    sabbath_insert_blank_new_drive(0, 0, 1, &drive_handler_1_new_drive_spa, &drive_handler_1_new_drive_spb);
    
    mut_printf(MUT_LOG_TEST_STATUS, "Power on system.");
    
    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);

    /*check PVD state*/
    upstream_connect_state = sabbath_check_drive_upstream_connection(0, 0, 1, &pvd_id, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, upstream_connect_state, "0_0_1 drive is connected to an upstream PVD, which is not expected.");

    mut_printf(MUT_LOG_TEST_STATUS, "Wait 20 seconds before returning 0_0_2");
    fbe_api_sleep(20000);

    /*now restore system*/
    mut_printf(MUT_LOG_TEST_STATUS, "Return 0_0_2");
    sabbath_reinsert_drive(0, 0, 2, drive_handler_2_spa, drive_handler_2_spb);

    mut_printf(MUT_LOG_TEST_STATUS, "Wait 160 seconds after returning 0_0_2.");
    fbe_api_sleep(160000);
    
    mut_printf(MUT_LOG_TEST_STATUS, "Now check 0_0_1 has connected to upstream PVD.");
    upstream_connect_state = sabbath_check_drive_upstream_connection(0, 0, 1, &pvd_id, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, upstream_connect_state, "0_0_1 drive is not connected to an upstream PVD, which is not expected.");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 1, pvd_id, "0_0_1 drive is not connected to system PVD 1, which is not expected.");
    upstream_connect_state = sabbath_check_drive_upstream_connection(0, 0, 2, &pvd_id, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, upstream_connect_state, "0_0_2 drive is not connected to an upstream PVD, which is not expected.");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 2, pvd_id, "0_0_2 drive is not connected to system PVD 2, which is not expected.");
}

/*Test:
  *1. Shutdown system
  *2. Remove system drive 0_0_0and insert a blank new drive into the slot
  *3. Remove system drive 0_0_2and insert a blank new drive into the slot
  *4. Poweron system and check database state is not ready
  *5. Restore the system
  */
static void sabbath_test_case_35_array_offline_insert_two_new_drives_into_db_slot_same_mirror(void)
{
    fbe_api_terminator_device_handle_t drive_handler_0_spa;
    fbe_api_terminator_device_handle_t drive_handler_0_spb;
    fbe_api_terminator_device_handle_t drive_handler_2_spa;
    fbe_api_terminator_device_handle_t drive_handler_2_spb;
    fbe_api_terminator_device_handle_t drive_handler_0_new_drive_spa;
    fbe_api_terminator_device_handle_t drive_handler_0_new_drive_spb;
    fbe_api_terminator_device_handle_t drive_handler_2_new_drive_spa;
    fbe_api_terminator_device_handle_t drive_handler_2_new_drive_spb;
	fbe_bool_t	downstream_connect_state;
	fbe_status_t status;

    /* case n. Insert  a new drive into DB slot */
    mut_printf(MUT_LOG_TEST_STATUS, "****Case n. Array offline insert two new drives into DB slot...\n");
    mut_printf(MUT_LOG_TEST_STATUS, "Shutdown system");    
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    
    mut_printf(MUT_LOG_TEST_STATUS, "Pull DB drive 0_0_0, 0_0_2...");    
    sabbath_pull_drive(0, 0, 0, &drive_handler_0_spa, &drive_handler_0_spb);
    sabbath_pull_drive(0, 0, 2, &drive_handler_2_spa, &drive_handler_2_spb);
    fbe_api_sleep(2000);
    mut_printf(MUT_LOG_TEST_STATUS, "Insert new drive into DB slot...");
    sabbath_insert_blank_new_drive(0, 0, 0, &drive_handler_0_new_drive_spa, &drive_handler_0_new_drive_spb);
    sabbath_insert_blank_new_drive(0, 0, 2, &drive_handler_2_new_drive_spa, &drive_handler_2_new_drive_spb);
    
    mut_printf(MUT_LOG_TEST_STATUS, "Power on system.");
	sabbath_poweron_systems_with_bootflash_mode(FBE_TRUE, FBE_TRUE);

    fbe_api_sleep(5000); /*wait for database to process drives*/
    
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_test_sep_util_wait_for_database_service(SABBATH_TEST_WAIT_MSEC);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_test_sep_util_wait_for_database_service(SABBATH_TEST_WAIT_MSEC);

    sabbath_check_database_states(FBE_DATABASE_STATE_BOOTFLASH_SERVICE_MODE, FBE_DATABASE_STATE_BOOTFLASH_SERVICE_MODE);

    /*check connection status*/
    mut_printf(MUT_LOG_TEST_STATUS, "Check drive connection status ...");
    downstream_connect_state = sabbath_check_pvd_downstream_connection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 0, 0, 0, 0, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, downstream_connect_state, " checked 0_0_0 PVD connects to a drive, which is not expected");
    downstream_connect_state = sabbath_check_pvd_downstream_connection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 2, 0, 0, 2, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, downstream_connect_state, " checked 0_0_2 PVD connects to a drive, which is not expected");

	/* REstore */
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    sabbath_pull_drive(0, 0, 0, &drive_handler_0_new_drive_spa, &drive_handler_0_new_drive_spb);
    sabbath_pull_drive(0, 0, 2, &drive_handler_2_new_drive_spa, &drive_handler_2_new_drive_spb);

    mut_printf(MUT_LOG_TEST_STATUS, "Insert drive 0_0_0, 0_0_2 back to system slot ");
    sabbath_reinsert_drive(0, 0, 0, drive_handler_0_spa, drive_handler_0_spb);
    sabbath_reinsert_drive(0, 0, 2, drive_handler_2_spa, drive_handler_2_spb);

    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);
}

static void sabbath_test_case_36_array_offline_insert_three_new_drives_into_db_slot(void)
{
    fbe_api_terminator_device_handle_t drive_handler_0_spa;
    fbe_api_terminator_device_handle_t drive_handler_0_spb;
    fbe_api_terminator_device_handle_t drive_handler_1_spa;
    fbe_api_terminator_device_handle_t drive_handler_1_spb;
    fbe_api_terminator_device_handle_t drive_handler_2_spa;
    fbe_api_terminator_device_handle_t drive_handler_2_spb;
    fbe_api_terminator_device_handle_t drive_handler_0_new_drive_spa;
    fbe_api_terminator_device_handle_t drive_handler_0_new_drive_spb;
    fbe_api_terminator_device_handle_t drive_handler_1_new_drive_spa;
    fbe_api_terminator_device_handle_t drive_handler_1_new_drive_spb;
    fbe_api_terminator_device_handle_t drive_handler_2_new_drive_spa;
    fbe_api_terminator_device_handle_t drive_handler_2_new_drive_spb;
	fbe_bool_t	downstream_connect_state;
	fbe_status_t status;

    /* case n. Insert  a new drive into DB slot */
    mut_printf(MUT_LOG_TEST_STATUS, "****Case n. Array offline insert two new drives into DB slot...\n");
    mut_printf(MUT_LOG_TEST_STATUS, "Shutdown system");    
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    
    mut_printf(MUT_LOG_TEST_STATUS, "Pull DB drive 0_0_0, 0_0_1, 0_0_2...");    
    sabbath_pull_drive(0, 0, 0, &drive_handler_0_spa, &drive_handler_0_spb);
    sabbath_pull_drive(0, 0, 1, &drive_handler_1_spa, &drive_handler_1_spb);
    sabbath_pull_drive(0, 0, 2, &drive_handler_2_spa, &drive_handler_2_spb);
    fbe_api_sleep(2000);
    mut_printf(MUT_LOG_TEST_STATUS, "Insert new drive into DB slot...");
    sabbath_insert_blank_new_drive(0, 0, 0, &drive_handler_0_new_drive_spa, &drive_handler_0_new_drive_spb);
    sabbath_insert_blank_new_drive(0, 0, 1, &drive_handler_1_new_drive_spa, &drive_handler_1_new_drive_spb);
    sabbath_insert_blank_new_drive(0, 0, 2, &drive_handler_2_new_drive_spa, &drive_handler_2_new_drive_spb);
    
    mut_printf(MUT_LOG_TEST_STATUS, "Power on system.");
	sabbath_poweron_systems_with_bootflash_mode(FBE_TRUE, FBE_TRUE);

    fbe_api_sleep(5000); /*wait for database to process drives*/
    
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_test_sep_util_wait_for_database_service(SABBATH_TEST_WAIT_MSEC);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_test_sep_util_wait_for_database_service(SABBATH_TEST_WAIT_MSEC);

    sabbath_check_database_states(FBE_DATABASE_STATE_BOOTFLASH_SERVICE_MODE, FBE_DATABASE_STATE_BOOTFLASH_SERVICE_MODE);

    /*check connection status*/
    mut_printf(MUT_LOG_TEST_STATUS, "Check drive connection status ...");
    downstream_connect_state = sabbath_check_pvd_downstream_connection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 0, 0, 0, 0, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, downstream_connect_state, " checked 0_0_0 PVD connects to a drive, which is not expected");
    downstream_connect_state = sabbath_check_pvd_downstream_connection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 1, 0, 0, 1, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, downstream_connect_state, " checked 0_0_1 PVD connects to a drive, which is not expected");
    downstream_connect_state = sabbath_check_pvd_downstream_connection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 2, 0, 0, 2, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, downstream_connect_state, " checked 0_0_2 PVD connects to a drive, which is not expected");

	/* REstore */
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    sabbath_pull_drive(0, 0, 0, &drive_handler_0_new_drive_spa, &drive_handler_0_new_drive_spb);
    sabbath_pull_drive(0, 0, 1, &drive_handler_1_new_drive_spa, &drive_handler_1_new_drive_spb);
    sabbath_pull_drive(0, 0, 2, &drive_handler_2_new_drive_spa, &drive_handler_2_new_drive_spb);

    mut_printf(MUT_LOG_TEST_STATUS, "Insert drive 0_0_0, 0_0_1, 0_0_2 back to system slot ");
    sabbath_reinsert_drive(0, 0, 0, drive_handler_0_spa, drive_handler_0_spb);
    sabbath_reinsert_drive(0, 0, 1, drive_handler_1_spa, drive_handler_1_spb);
    sabbath_reinsert_drive(0, 0, 2, drive_handler_2_spa, drive_handler_2_spb);

    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);
}


 /*1. sabbath_test_case_34_array_offline_remove_two_system_drive_different_mirror
  *2. shutdown both SPs
  *3. Remove system drive 0_0_0, 0_0_1
  *4. Poweron system and check database is not READY
  *5. Return two drives back to the slot.
  *6. Restore the system
  */
static void sabbath_test_case_34_array_offline_remove_two_system_drive_different_mirror(void)
{
    fbe_api_terminator_device_handle_t drive_handler_0_spa;
    fbe_api_terminator_device_handle_t drive_handler_0_spb;
    fbe_api_terminator_device_handle_t drive_handler_1_spa;
    fbe_api_terminator_device_handle_t drive_handler_1_spb;
	fbe_status_t status;

    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    mut_printf(MUT_LOG_TEST_STATUS, "Pull DB drive 0_0_0, 0_0_1 ..");
    sabbath_pull_drive(0, 0, 0, &drive_handler_0_spa, &drive_handler_0_spb);
    sabbath_pull_drive(0, 0, 1, &drive_handler_1_spa, &drive_handler_1_spb);
    fbe_api_sleep(2000);


    mut_printf(MUT_LOG_TEST_STATUS, "Poweron system");
	sabbath_poweron_systems_with_bootflash_mode(FBE_TRUE, FBE_TRUE);

    fbe_api_sleep(10000); /*wait for database to process drives*/

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_test_sep_util_wait_for_database_service(SABBATH_TEST_WAIT_MSEC);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_test_sep_util_wait_for_database_service(SABBATH_TEST_WAIT_MSEC);

    sabbath_check_database_states(FBE_DATABASE_STATE_READY, FBE_DATABASE_STATE_READY);
    /* restore the system */
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "Insert drive 0_0_0, 0_0_1 back to system slot ");
    sabbath_reinsert_drive(0, 0, 0, drive_handler_0_spa, drive_handler_0_spb);
    sabbath_reinsert_drive(0, 0, 1, drive_handler_1_spa, drive_handler_1_spb);


    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);
}

 /*1. sabbath_test_case_33_array_offline_remove_two_system_drive_same_mirror
  *2. shutdowns sytem
  *3. Remove system drive 0_0_0, 0_0_2
  *4. Poweron system and check database is not READY
  *5. Return two drives back to the slot.
  *6. Restore the system
  */
static void sabbath_test_case_33_array_offline_remove_two_system_drive_same_mirror(void)
{
    fbe_api_terminator_device_handle_t drive_handler_0_spa;
    fbe_api_terminator_device_handle_t drive_handler_0_spb;
    fbe_api_terminator_device_handle_t drive_handler_2_spa;
    fbe_api_terminator_device_handle_t drive_handler_2_spb;
	fbe_status_t status;

    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    mut_printf(MUT_LOG_TEST_STATUS, "Pull DB drive 0_0_0, 0_0_2 ..");
    sabbath_pull_drive(0, 0, 0, &drive_handler_0_spa, &drive_handler_0_spb);
    sabbath_pull_drive(0, 0, 2, &drive_handler_2_spa, &drive_handler_2_spb);
    fbe_api_sleep(2000);


    mut_printf(MUT_LOG_TEST_STATUS, "Poweron system");
	sabbath_poweron_systems_with_bootflash_mode(FBE_TRUE, FBE_TRUE);

    fbe_api_sleep(10000); /*wait for database to process drives*/
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_test_sep_util_wait_for_database_service(SABBATH_TEST_WAIT_MSEC);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_test_sep_util_wait_for_database_service(SABBATH_TEST_WAIT_MSEC);

    sabbath_check_database_states(FBE_DATABASE_STATE_BOOTFLASH_SERVICE_MODE, FBE_DATABASE_STATE_BOOTFLASH_SERVICE_MODE);

    /* restore the system */
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "Insert drive 0_0_0, 0_0_2 back to system slot ");
    sabbath_reinsert_drive(0, 0, 0, drive_handler_0_spa, drive_handler_0_spb);
    sabbath_reinsert_drive(0, 0, 2, drive_handler_2_spa, drive_handler_2_spb);


    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);
}


/*Test:
  *1. Remove system drive 0_0_0
  *2. Insert a blank new drive which has smaller size into 0_0_0 location
  *3. Check the system PVD 0 NOT connects to this new drive and its state remains FAIL
  *4. Remove system drive 0_0_0
  *5. Insert a blank new drive which has 1.5*size into 0_0_0 location
  *6. Check the system PVD 0 connects to this new drive and its state becomes READY
  *7. Resume system
  *
  *   02/28/2013 Created. Zhipeng Hu
  */
static void sabbath_test_drive_replace_capacity_policy_test_online(void)
{
    fbe_bool_t  downstream_connect_state;
    fbe_bool_t  has_event;
    fbe_bool_t  wait_lifecycle;
    fbe_object_id_t pvd_id;
    fbe_status_t    status;
    fbe_api_terminator_device_handle_t drive_handler_spa;
    fbe_api_terminator_device_handle_t drive_handler_spb;
    fbe_api_terminator_device_handle_t drive_handler_new_drive_spa;
    fbe_api_terminator_device_handle_t drive_handler_new_drive_spb;
    

    mut_printf(MUT_LOG_TEST_STATUS, "(1) Pull system drive 0_0_0.\n");    
    sabbath_pull_drive(0, 0, 0, &drive_handler_spa, &drive_handler_spb);
    fbe_api_sleep(2000);
    
    mut_printf(MUT_LOG_TEST_STATUS, "(2) Insert a new SAS drive with smaller capacity into DB slot...");
    MUT_ASSERT_TRUE(TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY < SABBATH_TEST_DRIVE_CAPACITY);
    sabbath_insert_blank_new_drive_extended(0, 0, 0, 
                             TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY,
                             FBE_SAS_DRIVE_SIM_520,
                             520,
                             &drive_handler_new_drive_spa, &drive_handler_new_drive_spb);
    fbe_api_sleep(10000); /*wait for database to process*/

    /*check that the new drive is not connected*/
    downstream_connect_state = sabbath_check_pvd_downstream_connection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST, 0, 0, 0, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, downstream_connect_state, "drive_replace_capacity_policy_test_online: 0_0_0 PVD connects to a drive, which is not expected");

    /*check that there is event log for this incompatible drive*/
    status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_SEP_0, &has_event, 
                    SEP_ERROR_CODE_INCOMPATIBLE_REPLACED_SYSTEM_DRIVE, 0, 0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, has_event, "drive_replace_capacity_policy_test_online: NOT found the expected event msg");
    fbe_api_clear_event_log(FBE_PACKAGE_ID_SEP_0);

    mut_printf(MUT_LOG_TEST_STATUS, "(3) Pull system drive 0_0_0.\n");    
    sabbath_pull_drive(0, 0, 0, &drive_handler_new_drive_spa, &drive_handler_new_drive_spb);
    fbe_api_sleep(2000);
    
    mut_printf(MUT_LOG_TEST_STATUS, "(4) Insert a new SAS drive with 1.5*capacity into DB slot..."); 
    sabbath_insert_blank_new_drive_extended(0, 0, 0, 
                             (fbe_lba_t)(SABBATH_TEST_DRIVE_CAPACITY*1.5),
                             FBE_SAS_DRIVE_SIM_520,
                             520,
                             &drive_handler_new_drive_spa, &drive_handler_new_drive_spb);
    fbe_api_sleep(10000); /*wait for database to process*/

    /*check that the new drive is connected*/
    downstream_connect_state = sabbath_check_drive_upstream_connection(0, 0, 0, &pvd_id, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, downstream_connect_state, "0_0_0 drive is not connected to an upstream PVD, which is not expected.");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST, pvd_id, "0_0_0 drive is not connected to system PVD 0, which is not expected.");

    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id,
                                                       FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait 0_0_0's PVD READY");

    /*resume system*/
    mut_printf(MUT_LOG_TEST_STATUS, "(5) Pull the new system drive 0_0_0.\n");
    sabbath_pull_drive(0, 0, 0, &drive_handler_new_drive_spa, &drive_handler_new_drive_spb);
    fbe_api_sleep(2000);
    
    mut_printf(MUT_LOG_TEST_STATUS, "(6) Reinsert the original system drive 0_0_0...");    
    sabbath_reinsert_drive(0, 0, 0, drive_handler_spa, drive_handler_spb);
    fbe_api_sleep(10000); /*wait for database to process*/

    /*check that the original drive is connected*/
    downstream_connect_state = sabbath_check_drive_upstream_connection(0, 0, 0, &pvd_id, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, downstream_connect_state, "0_0_0 drive is not connected to an upstream PVD, which is not expected.");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST, pvd_id, "0_0_0 drive is not connected to system PVD 0, which is not expected.");
    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id,
                                                       FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait 0_0_0's PVD READY");
}

 /*Test:
  *1. Start a rebuild
  *2. Issue two IOs from each SP, and each will read paged MD due to degraded RG.
  *   So we can generate paged MD IO here.
  *3. On the active SP, delay the paged MD IO
  *4. While the paged MD IO is delaying on the active SP, pull drives from RG so it is broken.
  *   When RG shutdown, the paged MD IO is failed with non-retryable error. 
  *   So active RG sets the clustered activate cond.
  *5. Reinsert the pulled drives
  *6. RG should transition to READY on both SPs
  */
static void sabbath_test_drive_failure_io_handling(void)
{
	fbe_object_id_t	rg_object_id;
	fbe_api_raid_group_get_info_t	rg_info;
	fbe_sim_transport_connection_target_t	active_sp;
	fbe_sim_transport_connection_target_t	passive_sp;
	fbe_metadata_info_t	metadata_info;
	fbe_status_t 	status;
	fbe_api_rdgen_context_t	io_context;
	fbe_api_rdgen_context_t	io_context2;
	fbe_api_logical_error_injection_get_stats_t	obj_error_injected;
	fbe_u32_t		wait_count = 10;
	fbe_api_terminator_device_handle_t	drive_handler_spa_1;
	fbe_api_terminator_device_handle_t	drive_handler_spa_2;
	fbe_api_terminator_device_handle_t	drive_handler_spb_1;
	fbe_api_terminator_device_handle_t	drive_handler_spb_2;
    fbe_api_terminator_device_handle_t drive_handler_spa;
    fbe_api_terminator_device_handle_t drive_handler_spb;
    fbe_api_terminator_device_handle_t drive_handler_new_drive_spa;
    fbe_api_terminator_device_handle_t drive_handler_new_drive_spb;
	fbe_bool_t		wait_lifecycle;
    fbe_u32_t       pos;

	status = fbe_api_database_lookup_raid_group_by_number(raid_group_create_on_system_drive_cmd.raid_group_id, &rg_object_id);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	/*First let's determine which SP object is the active/passive one.*/
    active_sp = fbe_api_sim_transport_get_target_server();
    passive_sp = (active_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    status = fbe_api_base_config_metadata_get_info(rg_object_id, &metadata_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (metadata_info.metadata_element_state == FBE_METADATA_ELEMENT_STATE_PASSIVE)
    {
        active_sp ^= passive_sp;
		passive_sp ^= active_sp;
		active_sp ^= passive_sp;
    }

	/* set target to passive object SP first */
	fbe_api_sim_transport_set_target_server(active_sp);


	/* replace old drive with new drive, so RG will do rebuild. When a new IO comes, paged MD
	 * will be read first.*/
	mut_printf(MUT_LOG_TEST_STATUS, "(0) Replace drive 0_0_0 with a new drive.\n");	  
	sabbath_pull_drive(0, 0, 0, &drive_handler_spa, &drive_handler_spb);
	fbe_api_sleep(2000);

    sabbath_insert_blank_new_drive(0, 0, 0, &drive_handler_new_drive_spa, &drive_handler_new_drive_spb);
	fbe_api_sleep(10000); /*wait PVD becomes online*/

    /* wait until rebuild logging is cleared */
    wait_count = 10;
	while(wait_count-- > 0)
	{
        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        for (pos = 0; pos < rg_info.width; pos++)
        {
            if (rg_info.b_rb_logging[pos])
            {
                fbe_api_sleep(20000);
                continue;
            }
        }
        break;
    }
    MUT_ASSERT_INT_NOT_EQUAL(0, wait_count);

	status = fbe_api_logical_error_injection_disable_records(0, FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS); 
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	
	/*	Enable the error injection service
	 */
	status = fbe_api_logical_error_injection_enable();
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	mut_printf(MUT_LOG_TEST_STATUS, "(1) Inject down IO delay error on the 1st paged metadata block region.\n");
	sabbath_test_inject_error_record(rg_object_id,
									 0xF, /*inject on all drives*/
									 rg_info.paged_metadata_start_lba / rg_info.num_data_disk,    /*lba*/
									 10,    /*block count*/
									 FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_DOWN, /*inject delay error*/
									 0xF,
									 FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	/*add pause hook for active RG in clustered activate condition*/
	mut_printf(MUT_LOG_TEST_STATUS, "(3) Set hook to pause the passive raid group from passing downstream health disabled cond..\n");
	
	status = fbe_api_scheduler_add_debug_hook(rg_object_id,
                                              SCHEDULER_MONITOR_STATE_BASE_CONFIG_CLUSTERED_ACTIVATE,
                                              FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_ENTRY,
                                              0,
                                              0,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);	

	mut_printf(MUT_LOG_TEST_STATUS, "(4) Read the 1st block from active SP and 0x190th block from passive object\n");
    fbe_test_sep_io_setup_fill_rdgen_test_context(&io_context,
                                                  rg_object_id,
                                                  FBE_CLASS_ID_INVALID,
                                                  FBE_RDGEN_OPERATION_WRITE_ONLY,
                                                  FBE_LBA_INVALID, /* use max capacity */
                                                  0x80 /* max blocks.  We override this later with regions.  */);

    io_context.start_io.specification.region_list_length = 1; 
    io_context.start_io.specification.region_list[0].lba = 0;
    io_context.start_io.specification.region_list[0].blocks = 1;
    io_context.start_io.specification.region_list[0].pattern = FBE_RDGEN_PATTERN_LBA_PASS;
    io_context.start_io.specification.region_list[0].pass_count = 1;
    io_context.start_io.specification.region_list[0].sp_id = active_sp == FBE_SIM_SP_A ? FBE_DATA_PATTERN_SP_ID_A:FBE_DATA_PATTERN_SP_ID_B;

    /* Set flag to use regions */
    status = fbe_api_rdgen_io_specification_set_options(&io_context.start_io.specification, FBE_RDGEN_OPTIONS_USE_REGIONS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_test_sep_io_setup_fill_rdgen_test_context(&io_context2,
                                                  rg_object_id,
                                                  FBE_CLASS_ID_INVALID,
                                                  FBE_RDGEN_OPERATION_WRITE_ONLY,
                                                  FBE_LBA_INVALID, /* use max capacity */
                                                  0x80 /* max blocks.  We override this later with regions.  */);

    io_context2.start_io.specification.region_list_length = 1; 
    io_context2.start_io.specification.region_list[0].lba = 0x190;
    io_context2.start_io.specification.region_list[0].blocks = 1;
    io_context2.start_io.specification.region_list[0].pattern = FBE_RDGEN_PATTERN_LBA_PASS;
    io_context2.start_io.specification.region_list[0].pass_count = 1;
    io_context2.start_io.specification.region_list[0].sp_id = passive_sp == FBE_SIM_SP_A ? FBE_DATA_PATTERN_SP_ID_A:FBE_DATA_PATTERN_SP_ID_B;

    /* Set flag to use regions */
    status = fbe_api_rdgen_io_specification_set_options(&io_context2.start_io.specification, FBE_RDGEN_OPTIONS_USE_REGIONS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Send IO from both SPs*/
	fbe_api_sim_transport_set_target_server(active_sp);
    status = fbe_api_rdgen_start_tests(&io_context, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	fbe_api_sim_transport_set_target_server(passive_sp);
    status = fbe_api_rdgen_start_tests(&io_context2, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	fbe_api_sim_transport_set_target_server(active_sp);

    wait_count = 10;
	while(wait_count-- > 0)
	{
		status = fbe_api_logical_error_injection_get_stats(&obj_error_injected);
		MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
		if(obj_error_injected.num_errors_injected > 0)
	    {
	    	fbe_api_sleep(300);
			break;
	    }
		fbe_api_sleep(300);
	}
	MUT_ASSERT_INT_NOT_EQUAL(0, wait_count);

	/*wait for IO complete*/
	fbe_api_sim_transport_set_target_server(passive_sp);
    status = fbe_api_rdgen_wait_for_ios(&io_context2, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	status = fbe_api_rdgen_test_context_destroy(&io_context2);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	fbe_api_sim_transport_set_target_server(active_sp);
	
	mut_printf(MUT_LOG_TEST_STATUS, "(6) Remove the 0th and 1th data drive\n");
    sabbath_pull_drive(0, 0, 2, &drive_handler_spa_1, &drive_handler_spb_1);
	sabbath_pull_drive(0, 0, 3, &drive_handler_spa_2, &drive_handler_spb_2);

    wait_lifecycle = sabbath_wait_for_sep_object_state(rg_object_id,
                                                       FBE_LIFECYCLE_STATE_FAIL, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fail to wait RG FAIL");

	/*wait for IO complete*/
    status = fbe_api_rdgen_wait_for_ios(&io_context, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	status = fbe_api_rdgen_test_context_destroy(&io_context);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	
	/*	Disable the error injection service
	 */
    sabbath_test_disable_error_injection(rg_object_id,
                                         0xF /*inject on all drives*/);
    status = fbe_api_logical_error_injection_disable();
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	status = fbe_api_logical_error_injection_disable_records(0, FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS); 
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	

	mut_printf(MUT_LOG_TEST_STATUS, "(7) Insert drives back\n");
    sabbath_reinsert_drive(0, 0, 2, drive_handler_spa_1, drive_handler_spb_1);
	sabbath_reinsert_drive(0, 0, 3, drive_handler_spa_2, drive_handler_spb_2);

	mut_printf(MUT_LOG_TEST_STATUS, "(8) Wait for the clustered activate cond hook\n");
	status = fbe_test_wait_for_debug_hook(rg_object_id,
                                          SCHEDULER_MONITOR_STATE_BASE_CONFIG_CLUSTERED_ACTIVATE,
                                          FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_ENTRY,
										  SCHEDULER_CHECK_STATES,
										  SCHEDULER_DEBUG_ACTION_PAUSE,
										  0,
                                          0);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	fbe_api_sleep(6000); /*give passive object enough time to run activate lifecycle*/


	mut_printf(MUT_LOG_TEST_STATUS, "(9) Check both SPs' objects goes into READY\n");
	status = fbe_api_scheduler_del_debug_hook(rg_object_id,
											  SCHEDULER_MONITOR_STATE_BASE_CONFIG_CLUSTERED_ACTIVATE,
											  FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_ENTRY,
											  0,
											  0,
											  SCHEDULER_CHECK_STATES,
											  SCHEDULER_DEBUG_ACTION_PAUSE);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	
	
    wait_lifecycle = sabbath_wait_for_sep_object_state(rg_object_id,
                                                       FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_OBJ_READY);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fail to wait RG READY");
	
}

/*Test:
  *1. Remove system drive 0_0_0
  *2. Add hook to delay the passive SP drive connection thread
  *3. Add hook to delay the active PVD metadata element verify
  *4. Insert a blank new drive which has larger size into 0_0_0 location
  *5. Wait for PVD metadata element verify hook trigger.
  *6. Remove the passive SP drive connection thread delay hook
  *7. Remove the PVD metadata element verify hook
  *8. Wait for both PVDs on two SPs to become READY
  *9. Check the PVD paged metadata geometry on both SPs
  *
  *   06/22/2013 Created. Zhipeng Hu
  */
static void sabbath_test_drive_replace_capacity_policy_test_timing(void)
{
    fbe_bool_t  downstream_connect_state;
    fbe_bool_t  wait_lifecycle;
    fbe_object_id_t pvd_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_0;
    fbe_status_t    status;
	fbe_sas_address_t	blank_new_sas_address;
    fbe_api_terminator_device_handle_t drive_handler_spa;
    fbe_api_terminator_device_handle_t drive_handler_spb;
    fbe_api_terminator_device_handle_t drive_handler_new_drive_spa;
    fbe_api_terminator_device_handle_t drive_handler_new_drive_spb;
    

    mut_printf(MUT_LOG_TEST_STATUS, "(1) Pull system drive 0_0_0.\n");    
    sabbath_pull_drive(0, 0, 0, &drive_handler_spa, &drive_handler_spb);
    fbe_api_sleep(2000);

	/*add pause hook for active PVD in metadata element verify condition*/
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
	status = fbe_api_scheduler_add_debug_hook(pvd_id,
                                              SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_EVAL_METADATA_ELEMENT,
                                              FBE_PROVISION_DRIVE_SUBSTATE_EVAL_METADATA_ELEM_NOT_FIRST_CREATE,
                                              0,
                                              0,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    mut_printf(MUT_LOG_TEST_STATUS, "(2) Insert a new SAS drive with larger capacity into DB slot and active side discovers it first...");
    blank_new_sas_address = fbe_test_pp_util_get_unique_sas_address();
    status = fbe_test_pp_util_insert_sas_drive_extend(0, 0, 0, 
                                                      520, (fbe_lba_t)(SABBATH_TEST_DRIVE_CAPACITY*2),
                                                      blank_new_sas_address, FBE_SAS_DRIVE_SIM_520, &drive_handler_new_drive_spa);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_insert_blank_new_drive_extended: fail to insert drive to SPA");

	/*wait for the PVD metadata element verify hook*/
	status = fbe_test_wait_for_debug_hook(pvd_id,
                                              SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_EVAL_METADATA_ELEMENT,
                                              FBE_PROVISION_DRIVE_SUBSTATE_EVAL_METADATA_ELEM_NOT_FIRST_CREATE,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE,
                                              0, 0);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "(3) Insert the new SAS drive into the passive side now...");
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
	status = fbe_test_pp_util_insert_sas_drive_extend(0, 0, 0, 
                                                      520, (fbe_lba_t)(SABBATH_TEST_DRIVE_CAPACITY*2),
                                                      blank_new_sas_address, FBE_SAS_DRIVE_SIM_520, &drive_handler_new_drive_spb);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_insert_blank_new_drive_extended: fail to insert drive to SPB");
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

	fbe_api_sleep(5000); /*give enough time for PVD to run its lifecycle*/

	/*delete the hook in SPEC so that PVD can goes into READY*/
	status = fbe_api_scheduler_del_debug_hook(pvd_id,
                                              SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_EVAL_METADATA_ELEMENT,
                                              FBE_PROVISION_DRIVE_SUBSTATE_EVAL_METADATA_ELEM_NOT_FIRST_CREATE,
                                              0,
                                              0,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id,
                                                       FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait 0_0_0's PVD READY");

    /*check that the new drive is connected*/
    downstream_connect_state = sabbath_check_drive_upstream_connection(0, 0, 0, &pvd_id, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, downstream_connect_state, "0_0_0 drive is not connected to an upstream PVD, which is not expected.");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST, pvd_id, "0_0_0 drive is not connected to system PVD 0, which is not expected.");

	/*********************************************************************************
	 * now we can set breakpoint here and check two PVDs' paged metadata geometry in windbg
	 * currently there is no interface for us to get the in-memory page md geo
	 *********************************************************************************/

    /*resume system*/
    mut_printf(MUT_LOG_TEST_STATUS, "(4) Pull the new system drive 0_0_0.\n");
    sabbath_pull_drive(0, 0, 0, &drive_handler_new_drive_spa, &drive_handler_new_drive_spb);
    fbe_api_sleep(2000);
    
    mut_printf(MUT_LOG_TEST_STATUS, "(5) Reinsert the original system drive 0_0_0...");    
    sabbath_reinsert_drive(0, 0, 0, drive_handler_spa, drive_handler_spb);
    fbe_api_sleep(10000); /*wait for database to process*/

    /*check that the original drive is connected*/
    downstream_connect_state = sabbath_check_drive_upstream_connection(0, 0, 0, &pvd_id, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, downstream_connect_state, "0_0_0 drive is not connected to an upstream PVD, which is not expected.");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST, pvd_id, "0_0_0 drive is not connected to system PVD 0, which is not expected.");
    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id,
                                                       FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait 0_0_0's PVD READY");
}

/*Test:
  *1. Shutdown system and remove system drive 0_0_0
  *2. Insert a blank new drive which has smaller size into 0_0_0 location
  *3. Poweron system and check the system PVD 0 NOT connects to this new drive
  *4. Shutdown system and remove system drive 0_0_0
  *5. Insert a blank new drive which has 1.5*size into 0_0_0 location
  *6. Poweron system and check the system PVD 0 connects to this new drive and its state becomes READY
  *7. Resume system
  *
  *   02/28/2013 Created. Zhipeng Hu
  */
static void sabbath_test_drive_replace_capacity_policy_test_offline(void)
{
    fbe_bool_t  downstream_connect_state;
    fbe_bool_t  has_event;
    fbe_status_t status;
    fbe_bool_t  wait_lifecycle;
    fbe_object_id_t pvd_id;
    fbe_api_terminator_device_handle_t drive_handler_spa;
    fbe_api_terminator_device_handle_t drive_handler_spb;
    fbe_api_terminator_device_handle_t drive_handler_new_drive_spa;
    fbe_api_terminator_device_handle_t drive_handler_new_drive_spb;    

    mut_printf(MUT_LOG_TEST_STATUS, "(1) Pull system drive 0_0_0 offline.\n");
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    sabbath_pull_drive(0, 0, 0, &drive_handler_spa, &drive_handler_spb);
    fbe_api_sleep(2000);
    
    mut_printf(MUT_LOG_TEST_STATUS, "(2) Insert a new SAS drive with smaller capacity into DB slot and poweron system...");
    MUT_ASSERT_TRUE(TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY < SABBATH_TEST_DRIVE_CAPACITY);
    sabbath_insert_blank_new_drive_extended(0, 0, 0, 
                             TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY,
                             FBE_SAS_DRIVE_SIM_520,
                             520,
                             &drive_handler_new_drive_spa, &drive_handler_new_drive_spb);

    /*power on system*/
    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);
    fbe_api_sleep(10000);  /*wait for database to process physical drives*/
    
    /*check that the new drive is not connected*/
    downstream_connect_state = sabbath_check_pvd_downstream_connection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST, 0, 0, 0, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, downstream_connect_state, "drive_replace_capacity_policy_test_online: 0_0_0 PVD connects to a drive, which is not expected");

    /*check that there is event log for this incompatible drive*/
    status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_SEP_0, &has_event, 
                    SEP_ERROR_CODE_INCOMPATIBLE_REPLACED_SYSTEM_DRIVE, 0, 0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, has_event, "drive_replace_capacity_policy_test_online: NOT found the expected event msg");
    fbe_api_clear_event_log(FBE_PACKAGE_ID_SEP_0);

    mut_printf(MUT_LOG_TEST_STATUS, "(3) Pull system drive 0_0_0 offline.\n");
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    sabbath_pull_drive(0, 0, 0, &drive_handler_new_drive_spa, &drive_handler_new_drive_spb);
    fbe_api_sleep(2000);
    
    mut_printf(MUT_LOG_TEST_STATUS, "(4) Insert a new SAS drive with 1.5*capacity into DB slot and then poweron system"); 
    sabbath_insert_blank_new_drive_extended(0, 0, 0, 
                             (fbe_lba_t)(SABBATH_TEST_DRIVE_CAPACITY*1.5),
                             FBE_SAS_DRIVE_SIM_520,
                             520,
                             &drive_handler_new_drive_spa, &drive_handler_new_drive_spb);
    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);
    fbe_api_sleep(10000); /*wait for database to process*/

    /*check that the new drive is connected*/
    downstream_connect_state = sabbath_check_drive_upstream_connection(0, 0, 0, &pvd_id, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, downstream_connect_state, "0_0_0 drive is not connected to an upstream PVD, which is not expected.");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST, pvd_id, "0_0_0 drive is not connected to system PVD 0, which is not expected.");

    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id,
                                                       FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait 0_0_0's PVD READY");

    /*resume system*/
    mut_printf(MUT_LOG_TEST_STATUS, "(5) Pull the new system drive 0_0_0 offline.\n");
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    sabbath_pull_drive(0, 0, 0, &drive_handler_new_drive_spa, &drive_handler_new_drive_spb);
    fbe_api_sleep(2000);
    
    mut_printf(MUT_LOG_TEST_STATUS, "(6) Reinsert the original system drive 0_0_0 and power on system...");    
    sabbath_reinsert_drive(0, 0, 0, drive_handler_spa, drive_handler_spb);
    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);
    fbe_api_sleep(10000); /*wait for database to process*/

    /*check that the original drive is connected*/
    downstream_connect_state = sabbath_check_drive_upstream_connection(0, 0, 0, &pvd_id, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, downstream_connect_state, "0_0_0 drive is not connected to an upstream PVD, which is not expected.");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST, pvd_id, "0_0_0 drive is not connected to system PVD 0, which is not expected.");
    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id,
                                                       FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait 0_0_0's PVD READY");
}

/*Test:
  *1. Remove system drive 0_0_0
  *2. Insert a blank new drive which has different drive type into 0_0_0 location
  *3. Check the system PVD 0 NOT connects to this new drive and its state remains FAIL
  *4. Resume system
  *
  *   03/01/2013 Created. Zhipeng Hu
  */
static void sabbath_test_drive_replace_type_policy_test_online(void)
{
    fbe_bool_t  downstream_connect_state;
    fbe_bool_t  has_event;
    fbe_bool_t  wait_lifecycle;
    fbe_object_id_t pvd_id;
    fbe_status_t    status;
    fbe_api_terminator_device_handle_t drive_handler_spa;
    fbe_api_terminator_device_handle_t drive_handler_spb;
    fbe_api_terminator_device_handle_t drive_handler_new_drive_spa;
    fbe_api_terminator_device_handle_t drive_handler_new_drive_spb;
    

    mut_printf(MUT_LOG_TEST_STATUS, "(1) Pull system drive 0_0_0.\n");    
    sabbath_pull_drive(0, 0, 0, &drive_handler_spa, &drive_handler_spb);
    fbe_api_sleep(2000);
    
    mut_printf(MUT_LOG_TEST_STATUS, "(2) Insert a new SAS FLASH drive into DB slot...");
    sabbath_insert_blank_new_drive_extended(0, 0, 0, 
                                            SABBATH_TEST_DRIVE_CAPACITY,
                                            FBE_SAS_DRIVE_SIM_520_FLASH_HE,
                                            520,
                                            &drive_handler_new_drive_spa, &drive_handler_new_drive_spb);
    fbe_api_sleep(10000); /*wait for database to process*/

    /*check that the new drive is not connected*/
    downstream_connect_state = sabbath_check_pvd_downstream_connection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST, 0, 0, 0, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, downstream_connect_state, "drive_replace_type_policy_test_online: 0_0_0 PVD connects to a drive, which is not expected");

    /*check that there is event log for this incompatible drive*/
    status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_SEP_0, &has_event, 
                    SEP_ERROR_CODE_INCOMPATIBLE_REPLACED_SYSTEM_DRIVE, 0, 0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, has_event, "drive_replace_type_policy_test_online: NOT found the expected event msg");
    fbe_api_clear_event_log(FBE_PACKAGE_ID_SEP_0);
    
    /*resume system*/
    mut_printf(MUT_LOG_TEST_STATUS, "(3) Pull the new system drive 0_0_0.\n");
    sabbath_pull_drive(0, 0, 0, &drive_handler_new_drive_spa, &drive_handler_new_drive_spb);
    fbe_api_sleep(2000);
    
    mut_printf(MUT_LOG_TEST_STATUS, "(4) Reinsert the original system drive 0_0_0...");    
    sabbath_reinsert_drive(0, 0, 0, drive_handler_spa, drive_handler_spb);
    fbe_api_sleep(10000); /*wait for database to process*/

    /*check that the original drive is connected*/
    downstream_connect_state = sabbath_check_drive_upstream_connection(0, 0, 0, &pvd_id, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, downstream_connect_state, "0_0_0 drive is not connected to an upstream PVD, which is not expected.");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST, pvd_id, "0_0_0 drive is not connected to system PVD 0, which is not expected.");
    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id,
                                                       FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait 0_0_0's PVD READY");
}



/*Test:
  *1. Shutdown system and remove system drive 0_0_0
  *2. Insert a blank new drive which has different drive type into 0_0_0 location
  *3. Poweron system and check the system PVD 0 NOT connects to this new drive
  *4. Resume system
  *
  *   03/01/2013 Created. Zhipeng Hu
  */
static void sabbath_test_drive_replace_type_policy_test_offline(void)
{
    fbe_bool_t  downstream_connect_state;
    fbe_bool_t  has_event;
    fbe_status_t status;
    fbe_bool_t  wait_lifecycle;
    fbe_object_id_t pvd_id;
    fbe_api_terminator_device_handle_t drive_handler_spa;
    fbe_api_terminator_device_handle_t drive_handler_spb;
    fbe_api_terminator_device_handle_t drive_handler_new_drive_spa;
    fbe_api_terminator_device_handle_t drive_handler_new_drive_spb;    

    mut_printf(MUT_LOG_TEST_STATUS, "(1) Pull system drive 0_0_0 offline.\n");
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    sabbath_pull_drive(0, 0, 0, &drive_handler_spa, &drive_handler_spb);
    fbe_api_sleep(2000);

    mut_printf(MUT_LOG_TEST_STATUS, "(2) Insert a new SAS FLASH drive into DB slot offline...");
    sabbath_insert_blank_new_drive_extended(0, 0, 0, 
                                            SABBATH_TEST_DRIVE_CAPACITY,
                                            FBE_SAS_DRIVE_SIM_520_FLASH_HE,
                                            520,
                                            &drive_handler_new_drive_spa, &drive_handler_new_drive_spb);

    /*power on system*/
    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);
    fbe_api_sleep(10000);  /*wait for database to process physical drives*/
    
    /*check that the new drive is not connected*/
    downstream_connect_state = sabbath_check_pvd_downstream_connection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST, 0, 0, 0, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, downstream_connect_state, "drive_replace_type_policy_test_offline: 0_0_0 PVD connects to a drive, which is not expected");

    /*check that there is event log for this incompatible drive*/
    status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_SEP_0, &has_event, 
                    SEP_ERROR_CODE_INCOMPATIBLE_REPLACED_SYSTEM_DRIVE, 0, 0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, has_event, "drive_replace_type_policy_test_offline: NOT found the expected event msg");
    fbe_api_clear_event_log(FBE_PACKAGE_ID_SEP_0);

    /*resume system*/
    mut_printf(MUT_LOG_TEST_STATUS, "(3) Pull system drive 0_0_0 offline.\n");
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    sabbath_pull_drive(0, 0, 0, &drive_handler_new_drive_spa, &drive_handler_new_drive_spb);
    fbe_api_sleep(2000);
    
    mut_printf(MUT_LOG_TEST_STATUS, "(4) Reinsert the original system drive 0_0_0 and power on system...");    
    sabbath_reinsert_drive(0, 0, 0, drive_handler_spa, drive_handler_spb);
    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_TRUE);
    fbe_api_sleep(10000); /*wait for database to process*/

    /*check that the original drive is connected*/
    downstream_connect_state = sabbath_check_drive_upstream_connection(0, 0, 0, &pvd_id, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, downstream_connect_state, "0_0_0 drive is not connected to an upstream PVD, which is not expected.");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST, pvd_id, "0_0_0 drive is not connected to system PVD 0, which is not expected.");
    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id,
                                                       FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait 0_0_0's PVD READY");
}


/*Test:
  *1. Modify PVD GC threshold to 0 minute
  *2. Set debug Hook to let 0_0_12 PVD SPEC rotary stuck
  *3. Pull drive 0_0_12, to let the PVD get destroyed
  *3. Reinsert drive 0_0_12, a new PVD gets recreated
  *4. Wait for debug Hook
  *5. Pull drive again, let PVD stuck into downstream_health_not_optimal_cond
  *6. Delete debug Hook
  *6. Reboot Passive SP, to let DB clear PVD's inital and NP load flags.
  *7. Reboot Active SP, to let Passive SP turn into Active, during this NP re-initialization would work
  *8. Check PVD Object state and generation num in Base_config and NP record, whether they are consistent
  *
  *   11/26/2013 Created. Jian
  */
static void broken_Rose_test_case_1_for_AR601723(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_terminator_device_handle_t drive_handle_SPA;
    fbe_api_terminator_device_handle_t drive_handle_SPB;
    fbe_system_time_threshold_info_t            time_threshold;
    fbe_object_id_t pvd_obj_id;
    fbe_lifecycle_state_t object_state = FBE_LIFECYCLE_STATE_SPECIALIZE;

    status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(0, 0, 12, &pvd_obj_id);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Get PVD Object ID failed.");

    mut_printf(MUT_LOG_TEST_STATUS, "*v* Set PVD GC timer to 0 *o*\n");
    time_threshold.time_threshold_in_minutes = 0;
    fbe_api_set_system_time_threshold_info(&time_threshold);

    /* set Hook */
    mut_printf(MUT_LOG_TEST_STATUS, "*v* Set  SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_NON_PAGED_INIT Hook *o*\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_scheduler_add_debug_hook(pvd_obj_id,
                                              SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_NON_PAGED_INIT,
                                              FBE_PROVISION_DRIVE_SUBSTATE_NONPAGED_INIT,
                                              0,
                                              0,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    mut_printf(MUT_LOG_TEST_STATUS, "*v* NP Initialization Test: broken_Rose_test_case_1_for_AR601723 starts *o*\n");
    mut_printf(MUT_LOG_TEST_STATUS, "*v* Pull drive 0_0_12 *o*\n");
    sabbath_pull_drive(0, 0, 12, &drive_handle_SPA, &drive_handle_SPB);
    /* Wait for garbage collection start work to destory the PVD */
    fbe_api_sleep(10000);

    /* Let new PVD corresponding to 0_0_12 gets created
      */
    mut_printf(MUT_LOG_TEST_STATUS, "*v* Reinsert drive 0_0_12 *o*\n");
    sabbath_reinsert_drive(0, 0, 12, drive_handle_SPA, drive_handle_SPB);
    /* Wait for Hook */
    status = fbe_test_wait_for_debug_hook(pvd_obj_id,
                                              SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_NON_PAGED_INIT,
                                              FBE_PROVISION_DRIVE_SUBSTATE_NONPAGED_INIT,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE,
                                              0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "*v* Pull drive 0_0_12 to let PVD stuck into downs_path_nonoptimal *o*\n");
    /* Let PVD stuck into SPEC state by downstream_nonoptimal_cond */
    sabbath_pull_drive(0, 0, 12, &drive_handle_SPA, &drive_handle_SPB);
    
    /* clear Hook to let PVD SPEC rotary continue */
    status = fbe_api_scheduler_del_debug_hook(pvd_obj_id,
                                              SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_NON_PAGED_INIT,
                                              FBE_PROVISION_DRIVE_SUBSTATE_NONPAGED_INIT,
                                              0,
                                              0,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Leave some time to Object to run its rotary */
    fbe_api_sleep(4000);

    mut_printf(MUT_LOG_TEST_STATUS, "*v* Reboot Passive SP *o*\n");
    /* reboot passive SP */
    sabbath_shutdown_systems(FBE_FALSE, FBE_TRUE);
    sabbath_poweron_systems_disable_backgroud_serivce(FBE_FALSE, FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "*v* Reboot Active SP *o*\n");
    /* reboot active SP */
    sabbath_shutdown_systems(FBE_TRUE, FBE_FALSE);
    sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_FALSE);

    mut_printf(MUT_LOG_TEST_STATUS, "*v* Reinset 0_0_12 back *o*\n");
    sabbath_reinsert_drive(0, 0, 12, drive_handle_SPA, drive_handle_SPB);

    fbe_api_sleep(10000);

     mut_printf(MUT_LOG_TEST_STATUS, "*v* Check PVD state *o*\n");
    status = fbe_api_get_object_lifecycle_state(pvd_obj_id, &object_state, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_LIFECYCLE_STATE_READY, object_state);

    mut_printf(MUT_LOG_TEST_STATUS, "*v* NP Initialization Test: broken_Rose_test_case_1_for_AR601723 Passed, end *o*\n");


}

/**
 * Test for system pvd reinit while setting drive fault
 *
 *  1. Remove drive 0_0_3 on acitve sp
 *  2. Set drive fault on 0_0_3 on passive sp
 *  3. remove drive 0_0_3 on passive sp
 *  4. Insert a new drive on 0_0_3
 *  5. Wait this PVD on both SP to be ready
 *
 *  2015-01-26 Created. Jamin Kang
 */
static void dark_archon_test_case_2_for_AR700491(void)
{
    fbe_sim_transport_connection_target_t active_sp, passive_sp;
    fbe_status_t status;
    fbe_u32_t slot = 3;
    fbe_object_id_t pvd_id;
    fbe_object_id_t pdo_id;
    fbe_bool_t  wait_lifecycle;
    fbe_api_terminator_device_handle_t  drive_handle_old, drive_handle_new_SPA, drive_handle_new_SPB;
    extern fbe_status_t sleeping_beauty_wait_for_state(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot,
                                                       fbe_lifecycle_state_t state, fbe_u32_t timeout_ms);

    mut_printf(MUT_LOG_TEST_STATUS, "%s system reinit test.", __FUNCTION__);
    sabbath_check_database_states(FBE_DATABASE_STATE_READY, FBE_DATABASE_STATE_READY);
    sabbath_get_active_sp(&active_sp);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_SIM_SP_A, active_sp, "Active SP isn't SPA");
    passive_sp = FBE_SIM_SP_B;

    mut_printf(MUT_LOG_TEST_STATUS, "  active SP: %s, passive SP: %s",
               fbe_test_common_get_sp_name(active_sp), fbe_test_common_get_sp_name(passive_sp));

    status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, slot, &pvd_id);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "fail to get pvd id by location");

    mut_printf(MUT_LOG_TEST_STATUS, "%s test reinit on PVD %x.", __FUNCTION__, pvd_id);

    mut_printf(MUT_LOG_TEST_STATUS, "  Pull drive on acitve side. Passive SP will become active");
    fbe_api_sim_transport_set_target_server(active_sp);
    status = fbe_test_pp_util_pull_drive(0, 0, slot, &drive_handle_old);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = sleeping_beauty_wait_for_state(0, 0, slot, FBE_LIFECYCLE_STATE_DESTROY, 10000);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_api_sim_transport_set_target_server(passive_sp);
    mut_printf(MUT_LOG_TEST_STATUS, "  Set drive fault");
    status = fbe_api_get_physical_drive_object_id_by_location(0, 0, slot, &pdo_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_api_physical_drive_update_drive_fault(pdo_id, FBE_TRUE);
    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id, FBE_LIFECYCLE_STATE_FAIL,
                                                       FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);

    mut_printf(MUT_LOG_TEST_STATUS, "  Pull drive on passive");
    status = fbe_test_pp_util_pull_drive(0, 0, slot, &drive_handle_old);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_api_sleep(5000);

    mut_printf(MUT_LOG_TEST_STATUS, "  Insert new drive");
    sabbath_insert_blank_new_drive(0, 0, slot, &drive_handle_new_SPA, &drive_handle_new_SPB);

    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id, FBE_LIFECYCLE_STATE_READY,
                                                       FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fail to wait pvd_id READY");
}

/**
 *  Test:
 *  1. Remove drive 0_0_5
 *  2. Reboot Passive
 *  3. Add debug hook to let PVD 0_0_5 on Active stuck in fail rotary
 *  4. Reinsert drive 0_0_5 on slot 0_0_7
 *  5. Wait this PVD on Passive to fail
 *  6. Remove debug hook
 *  7. Wait PVD ready in 30 seconds
 *
 *  2014-05-21 Created. Jamin Kang
 */
static void dark_archon_test_case_1_for_AR642445(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t pvd_id_5;
    fbe_object_id_t pvd_id_7;
    fbe_bool_t  wait_lifecycle;
	fbe_metadata_info_t	metadata_info;
    fbe_sim_transport_connection_target_t active_target, passive_target;
    fbe_api_terminator_device_handle_t drive_handler_5_spa;
    fbe_api_terminator_device_handle_t drive_handler_5_spb;
    fbe_api_terminator_device_handle_t drive_handler_7_spa;
    fbe_api_terminator_device_handle_t drive_handler_7_spb;


    mut_printf(MUT_LOG_TEST_STATUS, "%s start.\n", __FUNCTION__);
    mut_printf(MUT_LOG_TEST_STATUS, "SPA: Wait for database to be READY\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_test_sep_util_wait_for_database_service(60000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "SPB: Wait for database to be READY\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_test_sep_util_wait_for_database_service(60000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, 5, &pvd_id_5);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "fail to get pvd id by location");
    status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, 7, &pvd_id_7);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "fail to get pvd id by location");

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_base_config_metadata_get_info(pvd_id_5, &metadata_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (metadata_info.metadata_element_state == FBE_METADATA_ELEMENT_STATE_PASSIVE) {
        mut_printf(MUT_LOG_TEST_STATUS, "Active target is SPB.\n");
        active_target = FBE_SIM_SP_B;
        passive_target = FBE_SIM_SP_A;
    } else {
        mut_printf(MUT_LOG_TEST_STATUS, "Active target is SPA.\n");
        active_target = FBE_SIM_SP_A;
        passive_target = FBE_SIM_SP_B;
    }

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_DEBUG_HIGH);
    fbe_test_sep_util_set_object_trace_flags(FBE_TRACE_TYPE_OBJECT, pvd_id_5, FBE_PACKAGE_ID_SEP_0,
                                             FBE_TRACE_LEVEL_DEBUG_HIGH);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_DEBUG_HIGH);
    fbe_test_sep_util_set_object_trace_flags(FBE_TRACE_TYPE_OBJECT, pvd_id_5, FBE_PACKAGE_ID_SEP_0,
                                             FBE_TRACE_LEVEL_DEBUG_HIGH);

    fbe_api_sim_transport_set_target_server(active_target);

    mut_printf(MUT_LOG_TEST_STATUS, "Active: Pull consumed user drive 0_0_5 and unconsumed user drive 0_0_7...");
    sabbath_pull_drive(0, 0, 5, &drive_handler_5_spa, &drive_handler_5_spb);
    sabbath_pull_drive(0, 0, 7, &drive_handler_7_spa, &drive_handler_7_spb);

    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id_5, FBE_LIFECYCLE_STATE_FAIL,
                                                       FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait pvd_id_5 FAIL");

    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id_7, FBE_LIFECYCLE_STATE_FAIL,
                                                       FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait pvd_id_5 FAIL");

    mut_printf(MUT_LOG_TEST_STATUS, "Active: Wait hook for object 0x%x to update slot number.\n", pvd_id_5);
	status = fbe_api_scheduler_add_debug_hook(pvd_id_5,
                                              SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_FAIL,
                                              FBE_PROVISION_DRIVE_SUBSTATE_DS_HEALTH_BROKEN_MD_UPDATED,
                                              0,
                                              0,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	/*wait for the PVD broken cond hook*/
	status = fbe_test_wait_for_debug_hook(pvd_id_5,
                                          SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_FAIL,
                                          FBE_PROVISION_DRIVE_SUBSTATE_DS_HEALTH_BROKEN_MD_UPDATED,
                                          SCHEDULER_CHECK_STATES,
                                          SCHEDULER_DEBUG_ACTION_PAUSE,
                                          0, 0);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	status = fbe_api_scheduler_del_debug_hook(pvd_id_5,
                                              SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_FAIL,
                                              FBE_PROVISION_DRIVE_SUBSTATE_DS_HEALTH_BROKEN_MD_UPDATED,
											  0,
											  0,
											  SCHEDULER_CHECK_STATES,
											  SCHEDULER_DEBUG_ACTION_PAUSE);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "Active: Wait update slot number done.\n");

    mut_printf(MUT_LOG_TEST_STATUS, "Active: Wait ds broken hook.\n");
	status = fbe_api_scheduler_add_debug_hook(pvd_id_5,
                                              SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_FAIL,
                                              FBE_PROVISION_DRIVE_SUBSTATE_DS_HEALTH_BROKEN,
                                              0,
                                              0,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	/*wait for the PVD broken cond hook*/
	status = fbe_test_wait_for_debug_hook(pvd_id_5,
                                          SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_FAIL,
                                          FBE_PROVISION_DRIVE_SUBSTATE_DS_HEALTH_BROKEN,
                                          SCHEDULER_CHECK_STATES,
                                          SCHEDULER_DEBUG_ACTION_PAUSE,
                                          0, 0);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "Active: Wait hook done\n");

    /* reboot passive SP */
    mut_printf(MUT_LOG_TEST_STATUS, "Reboot Passive\n");
    if (active_target == FBE_SIM_SP_A) {
        sabbath_shutdown_systems(FBE_FALSE, FBE_TRUE);
        sabbath_poweron_systems_disable_backgroud_serivce(FBE_FALSE, FBE_TRUE);
    } else {
        sabbath_shutdown_systems(FBE_TRUE, FBE_FALSE);
        sabbath_poweron_systems_disable_backgroud_serivce(FBE_TRUE, FBE_FALSE);

    }
    sabbath_check_database_states(FBE_DATABASE_STATE_READY, FBE_DATABASE_STATE_READY);

    fbe_api_sim_transport_set_target_server(passive_target);
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_DEBUG_HIGH);
    /* fbe_test_sep_util_set_object_trace_flags(FBE_TRACE_TYPE_OBJECT, pvd_id_5, FBE_PACKAGE_ID_SEP_0, */
    /*                                          FBE_TRACE_LEVEL_DEBUG_HIGH); */
    mut_printf(MUT_LOG_TEST_STATUS, "Insert consumed user drive 0_0_5 to slot 0_0_7...");
    sabbath_reinsert_drive(0, 0, 7, drive_handler_5_spa, drive_handler_5_spb);

    /* Wait for PVD on Passive to join and fail. */
    mut_printf(MUT_LOG_TEST_STATUS, "Passive: Wait for PVD to fail...");
    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id_5, FBE_LIFECYCLE_STATE_FAIL,
                                                       FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait pvd_id_5 FAIL");

    /* Remove debug hook and wait for pvd ready */
    mut_printf(MUT_LOG_TEST_STATUS, "Active: Remove hook and wait PVD to be READY.\n");
    fbe_api_sim_transport_set_target_server(active_target);
	status = fbe_api_scheduler_del_debug_hook(pvd_id_5,
                                              SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_FAIL,
                                              FBE_PROVISION_DRIVE_SUBSTATE_DS_HEALTH_BROKEN,
											  0,
											  0,
											  SCHEDULER_CHECK_STATES,
											  SCHEDULER_DEBUG_ACTION_PAUSE);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id_5, FBE_LIFECYCLE_STATE_READY,
                                                       FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait pvd_id_5 Ready");

    mut_printf(MUT_LOG_TEST_STATUS, "Passive: Wait PVD to be READY.\n");
    fbe_api_sim_transport_set_target_server(passive_target);
    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id_5, FBE_LIFECYCLE_STATE_READY,
                                                       FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fail to wait pvd_id_5 Ready");

    mut_printf(MUT_LOG_TEST_STATUS, "Reinsert the drives");
    sabbath_pull_drive(0, 0, 7, &drive_handler_5_spa, &drive_handler_5_spb);
    sabbath_reinsert_drive(0, 0, 7, drive_handler_7_spa, drive_handler_7_spb);
    sabbath_reinsert_drive(0, 0, 5, drive_handler_5_spa, drive_handler_5_spb);

    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id_5, FBE_LIFECYCLE_STATE_READY,
                                                       FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fail to wait 0_0_5 PVD READY");
    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id_7, FBE_LIFECYCLE_STATE_READY,
                                                       FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fail to wait 0_0_7 PVD READY");

    mut_printf(MUT_LOG_TEST_STATUS, "%s: PASSED.\n", __FUNCTION__);
}


/*Test:
  1. Modify PVD GC threshold to 0 minute
  2. Set debug Hook to let 0_0_12 PVD SPEC rotary stuck in active SP
  3. Pull drive 0_0_12 on active SP
  3. Wait drive 0_0_12 on Passive SP finishes running activate rotary
  4. Wait PVD to be ready
  5. Check PVD B_E_S information
  2014-07-29 Created. Jamin Kang
**/
static void zealot_for_AR656681(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_terminator_device_handle_t drive_handle_SPA;
    fbe_api_terminator_device_handle_t drive_handle_SPB;
    fbe_system_time_threshold_info_t            time_threshold;
    fbe_object_id_t pvd_obj_id;
    fbe_sim_transport_connection_target_t   active_sp, passive_sp;
    fbe_u32_t slot = 12;
    fbe_api_provision_drive_info_t provision_drive_info;

    active_sp = fbe_api_sim_transport_get_target_server();
    passive_sp = (active_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Active sp: %u", __FUNCTION__, active_sp);
    status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(0, 0, slot, &pvd_obj_id);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Get PVD Object ID failed.");

    mut_printf(MUT_LOG_TEST_STATUS, "    Set PVD GC timer to 0");
    time_threshold.time_threshold_in_minutes = 0;
    fbe_api_set_system_time_threshold_info(&time_threshold);

    /* set Hook */
    mut_printf(MUT_LOG_TEST_STATUS, "    Set  METADATA EVAL Hook");
    fbe_api_sim_transport_set_target_server(active_sp);
    status = fbe_api_scheduler_add_debug_hook(pvd_obj_id,
                                              SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_EVAL_METADATA_ELEMENT,
                                              FBE_PROVISION_DRIVE_SUBSTATE_EVAL_METADATA_ELEM_NOT_FIRST_CREATE,
                                              0,
                                              0,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_api_sim_transport_set_target_server(passive_sp);
    status = fbe_api_scheduler_add_debug_hook(pvd_obj_id,
                                              SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_PASSIVE_REQUEST,
                                              FBE_PROVISION_DRIVE_SUBSTATE_JOIN_STARTED,
                                              0, 0,
                                              SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);

    fbe_api_sim_transport_set_target_server(active_sp);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: starts", __FUNCTION__);
    mut_printf(MUT_LOG_TEST_STATUS, "    Pull drive");
    sabbath_pull_drive(0, 0, slot, &drive_handle_SPA, &drive_handle_SPB);
    /* Wait for garbage collection start work to destory the PVD */
    fbe_api_sleep(10000);

    /* Let new PVD corresponding to be created
      */
    mut_printf(MUT_LOG_TEST_STATUS, "    Reinsert drive");
    sabbath_reinsert_drive(0, 0, slot, drive_handle_SPA, drive_handle_SPB);
    /* Wait for Hook */
    fbe_api_sim_transport_set_target_server(active_sp);
    mut_printf(MUT_LOG_TEST_STATUS, "    Wait active to update metadata");
    status = fbe_test_wait_for_debug_hook(pvd_obj_id,
                                          SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_EVAL_METADATA_ELEMENT,
                                          FBE_PROVISION_DRIVE_SUBSTATE_EVAL_METADATA_ELEM_NOT_FIRST_CREATE,
                                          SCHEDULER_CHECK_STATES,
                                          SCHEDULER_DEBUG_ACTION_PAUSE,
                                          0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "    Wait for passive SP to update location information");
    fbe_api_sim_transport_set_target_server(passive_sp);
    status = fbe_test_wait_for_debug_hook(pvd_obj_id,
                                          SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_PASSIVE_REQUEST,
                                          FBE_PROVISION_DRIVE_SUBSTATE_JOIN_STARTED,
                                          SCHEDULER_CHECK_STATES,
                                          SCHEDULER_DEBUG_ACTION_PAUSE,
                                          0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	status = fbe_api_scheduler_del_debug_hook(pvd_obj_id,
                                              SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_PASSIVE_REQUEST,
                                              FBE_PROVISION_DRIVE_SUBSTATE_JOIN_STARTED,
											  0,
											  0,
											  SCHEDULER_CHECK_STATES,
											  SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "    Pull drive to let PVD stuck into spec in active");
    fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (active_sp == FBE_SIM_SP_A) {
        status = fbe_test_pp_util_pull_drive(0, 0, slot, &drive_handle_SPA);
    } else {
        status = fbe_test_pp_util_pull_drive(0, 0, slot, &drive_handle_SPB);
    }
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_pull_drive: fail to pull drive from activesp");

    fbe_api_sim_transport_set_target_server(active_sp);
	status = fbe_api_scheduler_del_debug_hook(pvd_obj_id,
                                              SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_EVAL_METADATA_ELEMENT,
                                              FBE_PROVISION_DRIVE_SUBSTATE_EVAL_METADATA_ELEM_NOT_FIRST_CREATE,
											  0,
											  0,
											  SCHEDULER_CHECK_STATES,
											  SCHEDULER_DEBUG_ACTION_PAUSE);

    mut_printf(MUT_LOG_TEST_STATUS, "    Wait PVD to be ready on passive side");
    fbe_api_sim_transport_set_target_server(passive_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_wait_for_object_lifecycle_state(pvd_obj_id, FBE_LIFECYCLE_STATE_READY,
                                                     30000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "    reinsert drive on active side");
    fbe_api_sim_transport_set_target_server(active_sp);
    if (active_sp == FBE_SIM_SP_A) {
        status = fbe_test_pp_util_reinsert_drive(0, 0, slot, drive_handle_SPA);
    } else {
        status = fbe_test_pp_util_reinsert_drive(0, 0, slot, drive_handle_SPB);
    }
    status = fbe_api_wait_for_object_lifecycle_state(pvd_obj_id, FBE_LIFECYCLE_STATE_READY,
                                                     30000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_provision_drive_get_info(pvd_obj_id,&provision_drive_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (provision_drive_info.port_number == FBE_PORT_NUMBER_INVALID ||
        provision_drive_info.enclosure_number == FBE_ENCLOSURE_NUMBER_INVALID ||
        provision_drive_info.slot_number == FBE_SLOT_NUMBER_INVALID) {
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Test failed. Invalid BES: %u_%u_%u",
                   __FUNCTION__, provision_drive_info.port_number,
                   provision_drive_info.enclosure_number, provision_drive_info.slot_number);
        MUT_FAIL();
    }

    mut_printf(MUT_LOG_TEST_STATUS, "    Set PVD GC timer to 1800");
    time_threshold.time_threshold_in_minutes = 1800;
    fbe_api_set_system_time_threshold_info(&time_threshold);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Test finished", __FUNCTION__);
}


/*Test:
  1. Pull drive 0_0_13 and 0_0_11 on both SPs
  2. Set debug Hook to let 0_0_13 PVD stucks in activate rotary on active SP
  3. Reinsert 0_0_13 to 0_0_11 on both SPs.
  4. Wait debug to hit on active side and 0_0_11 on passive side to be ready
  5. Verify the B_E_S information

  2014-10-08 Created. Jamin Kang
**/
static void zealot_for_AR670381(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t pvd_id_13, pvd_id_11;
    fbe_sim_transport_connection_target_t   active_sp, passive_sp;
    fbe_api_provision_drive_info_t provision_drive_info;
    fbe_api_terminator_device_handle_t drive_handler_13_spa, drive_handler_13_spb;
    fbe_api_terminator_device_handle_t drive_handler_11_spa, drive_handler_11_spb;
    fbe_scheduler_debug_hook_t active_activate_hook, passive_request_hook;
    fbe_bool_t wait_lifecycle;
	fbe_metadata_info_t	metadata_info;

    mut_printf(MUT_LOG_TEST_STATUS, "%s start.\n", __FUNCTION__);
    mut_printf(MUT_LOG_TEST_STATUS, "SPA: Wait for database to be READY\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_test_sep_util_wait_for_database_service(60000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "SPB: Wait for database to be READY\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_test_sep_util_wait_for_database_service(60000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, 13, &pvd_id_13);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "fail to get pvd id by location");
    status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, 11, &pvd_id_11);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "fail to get pvd id by location");


    /* Initialize Hooks */
    active_activate_hook.object_id = pvd_id_13;
    active_activate_hook.monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE;
    active_activate_hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_DS_HEALTH_DISABLED;
    active_activate_hook.val1 = 0;
    active_activate_hook.val2 = 0;
    active_activate_hook.check_type = SCHEDULER_CHECK_STATES;
    active_activate_hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
    active_activate_hook.counter = 0;

    passive_request_hook.object_id = pvd_id_13;
    passive_request_hook.monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_PASSIVE_REQUEST;
    passive_request_hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_JOIN_STARTED;
    passive_request_hook.val1 = 0;
    passive_request_hook.val2 = 0;
    passive_request_hook.check_type = SCHEDULER_CHECK_STATES;
    passive_request_hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
    passive_request_hook.counter = 0;


    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_base_config_metadata_get_info(pvd_id_13, &metadata_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (metadata_info.metadata_element_state == FBE_METADATA_ELEMENT_STATE_PASSIVE) {
        mut_printf(MUT_LOG_TEST_STATUS, "Active target is SPB.\n");
        active_sp = FBE_SIM_SP_B;
        passive_sp = FBE_SIM_SP_A;
    } else {
        mut_printf(MUT_LOG_TEST_STATUS, "Active target is SPA.\n");
        active_sp = FBE_SIM_SP_A;
        passive_sp = FBE_SIM_SP_B;
    }

    fbe_api_sim_transport_set_target_server(active_sp);
    mut_printf(MUT_LOG_TEST_STATUS, "    Pull drive from 0_0_13");
    sabbath_pull_drive(0, 0, 13, &drive_handler_13_spa, &drive_handler_13_spb);
    mut_printf(MUT_LOG_TEST_STATUS, "    Pull drive from 0_0_11");
    sabbath_pull_drive(0, 0, 11, &drive_handler_11_spa, &drive_handler_11_spb);

    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id_13, FBE_LIFECYCLE_STATE_FAIL,
                                                       FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fail to wait pvd_id_5 FAIL");

    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id_11, FBE_LIFECYCLE_STATE_FAIL,
                                                       FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fail to wait pvd_id_5 FAIL");

    mut_printf(MUT_LOG_TEST_STATUS, "    Set  Hook");
    fbe_api_sim_transport_set_target_server(active_sp);
    status = fbe_test_add_scheduler_hook(&active_activate_hook);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_api_sim_transport_set_target_server(passive_sp);
    status = fbe_test_add_scheduler_hook(&passive_request_hook);


    mut_printf(MUT_LOG_TEST_STATUS, "    Insert drive 0_0_13 to slot 0_0_11");
    sabbath_reinsert_drive(0, 0, 11, drive_handler_13_spa, drive_handler_13_spb);

    /* Wait for Hook */
    fbe_api_sim_transport_set_target_server(active_sp);
    mut_printf(MUT_LOG_TEST_STATUS, "    Wait active to enter activate state");
    status = fbe_test_wait_for_scheduler_hook(&active_activate_hook);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "    Wait for passive SP to update location information");
    fbe_api_sim_transport_set_target_server(passive_sp);
    status = fbe_test_wait_for_scheduler_hook(&passive_request_hook);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_del_scheduler_hook(&passive_request_hook);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "    Pull drive to let PVD on active entered SLF");
    fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (active_sp == FBE_SIM_SP_A) {
        status = fbe_test_pp_util_pull_drive(0, 0, 11, &drive_handler_13_spa);
    } else {
        status = fbe_test_pp_util_pull_drive(0, 0, 11, &drive_handler_13_spb);
    }
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "    fail to pull drive from activesp");

    fbe_api_sim_transport_set_target_server(active_sp);
    status = fbe_test_del_scheduler_hook(&active_activate_hook);

    mut_printf(MUT_LOG_TEST_STATUS, "    Wait PVD to be ready on both SPs");
    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id_13, FBE_LIFECYCLE_STATE_READY,
                                                       FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fail to wait pvd_id_13 ready");

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "    reinsert drive on active side");
    fbe_api_sim_transport_set_target_server(active_sp);
    if (active_sp == FBE_SIM_SP_A) {
        status = fbe_test_pp_util_reinsert_drive(0, 0, 11, drive_handler_13_spa);
    } else {
        status = fbe_test_pp_util_reinsert_drive(0, 0, 11, drive_handler_13_spb);
    }
    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id_13, FBE_LIFECYCLE_STATE_READY,
                                                       FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fail to wait pvd_id_13 ready");

   mut_printf(MUT_LOG_TEST_STATUS, "   Validate B_E_S is 0_0_11");
     status = fbe_api_provision_drive_get_info(pvd_id_13,&provision_drive_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (provision_drive_info.port_number != 0 ||
        provision_drive_info.enclosure_number != 0||
        provision_drive_info.slot_number != 11) {
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Test failed. Invalid BES: %u_%u_%u",
                   __FUNCTION__, provision_drive_info.port_number,
                   provision_drive_info.enclosure_number, provision_drive_info.slot_number);
        MUT_FAIL();
    }
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Test finished", __FUNCTION__);
}


/*!**************************************************************
  * sabbath_test()
  ****************************************************************
  * @brief
  *  
  *
  * @param None.               
  *
  * @return None.
  *
  * @author
  *  12/08/2011 Created. Jian Gao 
  *  07/24/2012 Modify to dualsp test and some system behaviours
  *
  ****************************************************************/
void sabbath_test(void)
{
    /* When the fbesim.exe process is created and loaded into memory,
      * all of the array drives are blank and the invalidate disk flag is set, 
      * equal to situation of a physical array is booted after ICA. So the 
first 
      * Homewrecker test case: The first time system boot after ICA 
      * is tested during the fbesim process populating.
      */
    mut_printf(MUT_LOG_TEST_STATUS, "Start sabbath_test\n");
    mut_printf(MUT_LOG_TEST_STATUS, "*J* Homewrecker Test: sabbath_test_case_1_the_first_time_system_boot_after_ICA *L* \n");

    /* Check database state */
    sabbath_check_database_states(FBE_DATABASE_STATE_READY, FBE_DATABASE_STATE_READY);

    mut_printf(MUT_LOG_TEST_STATUS, "sabbath_test_case_1_the_first_time_system_boot_after_ICA: end\n");
    
    sabbath_test_case_2_normal_case();

    sabbath_test_case_3_remove_single_db_drive();

    sabbath_test_case_4_remove_two_db_drives();
  
    sabbath_test_case_5_unplanned_chassis_replacement();

    mut_printf(MUT_LOG_TEST_STATUS, "End sabbath_test\n");
}
/******************************************
 * end sabbath_test()
 ******************************************/


/*!**************************************************************
  * sabbath_test2()
  ****************************************************************
  * @brief
  *  
  *
  * @param None.               
  *
  * @return None.
  *
  * @author
  *  12/08/2011 Created. Jian Gao 
  *  07/24/2012 Modify to dualsp test and some system behaviours
  *  12/17/2012 Split from sabbath_test to avoid time out
  ****************************************************************/
void sabbath_test2(void)
{
    /* When the fbesim.exe process is created and loaded into memory,
      * all of the array drives are blank and the invalidate disk flag is set, 
      * equal to situation of a physical array is booted after ICA. So the first 
      * Homewrecker test case: The first time system boot after ICA 
      * is tested during the fbesim process populating.
      */
    mut_printf(MUT_LOG_TEST_STATUS, "Start sabbath_test2\n");

    /* Check database state */
    sabbath_check_database_states(FBE_DATABASE_STATE_READY, FBE_DATABASE_STATE_READY);

    sabbath_test_case_6_planned_chassis_replacement();

    sabbath_test_case_7_user_modified_wwn_seed();

    sabbath_test_case_8_set_both_wwn_seed_flag();

    mut_printf(MUT_LOG_TEST_STATUS, "End sabbath_test2\n");
}
/******************************************
 * end sabbath_test2()
 ******************************************/

/*!**************************************************************
  * sabbath_test_move_drive_when_array_is_online()
  ****************************************************************
  * @brief
  *  
  *
  * @param None.               
  *
  * @return None.
  *
  * @author
  *  12/08/2011 Created. Jian Gao 
  *  07/26/2012 Modify to dualsp test and some system behaviours
  *
  ****************************************************************/
void sabbath_test_move_drive_when_array_is_online(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_status_t    job_status;
    fbe_job_service_error_type_t    job_error_code;
    

    mut_printf(MUT_LOG_TEST_STATUS, "Start sabbath_test_move_drive_when_array_is_online...\n");

    /* verify database ready */
    sabbath_check_database_states(FBE_DATABASE_STATE_READY, FBE_DATABASE_STATE_READY);    

    /*create RGs for following test*/
    status = fbe_api_job_service_raid_group_create(&raid_group_create_cmd);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_test_move_drive_when_array_is_online: fail to create raid group");
    status = fbe_api_common_wait_for_job(raid_group_create_cmd.job_number,
                                                                                    SABBATH_TEST_WAIT_MSEC,
                                                                                    &job_error_code,
                                                                                    &job_status,
                                                                                    NULL);
    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to create RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");
    

    sabbath_test_case_11_array_online_reinsert_a_db_drive_to_its_original_slot();

    sabbath_test_case_12_array_online_reinsert_a_user_drive_to_its_original_slot();

    sabbath_test_case_13_array_online_move_a_user_drive_to_db_slot();

    sabbath_test_case_14_array_online_move_a_user_drive_to_another_user_slot();

    sabbath_test_case_15_array_online_move_a_db_drive_to_another_db_slot();

    sabbath_test_case_16_array_online_insert_a_new_drive_into_db_slot();

    sabbath_test_case_17_array_online_insert_a_new_drive_into_user_slot();

    mut_printf(MUT_LOG_TEST_STATUS, "End sabbath_test_move_drive_when_array_is_online.\n");
}

/*!**************************************************************
  * sabbath_test_move_drive_when_array_is_offline_test1()
  ****************************************************************
  * @brief
  *  
  *
  * @param None.               
  *
  * @return None.
  *
  * @author
  *  12/08/2011 Created. Jian Gao 
  *  07/28/2012 Modify to dualsp test and some system behaviours
  *
  ****************************************************************/
void sabbath_test_move_drive_when_array_is_offline_test1(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_database_state_t state = FBE_DATABASE_STATE_INVALID;
    fbe_job_service_error_type_t    job_error_code;
    fbe_status_t    job_status;

    /* Wait for database ready */
    status = fbe_test_sep_util_wait_for_database_service(SABBATH_TEST_WAIT_MSEC);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_database_get_state(&state);
    if(status == FBE_STATUS_OK && state != FBE_DATABASE_STATE_READY)
        status = FBE_STATUS_GENERIC_FAILURE;
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Start sabbath_test_move_drive_when_array_is_offline...\n");

    /* Wait for database ready */
    sabbath_check_database_states(FBE_DATABASE_STATE_READY, FBE_DATABASE_STATE_READY);

    /*create RGs for following test*/
    status = fbe_api_job_service_raid_group_create(&raid_group_create_cmd);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_test_move_drive_when_array_is_online: fail to create raid group");
    status = fbe_api_common_wait_for_job(raid_group_create_cmd.job_number,
                                                                                    SABBATH_TEST_WAIT_MSEC,
                                                                                    &job_error_code,
                                                                                    &job_status,
                                                                                    NULL);
    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to create RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    sabbath_test_case_23_array_offline_move_a_user_drive_to_db_slot();

    sabbath_test_case_24_array_offline_move_a_user_drive_to_another_user_slot();

    sabbath_test_case_25_array_offline_move_a_db_drive_to_another_db_slot();



    mut_printf(MUT_LOG_TEST_STATUS, "End sabbath_test_move_drive_when_array_is_offline.\n");
}

/*!**************************************************************
  * sabbath_test_move_drive_when_array_is_offline_test_bootflash_mode()
  ****************************************************************
  * @brief
  *  
  *
  * @param None.               
  *
  * @return None.
  *
  * @author
  *
  ****************************************************************/
void sabbath_test_move_drive_when_array_is_offline_test_bootflash_mode(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_database_state_t state = FBE_DATABASE_STATE_INVALID;
    fbe_job_service_error_type_t    job_error_code;
    fbe_status_t    job_status;

    /* Wait for database ready */
    status = fbe_test_sep_util_wait_for_database_service(SABBATH_TEST_WAIT_MSEC);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_database_get_state(&state);
    if(status == FBE_STATUS_OK && state != FBE_DATABASE_STATE_READY)
        status = FBE_STATUS_GENERIC_FAILURE;
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Start sabbath_test_move_drive_when_array_is_offline(bootflash mode)...\n");

    /* Wait for database ready */
    sabbath_check_database_states(FBE_DATABASE_STATE_READY, FBE_DATABASE_STATE_READY);

    /*create RGs for following test*/
    status = fbe_api_job_service_raid_group_create(&raid_group_create_cmd);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_test_move_drive_when_array_is_online: fail to create raid group");
    status = fbe_api_common_wait_for_job(raid_group_create_cmd.job_number,
                                                                                    SABBATH_TEST_WAIT_MSEC,
                                                                                    &job_error_code,
                                                                                    &job_status,
                                                                                    NULL);
    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to create RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    /* 31 - 35 are used to test mini-homewrecker cases */
    sabbath_test_case_33_array_offline_remove_two_system_drive_same_mirror();
    sabbath_test_case_34_array_offline_remove_two_system_drive_different_mirror();
    sabbath_test_case_35_array_offline_insert_two_new_drives_into_db_slot_same_mirror();
    sabbath_test_case_31_array_offline_remove_two_user_drives_to_system_slot_different_mirror();
    sabbath_test_case_32_array_offline_remove_two_user_drives_to_system_slot_same_mirror();

    mut_printf(MUT_LOG_TEST_STATUS, "End sabbath_test_move_drive_when_array_is_offline.\n");
}

/*!**************************************************************
  * sabbath_test_move_drive_when_array_is_offline_test2()
  ****************************************************************
  * @brief
  *  
  *
  * @param None.               
  *
  * @return None.
  *
  * @author
  *  12/08/2011 Created. Jian Gao 
  *  07/28/2012 Modify to dualsp test and some system behaviours
  *
  ****************************************************************/
void sabbath_test_move_drive_when_array_is_offline_test2(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_database_state_t state = FBE_DATABASE_STATE_INVALID;
    fbe_job_service_error_type_t    job_error_code;
    fbe_status_t    job_status;

    /* Wait for database ready */
    status = fbe_test_sep_util_wait_for_database_service(SABBATH_TEST_WAIT_MSEC);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_database_get_state(&state);
    if(status == FBE_STATUS_OK && state != FBE_DATABASE_STATE_READY)
        status = FBE_STATUS_GENERIC_FAILURE;
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Start sabbath_test_move_drive_when_array_is_offline2...\n");

    /* Wait for database ready */
    sabbath_check_database_states(FBE_DATABASE_STATE_READY, FBE_DATABASE_STATE_READY);

    /*create RGs for following test*/
    status = fbe_api_job_service_raid_group_create(&raid_group_create_cmd);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "sabbath_test_move_drive_when_array_is_online: fail to create raid group");
    status = fbe_api_common_wait_for_job(raid_group_create_cmd.job_number,
                                                                                    SABBATH_TEST_WAIT_MSEC,
                                                                                    &job_error_code,
                                                                                    &job_status,
                                                                                    NULL);
    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to create RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    sabbath_test_case_26_array_offline_insert_a_new_drive_into_db_slot();

    sabbath_test_case_27_array_offline_insert_a_new_drive_into_user_slot();


    sabbath_test_case_28_array_offline_insert_a_new_drive_into_db_slot_and_remove_one_db_drive();

    sabbath_test_case_29_array_offline_remove_a_db_drive_and_remove_another_db_drive_to_user_slot();


    sabbath_test_case_30_array_offline_remove_two_db_drives_to_user_slot();

    sabbath_test_fix_626807_start_new_job_when_reinit_system_pvd_job_fail_because_of_upstream_RG_not_ready();

    mut_printf(MUT_LOG_TEST_STATUS, "End sabbath_test_move_drive_when_array_is_offline2.\n");
}


/*!**************************************************************
  * sabbath_test_drive_replacement_policy_online_test()
  ****************************************************************
  * @brief
  *  Test the array behavior when a system drive is replaced by
  *  a new drive that does not satisefy the drive 'sparing' policy
  *  when system is online.
  *
  * @param None.               
  *
  * @return None.
  *
  * @author
  *  02/28/2013 Created. Zhipeng Hu
  *
  ****************************************************************/
void sabbath_test_drive_replacement_policy_online_test(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_database_state_t state = FBE_DATABASE_STATE_INVALID;
    fbe_job_service_error_type_t job_error_code;
    fbe_status_t job_status;

    /* Wait for database ready */
    status = fbe_test_sep_util_wait_for_database_service(SABBATH_TEST_WAIT_MSEC);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_database_get_state(&state);
    if(status == FBE_STATUS_OK && state != FBE_DATABASE_STATE_READY)
        status = FBE_STATUS_GENERIC_FAILURE;
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Wait for database ready */
    sabbath_check_database_states(FBE_DATABASE_STATE_READY, FBE_DATABASE_STATE_READY);

    /*create RGs for following test*/
    status = fbe_api_job_service_raid_group_create(&raid_group_create_on_system_drive_cmd);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "drive_replacement_policy_online_test: fail to create raid group");
    status = fbe_api_common_wait_for_job(raid_group_create_on_system_drive_cmd.job_number,
                                         SABBATH_TEST_WAIT_MSEC,
                                         &job_error_code,
                                         &job_status,
                                         NULL);
    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to create RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    mut_printf(MUT_LOG_TEST_STATUS, "======Start online testing size policy======\n");

    sabbath_test_drive_replace_capacity_policy_test_online();

    mut_printf(MUT_LOG_TEST_STATUS, "======End online testing size policy======\n");

    mut_printf(MUT_LOG_TEST_STATUS, "======Start online testing type policy======\n");

    sabbath_test_drive_replace_type_policy_test_online();

    mut_printf(MUT_LOG_TEST_STATUS, "======End online testing type policy======\n");

	sabbath_test_drive_replace_capacity_policy_test_timing();

    mut_printf(MUT_LOG_TEST_STATUS, "======Start paged metadata io failure======\n");

	sabbath_test_drive_failure_io_handling();

    mut_printf(MUT_LOG_TEST_STATUS, "======End paged metadata io failure======\n");

    mut_printf(MUT_LOG_TEST_STATUS, "======End entire sabbath_test_drive_replacement_policy_online_test======\n");
}

/*!**************************************************************
  * sabbath_test_drive_replacement_policy_offline_test()
  ****************************************************************
  * @brief
  *  Test the array behavior when a system drive is replaced by
  *  a new drive that does not satisefy the drive 'sparing' policy
  *  when system is offline.
  *
  * @param None.               
  *
  * @return None.
  *
  * @author
  *  02/28/2013 Created. Zhipeng Hu
  *
  ****************************************************************/
void sabbath_test_drive_replacement_policy_offline_test(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_database_state_t state = FBE_DATABASE_STATE_INVALID;
    fbe_job_service_error_type_t job_error_code;
    fbe_status_t job_status;

    /* Wait for database ready */
    status = fbe_test_sep_util_wait_for_database_service(SABBATH_TEST_WAIT_MSEC);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_database_get_state(&state);
    if(status == FBE_STATUS_OK && state != FBE_DATABASE_STATE_READY)
        status = FBE_STATUS_GENERIC_FAILURE;
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Wait for database ready */
    sabbath_check_database_states(FBE_DATABASE_STATE_READY, FBE_DATABASE_STATE_READY);

    /*create RGs for following test*/
    status = fbe_api_job_service_raid_group_create(&raid_group_create_on_system_drive_cmd);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "drive_replacement_policy_online_test: fail to create raid group");
    status = fbe_api_common_wait_for_job(raid_group_create_on_system_drive_cmd.job_number,
                                         SABBATH_TEST_WAIT_MSEC,
                                         &job_error_code,
                                         &job_status,
                                         NULL);
    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to create RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    mut_printf(MUT_LOG_TEST_STATUS, "======Start offline testing size policy======\n");

    sabbath_test_drive_replace_capacity_policy_test_offline();

    mut_printf(MUT_LOG_TEST_STATUS, "======End offline testing size policy======\n");

    mut_printf(MUT_LOG_TEST_STATUS, "======Start offline testing type policy======\n");

    sabbath_test_drive_replace_type_policy_test_offline();

    mut_printf(MUT_LOG_TEST_STATUS, "======End offline testing type policy======\n");

    mut_printf(MUT_LOG_TEST_STATUS, "======End entire sabbath_test_drive_replacement_policy_offline_test======\n");

}

/*Test:
  *1. Remove system drive 0_0_0
  *2. Insert a blank new drive which has larger size into 0_0_0 location
  *3. Inject hook which will fail the PVD reinit job before persist
  *4. Wait the job failure
  *5. Reboot system
  *6. Check the system PVD 0 connects to this new drive and its state becomes READY
  *
  * 03/20/2013 Created. Zhipeng Hu
  */
static void sabbath_test_drive_replace_error_handling_test_offline(void)
{
    fbe_bool_t  downstream_connect_state;
    fbe_bool_t  wait_lifecycle;
    fbe_object_id_t pvd_id;
    fbe_status_t    status;
    fbe_api_terminator_device_handle_t drive_handler_spa;
    fbe_api_terminator_device_handle_t drive_handler_spb;
    fbe_api_terminator_device_handle_t drive_handler_new_drive_spa;
    fbe_api_terminator_device_handle_t drive_handler_new_drive_spb;
    

    mut_printf(MUT_LOG_TEST_STATUS, "(1) Pull system drive 0_0_0.\n");    
    sabbath_pull_drive(0, 0, 0, &drive_handler_spa, &drive_handler_spb);
    fbe_api_sleep(2000);
    
    mut_printf(MUT_LOG_TEST_STATUS, "(2) Insert a new SAS drive with smaller capacity into DB slot...");
    MUT_ASSERT_TRUE(TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY < SABBATH_TEST_DRIVE_CAPACITY);

    /*add hook so job would fail before commit stage*/
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_persist_add_hook(FBE_PERSIST_HOOK_TYPE_RETURN_FAILED_TRANSACTION, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "failed to set persist hook for SEP package");
    
    sabbath_insert_blank_new_drive_extended(0, 0, 0, 
                             (fbe_lba_t)(SABBATH_TEST_DRIVE_CAPACITY*1.5),
                             FBE_SAS_DRIVE_SIM_520,
                             520,
                             &drive_handler_new_drive_spa, &drive_handler_new_drive_spb);
    fbe_api_sleep(10000); /*wait for database to process*/

    /*check that the new drive is not connected*/
    downstream_connect_state = sabbath_check_pvd_downstream_connection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST, 0, 0, 0, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, downstream_connect_state, "drive_replace_error_handling_test_online: 0_0_0 PVD connects to a drive, which is not expected");    
    
    mut_printf(MUT_LOG_TEST_STATUS, "(3) Reboot system...");
    /*As there is job failure in this case, so we should clear the error counter in order to 
     *successfully clean up the packages*/
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
    fbe_api_sleep(2000);
    sabbath_poweron_systems(FBE_TRUE, FBE_TRUE);
    sabbath_check_database_states(FBE_DATABASE_STATE_READY, FBE_DATABASE_STATE_READY);    
    fbe_api_sleep(10000); /*wait for database to process*/

    /*check that the new drive is connected*/
    downstream_connect_state = sabbath_check_drive_upstream_connection(0, 0, 0, &pvd_id, FBE_TRUE, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, downstream_connect_state, "0_0_0 drive is not connected to an upstream PVD, which is not expected.");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST, pvd_id, "0_0_0 drive is not connected to system PVD 0, which is not expected.");

    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id,
                                                       FBE_LIFECYCLE_STATE_READY, FBE_TRUE, FBE_TRUE, SABBATH_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait 0_0_0's PVD READY");

    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Fai to wait 0_0_0's PVD READY");

}

/*!**************************************************************
  * sabbath_test_drive_replacement_policy_error_handling_offline_test()
  ****************************************************************
  * @brief
  *  Test the array behavior when a system drive is replaced by
  *  a new drive and the PVD reinitialize job failed when processing
  *  this new drive
  *
  * @param None.               
  *
  * @return None.
  *
  * @author
  *  03/20/2013 Created. Zhipeng Hu
  *
  ****************************************************************/
void sabbath_test_drive_replacement_policy_error_handling_offline_test(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_database_state_t state = FBE_DATABASE_STATE_INVALID;
    fbe_job_service_error_type_t job_error_code;
    fbe_status_t job_status;

    /* Wait for database ready */
    status = fbe_test_sep_util_wait_for_database_service(SABBATH_TEST_WAIT_MSEC);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_database_get_state(&state);
    if(status == FBE_STATUS_OK && state != FBE_DATABASE_STATE_READY)
        status = FBE_STATUS_GENERIC_FAILURE;
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Wait for database ready */
    sabbath_check_database_states(FBE_DATABASE_STATE_READY, FBE_DATABASE_STATE_READY);

    /*create RGs for following test*/
    status = fbe_api_job_service_raid_group_create(&raid_group_create_on_system_drive_cmd);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "drive_replacement_policy_online_test: fail to create raid group");
    status = fbe_api_common_wait_for_job(raid_group_create_on_system_drive_cmd.job_number,
                                         SABBATH_TEST_WAIT_MSEC,
                                         &job_error_code,
                                         &job_status,
                                         NULL);
    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to create RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    mut_printf(MUT_LOG_TEST_STATUS, "======Start online testing error handling======\n");

    sabbath_test_drive_replace_error_handling_test_offline();

    mut_printf(MUT_LOG_TEST_STATUS, "======End online testing error handling======\n");

}

/*!**************************************************************
  * broken_Rose_test()
  ****************************************************************
  * @brief
  *  Test the PVD NP initialization as expected and PVD get into Ready lifecycle
  *
  * @param None.               
  *
  * @return None.
  *
  * @author
  *  11/26/2013 Created. Jian
  *
  ****************************************************************/
void broken_Rose_test(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "Start broken_Rose_test\n");

    /* Check database state */
    sabbath_check_database_states(FBE_DATABASE_STATE_READY, FBE_DATABASE_STATE_READY);
  
    broken_Rose_test_case_1_for_AR601723();

    mut_printf(MUT_LOG_TEST_STATUS, "End broken_Rose_test\n");}

/*!**************************************************************
  * dark_archon_test()
  ****************************************************************
  * @brief
  *  Test the PVD pull and reinsert.
  *  Verify that both PVDs can enter READY state.
  *
  * @param None
  *
  * @return None.
  *
  * @author
  *  2014-05-21 Created. Jamin Kang
  *
  ****************************************************************/
void dark_archon_test(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "Start %s\n", __FUNCTION__);

    /* Check database state */
    sabbath_check_database_states(FBE_DATABASE_STATE_READY, FBE_DATABASE_STATE_READY);
  
    dark_archon_test_case_1_for_AR642445();

    mut_printf(MUT_LOG_TEST_STATUS, "End %s\n", __FUNCTION__);
}

/*!**************************************************************
  * dark_archon_test()
  ****************************************************************
  * @brief
  *  Test the PVD pull and reinsert.
  *  Verify that both PVDs can enter READY state.
  *
  * @param None
  *
  * @return None.
  *
  * @author
  *  2014-05-21 Created. Jamin Kang
  *
  ****************************************************************/
void zealot_test(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "Start %s\n", __FUNCTION__);

    /* Check database state */
    sabbath_check_database_states(FBE_DATABASE_STATE_READY, FBE_DATABASE_STATE_READY);

    zealot_for_AR656681();
    zealot_for_AR670381();

    mut_printf(MUT_LOG_TEST_STATUS, "End %s\n", __FUNCTION__);
}


/*!**************************************************************
 * sabbath_setup()
 ****************************************************************
 * @brief
 *  
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  12/08/2011 - Created. Jian Gao
 *  07/23/2012 - Modified to dualsp test. Zhipeng Hu
 *
 ****************************************************************/
void sabbath_setup(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Sabbath test ===\n");

    if(FBE_FALSE == fbe_test_util_is_simulation())
        return;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Load Physical Package for SPB ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    /* Load the physical configuration */
    sabbath_test_load_physical_config();

    mut_printf(MUT_LOG_TEST_STATUS, "=== Load Physical Package for SPA ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    /* Load the physical configuration */
    sabbath_test_load_physical_config();

    /* Load sep and neit packages */
    sep_config_load_esp_sep_and_neit_both_sps();

    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);
    
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    sabbath_hot_spare_control(FBE_FALSE);
    fbe_api_base_config_disable_all_bg_services();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    sabbath_hot_spare_control(FBE_FALSE);
    fbe_api_base_config_disable_all_bg_services();

    return;
}


/*!**************************************************************
 * sabbath_setup()
 ****************************************************************
 * @brief
 *  
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  12/08/2011 - Created. Jian Gao
 *  07/23/2012 - Modified to dualsp test. Zhipeng Hu
 *
 ****************************************************************/
void sabbath_cleanup(void)
{
    if(FBE_FALSE == fbe_test_util_is_simulation())
        return;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for SPA ===\n");    
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
    fbe_test_sep_util_destroy_esp_neit_sep_physical();

    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for SPB ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
    fbe_test_sep_util_destroy_esp_neit_sep_physical();
}

/*!**************************************************************
 * sabbath_sabbath_test_inject_error_record()
 ****************************************************************
 * @brief
 *  Enable and inject the error for the specified raid group
 *  object and all positions in the bitmap at lba
 *
 * @author
 *  07/11/2013 - Zhipeng Hu
 *
 ****************************************************************/
static void sabbath_test_inject_error_record(
									fbe_object_id_t							raid_group_object_id,
                                    fbe_u16_t                               position_bitmap,
                                    fbe_lba_t                               lba,
                                    fbe_block_count_t                       num_blocks,
                                    fbe_api_logical_error_injection_type_t  error_type,
                                    fbe_u16_t                               err_adj_bitmask,
                                    fbe_u32_t								opcode)
{
    fbe_status_t                                status;
    fbe_api_logical_error_injection_record_t    error_record = {0};
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_u32_t                                   position;
    fbe_object_id_t                             pvd_object_id;

    
    /*  Enable error injection on the raw mirror.
     *  This is a dummy object for the LEI since the raw mirror does not have an object interface.
     */
    status = fbe_api_logical_error_injection_enable_object(raid_group_object_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*  Setup error record for error injection.
     *  Inject error on given disk position and LBA.
     */
    error_record.pos_bitmap = position_bitmap;                      /*  Injecting error on given disk position */
    error_record.width      = FBE_XOR_MAX_FRUS;
    error_record.lba        = lba;                              /*  Physical address to begin errors */
    error_record.blocks     = num_blocks;                       /*  Blocks to extend errors */
    error_record.err_type   = error_type;                       /* Error type */
    error_record.err_mode   = FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS;  /* Error mode */
    error_record.err_count  = 0;                                /*  Errors injected so far */
    error_record.err_limit  = FBE_XOR_MAX_FRUS;   /*  Limit of errors to inject */
    error_record.skip_count = 0;                                /*  Limit of errors to skip */
    error_record.skip_limit = 0;                                /*  Limit of errors to inject */
    error_record.err_adj    = err_adj_bitmask;                  /*  Error adjacency bitmask */
    error_record.start_bit  = 0;                                /*  Starting bit of an error */
    error_record.num_bits   = 0;                                /*  Number of bits of an error */
    error_record.bit_adj    = 0;                                /*  Indicates if erroneous bits adjacent */
    error_record.crc_det    = 0;                                /*  Indicates if CRC detectable */
	error_record.opcode     = opcode;
	
	if(error_type == FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_DOWN
		|| error_type == FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_UP)
	{
		error_record.err_limit  = 16000; /*delay for 16 seconds*/
	}

    /*  Create error injection record 
     */
    status = fbe_api_logical_error_injection_create_record(&error_record);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    for (position = 0; position < FBE_XOR_MAX_FRUS; position++)
    {
        if ((1 << position) & position_bitmap)
        {
            /* Enable error injection on the PVD at this position.
             */
            status = fbe_api_provision_drive_get_obj_id_by_location(0, /* bus */
                                                                    0, /* enclosure */
                                                                    position, /* slot */
                                                                    &pvd_object_id);
            status = fbe_api_logical_error_injection_enable_object(pvd_object_id, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
    }

    /*  Check error injection stats
     */
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);

    return;
}

/*!**************************************************************
 * sabbath_test_disable_error_injection()
 ****************************************************************
 * @brief
 *  Disable error injection for raid group and its pvds
 *
 * @author
 *  07/11/2013 - Zhipeng Hu
 *
 ****************************************************************/
static void sabbath_test_disable_error_injection(fbe_object_id_t	raid_group_object_id,
                                                 fbe_u16_t       position_bitmap)
{
    fbe_status_t                                status;
    fbe_u32_t                                   position;
    fbe_object_id_t                             pvd_object_id;

    status = fbe_api_logical_error_injection_disable_object(raid_group_object_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    for (position = 0; position < FBE_XOR_MAX_FRUS; position++)
    {
        if ((1 << position) & position_bitmap)
        {
            /* Enable error injection on the PVD at this position.
             */
            status = fbe_api_provision_drive_get_obj_id_by_location(0, /* bus */
                                                                    0, /* enclosure */
                                                                    position, /* slot */
                                                                    &pvd_object_id);
            status = fbe_api_logical_error_injection_disable_object(pvd_object_id, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
    }

    return;
}

/*************************
 * end file sabbath_test.c
 *************************/












#if 0
 static void  sabbath_test_case_7_swap_single_db_drive_by_another_array_db_drive(void)
 {
     fbe_status_t               status;
     fbe_database_state_t  state = FBE_DATABASE_STATE_INVALID;
     fbe_homewrecker_fru_descriptor_t test_data_fru_descriptor;
     fbe_bool_t warning;
     fbe_object_id_t pvd_id;
     fbe_bool_t  drive_upstream_connection;
     fbe_database_control_set_fru_descriptor_t fru_write;
     fbe_homewrecker_fru_descriptor_t original_fru_descriptor;
     
     mut_printf(MUT_LOG_TEST_STATUS, "*J* Homewrecker Test: sabbath_test_case_7_swap_single_db_drive_by_another_array_db_drive *L*\n");
     mut_printf(MUT_LOG_TEST_STATUS, "sabbath_test_case_7: store drive original wwn_seed\n");
     fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);    
     status = fbe_api_database_get_fru_descriptor_info(&original_fru_descriptor, 
                                                     (fbe_homewrecker_get_fru_disk_mask_t)SABBATH_TEST_REMOVED_DB_DRIVE_SLOT_1,
                                                     &warning);
     MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
     
     test_data_fru_descriptor = original_fru_descriptor;
 
     /* modify the test_data_fru_descriptor's wwn_seed */
     test_data_fru_descriptor.wwn_seed = 233391;
     mut_printf(MUT_LOG_TEST_STATUS, "sabbath_test_case_7: persist the test_data into db drive\n");
     /* persist the test data_fru_descriptor into db drive 1 */
     fru_write.wwn_seed = test_data_fru_descriptor.wwn_seed;
     status = fbe_api_database_set_fru_descriptor_info(&fru_write, FBE_FRU_WWN_SEED);
     MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
     
     /* reboot */
     mut_printf(MUT_LOG_TEST_STATUS, "sabbath_test_case_7: reboot the system ......\n");
     sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
     sabbath_poweron_systems(FBE_TRUE, FBE_TRUE);
     sabbath_check_database_states(FBE_DATABASE_STATE_READY, FBE_DATABASE_STATE_READY);
 
     /*check connection state*/
     drive_upstream_connection = sabbath_check_drive_upstream_connection(0, 0, 1, NULL, FBE_TRUE, FBE_TRUE);
     MUT_ASSERT_INT_NOT_EQUAL_MSG(FBE_TRUE, drive_upstream_connection, "sabbath_test_case_7: drive 0_0_1 connects to a PVD, which is not expected");
 
     mut_printf(MUT_LOG_TEST_STATUS, "sabbath_test_case_7: restore the orginal wwn_seed to db drive\n");    
     fru_write.wwn_seed = original_fru_descriptor.wwn_seed;
     fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);    
     status = fbe_api_database_set_fru_descriptor_info(&fru_write, FBE_FRU_WWN_SEED);
 
     /*reboot system*/
     sabbath_shutdown_systems(FBE_TRUE, FBE_TRUE);
     sabbath_poweron_systems(FBE_TRUE, FBE_TRUE);
     sabbath_check_database_states(FBE_DATABASE_STATE_READY, FBE_DATABASE_STATE_READY);    
     /*check connection state*/
     drive_upstream_connection = sabbath_check_drive_upstream_connection(0, 0, 1, &pvd_id, FBE_TRUE, FBE_TRUE);
     MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, drive_upstream_connection, "sabbath_test_case_7: drive 0_0_1 not connects to a PVD, which is not expected");
     MUT_ASSERT_INT_EQUAL_MSG(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 1, pvd_id, "sabbath_test_case_7: drive 0_0_1 not connects to the system PVD, which is not expected");
     
     mut_printf(MUT_LOG_TEST_STATUS, "Sabbath_test_case_7_swap_single_db_drive_by_another_array_db_drive: 6th case PASSed, end\n");
     
 }
 
 static void obsolete_sabbath_test_case_8_swap_two_db_drives_by_another_array_db_drives(void)
 {
     fbe_status_t               status;
     fbe_database_state_t  state = FBE_DATABASE_STATE_INVALID;
     fbe_homewrecker_fru_descriptor_t original_fru_descriptor;
     fbe_homewrecker_fru_descriptor_t test_data_fru_descriptor;
     fbe_bool_t warning;
     fbe_database_control_set_fru_descriptor_t fru_write;
     
     mut_printf(MUT_LOG_TEST_STATUS, "*J* Homewrecker Test: sabbath_test_case_8_swap_two_db_drives_by_another_array_db_drives *L*\n");
     mut_printf(MUT_LOG_TEST_STATUS, "sabbath_test_case_8: store drive original wwn_seed\n");
     status = fbe_api_database_get_fru_descriptor_info(&original_fru_descriptor, 
                                                     (fbe_homewrecker_get_fru_disk_mask_t)SABBATH_TEST_REMOVED_DB_DRIVE_SLOT_1,
                                                     &warning);
     MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
     
     test_data_fru_descriptor = original_fru_descriptor;
 
     /* modify the test_data_fru_descriptor's wwn_seed */
     test_data_fru_descriptor.wwn_seed = 233391;
     mut_printf(MUT_LOG_TEST_STATUS, "sabbath_test_case_8: persist the test_data into db drives 0 1\n");    
     fru_write.wwn_seed = test_data_fru_descriptor.wwn_seed;
     status = fbe_api_database_set_fru_descriptor_info(&fru_write, FBE_FRU_WWN_SEED);
     MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
 
     /* reboot */
     mut_printf(MUT_LOG_TEST_STATUS, "sabbath_test_case_8: reboot the system ......\n");
     fbe_test_sep_util_destroy_esp_neit_sep();
     sep_config_load_esp_sep_and_neit();
 
     status = fbe_api_database_get_state(&state);
     if(status == FBE_STATUS_OK && state != FBE_DATABASE_STATE_SERVICE_MODE)
         status = FBE_STATUS_GENERIC_FAILURE;
     MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
 
     mut_printf(MUT_LOG_TEST_STATUS, "sabbath_test_case_8: database maintenance mode!!!\n");
 
     mut_printf(MUT_LOG_TEST_STATUS, "sabbath_test_case_8: restore the db orginal wwn_seed to db drives\n");
     //status = fbe_api_database_set_fru_descriptor_info(original_fru_descriptor.wwn_seed, SABBATH_TEST_REMOVED_DB_DRIVE_SLOT_0);
     //MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
     //status = fbe_api_database_set_fru_descriptor_info(original_fru_descriptor.wwn_seed, SABBATH_TEST_REMOVED_DB_DRIVE_SLOT_1);
 
     
     fru_write.wwn_seed = original_fru_descriptor.wwn_seed;
     status = fbe_api_database_set_fru_descriptor_info(&fru_write, FBE_FRU_WWN_SEED);
     MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
 
     mut_printf(MUT_LOG_TEST_STATUS, "sabbath_test_case_8: 7th case PASSed, end\n");
 
 }
 
 static void obsolete_sabbath_test_case_9_single_db_drive_replaced_by_unrecognized_drive(void)
 {
     fbe_status_t               status;
     fbe_database_state_t  state = FBE_DATABASE_STATE_INVALID;
     fbe_api_terminator_device_handle_t original_db_drive_handle;
     fbe_api_terminator_device_handle_t new_drive_handle;
 
 
   
         
     mut_printf(MUT_LOG_TEST_STATUS, "*J* Homewrecker Test: sabbath_test_case_8_single_db_drive_replaced_by_unrecognized_drive *L*\n");
     mut_printf(MUT_LOG_TEST_STATUS, "Sabbath_test_case_8_single_db_drive_replaced_by_unrecognized_drive: pull db drive 0_0_1 out\n");
       
     /* In this case, one db drive need to be pulled out from enclosure.*/
     /* Remove the 0_0_1 drive */
     status = fbe_test_pp_util_pull_drive(SABBATH_TEST_REMOVED_DB_DRIVE_BUS_0, 
                                          SABBATH_TEST_REMOVED_DB_DRIVE_ENCL_0, 
                                          SABBATH_TEST_REMOVED_DB_DRIVE_SLOT_1,
                                          &original_db_drive_handle);
     
     MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
     mut_printf(MUT_LOG_TEST_STATUS, "Sabbath_test_case_8_single_db_drive_replaced_by_unrecognized_drive: shutdown system.\n");
     fbe_test_sep_util_destroy_esp_neit_sep();
 
     mut_printf(MUT_LOG_TEST_STATUS, "Sabbath_test_case_8_single_db_drive_replaced_by_unrecognized_drive: insert a new drive.\n");
 
     status = fbe_test_pp_util_insert_unique_sas_drive(SABBATH_TEST_REMOVED_DB_DRIVE_BUS_0,
                                                                                                           SABBATH_TEST_REMOVED_DB_DRIVE_ENCL_0,
                                                                                                           SABBATH_TEST_REMOVED_DB_DRIVE_SLOT_1,
                                                                                                           520,
                                                                                                           SABBATH_TEST_DRIVE_CAPACITY,
                                                                                                           &new_drive_handle);
 
     MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
 
     mut_printf(MUT_LOG_TEST_STATUS, "Sabbath_test_case_8_single_db_drive_replaced_by_unrecognized_drive: boot system.\n");
 
     sep_config_load_esp_sep_and_neit();
 
     status = fbe_api_database_get_state(&state);
     if(status == FBE_STATUS_OK && state != FBE_DATABASE_STATE_READY)
         status = FBE_STATUS_GENERIC_FAILURE;
     MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
 
     mut_printf(MUT_LOG_TEST_STATUS, "Sabbath_test_case_8_single_db_drive_replaced_by_unrecognized_drive: database ready!!!\n");
     
     mut_printf(MUT_LOG_TEST_STATUS, "Sabbath_test_case_8_single_db_drive_replaced_by_unrecognized_drive: 8th case PASSed, end\n");
 
 }
#endif
