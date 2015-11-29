
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file peep_test.c
 ***************************************************************************
 *
 * @brief
 *   This file includes for Binding/Unbinding LUNs when degraded/rebuilding.
 *
 * @version
 * 
 * @author
 *   09/22/2011 - Created. Arun S 
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
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe/fbe_random.h"

/*-----------------------------------------------------------------------------
    TEST DESCRIPTION:
*/

char* peep_short_desc = "Bind/Unbind LUNs when degraded/rebuilding - RAID 5";
char* peep_long_desc =
    "\n"
    "\n"
    "The Peep scenario tests destroying/recovering degraded/stuck RG/LUNs\n"
    "\n"
    "\n"
    "*** Peep **************************************************************\n"
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
    "STEP 2: Bind LUNs \n"
    "    - Bind LUNs on the already created Raid group.\n"
    "    - Verify that the new LUN Objects are created successfully by\n"
    "      verifying the following:\n"
    "       - LUN Object in the READY state\n" 
    "\n"
    "STEP 3: Bind/Unbind LUNs when the RG/LUN is degraded.\n"
    "    - Degraded RG/LUNs.\n"
    "       - Write some data pattern to the disk before degrading the RG/LUN.\n"
    "       - Remove # of drives which can put the RG/LUN degraded.\n"
    "       - Verify the drive PDO and LDO is removed.\n"
    "       - Verify the VD/PVD all put to FAIL state.\n"
    "       - Verify the RG/LUN stays in READY state.\n"
    "       - Verify the rebuild logging is set for the removed drive.\n"
    "    - Unbind/Bind LUNs when the RG/LUN is in degraded state.\n"
    "       - Unbound a LUN and verify it is successfull.\n"
    "       - Bind some new LUNs and verify if it gets created successfully.\n"
    "    - Recover the degraded RG/LUN.\n"
    "       - Re-insert the removed drive.\n"
    "       - Verify the PDO, LDO, PVD, VD all trasition back to READY state.\n"
    "       - Verify the RG and LUN in READY state.\n"
    "    - Verify the data pattern.\n"
    "       - Read back the data pattern from the recovered RG/LUNs.\n"
    "       - Verify if it is the same that we wrote before degrading.\n"
    "    - Restore the original RG/LUN config for the next use case.\n"
    "\n"
    "STEP 4: Bind/Unbind LUNs when the drive is marked NR.\n"
    "    - Degraded RG/LUNs.\n"
    "       - Write some data pattern to the disk before degrading the RG/LUN.\n"
    "       - Remove # of drives which can put the RG/LUN degraded.\n"
    "       - Verify the drive PDO and LDO is removed.\n"
    "       - Verify the VD/PVD all put to FAIL state.\n"
    "       - Verify the RG/LUN stays in READY state.\n"
    "       - Verify the rebuild logging is set for the removed drive.\n"
    "    - Disable/Stop the background operation (REBUILD) from running.\n"
    "    - Recover the degraded RG/LUN.\n"
    "       - Re-insert the removed drive.\n"
    "       - Verify the PDO, LDO, PVD, VD all trasition back to READY state.\n"
    "       - Verify the RG and LUN in READY state.\n"
    "    - Verify the rebuild logging bit is cleared and checkpoint is set to 0.\n"
    "    - Unbind/Bind LUNs when the drive is marked NR.\n"
    "       - Unbound a LUN and verify it is successfull.\n"
    "       - Bind some new LUNs and verify if it gets created successfully.\n"
    "    - Verify the data pattern.\n"
    "       - Read back the data pattern from the recovered RG/LUNs.\n"
    "       - Verify if it is the same that we wrote before degrading.\n"
    "    - Enable/Start the background operation (REBUILD) to run.\n"
    "    - Wait for the rebuild to complete.\n"
    "    - Restore the original RG/LUN config for the next use case.\n"
    "\n"
    "STEP 5: Bind/Unbind LUNs when RG/LUN is rebuilding.\n"
    "    - Degraded RG/LUNs.\n"
    "       - Write some data pattern to the disk before degrading the RG/LUN.\n"
    "       - Remove # of drives which can put the RG/LUN degraded.\n"
    "       - Verify the drive PDO and LDO is removed.\n"
    "       - Verify the VD/PVD all put to FAIL state.\n"
    "       - Verify the RG/LUN stays in READY state.\n"
    "       - Verify the rebuild logging is set for the removed drive.\n"
    "    - Add a scheduler hook for the rebuild to stop at checkpoint 0x800.\n"
    "    - Recover the degraded RG/LUN.\n"
    "       - Re-insert the removed drive.\n"
    "       - Verify the PDO, LDO, PVD, VD all trasition back to READY state.\n"
    "       - Verify the RG and LUN in READY state.\n"
    "    - Wait for the rebuild to start.\n"
    "    - Verify that the rebuild starts and stop at checkpoint 0x800.\n"
    "    - Unbind/Bind LUNs when the RG/LUN is in degraded state.\n"
    "       - Unbound a LUN and verify it is successfull.\n"
    "       - Bind some new LUNs and verify if it gets created successfully.\n"
    "    - Verify the data pattern.\n"
    "       - Read back the data pattern from the recovered RG/LUNs.\n"
    "       - Verify if it is the same that we wrote before degrading.\n"
    "    - Delete the scheduler hook and let the rebuild continue and complete.\n"
    "    - Wait for the rebuild to complete.\n"
    "    - Restore the original RG/LUN config for the next use case.\n"
    "\n"
    "STEP 6: Bind/Unbind LUNs when paged metadata for the RG is marked NR.\n"
    "    - Degraded RG/LUNs.\n"
    "       - Write some data pattern to the disk before degrading the RG/LUN.\n"
    "       - Remove # of drives which can put the RG/LUN degraded.\n"
    "       - Verify the drive PDO and LDO is removed.\n"
    "       - Verify the VD/PVD all put to FAIL state.\n"
    "       - Verify the RG/LUN stays in READY state.\n"
    "       - Verify the rebuild logging is set for the removed drive.\n"
    "    - Disable/Stop the background operation (PAGED METADATA REBUILD) from running.\n"
    "    - Recover the degraded RG/LUN.\n"
    "       - Re-insert the removed drive.\n"
    "       - Verify the PDO, LDO, PVD, VD all trasition back to READY state.\n"
    "       - Verify the RG and LUN in READY state.\n"
    "    - Wait for the rebuild to start.\n"
    "    - Verify the paged metadata NR is set but not started the rebuild.\n"
    "    - Unbind/Bind LUNs when the RG/LUN is in degraded state.\n"
    "       - Unbound a LUN and verify it is successfull.\n"
    "       - Bind some new LUNs and verify if it gets created successfully.\n"
    "    - Verify the data pattern.\n"
    "       - Read back the data pattern from the recovered RG/LUNs.\n"
    "       - Verify if it is the same that we wrote before degrading.\n"
    "    - Enable/Start the background operation (PAGED METADATA REBUILD) to run.\n"
    "    - Wait for the rebuild to complete.\n"
    "    - Restore the original RG/LUN config for the next use case.\n"
    "\n"
    "STEP 7: Destroy raid groups and LUNs.\n"
    "\n"
    "Description last updated: 09/30/2011.\n"
    "\n"
    "\n";
    
