
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file chitti_test.c
 ***************************************************************************
 *
 * @brief
 *   This file includes for Destroying/Recovering degraded/stuck RG/LUNs.
 *
 * @version
 * 
 * @author
 *   09/15/2011 - Created. Arun S 
 *
 ***************************************************************************/


/*-----------------------------------------------------------------------------
    INCLUDES OF REQUIRED HEADER FILES:
*/

#include "sep_tests.h"
#include "sep_utils.h"
#include "sep_rebuild_utils.h"
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
#include "fbe_test_common_utils.h"
#include "physical_package_tests.h"
#include "fbe/fbe_api_event_log_interface.h"

/*-----------------------------------------------------------------------------
    TEST DESCRIPTION:
*/

char* chitti_short_desc = "Destroy/Recover degraded/stuck RG/LUNs - RAID 5";
char* chitti_long_desc =
    "\n"
    "\n"
    "The Chitti scenario tests destroying/recovering degraded/stuck RG/LUNs\n"
    "\n"
    "\n"
    "*** Chitti **************************************************************\n"
    "\n"    
    "Dependencies:\n"
    "    - Job Control Service (JCS) API solidified\n"
    "    - JCS creation queue operational\n"
    "    - Database (DB) Service able to persist database both locally and on the peer\n"
    "    - Ability to get lun info using fbe_api\n"
    "    - Ability to change lun info using fbe_api\n"
    "    - Ability to get remove/insert drive using terminator\n"
    "    - Ability to destroy RG/LUN using fbe_api\n"
    "\n"
    "Starting Config:\n"
    "    [PP] 1 - armada board\n"
    "    [PP] 1 - SAS PMC port\n"
    "    [PP] 2 - viper enclosures\n"
    "    [PP] 30 - SAS drives (PDOs)\n"
    "    [PP] 30 - Logical drives (LDs)\n"
    "\n"
    "STEP 1: Bring up the initial topology\n"
    "    - Create and verify the initial physical package config\n"
    "    - For each of the drives:(PDO1-PDO2-PDO3)\n"
    "        - Create a virtual drive (VD) with an I/O edge to PVD\n"
    "        - Verify that PVD and VD are both in the READY state\n"
    "    - Create a RAID object(R5) and attach edges to the virtual drives\n"
    "\n" 
    "STEP 2: Send a LUN Bind request using fbe_api\n"
    "    - Verify that JCS gets request and puts it on its creation queue for processing\n"
    "    - Verify that JCS processes request and new LUN Object is created successfully by\n"
    "      verifying the following:\n"
    "       - LUN Object in the READY state\n" 
    "\n"
    "STEP 3: Make the RG/LUN degraded.\n"
    "    - Write some data pattern to the disk before degrading the RG/LUN.\n"
    "    - Remove # of drives which can put the RG/LUN degraded.\n"
    "    - Verify the drive PDO and LDO is removed.\n"
    "    - Verify the RG/LUN/VD/PVD all put to FAIL state.\n"
    "\n"
    "STEP 4: Make the RG/LUN stuck in SPECLIAZE state.\n"
    "    - Reboot the SEP to put the RG/LUN/PVD/VD from FAIL to SPECIALIZE state.\n"
    "    - Verify the RG/LUN/PVD/VD is in SPECIALIZE state.\n"
    "\n"
    "STEP 5: Destroy the RG/LUN which is in SPECLIAZE state.\n"
    "    - Destroy the RG/LUN.\n"
    "    - Verify the RG/LUN/VD is destroyed successfully.\n"
    "\n"
    "STEP 6: Recover the RG/LUN which got destroyed.\n"
    "    - Re-insert the removed drives.\n"
    "    - Verify the PDO, LDO and PVD goes to READY state for the inserted drives.\n"
    "    - Create RG with the same set of drives and same configuration that got created earlier.\n"
    "    - Re-bind the destroyed LUNs with the same offset and capacity but with NDB flag SET.\n"
    "    - Verify the recreated RG/LUN gets created successfully.\n"
    "\n"
    "STEP 7: Verifying the data in the newly created RG/LUNs.\n"
    "    - Read the data pattern in the newly created RG/LUNs.\n"
    "    - Verify if it matches what we wrote before degrading/destroying it.\n"
    "\n"
    "STEP 8: Destroy raid groups and LUNs.\n"
    "\n"
    "Description last updated: 09/15/2011.\n"
    "\n"
    "\n";
    
typedef struct chitti_lun_s
{
    fbe_api_lun_create_t    fbe_lun_create_req;
    fbe_bool_t              expected_to_fail;
    fbe_object_id_t         lun_object_id;
}chitti_lun_t;

typedef struct chitti_lun_info_s
{
    fbe_object_id_t lun_object_id;
    fbe_lba_t offset;
    fbe_lba_t capacity;
}chitti_lun_info_t;

typedef struct chitti_dualsp_pass_config_s
{
    fbe_sim_transport_connection_target_t creation_sp;
    fbe_sim_transport_connection_target_t destroy_sp;
    fbe_sim_transport_connection_target_t reboot_sp;
}chitti_dualsp_pass_config_t;

#define CHITTI_MAX_PASS_COUNT 8

