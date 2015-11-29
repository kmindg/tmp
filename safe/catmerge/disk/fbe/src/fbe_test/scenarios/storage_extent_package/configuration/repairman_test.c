/*********************************************************************************************
 * Copyright (C) EMC Corporation 2011-2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 *********************************************************************************************/

/*!*******************************************************************************************
 * @file repairman_test.c
 *********************************************************************************************
 *
 * @brief
 *  
 *
 * @version
 *  11/01/2012 - Created. gaoh1
 *  *********************************************************************************************/

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
#include "fbe/fbe_api_lun_interface.h"


/*-----------------------------------------------------------------------------
    DEFINITIONS:
*/

/*!*******************************************************************
 * @def REPAIRMAN_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects to create
 *        (1 board + 1 port + 1 encl + 15 pdo) 
 *
 *********************************************************************/
#define REPAIRMAN_TEST_NUMBER_OF_PHYSICAL_OBJECTS 18
/*#define REPAIRMAN_TEST_NUMBER_OF_PHYSICAL_OBJECTS 33*/

#define REPAIRMAN_TEST_REMOVED_DB_DRIVE_BUS_0 0
#define REPAIRMAN_TEST_REMOVED_DB_DRIVE_ENCL_0 0
#define REPAIRMAN_TEST_REMOVED_DB_DRIVE_SLOT_0 0
#define REPAIRMAN_TEST_REMOVED_DB_DRIVE_SLOT_1 1

/*!*******************************************************************
 * @def REPAIRMAN_TEST_DRIVE_CAPACITY
 *********************************************************************
 * @brief Number of blocks in the physical drive
 *
 *********************************************************************/
#define REPAIRMAN_TEST_DRIVE_CAPACITY 0x318cd800


/************************************************************************************
 * Components to support Table driven test methodology.
 * Following this approach, the parameters for the test will be stored
 * as rows in a table.
 * The test will then be executed for each row(i.e. set of parameters)
 * in the table.
 ***********************************************************************************/

/* The different test numbers.
 */
typedef enum repairman_test_enclosure_num_e
{
    REPAIRMAN_TEST_ENCL1_WITH_SAS_DRIVES = 0,
} repairman_test_enclosure_num_t;

/* The different drive types.
 */
typedef enum repairman_test_drive_type_e
{
    SAS_DRIVE
} repairman_test_drive_type_t;

/* This is a set of parameters for the Scoobert spare drive selection tests.
 */
typedef struct enclosure_drive_details_s
{
    repairman_test_drive_type_t drive_type; /* Type of the drives to be inserted in this enclosure */
    fbe_u32_t port_number;                    /* The port on which current configuration exists */
    repairman_test_enclosure_num_t  encl_number;      /* The unique enclosure number */
    fbe_block_size_t block_size;
} enclosure_drive_details_t;

/* Table containing actual enclosure and drive details to be inserted.
 */
static enclosure_drive_details_t encl_table[] = 
{
    {   SAS_DRIVE,
        0,
        REPAIRMAN_TEST_ENCL1_WITH_SAS_DRIVES,
        520
    },
};

#define RAID_GROUP_NUMBER_1 10
static fbe_api_job_service_raid_group_create_t raid_group_create_cmd_raid1 = 
{
    RAID_GROUP_NUMBER_1,
    3,
    FBE_LUN_DEFAULT_POWER_SAVE_IDLE_TIME,
    8000,
    FBE_RAID_GROUP_TYPE_RAID5,
    FBE_TRI_FALSE,
    FBE_TRUE,
    5000,
    0,
    {
            {0, 0, 1},
            {0, 0, 6},
            {0, 0, 7},
    },
    FBE_FALSE
};