typedef struct peep_lun_s
{
    fbe_api_lun_create_t    fbe_lun_create_req;
    fbe_bool_t              expected_to_fail;
    fbe_object_id_t         lun_object_id;
}peep_lun_t;


/*!*******************************************************************
 * @def PEEP_TEST_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Max number of RGs to create
 *
 *********************************************************************/
#define PEEP_TEST_RAID_GROUP_COUNT           1

/*!*******************************************************************
 * @def PEEP_TEST_LUN_COUNT
 *********************************************************************
 * @brief  number of luns to be created
 *
 *********************************************************************/
#define PEEP_TEST_LUN_COUNT        2

/*!*******************************************************************
 * @def PEEP_TEST_EXTRA_LUN_COUNT
 *********************************************************************
 * @brief  number of extra luns to be created
 *
 *********************************************************************/
#define PEEP_TEST_EXTRA_LUN_COUNT        2

/*!*******************************************************************
 * @def PEEP_TEST_TOTAL_LUNS
 *********************************************************************
 * @brief  Total number of luns
 *
 *********************************************************************/
#define PEEP_TEST_TOTAL_LUNS        (PEEP_TEST_LUN_COUNT+PEEP_TEST_EXTRA_LUN_COUNT)

/*!*******************************************************************
 * @def PEEP_TEST_STARTING_LUN
 *********************************************************************
 * @brief  Starting lun number
 *
 *********************************************************************/
#define PEEP_TEST_STARTING_LUN       0xF

/*!*******************************************************************
 * @def PEEP_TEST_STARTING_EXTRA_LUN
 *********************************************************************
 * @brief  Starting lun number for extra luns 
 *
 *********************************************************************/
#define PEEP_TEST_STARTING_EXTRA_LUN       0xFF

/*!*******************************************************************
 * @def PEEP_TEST_MAX_RG_WIDTH
 *********************************************************************
 * @brief Max number of drives in a RG
 *
 *********************************************************************/
#define PEEP_TEST_MAX_RG_WIDTH               16


/*!*******************************************************************
 * @def PEEP_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects to create
 *        (1 board + 1 port + 2 encl + 30 pdo) 
 *
 *********************************************************************/
#define PEEP_TEST_NUMBER_OF_PHYSICAL_OBJECTS 34 


/*!*******************************************************************
 * @def PEEP_TEST_DRIVE_CAPACITY
 *********************************************************************
 * @brief Number of blocks in the physical drive
 *
 *********************************************************************/
#define PEEP_TEST_DRIVE_CAPACITY             TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY


/*!*******************************************************************
 * @def PEEP_TEST_NS_TIMEOUT
 *********************************************************************
 * @brief  Notification to timeout value in milliseconds 
 *
 *********************************************************************/
#define PEEP_TEST_NS_TIMEOUT        30000 /*wait maximum of 30 seconds*/

/*!*******************************************************************
 * @def PEEP_TEST_RAID_GROUP_ID
 *********************************************************************
 * @brief RAID Group id used by this test
 *
 *********************************************************************/
#define PEEP_TEST_RAID_GROUP_ID          0

#define PEEP_INVALID_TEST_RAID_GROUP_ID  5

#define PEEP_TEST_MAX_DRIVES_TO_REMOVE    2

#define PEEP_TEST_CONFIGS       1

static fbe_api_rdgen_context_t fbe_peep_test_context[PEEP_TEST_LUN_COUNT * PEEP_TEST_RAID_GROUP_COUNT];