static chitti_dualsp_pass_config_t chitti_dualsp_pass_config[CHITTI_MAX_PASS_COUNT] =
{
    {FBE_SIM_SP_A, FBE_SIM_SP_A, FBE_SIM_INVALID_SERVER},/*FBE_SIM_INVALID_SERVER means no reboot*/ 
    {FBE_SIM_SP_B, FBE_SIM_SP_B, FBE_SIM_INVALID_SERVER},/*FBE_SIM_INVALID_SERVER means no reboot*/
    {FBE_SIM_SP_A, FBE_SIM_SP_B, FBE_SIM_INVALID_SERVER},/*FBE_SIM_INVALID_SERVER means no reboot*/
    {FBE_SIM_SP_B, FBE_SIM_SP_A, FBE_SIM_INVALID_SERVER},/*FBE_SIM_INVALID_SERVER means no reboot*/
    {FBE_SIM_SP_A, FBE_SIM_SP_A, FBE_SIM_SP_A}, 
    {FBE_SIM_SP_B, FBE_SIM_SP_B, FBE_SIM_SP_B},
    {FBE_SIM_SP_A, FBE_SIM_SP_B, FBE_SIM_SP_A},
    {FBE_SIM_SP_B, FBE_SIM_SP_A, FBE_SIM_SP_B},
};

/*!*******************************************************************
 * @def CHITTI_TEST_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Max number of RGs to create
 *
 *********************************************************************/
#define CHITTI_TEST_RAID_GROUP_COUNT           1

/*!*******************************************************************
 * @def CHITTI_TEST_LUN_COUNT
 *********************************************************************
 * @brief  number of luns to be created
 *
 *********************************************************************/
#define CHITTI_TEST_LUN_COUNT        2


/*!*******************************************************************
 * @def CHITTI_TEST_MAX_RG_WIDTH
 *********************************************************************
 * @brief Max number of drives in a RG
 *
 *********************************************************************/
#define CHITTI_TEST_MAX_RG_WIDTH               16


/*!*******************************************************************
 * @def CHITTI_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects to create
 *        (1 board + 1 port + 2 encl + 30 pdo ) 
 *
 *********************************************************************/
#define CHITTI_TEST_NUMBER_OF_PHYSICAL_OBJECTS 34 


/*!*******************************************************************
 * @def CHITTI_TEST_DRIVE_CAPACITY
 *********************************************************************
 * @brief Number of blocks in the physical drive
 *
 *********************************************************************/
#define CHITTI_TEST_DRIVE_CAPACITY             TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY


/*!*******************************************************************
 * @def CHITTI_TEST_RAID_GROUP_ID
 *********************************************************************
 * @brief RAID Group id used by this test
 *
 *********************************************************************/
#define CHITTI_TEST_RAID_GROUP_ID          0

#define CHITTI_INVALID_TEST_RAID_GROUP_ID  5

#define CHITTI_TEST_MAX_DRIVES_TO_REMOVE    2

#define CHITTI_TEST_CONFIGS       1

static fbe_api_rdgen_context_t fbe_chitti_test_context[CHITTI_TEST_LUN_COUNT * CHITTI_TEST_RAID_GROUP_COUNT];

/*!*******************************************************************
 * @var chitti_disk_set_520
 *********************************************************************
 * @brief This is the disk set for the 520 RG (520 byte per block SAS)
 *
 *********************************************************************/
#if 0
static fbe_u32_t chitti_disk_set_520[CHITTI_TEST_RAID_GROUP_COUNT][CHITTI_TEST_MAX_RG_WIDTH][3] = 
{
    /* width = 3 */
    { {0,0,3}, {0,0,4}, {0,0,5}}
};
#endif