#define RAID_GROUP_NUMBER_2 11
static fbe_api_job_service_raid_group_create_t CSX_MAYBE_UNUSED raid_group_create_cmd_raid2 = 
{
    RAID_GROUP_NUMBER_2,
    3,
    FBE_LUN_DEFAULT_POWER_SAVE_IDLE_TIME,
    8000,
    FBE_RAID_GROUP_TYPE_RAID5,
    FBE_TRI_FALSE,
    FBE_TRUE,
    5000,
    0,
    {
            {0, 0, 1},
            {0, 0, 2},
            {0, 0, 9},
    },
    FBE_FALSE
};


/* Count of rows in the table.
 */
#define REPAIRMAN_TEST_ENCL_MAX sizeof(encl_table)/sizeof(encl_table[0])

#define REPAIRMAN_TEST_WAIT_MSEC                  30000
#define REPAIRMAN_TEST_WAIT_SEC                       60
#define REPAIRMAN_TEST_SLEEP_MESC                 10000
char * repairman_test_short_desc = "Broken Vault RG recovery test";
char * repairman_test_long_desc =
    "Broken Vault RG recovery test\n"
    ;

/*!**************************************************************
 * repairman_test_load_physical_config()
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
static void repairman_test_load_physical_config(void)
{
    fbe_status_t                            status;
    fbe_u32_t                               object_handle;
    fbe_u32_t                               port_idx;
    fbe_u32_t                               enclosure_idx;
    fbe_u32_t                               drive_idx;
    fbe_u32_t                               total_objects;
    fbe_class_id_t                          class_id;
    fbe_api_terminator_device_handle_t      port0_handle;
    fbe_api_terminator_device_handle_t      port0_encl_handle[REPAIRMAN_TEST_ENCL_MAX];
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
    for ( enclosure = 0; enclosure < REPAIRMAN_TEST_ENCL_MAX; enclosure++)
    { 
        fbe_test_pp_util_insert_viper_enclosure(0,
                                                enclosure,
                                                port0_handle,
                                                &port0_encl_handle[enclosure]);
    }

    /* Insert drives for enclosures.
     */
    for ( enclosure = 0; enclosure < REPAIRMAN_TEST_ENCL_MAX; enclosure++)
    {
        for ( slot = 0; slot < 15; slot++)
        {
            if(SAS_DRIVE == encl_table[enclosure].drive_type)
            {
                fbe_test_pp_util_insert_sas_drive(encl_table[enclosure].port_number,
                                                encl_table[enclosure].encl_number,
                                                slot,
                                                encl_table[enclosure].block_size,
                                                REPAIRMAN_TEST_DRIVE_CAPACITY,
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
    status = fbe_api_wait_for_number_of_objects(REPAIRMAN_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 20000, FBE_PACKAGE_ID_PHYSICAL);
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
    MUT_ASSERT_TRUE(total_objects == REPAIRMAN_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");

    return;

}

static void repairman_disable_background_service(void)
{
    
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_sim_transport_connection_target_t   original_target;

    original_target = fbe_api_sim_transport_get_target_server();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_base_config_disable_all_bg_services();
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK,status,"repairman_test: fail to disable backgroud service in SPA");

    fbe_api_sim_transport_set_target_server(original_target);
}

/*!**************************************************************
 * repairman_check_database_states()
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
static void repairman_check_database_states(fbe_database_state_t  SPA_expected_state)
{
    fbe_database_state_t    state;
    fbe_status_t    status;
    fbe_sim_transport_connection_target_t   original_target;
    
    original_target = fbe_api_sim_transport_get_target_server();
    
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_database_get_state(&state);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK,status,"repairman_test: fail to get db state of SPA");
    MUT_ASSERT_INT_EQUAL_MSG(SPA_expected_state,state,"repairman_test: SPA's state is not expected one");
    
    fbe_api_sim_transport_set_target_server(original_target);

}

/*!**************************************************************
 * repairman_shutdown_systems()
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
static void repairman_shutdown_systems(fbe_bool_t shutdown_spa, fbe_bool_t shutdown_spb)
{
    fbe_sim_transport_connection_target_t   original_target;
    
    original_target = fbe_api_sim_transport_get_target_server();

    if(FBE_TRUE == shutdown_spb)
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
        fbe_test_sep_util_destroy_neit_sep();
    }

    if(FBE_TRUE == shutdown_spa)
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
        fbe_test_sep_util_destroy_neit_sep();
    }

    fbe_api_sim_transport_set_target_server(original_target);
}
/*!**************************************************************
 * repairman_poweron_systems()
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
static void repairman_poweron_systems(fbe_bool_t poweron_spa, fbe_bool_t poweron_spb)
{
    fbe_sim_transport_connection_target_t   original_target;
    
    original_target = fbe_api_sim_transport_get_target_server();

    if(FBE_TRUE == poweron_spa)
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_sep_and_neit();
        fbe_api_control_automatic_hot_spare(FBE_FALSE);  /*disable automatic hotspare activity*/
    }

    if(FBE_TRUE == poweron_spb)
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        sep_config_load_sep_and_neit();
        fbe_api_control_automatic_hot_spare(FBE_FALSE);  /*disable automatic hotspare activity*/
    }


    fbe_api_sim_transport_set_target_server(original_target);
}

