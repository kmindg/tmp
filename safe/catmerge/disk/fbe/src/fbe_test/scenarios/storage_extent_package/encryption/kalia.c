/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file kalia_test.c
 ***************************************************************************
 *
 * @brief
 *   This file includes tests for Basic Encryption Setup.
 *
 * @version
 *   10/03/2013 - Created.  Ashok Tamilarasan
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
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_encryption_interface.h"
#include "fbe/fbe_api_port_interface.h"
#include "pp_utils.h"
#include "fbe_test_common_utils.h"
#include "fbe_test_configurations.h"

/*-----------------------------------------------------------------------------
    TEST DESCRIPTION:
*/

char * kalia_short_desc = "Basic Encryption Key Setup";
char * kalia_long_desc =
    "\n"
    "\n"
    "The Kalia scenario tests that the Keys are registered correctly and key handles setup\n"
    "\n"
    "Starting Config:\n"
    "\t [PP] 1-armada board\n"
    "\t [PP] 1-SAS PMC port\n"
    "\t [PP] 4-viper enclosure\n"
    "\t [PP] 60 SAS drives (PDO)\n"
    "\t [PP] 60 logical drives (LD)\n"
    "\n"
    "STEP 1: Bring up the initial topology.\n"
    "\t- Create and verify the initial physical package config.\n"
    "\t- Verify all the PDO, LDO and PVDs are created and in expected state.\n"
    "\n"
    "STEP 2: Create RG and LUNs\n"
    "\t- Create a raid object with I/O edges attached to all VDs.\n"
    "\t- Create a lun object with an I/O edge attached to the RAID\n"
    "\t  Lun object can be created with exact bind size\n"
    "\t  Lun object can be created with bind offset to test bind offset sizes\n"
    "\t- Verify that all configured objects are in the READY state.\n"
    "\n";

/*-----------------------------------------------------------------------------
    DEFINITIONS:
*/
/*!*******************************************************************
 * @def KALIA_TEST_RAID_GROUP_ID
 *********************************************************************
 * @brief RAID Group id used by this test
 *
 *********************************************************************/
#define KALIA_TEST_RAID_GROUP_ID        0

/*!*******************************************************************
 * @def KALIA_TEST_RAID_GROUP_WIDTH
 *********************************************************************
 * @brief RAID Group width used by this test
 *
 *********************************************************************/
#define KALIA_TEST_RAID_GROUP_WIDTH        3

#define FBE_TEST_KALIA_NUM_PORTS           3
/*!*******************************************************************
 * @var kalia_rg_configuration
 *********************************************************************
 * @brief This is rg_configuration
 *
 *********************************************************************/
static fbe_test_rg_configuration_t kalia_rg_configuration[] = 
{
 /* width, capacity,        raid type,                        class,               block size, RAID-id,                        bandwidth.*/
   { KALIA_TEST_RAID_GROUP_WIDTH,     0xE000, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY, 520,        KALIA_TEST_RAID_GROUP_ID, 0},
    
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};


/*-----------------------------------------------------------------------------
    FORWARD DECLARATIONS:
*/

static void kalia_test_load_physical_config(void);
static void kalia_test_create_rg(fbe_test_rg_configuration_t     *rg_config_p);
static void kalia_run_test(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p);
static void kalia_run_dualsp_test(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p);
void kalia_test_encryption_mode_setup(fbe_test_rg_configuration_t *rg_config_ptr);
void kalia_test_validate_locked_rg_info(fbe_test_rg_configuration_t *rg_config_ptr);
void kalia_test_validate_pvd_config_type(fbe_test_rg_configuration_t *rg_config_ptr);
static void kalia_test_insert_drive_after_encryption_enabled(fbe_test_rg_configuration_t *rg_config_ptr);
static void kalia_test_set_backup_info(fbe_test_rg_configuration_t *rg_config_ptr);
static void kalia_test_validate_port_info(fbe_test_rg_configuration_t *rg_config_ptr);

/*-----------------------------------------------------------------------------
    FUNCTIONS:
*/


