/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file hulk_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the PVD Pool-id update test.
 *  There are two way to update pvd pool id: 1. update one pvd pool id per transaction(dumku_test cover this case);
 *  2. update multiple pvds pool id per transaction.
 *
 * @version
 *   04/08/2013   - Created . Hongpo Gao
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe_test_package_config.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe_test_configurations.h"
#include "pp_utils.h"
#include "sep_utils.h"
#include "sep_tests.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"


char * hulk_short_desc = "PVD Storage Pool-id test(update in parallel)";
char * hulk_long_desc ="\n"
    "The Hulk scenario tests that the parallel PVD Pool-id update .\n"
    "\n"
    "\n"
    "Starting Config:\n"
    "    [PP] 1 - armada board\n"
    "    [PP] 3 - SAS PMC port\n"
    "    [PP] 5 - viper enclosures\n"
    "    [PP] 75 - SAS drives (PDOs)\n"
    "    [PP] 75 - Logical drives (LDs)\n"
    "\n"
    "\n"
    "STEP 1: Bring up the initial topology\n"
    "    - Create and verify the initial physical package config\n"
    "\n"
    "STEP 2: Update PVD pool id with the new interface."
    "STEP 3: reboot and verify it."
    "STEP 4: compare the performance between old and new interface"
    "Update PVD pool id one by one with the old interface. record the start and end timestamp "
    "Update PVD pool id in parallel with the new interface. record the start and end timestamp"
    "\n"
    "Description last updated: 04/08/2013.\n";
   

/*!*******************************************************************
 * @def HULK_TEST_DRIVE_CAPACITY
 *********************************************************************
 * @brief Number of blocks in the physical drive.
 *
 *********************************************************************/
#define HULK_TEST_DRIVE_CAPACITY (TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY)

/*!*******************************************************************
 * @def HULK_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects we will create.
 *        (1 board + 3 port + 5 encl + 75 pdo) 
 *********************************************************************/
#define HULK_TEST_NUMBER_OF_PHYSICAL_OBJECTS 84


#define HULK_TEST_PVD_NUMBER 60
fbe_object_id_t hulk_pvd_id_array[HULK_TEST_PVD_NUMBER];


/*!*******************************************************************
 * @def HULK_WAIT_MSEC
 *********************************************************************
 * @brief Number of seconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
/* Maximum number of hot spares. */
#define HULK_WAIT_MSEC 30000

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/
static void hulk_physical_config(void);
static void hulk_verify_pvd_pool_id(void);
static void hulk_test_create_rg(void);
static void hulk_reboot_and_verify_pvd_pool_id(void);
static void hulk_compare_two_update_pvd_pook_id_interface(void);

/*!****************************************************************************
 *  hulk_setup
 ******************************************************************************
 *
 * @brief
 *   This is the setup function for the dumku test.  
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void hulk_setup(void)
{    
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t           slot;
    fbe_u32_t           port;
    fbe_u32_t           encl;
    fbe_u32_t           index;
    fbe_object_id_t     pvd_id;

    mut_printf(MUT_LOG_LOW, "=== dumku setup ===\n");       

    /* load the physical package */
    hulk_physical_config();

    /* load the logical package */
    sep_config_load_sep_and_neit();
    /* Wait for all pvds ready before getting pvd object id */
    fbe_test_wait_for_all_pvds_ready();
    /* Get all the pvd ids */
    fbe_zero_memory(hulk_pvd_id_array, sizeof(fbe_object_id_t)*HULK_TEST_PVD_NUMBER);
    index = 0;
    port = 0;
    for (encl = 1; encl < 3; encl++)
    {
        for(slot = 0; slot < 15; slot++)
        {
            status = fbe_api_provision_drive_get_obj_id_by_location(port, encl, slot, &pvd_id);
            MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);
            hulk_pvd_id_array[index++] = pvd_id;
        }
    }
    encl = 0;
    for (port = 0; port < 2; port++)
    {
        for(slot = 0; slot < 15; slot++)
        {
            status = fbe_api_provision_drive_get_obj_id_by_location(port, encl, slot, &pvd_id);
            MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);
            hulk_pvd_id_array[index++] = pvd_id;
        }
    }  

    return;
}
/***************************************************************
 * end hulk_setup()
 ***************************************************************/