/*!**************************************************************
 * repairman_pull_drive()
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
static void repairman_pull_drive(fbe_u32_t port_number, 
                                         fbe_u32_t enclosure_number, 
                                         fbe_u32_t slot_number,
                                         fbe_api_terminator_device_handle_t *drive_handle_p_spa)                                         
{
    fbe_status_t    status;
    fbe_sim_transport_connection_target_t   original_target;
    
    original_target = fbe_api_sim_transport_get_target_server();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_test_pp_util_pull_drive(port_number, enclosure_number, slot_number, drive_handle_p_spa);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "repairman_pull_drive: fail to pull drive from SPA");

    fbe_api_sim_transport_set_target_server(original_target);
}

/*!**************************************************************
 * repairman_reinsert_drive()
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
static void repairman_reinsert_drive(fbe_u32_t port_number, 
                                         fbe_u32_t enclosure_number, 
                                         fbe_u32_t slot_number,
                                         fbe_api_terminator_device_handle_t drive_handle_p_spa)                                        
{
    fbe_status_t    status;
    fbe_sim_transport_connection_target_t   original_target;
    
    original_target = fbe_api_sim_transport_get_target_server();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_test_pp_util_reinsert_drive(port_number, enclosure_number, slot_number, drive_handle_p_spa);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "repairman_reinsert_drive: fail to reinsert drive from SPA");

    fbe_api_sim_transport_set_target_server(original_target);
}

/*!**************************************************************
 * repairman_insert_blank_new_drive()
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
 *  
 ****************************************************************/
static void repairman_insert_blank_new_drive(fbe_u32_t port_number, 
                                         fbe_u32_t enclosure_number, 
                                         fbe_u32_t slot_number,
                                         fbe_api_terminator_device_handle_t *drive_handle_p_spa)                                         
{
    fbe_status_t    status;
    fbe_sim_transport_connection_target_t   original_target;
    fbe_sas_address_t   blank_new_sas_address;
    
    original_target = fbe_api_sim_transport_get_target_server();
    blank_new_sas_address = fbe_test_pp_util_get_unique_sas_address();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_test_pp_util_insert_sas_drive_extend(port_number, enclosure_number, slot_number, 
                                                                                                        520, REPAIRMAN_TEST_DRIVE_CAPACITY, 
                                                                                                        blank_new_sas_address, FBE_SAS_DRIVE_SIM_520, drive_handle_p_spa);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "repairman_insert_blank_new_drive: fail to insert drive from SPA");


    fbe_api_sim_transport_set_target_server(original_target);
}

/*!**************************************************************
 * repairman_wait_for_sep_object_state()
 ****************************************************************
 * @brief
 *  Wait for a SEP object to become the specified lifecycle state
 *
 * @param object_id - SEP object id.
 * @param expected_lifecycle_state - the specified lifecycle state.
 * @param wait_spa - whether to wait on SPA.
 * @param wait_sec - how many seconds to wait
 *
 * @return whether waited the expected state.   
 *
 * @author
 *
 ****************************************************************/