static fbe_test_rg_configuration_t chitti_rg_configuration[] = 
{
    /* width, capacity,        raid type,                        class,       block size, RAID-id,                   bandwidth.*/
    { 3,     FBE_LBA_INVALID, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY, 520,        CHITTI_TEST_RAID_GROUP_ID, 0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

static fbe_u32_t drive_slots_to_be_removed[CHITTI_TEST_CONFIGS][CHITTI_TEST_MAX_DRIVES_TO_REMOVE];
fbe_u32_t number_physical_objects = 0;
fbe_u32_t removed_drive_position[FBE_RAID_MAX_DISK_ARRAY_WIDTH] = {0};
fbe_api_terminator_device_handle_t      drive_info[CHITTI_TEST_CONFIGS][CHITTI_TEST_MAX_DRIVES_TO_REMOVE] = {0};       //  drive info needed for reinsertion
chitti_lun_info_t chitti_lun_info[CHITTI_TEST_LUN_COUNT] = {0};
static fbe_bool_t is_dualsp = FBE_FALSE;

/*!*******************************************************************
 * @def CHITTI_TEST_LUN_NUMBER
 *********************************************************************
 * @brief  Lun number used by this test
 *
 *********************************************************************/
static chitti_lun_t chitti_luns [CHITTI_TEST_LUN_COUNT] = {
    {{FBE_RAID_GROUP_TYPE_RAID5, 
        CHITTI_TEST_RAID_GROUP_ID, 
        {01,02,03,04,05,06,07,00},
        {0xD3,0xE4,0xF0,0x00}, 
        123, 
        0x1000, 
        FBE_BLOCK_TRANSPORT_BEST_FIT,
        FBE_LBA_INVALID, 
        FBE_FALSE,
        FBE_FALSE},
     FBE_FALSE},
    {{FBE_RAID_GROUP_TYPE_RAID5, 
        CHITTI_TEST_RAID_GROUP_ID, 
        {01,02,03,04,05,06,07,01},
        {0xD3,0xE4,0xF1,0x00}, 
        234, 
        0x1000, 
        FBE_BLOCK_TRANSPORT_BEST_FIT,
        FBE_LBA_INVALID, 
        FBE_FALSE,
        FBE_FALSE}, 
    FBE_FALSE},
/*    {{FBE_RAID_GROUP_TYPE_RAID5, 
        CHITTI_INVALID_TEST_RAID_GROUP_ID,
        {01,02,03,04,05,06,07,03},
        {0xD3,0xE4,0xF3,0x00}, 
        999,
        0x1000, 
        FBE_BLOCK_TRANSPORT_BEST_FIT,
        FBE_LBA_INVALID,
        FBE_FALSE, 
        FBE_FALSE}, 
    FBE_TRUE},/* this lun create will fail, due to invalid RG id*/
};

/*!*******************************************************************
 * @def CHITTI_TEST_NS_TIMEOUT
 *********************************************************************
 * @brief  Notification to timeout value in milliseconds 
 *
 *********************************************************************/
#define CHITTI_TEST_NS_TIMEOUT        30000 /*wait maximum of 30 seconds*/


/*-----------------------------------------------------------------------------
    FORWARD DECLARATIONS:
*/

static void chitti_test_load_physical_config(void);
static void chitti_test_create_rg(fbe_test_rg_configuration_t *rg_config_p);
static void chitti_test_create_lun(fbe_bool_t is_ndb);
static void chitti_verify_lun_info(fbe_object_id_t in_lun_object_id, fbe_api_lun_create_t  *in_fbe_lun_create_req_p);
static void chitti_verify_lun_creation(chitti_lun_t * lun_config);
static void chitti_reboot_sp(void);
static void chitti_run_test(fbe_test_rg_configuration_t *rg_config_ptr, void *context_p);
static void chitti_test_degrade_reboot_destroy_recover_rg_luns(fbe_test_rg_configuration_t *rg_config_p);
static void chitti_test_degrade_reboot_rg_luns(fbe_test_rg_configuration_t *in_rg_config_p,
                                               fbe_u32_t drives_to_remove);
static void chitti_test_verify_rg_lun_state(fbe_lifecycle_state_t expected_state);
static void chitti_test_destroy_stuck_rg_luns(void);
static void chitti_test_destroy_lun(fbe_lun_number_t lun_number);
static void chitti_test_recover_verify_rg_luns(fbe_test_rg_configuration_t *in_rg_config_p);
static void chitti_reboot_dualsp(void);

/*-----------------------------------------------------------------------------
    FUNCTIONS:
*/

/*!****************************************************************************
 *  chitti_test
 ******************************************************************************
 *
 * @brief
 *  This is the main entry point for the Chitti test.  
 *
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *   09/15/2011 - Created. Arun S 
 *****************************************************************************/
void chitti_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    rg_config_p = &chitti_rg_configuration[0];

    fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

    fbe_test_run_test_on_rg_config_no_create(rg_config_p, 0, chitti_run_test);

    return;
}

/*!****************************************************************************
 *  chitti_run_test
 ******************************************************************************
 *
 * @brief
 *  This is the main entry point for the Chitti test.  
 *
 * @param   IN: rg_config_ptr - pointer to the RG configuration.
 *
 * @return  None 
 *
 * @author
 *   09/15/2011 - Created. Arun S 
 *****************************************************************************/
static void chitti_run_test(fbe_test_rg_configuration_t *rg_config_ptr, void *context_p)
{
    fbe_u32_t                       num_raid_groups = 0;
    fbe_u32_t                       index = 0;
    fbe_test_rg_configuration_t     *rg_config_p = NULL;

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_ptr);
    
    mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Chitti Test ===\n");

    for(index = 0; index < num_raid_groups; index++)
    {
        rg_config_p = rg_config_ptr + index;

        if(fbe_test_rg_config_is_enabled(rg_config_p))
        {
            /* Create RG */
            chitti_test_create_rg(rg_config_p);
        }
    }

    /* Create LUN's */
    chitti_test_create_lun(FBE_FALSE);

    /* Use Case: Put RG/LUN in degraded by removing drives. Reboot and verify 
     * if it is in SPECIALIZE state. If so, issue destroy. 
     */
    chitti_test_degrade_reboot_destroy_recover_rg_luns(rg_config_p);

    return;
}

/*!****************************************************************************
 *  chitti_test_degrade_reboot_destroy_recover_rg_luns
 ******************************************************************************
 *
 * @brief
 *  This function destroys the degrades, reboots, destroys the stuck RG/LUNs.  
 *
 * @param   IN: rg_config_p - Pointer to the RG configuration.
 *
 * @return  None 
 *
  * @author
 *   09/15/2011 - Created. Arun S 
*****************************************************************************/
static void chitti_test_degrade_reboot_destroy_recover_rg_luns(fbe_test_rg_configuration_t *rg_config_p)
{
    /* Degrade the RG/LUNs and reboot to get it stuck in SPECIALIZE STATE. */
    chitti_test_degrade_reboot_rg_luns(rg_config_p, 2);

    /* Destroy the stuck RG/LUN's */
    chitti_test_destroy_stuck_rg_luns();

    /* Now that we have destroyed the RG/LUN's that are stuck in 
     * SPECIALIZE state, recover those by recreating it thru NDB - 
     * NON DESTRUCTIVE BIND.
     */
    chitti_test_recover_verify_rg_luns(rg_config_p);

    return;
}

/*!****************************************************************************
 *  chitti_test_degrade_reboot_rg_luns
 ******************************************************************************
 *
 * @brief
 *  This function degrades the RG/LUNs by removing a drive and reboot.  
 *
 * @param  IN: in_rg_config_p - Pointer to the RG configuration
 *         IN: drives_to_remove - no. of drives to be removed from the RG.
 *
 * @return  None 
 *
 * @author
 *   09/15/2011 - Created. Arun S 
 *****************************************************************************/
