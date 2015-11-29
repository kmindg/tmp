

/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file kungfu_panda_test.c
 ***************************************************************************
 *
 * @brief
 *   This file includes tests for binding new LUNs in a hole.
 *
 * @version
 *   11/01/2011 - Created. Vera Wang (wangv28)
 *
 ***************************************************************************/


/*-----------------------------------------------------------------------------
    INCLUDES OF REQUIRED HEADER FILES:
*/

#include "sep_tests.h"
#include "sep_utils.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "pp_utils.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe_private_space_layout.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_random.h"
#include "sep_hook.h"
#include "fbe/fbe_api_scheduler_interface.h"

/*-----------------------------------------------------------------------------
    TEST DESCRIPTION:
*/

char* kungfu_panda_short_desc = "LUN Bind in a hole operation";
char* kungfu_panda_long_desc =
    "\n"
    "\n"
    "The Kungfu Panda scenario tests binding new LUNs in a hole:\n"
    "bind 3 luns, unbind the middle one then bind 2 luns in the space of the unbound lun.\n"
    "\n"
    "\n"
    "Starting Config:\n"
    "    [PP] armada board\n"
    "    [PP] SAS PMC port\n"
    "    [PP] viper enclosure\n"
    "    [PP] 3 SAS drives (PDOs)\n"
    "    [PP] 3 logical drives (LDs)\n"
    "    [SEP] 3 provision drives (PVDs)\n"
    "    [SEP] 3 virtual drives (VDs)\n"
    "    [SEP] 1 RAID object (RAID-10)\n"
    "    [SEP] 1 Virtual RAID object (VR)\n"
    "\n"
    "STEP 1: Bring up the initial topology\n"
    "    - Create and verify the initial physical package config\n"
    "    - For each of the drives:(PDO1-PDO2-PDO3)\n"
    "        - Create a virtual drive (VD) with an I/O edge to PVD\n"
    "        - Verify that PVD and VD are both in the READY state\n"
    "    - Create a RAID object and attach edges to the virtual drives\n"
    "\n" 
    "STEP 2: Bind 3 luns\n"
    "    - This is to execise the new interface that calculates the lun capacity.\n"
    "    - Simulate navi command where user specify a rg and number of luns to bind.\n"
    "    - Test bind multiple luns using the full rg capacity.\n"
    "\n"
    "STEP 3: Unbind the middle one"
    "\n"
    "STEP 4: Bind 2 luns in the hole of the unbound lun\n"
    "\n"
    "STEP 5: Verify RG/PVD with Removed drive.\n"
    "\n"
    "DualSP mode: repeat the aboved test on different SP defined in kungfu_panda_dualsp_pass_config[]\n" 
    "\n"
    "\n"
    "Description last updated: 12/3/2012.\n";        


/*!*******************************************************************
 * @def KUNGFU_PANDA_TEST_MAX_RG_WIDTH
 *********************************************************************
 * @brief Max number of drives in a RG
 *
 *********************************************************************/
#define KUNGFU_PANDA_TEST_MAX_RG_WIDTH               16


/*!*******************************************************************
 * @def KUNGFU_PANDA_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects to create
 *        (1 board + 1 port + 4 encl + 60 pdo) 
 *
 *********************************************************************/
#define KUNGFU_PANDA_TEST_NUMBER_OF_PHYSICAL_OBJECTS 66 


/*!*******************************************************************
 * @def KUNGFU_PANDA_TEST_DRIVE_CAPACITY
 *********************************************************************
 * @brief Number of blocks in the physical drive
 *
 *********************************************************************/
#define KUNGFU_PANDA_TEST_DRIVE_CAPACITY             TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY

/*!*******************************************************************
 * @def PLANKTON_TEST_LUN_CHUNK_SIZE
 *********************************************************************
 * @brief LUN chunk size
 *
 *********************************************************************/
#define KUNGFU_PANDA_TEST_LUN_CHUNK_SIZE             0x800

/*!*******************************************************************
 * @def  KUNGFU_PANDA_MAX_RG_SIZE
 *********************************************************************
 * @brief max size of a raid group.
 *
 *********************************************************************/
#define KUNGFU_PANDA_MAX_RG_SIZE 200 * KUNGFU_PANDA_TEST_LUN_CHUNK_SIZE


#define KUNGFU_PANDA_TEST_RAID_GROUP_COUNT 1
#ifndef __SAFE__
#define KUNGFU_PANDA_TEST_PRIVATE_RAID_GROUP_COUNT fbe_private_space_layout_get_number_of_system_raid_groups()
#else
#define KUNGFU_PANDA_TEST_PRIVATE_RAID_GROUP_COUNT (fbe_private_space_layout_get_number_of_system_raid_groups() - 1)
#endif

#define KUNGFU_PANDA_TEST_REMOVE_DRIVE_POS         2

/*!*******************************************************************
 * @var kungfu_panda_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t kungfu_panda_rg_config[] = 
{
    /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
    {4,       KUNGFU_PANDA_MAX_RG_SIZE,     FBE_RAID_GROUP_TYPE_RAID10,    FBE_CLASS_ID_STRIPER,    512,            1,          0,
     {{0,0,1}, {0,1,3}, {0,1,4}, {0,1,5}}},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,/* Terminator. */},
};

/*!*******************************************************************
 * @def KUNGFU_PANDA_TEST_LUN_COUNT
 *********************************************************************
 * @brief  number of luns to be created
 *
 *********************************************************************/
#define KUNGFU_PANDA_TEST_LUN_COUNT        3


/*!*******************************************************************
 * @def KUNGFU_PANDA_TEST_NS_TIMEOUT
 *********************************************************************
 * @brief  Notification to timeout value in milliseconds 
 *
 *********************************************************************/
#define KUNGFU_PANDA_TEST_NS_TIMEOUT        120000 /*wait maximum of 120 seconds*/

/************************************************************************************
 * Components to support Table driven test methodology.
 * Following this approach, the parameters for the test will be stored
 * as rows in a table.
 * The test will then be executed for each row(i.e. set of parameters)
 * in the table.
 ***********************************************************************************/


/* The different test numbers.
 */
typedef enum kungfu_panda_test_enclosure_num_e
{
    KUNGFU_PANDA_TEST_ENCL1_WITH_SAS_DRIVES = 0,
    KUNGFU_PANDA_TEST_ENCL2_WITH_SAS_DRIVES,
    KUNGFU_PANDA_TEST_ENCL3_WITH_SAS_DRIVES,
    KUNGFU_PANDA_TEST_ENCL4_WITH_SAS_DRIVES
} kungfu_panda_test_enclosure_num_t;

/* The different drive types.
 */
typedef enum kungfu_panda_test_drive_type_e
{
    SAS_DRIVE,
} kungfu_panda_test_drive_type_t;

/* This is a set of parameters for the Scoobert spare drive selection tests.
 */
typedef struct enclosure_drive_details_s
{
    kungfu_panda_test_drive_type_t drive_type; /* Type of the drives to be inserted in this enclosure */
    fbe_u32_t port_number;                    /* The port on which current configuration exists */
    kungfu_panda_test_enclosure_num_t  encl_number;      /* The unique enclosure number */
    fbe_block_size_t block_size;
} enclosure_drive_details_t;

/* Table containing actual enclosure and drive details to be inserted.
 */
static enclosure_drive_details_t encl_table[] = 
{
    {   SAS_DRIVE,
        0,
        KUNGFU_PANDA_TEST_ENCL1_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        KUNGFU_PANDA_TEST_ENCL2_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        KUNGFU_PANDA_TEST_ENCL3_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        KUNGFU_PANDA_TEST_ENCL4_WITH_SAS_DRIVES,
        520
    },
};

/* Count of rows in the table.
 */
#define KUNGFU_PANDA_TEST_ENCL_MAX sizeof(encl_table)/sizeof(encl_table[0])

static fbe_sim_transport_connection_target_t kungfu_panda_dualsp_pass_config[] =
{
    FBE_SIM_SP_A, FBE_SIM_SP_B,
};

