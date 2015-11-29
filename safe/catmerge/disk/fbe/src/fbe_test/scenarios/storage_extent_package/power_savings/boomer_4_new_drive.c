/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file boomer_4_new_drive_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the test for power saving enable/disbale and clone from 
 *  Boomer for new drive testing. 
 *
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "sep_tests.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "pp_utils.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "sep_utils.h"
#include "fbe/fbe_api_power_saving_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_protocol_error_injection_service.h"
#include "neit_utils.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe_test_common_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*************************
 *   TEST DESCRIPTION
 *************************/
char * boomer_4_new_drive_short_desc = "Power saving capabilities(single SP)";
char * boomer_4_new_drive_long_desc =
"The Power Saving Scenario (boomer_4_new_drive) tests the ability of the system to enable or disable power saving and other PS related functions\n"
"It covers the following cases: \n"
"- The system wide power saving is enabled, and disbaled and verification make sure all objects are hibernating and then getting out\n"
"- Object based power saving is enabled, and disbaled and verification make sure all objects are\n"
"- Verify backgound IO does not count as IO towards power saving\n"
"- Verify unbound PDOs alos save power\n"
"- Verify hibernating objects will wake up periodically\n"
"- Verify power saving statistics is accurate\n"
"Starting Config: \n"
"        [PP] armada board \n"
"        [PP] SAS PMC port \n"
"        [PP] viper enclosure \n"
"        [PP] two SAS drives (PDO-0 and PDO-1)) \n"
"        [PP] two logical drives (LD-0 and LD-1) \n"
"        [SEP] two provision drives (PVD-0 and PVD-1) \n"
"        [SEP] two virtual drives (VD-0 and VD-1) \n"
"        [SEP] raid object (mirror) \n"
"        [SEP] two LUN objects (LUN-0 and LUN-1) \n"
"        Last update : 10/21/2011\n"
;

static fbe_object_id_t          expected_object_id = FBE_OBJECT_ID_INVALID;
static fbe_lifecycle_state_t    expected_state = FBE_LIFECYCLE_STATE_INVALID;
static fbe_api_rdgen_context_t 	boomer_4_new_drive_test_rdgen_contexts[5];
static fbe_semaphore_t          sem;
static fbe_notification_registration_id_t          reg_id;

static void boomer_4_new_drive_test_create_lun(fbe_test_rg_configuration_t *rg_config_p, fbe_lun_number_t lun_number);
static void boomer_4_new_drive_test_destroy_lun(fbe_lun_number_t lun_number);
static void boomer_4_new_drive_commmand_response_callback_function (update_object_msg_t * update_object_msg, void *context);
static void boomer_4_new_drive_test_write_background_pattern(fbe_element_size_t element_size, fbe_object_id_t lun_object_id);

static void boomer_4_new_drive_test_setup_rdgen_test_context(  fbe_api_rdgen_context_t *context_p,
                                                        fbe_object_id_t object_id,
                                                        fbe_class_id_t class_id,
                                                        fbe_rdgen_operation_t rdgen_operation,
                                                        fbe_lba_t capacity,
                                                        fbe_u32_t max_passes,
                                                        fbe_u32_t threads,
                                                        fbe_u32_t io_size_blocks);

static fbe_status_t boomer_4_new_drive_test_run_io_load(fbe_object_id_t lun_object_id);

static void boomer_4_new_drive_test_setup_fill_rdgen_test_context(fbe_api_rdgen_context_t *context_p,
                                                    fbe_object_id_t object_id,
                                                    fbe_class_id_t class_id,
                                                    fbe_rdgen_operation_t rdgen_operation,
                                                    fbe_lba_t max_lba,
                                                    fbe_u32_t io_size_blocks);

static void boomer_4_new_drive_test_verify_system_enable_disable(void);
static void boomer_4_new_drive_test_verify_object_enable_disable(fbe_test_rg_configuration_t *rg_config_p);
static void boomer_4_new_drive_test_verify_background_io_not_count(fbe_test_rg_configuration_t *rg_config_p);
static void boomer_4_new_drive_test_verify_power_saving_policy(fbe_test_rg_configuration_t *rg_config_p);
static void boomer_4_new_drive_test_verify_bound_objects_save_power(fbe_test_rg_configuration_t *rg_config_p);
static void boomer_4_new_drive_test_verify_unbound_objects_save_power(fbe_test_rg_configuration_t *rg_config_p);
static void boomer_4_new_drive_test_verify_objects_get_out_of_power_save(fbe_test_rg_configuration_t *rg_config_p);
static void boomer_4_new_drive_test_verify_periodic_wakeup(fbe_test_rg_configuration_t *rg_config_p);
static void boomer_4_new_drive_test_verify_io_wakeup(fbe_test_rg_configuration_t *rg_config_p);
static void boomer_4_new_drive_test_verify_power_save_stat(fbe_test_rg_configuration_t *rg_config_p);
static void boomer_4_new_drive_test_inject_not_spinning_error(fbe_test_rg_configuration_t *rg_config_p);
static void boomer_4_new_drive_test_load_physical_config(void);
static void boomer_4_new_drive_test_create_rg(fbe_test_rg_configuration_t *rg_config_p);
static void boomer_4_new_drive_test_verify_lun_creation_on_a_hibernating_rg(fbe_test_rg_configuration_t *rg_config_p);

/**********************************************************************************/
#define BOOMER_4_NEW_DRIVE_TEST_LUNS_PER_RAID_GROUP         2
#define BOOMER_4_NEW_DRIVE_TEST_CHUNKS_PER_LUN              1
#define BOOMER_4_NEW_DRIVE_TEST_ENCL_MAX                    2
#define BOOMER_4_NEW_DRIVE_TEST_NUMBER_OF_PHYSICAL_OBJECTS  34 /* (1 board + 1 port + 2 encl + 30 pdo) */
#define BOOMER_4_NEW_DRIVE_JOB_SERVICE_CREATION_TIMEOUT     150000 /* in ms*/