static void chitti_test_degrade_reboot_rg_luns(fbe_test_rg_configuration_t *in_rg_config_p,
                                               fbe_u32_t drives_to_remove)
{
    fbe_u32_t i = 0;
    fbe_u32_t rg_index = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t drives_to_remove_p = 0;
    fbe_test_rg_configuration_t *rg_config_p = NULL;

	/* Get the number of physical objects in existence at this point in time.  
     * This number is used when checking if a drive has been removed or has been reinserted.
     */
	status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	/* Go through all the RG for which user has provided configuration data. */
	for (rg_index = 0; rg_index < CHITTI_TEST_CONFIGS; rg_index++)
	{
		/* Write a seed pattern to the RG */
        mut_printf(MUT_LOG_TEST_STATUS, "Writing BG Pattern...");
		sep_rebuild_utils_write_bg_pattern(&fbe_chitti_test_context[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);
	}

    rg_config_p = in_rg_config_p;
    drives_to_remove_p = drives_to_remove;
    for (rg_index = 0; rg_index < CHITTI_TEST_CONFIGS; rg_index++)
    {
        if (fbe_test_sep_util_get_cmd_option("-use_default_drives"))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "Using the Default drives...");
            drive_slots_to_be_removed[rg_index][0] = SEP_REBUILD_UTILS_POSITION_1;
            drive_slots_to_be_removed[rg_index][1] = SEP_REBUILD_UTILS_POSITION_2;
        }
        else
        {
            mut_printf(MUT_LOG_TEST_STATUS, "Using random drives...");
            sep_rebuild_utils_get_random_drives(rg_config_p, drives_to_remove_p, drive_slots_to_be_removed[rg_index]);
        }

        rg_config_p++;
        //drives_to_remove_p++;
    }

    /* Before putting the RG/LUNs do a READ and WRITE new pattern so that
     * we can verify if we have the same data once we recover the RG/LUNs
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Reading BG Pattern...");
    sep_rebuild_utils_read_bg_pattern(&fbe_chitti_test_context[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);

    rg_config_p = in_rg_config_p;
    for (rg_index = 0; rg_index < CHITTI_TEST_CONFIGS; rg_index++)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "Writing BG Pattern...0x200");
        sep_rebuild_utils_write_new_data(rg_config_p, &fbe_chitti_test_context[0], 0x200);
        rg_config_p++;
    }

    /* Remove the drives to put RG/LUN in degraded state. */
    rg_config_p = in_rg_config_p;
    drives_to_remove_p = drives_to_remove;
    for (rg_index = 0; rg_index < CHITTI_TEST_CONFIGS; rg_index++)
    {
        for(i=0; i<drives_to_remove_p; i++)
        {
            number_physical_objects -= 1;

            /* Remove drives in the RG. */
            sep_rebuild_utils_remove_drive_and_verify(rg_config_p, drive_slots_to_be_removed[rg_index][i], number_physical_objects, &drive_info[rg_index][i]);
            removed_drive_position[i] = drive_slots_to_be_removed[rg_index][i];
            mut_printf(MUT_LOG_TEST_STATUS, "Drive removed from position: %d\n", removed_drive_position[i]);
        }

        rg_config_p++;
        //drives_to_remove_p++;
    }

    /* Reboot - SP */
    if(is_dualsp == FBE_FALSE)
    {
        chitti_reboot_sp();
    }
    else
    {
        /* Dual SP */
        chitti_reboot_dualsp();
    }

    /* Verify the RG/LUNs are in SPECIALIZE state */
    mut_printf(MUT_LOG_TEST_STATUS, "Verifying the RG/LUNs are in SPECIALIZE state");
    chitti_test_verify_rg_lun_state(FBE_LIFECYCLE_STATE_SPECIALIZE);

    return;
}

/*!****************************************************************************
 *  chitti_test_verify_rg_lun_state
 ******************************************************************************
 *
 * @brief
 *  This function verifies the RG/LUNs are in expected lifecycle state.  
 *
 * @param   IN: expected_state - expected lifecycle state of the object.
 *
 * @return  None 
 *
 * @author
 *   09/15/2011 - Created. Arun S 
 *****************************************************************************/
static void chitti_test_verify_rg_lun_state(fbe_lifecycle_state_t expected_state)
{
    fbe_u32_t i = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;

    /* When a drive is removed, and the SP is rebooted RAID object would be stuck in specialize. */
    fbe_api_database_lookup_raid_group_by_number(CHITTI_TEST_RAID_GROUP_ID, &rg_object_id);
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id,
                                                     expected_state,
                                                     60000,
                                                     FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "RG %d is in expected state\n", CHITTI_TEST_RAID_GROUP_ID);

    for (i = 0; i < CHITTI_TEST_LUN_COUNT; i++)
    {
        status = fbe_api_wait_for_object_lifecycle_state(chitti_luns[i].lun_object_id,
                                                         expected_state,
                                                         60000,
                                                         FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, "LUN %d is in expected state\n", chitti_luns[i].lun_object_id);
    }

    return;
}

/*!****************************************************************************
 *  chitti_test_destroy_stuck_rg_luns
 ******************************************************************************
 *
 * @brief
 *  This function destroys the degraded/stuck RG/LUNs and recovers it.  
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *   09/15/2011 - Created. Arun S 
 *****************************************************************************/