#define KUNGFU_PANDA_MAX_PASS_COUNT (sizeof(kungfu_panda_dualsp_pass_config)/sizeof(kungfu_panda_dualsp_pass_config[0]))

/*-----------------------------------------------------------------------------
    FORWARD DECLARATIONS:
*/
static void kungu_panda_test_NP_metadata_not_initialized_hook_setup(fbe_object_id_t rg_object_id,
                                                                    fbe_object_id_t *vd_object_id, 
                                                                    fbe_sep_package_load_params_t *sep_params_p);
static void kungu_panda_test_NP_metadata_not_initialized(fbe_object_id_t vd_object_id);
static fbe_status_t kungfu_panda_test_create_lun(fbe_lun_number_t lun_number, fbe_lba_t capacity);
static fbe_status_t kungfu_panda_test_destroy_lun(fbe_lun_number_t lun_number);
static fbe_status_t kungfu_panda_test_bind_two_luns(fbe_lba_t capacity);
static void kungfu_panda_test_create_rg(void);
static void kungfu_panda_test_load_physical_config(void);
static void kungfu_panda_test_verify_lun(fbe_lun_number_t lun_number);
static void kungfu_panda_test_verify_get_all_luns(void);
static void kungfu_panda_test_verify_get_all_raid_groups(void);
static void kungfu_panda_test_verify_get_all_pvds(void);
static void kungfu_panda_test_verify_get_single_raid_group(void);
static void kungfu_panda_test_verify_get_single_pvd(void);
static void kungfu_panda_test_invalidate_config_flag(void);
static void kungfu_panda_test_verify_lun_rg_map(void);
static void kungfu_panda_test_removed_drive_in_rg(void);
static void kungfu_panda_test_verify_multiple_full_polls(void);
static void kungfu_panda_test_verify_full_poll(void);

/*-----------------------------------------------------------------------------
    FUNCTIONS:
*/

/*!****************************************************************************
 *  kungfu_panda_test
 ******************************************************************************
 *
 * @brief
 *  This is the main entry point for the kungfu_panda_test.  
 *
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void kungfu_panda_test(void)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_lba_t                               lun_exported_capacity;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Kungfu Panda Test ===\n");

    kungfu_panda_test_create_rg();
    
    // Calculate the lun exported size based on rg and number of luns to bind.
    status = fbe_api_lun_calculate_max_lun_size(kungfu_panda_rg_config[0].raid_group_id, KUNGFU_PANDA_MAX_RG_SIZE, 
                                          KUNGFU_PANDA_TEST_LUN_COUNT, &lun_exported_capacity);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // Bind KUNGFU_PANDA_TEST_LUN_COUNT = 3 numbers of luns 
    mut_printf(MUT_LOG_TEST_STATUS, "=== Binding KUNGFU_PANDA_TEST_LUN_COUNT(3) luns ===\n");
    kungfu_panda_test_create_lun(123, lun_exported_capacity);
    kungfu_panda_test_create_lun(234, lun_exported_capacity);
    kungfu_panda_test_create_lun(345, lun_exported_capacity);

    // Unbind the middle lun 234
    mut_printf(MUT_LOG_TEST_STATUS, "=== Unbind the middle lun among the 3 luns ===\n");
    kungfu_panda_test_destroy_lun(234); 

    // Bind another two luns in the hole of lun 234
    mut_printf(MUT_LOG_TEST_STATUS, "=== Binding another 2 luns in the hole of the unbound lun===\n");
    kungfu_panda_test_bind_two_luns(lun_exported_capacity);

    kungfu_panda_test_verify_lun(123);
    mut_printf(MUT_LOG_TEST_STATUS, "=== Verify lun successfully. ===\n");

    // Verify full poll statistics. 
    // Note: placing this first in case stats are skewed by previous calls 
    kungfu_panda_test_verify_multiple_full_polls();
    mut_printf(MUT_LOG_TEST_STATUS, "=== Verify multiple full polls successfully. ===\n");

    kungfu_panda_test_verify_get_all_luns();
    mut_printf(MUT_LOG_TEST_STATUS, "=== Verify get all luns successfully. ===\n");

    kungfu_panda_test_verify_get_all_raid_groups();
    mut_printf(MUT_LOG_TEST_STATUS, "=== Verify get all raid groups successfully. ===\n");

    kungfu_panda_test_verify_get_all_pvds();
    mut_printf(MUT_LOG_TEST_STATUS, "=== Verify get all pvds successfully. ===\n");

    kungfu_panda_test_verify_get_single_raid_group();
    mut_printf(MUT_LOG_TEST_STATUS, "=== Verify get single raid group successfully. ===\n");

    kungfu_panda_test_verify_get_single_pvd();
    mut_printf(MUT_LOG_TEST_STATUS, "=== Verify get single pvd successfully. ===\n");

    kungfu_panda_test_verify_lun_rg_map();
    mut_printf(MUT_LOG_TEST_STATUS, "=== Verify LUN/RG map with all Removed drives successfully. ===\n");

    kungfu_panda_test_removed_drive_in_rg();
    mut_printf(MUT_LOG_TEST_STATUS, "=== Verify RG/PVD with Removed drive successfully. ===\n");    

    kungfu_panda_test_invalidate_config_flag();  
    mut_printf(MUT_LOG_TEST_STATUS, "=== Verify invalidate config flag successfully. ===\n");

    return;

}/* End kungfu_panda_test() */

static void kungu_panda_test_NP_metadata_not_initialized_hook_setup(fbe_object_id_t rg_object_id,
                                                                    fbe_object_id_t *vd_object_id, 
                                                                    fbe_sep_package_load_params_t *sep_params_p)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                               random = 0;

    // Randomly choose one VD object ID
    random = fbe_random()%(kungfu_panda_rg_config[0].width);
    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, random, vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    // Add debug hook on all VDs
    sep_params_p->scheduler_hooks[0].object_id = *vd_object_id;
    sep_params_p->scheduler_hooks[0].monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_NON_PAGED_INIT;
    sep_params_p->scheduler_hooks[0].monitor_substate = FBE_RAID_GROUP_SUBSTATE_NONPAGED_INIT;
    sep_params_p->scheduler_hooks[0].check_type = SCHEDULER_CHECK_STATES;
    sep_params_p->scheduler_hooks[0].action = SCHEDULER_DEBUG_ACTION_PAUSE;
    sep_params_p->scheduler_hooks[0].val1 = NULL;
    sep_params_p->scheduler_hooks[1].object_id = FBE_OBJECT_ID_INVALID; //signal to stop

    return;
}

static void kungu_panda_test_NP_metadata_not_initialized(fbe_object_id_t vd_object_id)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_test_wait_for_debug_hook(vd_object_id, 
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_NON_PAGED_INIT, 
                                              FBE_RAID_GROUP_SUBSTATE_NONPAGED_INIT,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE,
                                              0, 0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "NP init hook didnt get hit\n");

    kungfu_panda_test_verify_get_all_pvds();
    mut_printf(MUT_LOG_TEST_STATUS, "=== Verify get all pvds successfully. ===\n"); 

    // Remove all debug hooks
    status = fbe_api_scheduler_del_debug_hook(vd_object_id, 
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_NON_PAGED_INIT, 
                                              FBE_RAID_GROUP_SUBSTATE_NONPAGED_INIT, 0, 0,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return;
}

/*!**************************************************************
 * kungfu_panda_test_verify_lun()
 ****************************************************************
 * @brief
 *  Lun verify function for kungfu_panda_test.
 *
 * @param fbe_lun_number_t lun_number
 *        
 *
 * @return None.
 *
 ****************************************************************/