static fbe_test_rg_configuration_t peep_rg_configuration[] = 
{
    /* width,                                   capacity,                       raid type,             class,         block size, RAID-id, bandwidth.*/
    {SEP_REBUILD_UTILS_R5_RAID_GROUP_WIDTH, SEP_REBUILD_UTILS_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY, 520, 0, 0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

static peep_lun_t peep_luns [PEEP_TEST_TOTAL_LUNS] = {0};
static fbe_u32_t drive_slots_to_be_removed[PEEP_TEST_CONFIGS][PEEP_TEST_MAX_DRIVES_TO_REMOVE];
fbe_u32_t peep_number_physical_objects = 0;
fbe_u32_t peep_total_luns = 0;
fbe_api_terminator_device_handle_t      peep_drive_info[PEEP_TEST_CONFIGS][PEEP_TEST_MAX_DRIVES_TO_REMOVE] = {0};       //  drive info needed for reinsertion


/*-----------------------------------------------------------------------------
    FORWARD DECLARATIONS:
*/

static void peep_test_load_physical_config(void);
static void peep_test_create_rg(fbe_test_rg_configuration_t *rg_config_p);
static void peep_test_create_lun(fbe_bool_t more_luns);
static void peep_verify_lun_info(fbe_object_id_t in_lun_object_id, fbe_api_lun_create_t  *in_fbe_lun_create_req_p);
static void peep_verify_lun_creation(peep_lun_t * lun_config);
static void peep_reboot_sp(void);
static void peep_run_test(fbe_test_rg_configuration_t *rg_config_ptr, void *context_p);
static void peep_test_degrade_rg_luns(fbe_test_rg_configuration_t *in_rg_config_p, 
                                      fbe_u32_t drives_to_remove);
static void peep_test_verify_rg_lun_state(fbe_lifecycle_state_t expected_state, fbe_bool_t more_luns);
static void peep_test_destroy_lun(fbe_lun_number_t lun_number);
static void peep_test_recover_rg_luns(fbe_test_rg_configuration_t *in_rg_config_p, 
                                      fbe_u32_t drives_to_insert, fbe_bool_t more_luns);
static void peep_reboot_dualsp(void);
static void peep_test_unbind_bind_luns(fbe_test_rg_configuration_t *rg_config_p);
static void peep_test_restore_original_config(fbe_bool_t more_luns);
static void peep_test_bind_unbind_when_degraded(fbe_test_rg_configuration_t *rg_config_p);
static void peep_test_bind_unbind_when_NR_marked(fbe_test_rg_configuration_t *rg_config_p);
static void peep_test_bind_unbind_when_rebuilding(fbe_test_rg_configuration_t *rg_config_p);
static void peep_test_bind_unbind_when_paged_metadata_marked_NR(fbe_test_rg_configuration_t *in_rg_config_p);
static void peep_test_set_hook(fbe_object_id_t rg_object_id, fbe_lba_t checkpoint);
static void peep_test_unset_hook(fbe_object_id_t rg_object_id, fbe_lba_t checkpoint);
static void peep_test_verify_rg_paged_metadata_info(fbe_object_id_t rg_object_id);

/*-----------------------------------------------------------------------------
    FUNCTIONS:
*/

/*!****************************************************************************
 *  peep_test
 ******************************************************************************
 *
 * @brief
 *  This is the main entry point for the Peep test.  
 *
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *   09/22/2011 - Created. Arun S 
 *****************************************************************************/
void peep_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    rg_config_p = &peep_rg_configuration[0];

    fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

    fbe_test_run_test_on_rg_config_no_create(rg_config_p, 0, peep_run_test);

    return;
}

/*!****************************************************************************
 *  peep_run_test
 ******************************************************************************
 *
 * @brief
 *  This is the main entry point for the Peep test.  
 *
 * @param   IN: rg_config_ptr - pointer to the RG configuration.
 *
 * @return  None 
 *
 * @author
 *   09/22/2011 - Created. Arun S 
 *****************************************************************************/
static void peep_run_test(fbe_test_rg_configuration_t *rg_config_ptr, void *context_p)
{
    fbe_u32_t                       num_raid_groups = 0;
    fbe_u32_t                       index = 0;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t     *rg_config_p = NULL;

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_ptr);
    
    mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Peep Test ===\n");

    for(index = 0; index < num_raid_groups; index++)
    {
        rg_config_p = rg_config_ptr + index;

        if(fbe_test_rg_config_is_enabled(rg_config_p))
        {
            /* Create RG */
            peep_test_create_rg(rg_config_p);
        }
    }

    /* Create LUN's */
    peep_test_create_lun(FBE_FALSE);

    /* Case 1: Bind/Unbind LUNs when the LUN is in degraded mode. */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Use Case 1 ===\n");
    mut_printf(MUT_LOG_TEST_STATUS, "=== Bind/Unbind LUNs when the LUN is in degraded mode ===\n");
    peep_test_bind_unbind_when_degraded(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "=== End of Use Case 1 ===\n");

    /* Case 2: Bind/Unbind LUNs when the LUN is marked NR. */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Use Case 2 ===\n");
    mut_printf(MUT_LOG_TEST_STATUS, "=== Bind/Unbind LUNs when the LUN is degraded and drive is marked NR. ===\n");
    peep_test_bind_unbind_when_NR_marked(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "=== End of Use Case 2 ===\n");

    /* Case 3: Bind/Unbind LUNs when the LUN is rebuilding. */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Use Case 3 ===\n");
    mut_printf(MUT_LOG_TEST_STATUS, "=== Bind/Unbind LUNs when the LUN is rebuilding. ===\n");
    peep_test_bind_unbind_when_rebuilding(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "=== End of Use Case 3 ===\n");

    /* Case 4: Create/Destroy RG when paged MD is marked NR. */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Use Case 4 ===\n");
    mut_printf(MUT_LOG_TEST_STATUS, "=== Bind/Unbind LUNs when the RG is marked NR in paged metadata. ===\n");
    peep_test_bind_unbind_when_paged_metadata_marked_NR(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "=== End of Use Case 4 ===\n");

    /* Unbind all user LUNs..  */
    status = fbe_test_sep_util_destroy_all_user_luns();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Destroy RGs..  */
    status = fbe_test_sep_util_destroy_raid_group(rg_config_p->raid_group_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;
}

/*!****************************************************************************
 *  peep_test_bind_unbind_when_degraded
 ******************************************************************************
 *
 * @brief
 *  This function bind/unbinds LUNs when RG or LUN is in degraded mode. 
 *
 * @param   IN: rg_config_p - Pointer to the RG configuration.
 *
 * @return  None 
 *
  * @author
 *   09/22/2011 - Created. Arun S 
*****************************************************************************/
static void peep_test_bind_unbind_when_degraded(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t           i = 0;
    fbe_u32_t           rg_index = 0;

    /* First make the RG/LUN degraded by removing drive(s) */
    peep_test_degrade_rg_luns(rg_config_p, 1);

    /* Unbind/Bind Luns */
    peep_test_unbind_bind_luns(rg_config_p);

    /* Re-insert the drive(s) and recover the RG/LUNs. 
     * Note that we have some additional luns bound, so 
     * it is necessary to pass in more_luns as TRUE. 
     */
    peep_test_recover_rg_luns(rg_config_p, 1, FBE_TRUE);

    /* Wait for the rebuilds to finish. */
    for (rg_index = 0; rg_index < PEEP_TEST_CONFIGS; rg_index++)
    {
        for(i=0; i < 1 /* drives_to_remove */; i++)
        {
            /* Wait until the rebuilds finish */
            sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_slots_to_be_removed[rg_index][i]);
        }

        rg_config_p++;
    }

    /* Restore the original RG/LUN config for the next use case. */
    peep_test_restore_original_config(FBE_FALSE);

    return;
}