static fbe_bool_t  repairman_wait_for_sep_object_state(fbe_object_id_t object_id, fbe_lifecycle_state_t  expected_lifecycle_state, fbe_u32_t wait_sec)
{
    fbe_status_t    status;
    fbe_lifecycle_state_t   current_state;
    fbe_u32_t   wait_sec_count = wait_sec;

    fbe_sim_transport_connection_target_t   original_target;
    
    original_target = fbe_api_sim_transport_get_target_server();    

 

    while(wait_sec_count--)
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        status = fbe_api_get_object_lifecycle_state (object_id, &current_state, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "repairman_wait_for_sep_object_state: fail to get object lifecycle state in SPA");
        if(expected_lifecycle_state == current_state)
        {
            break;
        }
 
        fbe_api_sleep(1000);
    }

    fbe_api_sim_transport_set_target_server(original_target);

    return status;   
    
}

fbe_status_t repairman_destroy_raid_group_by_raid_group_number(fbe_raid_group_number_t raid_num)
{
    fbe_api_job_service_raid_group_destroy_t raid_group_destroy_req;
    fbe_job_service_error_type_t      error_type = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    fbe_status_t    job_status;
    fbe_status_t    status;

    fbe_zero_memory(&raid_group_destroy_req, sizeof(fbe_api_job_service_raid_group_destroy_t));
    raid_group_destroy_req.raid_group_id = raid_num; 
    status = fbe_api_job_service_raid_group_destroy(&raid_group_destroy_req);
    /* Check the status for SUCCESS*/
    if (status != FBE_STATUS_OK)
    {
        mut_printf(MUT_LOG_TEST_STATUS,"%s:\tRG %d removal failed %d\n", __FUNCTION__,
                      raid_group_destroy_req.raid_group_id, status);
        return status;
    }
    
    /* wait for it*/
    status = fbe_api_common_wait_for_job(raid_group_destroy_req.job_number, 
                             RG_WAIT_TIMEOUT, &error_type, &job_status, NULL);
    if (status != FBE_STATUS_OK || job_status != FBE_STATUS_OK) 
    {
        mut_printf(MUT_LOG_TEST_STATUS,"%s: wait for RG job failed. Status: 0x%X, job status:0x%X, job error:0x%x\n", 
                   __FUNCTION__, status, job_status, error_type);
    
        return status;
    }

    return status;

}