static void chitti_test_destroy_stuck_rg_luns(void)
{
    fbe_u32_t i = 0;
    fbe_status_t status = FBE_STATUS_OK;

    /* Destory the LUN's in SPECIALIZE state. */
    for (i = 0; i < CHITTI_TEST_LUN_COUNT; i++)
    {
        /* Test destroying LUN just created */
        chitti_test_destroy_lun(chitti_luns[i].fbe_lun_create_req.lun_number);
    }

    /* Destory the RG in SPECIALIZE state. */
    status = fbe_test_sep_util_destroy_raid_group(CHITTI_TEST_RAID_GROUP_ID);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "RG destroyed successfully.\n");

    return;
}

/*!****************************************************************************
 *  chitti_test_recover_verify_rg_luns
 ******************************************************************************
 *
 * @brief
 *  This function recreates the RG/LUNs thru NON-DESTRUCTIVE BIND. After the
 *  successful recovery of the LUNs verify if we have recovered the data as well.
 *
 * @param   IN: rg_config_ptr - pointer to the RG configuration.
 *
 * @return  None 
 *
 * @author
 *   09/15/2011 - Created. Arun S 
 *****************************************************************************/
static void chitti_test_recover_verify_rg_luns(fbe_test_rg_configuration_t *rg_config_ptr)
{
    fbe_u32_t                       i = 0;
    fbe_u32_t                       num_raid_groups = 0;
    fbe_u32_t                       rg_index = 0;
    fbe_u32_t                       in_position = 0;
    fbe_test_rg_configuration_t     *rg_config_p = NULL;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Re-Inserting Drives ===\n");

    /* First re-insert the drives */
    rg_config_p = rg_config_ptr;
    for (rg_index = 0; rg_index < CHITTI_TEST_CONFIGS; rg_index++)
    {
        /* Reinsert the drive */
        for(i=0; i < CHITTI_TEST_MAX_DRIVES_TO_REMOVE; i++)
        {
            number_physical_objects += 2;

            in_position = removed_drive_position[i];
            /* Reinsert the removed drives */
            sep_rebuild_utils_reinsert_removed_drive(rg_config_ptr->rg_disk_set[in_position].bus, 
                                                     rg_config_ptr->rg_disk_set[in_position].enclosure, 
                                                     rg_config_ptr->rg_disk_set[in_position].slot,
                                                     &drive_info[rg_index][i]);

            mut_printf(MUT_LOG_TEST_STATUS, "Drive inserted in position: %d\n", in_position);
        }

        rg_config_p++;
    }

    /* Now recreate the RG/LUNs */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Recreating RG/LUNs ===\n");

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_ptr);
    for(rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        rg_config_p = rg_config_ptr + rg_index;

        if(fbe_test_rg_config_is_enabled(rg_config_p))
        {
            /* Create RG */
            chitti_test_create_rg(rg_config_p);
        }
    }

    /* Create LUN's with NDB (Non-Destructive Bind) */
    chitti_test_create_lun(FBE_TRUE);

    /* Now that we have recovered the RG/LUNs by recreating it via NDB,
     * verify if we have the same data we wrote before destroying it. 
     */
    rg_config_p = rg_config_ptr;
    for (rg_index = 0; rg_index < CHITTI_TEST_CONFIGS; rg_index++)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "Reading and Verifying BG Pattern...0x200");
        sep_rebuild_utils_verify_new_data(rg_config_p, &fbe_chitti_test_context[0], 0x200);
        rg_config_p++;
    }

    return;
}

/*!****************************************************************************
 *  chitti_test_create_lun
 ******************************************************************************
 *
 * @brief
 *  This function creates LUNs for Chitti test.  
 *
 * @param   IN: is_ndb - Non-destructive flag.
 *
 * @return  None 
 *
 * @author
 *   09/15/2011 - Created. Arun S 
 *****************************************************************************/
static void chitti_test_create_lun(fbe_bool_t is_ndb)
{
    fbe_status_t   status;
    fbe_u32_t      i = 0;
    fbe_job_service_error_type_t    job_error_type;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Creating LUNs ===\n");

    for (i = 0; i < CHITTI_TEST_LUN_COUNT; i++)
    {
        if(is_ndb)
        {
            /* NDB (Non-Destructive Bind) mode is set */
            chitti_luns[i].fbe_lun_create_req.ndb_b = FBE_TRUE;

            /* Set the offset and capacity of the destroyed LUN's */
            chitti_luns[i].fbe_lun_create_req.addroffset = chitti_lun_info[i].offset;
            chitti_luns[i].fbe_lun_create_req.capacity = chitti_lun_info[i].capacity;

            mut_printf(MUT_LOG_TEST_STATUS, "=== Creating LUNs - NDB ===\n");
        }

        status = fbe_api_create_lun(&chitti_luns[i].fbe_lun_create_req, 
                                    FBE_TRUE, 
                                    CHITTI_TEST_NS_TIMEOUT, 
                                    &chitti_luns[i].lun_object_id,
                                    &job_error_type);

        if (chitti_luns[i].expected_to_fail)
        {
            MUT_ASSERT_INT_NOT_EQUAL_MSG(status, FBE_STATUS_OK, "fbe_api_create_lun expected to fail, but it succeeded!!!"); 
        }
        else
        {
            MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "fbe_api_create_lun failed!!!"); 
        }

        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, "Created LUN object %d\n", chitti_luns[i].lun_object_id);

        /* Store the lun_object_id in lun_info for tracking. */
        chitti_lun_info[i].lun_object_id = chitti_luns[i].lun_object_id;

        /* Verify the LUN create result */
        chitti_verify_lun_creation(&chitti_luns[i]);
    }
  
    return;

}/* End chitti_test() */


