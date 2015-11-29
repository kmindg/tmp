/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file marina_test.c
 ***************************************************************************
 *
 * @brief
 *   This file contains tests of rebuilds and Degraded System RGs with SP failures.
 *
 * @version
 *   07/29/2011 - Created. Arun S
 *
 ***************************************************************************/


//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:


#include "sep_rebuild_utils.h"                      //  SEP rebuild utils
#include "mut.h"                                    //  MUT unit testing infrastructure
#include "fbe/fbe_types.h"                          //  general types
#include "sep_utils.h"                              //  for fbe_test_raid_group_disk_set_t
#include "sep_tests.h"                              //  for sep_config_load_sep_and_neit()
#include "sep_test_io.h"                            //  for sep I/O tests
#include "fbe_test_configurations.h"                //  for general configuration functions (elmo_, grover_)
#include "fbe/fbe_api_utils.h"                      //  for fbe_api_wait_for_number_of_objects
#include "fbe/fbe_api_discovery_interface.h"        //  for fbe_api_get_total_objects
#include "fbe/fbe_api_raid_group_interface.h"       //  for fbe_api_raid_group_get_info_t
#include "fbe/fbe_api_provision_drive_interface.h"  //  for fbe_api_provision_drive_get_obj_id_by_location
#include "fbe/fbe_api_database_interface.h"         //  for fbe_api_database_lookup_raid_group_by_number
#include "fbe/fbe_api_sim_server.h"
#include "pp_utils.h"                               //  for fbe_test_pp_util_pull_drive, _reinsert_drive
#include "fbe/fbe_api_logical_error_injection_interface.h"  // for error injection definitions
#include "fbe/fbe_random.h"                         //  for fbe_random()
#include "fbe_test_common_utils.h"                  //  for discovering config

//-----------------------------------------------------------------------------
//  TEST DESCRIPTION:

char* marina_short_desc = "Rebuild System RG error cases: SP failures";
char* marina_long_desc =
    "\n"
    "\n"
    "The Marina Scenario is a test of rebuild error cases due to disk failures.\n"
    "\n"
    "Phase 2 is a 2-drive removal in system RG's (3-Way mirror and Parity RG)."
    "Make sure the 3-Way Mirror RG stays in READY state and Parity RG in SPECIALIZE state.\n"
    "\n"
    "\n"
    "Starting Config:\n"
    "    [PP] armada board\n"
    "    [PP] SAS PMC port\n"
    "    [PP] viper enclosure\n"
    "    [PP] 8 SAS drives (PDOs)\n"
    "    [PP] 8 logical drives (LDs)\n"
    "    [SEP] 4 provision drives (PVDs)\n"
    "    [SEP] 4 virtual drives (VDs)\n"
    "    [SEP] System RAID objects (RAID-1 and RAID-3)\n"
    "    [SEP] System LUN objects \n"
    "\n"
    "\n"
    "STEP 1: Bring up the initial topology\n"
    "\n"
    "STEP 2: Remove one of the drives in the RG (drive A)\n"
    "    - Remove the physical drive (PDO-A)\n"
    "    - Verify that 3-way Mirror and Parity RG stays in READY state\n"
    "    - Reboot the SP.\n"
    "    - Make sure the 3-way Mirror and Parity RG stays READY state\n"
    "    - (currently disabled) Re-insert all the drives and make sure we rebuild.\n"
    "\n"
    "STEP 3: (currently disabled) Two drive removal test\n"
    "    - Remove the physical drive 1 (PDO-A)\n"
    "    - Remove the physical drive 2 (PDO-B)\n"
    "    - Verify that 3-way Mirror is in READY state and Parity RG stays in FAIL state\n"
    "    - Reboot the SP.\n"
    "    - Make sure the 3-way Mirror is in READY and Parity RG stays SPECIALIZE state\n"
    "    - Re-insert all the drives and make sure we rebuild.\n"
    "\n"
    "STEP 4: Cleanup\n"
    "    - Destroy objects\n"
    "\n"
    "\n"
    "Description last updated: 10/17/2011.\n";