/*!****************************************************************************
 *  peep_test_bind_unbind_when_NR_marked
 ******************************************************************************
 *
 * @brief
 *  This function bind/unbinds LUNs when RG or LUN is in degraded mode and
 *  the drive is marked NR. 
 *
 * @param   IN: rg_config_p - Pointer to the RG configuration.
 *
 * @return  None 
 *
  * @author
 *   09/26/2011 - Created. Arun S 
*****************************************************************************/
static void peep_test_bind_unbind_when_NR_marked(fbe_test_rg_configuration_t *in_rg_config_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u32_t           i = 0;
    fbe_u32_t           rg_index = 0;
    fbe_object_id_t     rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    /* Get the raid group object id */
    status = fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* First make the RG/LUN degraded by removing drive(s) */
    peep_test_degrade_rg_luns(in_rg_config_p, 1);

    /* Disable the rebuild background operation */
    fbe_api_base_config_disable_background_operation(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_METADATA_REBUILD);
    fbe_api_base_config_disable_background_operation(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_REBUILD);

    /* Re-insert the drive and keep the RG/LUN for next use case. 
     * Note that we have don't have additional luns bound, so pass in 
     * more_luns as FALSE. 
     */
    peep_test_recover_rg_luns(in_rg_config_p, 1, FBE_FALSE);

    /* Verify the paged_metadata_info for the RG */
    peep_test_verify_rg_paged_metadata_info(rg_object_id);

    /* Unbind/Bind Luns */
    peep_test_unbind_bind_luns(in_rg_config_p);

    /* Enable rebuild background operation */
    fbe_api_base_config_enable_background_operation(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_METADATA_REBUILD);
    fbe_api_base_config_enable_background_operation(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_REBUILD);

    /* Wait for the rebuilds to finish. */
	rg_config_p = in_rg_config_p;
    for (rg_index = 0; rg_index < PEEP_TEST_CONFIGS; rg_index++)
    {
        for(i=0; i < 1 /* drives_to_remove */; i++)
        {
            /* Wait until the rebuilds finish */
            sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_slots_to_be_removed[rg_index][i]);
        }

        rg_config_p++;
        //drives_to_remove_p++;
    }

    /* Restore the original RG/LUN config for the next use case. */
    peep_test_restore_original_config(FBE_FALSE);

    return;
}

/*!****************************************************************************
 *  peep_test_bind_unbind_when_rebuilding
 ******************************************************************************
 *
 * @brief
 *  This function bind/unbinds LUNs when the LUN is rebuilding.
 *
 * @param   IN: rg_config_p - Pointer to the RG configuration.
 *
 * @return  None 
 *
  * @author
 *   09/27/2011 - Created. Arun S 
*****************************************************************************/
static void peep_test_bind_unbind_when_rebuilding(fbe_test_rg_configuration_t *in_rg_config_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u32_t           i = 0;
    fbe_u32_t           rg_index = 0;
    fbe_object_id_t     rg_object_id[PEEP_TEST_CONFIGS];
    fbe_lba_t           target_checkpoint[PEEP_TEST_CONFIGS];
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    /* First make the RG/LUN degraded by removing drive(s) */
    peep_test_degrade_rg_luns(in_rg_config_p, 1);

    rg_config_p = in_rg_config_p;
    for (rg_index = 0; rg_index < PEEP_TEST_CONFIGS; rg_index++)
    {
        /* Get the RG's object id. */
        status = fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &rg_object_id[rg_index]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        target_checkpoint[rg_index] = 0x1000;

        /* Set the scheduler hook to halt the rebuild */
        peep_test_set_hook(rg_object_id[rg_index], target_checkpoint[rg_index]);

        rg_config_p++;
    }

    /* Re-insert the drive and keep the RG/LUN for next use case. 
     * Note that we have don't have additional luns bound, so pass in 
     * more_luns as FALSE. 
     */
    peep_test_recover_rg_luns(in_rg_config_p, 1, FBE_FALSE);

    /* Wait for the rebuilds to start. */
    rg_config_p = in_rg_config_p;
    for (rg_index = 0; rg_index < PEEP_TEST_CONFIGS; rg_index++)
    {
        for(i=0; i < 1 /* drives_to_remove */; i++)
        {
            /* Wait until the rebuilds start */
            sep_rebuild_utils_wait_for_rb_to_start(rg_config_p, drive_slots_to_be_removed[rg_index][i]);
        }

        rg_config_p++;
        //drives_to_remove_p++;
    }

    /* Unbind/Bind Luns */
    peep_test_unbind_bind_luns(in_rg_config_p);

    /* Wait for the rebuilds to finish. */
    rg_config_p = in_rg_config_p;
    for (rg_index = 0; rg_index < PEEP_TEST_CONFIGS; rg_index++)
    {
        /* Delete the hook and let the rebuild proceed. */
        peep_test_unset_hook(rg_object_id[rg_index], target_checkpoint[rg_index]);

        for(i=0; i < 1 /* drives_to_remove */; i++)
        {
            /* Wait until the rebuilds finish */
            sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_slots_to_be_removed[rg_index][i]);
        }

        rg_config_p++;
        //drives_to_remove_p++;
    }

    /* Restore the original RG/LUN config for the next use case. */
    peep_test_restore_original_config(FBE_FALSE);

    return;
}

/*!****************************************************************************
 *  peep_test_bind_unbind_when_paged_metadata_marked_NR
 ******************************************************************************
 *
 * @brief
 *  This function bind/unbinds LUNs when RG or LUN is in degraded mode and
 *  the drive is re-inserted and paged metadata is marked NR. 
 *
 * @param   IN: rg_config_p - Pointer to the RG configuration.
 *
 * @return  None 
 *
  * @author
 *   09/30/2011 - Created. Arun S 
*****************************************************************************/
static void peep_test_bind_unbind_when_paged_metadata_marked_NR(fbe_test_rg_configuration_t *in_rg_config_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u32_t           i = 0;
    fbe_u32_t           rg_index = 0;
    fbe_object_id_t     rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    /* Get the raid group object id */
    status = fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* First make the RG/LUN degraded by removing drive(s) */
    peep_test_degrade_rg_luns(in_rg_config_p, 1);

    /* Disable the paged metadata rebuild background operation */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Disabling the background operation: Paged Metadata Rebuild ===\n");
    fbe_api_base_config_disable_background_operation(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_METADATA_REBUILD);

    /* Re-insert the drive and keep the RG/LUN for next use case. 
     * Note that we have don't have additional luns bound, so pass in 
     * more_luns as FALSE. 
     */
    peep_test_recover_rg_luns(in_rg_config_p, 1, FBE_FALSE);

    /* Verify the paged_metadata_info for the RG */
    peep_test_verify_rg_paged_metadata_info(rg_object_id);

    /* Unbind/Bind Luns */
    peep_test_unbind_bind_luns(in_rg_config_p);

    /* Enable paged metadata rebuild background operation */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Enabling the background operation: Paged Metadata Rebuild ===\n");
    fbe_api_base_config_enable_background_operation(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_METADATA_REBUILD);

    /* Wait for the rebuilds to finish. */
    rg_config_p = in_rg_config_p;
    for (rg_index = 0; rg_index < PEEP_TEST_CONFIGS; rg_index++)
    {
        for(i=0; i < 1 /* drives_to_remove */; i++)
        {
            /* Wait until the rebuilds finish */
            sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_slots_to_be_removed[rg_index][i]);
        }

        rg_config_p++;
        //drives_to_remove_p++;
    }

    /* Restore the original RG/LUN config for the next use case. */
    peep_test_restore_original_config(FBE_FALSE);

    return;
}