static void kungfu_panda_test_verify_lun(fbe_lun_number_t lun_number)
{
    fbe_status_t                status;
    fbe_database_lun_info_t     lun_info;
    fbe_object_id_t             lun_object_id;

    status = fbe_api_database_lookup_lun_by_number(lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);   
    lun_info.lun_object_id = lun_object_id;
    status = fbe_api_database_get_lun_info(&lun_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* since rg/lun is on top of flash drive in this test, rotational_rate must be 1.*/
    MUT_ASSERT_INT_EQUAL(lun_info.rotational_rate, 1);
    
    return;
}    

void kungfu_panda_test_verify_get_all_luns(void)
{
    fbe_u32_t                           total_luns, returned_luns;
    fbe_database_lun_info_t *           lun_info;
    fbe_status_t                        status;
    mut_printf(MUT_LOG_TEST_STATUS, "Making sure Interface for getting all LUNs is working"); 

    /*also, test the interface that is getting all the luns in one shot*/
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &total_luns);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "failed to enumerate lun class!!!"); 
    lun_info = (fbe_database_lun_info_t *)fbe_api_allocate_memory(total_luns * sizeof(fbe_database_lun_info_t));
    MUT_ASSERT_POINTER_NOT_EQUAL_MSG(lun_info, NULL, "failed to allocate memory!"); 
    fbe_zero_memory(lun_info, total_luns * sizeof(fbe_database_lun_info_t));
    status = fbe_api_database_get_all_luns(lun_info, total_luns, &returned_luns);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "failed to get all luns"); 
    MUT_ASSERT_INT_EQUAL_MSG(returned_luns, total_luns, "lun count mismatch"); 
    MUT_ASSERT_INT_NOT_EQUAL_MSG(lun_info[0].attributes, 0, "lun attributes mismatch"); 
    MUT_ASSERT_INT_EQUAL_MSG(lun_info->lifecycle_state, FBE_LIFECYCLE_STATE_READY,"lun lifecycle mismatch"); /*lifecycle for the first lun is the list*/

    for (;total_luns > 0; total_luns--) {
        /*for some specific LUNs we expect certain bits set*/
        if (lun_info[total_luns -1].lun_object_id == FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VCX_LUN_0) {
            if (lun_info[total_luns -1].attributes ^ (FBE_LU_SP_ONLY | FBE_LU_SYSTEM | FBE_LU_ATTR_VCX_0)){
                MUT_ASSERT_INT_EQUAL_MSG(0, 1, "failed to match LUN attributes"); 
            }else{
                break;/*we are good*/
            }
        }
    }
    fbe_api_free_memory(lun_info);
    return;
}


void kungfu_panda_test_verify_get_all_raid_groups(void)
{
    fbe_u32_t                           index, total_rgs, returned_rgs;
    fbe_database_raid_group_info_t *    rg_info;
    fbe_status_t                        status;
    fbe_u32_t                           total_rgs_expected = KUNGFU_PANDA_TEST_RAID_GROUP_COUNT + fbe_private_space_layout_get_number_of_system_raid_groups();
    fbe_database_raid_group_info_t      current_rg;
    fbe_u32_t                           pvd_index,lun_index;
    fbe_object_id_t                     pvd_object_id;
    fbe_lun_number_t                    lun_number;
    mut_printf(MUT_LOG_TEST_STATUS, "Making sure Interface for getting all Raid Groups is working"); 

    /*also, test the interface that is getting all the raid groups in one shot*/
    status = fbe_api_get_total_objects_of_all_raid_groups(FBE_PACKAGE_ID_SEP_0, &total_rgs);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "failed to enumerate all raid group class!!!"); 
    rg_info = (fbe_database_raid_group_info_t *)fbe_api_allocate_memory(total_rgs * sizeof(fbe_database_raid_group_info_t));
    MUT_ASSERT_POINTER_NOT_EQUAL_MSG(rg_info, NULL, "failed to allocate memory!"); 
    fbe_zero_memory(rg_info, total_rgs * sizeof(fbe_database_raid_group_info_t));
    status = fbe_api_database_get_all_raid_groups(rg_info, total_rgs, &returned_rgs);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "failed to get all raid groups"); 

    MUT_ASSERT_INT_EQUAL_MSG(returned_rgs, total_rgs_expected, "raid group count mismatch"); 
    for (index = 0; index < returned_rgs; index++) 
    {
        current_rg = rg_info[index];

        if (!fbe_private_space_layout_object_id_is_system_raid_group(current_rg.rg_object_id))
        {  
            /* verify for user rg*/
            MUT_ASSERT_TRUE_MSG((current_rg.rg_info.raid_type == kungfu_panda_rg_config[0].raid_type),"raid group type mismatch"); 
            MUT_ASSERT_TRUE_MSG((current_rg.rg_number == kungfu_panda_rg_config[0].raid_group_id), "raid group number mismatch"); 
            /* verify pvd_list*/
            for (pvd_index = 0; pvd_index < current_rg.pvd_count; pvd_index++) 
            {
                status = fbe_api_provision_drive_get_obj_id_by_location(kungfu_panda_rg_config[0].rg_disk_set[pvd_index].bus, 
                                                                        kungfu_panda_rg_config[0].rg_disk_set[pvd_index].enclosure, 
                                                                        kungfu_panda_rg_config[0].rg_disk_set[pvd_index].slot,
                                                                        &pvd_object_id);
                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                MUT_ASSERT_TRUE_MSG((pvd_object_id == current_rg.pvd_list[pvd_index]),"raid group pvd_list mismatch");      
            }
            /*verify lun_list*/
            for (lun_index = 0; lun_index < current_rg.lun_count; lun_index++) 
            {
                status = fbe_api_database_lookup_lun_by_object_id(current_rg.lun_list[lun_index], &lun_number);
                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                if (lun_number ==123 || lun_number ==345 || lun_number ==456 || lun_number ==567)
                {
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                }
                else
                {
                    MUT_ASSERT_TRUE(status != FBE_STATUS_OK);
                }
            }    
            /* validate the power save capable flag as FALSE since we have a system drive in this rg.
            Then the user rg on top the provision drive which is power save capable also power save capable.*/
            MUT_ASSERT_TRUE(current_rg.power_save_capable == FBE_FALSE); 
        }
        else{
            /* for system raid group, it's on top of system pvd, so it's not power save capable.*/
            MUT_ASSERT_TRUE(current_rg.power_save_capable == FBE_FALSE); 
        }
    }

    fbe_api_free_memory(rg_info);
    return;
}