fbe_status_t repairman_destroy_system_parity_rg_and_luns(void)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       index;
    fbe_u32_t       reg_index;
    fbe_u32_t       lun_num;
    fbe_api_lun_destroy_t lun_destroy_req;
    fbe_job_service_error_type_t      error_type = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    fbe_private_space_layout_region_t region = {0};
    fbe_private_space_layout_lun_info_t lun_info = {0};
    fbe_lifecycle_state_t lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_u8_t recreate_flags = 0;

    fbe_zero_memory(&lun_destroy_req, sizeof(fbe_api_lun_destroy_t));
        
    /* generate config for parity RG */
    for(reg_index = 0; reg_index < FBE_PRIVATE_SPACE_LAYOUT_MAX_REGIONS; reg_index++){
        status = fbe_private_space_layout_get_region_by_index(reg_index, &region);
        if (status != FBE_STATUS_OK) {
            return status;
        }
        else
        {
            // Hit the Terminator
            if(region.region_id == FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_INVALID) 
            {
                break;
            }
            else if(region.region_type != FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_RAID_GROUP)
            {
                continue;
            } else
            {
                /*Only process the Parity RG */
                if (region.raid_info.raid_type != FBE_RAID_GROUP_TYPE_RAID3 &&
                    region.raid_info.raid_type != FBE_RAID_GROUP_TYPE_RAID5)
                {
                    continue;
                }

                /*First we check whether the RG is in good state */
                status = fbe_api_get_object_lifecycle_state(region.raid_info.object_id, 
                                                                      &lifecycle_state, FBE_PACKAGE_ID_SEP_0);
                if (status != FBE_STATUS_OK)
                {
                    return status;
                }
                /*If the Raid group is in ready state, skip it */
                if (lifecycle_state == FBE_LIFECYCLE_STATE_READY)
                    continue;


                /* Before destroy the RG, destroy all the LUN on it.
                          *  We can't use fbe_api_base_config_get_upstream_object_list function to 
                          *  get all the LUN list if all the objects are in specialize state.
                          *  We use the PSL to get all the LUN above the RG.
                          */
                for (index = 0; index <= FBE_PRIVATE_SPACE_LAYOUT_MAX_PRIVATE_LUNS; index++)
                {
                    status = fbe_private_space_layout_get_lun_by_index(index, &lun_info);
                    if (status != FBE_STATUS_OK)
                    {
                        return status;
                    }
                    else if(lun_info.lun_number == FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_INVALID)
                    {
                        // hit the terminator
                        break;
                    }
                    else if(region.raid_info.raid_group_id == lun_info.raid_group_id)
                    {
                        status = fbe_api_database_lookup_lun_by_object_id(lun_info.object_id, &lun_num);
                        if (status != FBE_STATUS_OK)
                        {
                            return status;
                        }
                        lun_destroy_req.lun_number = lun_num;
                        lun_destroy_req.allow_destroy_broken_lun = FBE_TRUE;
                        status= fbe_api_destroy_lun(&lun_destroy_req, FBE_TRUE,LU_DESTROY_TIMEOUT, &error_type);
                        if (status != FBE_STATUS_OK)
                        {
                            return status;
                        }
                        mut_printf(MUT_LOG_TEST_STATUS, "Destroy System LUN: %d\n", lun_num);
                    }

                }

                /*After all the above LUNs destroyed, destroy the RG*/
                status = repairman_destroy_raid_group_by_raid_group_number(region.raid_info.raid_group_id);
                /* Check the status for SUCCESS*/
                if (status != FBE_STATUS_OK)
                {
                    mut_printf(MUT_LOG_TEST_STATUS,"%s:\tRG %d removal failed %d\n", __FUNCTION__,
                                  region.raid_info.raid_group_id, status);
                    return status;
                }

                mut_printf(MUT_LOG_TEST_STATUS,"Destroy System RG: %d\n", region.raid_info.raid_group_id);
                /*For system RG, we also need to zero out the nonpaged metadata. */
                recreate_flags |= FBE_SYSTEM_OBJECT_OP_RECREATE;
                status = fbe_api_database_set_system_object_recreation(region.raid_info.object_id, recreate_flags);

            }            

        }
            
    }

    return status;
}