/*!****************************************************************************
 *  peep_test_verify_rg_paged_metadata_info
 ******************************************************************************
 *
 * @brief
 *  This function verifies the paged metadata NR info.
 *
 * @param   IN: rg_object_id - Object id of the raid group.
 *
 * @return  None 
 *
 * @author
 *   09/30/2011 - Created. Arun S 
*****************************************************************************/
static void peep_test_verify_rg_paged_metadata_info(fbe_object_id_t rg_object_id)
{
    fbe_u32_t           rg_index = 0;
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_chunk_index_t   total_chunks;
    fbe_api_raid_group_get_info_t raid_group_info;
    fbe_api_raid_group_get_paged_info_t paged_info;

    /* Get the rebuild checkpoint */
    status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    total_chunks = (raid_group_info.capacity / raid_group_info.num_data_disk) / FBE_RAID_DEFAULT_CHUNK_SIZE;

    status = fbe_api_raid_group_get_paged_bits(rg_object_id, &paged_info, FBE_TRUE /* Get data from disk*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If the rebuild is done, no chunks should be needing a rebuild. */
    if (paged_info.num_nr_chunks == 0)
    {
        MUT_FAIL_MSG("There are chunks needing rebuild on SP A");
    }

    /* In the case of a dual sp test we will also check the peer since 
     * some of this information can be cached on the peer.  We want to make sure it is correct there also. 
     */
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

        status = fbe_api_raid_group_get_paged_bits(rg_object_id, &paged_info, FBE_TRUE /* Get data from disk*/);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* If the rebuild is done, no chunks should be needing a rebuild. */
        if (paged_info.num_nr_chunks == 0)
        {
            MUT_FAIL_MSG("There are chunks needing rebuild on SP B");
        }

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }

    /* Make sure that rebuild operation is disabled with checking of rebuild checkpoint */
    for (rg_index = 0; rg_index < PEEP_TEST_CONFIGS; rg_index++)
    {
        if(raid_group_info.rebuild_checkpoint[drive_slots_to_be_removed[rg_index][0]] != 0)
        {
            MUT_ASSERT_TRUE_MSG(FBE_FALSE, "Failed to disable rebuild background operation");
        }
    }

    return;
}

/*!****************************************************************************
 *  peep_test_set_hook
 ******************************************************************************
 *
 * @brief
 *  This function Sets the scheduler hook to stop the rebuild.
 *
 * @param   IN: rg_object_id - Object id of the raid group.
 * @param   IN: checkpoint - Checkpoint for the rebuild.
 *
 * @return  None 
 *
 * @author
 *   09/28/2011 - Created. Arun S 
*****************************************************************************/
static void peep_test_set_hook(fbe_object_id_t rg_object_id, fbe_lba_t checkpoint)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_sim_transport_connection_target_t active_target, target_server;

    /* Get the active SP */
    fbe_test_sep_get_active_target_id(&active_target);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, active_target);

    target_server = fbe_api_sim_transport_get_target_server();

    mut_printf(MUT_LOG_TEST_STATUS, "Active: %d, Target: %d", active_target, target_server);
    
    if(active_target == FBE_SIM_SP_A) 
    {
        if(active_target != target_server)
        {
            fbe_api_sim_transport_set_target_server(active_target);
        }
    }

    if(active_target == fbe_api_sim_transport_get_target_server())
    {
        mut_printf(MUT_LOG_TEST_STATUS, "Adding Debug Hook...");
        status = fbe_api_scheduler_add_debug_hook(rg_object_id,
                                SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                checkpoint,
                                NULL,
                                SCHEDULER_CHECK_VALS_LT,
                                SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    return;
}

/*!****************************************************************************
 *  peep_test_unset_hook
 ******************************************************************************
 *
 * @brief
 *  This function unsets the scheduler hook to stop the rebuild.
 *
 * @param   IN: rg_object_id - Object id of the raid group.
 * @param   IN: checkpoint - Checkpoint for the rebuild.
 *
 * @return  None 
 *
 * @author
 *   09/28/2011 - Created. Arun S 
*****************************************************************************/
static void peep_test_unset_hook(fbe_object_id_t rg_object_id, fbe_lba_t checkpoint)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_sim_transport_connection_target_t active_target, target_server;

    /* Get the active SP */
    fbe_test_sep_get_active_target_id(&active_target);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, active_target);

    target_server = fbe_api_sim_transport_get_target_server();

    mut_printf(MUT_LOG_TEST_STATUS, "Active: %d, Target: %d", active_target, target_server);

    if(active_target == FBE_SIM_SP_A) 
    {
        if(active_target != target_server)
        {
            fbe_api_sim_transport_set_target_server(active_target);
        }
    }

    if(active_target == fbe_api_sim_transport_get_target_server())
    {
        status = fbe_api_scheduler_del_debug_hook(rg_object_id,
                                SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                checkpoint,
                                NULL,
                                SCHEDULER_CHECK_VALS_LT,
                                SCHEDULER_DEBUG_ACTION_PAUSE);

        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    return;
}

/*!****************************************************************************
 *  peep_test_unbind_bind_luns
 ******************************************************************************
 *
 * @brief
 *  This function bind/unbinds LUNs when RG or LUN is in degraded mode. 
 *
 * @param   IN: rg_config_p - Pointer to the RG configuration.
 *
 * @return  None 
 *
  * @author
 *   09/22/2011 - Created. Arun S 
*****************************************************************************/
static void peep_test_unbind_bind_luns(fbe_test_rg_configuration_t *rg_config_p)
{
    /* Unbind a LUN when degraded */
    peep_test_destroy_lun(peep_luns[0].fbe_lun_create_req.lun_number);

    /* Bind a LUN when degraded */
    peep_test_create_lun(FBE_TRUE);

    return;
}

/*!****************************************************************************
 *  peep_test_restore_original_config
 ******************************************************************************
 *
 * @brief
 *  This function unbinds all use LUNs and restores the original Configuration. 
 *
 * @param   IN: more_luns - Flag to bind more (extra) LUNs.
 *
 * @return  None 
 *
  * @author
 *   09/22/2011 - Created. Arun S 
*****************************************************************************/
static void peep_test_restore_original_config(fbe_bool_t more_luns)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Unbind all user LUNs..  */
    status = fbe_test_sep_util_destroy_all_user_luns();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Reset the lun counter. */
    peep_total_luns = 0;

    /* Recreate the original configuration. */
    peep_test_create_lun(more_luns);

    return;
}