/*!****************************************************************************
 *  chitti_setup
 ******************************************************************************
 *
 * @brief
 *  This is the setup function for the Chitti test. 
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void chitti_setup(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Chitti test ===\n");

    /* Load the physical config on the target server */    
    chitti_test_load_physical_config();

    /* Load the SEP package on the target server */
    sep_config_load_sep_and_neit();

    fbe_test_common_util_test_setup_init();

    return;
} /* End chitti_setup() */


/*!****************************************************************************
 *  chitti_cleanup
 ******************************************************************************
 *
 * @brief
 *  This is the cleanup function for the Chitti test.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *   09/15/2011 - Created. Arun S 
 *****************************************************************************/
void chitti_cleanup(void)
{  
    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for Chitti test ===\n");

    fbe_test_sep_util_destroy_neit_sep_physical();

    return;
} /* End chitti_cleanup() */


/*!**************************************************************
 * chitti_test_load_physical_config()
 ****************************************************************
 * @brief
 *  Configure the Chitti test physical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 * @author
 *   09/15/2011 - Created. Arun S 
 ****************************************************************/

static void chitti_test_load_physical_config(void)
{
    fbe_status_t                            status;
    fbe_u32_t                               object_handle;
    fbe_u32_t                               port_idx;
    fbe_u32_t                               enclosure_idx;
    fbe_u32_t                               drive_idx;
    fbe_u32_t                               total_objects;
    fbe_class_id_t                          class_id;
    fbe_api_terminator_device_handle_t      port0_handle;
    fbe_api_terminator_device_handle_t      encl0_0_handle;
    fbe_api_terminator_device_handle_t      encl0_1_handle;
    fbe_api_terminator_device_handle_t      drive_handle;
    fbe_u32_t                               slot;

    /* Initialize local variables */
    status          = FBE_STATUS_GENERIC_FAILURE;
    object_handle   = 0;
    port_idx        = 0;
    enclosure_idx   = 0;
    drive_idx       = 0;
    total_objects   = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "=== configuring terminator ===\n");
    /* before loading the physical package we initialize the terminator */
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Insert a board */
    status = fbe_test_pp_util_insert_armada_board();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Insert a port */
    fbe_test_pp_util_insert_sas_pmc_port(   1, /* io port */
                                            2, /* portal */
                                            0, /* backend number */ 
                                            &port0_handle);

    /* Insert enclosures to port 0 */
    fbe_test_pp_util_insert_viper_enclosure(0, 0, port0_handle, &encl0_0_handle);
    fbe_test_pp_util_insert_viper_enclosure(0, 1, port0_handle, &encl0_1_handle);

    /* Insert drives for enclosures. */
    for (slot = 0; slot < 15; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 0, slot, 520, CHITTI_TEST_DRIVE_CAPACITY, &drive_handle);
    }

    for (slot = 0; slot < 15; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 1, slot, 520, CHITTI_TEST_DRIVE_CAPACITY, &drive_handle);
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
    status = fbe_api_wait_for_number_of_objects(CHITTI_TEST_NUMBER_OF_PHYSICAL_OBJECTS, READY_STATE_WAIT_TIME, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== verifying configuration ===\n");

    /* Inserted a armada board so we check for it;
     * board is always assumed to be object id 0
     */
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying board type....Passed");

    /* Make sure we have the expected number of objects. */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == CHITTI_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");

    return;

} /* End chitti_test_load_physical_config() */


/*!**************************************************************
 * chitti_verify_lun_info()
 ****************************************************************
 * @brief
 *  Verifies the lun info.
 *
 * @param IN: in_lun_object_id - Object-id of the LUN.
 *        IN: in_lun_create_req_p - Pointer to the lun creation request.
 *
 * @return None.
 *
 * @author
 *   09/15/2011 - Created. Arun S 
 ****************************************************************/
static void chitti_verify_lun_info(fbe_object_id_t in_lun_object_id, fbe_api_lun_create_t  *in_fbe_lun_create_req_p)
{
    fbe_status_t status;
    fbe_database_lun_info_t lun_info;
    fbe_object_id_t rg_obj_id;
    fbe_u32_t lun_index = 0;

    lun_info.lun_object_id = in_lun_object_id;
    status = fbe_api_database_get_lun_info(&lun_info);
    MUT_ASSERT_INTEGER_EQUAL_MSG(status, FBE_STATUS_OK, "fbe_api_lun_get_info failed!");

    /* check rd group obj_id*/
    status = fbe_api_database_lookup_raid_group_by_number(in_fbe_lun_create_req_p->raid_group_id, &rg_obj_id);
    MUT_ASSERT_INTEGER_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INTEGER_EQUAL_MSG(rg_obj_id, lun_info.raid_group_obj_id, "Lun_get_info return mismatch rg_obj_id");

    MUT_ASSERT_INTEGER_EQUAL_MSG(in_fbe_lun_create_req_p->capacity, lun_info.capacity, "Lun_get_info return mismatch capacity" );
    MUT_ASSERT_INTEGER_EQUAL_MSG(in_fbe_lun_create_req_p->raid_type, lun_info.raid_info.raid_type, "Lun_get_info return mismatch raid_type" );
    MUT_ASSERT_BUFFER_EQUAL_MSG((char *)&in_fbe_lun_create_req_p->world_wide_name, 
                                (char *)&lun_info.world_wide_name, 
                                sizeof(lun_info.world_wide_name),
                                "lun_get_info return mismatch WWN!");

    MUT_ASSERT_BUFFER_EQUAL_MSG((char *)&in_fbe_lun_create_req_p->user_defined_name, 
                                (char *)&lun_info.user_defined_name, 
                                sizeof(lun_info.user_defined_name),
                                "lun_get_info return mismatch user_defined_name!");

    /* Store the address offset and capacity of the LUN. We may need it while recreating the LUNs thru NDB. */
    for(lun_index = 0; lun_index < CHITTI_TEST_LUN_COUNT; lun_index++)
    {
        if(chitti_lun_info[lun_index].lun_object_id == lun_info.lun_object_id)
        {
            chitti_lun_info[lun_index].offset = lun_info.offset;
            chitti_lun_info[lun_index].capacity = lun_info.capacity;
        }
    }
}

