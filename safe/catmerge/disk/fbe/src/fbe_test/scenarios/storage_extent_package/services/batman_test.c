/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file batman_test.c
 ***************************************************************************
 *
 * @brief
 *   This file contains metadata operation test code.
 *
 * @version
 *   02/25/2010 - Created. Amit Dhaduk
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe_test_configurations.h"
#include "pp_utils.h"
#include "sep_utils.h"
#include "sep_tests.h"
#include "fbe_database.h"
#include "fbe_test_package_config.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe_test_common_utils.h"


/*************************
 *   TEST DESCRIPTION
 *************************/

char* batman_short_desc = "Metadata operation test";
char* batman_long_desc =
    "\n"
    "\n"
    " This scenario tests metadata operations. It tests following:\n"
    " 1. Base config paged metadata operation\n"
    " 2. PVD non paged metadata operaiton\n"
    " 3. PVD paged metadata operaiton\n"
    "\n"
    "\n"
    "******* Batman **************************************************************\n"
    "\n"    
    "\n"
    "\n"
    "Starting Config:\n"
    "    [PP] armada board\n"
    "    [PP] SAS PMC port\n"
    "    [PP] viper enclosure\n"
    "    [PP] 1 SAS drives (PDOs)\n"
    "    [PP] 1 logical drives (LDs)\n"
    "    [SEP] 1 provision drives (PVDs)\n"
    "\n"
    "STEP 1: Bring up the initial topology\n"
    "    - Insert a new enclosure having one non configured drives\n"
    "    - Create a PVD object\n"
    "\n"
    "STEP 2: Perform base config paged metadata operation test\n"
    "\n" 
    "STEP 3: Perform non paged metadata operation test for PVD object\n"
    "\n"
    "\n"
    "STEP 4: Perform paged metadata operation test for PVD object\n"
    "\n"
    "\n"
    "STEP 5: Cleanup\n"
    "    - Destroy objects\n"
	"\nUpdated on 10/14/2011\n";    
   

/*************************
 *   DEFINITIONS
 *************************/

static fbe_object_id_t batman_pvd_id = FBE_OBJECT_ID_INVALID;

/*!*******************************************************************
 * @def BATMAN_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects we will create.
 *        (1 board + 2 port + 1 encl + 15 pdo) 
 * Note: Whenever we change make sure to change in batman_test.c file.
 *********************************************************************/
#define BATMAN_TEST_NUMBER_OF_PHYSICAL_OBJECTS 19


/*!*******************************************************************
 * @def BATMAN_TEST_DRIVE_CAPACITY
 *********************************************************************
 * @brief Number of blocks in the physical drive.
 *
 *********************************************************************/
#define BATMAN_TEST_DRIVE_CAPACITY (TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY)

#define PROVISION_DRIVE_PAGED_BITS_SIZE 2

/*!*******************************************************************
 * @def BATMAN_NUMBER_OF_DRIVES_SIM
 *********************************************************************
 * @brief number of drives we configure in the simulator.
 *
 *********************************************************************/
#define BATMAN_NUMBER_OF_DRIVES_SIM 15 

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/

void batman_physical_config(void);
void batman_base_config_paged_metadata_operation_test(void);
void batman_pvd_non_paged_metadata_operation_test(void);

/*****************************************
 * LOCAL FUNCTIONS
 *****************************************/