/*!****************************************************************************
 *  peep_test_degrade_rg_luns
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
 *   09/22/2011 - Created. Arun S 
 *****************************************************************************/
static void peep_test_degrade_rg_luns(fbe_test_rg_configuration_t *in_rg_config_p, 
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
    status = fbe_api_get_total_objects(&peep_number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Go through all the RG for which user has provided configuration data. */
    for (rg_index = 0; rg_index < PEEP_TEST_CONFIGS; rg_index++)
    {
        /* Write a seed pattern to the RG */
        mut_printf(MUT_LOG_TEST_STATUS, "Writing BG Pattern...");
        sep_rebuild_utils_write_bg_pattern(&fbe_peep_test_context[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);
    }

    rg_config_p = in_rg_config_p;
    drives_to_remove_p = drives_to_remove;
    for (rg_index = 0; rg_index < PEEP_TEST_CONFIGS; rg_index++)
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
        drives_to_remove_p++;
    }

    /* Before putting the RG/LUNs do a READ and WRITE new pattern so that
     * we can verify if we have the same data once we recover the RG/LUNs
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Reading BG Pattern...");
    sep_rebuild_utils_read_bg_pattern(&fbe_peep_test_context[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);

    rg_config_p = in_rg_config_p;
    for (rg_index = 0; rg_index < PEEP_TEST_CONFIGS; rg_index++)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "Writing BG Pattern...0x200");
        sep_rebuild_utils_write_new_data(rg_config_p, &fbe_peep_test_context[0], 0x200);
        rg_config_p++;
    }

    /* Remove the drives to put RG/LUN in degraded state. */
    rg_config_p = in_rg_config_p;
    drives_to_remove_p = drives_to_remove;
    for (rg_index = 0; rg_index < PEEP_TEST_CONFIGS; rg_index++)
    {
        for(i=0; i < drives_to_remove_p; i++)
        {
            peep_number_physical_objects -= 1;

            /* Remove drives in the RG. */
            sep_rebuild_utils_remove_drive_and_verify(rg_config_p, drive_slots_to_be_removed[rg_index][i], peep_number_physical_objects, &peep_drive_info[rg_index][i]);
            mut_printf(MUT_LOG_TEST_STATUS, "Drive removed from position: %d\n", drive_slots_to_be_removed[rg_index][i]);

            /* Verify that rebuild logging is set for the drive and that the rebuild checkpoint is 0 */
            sep_rebuild_utils_verify_rb_logging(rg_config_p, drive_slots_to_be_removed[rg_index][i]);
        }

        rg_config_p++;
        drives_to_remove_p++;
    }

    /* Verify the RG/LUNs are in READY state */
    mut_printf(MUT_LOG_TEST_STATUS, "Verifying the RG/LUNs are in READY state");
    peep_test_verify_rg_lun_state(FBE_LIFECYCLE_STATE_READY, FBE_FALSE);

    /* Now that we have degraded the RG/LUNs by removing a drive,
     * verify if we have the same data we wrote before degrading it. 
     */
    rg_config_p = in_rg_config_p;
    for (rg_index = 0; rg_index < PEEP_TEST_CONFIGS; rg_index++)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "Reading and Verifying BG Pattern...0x200");
        sep_rebuild_utils_verify_new_data(rg_config_p, &fbe_peep_test_context[0], 0x200);
        rg_config_p++;
    }

    /* Write some new data on the degraded RG/LUNs */
    rg_config_p = in_rg_config_p;
    for (rg_index = 0; rg_index < PEEP_TEST_CONFIGS; rg_index++)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "Writing BG Pattern...0x500");
        sep_rebuild_utils_write_new_data(rg_config_p, &fbe_peep_test_context[0], 0x500);
        rg_config_p++;
    }

    return;
}

/*!****************************************************************************
 *  peep_test_verify_rg_lun_state
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
 *   09/22/2011 - Created. Arun S 
 *****************************************************************************/
static void peep_test_verify_rg_lun_state(fbe_lifecycle_state_t expected_state,
                                          fbe_bool_t more_luns)
{
    fbe_u32_t i = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t      total_luns = (more_luns == FBE_TRUE)? PEEP_TEST_TOTAL_LUNS: (PEEP_TEST_TOTAL_LUNS-PEEP_TEST_EXTRA_LUN_COUNT);

    /* When a drive is removed, and the SP is rebooted RAID object would be stuck in specialize. */
    fbe_api_database_lookup_raid_group_by_number(PEEP_TEST_RAID_GROUP_ID, &rg_object_id);
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id,
                                                     expected_state,
                                                     60000,
                                                     FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "RG %d is in expected state\n", PEEP_TEST_RAID_GROUP_ID);

    for (i = 0; i < total_luns; i++)
    {
        status = fbe_api_wait_for_object_lifecycle_state(peep_luns[i].lun_object_id,
                                                         expected_state,
                                                         60000,
                                                         FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, "LUN %d is in expected state\n", peep_luns[i].fbe_lun_create_req.lun_number);
    }

    return;
}

/*!****************************************************************************
 *  peep_test_recover_rg_luns
 ******************************************************************************
 *
 * @brief
 *  This function reinserts the RG/LUNs by re-inserting the drive. After the
 *  successful recovery of the LUNs, verify if we have recovered the data as well.
 *
 * @param   IN: rg_config_ptr - pointer to the RG configuration.
 *
 * @return  None 
 *
 * @author
 *   09/22/2011 - Created. Arun S 
 *****************************************************************************/