void kungfu_panda_test_verify_get_all_pvds(void)
{
    fbe_u32_t                   total_pvds, returned_pvds, index, rg_index;
    fbe_database_pvd_info_t *   pvd_list;
    fbe_status_t                status;
    fbe_database_pvd_info_t     current_pvd;
    fbe_u32_t                   pvd_index=1; /* pvd_index 0 is a system drive.*/
    fbe_object_id_t             pvd_object_id, rg_id, user_rg_id;
    
    mut_printf(MUT_LOG_TEST_STATUS, "Making sure Interface for getting all PVDs is working"); 

    /*also, test the interface that is getting all the raid groups in one shot*/
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PROVISION_DRIVE, FBE_PACKAGE_ID_SEP_0, &total_pvds);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "failed to enumerate all PVD class!!!"); 
    pvd_list = (fbe_database_pvd_info_t *)fbe_api_allocate_memory(total_pvds * sizeof(fbe_database_pvd_info_t));
    MUT_ASSERT_POINTER_NOT_EQUAL_MSG(pvd_list, NULL, "failed to allocate memory!"); 
    fbe_zero_memory(pvd_list, total_pvds * sizeof(fbe_database_pvd_info_t));
    status = fbe_api_database_get_all_pvds(pvd_list, total_pvds, &returned_pvds);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "failed to get all pvds"); 

    MUT_ASSERT_INT_EQUAL_MSG(returned_pvds, total_pvds, "PVD count mismatch");
    
    status = fbe_api_database_lookup_raid_group_by_number(kungfu_panda_rg_config[0].raid_group_id, &user_rg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 

    for (index = 0; index < returned_pvds; index++)                                                                                     
    {                                                                                                                                   
        current_pvd = pvd_list[index]; 
        /* validate the spin_down_qualified flag as TRUE since we insert the SAS drive with tla supported for power saving.*/
        MUT_ASSERT_TRUE(current_pvd.pvd_info.spin_down_qualified == FBE_FALSE);
        MUT_ASSERT_TRUE(current_pvd.pool_id == FBE_POOL_ID_INVALID);

        /* validate user PVD. pvd_index start from 1 because the first pvd on rg are system drive. */                                                                                                 
        if (current_pvd.pvd_object_id > FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LAST && (current_pvd.rg_count != 0))                                                       
        {                                                                                                                               
            MUT_ASSERT_TRUE_MSG((current_pvd.pool_id == FBE_POOL_ID_INVALID),"PVD pool id mismatch");
            MUT_ASSERT_TRUE_MSG((current_pvd.pvd_info.config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID),"PVD config type mismatch");
            MUT_ASSERT_TRUE_MSG((current_pvd.rg_count == 1),"RG count on top of user PVD mismatch. Should be one.");
            
            MUT_ASSERT_TRUE_MSG((current_pvd.rg_list[0] == user_rg_id),"raid group ID on top of PVD mismatch");
            MUT_ASSERT_TRUE_MSG((current_pvd.rg_number_list[0] == kungfu_panda_rg_config[0].raid_group_id),"raid group number on top of PVD mismatch");

            for(pvd_index=1; pvd_index<kungfu_panda_rg_config[0].width; pvd_index++)
            {
                if((current_pvd.pvd_info.port_number == kungfu_panda_rg_config[0].rg_disk_set[pvd_index].bus) &&
                    (current_pvd.pvd_info.enclosure_number == kungfu_panda_rg_config[0].rg_disk_set[pvd_index].enclosure) &&
                    (current_pvd.pvd_info.slot_number == kungfu_panda_rg_config[0].rg_disk_set[pvd_index].slot))
                {
                    status = fbe_api_provision_drive_get_obj_id_by_location(kungfu_panda_rg_config[0].rg_disk_set[pvd_index].bus,         
                                                        kungfu_panda_rg_config[0].rg_disk_set[pvd_index].enclosure,   
                                                        kungfu_panda_rg_config[0].rg_disk_set[pvd_index].slot,        
                                                        &pvd_object_id);    
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                    break;
                }
            }

            MUT_ASSERT_INT_EQUAL(pvd_object_id, current_pvd.pvd_object_id);
            MUT_ASSERT_TRUE(current_pvd.pvd_info.power_save_capable == FBE_FALSE);
        }                                                                                                                               
        else                                                                                                                            
        {
            /* verification for one private PVD object. */                                                                                                                               
            if (current_pvd.pvd_object_id == FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST)                                                        
            {                                                                                                                          
                MUT_ASSERT_INT_EQUAL_MSG(current_pvd.rg_count, KUNGFU_PANDA_TEST_PRIVATE_RAID_GROUP_COUNT,"raid group count on top of private PVD mismatch");             

                /*it sits on private RG*/
                status = fbe_api_database_lookup_raid_group_by_number(FBE_PRIVATE_SPACE_LAYOUT_RG_ID_VAULT, &rg_id);
                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                MUT_ASSERT_TRUE_MSG((current_pvd.rg_list[0] == rg_id),"raid group ID on top of PVD mismatch");
                MUT_ASSERT_TRUE_MSG((current_pvd.rg_number_list[0] == FBE_PRIVATE_SPACE_LAYOUT_RG_ID_VAULT),"raid group ID on top of PVD mismatch");
                MUT_ASSERT_TRUE(current_pvd.pvd_info.power_save_capable == FBE_FALSE);
            }
            /* verification for the (0,0,1) system drive. */
            status = fbe_api_provision_drive_get_obj_id_by_location(kungfu_panda_rg_config[0].rg_disk_set[0].bus,         
                                                                    kungfu_panda_rg_config[0].rg_disk_set[0].enclosure,   
                                                                    kungfu_panda_rg_config[0].rg_disk_set[0].slot,        
                                                                    &pvd_object_id);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            if (current_pvd.pvd_object_id == pvd_object_id) 
            {
                /* this PVD is system drive, so we need to check more fields such as lun_list. */
                 MUT_ASSERT_INT_EQUAL_MSG(current_pvd.rg_count, (KUNGFU_PANDA_TEST_RAID_GROUP_COUNT + KUNGFU_PANDA_TEST_PRIVATE_RAID_GROUP_COUNT),"raid group count on top of private PVD mismatch");
                 for(rg_index = 0; rg_index < current_pvd.rg_count; rg_index++) 
                 {
                     rg_id = current_pvd.rg_list[rg_index];
                     if (fbe_private_space_layout_object_id_is_system_raid_group(rg_id) || rg_id == user_rg_id) 
                     {
                         MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                     }
                     else
                     {
                         MUT_ASSERT_TRUE(status != FBE_STATUS_OK);
                     }
                 }

            }
        }                                                                                                                               
    }                                                                                                                                   


    fbe_api_free_memory(pvd_list);
    return;
}

/*!**************************************************************
 * kungfu_panda_test_verify_multiple_full_polls()
 ****************************************************************
 * @brief
 *  Verify that full polls up to the FBE_DATABASE_MAX_POLL_RECORDS
 *  will increase the threshold count meaning a trace message was printed
 *
 * @param None
 *
 * @return None
 *
 ****************************************************************/
void kungfu_panda_test_verify_multiple_full_polls(void)
{
    fbe_u32_t                           i = 0;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_database_control_get_stats_t *  get_stats = NULL;
    fbe_u32_t                           threshold_count = 0;

    get_stats = (fbe_database_control_get_stats_t *)fbe_api_allocate_memory(sizeof(fbe_database_control_get_stats_t));
    fbe_zero_memory(get_stats,  sizeof(fbe_database_control_get_stats_t));
    status = fbe_api_database_get_stats(get_stats);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "failed to get stats"); 
    threshold_count = get_stats->threshold_count;

    mut_printf(MUT_LOG_TEST_STATUS, "Verify threshold count increases after %d consecutive full polls",
               FBE_DATABASE_MAX_POLL_RECORDS); 
    // do N full polls to increase the threshold count
    for (i=0; i< FBE_DATABASE_MAX_POLL_RECORDS ; i++) {
        MUT_ASSERT_INT_EQUAL_MSG(get_stats->threshold_count, threshold_count, "Unxexpected threshold count");
        kungfu_panda_test_verify_full_poll();
    }

    
    status = fbe_api_database_get_stats(get_stats);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "failed to get stats"); 
    MUT_ASSERT_INT_EQUAL_MSG(get_stats->threshold_count, threshold_count+1, "Unxexpected threshold count");
    mut_printf(MUT_LOG_TEST_STATUS, "Threshold count increased from %d to %d",
               threshold_count, get_stats->threshold_count);

    fbe_api_free_memory(get_stats);
}

/*!**************************************************************
 * kungfu_panda_test_verify_full_poll()
 ****************************************************************
 * @brief
 *  Simulate a full poll by admin with consecutive calls for 
 *  get_all_luns, get_all_rgs, and get_all_pvds
 *
 * @param None
 *
 * @return None
 *
 ****************************************************************/
void kungfu_panda_test_verify_full_poll(void) {
    fbe_database_control_get_stats_t *  get_stats;
    fbe_status_t                        status;
    fbe_u32_t                           current_threshold_count = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "Issue a full poll"); 
    get_stats = (fbe_database_control_get_stats_t *)fbe_api_allocate_memory(sizeof(fbe_database_control_get_stats_t));
    fbe_zero_memory(get_stats,  sizeof(fbe_database_control_get_stats_t));

    kungfu_panda_test_verify_get_all_raid_groups();
    status = fbe_api_database_get_stats(get_stats);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "failed to get stats"); 
    MUT_ASSERT_TRUE_MSG(POLL_REQUEST_GET_ALL_RAID_GROUPS == get_stats->last_poll_bitmap, "Unmatching bitmap");
    current_threshold_count = get_stats->threshold_count;

    kungfu_panda_test_verify_get_all_luns();
    status = fbe_api_database_get_stats(get_stats);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "failed to get stats"); 
    MUT_ASSERT_TRUE_MSG((POLL_REQUEST_GET_ALL_LUNS | POLL_REQUEST_GET_ALL_RAID_GROUPS) == get_stats->last_poll_bitmap, "Unmatching bitmap");
    MUT_ASSERT_INT_EQUAL_MSG(get_stats->threshold_count, current_threshold_count, 
                             "Unxexpected threshold count");

    kungfu_panda_test_verify_get_all_pvds();
    status = fbe_api_database_get_stats(get_stats);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "failed to get stats"); 
    MUT_ASSERT_TRUE_MSG(0 == get_stats->last_poll_bitmap, "Unmatching bitmap");

    kungfu_panda_test_verify_get_all_raid_groups();
    status = fbe_api_database_get_stats(get_stats);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "failed to get stats"); 
    MUT_ASSERT_TRUE_MSG(POLL_REQUEST_GET_ALL_RAID_GROUPS == get_stats->last_poll_bitmap, "Unmatching bitmap");

    fbe_api_free_memory(get_stats);
}