//-----------------------------------------------------------------------------
//  TYPEDEFS, ENUMS, DEFINES, CONSTANTS, MACROS, GLOBALS:

//-----------------------------------------------------------------------------

/* The enums are hardcoded for now matching the ones in PSL.*/
typedef enum
{
    MARINA_TRIPLE_MIRROR_RG = 14,
    MARINA_4_DRIVE_R3_RG,
    MARINA_VAULT_RG
}marina_system_rg_ids;

/*!*******************************************************************
 * @def MARINA_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects we will create.
 *        (1 board + 2 port + 1 encl + 8 pdo ) 
 *********************************************************************/
#define MARINA_TEST_NUMBER_OF_PHYSICAL_OBJECTS 12


/*!*******************************************************************
 * @def MARINA_TEST_DRIVE_CAPACITY
 *********************************************************************
 * @brief Number of blocks in the physical drive.
 *
 * Note: change the drive capacity to smaller to reduce zeroing test time
 *********************************************************************/
#define MARINA_TEST_DRIVE_CAPACITY (TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY/2)

#define MARINA_READY_STATE_WAIT_TIME    20000

static fbe_u32_t no_of_physical_objects = MARINA_TEST_NUMBER_OF_PHYSICAL_OBJECTS;

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

//  FORWARD DECLARATIONS:

void marina_physical_config(void);
void marina_remove_drive_and_verify(fbe_device_physical_location_t *location, 
                                    fbe_u32_t                       in_num_objects, 
                                    fbe_api_terminator_device_handle_t *out_drive_info_p,
                                    fbe_bool_t                      verify);
void marina_remove_verify_reinsert_system_drives_both_sps(fbe_u32_t no_of_drives_to_be_removed);
void marina_reinsert_drive_and_verify(fbe_device_physical_location_t *location, 
                                      fbe_u32_t                       in_num_objects, 
                                      fbe_api_terminator_device_handle_t *in_drive_info_p);
static void marina_reboot_sp(void);
void marina_verify_system_rg_state(fbe_u32_t drive_removed, fbe_bool_t is_rebooted);

/*!****************************************************************************
 *  marina_dualsp_test
 ******************************************************************************
 *
 * @brief
 *   This is the main entry point for the Marina test.  The test does the
 *   following:
 *
 *   - Removes system drives and verifies the system RG state.
 *
 * @param   None
 *
 * @return  None
 *
 * @author
 * 07/29/2011 - Created. Arun S
 *
 *****************************************************************************/
void marina_dualsp_test(void)
{
    /* Enable dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    marina_verify_system_rg_state(0, FALSE);

    /*  Remove 2 drives in the system RG and verify the drive and RG states. */
    marina_remove_verify_reinsert_system_drives_both_sps(1);

    /*  Remove 2 drives in the system RG and verify the drive and RG states. */
    /* ==> Enable this call when the DB drives and its PVDs are persisted. 
     * Also enable drive insertions.. in marina_remove_verify_reinsert_system_drives_both_sps 
     */
    //marina_remove_verify_reinsert_system_drives_both_sps(2);

    /* Always clear dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
} //  End marina_dualsp_test()


/*!****************************************************************************
 *  marina_dualsp_setup
 ******************************************************************************
 *
 * @brief
 *   This is the common setup function for the Marina test.  It creates the
 *   physical configuration and loads the packages.
 *
 * @param   None
 *
 * @return  None
 *
 * @author
 * 07/29/2011 - Created. Arun S
 *
 *****************************************************************************/
void marina_dualsp_setup(void)
{
    /* We do this so that the fbe api does not trace info messages. 
     * Since we start so many different rdgen operations this gets chatty as 
     * the fbe api for rdgen traces each operation completion. 
     */
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_WARNING);

	if (fbe_test_util_is_simulation())
    {
        /* Instantiate the drives on SP A */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        marina_physical_config();
        
        /*! Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        marina_physical_config();
        
        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during
         * the setup.
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_sep_and_neit_both_sps();
    }

} //  End marina_dualsp_setup()

/*!****************************************************************************
 *  marina_dualsp_cleanup
 ******************************************************************************
 *
 * @brief
 *   This is the cleanup function for the Marina test.
 *
 * @param   None
 *
 * @return  None
 *
 * @author
 * 07/29/2011 - Created. Arun S
 *
 *****************************************************************************/
void marina_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Always clear dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        /* We already have cleanedup SPA. It's time to cleanup SPB. */
        fbe_test_sep_util_destroy_neit_sep_physical_both_sps();
    }

    return;

} // End marina_dualsp_cleanup()