static void repairman_test_case1(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_terminator_device_handle_t drive_handler_1_spa;
    fbe_api_terminator_device_handle_t drive_handler_2_spa;
    fbe_api_terminator_device_handle_t drive_handler_new_spa;

    fbe_bool_t wait_state;
    fbe_job_service_error_type_t        job_error_type = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    fbe_status_t                        job_status;

    mut_printf(MUT_LOG_TEST_STATUS, "****Case 1. pull drive offline, and there will be a user rg degraded after removing two system drives.\n");

    mut_printf(MUT_LOG_TEST_STATUS, "create a user R5 based on 0_0_1, 0_0_6 and 0_0_7\n ");  

    /*create an user Raid group RG, which will be degraded after pull 0_0_1 and 0_0_2 */
    status = fbe_api_job_service_raid_group_create(&raid_group_create_cmd_raid1);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "fail to create raid group");
    status = fbe_api_common_wait_for_job(raid_group_create_cmd_raid1.job_number,
                                         REPAIRMAN_TEST_WAIT_MSEC,
                                         &job_error_type,
                                         &job_status,
                                         NULL);
    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to create RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    mut_printf(MUT_LOG_TEST_STATUS, "Pull DB drive 0_0_1 and 0_0_2");    
    repairman_pull_drive(0, 0, 1, &drive_handler_1_spa);
    repairman_pull_drive(0, 0, 2, &drive_handler_2_spa);

    fbe_api_sleep(8000);
    
    /*Destroy system parity RG and LUNs*/
    mut_printf(MUT_LOG_TEST_STATUS, "Destroy all the system parity RGs and LUNs\n");
    status = repairman_destroy_system_parity_rg_and_luns();    
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "repairman: fail to destroy all system parity RGs and LUNs\n");


    mut_printf(MUT_LOG_TEST_STATUS, "insert two new DB drive to 0_0_1 and 0_0_2.");
    //repairman_poweron_systems(FBE_TRUE, FBE_TRUE);
    repairman_insert_blank_new_drive(0, 0, 1, &drive_handler_new_spa);
    repairman_insert_blank_new_drive(0, 0, 2, &drive_handler_new_spa);

    fbe_api_sleep(9000);

    wait_state = repairman_wait_for_sep_object_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 1, FBE_LIFECYCLE_STATE_READY, REPAIRMAN_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_state, "system PVD1 is not in READY state, not expected.");
    wait_state = repairman_wait_for_sep_object_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 2, FBE_LIFECYCLE_STATE_READY, 30);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_state, "system PVD2 is not in READY state, not expected.");

    mut_printf(MUT_LOG_TEST_STATUS, "Generate the configuration for system RG and LUNs.\n");
    status = fbe_api_database_generate_configuration_for_system_parity_rg_and_lun(FBE_FALSE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "repairman: fail to generate all system parity RGs and LUNs\n");

    mut_printf(MUT_LOG_TEST_STATUS, "Reboot the systems.\n");
    repairman_shutdown_systems(FBE_TRUE, FBE_FALSE);
    repairman_poweron_systems(FBE_TRUE, FBE_FALSE);

    mut_printf(MUT_LOG_TEST_STATUS, "Check if the database is ready.\n");
    repairman_check_database_states(FBE_DATABASE_STATE_READY);

    wait_state = repairman_wait_for_sep_object_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG, FBE_LIFECYCLE_STATE_READY, REPAIRMAN_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_state, "system Vault RG is not in READY state, not expected.");



    mut_printf(MUT_LOG_TEST_STATUS, "Finally, we destroy the user RG recreated before.\n");
    status = repairman_destroy_raid_group_by_raid_group_number(RAID_GROUP_NUMBER_1);
    /* Check the status for SUCCESS*/
    if (status != FBE_STATUS_OK)
    {
        mut_printf(MUT_LOG_TEST_STATUS,"%s:\tRG %d removal failed %d\n", __FUNCTION__,
                      RAID_GROUP_NUMBER_1, status);
        return;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "****Case 1. Done.\n");    
}