/*!****************************************************************************
 * kalia_dualsp_test()
 ******************************************************************************
 * @brief
 *  This is the main entry point for the Kalia dual SP test  
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  10/07/2013 - Created. Ashok Tamilarasan
 ******************************************************************************/

void kalia_dualsp_test(void)
{

    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t num_raid_groups = 0;

    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    rg_config_p = &kalia_rg_configuration[0];

    fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

    fbe_test_run_test_on_rg_config_no_create(rg_config_p,0,kalia_run_dualsp_test);
    return;

}/* End plankton_test() */

/*!****************************************************************************
 * kalia_test()
 ******************************************************************************
 * @brief
 *  This is the main entry point for the Kalia test  
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  10/07/2013 - Created. Ashok Tamilarasan
 ******************************************************************************/

void kalia_test(void)
{

    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t num_raid_groups = 0;

    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    rg_config_p = &kalia_rg_configuration[0];

    fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

    fbe_test_run_test_on_rg_config_no_create(rg_config_p,0,kalia_run_test);
    return;

}/* End plankton_test() */


/*!****************************************************************************
 *  kalia_run_test
 ******************************************************************************
 *
 * @brief
 *  This is the main entry point for the Kalia Test
 *
 *
 * @param   None
 *
 * @return  None 
 *
 * @Modified
 * 10/07/2013 - Created. Ashok Tamilarasan
 *****************************************************************************/