/*!**************************************************************
 * marina_physical_config()
 ****************************************************************
 * @brief
 *  Configure the Marina test's physical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 * @author
 *  7/29/2011 - Created. Arun S
 *
 ****************************************************************/

void marina_physical_config(void)
{
    fbe_status_t                        status  = FBE_STATUS_GENERIC_FAILURE;

    fbe_u32_t                           total_objects = 0;
    fbe_class_id_t                      class_id;

    fbe_api_terminator_device_handle_t  port0_handle;
    fbe_api_terminator_device_handle_t  port1_handle;
    fbe_api_terminator_device_handle_t  encl0_0_handle;
    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_u32_t slot;
	fbe_object_id_t object_id;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: using new creation API and Terminator Class Management\n", __FUNCTION__);
    mut_printf(MUT_LOG_LOW, "=== configuring terminator ===\n");
    /*before loading the physical package we initialize the terminator*/
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    /* insert a board
     */
    status = fbe_test_pp_util_insert_armada_board();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* insert a port
     */
    fbe_test_pp_util_insert_sas_pmc_port(1, /* io port */
                                         2, /* portal */
                                         0, /* backend number */ 
                                         &port0_handle);

    fbe_test_pp_util_insert_sas_pmc_port(2, /* io port */
                                         2, /* portal */
                                         1, /* backend number */ 
                                         &port1_handle);

    /* insert an enclosures to port 0
     */
    fbe_test_pp_util_insert_viper_enclosure(0, 0, port0_handle, &encl0_0_handle);

    /* Insert drives for enclosures.
     */
    for ( slot = 0; slot < 8; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 0, slot, 520, MARINA_TEST_DRIVE_CAPACITY, &drive_handle);
    }

    /* Init fbe api on server */
    mut_printf(MUT_LOG_LOW, "=== starting Server FBE_API ===\n");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== starting physical package===\n");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== set Physical Package entries ===\n");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== starting system discovery ===\n");
    /* wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(MARINA_TEST_NUMBER_OF_PHYSICAL_OBJECTS, MARINA_READY_STATE_WAIT_TIME, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_LOW, "=== verifying configuration ===\n");

    /* We inserted a armada board so we check for it.
     * board is always assumed to be object id 0.
     */
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_LOW, "Verifying board type....Passed");

    /* Make sure we have the expected number of objects.
     */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == MARINA_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");

	for (object_id = 0 ; object_id < MARINA_TEST_NUMBER_OF_PHYSICAL_OBJECTS; object_id++){
		status = fbe_api_wait_for_object_lifecycle_state(object_id, FBE_LIFECYCLE_STATE_READY, MARINA_READY_STATE_WAIT_TIME, FBE_PACKAGE_ID_PHYSICAL);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	}
    return;
}
/******************************************
 * end marina_physical_config()
 ******************************************/


/*!****************************************************************************
 *  marina_remove_drive_and_verify
 ******************************************************************************
 *
 * @brief
 *    This function removes a drive.  First it waits for the logical and physical
 *    drive objects to be destroyed.  Then checks the object states of the
 *    PVD and VD for that drive are Activate.
 *
 * @param location              - Info on the drive location
 * @param in_num_objects        - the number of physical objects that should exist
 *                                after the disk is removed
 * @param out_drive_info_p      - pointer to structure that gets filled in with the
 *                                "drive info" for the drive
 * @param verify                - Whether to verify pvd status
 * @return None
 * 
 * @author
 * 08/01/2011 - Created. Arun S
 *
 *****************************************************************************/