static fbe_test_rg_configuration_t boomer_4_new_drive_rg_config[] = 
{
 /* width, capacity,        raid type,                        class,        block size, RAID-id,    bandwidth.*/
    {3,    0x50000, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY, 520,        0,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!**************************************************************
 * boomer_4_new_drive_reboot_sp()
 ****************************************************************
 * @brief
 *  reboot SP
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/
static void boomer_4_new_drive_reboot_sp(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    /*reboot sep*/
    fbe_test_sep_util_destroy_neit_sep();
    sep_config_load_sep_and_neit();
    status = fbe_test_sep_util_wait_for_database_service(BOOMER_4_NEW_DRIVE_JOB_SERVICE_CREATION_TIMEOUT);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;
} /* end of boomer_4_new_drive_reboot_sp*/

/*!**************************************************************
 * boomer_4_new_drive_test_load_physical_config()
 ****************************************************************
 * @brief
 *  Configure the Boomer For New Drive Test physical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/
static void boomer_4_new_drive_test_load_physical_config(void)
{

    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                               total_objects = 0;
    fbe_class_id_t                          class_id;
    fbe_api_terminator_device_handle_t      port0_handle = 0;
    fbe_api_terminator_device_handle_t      port0_encl_handle[BOOMER_4_NEW_DRIVE_TEST_ENCL_MAX] = {0};
    fbe_api_terminator_device_handle_t      drive_handle = 0;
    fbe_u32_t                               slot = 0;
    fbe_u32_t                               enclosure = 0;
    fbe_sas_address_t                       new_sas_address;


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
    for ( enclosure = 0; enclosure < BOOMER_4_NEW_DRIVE_TEST_ENCL_MAX; enclosure++)
    { 
        fbe_test_pp_util_insert_viper_enclosure(0,
                                                enclosure,
                                                port0_handle,
                                                &port0_encl_handle[enclosure]);
    }

    /* Insert drives for enclosures.
     */
    for ( enclosure = 0; enclosure < BOOMER_4_NEW_DRIVE_TEST_ENCL_MAX; enclosure++)
    {
        for ( slot = 0; slot < FBE_TEST_DRIVES_PER_ENCL; slot++)
        {
            if (slot >= 0 && slot <= 3) {
                fbe_test_pp_util_insert_sas_drive(0, enclosure, slot, 520, 
                                                  TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY, 
                                                  &drive_handle);
            }else{
                new_sas_address = fbe_test_pp_util_get_unique_sas_address();
                fbe_test_pp_util_insert_sas_drive_extend(0, enclosure, slot, 520, 
                                                         (TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY + TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY),
                                                         new_sas_address, FBE_SAS_DRIVE_COBRA_E_10K, &drive_handle);

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
    status = fbe_api_wait_for_number_of_objects(BOOMER_4_NEW_DRIVE_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 20000, FBE_PACKAGE_ID_PHYSICAL);
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
    MUT_ASSERT_TRUE(total_objects == BOOMER_4_NEW_DRIVE_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");

    return;

} /* End boomer_4_new_drive_test_load_physical_config() */



/*!**************************************************************
 * boomer_4_new_drive_test_create_lun()
 ****************************************************************
 * @brief
 *  Create LUNs.
 *
 * @param rg_config_p - RG Configuration
 * @param lun_number - LUN
 *
 * @return None.
 *
 ****************************************************************/
static void boomer_4_new_drive_test_create_lun(fbe_test_rg_configuration_t *rg_config_p, fbe_lun_number_t lun_number)
{
    fbe_status_t            status;
    fbe_api_lun_create_t    fbe_lun_create_req;
    fbe_object_id_t         lu_id;
    fbe_job_service_error_type_t job_error_type;
   
    /* Create a LUN */
    fbe_zero_memory(&fbe_lun_create_req, sizeof(fbe_api_lun_create_t));
    fbe_lun_create_req.raid_type = rg_config_p->raid_type;
    fbe_lun_create_req.raid_group_id = rg_config_p->raid_group_id; /*sep_standard_logical_config_one_r5 is 5 so this is what we use*/
    fbe_lun_create_req.lun_number = lun_number;
    fbe_lun_create_req.capacity = 0x100;
    fbe_lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
    fbe_lun_create_req.ndb_b = FBE_FALSE;
    fbe_lun_create_req.noinitialverify_b = FBE_FALSE;
    fbe_lun_create_req.addroffset = FBE_LBA_INVALID; /* set to a valid offset for NDB */

    status = fbe_api_create_lun(&fbe_lun_create_req, FBE_TRUE, 100000, &lu_id, &job_error_type);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);	

    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X created successfully! \n", __FUNCTION__, lu_id);

    return;

} /* end of boomer_4_new_drive_test_create_lun() */

/*!**************************************************************
 * boomer_4_new_drive_test_destroy_lun()
 ****************************************************************
 * @brief
 *  Destroy LUNs.
 *
 * @param lun_number - LUN
 * 
 * @return None.
 *
 ****************************************************************/
static void boomer_4_new_drive_test_destroy_lun(fbe_lun_number_t lun_number)
{
    fbe_status_t                status;
    fbe_api_lun_destroy_t       fbe_lun_destroy_req;
    fbe_job_service_error_type_t job_error_type;
    
    /* Destroy a LUN */
    fbe_lun_destroy_req.lun_number = lun_number;

    status = fbe_api_destroy_lun(&fbe_lun_destroy_req, FBE_TRUE, 100000, &job_error_type);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    mut_printf(MUT_LOG_TEST_STATUS, " LUN destroyed successfully! \n");

    return;
} /* end of boomer_4_new_drive_test_destroy_lun() */


/*!**************************************************************
 * boomer_4_new_drive_commmand_response_callback_function()
 ****************************************************************
 * @brief
 *  command response call back.
 *
 * @param update_object_msg - update message
 * @param context - context of the message
 * 
 * @return None.
 *
 ****************************************************************/
static void boomer_4_new_drive_commmand_response_callback_function (update_object_msg_t * update_object_msg, void *context)
{
    fbe_semaphore_t *sem_p = (fbe_semaphore_t *)context;
    fbe_lifecycle_state_t	state;

    fbe_notification_convert_notification_type_to_state(update_object_msg->notification_info.notification_type, &state);
    
    if (update_object_msg->object_id == expected_object_id && expected_state == state) {
        fbe_semaphore_release(sem_p, 0, 1 ,FALSE);
    }
} /* end of boomer_4_new_drive_commmand_response_callback_function() */

/*!**************************************************************
 * boomer_4_new_drive_test_run_io_load()
 ****************************************************************
 * @brief
 *  run IO load
 *
 * @param lun_object_id - LUN Object ID
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
static fbe_status_t boomer_4_new_drive_test_run_io_load(fbe_object_id_t lun_object_id)
{
    fbe_api_rdgen_context_t*    context_p = &boomer_4_new_drive_test_rdgen_contexts[0];
    fbe_status_t                status;


    /* Write a background pattern; necessary until LUN zeroing is fully supported */
    boomer_4_new_drive_test_write_background_pattern(128, lun_object_id);

    /* Set up for issuing reads forever
     */
    boomer_4_new_drive_test_setup_rdgen_test_context(context_p,
                                         lun_object_id,
                                         FBE_CLASS_ID_INVALID,
                                         FBE_RDGEN_OPERATION_READ_ONLY,
                                         FBE_LBA_INVALID,    /* use capacity */
                                         0,      /* run forever*/ 
                                         3,      /* threads per lun */
                                         1024);

    /* Run our I/O test
     */
    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_thread_delay(5000);/*let IO run for a while*/

    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    return FBE_STATUS_OK;

} /* end of boomer_4_new_drive_test_run_io_load() */


/*!**************************************************************
 * boomer_4_new_drive_test_setup_rdgen_test_context()
 ****************************************************************
 * @brief
 *  set rdgen test context
 *
 * @param context_p - context
 * @param object_id - object ID
 * @param class_id - class ID
 * @param rdgen_operation - rdgen operation
 * @param capacity - capacity
 * @param max_passes - max passes
 * @param threads - threads
 * @param io_size_blocks - IO block size
 * 
 * @return none
 *
 ****************************************************************/
static void boomer_4_new_drive_test_setup_rdgen_test_context(  fbe_api_rdgen_context_t *context_p,
                                                        fbe_object_id_t object_id,
                                                        fbe_class_id_t class_id,
                                                        fbe_rdgen_operation_t rdgen_operation,
                                                        fbe_lba_t capacity,
                                                        fbe_u32_t max_passes,
                                                        fbe_u32_t threads,
                                                        fbe_u32_t io_size_blocks)
{
    fbe_status_t status;

    status = fbe_api_rdgen_test_context_init(   context_p, 
                                                object_id, 
                                                class_id,
                                                FBE_PACKAGE_ID_SEP_0,
                                                rdgen_operation,
                                                FBE_RDGEN_PATTERN_LBA_PASS,
                                                max_passes,
                                                0, /* io count not used */
                                                0, /* time not used*/
                                                threads,
                                                FBE_RDGEN_LBA_SPEC_RANDOM,
                                                0,    /* Start lba*/
                                                0,    /* Min lba */
                                                capacity, /* max lba */
                                                FBE_RDGEN_BLOCK_SPEC_RANDOM,
                                                1,    /* Min blocks */
                                                io_size_blocks  /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;

} /* end of boomer_4_new_drive_test_setup_rdgen_test_context() */


/*!**************************************************************
 * boomer_4_new_drive_test_write_background_pattern()
 ****************************************************************
 * @brief
 *  Write background pattern
 *
 * @param element_size - element size
 * @param lun_object_id - LUN object ID
 * 
 * @return none
 *
 ****************************************************************/
static void boomer_4_new_drive_test_write_background_pattern(fbe_element_size_t element_size, fbe_object_id_t lun_object_id)
{
    fbe_api_rdgen_context_t *context_p = &boomer_4_new_drive_test_rdgen_contexts[0];
    fbe_status_t status;

    /* Write a background pattern to the LUN
     */
    boomer_4_new_drive_test_setup_fill_rdgen_test_context( context_p,
                                                    lun_object_id,
                                                    FBE_CLASS_ID_INVALID,
                                                    FBE_RDGEN_OPERATION_WRITE_ONLY,
                                                    FBE_LBA_INVALID, /* use max capacity */
                                                    128);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    
    /* Make sure there were no errors
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    /* Read back the pattern and check it was written OK
     */
    boomer_4_new_drive_test_setup_fill_rdgen_test_context( context_p,
                                                    lun_object_id,
                                                    FBE_CLASS_ID_INVALID,
                                                    FBE_RDGEN_OPERATION_READ_CHECK,
                                                    FBE_LBA_INVALID, /* use max capacity */
                                                    128);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    return;

} /* end boomer_4_new_drive_test_write_background_pattern() */

/*!**************************************************************
 * boomer_4_new_drive_test_setup_fill_rdgen_test_context()
 ****************************************************************
 * @brief
 *  set-up rdgen test
 *
 * @param context_p - context
 * @param object_id - object id
 * @param class_id - class id
 * @param rdgen_operation - rdgen operation
 * @param max_lba - max lba
 * @param io_size_blocks - IO block size
 * 
 * @return none
 *
 ****************************************************************/
static void boomer_4_new_drive_test_setup_fill_rdgen_test_context(fbe_api_rdgen_context_t *context_p,
                                                    fbe_object_id_t object_id,
                                                    fbe_class_id_t class_id,
                                                    fbe_rdgen_operation_t rdgen_operation,
                                                    fbe_lba_t max_lba,
                                                    fbe_u32_t io_size_blocks)
{
    fbe_status_t status;

    status = fbe_api_rdgen_test_context_init(   context_p, 
                                                object_id,
                                                class_id, 
                                                FBE_PACKAGE_ID_SEP_0,
                                                rdgen_operation,
                                                FBE_RDGEN_PATTERN_LBA_PASS,
                                                1,    /* We do one full sequential pass. */
                                                0,    /* num ios not used */
                                                0,    /* time not used*/
                                                1,    /* threads */
                                                FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                                0,    /* Start lba*/
                                                0,    /* Min lba */
                                                max_lba,    /* max lba */
                                                FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE,
                                                16,    /* Number of stripes to write. */
                                                io_size_blocks    /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;

} /* end of boomer_4_new_drive_test_setup_fill_rdgen_test_context() */

/*!**************************************************************
 * boomer_4_new_drive_test_verify_system_enable_disable()
 ****************************************************************
 * @brief
 *   Verify that sysem power saving can be enable/disable 
 *
 * @param none
 * 
 * @return none
 *
 ****************************************************************/
static void boomer_4_new_drive_test_verify_system_enable_disable(void)
{
    fbe_system_power_saving_info_t              power_save_info;
    fbe_status_t                                status;
    
    power_save_info.enabled = FBE_TRUE;

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Enabling system wide power saving ===\n", __FUNCTION__);

    status  = fbe_api_enable_system_power_save();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_enable_system_power_save_statistics();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*make sure it worked*/
    power_save_info.enabled = FBE_FALSE;
    power_save_info.stats_enabled = FBE_FALSE;
    status  = fbe_api_get_system_power_save_info(&power_save_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(power_save_info.enabled == FBE_TRUE);
    MUT_ASSERT_TRUE(power_save_info.stats_enabled == FBE_TRUE); 

    return;
    
} /* end of boomer_4_new_drive_test_verify_system_enable_disable() */

/*!**************************************************************
 * boomer_4_new_drive_test_verify_object_enable_disable()
 ****************************************************************
 * @brief
 *   Verify power saving enable/disable on the RG
 *
 * @param rg_config_p - RG Config
 * 
 * @return none
 *
 ****************************************************************/
static void boomer_4_new_drive_test_verify_object_enable_disable(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_object_id_t                             lu_object_id, rg_object_id;
    fbe_base_config_object_power_save_info_t    object_ps_info;
    fbe_status_t                                status;
    fbe_raid_group_get_power_saving_info_t      rg_ps_info;

    /*and enable the power saving on the lun*/
    mut_printf(MUT_LOG_TEST_STATUS, "=== Enabling / Disabling power saving on the RAID ===\n");
    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number, &lu_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*make sure it's not saving power */
    status = fbe_api_get_object_power_save_info(lu_object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(object_ps_info.power_save_state, FBE_POWER_SAVE_STATE_NOT_SAVING_POWER);

    status = fbe_api_get_object_power_save_info(rg_object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(object_ps_info.power_save_state, FBE_POWER_SAVE_STATE_NOT_SAVING_POWER);

    status = fbe_api_enable_raid_group_power_save(rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*let's wait for a while since RG propagate it to the LU*/
    fbe_api_sleep(14000);

    /*check it's enabled*/
    status = fbe_api_get_object_power_save_info(lu_object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(object_ps_info.power_saving_enabled, FBE_TRUE);

    status = fbe_api_get_object_power_save_info(rg_object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(object_ps_info.power_saving_enabled, FBE_TRUE);

    /*try to disable the LU (should be invalid)*/
    status = fbe_api_disable_object_power_save(lu_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_GENERIC_FAILURE, status);

    status = fbe_api_disable_raid_group_power_save(rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*let's wait for a while since RG propagate it to the LU*/
    fbe_api_sleep(14000);

    /*check it's disbaled*/
    status = fbe_api_get_object_power_save_info(lu_object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(object_ps_info.power_saving_enabled, FBE_FALSE);
    status = fbe_api_get_object_power_save_info(rg_object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(object_ps_info.power_saving_enabled, FBE_FALSE);

    /*since we did not run any IO recently on the LU, we also expect the elapsed time since last IO to be more than 0*/
    MUT_ASSERT_TRUE(object_ps_info.seconds_since_last_io > 0);

    /* Get the RG power saving info */ 
    status  = fbe_api_raid_group_get_power_saving_policy(rg_object_id, & rg_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(rg_ps_info.power_saving_enabled, FBE_FALSE);
    MUT_ASSERT_UINT64_EQUAL(rg_ps_info. max_raid_latency_time_is_sec, 120); 

    return;

} /* end of boomer_4_new_drive_test_verify_object_enable_disable() */

/*!**************************************************************
 * boomer_4_new_drive_test_verify_background_io_not_count()
 ****************************************************************
 * @brief
 *   Verify that sniff does not count as IO.
 *
 * @param rg_config_p - RG Config
 * 
 * @return none
 *
 ****************************************************************/
static void boomer_4_new_drive_test_verify_background_io_not_count(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_object_id_t                             object_id;
    fbe_base_config_object_power_save_info_t    object_ps_info;
    fbe_status_t                                status;
    fbe_u32_t                                   object_seconds_since_last_io = 0;

    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[1].bus,
                                                            rg_config_p->rg_disk_set[1].enclosure,
                                                            rg_config_p->rg_disk_set[1].slot,
                                                            &object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Disable background zeroing */
    status = fbe_test_sep_util_provision_drive_disable_zeroing( object_id );
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Make sure Sniff does not count as IO ===\n");
    /*start a verify*/
    /*before the verify we make sure that we take a snapshot of the time since the last IO*/
    status = fbe_api_get_object_power_save_info(object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    object_seconds_since_last_io = object_ps_info.seconds_since_last_io;

    status = fbe_api_provision_drive_set_verify_checkpoint(object_id, FBE_PACKET_FLAG_NO_ATTRIB, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_sep_util_provision_drive_enable_verify( object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_api_sleep(12000);/*let's verify for a while*/

    fbe_test_sep_util_provision_drive_disable_verify( object_id);

    status = fbe_api_get_object_power_save_info(object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(object_ps_info.power_saving_enabled, FBE_TRUE);/*PVD's should have their power saving enabled by default*/

    /*since verify does not need to count as IO, we want to make sure we did not zero the seconds_since_last_io of the object
    if we did, it means sniff verify was counted as IO and this is not good*/
    MUT_ASSERT_TRUE(object_seconds_since_last_io < object_ps_info.seconds_since_last_io);
    
    return;

} /* end of boomer_4_new_drive_test_verify_background_io_not_count() */

/*!**************************************************************
 * boomer_4_new_drive_test_verify_power_saving_policy()
 ****************************************************************
 * @brief
 *   Verify power saving policy.
 *
 * @param rg_config_p - RG Config
 * 
 * @return none
 *
 ****************************************************************/
static void boomer_4_new_drive_test_verify_power_saving_policy(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_lun_set_power_saving_policy_t           lun_ps_policy;
    fbe_object_id_t                             lu_object_id;
    fbe_status_t                                status;
    fbe_u32_t                                   lu_index;
    
    mut_printf(MUT_LOG_TEST_STATUS, "=== setting power saving policy ===\n");

    for (lu_index = 0; lu_index < BOOMER_4_NEW_DRIVE_TEST_LUNS_PER_RAID_GROUP; lu_index++) 
    {

        status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list[lu_index].lun_number, &lu_object_id);

        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        lun_ps_policy.lun_delay_before_hibernate_in_sec = 5 ;/*this is for 5 seconds delay before hibernation*/
        lun_ps_policy.max_latency_allowed_in_100ns = (fbe_time_t)((fbe_time_t)60000 * (fbe_time_t)10000000);/*we allow 1000 minutes for the drives to wake up (default for drives is 100 minutes)*/

        status = fbe_api_lun_set_power_saving_policy(lu_object_id, &lun_ps_policy);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        lun_ps_policy.lun_delay_before_hibernate_in_sec = 0;
        lun_ps_policy.max_latency_allowed_in_100ns = 0;

        mut_printf(MUT_LOG_TEST_STATUS, "=== getting power saving policy ===\n");

        status = fbe_api_lun_get_power_saving_policy(lu_object_id, &lun_ps_policy);
        MUT_ASSERT_TRUE(FBE_STATUS_OK == status);
        MUT_ASSERT_TRUE(lun_ps_policy.lun_delay_before_hibernate_in_sec == 5);
        MUT_ASSERT_TRUE(lun_ps_policy.max_latency_allowed_in_100ns == (fbe_time_t)((fbe_time_t)60000 * (fbe_time_t)10000000));
    }

    return;

} /* end of boomer_4_new_drive_test_verify_power_saving_policy() */

/*!**************************************************************
 * boomer_4_new_drive_test_verify_bound_objects_save_power()
 ****************************************************************
 * @brief
 *   Verify bound object going into power saving.
 *
 * @param rg_config_p - RG Config
 * 
 * @return none
 *
 ****************************************************************/
static void boomer_4_new_drive_test_verify_bound_objects_save_power(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_object_id_t                             object_id, rg_object_id, lu_object_id, pdo_object_id;
    fbe_base_config_object_power_save_info_t    object_ps_info;
    fbe_lifecycle_state_t                       object_state;
    fbe_u32_t                                   disk_index = 0;
    DWORD                                       dwWaitResult;
    fbe_status_t                                status;
    fbe_u32_t                                   lu_index;
    
    /* Disable background zeroing */
    status = fbe_test_sep_util_rg_config_disable_zeroing( rg_config_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "=== changing power saving time of all bound objects to 20 seconds ===\n");

    /*BUBBLE_BASS_TEST_RAID_GROUP_ID is 0 so this is what we use*/
    status = fbe_api_database_lookup_raid_group_by_number(0, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_set_raid_group_power_save_idle_time(rg_object_id, 20);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    for (disk_index = 0; disk_index < rg_config_p->width; disk_index++) {
        status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[disk_index].bus,
                                                                rg_config_p->rg_disk_set[disk_index].enclosure,
                                                                rg_config_p->rg_disk_set[disk_index].slot,
                                                                &object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_set_object_power_save_idle_time(object_id, 20);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, disk_index, &object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_set_object_power_save_idle_time(object_id, 20);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "===Enabling power saving on RG(should propagate to all objects) ===\n");

    status = fbe_api_get_physical_drive_object_id_by_location(rg_config_p->rg_disk_set[0].bus,
                                                              rg_config_p->rg_disk_set[0].enclosure,
                                                              rg_config_p->rg_disk_set[0].slot,
                                                              &pdo_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    expected_object_id = pdo_object_id;/*we want to get notification from this RAID*/
    expected_state = FBE_LIFECYCLE_STATE_HIBERNATE;

    status = fbe_api_enable_raid_group_power_save(rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*now let's wait and make sure the LU and other objects connected to it are hibernating*/
    mut_printf(MUT_LOG_TEST_STATUS, "=== waiting up to 300 sec. for system to power save ===\n");

    dwWaitResult = fbe_semaphore_wait_ms(&sem, 480000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: Verified PDO is saving power \n", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Making sure all bound objects saving power ===\n");
    for (lu_index = 0; lu_index < BOOMER_4_NEW_DRIVE_TEST_LUNS_PER_RAID_GROUP; lu_index++){
    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list[lu_index].lun_number, &lu_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    status = fbe_api_get_object_lifecycle_state(lu_object_id, &object_state, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_LIFECYCLE_STATE_HIBERNATE, object_state);

    /*make sure LU is saving power */
    status = fbe_api_get_object_power_save_info(lu_object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(object_ps_info.power_save_state, FBE_POWER_SAVE_STATE_NOT_SAVING_POWER);
    MUT_ASSERT_INT_NOT_EQUAL(object_ps_info.power_save_state, FBE_POWER_SAVE_STATE_EXITING_POWER_SAVE);
}
    /*make sure RAID saving power */
    status = fbe_api_get_object_lifecycle_state(rg_object_id, &object_state, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_LIFECYCLE_STATE_HIBERNATE, object_state);

    status = fbe_api_get_object_power_save_info(rg_object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(object_ps_info.power_save_state, FBE_POWER_SAVE_STATE_NOT_SAVING_POWER);
    MUT_ASSERT_INT_NOT_EQUAL(object_ps_info.power_save_state, FBE_POWER_SAVE_STATE_EXITING_POWER_SAVE);
    
    /*make sure VD saving power */
    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, 0, &object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_get_object_lifecycle_state(object_id, &object_state, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_LIFECYCLE_STATE_HIBERNATE, object_state);
    status = fbe_api_get_object_power_save_info(object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(object_ps_info.power_save_state, FBE_POWER_SAVE_STATE_NOT_SAVING_POWER);
    MUT_ASSERT_INT_NOT_EQUAL(object_ps_info.power_save_state, FBE_POWER_SAVE_STATE_EXITING_POWER_SAVE);

    /*make sure PVD saving power */
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[0].bus,
                                                            rg_config_p->rg_disk_set[0].enclosure,
                                                            rg_config_p->rg_disk_set[0].slot,
                                                            &object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_get_object_lifecycle_state(object_id, &object_state, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_LIFECYCLE_STATE_HIBERNATE, object_state);
    status = fbe_api_get_object_power_save_info(object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(object_ps_info.power_save_state, FBE_POWER_SAVE_STATE_NOT_SAVING_POWER);
    MUT_ASSERT_INT_NOT_EQUAL(object_ps_info.power_save_state, FBE_POWER_SAVE_STATE_EXITING_POWER_SAVE);

    return;

} /* end of boomer_4_new_drive_test_verify_bound_objects_save_power() */


/*!**************************************************************
 * boomer_4_new_drive_test_verify_unbound_objects_save_power()
 ****************************************************************
 * @brief
 *   Verify unbound object going into power saving.
 *
 * @param rg_config_p - RG Config
 * 
 * @return none
 *
 ****************************************************************/
static void boomer_4_new_drive_test_verify_unbound_objects_save_power(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_object_id_t     object_id;
    DWORD               dwWaitResult;
    fbe_status_t        status;
    

    mut_printf(MUT_LOG_TEST_STATUS, "=== waiting for unbound PVD's to be in hibernate in ~2.5 minute ===\n");
    
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->unused_disk_set[0].bus,
                                                            rg_config_p->unused_disk_set[0].enclosure,
                                                            rg_config_p->unused_disk_set[0].slot,
                                                            &object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_test_sep_util_provision_drive_disable_zeroing( object_id );
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    expected_object_id = object_id;/*we want to get notification from this drive*/
    expected_state = FBE_LIFECYCLE_STATE_HIBERNATE;

    /*wait about 1.5 minutes more from the timerwe enabled hibernation for all
    so unbound PVDs will get to hibernation in 2 minutes*/
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 300000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    mut_printf(MUT_LOG_TEST_STATUS, " === Verified unbound PVD is hibernating in 2 minutes ===\n");

    return;

} /* end of boomer_4_new_drive_test_verify_unbound_objects_save_power() */

/*!**************************************************************
 * boomer_4_new_drive_test_verify_objects_get_out_of_power_save()
 ****************************************************************
 * @brief
 *  Verify objects get out of power saving.
 *
 * @param rg_config_p - RG config
 * 
 * @return none
 *
 ****************************************************************/
static void boomer_4_new_drive_test_verify_objects_get_out_of_power_save(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_object_id_t                             object_id, lu_object_id;
    fbe_status_t                                status;
    fbe_base_config_object_power_save_info_t    object_ps_info;
    fbe_system_power_saving_info_t              power_save_info;
    
    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number, &lu_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    mut_printf(MUT_LOG_TEST_STATUS, " === Disabling system wide power saving ===\n");

    status  = fbe_api_disable_system_power_save();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    
    status  = fbe_api_get_system_power_save_info(&power_save_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(power_save_info.enabled == FBE_FALSE);

    mut_printf(MUT_LOG_TEST_STATUS, " === Waiting for LU to become ready ===\n");

    /*wait and verify LU is back to ready*/
    status = fbe_api_wait_for_object_lifecycle_state(lu_object_id, FBE_LIFECYCLE_STATE_READY, 150000, FBE_PACKAGE_ID_SEP_0);
    
    mut_printf(MUT_LOG_TEST_STATUS, " ===Verified LU is back to READY ===\n");

    status = fbe_api_get_object_power_save_info(lu_object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(object_ps_info.power_save_state, FBE_POWER_SAVE_STATE_NOT_SAVING_POWER);

    mut_printf(MUT_LOG_TEST_STATUS, "=== verify unbound PVD is out of hibernation ===\n");
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->unused_disk_set[0].bus,
                                                            rg_config_p->unused_disk_set[0].enclosure,
                                                            rg_config_p->unused_disk_set[0].slot,
                                                            &object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_wait_for_object_lifecycle_state(object_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_SEP_0);
     MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_get_object_power_save_info(object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(object_ps_info.power_save_state, FBE_POWER_SAVE_STATE_NOT_SAVING_POWER);
    
    return;

} /* end of boomer_4_new_drive_test_verify_objects_get_out_of_power_save() */

/*!**************************************************************
 * boomer_4_new_drive_test_verify_periodic_wakeup()
 ****************************************************************
 * @brief
 *  Verify that RG wakes up periodicly
 *
 * @param rg_config_p - RG Config
 * 
 * @return none
 *
 ****************************************************************/
static void boomer_4_new_drive_test_verify_periodic_wakeup(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_object_id_t         rg_object_id, lu_object_id;
    DWORD                   dwWaitResult;
    fbe_status_t            status;

    
    /*test ability to wake up after a while*/
    mut_printf(MUT_LOG_TEST_STATUS, " ===Testing periodic wakeup (will take 2-3 minutes) ===\n");
    status  = fbe_api_enable_system_power_save();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number, &lu_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_enable_raid_group_power_save(rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_HIBERNATE, 60000, FBE_PACKAGE_ID_SEP_0);
     MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " === Verified RG is back to HIBERNATE === \n");

    status = fbe_api_set_power_save_periodic_wakeup(1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    expected_object_id = rg_object_id;/*we want to get notification from this RAID*/
    expected_state = FBE_LIFECYCLE_STATE_READY;
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 90000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    mut_printf(MUT_LOG_TEST_STATUS, " === Verified RAID is back to READY===\n");

    return;

} /* end of boomer_4_new_drive_test_verify_periodic_wakeup() */

/*!**************************************************************
 * boomer_4_new_drive_test_verify_io_wakeup()
 ****************************************************************
 * @brief
 *  Verify IO wakes up from hibernation
 *
 * @param rg_config_p - RG Config
 * 
 * @return none
 *
 ****************************************************************/
static void boomer_4_new_drive_test_verify_io_wakeup(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_object_id_t                             object_id, rg_object_id, lu_object_id;
    fbe_base_config_object_power_save_info_t    object_ps_info;
    fbe_status_t                                status;
    fbe_lifecycle_state_t                       object_state;
    

    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number, &lu_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_wait_for_object_lifecycle_state(lu_object_id, FBE_LIFECYCLE_STATE_HIBERNATE, 60000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " === Verified LU is back to HIBERNATE === \n");

    status = fbe_api_set_power_save_periodic_wakeup(10);/*make sure we sleep long enough*/
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, " ===Waiting to RAID to save power again===\n");
    /*wait for RAID to power save again*/
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_HIBERNATE, 60000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, " ===Raid is saving power, waiting for physical drives to save power===\n");

    status = fbe_api_get_physical_drive_object_id_by_location(rg_config_p->rg_disk_set[0].bus,
                                                              rg_config_p->rg_disk_set[0].enclosure,
                                                              rg_config_p->rg_disk_set[0].slot,
                                                              &object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*we don't want to rely on the notification here because we might miss it, we will do a simple delay*/
    fbe_api_wait_for_object_lifecycle_state(object_id, FBE_LIFECYCLE_STATE_HIBERNATE, 20000 ,FBE_PACKAGE_ID_PHYSICAL);

    mut_printf(MUT_LOG_TEST_STATUS, " ===Physical drives saving power, starting IO on RAID===\n");

    boomer_4_new_drive_test_run_io_load(lu_object_id);

    status = fbe_api_get_object_power_save_info(rg_object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(object_ps_info.power_save_state, FBE_POWER_SAVE_STATE_NOT_SAVING_POWER);
    status = fbe_api_get_object_lifecycle_state(rg_object_id, &object_state, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_LIFECYCLE_STATE_READY, object_state);

    return;

} /* end of boomer_4_new_drive_test_verify_io_wakeup() */

/*!**************************************************************
 * boomer_4_new_drive_test_verify_power_save_stat()
 ****************************************************************
 * @brief
 *  Verify power save stat
 *
 * @param rg_config_p - RG configuration
 * 
 * @return none
 *
 ****************************************************************/
static void boomer_4_new_drive_test_verify_power_save_stat(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                        status;
    fbe_physical_drive_information_t    drive_info;
    fbe_object_id_t                     object_id;

    mut_printf(MUT_LOG_TEST_STATUS, " ===Verifying statistics counters ===\n");
    
    /*lets make sure PDO 10 have some idle counters*/
    status = fbe_api_get_physical_drive_object_id_by_location(rg_config_p->unused_disk_set[0].bus,
                                                              rg_config_p->unused_disk_set[0].enclosure,
                                                              rg_config_p->unused_disk_set[0].slot,
                                                              &object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    status = fbe_api_physical_drive_get_cached_drive_info(object_id, &drive_info, FBE_PACKET_FLAG_NO_ATTRIB);

    mut_printf(MUT_LOG_TEST_STATUS, "Drive %d_%d_%d Up time:%d, Standby time:%d spin up count:%d\n",
                                       rg_config_p->unused_disk_set[0].bus,
                                       rg_config_p->unused_disk_set[0].enclosure,
                                       rg_config_p->unused_disk_set[0].slot,
                                       drive_info.spin_up_time_in_minutes,
                                       drive_info.stand_by_time_in_minutes,
                                       drive_info.spin_up_count );

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_TRUE(drive_info.spin_up_time_in_minutes > 0);
    MUT_ASSERT_TRUE(drive_info.stand_by_time_in_minutes > 0);
    MUT_ASSERT_TRUE(drive_info.spin_up_count == 1);
    
    /*DB drives should never go down*/
    status = fbe_api_get_physical_drive_object_id_by_location(0, 0, 0, &object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_physical_drive_get_cached_drive_info(object_id, &drive_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "Drive 0_0_0 Up time:%d, Standby time:%d\n",
               drive_info.spin_up_time_in_minutes, drive_info.stand_by_time_in_minutes);

    MUT_ASSERT_TRUE(drive_info.spin_up_time_in_minutes > 0);
    MUT_ASSERT_TRUE(drive_info.stand_by_time_in_minutes == 0);
    MUT_ASSERT_TRUE(drive_info.spin_up_count == 0);/*since we never injected any error on this drive it never faces a spin up count even at boot time*/
    
    mut_printf(MUT_LOG_TEST_STATUS, " ===Verifying statistics counters: PASSED ===\n");
    
} /* end of boomer_4_new_drive_test_verify_power_save_stat()*/

/*!**************************************************************
 * boomer_4_new_drive_test_inject_not_spinning_error()
 ****************************************************************
 * @brief
 *  Inject not spinning error
 *
 * @param rg_config_p - RG configuration
 * 
 * @return none
 *
 ****************************************************************/
static void boomer_4_new_drive_test_inject_not_spinning_error(fbe_test_rg_configuration_t *rg_config_p)
{
    /*in simulation, the terminator always returns a good spinup status so we have to force it for at least one drive
    so later we can test the spin up counter going up*/
    fbe_protocol_error_injection_error_record_t     record;                 // error injection record 
    fbe_status_t                                    status;                 // fbe stat
    fbe_object_id_t                                 object_id;
    fbe_protocol_error_injection_record_handle_t    out_record_handle_p;

    fbe_test_neit_utils_init_error_injection_record(&record);

    /*we'll set a spinup error on PDO 10 so when in ACTIVATE it will explicitly set the not_spinning condition and then 
    the counter will go up once spin up is successfull*/
    status = fbe_api_get_physical_drive_object_id_by_location(rg_config_p->unused_disk_set[0].bus,
                                                              rg_config_p->unused_disk_set[0].enclosure,
                                                              rg_config_p->unused_disk_set[0].slot,
                                                              &object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    record.object_id                = object_id; 
    record.lba_start                = 0;
    record.lba_end                  = 0xFFFFFF;
    record.num_of_times_to_insert   = 1;

    record.protocol_error_injection_error_type = FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[0] = FBE_SCSI_TEST_UNIT_READY;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_sense_key = FBE_SCSI_SENSE_KEY_NOT_READY;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code = FBE_SCSI_ASC_NOT_READY;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code_qualifier = FBE_SCSI_ASCQ_NOT_SPINNING;

    //  Add the error injection record to the service, which also returns the record handle for later use
    status = fbe_api_protocol_error_injection_add_record(&record, &out_record_handle_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    //  Start the error injection 
    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

} /* end of boomer_4_new_drive_test_inject_not_spinning_error() */

/*!**************************************************************
 * boomer_4_new_drive_test_create_rg()
 ****************************************************************
 * @brief
 *  Create RG
 *
 * @param rg_config_p - RG config              
 *
 * @return None.
 *
 ****************************************************************/

static void boomer_4_new_drive_test_create_rg(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_object_id_t                     rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_job_service_error_type_t        job_error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    fbe_status_t                        job_status = FBE_STATUS_OK;

    /* Create a RG */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Create a RAID Group ===\n");
    status = fbe_test_sep_util_create_raid_group(rg_config_p);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "RG creation failed");

    /* wait for notification from job service. */
    status = fbe_api_common_wait_for_job(rg_config_p->job_number,
                                         BOOMER_4_NEW_DRIVE_JOB_SERVICE_CREATION_TIMEOUT,
                                         &job_error_code,
                                         &job_status,
                                         NULL);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to created RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    /* Verify the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    /* Verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
                                                     FBE_LIFECYCLE_STATE_READY, 20000,
                                                     FBE_PACKAGE_ID_SEP_0);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "Created Raid Group object %d\n", rg_object_id);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: END\n", __FUNCTION__);

    return;

} /* End boomer_4_new_drive_test_create_rg() */

/*!**************************************************************
 * boomer_4_new_drive_test_verify_system_power_save_after_reboot()
 ****************************************************************
 * @brief
 *   Verify system power saving after reboot
 *
 * @param none
 * 
 * @return none
 *
 ****************************************************************/
static void boomer_4_new_drive_test_verify_system_power_save_after_reboot(void)
{
    fbe_system_power_saving_info_t              power_save_info;
    fbe_status_t                                status;

    mut_printf(MUT_LOG_TEST_STATUS, " %s: Enabling system wide power saving \n", __FUNCTION__);
    status  = fbe_api_enable_system_power_save();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status  = fbe_api_enable_system_power_save_statistics();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    boomer_4_new_drive_reboot_sp();
   
    status  = fbe_api_get_system_power_save_info(&power_save_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(power_save_info.enabled == FBE_TRUE); 
    MUT_ASSERT_TRUE(power_save_info.stats_enabled == FBE_TRUE);   

    status  = fbe_api_disable_system_power_save();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status  = fbe_api_disable_system_power_save_statistics();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    return;
} /* end of boomer_4_new_drive_test_verify_system_power_save_after_reboot() */

/*!****************************************************************************
 *  boomer_4_new_drive_run_test
 ******************************************************************************
 *
 * @brief
 *  This is the main executing function of the Boomer For New Drive Test.
 *
 * @param
 *
 * rg_config_ptr    - pointer to raid group 
 * context_pp       - pointer to context
 *
 * @return  None 
 *****************************************************************************/
void boomer_4_new_drive_run_test(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p)
{
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                   rg_index = 0;
    fbe_u32_t                   num_raid_groups = 0;
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_ptr);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Start ...", __FUNCTION__);

    for(rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        rg_config_p = rg_config_ptr + rg_index;

        status = fbe_test_sep_util_rg_config_disable_zeroing( rg_config_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        fbe_semaphore_init(&sem, 0, 1);
        status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_HIBERNATE | FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_READY,
                                                                      FBE_PACKAGE_NOTIFICATION_ID_SEP_0 | FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL,
                                                                      FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE |
                                                                      FBE_TOPOLOGY_OBJECT_TYPE_LUN |
                                                                      FBE_TOPOLOGY_OBJECT_TYPE_RAID_GROUP |
                                                                      FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE,
                                                                      boomer_4_new_drive_commmand_response_callback_function,
                                                                      &sem,
                                                                      &reg_id);

        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        boomer_4_new_drive_test_inject_not_spinning_error(rg_config_p);
        boomer_4_new_drive_test_verify_system_enable_disable();
        boomer_4_new_drive_test_verify_object_enable_disable(rg_config_p);
        boomer_4_new_drive_test_verify_background_io_not_count(rg_config_p);
        boomer_4_new_drive_test_verify_power_saving_policy(rg_config_p);
        boomer_4_new_drive_test_verify_bound_objects_save_power(rg_config_p);
        boomer_4_new_drive_test_verify_unbound_objects_save_power(rg_config_p);
        boomer_4_new_drive_test_verify_objects_get_out_of_power_save(rg_config_p);
        boomer_4_new_drive_test_verify_periodic_wakeup(rg_config_p);
        boomer_4_new_drive_test_verify_io_wakeup(rg_config_p);
        boomer_4_new_drive_test_verify_lun_creation_on_a_hibernating_rg(rg_config_p);
        
        /*we want to clean and go out w/o leaving the power saving persisted on the disks for next test*/
        mut_printf(MUT_LOG_TEST_STATUS, " %s: Disabling system wide power saving \n", __FUNCTION__);
        status  = fbe_api_disable_system_power_save();
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        boomer_4_new_drive_test_verify_power_save_stat(rg_config_p); 

        /* make sure all the objects in READY state before rebooting, otherwise the RG will stay SPECIALIZE forever, 
         * since LDO/PDO stay HIBERNATE after rebooting */
        boomer_4_new_drive_test_verify_objects_get_out_of_power_save(rg_config_p);
        boomer_4_new_drive_test_verify_system_power_save_after_reboot();

        status = fbe_api_notification_interface_unregister_notification(boomer_4_new_drive_commmand_response_callback_function, reg_id);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        mut_printf(MUT_LOG_TEST_STATUS, " %s: Waiting for objects to get out of hibernation \n", __FUNCTION__);
        
        fbe_semaphore_destroy(&sem);
    }

    return;
} /* end of boomer_4_new_drive_run_test() */

/*!****************************************************************************
 * boomer_4_new_drive_test()
 ******************************************************************************
 * @brief
 *  This is the main entry point for the Boomer For New Drive Test.  
 *
 * @param None.
 *
 * @return None.
 *
 ******************************************************************************/
void boomer_4_new_drive_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    rg_config_p = &boomer_4_new_drive_rg_config[0];

    mut_printf(MUT_LOG_TEST_STATUS, "=== Called boomer_4_new_drive_test ===\n");
    
    fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
    fbe_test_run_test_on_rg_config(rg_config_p, NULL, boomer_4_new_drive_run_test, 
                                    BOOMER_4_NEW_DRIVE_TEST_LUNS_PER_RAID_GROUP, 
                                    BOOMER_4_NEW_DRIVE_TEST_CHUNKS_PER_LUN);

    return;
}/* End boomer_4_new_drive_test() */

/*!****************************************************************************
 * boomer_4_new_drive_test_setup()
 ******************************************************************************
 * @brief
 *  This is the setup function for the Boomer For New Drive Test. It is responsible
 *  for loading the physical and logical configuration for this test suite.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 ******************************************************************************/
void boomer_4_new_drive_test_setup(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Boomer For New Drive test ===\n");

    if(fbe_test_util_is_simulation())
    {
        /* Load the physical configuration */
        boomer_4_new_drive_test_load_physical_config();

        /* Load the logical configuration */
        sep_config_load_sep_and_neit();
    }

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();

    return;
}/* End boomer_4_new_drive_test_setup() */


/*!**************************************************************
 * boomer_4_new_drive_test_cleanup()
 ****************************************************************
 *
 * @brief
 *  This is the cleanup function for the Boomer_4_new_drive test.
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
void boomer_4_new_drive_test_cleanup(void)
{  

    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for Boomer For New Drive Test ===\n");

    if(fbe_test_util_is_simulation())
    {
        /* Destroy the test configuration */
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
} /* End boomer_4_new_drive_test_cleanup() */


/*!**************************************************************
 * boomer_4_new_drive_test_verify_unbound_objects_save_power()
 ****************************************************************
 * @brief
 *  Verify LUN creation on hibernating RG.
 *
 * @param rg_config_p - RG configuration
 * 
 * @return none
 *
 ****************************************************************/
static void boomer_4_new_drive_test_verify_lun_creation_on_a_hibernating_rg(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_object_id_t         		rg_object_id, pvd_object_id;
    DWORD                   		dwWaitResult;
    fbe_status_t            		status;
    fbe_api_raid_group_create_t 	create_rg;
    fbe_u32_t						slot = 0;
    fbe_job_service_error_type_t	job_error_code;
    fbe_api_lun_create_t    		fbe_lun_create_req;
    fbe_object_id_t         		lu_id;
    fbe_database_raid_group_info_t  rg_info;
    fbe_database_pvd_info_t         pvd_info;
    fbe_u32_t                       pvd_index;
    fbe_api_base_config_downstream_object_list_t  downstream_object_list;
    fbe_u32_t index; 
    
    mut_printf(MUT_LOG_TEST_STATUS, " === Testing creation of a LUN on a hibernating R1_0, creating RG and waiting for it to hibernate === \n");

    fbe_zero_memory(&create_rg, sizeof(fbe_api_raid_group_create_t));
    create_rg.raid_group_id = 103;
    create_rg.b_bandwidth = 0;
    create_rg.capacity = 0x1000;
    create_rg.drive_count = 4;
    create_rg.raid_type = FBE_RAID_GROUP_TYPE_RAID10;
    create_rg.is_private = FBE_FALSE;
    create_rg.max_raid_latency_time_is_sec = 120;

    for (;slot < 4; slot++) {
        create_rg.disk_array[slot].bus = 0;
        create_rg.disk_array[slot].enclosure = 1;
        create_rg.disk_array[slot].slot = slot;

        status = fbe_api_provision_drive_get_obj_id_by_location(create_rg.disk_array[slot].bus,
                                                       create_rg.disk_array[slot].enclosure,
                                                       create_rg.disk_array[slot].slot,
                                                       &pvd_object_id);
        MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

        status = fbe_test_sep_util_provision_drive_disable_zeroing(pvd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    status = fbe_api_create_rg(&create_rg, FBE_TRUE, 20000, &rg_object_id, &job_error_code);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "RG creation failed");
    MUT_ASSERT_INT_EQUAL_MSG(job_error_code, FBE_JOB_SERVICE_ERROR_NO_ERROR, 
                               "job error code is not FBE_JOB_SERVICE_ERROR_NO_ERROR");

    status = fbe_api_enable_raid_group_power_save(rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_set_raid_group_power_save_idle_time(rg_object_id, 15);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_HIBERNATE, 300000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " === Verified RG is HIBERNATE, creating a lun on top of it === \n");

    status = fbe_api_database_get_raid_group(rg_object_id, &rg_info);                                                      
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);                                                                           
    for (pvd_index=0; pvd_index < rg_info.pvd_count; pvd_index++){                                                         
        status = fbe_api_database_get_pvd(rg_info.pvd_list[pvd_index], &pvd_info);                                         
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);                                                                       
        MUT_ASSERT_INT_EQUAL(FBE_TRUE, pvd_info.power_save_info.power_saving_enabled);                                     
    }                                                                                                                                                                                                                                                                                                                      
                                                                                                                           
    fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);                                                                                                                                                 
    for (index = 0; index < downstream_object_list.number_of_downstream_objects; index++)                        
    {                                                                                                                                        
        status = fbe_api_wait_for_object_lifecycle_state(downstream_object_list.downstream_object_list[index], 
                                                         FBE_LIFECYCLE_STATE_HIBERNATE, 150000, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);                                                   
        mut_printf(MUT_LOG_TEST_STATUS, " === Verified RAID INTERNAL_MIRROR_UNDER_STRIPER is in hibernate===\n");
    }                                                                                                            

    /*let's bind a lun on this RG*/
    fbe_zero_memory(&fbe_lun_create_req, sizeof(fbe_api_lun_create_t));
    fbe_lun_create_req.raid_type = FBE_RAID_GROUP_TYPE_RAID10;
    fbe_lun_create_req.raid_group_id = 103;
    fbe_lun_create_req.lun_number = 11;
    fbe_lun_create_req.capacity = 0x100;
    fbe_lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
    fbe_lun_create_req.ndb_b = FBE_FALSE;
    fbe_lun_create_req.noinitialverify_b = FBE_TRUE;
    fbe_lun_create_req.addroffset = FBE_LBA_INVALID; /* set to a valid offset for NDB */

    status = fbe_api_create_lun(&fbe_lun_create_req, FBE_FALSE, 15000, &lu_id, &job_error_code);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);	
    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X created successfully! \n", __FUNCTION__, lu_id);

    expected_object_id = rg_object_id;/*we want to get notification from this RAID*/
    expected_state = FBE_LIFECYCLE_STATE_READY;
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 120000); 
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    mut_printf(MUT_LOG_TEST_STATUS, " === Verified RAID is back to READY due to lun creation ===\n");
    
    status = fbe_test_sep_utils_wait_for_attribute(VOL_ATTR_STANDBY_POSSIBLE, 10000, lu_id, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X Success! VOL_ATTR_STANDBY_POSSIBLE set! \n", __FUNCTION__, lu_id);
    
    status = fbe_test_sep_utils_wait_for_attribute(VOL_ATTR_IDLE_TIME_MET, 60000, lu_id, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X Success! VOL_ATTR_IDLE_TIME_MET set! \n", __FUNCTION__, lu_id);
    
    status = fbe_api_wait_for_object_lifecycle_state(lu_id, FBE_LIFECYCLE_STATE_HIBERNATE, 200000, FBE_PACKAGE_ID_SEP_0);
     MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " === Verified LUN is back to HIBERNATE === \n");
    
    status = fbe_test_sep_utils_wait_for_attribute(VOL_ATTR_STANDBY_TIME_MET, 80000, lu_id, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X Success! VOL_ATTR_STANDBY_TIME_MET set! \n", __FUNCTION__, lu_id);
    
    status = fbe_test_sep_utils_wait_for_attribute(VOL_ATTR_STANDBY_POSSIBLE, 80000, lu_id, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X Success! VOL_ATTR_STANDBY_POSSIBLE set! \n", __FUNCTION__, lu_id);
    
    status = fbe_api_disable_raid_group_power_save(rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: Disabling power saving on the raid group \n", __FUNCTION__); 
    
    status = fbe_api_wait_for_object_lifecycle_state(lu_id, FBE_LIFECYCLE_STATE_READY, 300000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " ===Verified LU is back to READY ===\n");
    
    status = fbe_test_sep_utils_wait_for_attribute(VOL_ATTR_IDLE_TIME_MET, 1000, lu_id, FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X Success! VOL_ATTR_IDLE_TIME_MET not set! \n", __FUNCTION__, lu_id);
    
    status = fbe_test_sep_utils_wait_for_attribute(VOL_ATTR_STANDBY_POSSIBLE, 1000, lu_id, FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X Success! VOL_ATTR_STANDBY_POSSIBLE not set! \n", __FUNCTION__, lu_id);
    
    status = fbe_test_sep_utils_wait_for_attribute(VOL_ATTR_STANDBY_TIME_MET, 1000, lu_id, FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X VOL_ATTR_STANDBY_TIME_MET not set! \n", __FUNCTION__, lu_id);
    
} /* end of boomer_4_new_drive_test_verify_lun_creation_on_a_hibernating_rg() */