/*!**************************************************************
 * hulk_physical_config()
 ****************************************************************
 * @brief
 *  Configure the yeti test's physical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 *
 ****************************************************************/

static void hulk_physical_config(void)
{
    fbe_status_t                        status  = FBE_STATUS_GENERIC_FAILURE;

    fbe_u32_t                           total_objects = 0;
    fbe_class_id_t                      class_id;

    fbe_terminator_api_device_handle_t  port0_handle;
    fbe_terminator_api_device_handle_t  port1_handle;
    fbe_terminator_api_device_handle_t  port2_handle;
    fbe_terminator_api_device_handle_t  encl0_0_handle;
    fbe_terminator_api_device_handle_t  encl0_1_handle;
    fbe_terminator_api_device_handle_t  encl0_2_handle;
    fbe_terminator_api_device_handle_t  encl1_0_handle;
    fbe_terminator_api_device_handle_t  encl2_0_handle;
    fbe_terminator_api_device_handle_t  drive_handle;
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
    fbe_test_pp_util_insert_sas_pmc_port(3, /* io port */
                                         2, /* portal */
                                         2, /* backend number */ 
                                         &port2_handle);

    /* insert an enclosures to port 0
    */
    fbe_test_pp_util_insert_viper_enclosure(0, 0, port0_handle, &encl0_0_handle);
    fbe_test_pp_util_insert_viper_enclosure(0, 1, port0_handle, &encl0_1_handle);
    fbe_test_pp_util_insert_viper_enclosure(0, 2, port0_handle, &encl0_2_handle);

    /* insert an enclosures to port 1
    */
    fbe_test_pp_util_insert_viper_enclosure(1, 0, port1_handle, &encl1_0_handle);

    /* insert an enclosures to port 2
    */
    fbe_test_pp_util_insert_viper_enclosure(2, 0, port2_handle, &encl2_0_handle);

    /* Insert drives for enclosures.
    */
    for ( slot = 0; slot < 15; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 0, slot, 520, HULK_TEST_DRIVE_CAPACITY, &drive_handle);
    }
    for ( slot = 0; slot < 15; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 1, slot, 520, HULK_TEST_DRIVE_CAPACITY, &drive_handle);
    }
    for ( slot = 0; slot < 15; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 2, slot, 520, HULK_TEST_DRIVE_CAPACITY, &drive_handle);
    }
    for ( slot = 0; slot < 15; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(1, 0, slot, 520, HULK_TEST_DRIVE_CAPACITY, &drive_handle);
    }
    for ( slot = 0; slot < 15; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(2, 0, slot, 520, HULK_TEST_DRIVE_CAPACITY, &drive_handle);
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
    status = fbe_api_wait_for_number_of_objects(HULK_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 20000, FBE_PACKAGE_ID_PHYSICAL);
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
    MUT_ASSERT_TRUE(total_objects == HULK_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");

    for (object_id = 0 ; object_id < HULK_TEST_NUMBER_OF_PHYSICAL_OBJECTS; object_id++)
    {
        status = fbe_api_wait_for_object_lifecycle_state(object_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    return;
}
/******************************************
 * end hulk_physical_config()
 ******************************************/

/*!****************************************************************************
 *  hulk_test
 ******************************************************************************
 *
 * @brief
 *   This is the main entry point for the dumku test.  
 *
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void hulk_test(void)
{

    mut_printf(MUT_LOG_LOW, "=== Dumku test started ===\n");   
	
    /* Verify the Pool-id for the PVD */
    hulk_verify_pvd_pool_id();

    /* Reboot and verify the PVD pool-id */
    hulk_reboot_and_verify_pvd_pool_id();

    /* Update multi pvds pool id (60) with two kinds of interface */
    hulk_compare_two_update_pvd_pook_id_interface();
    
    return;

}
/***************************************************************
 * end hulk_test()
 ***************************************************************/

/*!****************************************************************************
 *  hulk_verify_pvd_pool_id
 ******************************************************************************
 *
 * @brief
 *   This function verifies the PVD pool-id.
 *
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void hulk_verify_pvd_pool_id(void)
{
    fbe_status_t                                status;
    fbe_object_id_t                             expected_pool_id;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Verifying the GET/SET pool-id API for two PVD.\n", __FUNCTION__);
    /* We use the pvd object id as the pool id */
    fbe_test_sep_util_update_multi_provision_drive_pool_id(hulk_pvd_id_array, (fbe_u32_t *)hulk_pvd_id_array, 2);
    /* Verify the pool id is as expected */
    status = fbe_api_provision_drive_get_pool_id(hulk_pvd_id_array[0], &expected_pool_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);   
	MUT_ASSERT_INTEGER_EQUAL_MSG(hulk_pvd_id_array[0], expected_pool_id, "Unexpected Pool id");
    status = fbe_api_provision_drive_get_pool_id(hulk_pvd_id_array[1], &expected_pool_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);   
	MUT_ASSERT_INTEGER_EQUAL_MSG(hulk_pvd_id_array[1], expected_pool_id, "Unexpected Pool id");

    return;
}

void hulk_compare_two_update_pvd_pook_id_interface(void)
{
    fbe_u32_t index;
    
    /* Update the pvd pool id one by one */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: update pvd pool id one by one with the old interface -----START.\n", __FUNCTION__);
    for (index = 0; index < HULK_TEST_PVD_NUMBER; index++)
    {
        /* We use the object id as the pool id */
        fbe_test_sep_util_provision_drive_pool_id(hulk_pvd_id_array[index], (fbe_u32_t)hulk_pvd_id_array);
    }
    mut_printf(MUT_LOG_TEST_STATUS, "%s: update pvd pool id one by one with the old interface -----END.\n", __FUNCTION__);

    /* Update the pvd pool id in parallel with new interface */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: update pvd pool id in parallel with the new interface -----START.\n", __FUNCTION__);
    /* We use the object id as the pool id */
    fbe_test_sep_util_update_multi_provision_drive_pool_id(hulk_pvd_id_array, (fbe_u32_t*)hulk_pvd_id_array, HULK_TEST_PVD_NUMBER);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: update pvd pool id in parallel with the new interface -----END .\n", __FUNCTION__);
 
}

/*!****************************************************************************
 *  hulk_reboot_and_verify_pvd_pool_id
 ******************************************************************************
 *
 * @brief
 *   This function verifies the PVD pool-id after the reboot.
 *
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void hulk_reboot_and_verify_pvd_pool_id(void)
{
    fbe_status_t                                status;
    fbe_u32_t                                   expected_pool_id;

    mut_printf(MUT_LOG_LOW, "=== Rebooting the SP ===\n");
    /*reboot sep*/
    fbe_test_sep_util_destroy_neit_sep();
    sep_config_load_sep_and_neit();
    status = fbe_test_sep_util_wait_for_database_service(HULK_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Now get the pool-id for the PVD. It is Persisted and so should contain the new id. */
    /* Verify the pool id is as expected */
    status = fbe_api_provision_drive_get_pool_id(hulk_pvd_id_array[0], &expected_pool_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);   
	MUT_ASSERT_INTEGER_EQUAL_MSG(hulk_pvd_id_array[0], expected_pool_id, "Unexpected Pool id");
    status = fbe_api_provision_drive_get_pool_id(hulk_pvd_id_array[1], &expected_pool_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);   
	MUT_ASSERT_INTEGER_EQUAL_MSG(hulk_pvd_id_array[1], expected_pool_id, "Unexpected Pool id");
}

/*!****************************************************************************
 *  hulk_destroy
 ******************************************************************************
 *
 * @brief
 *   This function unloads the logical and physical packages and destroy the configuration.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void hulk_destroy(void)
{
    mut_printf(MUT_LOG_LOW, "=== dumku destroy starts ===\n");
    fbe_test_sep_util_destroy_neit_sep_physical();
    mut_printf(MUT_LOG_LOW, "=== dumku destroy completes ===\n");

    return;
}
/***************************************************************
 * end hulk_destroy()
 ***************************************************************/