void marina_remove_drive_and_verify(fbe_device_physical_location_t *location, 
                                    fbe_u32_t                       in_num_objects, 
                                    fbe_api_terminator_device_handle_t *out_drive_info_p,
                                    fbe_bool_t verify)
{
    fbe_sim_transport_connection_target_t   sp;
    fbe_status_t                status;                             // fbe status
    fbe_object_id_t             pvd_object_id;                      // pvd's object id

    /* Get the loca SP ID */
    sp = fbe_api_sim_transport_get_target_server();
    MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, sp);

    mut_printf(MUT_LOG_LOW, "=== Now operating on - %s ===", sp == FBE_SIM_SP_A ? "SP A" : "SP B");

    /* Get the PVD object id before we remove the drive */
    status = fbe_api_provision_drive_get_obj_id_by_location(location->bus,
                                                            location->enclosure,
                                                            location->slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*  Remove the drive */
    if (fbe_test_util_is_simulation())
    {
        mut_printf(MUT_LOG_LOW, "=== Removing drive [%d_%d_%d] ===", location->bus, location->enclosure, location->slot);
        status = fbe_test_pp_util_pull_drive(location->bus, location->enclosure, location->slot, out_drive_info_p);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Verify the PDO and LDO are destroyed by waiting for the number of physical objects to be equal to the
         * value that is passed in.
         */
        status = fbe_api_wait_for_number_of_objects(in_num_objects, MARINA_READY_STATE_WAIT_TIME, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    else
    {
        status = fbe_test_pp_util_pull_drive_hw(location->bus, location->enclosure, location->slot);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    if (verify)
    {
        mut_printf(MUT_LOG_LOW, "=== Verifying the PVD and its state on - %s ===", sp == FBE_SIM_SP_A ? "SP A" : "SP B");
        /*  Verify that the PVD and VD objects are in the FAIL state */
        sep_rebuild_utils_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_FAIL);
    }
    mut_printf(MUT_LOG_LOW, "=== Removed drive [%d_%d_%d], PVD: 0x%x, state: FAIL. ===\n", 
               location->bus, location->enclosure, location->slot, pvd_object_id);

    return;
} // End marina_remove_drive_and_verify()


/*!****************************************************************************
 *  marina_remove_verify_reinsert_system_drives_both_sps
 ******************************************************************************
 *
 * @brief
 *    This function removes a drive.  First it waits for the logical and physical
 *    drive objects to be destroyed.  Then checks the object states of the
 *    PVD and VD for that drive are Activate.
 *
 * @param drive_to_be_removed           - drive to be removed.
 * @param no_of_drives_to_be_removed    - total number of drives to be removed.
 * 
 * @return None
 * 
 * @author
 * 08/04/2011 - Created. Arun S
 *
 *****************************************************************************/
void marina_remove_verify_reinsert_system_drives_both_sps(fbe_u32_t no_of_drives_to_be_removed)
{
    fbe_u32_t      i = 0;
    fbe_device_physical_location_t drive_location[2] = {0};
    fbe_api_terminator_device_handle_t  drive_info[2];       //  drive info needed for reinsertion

    if(no_of_drives_to_be_removed > SEP_REBUILD_UTILS_DB_DRIVES_SLOT_1)
    {
        drive_location[0].slot = SEP_REBUILD_UTILS_DB_DRIVES_SLOT_1;
        drive_location[1].slot = SEP_REBUILD_UTILS_DB_DRIVES_SLOT_2;
    }
    else
    {
        drive_location[0].slot = SEP_REBUILD_UTILS_DB_DRIVES_SLOT_1;
    }

    /* Now remove the drive on both the SPs and verify. */
    for(i=0; i < no_of_drives_to_be_removed; i++)
    {
        no_of_physical_objects -=1;

        /* First SPA */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        marina_remove_drive_and_verify(&drive_location[i], no_of_physical_objects, &drive_info[i], FBE_FALSE);

        /* Now SPB */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        marina_remove_drive_and_verify(&drive_location[i], no_of_physical_objects, &drive_info[i], FBE_TRUE);
    }

    /* For case - 1 drive removed: Verify the PSM and VAULT lun is in READY state.
     * For case - 2 drive removed: 3-WAY Mirror - READY, PARITY RG - FAIL
     */
    marina_verify_system_rg_state(no_of_drives_to_be_removed, FALSE);

    /* Reboot the SPs */
    marina_reboot_sp();

    /* For case - 1 drive removed: Verify the PSM and VAULT lun is in READY state.
     * For case - 2 drive removed: 3-WAY Mirror - READY, PARITY RG - SPECIALIZE
     */
    marina_verify_system_rg_state(no_of_drives_to_be_removed, TRUE);

    /* === Take out this code when the below #if 0 is enabled.. ==== */
    no_of_physical_objects = MARINA_TEST_NUMBER_OF_PHYSICAL_OBJECTS;

    /* Enable the code in #if 0 when the code for persisting DB drives and its PVD's is in place.
     * Also, enable the 2 drive removal scenario which is currently disabled in marina_dualsp_test() 
     * and take out the code 'no_of_physical_objects = MARINA_TEST_NUMBER_OF_PHYSICAL_OBJECTS;' just 
     * above this comment as well. 
     */
#if 0
    /* Now re-insert the drives on both the SPs and verify. */
    for(i=0; i < no_of_drives_to_be_removed; i++)
    {
        no_of_physical_objects +=1;

        /* First SPA */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        marina_reinsert_drive_and_verify(&drive_location[i], no_of_physical_objects, &drive_info[i]);

        /* Now SPB */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        marina_reinsert_drive_and_verify(&drive_location[i], no_of_physical_objects, &drive_info[i]);
    }

    /* Verify the PSM and VAULT lun is in READY state. */
    marina_verify_system_rg_state(0, TRUE);
#endif

    return;
}

/*!****************************************************************************
 *  marina_reinsert_drive_and_verify
 ******************************************************************************
 *
 * @brief
 *    This function reinserts a drive.  First it waits for the logical and physical
 *    drive objects to be created.  Then checks the object states of the
 *    PVD and VD for that drive are Activate.
 *
 * @param location              - Info on the drive location
 * @param in_num_objects        - the number of physical objects that should exist
 *                                after the disk is removed
 * @param in_drive_info_p       - pointer to structure that gets filled in with the
 *                                "drive info" for the drive
 * @return None
 * 
 * @author
 * 08/04/2011 - Created. Arun S
 *
 *****************************************************************************/
void marina_reinsert_drive_and_verify(fbe_device_physical_location_t *location, 
                                      fbe_u32_t                       in_num_objects, 
                                      fbe_api_terminator_device_handle_t *in_drive_info_p)
{
    fbe_sim_transport_connection_target_t   sp;
    fbe_status_t                        status;                             // fbe status
    fbe_object_id_t                     pvd_object_id;                      // pvd's object id

    /* Get the loca SP ID */
    sp = fbe_api_sim_transport_get_target_server();
    MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, sp);

    mut_printf(MUT_LOG_LOW, "=== Now operating on - %s ===", sp == FBE_SIM_SP_A ? "SP A" : "SP B");

    /*  Insert the drive */
    if (fbe_test_util_is_simulation())
    {
        mut_printf(MUT_LOG_LOW, "=== Reinserting drive [%d_%d_%d] ===", location->bus, location->enclosure, location->slot);
        fbe_test_pp_util_reinsert_drive(location->bus, location->enclosure, location->slot, *in_drive_info_p);

        /* Verify the PDO and LDO are created by waiting for the number of physical objects to be equal to the
         * value that is passed in.
         */
        status = fbe_api_wait_for_number_of_objects(in_num_objects, MARINA_READY_STATE_WAIT_TIME, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    else
    {
        fbe_test_pp_util_reinsert_drive_hw(location->bus, location->enclosure, location->slot);
    }


    /* Verify the PDO and LDO are in the READY state */
    shaggy_verify_pdo_state(location->bus,
                            location->enclosure,
                            location->slot,
                            FBE_LIFECYCLE_STATE_READY,
                            FBE_FALSE);

    /*  Get the PVD object id */
    status = fbe_api_provision_drive_get_obj_id_by_location(location->bus,
                                                            location->enclosure,
                                                            location->slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== Verifying the PVD and its state on - %s ===", sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    /* Verify that the PVD and VD objects are in the READY state */
    sep_rebuild_utils_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_READY);
    mut_printf(MUT_LOG_LOW, "=== Reinserted drive [%d_%d_%d], PVD: 0x%x, state: READY. ===\n", 
               location->bus, location->enclosure, location->slot, pvd_object_id);

    return;

} // End marina_reinsert_drive_and_verify()

/*!****************************************************************************
 *  marina_reboot_sp
 ******************************************************************************
 *
 * @brief
 *    This function unloads SEP and NEIT package for a particular SP and
 *    restarts it...kinda rebooting the system.
 *
 * @param none
 *
 * @return none
 *
 * @author
 * 08/01/2011 - Created. Arun S
 *
 *****************************************************************************/
static void marina_reboot_sp(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    /* Reboot SEP - Dual SP */
    fbe_test_sep_util_destroy_neit_sep_both_sps();
    sep_config_load_sep_and_neit_both_sps();
    status = fbe_test_sep_util_wait_for_database_service(MARINA_READY_STATE_WAIT_TIME);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

/*!****************************************************************************
 *  marina_verify_system_rg_state
 ******************************************************************************
 *
 * @brief
 *    This function verifies the system RG state before and after drive removals. 
 *
 * @param IN - drive_removed - Drive that is removed.
 *
 * @return none
 *
 * @author
 * 08/05/2011 - Created. Arun S
 *
 *****************************************************************************/
void marina_verify_system_rg_state(fbe_u32_t drive_removed, fbe_bool_t is_rebooted)
{
    marina_system_rg_ids rg_id;

    /* When a drive is removed, and before the SP is rebooted make sure the RAID object is NOT stuck in specialize state. */
    if(drive_removed == SEP_REBUILD_UTILS_DB_DRIVES_SLOT_2)
    {
        /* 2 Drives removed - Verify the PSM is in READY and VAULT lun is in SPECIALIZE state. */
        for(rg_id = MARINA_TRIPLE_MIRROR_RG; rg_id <= MARINA_VAULT_RG; rg_id++)
        {
            if(rg_id == MARINA_TRIPLE_MIRROR_RG)
            {
                shaggy_verify_sep_object_state(MARINA_TRIPLE_MIRROR_RG, FBE_LIFECYCLE_STATE_READY);
            }
            else
            {
                if(is_rebooted)
                {
                    shaggy_verify_sep_object_state(rg_id, FBE_LIFECYCLE_STATE_SPECIALIZE);
                }
                else
                {
                    shaggy_verify_sep_object_state(rg_id, FBE_LIFECYCLE_STATE_FAIL);
                }
            }
        }
    }
    else
    {
        /* No drive removal or 1 Drive removal - Verify the PSM and VAULT lun is in READY state. */
        for(rg_id = MARINA_TRIPLE_MIRROR_RG; rg_id <= MARINA_VAULT_RG; rg_id++)
        {
            shaggy_verify_sep_object_state(rg_id, FBE_LIFECYCLE_STATE_READY);
        }
    }

    return;
}

/*************************
 * end file marina_physical_config.c
 *************************/