/*!**************************************************************
 * chitti_verify_lun_creation()
 ****************************************************************
 * @brief
 *  Verifies the lun creation.
 *
 * @param IN: lun_config - pointer to the lun_config structure.              
 *
 * @return None.
 *
 * @author
 *   09/15/2011 - Created. Arun S 
 ****************************************************************/
static void chitti_verify_lun_creation(chitti_lun_t * lun_config)
{
    fbe_sim_transport_connection_target_t active_target;
    fbe_sim_transport_connection_target_t passive_target;

    fbe_bool_t b_run_on_dualsp = fbe_test_sep_util_get_dualsp_test_mode();

    /* Get the active SP */
    fbe_test_sep_get_active_target_id(&active_target);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, active_target);

    /* set the passive SP */
    passive_target = (active_target == FBE_SIM_SP_A ? FBE_SIM_SP_B : FBE_SIM_SP_A);

    /* verify on the active SP first */
    fbe_api_sim_transport_set_target_server(active_target);

    if ( !lun_config->expected_to_fail )
    {
        chitti_verify_lun_info(lun_config->lun_object_id, &lun_config->fbe_lun_create_req);
    }

    mut_printf(MUT_LOG_LOW, "=== Verify on ACTIVE side (%s) Passed ===", active_target == FBE_SIM_SP_A ? "SP A" : "SP B");

    if ( b_run_on_dualsp )
    {
        /* verify on the passive SP */
        fbe_api_sim_transport_set_target_server(passive_target);

        if ( !lun_config->expected_to_fail )
        {
            chitti_verify_lun_info(lun_config->lun_object_id, &lun_config->fbe_lun_create_req);
        }

        mut_printf(MUT_LOG_LOW, "=== Verify on Passive side (%s) Passed ===", passive_target == FBE_SIM_SP_A ? "SP A" : "SP B");
    }
}


/*!****************************************************************************
 *  chitti_dualsp_setup
 ******************************************************************************
 *
 * @brief
 *  This is the setup function for the Chitti test in dualsp mode. It is responsible
 *  for loading the physical and logical configuration for this test suite on both SPs.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *   09/15/2011 - Created. Arun S 
 *****************************************************************************/
void chitti_dualsp_setup(void)
{
    fbe_sim_transport_connection_target_t   sp;

    if (fbe_test_util_is_simulation())
    { 
        mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Chitti dualsp test ===\n");
        /* Set the target server */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    
        /* Make sure target set correctly */
        sp = fbe_api_sim_transport_get_target_server();
        MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_B, sp);
    
        mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n", 
                   sp == FBE_SIM_SP_A ? "SP A" : "SP B");

        /* Load the physical config on the target server */
        chitti_test_load_physical_config();
    
        /* Set the target server */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    
        /* Make sure target set correctly */
        sp = fbe_api_sim_transport_get_target_server();
        MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_A, sp);
    
        mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n", 
                   sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    
        /* Load the physical config on the target server */    
        chitti_test_load_physical_config();

        sep_config_load_sep_and_neit_load_balance_both_sps();

    }

    return;

} /* End chitti_setup() */


/*!****************************************************************************
 *  chitti_dualsp_cleanup
 ******************************************************************************
 *
 * @brief
 *  This is the cleanup function for the Chitti test.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *   09/15/2011 - Created. Arun S 
 *****************************************************************************/
void chitti_dualsp_cleanup(void)
{  
    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for Chitti test ===\n");

    /* Destroy the test configuration on both SPs */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_destroy_neit_sep_physical();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_test_sep_util_destroy_neit_sep_physical();

    return;
} /* End chitti_dualsp_cleanup() */

/*!****************************************************************************
 *  chitti_reboot_sp
 ******************************************************************************
 *
 * @brief
 *  This is the reboot SP function for the Chitti test.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *   09/15/2011 - Created. Arun S 
 *****************************************************************************/
static void chitti_reboot_sp(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    /*reboot sep*/
    fbe_test_sep_util_destroy_neit_sep();
    sep_config_load_sep_and_neit();
    status = fbe_test_sep_util_wait_for_database_service(CHITTI_TEST_NS_TIMEOUT);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_api_database_set_load_balance(FBE_TRUE);
}

/*!****************************************************************************
 *  chitti_reboot_dualsp
 ******************************************************************************
 *
 * @brief
 *  This is the reboot SP function for the Chitti test.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *   09/15/2011 - Created. Arun S 
 *****************************************************************************/
static void chitti_reboot_dualsp(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    /*reboot sep*/
    fbe_test_sep_util_destroy_neit_sep_both_sps();

    sep_config_load_sep_and_neit_load_balance_both_sps();

    status = fbe_test_sep_util_wait_for_database_service(CHITTI_TEST_NS_TIMEOUT);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

}