static void peep_test_recover_rg_luns(fbe_test_rg_configuration_t *rg_config_ptr,
                                      fbe_u32_t drives_to_insert, fbe_bool_t more_luns)
{
    fbe_u32_t                       i = 0;
    fbe_u32_t                       rg_index = 0;
    fbe_test_rg_configuration_t     *rg_config_p = NULL;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Re-Inserting Drives ===\n");

    /* First re-insert the drives */
    rg_config_p = rg_config_ptr;
    for (rg_index = 0; rg_index < PEEP_TEST_CONFIGS; rg_index++)
    {
        /* Reinsert the drive */
        for(i=0; i < drives_to_insert; i++)
        {
            peep_number_physical_objects += 1;

            /* Reinsert the removed drive(s) */
            sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, drive_slots_to_be_removed[rg_index][i], 
                                                        peep_number_physical_objects, &peep_drive_info[rg_index][i]);
            mut_printf(MUT_LOG_TEST_STATUS, "Drive inserted in position: %d\n", drive_slots_to_be_removed[rg_index][i]);

            /* Verify that rebuild logging is cleared for the drive */
            sep_rebuild_utils_verify_not_rb_logging(rg_config_p, drive_slots_to_be_removed[rg_index][i]);
        }

        rg_config_p++;
    }

    /* Verify the RG/LUNs are in READY state */
    mut_printf(MUT_LOG_TEST_STATUS, "Verifying the RG/LUNs are in READY state");
    peep_test_verify_rg_lun_state(FBE_LIFECYCLE_STATE_READY, more_luns);

    /* Now that we have recovered the RG/LUNs by re-inserting the drives,
     * verify if we have the same data we wrote before degrading it. 
     */
    rg_config_p = rg_config_ptr;
    for (rg_index = 0; rg_index < PEEP_TEST_CONFIGS; rg_index++)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "Reading and Verifying BG Pattern...0x500");
        sep_rebuild_utils_verify_new_data(rg_config_p, &fbe_peep_test_context[0], 0x500);
        rg_config_p++;
    }

    return;
}

/*!****************************************************************************
 *  peep_test_create_lun
 ******************************************************************************
 *
 * @brief
 *  This function creates LUNs for Peep test.  
 *
 * @param   IN: is_ndb - Non-destructive flag.
 *
 * @return  None 
 *
 * @author
 *   09/22/2011 - Created. Arun S 
 *****************************************************************************/
static void peep_test_create_lun(fbe_bool_t more_luns)
{
    fbe_status_t   status;
    fbe_u32_t      lun_index = 0;
    fbe_u32_t      total_luns = (more_luns == FBE_TRUE)? PEEP_TEST_TOTAL_LUNS: (PEEP_TEST_TOTAL_LUNS-PEEP_TEST_EXTRA_LUN_COUNT);
    fbe_job_service_error_type_t job_error_type;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Creating LUNs ===\n");

    for (lun_index = 0; lun_index < total_luns; lun_index++)
    {
        if(more_luns)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "=== Creating Additional LUNs ===\n");
            /* Set the Lun number */
            peep_luns[lun_index].fbe_lun_create_req.lun_number = PEEP_TEST_STARTING_EXTRA_LUN + lun_index;
        }
        else
        {
            /* Set the Lun number */
            peep_luns[lun_index].fbe_lun_create_req.lun_number = PEEP_TEST_STARTING_LUN + lun_index;
        }

        peep_luns[lun_index].fbe_lun_create_req.raid_group_id = PEEP_TEST_RAID_GROUP_ID;
        peep_luns[lun_index].fbe_lun_create_req.raid_type = FBE_RAID_GROUP_TYPE_RAID5;
        peep_luns[lun_index].fbe_lun_create_req.capacity = 0x2000;
        peep_luns[lun_index].fbe_lun_create_req.addroffset = FBE_LBA_INVALID;
        peep_luns[lun_index].fbe_lun_create_req.ndb_b = FBE_FALSE;
        peep_luns[lun_index].fbe_lun_create_req.noinitialverify_b = FBE_FALSE;
        peep_luns[lun_index].fbe_lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
        peep_luns[lun_index].fbe_lun_create_req.world_wide_name.bytes[0] = fbe_random() & 0xf;
        peep_luns[lun_index].fbe_lun_create_req.world_wide_name.bytes[1] = fbe_random() & 0xf;
        peep_luns[lun_index].expected_to_fail = FBE_FALSE;

        status = fbe_api_create_lun(&peep_luns[lun_index].fbe_lun_create_req, 
                                    FBE_TRUE, 
                                    PEEP_TEST_NS_TIMEOUT, 
                                    &peep_luns[lun_index].lun_object_id,
                                    &job_error_type);

        if (peep_luns[lun_index].expected_to_fail)
        {
            MUT_ASSERT_INT_NOT_EQUAL_MSG(status, FBE_STATUS_OK, "fbe_api_create_lun expected to fail, but it succeeded!!!"); 
        }
        else
        {
            MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "fbe_api_create_lun failed!!!"); 
        }

        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, "Created LUN: %d\n", peep_luns[lun_index].fbe_lun_create_req.lun_number);

        /* Verify the LUN create result */
        peep_verify_lun_creation(&peep_luns[lun_index]);

        /* Total no. of luns */
        peep_total_luns++;
    }
  
    return;

}/* End peep_test() */


/*!****************************************************************************
 *  peep_setup
 ******************************************************************************
 *
 * @brief
 *  This is the setup function for the Peep test. 
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void peep_setup(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Peep test ===\n");

    /* Load the physical config on the target server */    
    peep_test_load_physical_config();

    /* Load the SEP package on the target server */
    sep_config_load_sep_and_neit();

    return;
} /* End peep_setup() */


/*!****************************************************************************
 *  peep_cleanup
 ******************************************************************************
 *
 * @brief
 *  This is the cleanup function for the Peep test.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *   09/22/2011 - Created. Arun S 
 *****************************************************************************/
void peep_cleanup(void)
{  
    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for Peep test ===\n");

    fbe_test_sep_util_destroy_neit_sep_physical();

    return;
} /* End peep_cleanup() */


/*!**************************************************************
 * peep_test_load_physical_config()
 ****************************************************************
 * @brief
 *  Configure the Peep test physical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 * @author
 *   09/22/2011 - Created. Arun S 
 ****************************************************************/