/*!**************************************************************
 * kungfu_panda_test_create_lun()
 ****************************************************************
 * @brief
 *  Lun Creation function for kungfu_panda_test.
 *
 * @param fbe_lun_number_t lun_number
 *        fbe_lba_t        capacity
 *
 * @return fbe_status_t    status
 *
 ****************************************************************/
static fbe_status_t kungfu_panda_test_create_lun(fbe_lun_number_t lun_number, fbe_lba_t capacity)
{
    fbe_status_t                    status;
    fbe_api_lun_create_t            fbe_lun_create_req;
    fbe_object_id_t                 lu_id; 
    fbe_job_service_error_type_t    job_error_type;
     
    // Create a LUN
    fbe_zero_memory(&fbe_lun_create_req, sizeof(fbe_api_lun_create_t));
    fbe_lun_create_req.raid_type = kungfu_panda_rg_config[0].raid_type;
    fbe_lun_create_req.raid_group_id = kungfu_panda_rg_config[0].raid_group_id; /*sep_standard_logical_config_one_r5 is 5 so this is what we use*/
    fbe_lun_create_req.lun_number = lun_number;
    fbe_lun_create_req.capacity = capacity;
    fbe_lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
    fbe_lun_create_req.ndb_b = FBE_FALSE;
    fbe_lun_create_req.noinitialverify_b = FBE_FALSE;
    fbe_lun_create_req.addroffset = FBE_LBA_INVALID; /* set to a valid offset for NDB */
    fbe_lun_create_req.world_wide_name.bytes[0] = (fbe_u8_t)fbe_random() & 0xf;
    fbe_lun_create_req.world_wide_name.bytes[1] = (fbe_u8_t)fbe_random() & 0xf;

    status = fbe_api_create_lun(&fbe_lun_create_req, FBE_TRUE, KUNGFU_PANDA_TEST_NS_TIMEOUT, &lu_id, &job_error_type);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);   

    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X created successfully! \n", __FUNCTION__, lu_id);

    return status;   
} /* End kungfu_panda_test_create_lun() */



/*!**************************************************************
 * kungfu_panda_test_destroy_lun()
 ****************************************************************
 * @brief
 *  Lun destroy function for kungfu_panda_test.
 *
 * @param fbe_lun_number_t lun_number
 *
 * @return fbe_status_t    status
 *
 ****************************************************************/
static fbe_status_t kungfu_panda_test_destroy_lun(fbe_lun_number_t lun_number)
{
    fbe_status_t                            status;
    fbe_api_lun_destroy_t                   fbe_lun_destroy_req;
    fbe_job_service_error_type_t            job_error_type;
    
    // Destroy a LUN
    fbe_lun_destroy_req.lun_number = lun_number;

    status = fbe_api_destroy_lun(&fbe_lun_destroy_req, FBE_TRUE, KUNGFU_PANDA_TEST_NS_TIMEOUT, &job_error_type);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    mut_printf(MUT_LOG_TEST_STATUS, " LUN destroyed successfully! \n");

    return status;
}/* End kungfu_panda_test_destroy_lun() */


/*!**************************************************************
 * kungfu_panda_test_bind_two_luns()
 ****************************************************************
 * @brief
 *  Bind another two luns in a hole for kungfu_panda_test.
 *
 * @param  fbe_lba_t        capacity-- This is the exported capacity
 *                                     of the unbound lun(hole)
 *
 * @return fbe_status_t     status
 *
 ****************************************************************/
static fbe_status_t kungfu_panda_test_bind_two_luns(fbe_lba_t capacity)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_lba_t                               lun_exported_capacity;
    fbe_api_lun_calculate_capacity_info_t   capacity_info;
    fbe_object_id_t                         rg_object_id;
    fbe_api_raid_group_get_info_t           get_rg_info;

    // Get RG Object ID based on RG number
    status = fbe_api_database_lookup_raid_group_by_number(kungfu_panda_rg_config[0].raid_group_id, &rg_object_id);
    if ( status != FBE_STATUS_OK)
    {
        mut_printf(FBE_TRACE_LEVEL_ERROR,  "Failed to get RG ObjID based on RG:%d, Sts=0x%x\n", kungfu_panda_rg_config[0].raid_group_id, status);
        return status;
    } 

    // Get the RAID Info in order to get the lun_align_size for further use.
    fbe_zero_memory(&get_rg_info, sizeof(get_rg_info));
    status = fbe_api_raid_group_get_info(rg_object_id, &get_rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK) 
    {
        mut_printf(FBE_TRACE_LEVEL_ERROR, "Failed to get RG info for ObjID:0x%x, Sts=0x%x\n", rg_object_id, status);
        return status;
    }

    // Calculate the hole capacity, which is the imported capacity of lun 234
    capacity_info.exported_capacity = capacity;   
    capacity_info.lun_align_size = get_rg_info.lun_align_size;  
    status = fbe_api_lun_calculate_imported_capacity(&capacity_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // Calculate the lun_exported_capacity given the hole capacity, number of lun to bind and rg id.
    status = fbe_api_lun_calculate_max_lun_size(kungfu_panda_rg_config[0].raid_group_id, capacity_info.imported_capacity, 2, &lun_exported_capacity);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
 
    // Bind another 2 luns in the hole where we just unbind lun 234.
    kungfu_panda_test_create_lun(456, lun_exported_capacity);
    kungfu_panda_test_create_lun(567, lun_exported_capacity);  

    return status;
}/* End kungfu_panda_test_bind_two_luns() */


/*!**************************************************************
 * kungfu_panda_test_create_rg()
 ****************************************************************
 * @brief
 *  Configure the kungfu panda test logical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/

static void kungfu_panda_test_create_rg(void)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_object_id_t                     rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                     rg_object_id_from_job = FBE_OBJECT_ID_INVALID;
    fbe_job_service_error_type_t        job_error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    fbe_status_t                        job_status = FBE_STATUS_OK;
    fbe_u32_t                           rg_index;

    // Create RGs using kungfu_panda_rg_config
    mut_printf(MUT_LOG_TEST_STATUS, "=== Create RAID Groups ===\n");

    for (rg_index = 0; rg_index < KUNGFU_PANDA_TEST_RAID_GROUP_COUNT; rg_index++)
    {
        status = fbe_test_sep_util_create_raid_group(&kungfu_panda_rg_config[rg_index]);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "RG creation failed");

        // wait for notification from job service. 
        status = fbe_api_common_wait_for_job(kungfu_panda_rg_config[rg_index].job_number,
                                             KUNGFU_PANDA_TEST_NS_TIMEOUT,
                                             &job_error_code,
                                             &job_status,
                                             &rg_object_id_from_job);

        MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to created RG");
        MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

        // Verify the object id of the raid group 
        status = fbe_api_database_lookup_raid_group_by_number(kungfu_panda_rg_config[rg_index].raid_group_id, &rg_object_id);

        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_EQUAL(rg_object_id_from_job, rg_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

        // Verify the raid group get to ready state in reasonable time 
        status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
                                                         FBE_LIFECYCLE_STATE_READY, KUNGFU_PANDA_TEST_NS_TIMEOUT,
                                                         FBE_PACKAGE_ID_SEP_0);

        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, "Created Raid Group object %d\n", rg_object_id);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "%s: END\n", __FUNCTION__);

    return;

} /* End kungfu_panda_test_create_rg() */