/*!****************************************************************************
 *  chitti_dualsp_test
 ******************************************************************************
 *
 * @brief
 *  This is the dual SP test for the Chitti test.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *   09/15/2011 - Created. Arun S 
 *****************************************************************************/
void chitti_dualsp_test(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_sim_transport_connection_target_t active_target;

    fbe_u32_t pass;

    /* Enable dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    for (pass = 0 ; pass < 1 /* CHITTI_MAX_PASS_COUNT */; pass++)
    {
        /* Get the active SP */
        fbe_test_sep_get_active_target_id(&active_target);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, active_target);
        mut_printf(MUT_LOG_TEST_STATUS, "=== DUAL SP test pass %d: Active SP is %s ===\n", pass,
                   active_target == FBE_SIM_SP_A ? "SP A" : "SP B");

        fbe_api_sim_transport_set_target_server(chitti_dualsp_pass_config[pass].creation_sp);
        mut_printf(MUT_LOG_TEST_STATUS, "=== DUAL SP test pass %d: Create LUNs on %s ===\n", pass,
                   chitti_dualsp_pass_config[pass].creation_sp == FBE_SIM_SP_A ? "SP A" : "SP B");

        /* Set the dualSP mode */
        is_dualsp = FBE_TRUE;

        /* Now run the create tests */
        chitti_test();
    
        /* reboot */
        if (chitti_dualsp_pass_config[pass].reboot_sp != FBE_SIM_INVALID_SERVER)
        {
            fbe_api_sim_transport_set_target_server(chitti_dualsp_pass_config[pass].reboot_sp);
            mut_printf(MUT_LOG_TEST_STATUS, "=== DUAL SP test pass %d: Rebooting %s ===\n", pass,
                       chitti_dualsp_pass_config[pass].reboot_sp == FBE_SIM_SP_A ? "SP A" : "SP B");

            chitti_reboot_sp();
        }

        /* Destroy RG/LUNs */
        fbe_api_sim_transport_set_target_server(chitti_dualsp_pass_config[pass].destroy_sp);
        mut_printf(MUT_LOG_TEST_STATUS, "=== DUAL SP test pass %d: Destroy RG and LUNs on %s ===\n", pass,
                   chitti_dualsp_pass_config[pass].destroy_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
        status = fbe_test_sep_util_destroy_all_user_luns();
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_test_sep_util_destroy_raid_group(CHITTI_TEST_RAID_GROUP_ID);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Reboot the SPs to start clean.. */
        chitti_reboot_dualsp();
    }

    /* Always clear dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
}


/*!****************************************************************************
 *  chitti_test_destroy_lun
 ******************************************************************************
 *
 * @brief
 *   This is the test function for destroying a LUN.  
 *
 * @param   lun_number - LUN # to destroy
 *
 * @return  None 
 *
 * @author
 *   09/15/2011 - Created. Arun S 
 *****************************************************************************/
static void chitti_test_destroy_lun(fbe_lun_number_t   lun_number)
{
    fbe_status_t                    status;
    fbe_api_lun_destroy_t			fbe_lun_destroy_req;
	fbe_object_id_t                 lun_object_id;
    fbe_job_service_error_type_t    job_error_type;
    
    /* Destroy a LUN */
    fbe_lun_destroy_req.lun_number = lun_number;
	
    status = fbe_api_database_lookup_lun_by_number(fbe_lun_destroy_req.lun_number, &lun_object_id);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Issue LUN destroy! ===\n");

    status = fbe_api_destroy_lun(&fbe_lun_destroy_req, FBE_TRUE,  CHITTI_TEST_NS_TIMEOUT, &job_error_type);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    /* Get object ID for LUN should fail, because it is just destroyed */
    status = fbe_api_database_lookup_lun_by_number(fbe_lun_destroy_req.lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_GENERIC_FAILURE, status);
    mut_printf(MUT_LOG_TEST_STATUS, "LUN destroyed successfully.\n");

    return;
}  /* End chitti_test_destroy_lun() */

/*!**************************************************************
 * chitti_test_create_rg()
 ****************************************************************
 * @brief
 *  Configure the Raid Group for Chitti test.
 *
 * @param None.              
 *
 * @return None.
 *
 * @author
 *   09/15/2011 - Created. Arun S 
 ****************************************************************/

static void chitti_test_create_rg(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                                status;
    fbe_object_id_t                             rg_object_id;
    fbe_object_id_t                             rg_object_id_from_job = FBE_OBJECT_ID_INVALID;
    fbe_job_service_error_type_t                job_error_code;
    fbe_status_t                                job_status;

    /* Initialize local variables */
    rg_object_id = FBE_OBJECT_ID_INVALID;

    /* Create a RG */
    mut_printf(MUT_LOG_TEST_STATUS, "=== create a RAID Group ===\n");
    status = fbe_test_sep_util_create_raid_group(rg_config_p);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "RG creation failed");

    /* wait for notification from job service. */
    status = fbe_api_common_wait_for_job(rg_config_p->job_number,
                                         CHITTI_TEST_NS_TIMEOUT,
                                         &job_error_code,
                                         &job_status,
                                         &rg_object_id_from_job);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to created RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    /* Verify the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(
             rg_config_p->raid_group_id, &rg_object_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
    MUT_ASSERT_INT_EQUAL(rg_object_id_from_job, rg_object_id);

    /* Verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY, 
                                                     READY_STATE_WAIT_TIME, FBE_PACKAGE_ID_SEP_0);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "Created Raid Group object %d\n", rg_object_id);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: END\n", __FUNCTION__);

    return;

} /* End chitti_test_create_rg() */