static void repairman_test_case3_for_debug_usage(void)
{
    fbe_api_terminator_device_handle_t drive_handler_1_spa;
    fbe_api_terminator_device_handle_t drive_handler_2_spa;
    fbe_api_terminator_device_handle_t drive_handler_new_spa;
    fbe_bool_t wait_state;

    mut_printf(MUT_LOG_TEST_STATUS, "Destroy the SEP and pull two db drives\n");
    repairman_shutdown_systems(FBE_TRUE, FBE_FALSE);
    
    mut_printf(MUT_LOG_TEST_STATUS, "Pull DB drive 0_0_1 and 0_0_2");    
    repairman_pull_drive(0, 0, 1, &drive_handler_1_spa);
    repairman_pull_drive(0, 0, 2, &drive_handler_2_spa);

    repairman_poweron_systems(FBE_TRUE, FBE_FALSE);

    mut_printf(MUT_LOG_TEST_STATUS, "insert two new DB drive to 0_0_1 and 0_0_2.");    
    repairman_insert_blank_new_drive(0, 0, 1, &drive_handler_new_spa);
    repairman_insert_blank_new_drive(0, 0, 2, &drive_handler_new_spa);

    wait_state = repairman_wait_for_sep_object_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 1, FBE_LIFECYCLE_STATE_READY, REPAIRMAN_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_state, "system PVD1 is not in READY state, not expected.");
    wait_state = repairman_wait_for_sep_object_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + 2, FBE_LIFECYCLE_STATE_READY, REPAIRMAN_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_state, "system PVD2 is not in READY state, not expected.");


    mut_printf(MUT_LOG_TEST_STATUS, "Reboot the systems.\n");
    repairman_shutdown_systems(FBE_TRUE, FBE_FALSE);
    repairman_poweron_systems(FBE_TRUE, FBE_FALSE);

    mut_printf(MUT_LOG_TEST_STATUS, "Check if the database is ready.\n");
    repairman_check_database_states(FBE_DATABASE_STATE_READY);

    wait_state = repairman_wait_for_sep_object_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG, FBE_LIFECYCLE_STATE_READY, REPAIRMAN_TEST_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_state, "system Vault RG is not in READY state, not expected.");

    mut_printf(MUT_LOG_TEST_STATUS, "Destroy the SEP and pull two db drives\n");
    repairman_shutdown_systems(FBE_TRUE, FBE_FALSE);
    
    mut_printf(MUT_LOG_TEST_STATUS, "Pull DB drive 0_0_1 and 0_0_2");    
    repairman_pull_drive(0, 0, 1, &drive_handler_1_spa);
    repairman_pull_drive(0, 0, 2, &drive_handler_2_spa);

    repairman_poweron_systems(FBE_TRUE, FBE_FALSE);    

    
    mut_printf(MUT_LOG_TEST_STATUS, "insert two new DB drive to 0_0_1 and 0_0_2.");    
    repairman_insert_blank_new_drive(0, 0, 1, &drive_handler_new_spa);
    repairman_insert_blank_new_drive(0, 0, 2, &drive_handler_new_spa);

    mut_printf(MUT_LOG_TEST_STATUS, "****Case 2. Done.\n");    

}


/*!**************************************************************
  * repairman_test()
  ****************************************************************
  * @brief
  *  
  *
  * @param None.               
  *
  * @return None.
  *
  * @author
  *  11/01/2012 Created. Gaoh1
  *
  ****************************************************************/
void repairman_test(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_database_state_t state = FBE_DATABASE_STATE_INVALID;

    /* Wait for database ready */
    status = fbe_test_sep_util_wait_for_database_service(REPAIRMAN_TEST_WAIT_MSEC);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_database_get_state(&state);
    if(status == FBE_STATUS_OK && state != FBE_DATABASE_STATE_READY)
        status = FBE_STATUS_GENERIC_FAILURE;
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Start repairman_test_move_drive_when_array_is_offline...\n");

    /* Wait for database ready */
    repairman_check_database_states(FBE_DATABASE_STATE_READY);

    repairman_test_case1();
       
    mut_printf(MUT_LOG_TEST_STATUS, "End repairman_test_move_drive_when_array_is_offline.\n");
}


/*!**************************************************************
 * repairman_setup()
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
void repairman_setup(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for repairman test ===\n");

    if(FBE_FALSE == fbe_test_util_is_simulation())
        return;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Load Physical Package for SPA ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    /* Load the physical configuration */
    repairman_test_load_physical_config();

    /* Load sep and neit packages */
    sep_config_load_sep_and_neit();

    fbe_api_control_automatic_hot_spare(FBE_FALSE);
    //fbe_api_base_config_disable_all_bg_services();

    return;
}


/*!**************************************************************
 * repairman_setup()
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
void repairman_cleanup(void)
{
    if(FBE_FALSE == fbe_test_util_is_simulation())
        return;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for SPA ===\n");    
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
    fbe_test_sep_util_destroy_neit_sep_physical();

}

/*************************
 * end file repairman_test.c
 *************************/