/*!****************************************************************************
 *  kungfu_panda_setup
 ******************************************************************************
 *
 * @brief
 *  This is the setup function for the kungfu_panda_test. It is responsible
 *  for loading the physical configuration for this test suite on both SPs.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void kungfu_panda_setup(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Kungfu_Panda test ===\n");

    /* Load the physical config on the target server
     */    
    kungfu_panda_test_load_physical_config();

    /* Load the SEP package on the target server
     */
    sep_config_load_sep();

    fbe_test_common_util_test_setup_init();

    return;

} /* End kungfu_panda_setup() */


/*!****************************************************************************
 *  kungfu_panda_cleanup
 ******************************************************************************
 *
 * @brief
 *  This is the cleanup function for the kungfu_panda_test.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void kungfu_panda_cleanup(void)
{  
    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for kungfu_Panda test ===\n");

    fbe_test_sep_util_destroy_sep_physical();

    return;

} /* End kungfu_panda_cleanup() */


/*
 * The following functions are utility functions used by this test suite
 */

/*!**************************************************************
 * kungfu_panda_test_load_physical_config()
 ****************************************************************
 * @brief
 *  Configure the kungfu_panda test physical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/

static void kungfu_panda_test_load_physical_config(void)
{

    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                               total_objects = 0;
    fbe_class_id_t                          class_id;
    fbe_api_terminator_device_handle_t      port0_handle;
    fbe_api_terminator_device_handle_t      port0_encl_handle[KUNGFU_PANDA_TEST_ENCL_MAX];
    fbe_api_terminator_device_handle_t      drive_handle;
    fbe_u32_t                               slot = 0;
    fbe_u32_t                               enclosure = 0;

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
    for ( enclosure = 0; enclosure < KUNGFU_PANDA_TEST_ENCL_MAX; enclosure++)
    { 
        fbe_test_pp_util_insert_viper_enclosure(0,
                                                enclosure,
                                                port0_handle,
                                                &port0_encl_handle[enclosure]);
    }

    /* Insert drives for enclosures.
     */
    for ( enclosure = 0; enclosure < KUNGFU_PANDA_TEST_ENCL_MAX; enclosure++)
    {
        for ( slot = 0; slot < FBE_TEST_DRIVES_PER_ENCL; slot++)
        {
            fbe_test_pp_util_insert_sas_flash_drive(encl_table[enclosure].port_number, 
                                              encl_table[enclosure].encl_number,   
                                              slot,                                
                                              encl_table[enclosure].block_size,    
                                              TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY * 4,      
                                              &drive_handle);
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
    status = fbe_api_wait_for_number_of_objects(KUNGFU_PANDA_TEST_NUMBER_OF_PHYSICAL_OBJECTS, KUNGFU_PANDA_TEST_NS_TIMEOUT, FBE_PACKAGE_ID_PHYSICAL);
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
    MUT_ASSERT_TRUE(total_objects == KUNGFU_PANDA_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");

    return;

} /* End kungfu_panda_test_load_physical_config() */


/*!****************************************************************************
 *  kungfu_panda_dualsp_setup
 ******************************************************************************
 *
 * @brief
 *  This is the setup function for kungfu_pand_test in dualsp mode. It is responsible
 *  for loading the physical configuration for this test suite on both SPs.
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
void kungfu_panda_dualsp_setup(void)
{
    fbe_sim_transport_connection_target_t   sp;

    if (fbe_test_util_is_simulation())
    {
        mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for kungfu panda dualsp test ===\n");
        /* Set the target server */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

        /* Make sure target set correctly */
        sp = fbe_api_sim_transport_get_target_server();
        MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_B, sp);

        mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n",
                   sp == FBE_SIM_SP_A ? "SP A" : "SP B");

        /* Load the physical config on the target server */
        kungfu_panda_test_load_physical_config();

        /* Set the target server */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

        /* Make sure target set correctly */
        sp = fbe_api_sim_transport_get_target_server();
        MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_A, sp);

        mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n",
                   sp == FBE_SIM_SP_A ? "SP A" : "SP B");

        /* Load the physical config on the target server */
        kungfu_panda_test_load_physical_config();

        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during
         * the setup.
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_sep_and_neit_both_sps();
    }

    return;

} /* End kungfu_panda_dualsp_setup() */


/*!****************************************************************************
 *  kungfu_panda_dualsp_cleanup
 ******************************************************************************
 *
 * @brief
 *  This is the cleanup function for the kungfu_panda_dualsp_test.
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
void kungfu_panda_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for kungfu_panda_dualsp_test ===\n");

    // Always clear dualsp mode
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        // First execute teardown on SP B then on SP A
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_destroy_neit_sep_physical();

        // First execute teardown on SP A
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;

} /* End kungfu_panda_dualsp_test_cleanup() */


void kungfu_panda_dualsp_test(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_sim_transport_connection_target_t active_target;
    fbe_u32_t pass;
 
    // Enable dualsp mode
     
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    for (pass = 0 ; pass < KUNGFU_PANDA_MAX_PASS_COUNT; pass++) 
    {
        // Get the active SP
        fbe_test_sep_get_active_target_id(&active_target);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, active_target);
        mut_printf(MUT_LOG_TEST_STATUS, "=== DUAL SP test pass %d: Active SP is %s ===\n", pass,
                   active_target == FBE_SIM_SP_A ? "SP A" : "SP B");

        fbe_api_sim_transport_set_target_server(kungfu_panda_dualsp_pass_config[pass]);
        mut_printf(MUT_LOG_TEST_STATUS, "=== DUAL SP test pass %d: Create LUNs on %s ===\n", pass,
                   kungfu_panda_dualsp_pass_config[pass] == FBE_SIM_SP_A ? "SP A" : "SP B");

        // Now run the create tests
        kungfu_panda_test();

        // Destroy all luns and rgs 
        status = fbe_test_sep_util_destroy_all_user_luns();
        status = fbe_test_sep_util_destroy_all_user_raid_groups();
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    // Always clear dualsp mode
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
}