void kalia_run_test(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p)
{
    fbe_status_t        status;
    fbe_database_control_setup_encryption_key_t encryption_info;
    fbe_object_id_t rg_object_id;
    
    mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Kalia Single SP Test ===\n");

    /* Before we create the RG, enable encryption */
    status = fbe_api_enable_system_encryption(0, 0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Enabling of encryption failed\n");


    /* Test if re-enables gives us already enabled status*/
    status = fbe_api_enable_system_encryption(0, 0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_NO_ACTION, status, "Re-Enabling of encryption failed\n");

    /* We have to provide keys for the vault */
    fbe_test_sep_util_generate_encryption_key(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG, &encryption_info);
    status = fbe_api_database_setup_encryption_key(&encryption_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_test_sep_util_create_configuration(rg_config_ptr, 1, FBE_TRUE);

    /* Test Case #1: Validate that KEK is correctly being setup and destroyed properly */
    fbe_test_sep_util_setup_kek(rg_config_ptr);
    fbe_test_sep_util_validate_and_destroy_kek_setup(rg_config_ptr);

    /* Test case #2: Validate that the keys are being setup correctly */
    fbe_test_sep_util_validate_key_setup(rg_config_ptr);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Validate that command is rejected if there is a key already setup for this... ===\n");

    /* Verify the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_ptr->raid_group_id, &rg_object_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
    fbe_zero_memory(&encryption_info, sizeof(fbe_database_control_setup_encryption_key_t));
    fbe_test_sep_util_generate_encryption_key(rg_object_id, &encryption_info);
    status = fbe_api_database_setup_encryption_key(&encryption_info);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);

    /* Test Case #3: Validate that encryption mode is correctly sent to the objects */
    kalia_test_encryption_mode_setup(rg_config_ptr);

    /* Test Case #4: Validate that Locked RG information is being returned correctly */
    //kalia_test_validate_locked_rg_info(rg_config_ptr);

	/* Test Case #5: Validate that PVD has valid config type */
	kalia_test_validate_pvd_config_type(rg_config_ptr);

    /* Test case #6: Validate that KEKs of KEKs support */
    kalia_test_validate_kek_of_kek_handling(rg_config_ptr);

    /* Test case #7: insert a new drive, and verify the PVD state */
    kalia_test_insert_drive_after_encryption_enabled(rg_config_ptr);

    /* Test case #8: test fbe_api_database_set_backup_info */
    kalia_test_set_backup_info(rg_config_ptr);

    /* Test Case #9: Validate all the port information */
    kalia_test_validate_port_info(rg_config_ptr);
}

/*!****************************************************************************
 *  kalia_test
 ******************************************************************************
 *
 * @brief
 *  This is the main entry point for the Kalia Test
 *
 *
 * @param   None
 *
 * @return  None 
 *
 * @Modified
 * 10/07/2013 - Created. Ashok Tamilarasan
 *****************************************************************************/
void kalia_run_dualsp_test(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p)
{
    fbe_status_t        status;
    fbe_database_control_setup_encryption_key_t encryption_info;
    fbe_object_id_t rg_object_id;
    
    mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Kalia Single SP Test ===\n");

    /* Before we create the RG, enable encryption */
    status = fbe_api_enable_system_encryption(0, 0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Enabling of encryption failed\n");


    /* Test if re-enables gives us already enabled status*/
    status = fbe_api_enable_system_encryption(0, 0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_NO_ACTION, status, "Re-Enabling of encryption failed\n");

    fbe_test_sep_util_create_configuration(rg_config_ptr, 1, FBE_TRUE);

    /* Test Case #1: Validate that KEK is correctly being setup and destroyed properly */
    fbe_test_sep_util_setup_kek(rg_config_ptr);
    fbe_test_sep_util_validate_and_destroy_kek_setup(rg_config_ptr);

    /* Test case #2: Validate that the keys are being setup correctly */
    fbe_test_sep_util_validate_key_setup(rg_config_ptr);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Validate that command is rejected if there is a key already setup for this... ===\n");

    /* Verify the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_ptr->raid_group_id, &rg_object_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
    fbe_zero_memory(&encryption_info, sizeof(fbe_database_control_setup_encryption_key_t));
    fbe_test_sep_util_generate_encryption_key(rg_object_id, &encryption_info);
    status = fbe_api_database_setup_encryption_key(&encryption_info);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);

	/* Test Case #5: Validate that PVD has valid config type */
	kalia_test_validate_pvd_config_type(rg_config_ptr);

    /* Test case #6: Validate that KEKs of KEKs support */
    kalia_test_validate_kek_of_kek_handling(rg_config_ptr);

    /* Test Case #7: Validate all the port information */
    kalia_test_validate_port_info(rg_config_ptr);
}

static void kalia_test_set_backup_info(fbe_test_rg_configuration_t *rg_config_ptr)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_database_backup_info_t          backup_info;

    mut_printf(MUT_LOG_TEST_STATUS, "=== set encryption backup state to FBE_ENCRYPTION_BACKUP_REQUIERED... ===\n");

    backup_info.encryption_backup_state = FBE_ENCRYPTION_BACKUP_REQUIERED;
    status = fbe_api_database_set_backup_info(&backup_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_database_get_backup_info(&backup_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_ENCRYPTION_BACKUP_REQUIERED, backup_info.encryption_backup_state);

    mut_printf(MUT_LOG_TEST_STATUS, "=== set encryption backup state to FBE_ENCRYPTION_BACKUP_IN_PROGRESS... ===\n");
    backup_info.encryption_backup_state = FBE_ENCRYPTION_BACKUP_IN_PROGRESS;
    status = fbe_api_database_set_backup_info(&backup_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    backup_info.encryption_backup_state = FBE_ENCRYPTION_BACKUP_INVALID;
    status = fbe_api_database_get_backup_info(&backup_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_ENCRYPTION_BACKUP_IN_PROGRESS, backup_info.encryption_backup_state);

    mut_printf(MUT_LOG_TEST_STATUS, "=== set encryption backup state to FBE_ENCRYPTION_BACKUP_COMPLETED... ===\n");
    backup_info.encryption_backup_state = FBE_ENCRYPTION_BACKUP_COMPLETED;
    status = fbe_api_database_set_backup_info(&backup_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_database_get_backup_info(&backup_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_ENCRYPTION_BACKUP_COMPLETED, backup_info.encryption_backup_state);
}
static void kalia_test_insert_drive_after_encryption_enabled(fbe_test_rg_configuration_t *rg_config_ptr)
{
    fbe_api_terminator_device_handle_t  drive_handle;
	fbe_api_provision_drive_info_t      provision_drive_info_p;
    fbe_object_id_t                     pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_u32_t                           wait_attempts = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Inserting new drive and verify pvd state... ===\n");

    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    fbe_test_pp_util_insert_sas_drive(0, 1, 0, 520, TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY, &drive_handle);

   
    status = FBE_STATUS_GENERIC_FAILURE;
    while ((status != FBE_STATUS_OK) &&
        (wait_attempts < 20)         )
    {
        /* Wait until we can get pvd object id.
        */
        status = fbe_api_provision_drive_get_obj_id_by_location(0, 1, 0, &pvd_object_id);
        if (status != FBE_STATUS_OK)
        {
            fbe_api_sleep(500);
        }
    }
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id, FBE_LIFECYCLE_STATE_READY, 70000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    MUT_ASSERT_INT_EQUAL(provision_drive_info_p.created_after_encryption_enabled, FBE_TRUE);

    /* reboot SP */
    fbe_test_common_reboot_sp(FBE_SIM_SP_A, NULL, NULL);

    status = fbe_api_provision_drive_get_obj_id_by_location(0, 1, 0, &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    MUT_ASSERT_INT_EQUAL(provision_drive_info_p.created_after_encryption_enabled, FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Completed validation of new drives after system encrypted  ===\n");
    return;
}



void kalia_test_encryption_mode_setup(fbe_test_rg_configuration_t *rg_config_ptr)
{
    fbe_status_t        status;
    fbe_database_control_setup_encryption_key_t encryption_info;
    fbe_object_id_t rg_object_id;
    fbe_u32_t rg_count;
    fbe_u32_t total_locked_objects;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Validate that object are getting the encryption mode correctly... ===\n");
    /* Make sure the encryption mode is setup correctly */
    fbe_test_sep_util_validate_encryption_mode_setup(rg_config_ptr, FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED);

    /* First get the number of RGs in the configuration */
    rg_count = fbe_test_get_rg_count(rg_config_ptr);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Reboot SPs.... ===\n");

    /* Reboot the SP */
    fbe_test_common_reboot_sp(FBE_SIM_SP_A, NULL, NULL);

    mut_printf(MUT_LOG_TEST_STATUS, "=== After reboot..Validate that object are getting the encryption mode correctly... ===\n");

    status = fbe_test_util_wait_for_rg_object_id_by_number(rg_config_ptr->raid_group_id, &rg_object_id, 18000);
    MUT_ASSERT_INT_NOT_EQUAL(rg_object_id, FBE_OBJECT_ID_INVALID);

    status = fbe_test_util_wait_for_encryption_state(rg_object_id, 
                                                     FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_CURRENT_KEYS_REQUIRED, 
                                                     18000);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Encryption state unexpected\n");

    fbe_api_wait_for_object_lifecycle_state(rg_object_id,FBE_LIFECYCLE_STATE_SPECIALIZE, 1800, FBE_PACKAGE_ID_SEP_0);


    status = fbe_test_util_wait_for_encryption_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG, 
                                                     FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_CURRENT_KEYS_REQUIRED, 
                                                     18000);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Encryption state unexpected for vault\n");


    fbe_api_wait_for_object_lifecycle_state(rg_object_id,FBE_LIFECYCLE_STATE_SPECIALIZE, 1800, FBE_PACKAGE_ID_SEP_0);

    /* Get the number of objects in locked state */
    status = fbe_api_database_get_total_locked_objects_of_class(DATABASE_CLASS_ID_RAID_GROUP, &total_locked_objects);

    /* Validate that the number of objects in locked state is same as the number of RGs in Config
     * We need to include vault which also wants to be encrypted. 
     */
 
    MUT_ASSERT_INT_EQUAL_MSG(rg_count + 1, total_locked_objects, "Unexpected number of locked objects\n");

     /* Verify the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_ptr->raid_group_id, &rg_object_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
    fbe_zero_memory(&encryption_info, sizeof(fbe_database_control_setup_encryption_key_t));
    fbe_test_sep_util_generate_encryption_key(rg_object_id, &encryption_info);
    status = fbe_api_database_setup_encryption_key(&encryption_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

     /* verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
                                                     FBE_LIFECYCLE_STATE_READY, 120000,
                                                     FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Make sure we read from the persistent storage and populate the objects */
    fbe_test_sep_util_validate_encryption_mode_setup(rg_config_ptr, FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED);        

    return;

} /* End kalia_test() */


void kalia_test_validate_locked_rg_info(fbe_test_rg_configuration_t *rg_config_ptr)
{
    fbe_u32_t rg_count;
    fbe_u32_t index;
    fbe_object_id_t rg_object_id;
    fbe_status_t status;
    fbe_database_control_get_locked_object_info_t *info_buffer;
    fbe_u32_t actual_num_objects;
    fbe_config_generation_t generation_number;
    fbe_database_control_get_locked_object_info_t *temp_buffer_ptr;
    fbe_test_rg_configuration_t *temp_rg_ptr;
    fbe_api_base_config_get_width_t width_info;
    fbe_u32_t total_locked_objects;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Validate that RG Info for Locked objects are returned correctly ===\n");

    /* First get the number of RGs in the configuration */
    rg_count = fbe_test_get_rg_count(rg_config_ptr);

    /* Get the number of objects in locked state */
    status = fbe_api_database_get_total_locked_objects_of_class(DATABASE_CLASS_ID_RAID_GROUP, &total_locked_objects);

    info_buffer = (fbe_database_control_get_locked_object_info_t *) fbe_api_allocate_memory( total_locked_objects * 
                                                                                           sizeof(fbe_database_control_get_locked_object_info_t));

    MUT_ASSERT_NOT_NULL_MSG(info_buffer, "Unable to allocate memory \n");

    /* Get the information about the locked objects */

    status = fbe_api_database_get_locked_object_info_of_class(DATABASE_CLASS_ID_RAID_GROUP,
                                                              info_buffer, 
                                                              total_locked_objects,
                                                              &actual_num_objects);

    MUT_ASSERT_INT_EQUAL_MSG(total_locked_objects, actual_num_objects, "Unexpected number of objects\n");

    temp_rg_ptr = rg_config_ptr;
    temp_buffer_ptr = info_buffer;

    /* For each of the object validate that the information is all as expected */
    /* Set the encryption state to be a locked state */
    for(index = 0; index < rg_count; index ++)
    {
        /* Verify the object id of the raid group */
        status = fbe_api_database_lookup_raid_group_by_number(
                    rg_config_ptr->raid_group_id, &rg_object_id);
        /* Skip over vault since we did not encrypt it.
         */
        if (info_buffer->control_number == FBE_PRIVATE_SPACE_LAYOUT_RG_ID_VAULT) {
            info_buffer++;
            continue;
        }
        MUT_ASSERT_INT_EQUAL_MSG(rg_config_ptr->raid_group_id,
                                 info_buffer->control_number, "RG Number not expected\n");
        MUT_ASSERT_INT_EQUAL_MSG(rg_object_id, info_buffer->object_id, "Object ID Unexpected\n");

        status = fbe_api_base_config_get_generation_number(rg_object_id, &generation_number);
        MUT_ASSERT_UINT64_EQUAL_MSG(generation_number, info_buffer->generation_number, "Gen Number Unexpected\n");

        status = fbe_api_base_config_get_width(rg_object_id, &width_info);
        MUT_ASSERT_UINT64_EQUAL_MSG(width_info.width, info_buffer->width, "Width Unexpected\n");

        info_buffer++;
        rg_config_ptr++;
    }

    fbe_api_free_memory(temp_buffer_ptr);
    return;
}
/*!****************************************************************************
 *  kalia_test_setup
 ******************************************************************************
 *
 * @brief
 *  This is the setup function for the Bubble Bass test. It is responsible
 *  for loading the physical and logical configuration for this test suite.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void kalia_dualsp_setup(void)
{
    fbe_status_t status;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Kalia Dual SP test ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");

     if (fbe_test_util_is_simulation())
    {
         /* Instantiate the drives on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        scoobert_physical_config(520);

        /* remove drive 0_1_0 to insert after encryption enabled */
        status = fbe_test_pp_util_remove_sas_drive(0,1,0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


        /*! Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        scoobert_physical_config(520);
       
        status = fbe_test_pp_util_remove_sas_drive(0,1,0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        sep_config_load_sep_and_neit_both_sps();
    }

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    
    return;

} /* End kalia_test_setup() */


/*!****************************************************************************
 *  kalia_test_cleanup
 ******************************************************************************
 *
 * @brief
 *  This is the cleanup function for the Bubble Bass test.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void kalia_dualsp_cleanup(void)
{  
    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for Kalia Dual SP test ===\n");

    if(fbe_test_util_is_simulation())
    {
        /* Turn off delay for I/O completion in the terminator */
        fbe_api_terminator_set_io_global_completion_delay(0);

        /* First execute teardown on SP B then on SP A
        */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_destroy_neit_sep_physical();

        /* First execute teardown on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_test_sep_util_destroy_neit_sep_physical();
    }

    return;

} /* End kalia_test_cleanup() */

/*!****************************************************************************
 *  kalia_test_setup
 ******************************************************************************
 *
 * @brief
 *  This is the setup function for the Bubble Bass test. It is responsible
 *  for loading the physical and logical configuration for this test suite.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void kalia_setup(void)
{
    fbe_status_t status;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Kalia test ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");

     if (fbe_test_util_is_simulation())
    {
         /* Instantiate the drives on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        scoobert_physical_config(520);

        /* remove drive 0_1_0 to insert after encryption enabled */
        status = fbe_test_pp_util_remove_sas_drive(0,1,0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        sep_config_load_sep_and_neit();
    }

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    
    return;

} /* End kalia_test_setup() */


/*!****************************************************************************
 *  kalia_test_cleanup
 ******************************************************************************
 *
 * @brief
 *  This is the cleanup function for the Bubble Bass test.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void kalia_cleanup(void)
{  
    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for Kalia test ===\n");

    if(fbe_test_util_is_simulation())
    {
        /* Turn off delay for I/O completion in the terminator */
        fbe_api_terminator_set_io_global_completion_delay(0);

        /* First execute teardown on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_test_sep_util_destroy_neit_sep_physical();
    }

    return;

} /* End kalia_test_cleanup() */



void kalia_test_validate_pvd_config_type(fbe_test_rg_configuration_t *rg_config_ptr)
{
    fbe_u32_t rg_count;
    fbe_u32_t index;
    fbe_object_id_t pvd_object_id;
    fbe_status_t status;
	fbe_u32_t position;
	fbe_api_provision_drive_info_t provision_drive_info_p;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Validate PVD configuration type ===\n");

    /* First get the number of RGs in the configuration */
    rg_count = fbe_test_get_rg_count(rg_config_ptr);

    /* Set the encryption state to be a locked state */
    for(index = 0; index < rg_count; index ++) {
		for(position = 0; position < rg_config_ptr[index].width; position++){
			status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_ptr[index].rg_disk_set[position].bus,
																	rg_config_ptr[index].rg_disk_set[position].enclosure,
																	rg_config_ptr[index].rg_disk_set[position].slot,
																	&pvd_object_id);
			MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

			status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info_p);
			MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

			MUT_ASSERT_INT_EQUAL(provision_drive_info_p.config_type, FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID);
			mut_printf(MUT_LOG_TEST_STATUS, "=== PVD %d_%d_%d configuration type RAID ===\n", 
																	rg_config_ptr[index].rg_disk_set[position].bus,
																	rg_config_ptr[index].rg_disk_set[position].enclosure,
																	rg_config_ptr[index].rg_disk_set[position].slot);

		}        
    }

    return;
}
/******************************************
 * end fbe_test_sep_util_validate_kek_setup()
 ******************************************/ 
/*!**************************************************************
 * fbe_test_sep_util_setup_kek()
 ****************************************************************
 * @brief
 *  Make sure the key encryption key was setup properly for a RG.
 *
 * @param rg_config_ptr - RG to validate for.               
 *
 * @return None.   
 *
 ****************************************************************/
void kalia_test_validate_kek_of_kek_handling(fbe_test_rg_configuration_t *rg_config_ptr)
{
    fbe_database_control_setup_kek_kek_t encryption_info;
    fbe_database_control_destroy_kek_kek_t unregister_info;
    fbe_database_control_reestablish_kek_kek_t reestablish_key_info;
    fbe_u8_t            keys[FBE_ENCRYPTION_KEY_SIZE];
    fbe_status_t        status;
    fbe_u32_t           port_idx;
    fbe_api_terminator_device_handle_t port_handle;
    fbe_u8_t            terminator_key[FBE_ENCRYPTION_KEY_SIZE];
    fbe_bool_t          key_match = 1;
    fbe_database_control_get_be_port_info_t be_port_info;
    fbe_u32_t sp_count = 0;
    fbe_key_handle_t    kek_kek_handle[3][FBE_TEST_KALIA_NUM_PORTS];
    fbe_sim_transport_connection_target_t  current_sp;
    fbe_sim_transport_connection_target_t  target_sp;
    fbe_u32_t get_total_sp_count;

    fbe_zero_memory(&encryption_info, sizeof(fbe_database_control_setup_kek_kek_t));
    mut_printf(MUT_LOG_TEST_STATUS, "=== Validate that KEK of KEKs are sent correctly via the topology  ===\n");

    fbe_zero_memory(&be_port_info, sizeof(fbe_database_control_get_be_port_info_t));
    be_port_info.sp_id = FBE_CMI_SP_ID_A;
    current_sp = fbe_api_sim_transport_get_target_server();
    target_sp = FBE_SIM_SP_A; 

    get_total_sp_count = (fbe_test_sep_util_get_dualsp_test_mode() == FBE_TRUE) ? 2 : 1;
    for(sp_count = 0; sp_count < get_total_sp_count; sp_count++) 
    {
        status = fbe_api_database_get_be_port_info(&be_port_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
        /* Validate that the key is sent over to each of the port correctly and validate
         * the keys valus are correct */
        for (port_idx = 0; port_idx < FBE_TEST_KALIA_NUM_PORTS; port_idx++) 
        {
            mut_printf(MUT_LOG_TEST_STATUS, "=== Setting up KEK KEK for port num:%d  ===\n", port_idx);

            encryption_info.sp_id = be_port_info.sp_id;
            encryption_info.port_object_id = be_port_info.port_info[port_idx].port_object_id;
            fbe_zero_memory(keys, FBE_ENCRYPTION_KEY_SIZE);
            fbe_zero_memory(terminator_key, FBE_ENCRYPTION_KEY_SIZE);
            _snprintf(&keys[0], FBE_ENCRYPTION_KEY_SIZE, "FBEFBEKEKKEK%d\n",port_idx, 0);
            fbe_copy_memory(&encryption_info.kek_kek, keys, FBE_ENCRYPTION_KEY_SIZE);
            encryption_info.key_size = FBE_ENCRYPTION_KEY_SIZE;

            /* We want to always target one SP to test the CMI path */
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

            /*  Send this key over to the topology to be sent to miniport */
            status = fbe_api_database_setup_kek_kek(&encryption_info);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            kek_kek_handle[be_port_info.sp_id][port_idx] = encryption_info.kek_kek_handle;

            fbe_api_sim_transport_set_target_server(target_sp); 
            status = fbe_api_terminator_get_port_handle(port_idx,
                                                        &port_handle);

            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* From the key handle get the key value and make sure they match */
            fbe_api_terminator_get_encryption_keys(port_handle, encryption_info.kek_kek_handle , terminator_key);
            key_match = fbe_equal_memory(&keys[0], terminator_key, FBE_ENCRYPTION_KEY_SIZE);

            MUT_ASSERT_TRUE_MSG((key_match == 1), "Key values does NOT match\n");
        }
        fbe_zero_memory(&be_port_info, sizeof(fbe_database_control_get_be_port_info_t));
        be_port_info.sp_id = FBE_CMI_SP_ID_B;
        target_sp = FBE_SIM_SP_B;
    }

    fbe_zero_memory(&be_port_info, sizeof(fbe_database_control_get_be_port_info_t));
    be_port_info.sp_id = FBE_CMI_SP_ID_A;
    for(sp_count = 0; sp_count < get_total_sp_count; sp_count++)
    {
        status = fbe_api_database_get_be_port_info(&be_port_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Validate that the key is sent over to each of the port correctly and validate
         * the keys valus are correct */
        for (port_idx = 0; port_idx < FBE_TEST_KALIA_NUM_PORTS; port_idx++) 
        {
            mut_printf(MUT_LOG_TEST_STATUS, "=== Reestablish KEK KEK for port num:%d  ===\n", port_idx);
            reestablish_key_info.port_object_id = be_port_info.port_info[port_idx].port_object_id;
            reestablish_key_info.sp_id = be_port_info.sp_id;
            reestablish_key_info.kek_kek_handle = kek_kek_handle[be_port_info.sp_id][port_idx];
            status = fbe_api_database_reestablish_kek_kek(&reestablish_key_info);
            MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Reestablish key handle failed\n");

            mut_printf(MUT_LOG_TEST_STATUS, "=== Destroy KEK KEK for port num:%d  ===\n", port_idx);

            unregister_info.port_object_id = be_port_info.port_info[port_idx].port_object_id;
            unregister_info.sp_id = be_port_info.sp_id;
            unregister_info.kek_kek_handle = kek_kek_handle[be_port_info.sp_id][port_idx];
            status = fbe_api_database_destroy_kek_kek(&unregister_info);
            MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Unregistration failed\n");
        }
        fbe_zero_memory(&be_port_info, sizeof(fbe_database_control_get_be_port_info_t));
        be_port_info.sp_id = FBE_CMI_SP_ID_B;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "=== Completed Validation of KEKs  ===\n");
    return;

}
/******************************************
 * end fbe_test_sep_util_validate_kek_setup()
 ******************************************/ 

/*!**************************************************************
 * kalia_test_validate_port_info()
 ****************************************************************
 * @brief
 *  Make sure the key encryption key was setup properly for a RG.
 *
 * @param rg_config_ptr - RG to validate for.               
 *
 * @return None.   
 *
 ****************************************************************/
static void kalia_test_validate_port_info(fbe_test_rg_configuration_t *rg_config_ptr)
{
    fbe_status_t        status;
    fbe_u32_t           port_idx;
    fbe_database_control_get_be_port_info_t be_port_info;
    fbe_u32_t count = 0;
    fbe_u32_t get_total_sp_count;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Test to validate BE Port information  ===\n");

    fbe_zero_memory(&be_port_info, sizeof(fbe_database_control_get_be_port_info_t));
    be_port_info.sp_id = FBE_CMI_SP_ID_A;
    get_total_sp_count = (fbe_test_sep_util_get_dualsp_test_mode() == FBE_TRUE) ? 2 : 1;
    do
    {
        status = fbe_api_database_get_be_port_info(&be_port_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
        MUT_ASSERT_INT_EQUAL_MSG(FBE_TEST_KALIA_NUM_PORTS, be_port_info.num_be_ports, "Number of BE ports unexpected\n");
    
        /* Validate that the key is sent over to each of the port correctly and validate
         * the keys valus are correct */
        for (port_idx = 0; port_idx < FBE_TEST_KALIA_NUM_PORTS; port_idx++) 
        {
            MUT_ASSERT_INT_EQUAL_MSG(port_idx, be_port_info.port_info[port_idx].be_number, "Incorrect BE Number\n");
        }
        count++;
        fbe_zero_memory(&be_port_info, sizeof(fbe_database_control_get_be_port_info_t));
        be_port_info.sp_id = FBE_CMI_SP_ID_B;
    }while (count < get_total_sp_count);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Test to validate BE Port information ---- Done  ===\n");
    return;

}
/******************************************
 * end kalia_test_validate_port_info()
 ******************************************/ 