static void peep_test_load_physical_config(void)
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
        fbe_test_pp_util_insert_sas_flash_drive(0, 0, slot, 520, PEEP_TEST_DRIVE_CAPACITY, &drive_handle);
    }

    for (slot = 0; slot < 15; slot++)
    {
        fbe_test_pp_util_insert_sas_flash_drive(0, 1, slot, 520, PEEP_TEST_DRIVE_CAPACITY, &drive_handle);
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
    status = fbe_api_wait_for_number_of_objects(PEEP_TEST_NUMBER_OF_PHYSICAL_OBJECTS, READY_STATE_WAIT_TIME, FBE_PACKAGE_ID_PHYSICAL);
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
    MUT_ASSERT_TRUE(total_objects == PEEP_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");

    return;

} /* End peep_test_load_physical_config() */


/*!**************************************************************
 * peep_verify_lun_info()
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
 *   09/22/2011 - Created. Arun S 
 ****************************************************************/
static void peep_verify_lun_info(fbe_object_id_t in_lun_object_id, fbe_api_lun_create_t  *in_fbe_lun_create_req_p)
{
    fbe_status_t status;
    fbe_database_lun_info_t lun_info;
    fbe_object_id_t rg_obj_id;

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
    /* Since we insert SAS FLASH DRIVE which means the rg/lun is build on top of FLASH DRIVE,
       then the rotational_rate must be one. */
    MUT_ASSERT_INTEGER_EQUAL_MSG(lun_info.rotational_rate, 1, "Lun_get_info return mismatch rotational_rate.");

    return;
}

/*!**************************************************************
 * peep_verify_lun_creation()
 ****************************************************************
 * @brief
 *  Verifies the lun creation.
 *
 * @param IN: lun_config - pointer to the lun_config structure.              
 *
 * @return None.
 *
 * @author
 *   09/22/2011 - Created. Arun S 
 ****************************************************************/
static void peep_verify_lun_creation(peep_lun_t * lun_config)
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
        peep_verify_lun_info(lun_config->lun_object_id, &lun_config->fbe_lun_create_req);
    }

    mut_printf(MUT_LOG_LOW, "=== Verify on ACTIVE side (%s) Passed ===", active_target == FBE_SIM_SP_A ? "SP A" : "SP B");

    if ( b_run_on_dualsp )
    {
        /* verify on the passive SP */
        fbe_api_sim_transport_set_target_server(passive_target);

        if ( !lun_config->expected_to_fail )
        {
            peep_verify_lun_info(lun_config->lun_object_id, &lun_config->fbe_lun_create_req);
        }

        mut_printf(MUT_LOG_LOW, "=== Verify on Passive side (%s) Passed ===", passive_target == FBE_SIM_SP_A ? "SP A" : "SP B");
    }
}


/*!****************************************************************************
 *  peep_dualsp_setup
 ******************************************************************************
 *
 * @brief
 *  This is the setup function for the Peep test in dualsp mode. It is responsible
 *  for loading the physical and logical configuration for this test suite on both SPs.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *   09/22/2011 - Created. Arun S 
 *****************************************************************************/
void peep_dualsp_setup(void)
{
    fbe_sim_transport_connection_target_t   sp;

    if (fbe_test_util_is_simulation())
    { 
        mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Peep dualsp test ===\n");
        /* Set the target server */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    
        /* Make sure target set correctly */
        sp = fbe_api_sim_transport_get_target_server();
        MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_B, sp);
    
        mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n", 
                   sp == FBE_SIM_SP_A ? "SP A" : "SP B");

        /* Load the physical config on the target server */
        peep_test_load_physical_config();
    
        /* Set the target server */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    
        /* Make sure target set correctly */
        sp = fbe_api_sim_transport_get_target_server();
        MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_A, sp);
    
        mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n", 
                   sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    
        /* Load the physical config on the target server */    
        peep_test_load_physical_config();

        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during 
         * the setup.
         */
        sep_config_load_sep_and_neit_both_sps();
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }

    return;

} /* End peep_setup() */


/*!****************************************************************************
 *  peep_dualsp_cleanup
 ******************************************************************************
 *
 * @brief
 *  This is the cleanup function for the Peep test.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *   09/22/2011 - Created. Arun S 
 *****************************************************************************/
void peep_dualsp_cleanup(void)
{  
    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for Peep test ===\n");

    /* Destroy the test configuration on both SPs */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_destroy_neit_sep_physical();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_test_sep_util_destroy_neit_sep_physical();

    return;
} /* End peep_dualsp_cleanup() */

/*!****************************************************************************
 *  peep_reboot_sp
 ******************************************************************************
 *
 * @brief
 *  This is the reboot SP function for the Peep test.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *   09/22/2011 - Created. Arun S 
 *****************************************************************************/
static void peep_reboot_sp(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    /*reboot sep*/
    fbe_test_sep_util_destroy_neit_sep();
    sep_config_load_sep_and_neit();
    status = fbe_test_sep_util_wait_for_database_service(PEEP_TEST_NS_TIMEOUT);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

/*!****************************************************************************
 *  peep_reboot_dualsp
 ******************************************************************************
 *
 * @brief
 *  This is the reboot SP function for the Peep test.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *   09/22/2011 - Created. Arun S 
 *****************************************************************************/
static void peep_reboot_dualsp(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    /*reboot sep*/
    fbe_test_sep_util_destroy_neit_sep_both_sps();
    sep_config_load_sep_and_neit_both_sps();
    status = fbe_test_sep_util_wait_for_database_service(PEEP_TEST_NS_TIMEOUT);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

/*!****************************************************************************
 *  peep_dualsp_test
 ******************************************************************************
 *
 * @brief
 *  This is the dual SP test for the Peep test.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *   09/22/2011 - Created. Arun S 
 *****************************************************************************/
void peep_dualsp_test(void)
{
    /* Enable dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* Now run the create tests */
    peep_test();

    /* Always clear dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
}


/*!****************************************************************************
 *  peep_test_destroy_lun
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
 *   09/22/2011 - Created. Arun S 
 *****************************************************************************/
static void peep_test_destroy_lun(fbe_lun_number_t   lun_number)
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

    status = fbe_api_destroy_lun(&fbe_lun_destroy_req, FBE_TRUE,  PEEP_TEST_NS_TIMEOUT, &job_error_type);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    /* Get object ID for LUN should fail, because it is just destroyed */
    status = fbe_api_database_lookup_lun_by_number(fbe_lun_destroy_req.lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_GENERIC_FAILURE, status);
    mut_printf(MUT_LOG_TEST_STATUS, "LUN %d destroyed successfully.\n", fbe_lun_destroy_req.lun_number);

    /* Track the total no. of luns */
    peep_total_luns--;

    return;
}  /* End peep_test_destroy_lun() */

/*!**************************************************************
 * peep_test_create_rg()
 ****************************************************************
 * @brief
 *  Configure the Raid Group for Peep test.
 *
 * @param None.              
 *
 * @return None.
 *
 * @author
 *   09/22/2011 - Created. Arun S 
 ****************************************************************/

static void peep_test_create_rg(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                                status;
    fbe_object_id_t                             rg_object_id;
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
                                         PEEP_TEST_NS_TIMEOUT,
                                         &job_error_code,
                                         &job_status,
                                         NULL);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to created RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    /* Verify the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(
             rg_config_p->raid_group_id, &rg_object_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    /* Verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY, 
                                                     READY_STATE_WAIT_TIME, FBE_PACKAGE_ID_SEP_0);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "Created Raid Group: %d\n", rg_config_p->raid_group_id);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: END\n", __FUNCTION__);

    return;

} /* End peep_test_create_rg() */