/*!****************************************************************************
 *  batman_test
 ******************************************************************************
 *
 * @brief
 *   This is the main entry point for the batman test.  
 *
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void batman_test(void)
{

    fbe_status_t                            status;
    fbe_test_discovered_drive_locations_t   drive_locations;
    fbe_test_raid_group_disk_set_t          disk_set;

    /* fill out  all available drive information
     */
    fbe_test_sep_util_discover_all_drive_information(&drive_locations);
    
    /* check if we have atleast one drive with 520 block size
     */
    
    if(fbe_test_get_520_drive_location(&drive_locations, &disk_set) != FBE_STATUS_OK)
    {
        mut_printf(MUT_LOG_LOW, "=== There is no drive available of 520 block  size ===\n");

        /* check if we have atleast one drive with 4160 block size
        */
        status = fbe_test_get_4160_drive_location(&drive_locations, &disk_set);

        MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK,"No drive available of 512 and 520 block size \n");
    }

    /* get  the PVD object id from drive location */
    status = fbe_api_provision_drive_get_obj_id_by_location(disk_set.bus,disk_set.enclosure,disk_set.slot,&batman_pvd_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    /* wait until the PVD object transition to READY state */
    status = fbe_api_wait_for_object_lifecycle_state(batman_pvd_id, FBE_LIFECYCLE_STATE_READY, 10000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    /* disable background zeroing for all PVDs for this test
     * This test is validating paged and non paged metadata IO requests to PVD and if
     * background zeroing is running in parallel, it can change the metadata IO results. 
     */
    fbe_test_sep_drive_disable_background_zeroing_for_all_pvds();

    /* Run the test*/
    
    batman_base_config_paged_metadata_operation_test();

    batman_pvd_non_paged_metadata_operation_test();

    return;

}
/***************************************************************
 * end batman_test()
 ***************************************************************/

/*!****************************************************************************
 *          batman_pvd_non_paged_metadata_operation_test
 ******************************************************************************
 *
 * @brief
 *   This function tests the PVD non paged metadata operation functionality with sending 
 *   set/get checkpoint requests. This function is implemented for testing purpose.
 *
 *
 * @param   None
 *
 * @return  None 

 * @author 
 *  2/24/2010 - Created. Amit Dhaduk
 *
 *****************************************************************************/
void batman_pvd_non_paged_metadata_operation_test(void)
{
    fbe_status_t                    status; 
    fbe_bool_t                      b_is_enabled = FBE_FALSE;
    fbe_api_provision_drive_info_t  provision_drive_info;
	fbe_lba_t                       zero_checkpoint = 0;
	fbe_lba_t                       current_zero_checkpoint = 0;

    /* Set the check point to the second chunk.  The checkpoint must be aligned
     * to the provision drive chunk size.
     */
    status = fbe_api_base_config_is_background_operation_enabled(batman_pvd_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING, &b_is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* disable disk zeroing background operation 
     */
    status = fbe_api_base_config_disable_background_operation(batman_pvd_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the lowest allowable background zero checkpoint plus (1) chunk.
     */
    status = fbe_api_provision_drive_get_info(batman_pvd_id, &provision_drive_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    zero_checkpoint = provision_drive_info.default_offset + provision_drive_info.default_chunk_size;
    
    /* Now set the checkpoint
     */ 
	status = fbe_api_provision_drive_set_zero_checkpoint(batman_pvd_id, zero_checkpoint);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	/* Sleep for a 5 seconds and then confirm that the checkpoint hasn't changed*/
    fbe_api_sleep(5000);
	status = fbe_api_provision_drive_get_zero_checkpoint(batman_pvd_id,  &current_zero_checkpoint);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(current_zero_checkpoint == zero_checkpoint);

    /* Re-enable background zeroing if required
     */
    if (b_is_enabled == FBE_TRUE)
    {
        status = fbe_api_base_config_enable_background_operation(batman_pvd_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    return;
}
/***************************************************************
 * end batman_pvd_non_paged_metadata_operation_test()
 ***************************************************************/
/*!****************************************************************************
 *          batman_base_config_paged_metadata_operation_test
 ******************************************************************************
 *
 * @brief
 *   This function tests the base config paged metadata operation functionality with
 *   sending set/get/clear requests. This function is implemented for testing purpose.
 *
 *
 * @param   None
 *
 * @return  None 

 * @author 
 *  2/24/2010 - Created. Amit Dhaduk
 *
 *****************************************************************************/
void batman_base_config_paged_metadata_operation_test(void)
{

    fbe_status_t status;

    fbe_api_base_config_metadata_paged_change_bits_t paged_change_bits;
    fbe_api_base_config_metadata_paged_get_bits_t paged_get_bits;
    fbe_u32_t i;

    paged_change_bits.metadata_offset = 0;
    paged_change_bits.metadata_record_data_size = 16;
    paged_change_bits.metadata_repeat_count = 1;
    paged_change_bits.metadata_repeat_offset = 0;

    memset(&paged_change_bits.metadata_record_data[0], 0xFF, 16);
    status = fbe_api_base_config_metadata_paged_clear_bits(batman_pvd_id, &paged_change_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    paged_get_bits.metadata_offset = 0;
    paged_get_bits.metadata_record_data_size = 16;
    * (fbe_u32_t *)paged_get_bits.metadata_record_data = 0;
    paged_get_bits.get_bits_flags = 0;
    status = fbe_api_base_config_metadata_paged_get_bits(batman_pvd_id, &paged_get_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(* (fbe_u32_t *)paged_get_bits.metadata_record_data, 0);

    for (i = 0; i < 16; i+=2) {
        paged_change_bits.metadata_record_data[i] = 0xE0; 
        paged_change_bits.metadata_record_data[i+1] = 0xF0; 
    }
    status = fbe_api_base_config_metadata_paged_set_bits(batman_pvd_id, &paged_change_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    * (fbe_u32_t *)paged_get_bits.metadata_record_data = 0;
    status = fbe_api_base_config_metadata_paged_get_bits(batman_pvd_id, &paged_get_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    for (i = 0; i < 16; i+=2) {
        MUT_ASSERT_INT_EQUAL(paged_get_bits.metadata_record_data[i], 0xE0); 
        MUT_ASSERT_INT_EQUAL(paged_get_bits.metadata_record_data[i+1], 0xF0); 
    }

    for (i = 0; i < 16; i+=2) {
        paged_change_bits.metadata_record_data[i] = 0x00; 
        paged_change_bits.metadata_record_data[i+1] = 0x0B; 
    }
    status = fbe_api_base_config_metadata_paged_set_bits(batman_pvd_id, &paged_change_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    * (fbe_u32_t *)paged_get_bits.metadata_record_data = 0;
    status = fbe_api_base_config_metadata_paged_get_bits(batman_pvd_id, &paged_get_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    for (i = 0; i < 16; i+=2) {
        MUT_ASSERT_INT_EQUAL(paged_get_bits.metadata_record_data[i], 0xE0); 
        MUT_ASSERT_INT_EQUAL(paged_get_bits.metadata_record_data[i+1], 0xFB); 
    }
    
    for (i = 0; i < 16; i+=2) {
        paged_change_bits.metadata_record_data[i] = 0xE0; 
        paged_change_bits.metadata_record_data[i+1] = 0xF0; 
    }
    status = fbe_api_base_config_metadata_paged_clear_bits(batman_pvd_id, &paged_change_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    * (fbe_u32_t *)paged_get_bits.metadata_record_data = 0;
    status = fbe_api_base_config_metadata_paged_get_bits(batman_pvd_id, &paged_get_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    for (i = 0; i < 16; i+=2) {
        MUT_ASSERT_INT_EQUAL(paged_get_bits.metadata_record_data[i], 0x00); 
        MUT_ASSERT_INT_EQUAL(paged_get_bits.metadata_record_data[i+1], 0x0B); 
    }
    
    /* Offset test */
    paged_change_bits.metadata_offset = 512 + 64;
    paged_change_bits.metadata_record_data_size = 16;
    paged_change_bits.metadata_repeat_count = 1;
    paged_change_bits.metadata_repeat_offset = 0;

    memset(&paged_change_bits.metadata_record_data[0], 0xFF, 16);
    status = fbe_api_base_config_metadata_paged_clear_bits(batman_pvd_id, &paged_change_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    paged_get_bits.metadata_offset = 512 + 64;
    paged_get_bits.metadata_record_data_size = 16;
    * (fbe_u32_t *)paged_get_bits.metadata_record_data = 0;
    status = fbe_api_base_config_metadata_paged_get_bits(batman_pvd_id, &paged_get_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(* (fbe_u32_t *)paged_get_bits.metadata_record_data, 0);

    for (i = 0; i < 16; i+=2) {
        paged_change_bits.metadata_record_data[i] = 0xE0; 
        paged_change_bits.metadata_record_data[i+1] = 0xF0; 
    }
    status = fbe_api_base_config_metadata_paged_set_bits(batman_pvd_id, &paged_change_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    * (fbe_u32_t *)paged_get_bits.metadata_record_data = 0;
    status = fbe_api_base_config_metadata_paged_get_bits(batman_pvd_id, &paged_get_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    for (i = 0; i < 16; i+=2) {
        MUT_ASSERT_INT_EQUAL(paged_get_bits.metadata_record_data[i], 0xE0); 
        MUT_ASSERT_INT_EQUAL(paged_get_bits.metadata_record_data[i+1], 0xF0); 
    }

    for (i = 0; i < 16; i+=2) {
        paged_change_bits.metadata_record_data[i] = 0x00; 
        paged_change_bits.metadata_record_data[i+1] = 0x0B; 
    }
    status = fbe_api_base_config_metadata_paged_set_bits(batman_pvd_id, &paged_change_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    * (fbe_u32_t *)paged_get_bits.metadata_record_data = 0;
    status = fbe_api_base_config_metadata_paged_get_bits(batman_pvd_id, &paged_get_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    for (i = 0; i < 16; i+=2) {
        MUT_ASSERT_INT_EQUAL(paged_get_bits.metadata_record_data[i], 0xE0); 
        MUT_ASSERT_INT_EQUAL(paged_get_bits.metadata_record_data[i+1], 0xFB); 
    }
    
    for (i = 0; i < 16; i+=2) {
        paged_change_bits.metadata_record_data[i] = 0xE0; 
        paged_change_bits.metadata_record_data[i+1] = 0xF0; 
    }
    status = fbe_api_base_config_metadata_paged_clear_bits(batman_pvd_id, &paged_change_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    * (fbe_u32_t *)paged_get_bits.metadata_record_data = 0;
    status = fbe_api_base_config_metadata_paged_get_bits(batman_pvd_id, &paged_get_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    for (i = 0; i < 16; i+=2) {
        MUT_ASSERT_INT_EQUAL(paged_get_bits.metadata_record_data[i], 0x00); 
        MUT_ASSERT_INT_EQUAL(paged_get_bits.metadata_record_data[i+1], 0x0B); 
    }

    return;
}
/***************************************************************
 * batman_base_config_paged_metadata_operation_test()
 ***************************************************************/



/*!****************************************************************************
 *  batman_setup
 ******************************************************************************
 *
 * @brief
 *   This is the setup function for the batman test.  
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void batman_setup(void)
{    
    fbe_status_t status = FBE_STATUS_OK;
    
    mut_printf(MUT_LOG_LOW, "=== batman setup ===\n");

    if(fbe_test_util_is_simulation())
    {
        /* load the physical package */
        batman_physical_config();

        sep_config_load_sep();

        /* Wait for this number of PVD objects to exist.
         */
        status = fbe_api_wait_for_number_of_class(FBE_CLASS_ID_PROVISION_DRIVE, 
                                                  BATMAN_NUMBER_OF_DRIVES_SIM,
                                                  120000, 
                                                  FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        fbe_test_wait_for_objects_of_class_state(FBE_CLASS_ID_PROVISION_DRIVE, 
                                                 FBE_PACKAGE_ID_SEP_0,
                                                 FBE_LIFECYCLE_STATE_READY,
                                                 120000);
    }
    mut_printf(MUT_LOG_LOW, "=== complete SEP setup ===\n");		

    
    return;

}
/***************************************************************
 * end batman_setup()
 ***************************************************************/



/*!****************************************************************************
 *  batman_destroy
 ******************************************************************************
 *
 * @brief
 *   This function unloads the logical and physical packages and destroy the configuration.
 *
 * @param   None
 *
 * @return  None 
 *
 *
 * @author 
 *  2/24/2010 - Created. Amit Dhaduk
 *
 *****************************************************************************/
void batman_destroy(void)
{
    mut_printf(MUT_LOG_LOW, "=== batman destroy starts ===\n");
    if(fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_sep_physical();
    }
    mut_printf(MUT_LOG_LOW, "=== batman destroy completes ===\n");

    return;
}
/***************************************************************
 * end batman_destroy()
 ***************************************************************/


/*!**************************************************************
 * batman_physical_config()
 ****************************************************************
 * @brief
 *  Configure the batman test's physical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 * @author
 *  2/24/2010 - Created. Amit Dhaduk
 *
 ****************************************************************/

void batman_physical_config(void)
{
    fbe_status_t                        status  = FBE_STATUS_GENERIC_FAILURE;

    fbe_u32_t                           total_objects = 0;
    fbe_class_id_t                      class_id;

    fbe_api_terminator_device_handle_t  port0_handle;
    fbe_api_terminator_device_handle_t  port1_handle;
    fbe_api_terminator_device_handle_t  encl0_0_handle;
    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_u32_t slot;

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
    for ( slot = 0; slot < BATMAN_NUMBER_OF_DRIVES_SIM; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 0, slot, 520, BATMAN_TEST_DRIVE_CAPACITY, &drive_handle);
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
    status = fbe_api_wait_for_number_of_objects(BATMAN_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 20000, FBE_PACKAGE_ID_PHYSICAL);
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
    MUT_ASSERT_TRUE(total_objects == BATMAN_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");

    return;
}
/******************************************
 * end batman_physical_config()
 ******************************************/


/*************************
 * end file batman_test.c
 *************************/