static void kungfu_panda_test_verify_get_single_pvd(void)
{
    fbe_status_t                status;
    fbe_object_id_t             pvd_object_id, rg_id;
    fbe_database_pvd_info_t     pvd_info;

    status = fbe_api_provision_drive_get_obj_id_by_location(kungfu_panda_rg_config[0].rg_disk_set[1].bus,         
                                                            kungfu_panda_rg_config[0].rg_disk_set[1].enclosure,   
                                                            kungfu_panda_rg_config[0].rg_disk_set[1].slot,        
                                                            &pvd_object_id);    

    MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 

    status = fbe_api_database_get_pvd(pvd_object_id, &pvd_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 

    status = fbe_api_database_lookup_raid_group_by_number(kungfu_panda_rg_config[0].raid_group_id, &rg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                                                               
    MUT_ASSERT_TRUE_MSG((pvd_info.pool_id == FBE_POOL_ID_INVALID),"PVD pool id mismatch");
    MUT_ASSERT_TRUE_MSG((pvd_info.pvd_info.config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID),"PVD config type mismatch");
    MUT_ASSERT_TRUE_MSG((pvd_info.rg_count == 1),"RG count on top of user PVD mismatch. Should be one.");
    MUT_ASSERT_TRUE_MSG((pvd_info.rg_number_list[0] == kungfu_panda_rg_config[0].raid_group_id),"raid group number on top of PVD mismatch");
    MUT_ASSERT_TRUE_MSG((pvd_info.rg_list[0] == rg_id),"raid group ID on top of PVD mismatch");
    MUT_ASSERT_TRUE_MSG((pvd_info.lifecycle_state == FBE_LIFECYCLE_STATE_READY),"PVD lifecycle mismatch");
    
}

static void kungfu_panda_test_verify_get_single_raid_group(void)
{
    fbe_status_t                        status;
    fbe_database_raid_group_info_t      rg_info;
    fbe_object_id_t                     rg_id, pvd_object_id;
    fbe_u32_t                           pvd_index;


    status = fbe_api_database_lookup_raid_group_by_number(kungfu_panda_rg_config[0].raid_group_id, &rg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 

    status = fbe_api_database_get_raid_group(rg_id, &rg_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 

    MUT_ASSERT_TRUE_MSG((rg_info.rg_info.raid_type == kungfu_panda_rg_config[0].raid_type),"raid group type mismatch"); 
    MUT_ASSERT_TRUE_MSG((rg_info.rg_number == kungfu_panda_rg_config[0].raid_group_id), "raid group number mismatch"); 
    MUT_ASSERT_TRUE_MSG((rg_info.rg_info.capacity == kungfu_panda_rg_config[0].capacity), "raid group capacity mismatch");
    MUT_ASSERT_TRUE_MSG((rg_info.lifecycle_state == FBE_LIFECYCLE_STATE_READY), "raid group lifecycle mismatch"); 
    MUT_ASSERT_TRUE_MSG((rg_info.free_non_contigous_capacity > rg_info.extent_size.extent_size), 
                       "raid group free_non_contigous_capacity less than extent size"); //for this rg, free_non_contigous should be bigger than extent_size
    for (pvd_index = 0; pvd_index < rg_info.pvd_count; pvd_index++) 
    {
        status = fbe_api_provision_drive_get_obj_id_by_location(kungfu_panda_rg_config[0].rg_disk_set[pvd_index].bus, 
                                                                kungfu_panda_rg_config[0].rg_disk_set[pvd_index].enclosure, 
                                                                kungfu_panda_rg_config[0].rg_disk_set[pvd_index].slot,
                                                                &pvd_object_id);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE_MSG((pvd_object_id == rg_info.pvd_list[pvd_index]),"raid group pvd_list mismatch");      
    }

}

static void kungfu_panda_test_invalidate_config_flag(void)
{
    fbe_status_t                        status;
    fbe_object_id_t                     expected_object_id;
    fbe_object_id_t                     rg_obj_id;
    fbe_object_id_t                     system_rg_object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_4_DRIVE_R5_RG;
    fbe_bool_t                          b_run_on_dualsp = fbe_test_sep_util_get_dualsp_test_mode();

    if (!b_run_on_dualsp)
    {
        /* fbe_test_common_util_test_setup_init will randomly disbale system drive zeroing, we have to enable it explictly */
        fbe_test_sep_util_enable_system_drive_zeroing();

        status = fbe_api_database_lookup_raid_group_by_number(kungfu_panda_rg_config[0].raid_group_id, &expected_object_id);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 
    
        /* we need to wait for zeroing to DDBS area completes */
        status = fbe_test_sep_util_provision_drive_wait_disk_zeroing(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST, 0x24000, 50 * 1000);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 

        status = fbe_test_sep_util_provision_drive_wait_disk_zeroing(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_LAST, 0x24000, 50 * 1000);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 

        /* wait for rebuild operation to complete */
        status = fbe_test_sep_util_wait_for_raid_group_to_rebuild(system_rg_object_id);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 
        
        /*At the end of Kungfu_Panda test, we can start test invalidate config flag. */
        status = fbe_api_database_clear_invalidate_config_flag();
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 
    
        /*reboot sep*/
        fbe_test_sep_util_destroy_sep();
        sep_config_load_sep();
        status = fbe_test_sep_util_wait_for_database_service(KUNGFU_PANDA_TEST_NS_TIMEOUT);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
        // Get RG Object ID based on RG number
        status = fbe_api_database_lookup_raid_group_by_number(kungfu_panda_rg_config[0].raid_group_id, &rg_obj_id);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        // Assert rg object id are the same since we clear the invalidate configuration flag before reboot.
        MUT_ASSERT_TRUE(expected_object_id == rg_obj_id);
    
        status = fbe_api_database_set_invalidate_config_flag();
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 
    
        /*reboot sep*/
        fbe_test_sep_util_destroy_sep();
        sep_config_load_sep();
        status = fbe_test_sep_util_wait_for_database_service(KUNGFU_PANDA_TEST_NS_TIMEOUT);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
        /* Get RG Object ID based on RG number */
        status = fbe_api_database_lookup_raid_group_by_number(kungfu_panda_rg_config[0].raid_group_id, &rg_obj_id);
        /* We should fail to get the rg_obj_id since we set the invalidate configuration flag before reboot.*/
        MUT_ASSERT_FALSE(status == FBE_STATUS_OK); 
        /* Assert rg object id are invalid and not the same as before. */
        MUT_ASSERT_TRUE(rg_obj_id == FBE_OBJECT_ID_INVALID);  
        /* wait for a while, otherwise test will fail during the cleanup.*/
        fbe_api_sleep(15000);
    }
}


static void kungfu_panda_test_removed_drive_in_rg(void)
{
    fbe_status_t                    status;
    fbe_u32_t                       index;
    fbe_database_pvd_info_t         current_pvd;
    fbe_object_id_t                 rg_id;
    fbe_database_raid_group_info_t  rg_info, new_rg_info;
    fbe_bool_t                      b_run_on_dualsp = fbe_test_sep_util_get_dualsp_test_mode();
    fbe_database_lun_info_t         lun_info;
    fbe_test_raid_group_disk_set_t  *drive_to_remove_p = NULL;

    if (!b_run_on_dualsp)
    {    
        /* Get RG infor for the user RG. */
        status = fbe_api_database_lookup_raid_group_by_number(kungfu_panda_rg_config[0].raid_group_id, &rg_id);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 

        status = fbe_api_database_get_raid_group(rg_id, &rg_info);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        /* Shutdown SEP, Remove first drive in RG and then start SEP.*/
        mut_printf(MUT_LOG_TEST_STATUS, "Removing a RG drive in powered down state and then bringup SEP.");     
        fbe_test_sep_util_destroy_sep();
        fbe_test_sep_util_remove_drives(KUNGFU_PANDA_TEST_REMOVE_DRIVE_POS, &kungfu_panda_rg_config[0]);   
        sep_config_load_sep();
        status = fbe_test_sep_util_wait_for_database_service(KUNGFU_PANDA_TEST_NS_TIMEOUT);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_database_get_raid_group(rg_id, &new_rg_info);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE_MSG((new_rg_info.lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE),"RG lifecycle mismatch");

        /* Loop over all PVDs in this RG and valdiate the properties. */
        for (index = 0; index < rg_info.pvd_count; index++)                                                                                     
        {
            status = fbe_api_database_get_pvd(rg_info.pvd_list[index], &current_pvd);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

            MUT_ASSERT_TRUE_MSG((current_pvd.pvd_info.config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID),"PVD config type mismatch");
            MUT_ASSERT_TRUE_MSG((current_pvd.pvd_info.port_number == kungfu_panda_rg_config[0].rg_disk_set[index].bus),"Bus number mismatch");
            MUT_ASSERT_TRUE_MSG((current_pvd.pvd_info.enclosure_number == kungfu_panda_rg_config[0].rg_disk_set[index].enclosure),"Enclosure number mismatch");
            MUT_ASSERT_TRUE_MSG((current_pvd.pvd_info.slot_number == kungfu_panda_rg_config[0].rg_disk_set[index].slot),"Slot number mismatch");        
 
            if(index == KUNGFU_PANDA_TEST_REMOVE_DRIVE_POS)
            {
                MUT_ASSERT_TRUE_MSG((current_pvd.lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE),"PVD lifecycle mismatch");
            }
            else
            {
                MUT_ASSERT_TRUE_MSG((current_pvd.lifecycle_state == FBE_LIFECYCLE_STATE_READY),"PVD lifecycle mismatch");
            }
        }

        MUT_ASSERT_TRUE_MSG((rg_info.lun_count != 0),"Should see some luns");

        /*make sure we can get specialized luns*/    
        for (index = 0; index < rg_info.lun_count; index++)                                                                                     
        {
            lun_info.lun_object_id = new_rg_info.lun_list[index];
            fbe_api_database_get_lun_info(&lun_info); 
            MUT_ASSERT_TRUE_MSG((lun_info.lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE),"Lun is expected to be specialized");
            MUT_ASSERT_TRUE_MSG((lun_info.raid_info.width == 2),"RG width has to be 2 for this test");
        }

        /* Reinsert the drive removed earlier. */
        drive_to_remove_p = &(kungfu_panda_rg_config[0].rg_disk_set[KUNGFU_PANDA_TEST_REMOVE_DRIVE_POS]);
        mut_printf(MUT_LOG_TEST_STATUS, "reinserting drive %d_%d_%d\n", 
                   drive_to_remove_p->bus,drive_to_remove_p->enclosure, drive_to_remove_p->slot);
        fbe_test_sep_util_insert_drives(KUNGFU_PANDA_TEST_REMOVE_DRIVE_POS, &kungfu_panda_rg_config[0]);
    }
}

void kungfu_panda_test_verify_lun_rg_map(void)
{
    fbe_status_t                        status;
    fbe_database_raid_group_info_t      rg_info, new_rg_info;
    fbe_object_id_t                     rg_id;
    fbe_u32_t                           disk_index, lun_index;
    fbe_database_lun_info_t             current_lun;
    fbe_u32_t                           position_to_test = (fbe_random()%(kungfu_panda_rg_config[0].width));
    fbe_test_raid_group_disk_set_t      *drive_to_test_p = NULL;
    fbe_object_id_t                     pvd_id;
    fbe_database_pvd_info_t             pvd_info;
    fbe_object_id_t                     vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_sep_package_load_params_t       sep_params_p;

    status = fbe_api_database_lookup_raid_group_by_number(kungfu_panda_rg_config[0].raid_group_id, &rg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);            
    
    status = fbe_api_database_get_raid_group(rg_id, &rg_info);                                             
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);                                

    /* Get the PVD object id for test */
    drive_to_test_p = &kungfu_panda_rg_config[0].rg_disk_set[position_to_test];
    status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_test_p->bus, 
                                                            drive_to_test_p->enclosure,
                                                            drive_to_test_p->slot,
                                                            &pvd_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Pull out all drives in the rg and reboot to make rg in SPECIALIZE */ 
    mut_printf(MUT_LOG_TEST_STATUS, "=== Pull out all drives in raid group ===\n");
    for (disk_index = 0; disk_index < kungfu_panda_rg_config[0].width; disk_index++) {
        status = fbe_test_pp_util_pull_drive(kungfu_panda_rg_config[0].rg_disk_set[disk_index].bus, 
                                             kungfu_panda_rg_config[0].rg_disk_set[disk_index].enclosure,
                                             kungfu_panda_rg_config[0].rg_disk_set[disk_index].slot, 
                                             &kungfu_panda_rg_config[0].drive_handle[disk_index]);
        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            fbe_sim_transport_connection_target_t   local_sp;
            fbe_sim_transport_connection_target_t   peer_sp;
    
            /*  Get the ID of the local SP. */
            local_sp = fbe_api_sim_transport_get_target_server();
    
            /*  Get the ID of the peer SP. */
            if (FBE_SIM_SP_A == local_sp)
            {
                peer_sp = FBE_SIM_SP_B;
            }
            else
            {
                peer_sp = FBE_SIM_SP_A;
            }
    
            /*  Set the target server to the peer and reinsert the drive there. */
            fbe_api_sim_transport_set_target_server(peer_sp);
    
            /* remove the drive. */
            status = fbe_test_pp_util_pull_drive(kungfu_panda_rg_config[0].rg_disk_set[disk_index].bus, 
                                                 kungfu_panda_rg_config[0].rg_disk_set[disk_index].enclosure,
                                                 kungfu_panda_rg_config[0].rg_disk_set[disk_index].slot,
                                                 &kungfu_panda_rg_config[0].peer_drive_handle[disk_index]);
    
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
            /*  Set the target server back to the local SP. */
            fbe_api_sim_transport_set_target_server(local_sp);
        }
    }

    /*reboot sep*/
    fbe_test_sep_util_destroy_sep();
    sep_config_load_sep();
    status = fbe_test_sep_util_wait_for_database_service(KUNGFU_PANDA_TEST_NS_TIMEOUT);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);                                            

    /* verify the PVD info. */                                                                                                       
    status = fbe_api_database_get_pvd(pvd_id, &pvd_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 
    MUT_ASSERT_TRUE_MSG((pvd_info.lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE),"PVD lifecycle mismatch");
    MUT_ASSERT_TRUE(pvd_info.pvd_info.port_number == drive_to_test_p->bus); 
    MUT_ASSERT_TRUE(pvd_info.pvd_info.enclosure_number == drive_to_test_p->enclosure); 
    MUT_ASSERT_TRUE(pvd_info.pvd_info.slot_number == drive_to_test_p->slot); 

    /* verify the LUN-RG mapping. */                                                                                                       
    status = fbe_api_database_get_raid_group(rg_id, &new_rg_info);                                             
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);       
    MUT_ASSERT_TRUE_MSG((new_rg_info.lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE),"RG lifecycle mismatch");

    /* Loop over all LUNs in this RG and valdiate the properties. */
    for (lun_index = 0; lun_index < rg_info.lun_count; lun_index++)                                                   
    {                                                                                                                 
        current_lun.lun_object_id = rg_info.lun_list[lun_index];                                                      
        status = fbe_api_database_get_lun_info(&current_lun);                                                         
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);                                                                     
                                                                                                                      
        MUT_ASSERT_TRUE_MSG((current_lun.lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE),"LUN lifecycle mismatch");
    }                                                                                                                  

    /* Reinsert all drives in the rg and reboot to make rg back to READY. */ 
    mut_printf(MUT_LOG_TEST_STATUS, "=== Reinsert all drives in raid group ===\n");
    for (disk_index = 0; disk_index < kungfu_panda_rg_config[0].width; disk_index++) {
        status = fbe_test_pp_util_reinsert_drive(kungfu_panda_rg_config[0].rg_disk_set[disk_index].bus, 
                                                 kungfu_panda_rg_config[0].rg_disk_set[disk_index].enclosure,
                                                 kungfu_panda_rg_config[0].rg_disk_set[disk_index].slot, 
                                                 kungfu_panda_rg_config[0].drive_handle[disk_index]);

        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            fbe_sim_transport_connection_target_t   local_sp;
            fbe_sim_transport_connection_target_t   peer_sp;
    
            /*  Get the ID of the local SP. */
            local_sp = fbe_api_sim_transport_get_target_server();
    
            /*  Get the ID of the peer SP. */
            if (FBE_SIM_SP_A == local_sp)
            {
                peer_sp = FBE_SIM_SP_B;
            }
            else
            {
                peer_sp = FBE_SIM_SP_A;
            }
    
            /*  Set the target server to the peer and reinsert the drive there. */
            fbe_api_sim_transport_set_target_server(peer_sp);
    
            /* reinsert the drive. */
            status = fbe_test_pp_util_reinsert_drive(kungfu_panda_rg_config[0].rg_disk_set[disk_index].bus, 
                                                     kungfu_panda_rg_config[0].rg_disk_set[disk_index].enclosure,
                                                     kungfu_panda_rg_config[0].rg_disk_set[disk_index].slot, 
                                                     kungfu_panda_rg_config[0].peer_drive_handle[disk_index]);
    
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
            /*  Set the target server back to the local SP. */
            fbe_api_sim_transport_set_target_server(local_sp);
        }
    }

    /* Test when NP metadata is not initialized*/
    fbe_zero_memory(&sep_params_p, sizeof(fbe_sep_package_load_params_t));
    kungu_panda_test_NP_metadata_not_initialized_hook_setup(rg_id, &vd_object_id, &sep_params_p);

    /*reboot sep*/
    mut_printf(MUT_LOG_TEST_STATUS, "Rebooting SP to see if RG NP inits..");
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_destroy_neit_sep();
    }
    else
    {
        fbe_test_sep_util_destroy_sep();
    }
    sep_config_load_sep_and_neit_params(&sep_params_p, NULL);
    status = fbe_test_sep_util_wait_for_database_service(KUNGFU_PANDA_TEST_NS_TIMEOUT);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    kungu_panda_test_NP_metadata_not_initialized(vd_object_id);

    // Verify the raid group get to ready state in reasonable time 
    status = fbe_api_wait_for_object_lifecycle_state(rg_id, 
                                                     FBE_LIFECYCLE_STATE_READY, KUNGFU_PANDA_TEST_NS_TIMEOUT,
                                                     FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return;
} 
