/***************************************************************************
 *  terminator_eses_tests_led_patterns.c
 ***************************************************************************
 *
 *  Description
 *      ESES enclosure tests related to LED Patterns.
 *      
 *
 *  History:
 *     Dec-02-08   Rajesh V. Created
 *    
 ***************************************************************************/


#include "terminator_test.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe_terminator_miniport_interface.h"
#include "fbe_terminator.h"

#include "fbe/fbe_eses.h"
#include "fbe_scsi.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_topology_interface.h"
#include "terminator_eses_test.h"

#define LED_TEST_POWER_CYCLE_TIME 10000
#define LED_TEST_SLEEP_TIME 5000

/**********************************/
/*        local variables        */
/**********************************/
typedef struct led_patterns_test_io_completion_context_s
{
    fbe_u8_t encl_id;
    fbe_u8_t drive_slot;
    ses_stat_elem_array_dev_slot_struct array_dev_slot_stat;
    ses_stat_elem_encl_struct encl_stat;
    ses_stat_elem_encl_struct chassis_encl_stat;
}led_patterns_test_io_completion_context_t;

static led_patterns_test_io_completion_context_t led_patterns_test_io_completion_context;
static fbe_semaphore_t   led_patterns_test_continue_semaphore;
static fbe_u32_t io_completion_call_count;
static fbe_u8_t led_patterns_login_count;
static fbe_u8_t led_patterns_logout_count;

/**********************************/
/*       LOCAL FUNCTIONS          */
/**********************************/
static fbe_status_t terminator_eses_tests_led_patterns_io_completion (fbe_payload_ex_t * payload, fbe_payload_ex_completion_context_t context);
static fbe_status_t terminator_eses_tests_led_patterns_callback(fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context);
static void init_led_patterns_test_io_completion_context(void);
static void led_pattern_test_build_chassis_encl_ctrl_elem(fbe_sg_element_t *sg_p,
                                                          ses_ctrl_elem_encl_struct chassis_encl_ctrl_elem_to_fill);
void led_pattern_test_build_local_encl_ctrl_elem(fbe_sg_element_t *sg_p,
                                                 ses_ctrl_elem_encl_struct encl_ctrl_elem_to_fill);
void led_pattern_test_build_ctrl_page(fbe_sg_element_t *sg_p);
void led_pattern_test_build_array_dev_slot_ctrl_elem(fbe_sg_element_t *sg_p,
                                                     fbe_u8_t slot_num,
                                                     ses_ctrl_elem_array_dev_slot_struct array_dev_slot_ctrl_elem_to_fill);
static void led_pattern_test_build_chassis_encl_ctrl_elem(fbe_sg_element_t *sg_p,
                                                          ses_ctrl_elem_encl_struct chassis_encl_ctrl_elem_to_fill);
static void led_pattern_test_build_local_encl_ctrl_elem(fbe_sg_element_t *sg_p,
                                                        ses_ctrl_elem_encl_struct encl_ctrl_elem_to_fill);


static fbe_status_t terminator_eses_tests_led_patterns_io_completion (fbe_payload_ex_t * payload, fbe_payload_ex_completion_context_t context);




void init_led_patterns_test_io_completion_context(void)
{
    memset(&led_patterns_test_io_completion_context, 0, sizeof(led_patterns_test_io_completion_context_t));
}

static fbe_status_t terminator_eses_tests_led_patterns_callback(fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context)
{
    if(callback_info->callback_type == FBE_PMC_SHIM_CALLBACK_TYPE_LOGOUT)
    {
        led_patterns_logout_count++;
    }
    else if(callback_info->callback_type == FBE_PMC_SHIM_CALLBACK_TYPE_LOGIN)
    {
        led_patterns_login_count++;
    }

    return FBE_STATUS_OK;
}


void terminator_led_patterns_test(void)
{
    fbe_status_t                    status;
    fbe_terminator_board_info_t     board_info;
    fbe_terminator_sas_port_info_t  sas_port;
    fbe_terminator_sas_encl_info_t  sas_encl;
    fbe_terminator_sas_drive_info_t sas_drive;
    fbe_terminator_api_device_handle_t  encl_id;
    fbe_terminator_api_device_handle_t  drive_id, drive_id1;
    fbe_sg_element_t *sg_p = NULL;
    fbe_u32_t  port_index;
    ses_ctrl_elem_array_dev_slot_struct array_dev_slot_ctrl_elem;
    ses_ctrl_elem_encl_struct encl_ctrl_elem;
    ses_ctrl_elem_encl_struct chassis_encl_ctrl_elem;
    fbe_u8_t subencl_id = 4;

    mut_printf(MUT_LOG_HIGH, "%s %s ",__FILE__, __FUNCTION__);

    fbe_semaphore_init(&led_patterns_test_continue_semaphore, 0, FBE_SEMAPHORE_MAX);
    init_led_patterns_test_io_completion_context();

     /*initialize the terminator*/
    status = fbe_terminator_api_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator initialized.", __FUNCTION__);

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = SPID_DREADNOUGHT_HW_TYPE;
    status  = fbe_terminator_api_insert_board (&board_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*insert a port*/
    sas_port.sas_address = (fbe_u64_t)0x111111111;
    sas_port.io_port_number = 0;
    sas_port.portal_number = 1;
    sas_port.backend_number = 0;
    sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    status  = fbe_terminator_api_insert_sas_port (0, &sas_port);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_terminator_miniport_api_port_init(0, 1, &port_index);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Register a callback to port 0, so we can destroy the object properly */
    status = fbe_terminator_miniport_api_register_callback(port_index, 
                                                           terminator_eses_tests_led_patterns_callback, 
                                                           NULL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);


    /*insert an enclosure*/
    // id 2
    sas_encl.uid[0] = 2; // some unique ID.
    sas_encl.sas_address = (fbe_u64_t)0x123456780;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    sas_encl.connector_id = 0;
    status  = fbe_terminator_api_insert_sas_enclosure (0, &sas_encl, &encl_id);/*add encl on port 0*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*insert an enclosure*/
    // id 4
    sas_encl.uid[0] = 4; // some unique ID.
    sas_encl.sas_address = (fbe_u64_t)0x123456782;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    sas_encl.connector_id = 0;
    status  = fbe_terminator_api_insert_sas_enclosure (0, &sas_encl, &encl_id);/*add encl on port 0*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*insert an enclosure*/
    // id 6
    sas_encl.uid[0] = 6; // some unique ID.
    sas_encl.sas_address = (fbe_u64_t)0x123456784;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    sas_encl.connector_id = 0;
    status  = fbe_terminator_api_insert_sas_enclosure (0, &sas_encl, &encl_id);/*add encl on port 0*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);


    /*insert a sas_drive to the second  enclosure*/
    // inserted in encl id 4, in slot 0.
    sas_drive.sas_address = (fbe_u64_t)0x987654321;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.capacity = 0x10BD0;
    sas_drive.block_size = 520;
    status = fbe_terminator_api_insert_sas_drive (0, 4, 0, &sas_drive, &drive_id);/*add drive to port 0 encl 1*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*insert a sas_drive to the third enclosure*/
    // inserted in encl id 6, in slot 1.
    sas_drive.sas_address = (fbe_u64_t)0x987654326;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.capacity = 0x10BD0;
    sas_drive.block_size = 520;
    status = fbe_terminator_api_insert_sas_drive (0, 6, 1, &sas_drive, &drive_id1);/*add drive to port 0 encl 2*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);


    // Register the IO completion function
	status = fbe_terminator_miniport_api_register_payload_completion(0,
																    terminator_eses_tests_led_patterns_io_completion,
																    &led_patterns_test_io_completion_context);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 

    // Test 1. Set the ident bit to activate the identify pattern.
    memset(&array_dev_slot_ctrl_elem, 0, sizeof(ses_ctrl_elem_array_dev_slot_struct));
    array_dev_slot_ctrl_elem.rqst_ident = 1;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    led_pattern_test_build_ctrl_page(sg_p);
    led_pattern_test_build_array_dev_slot_ctrl_elem(sg_p, 6, array_dev_slot_ctrl_elem);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_SEND_DIAGNOSTIC, &ses_pg_code_encl_ctrl,  sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&led_patterns_test_continue_semaphore, NULL);
    // set the completion context values accordingly
    memset(&led_patterns_test_io_completion_context, 0, sizeof(led_patterns_test_io_completion_context_t));
    led_patterns_test_io_completion_context.encl_id = 4;
    led_patterns_test_io_completion_context.drive_slot = 6;
    led_patterns_test_io_completion_context.array_dev_slot_stat.ident = 1;  
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&led_patterns_test_continue_semaphore, NULL);

    // Test 2. Set the rqst fault bit to activate fault pattern.
    memset(&array_dev_slot_ctrl_elem, 0, sizeof(ses_ctrl_elem_array_dev_slot_struct));
    array_dev_slot_ctrl_elem.rqst_fault = 1;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    led_pattern_test_build_ctrl_page(sg_p);
    led_pattern_test_build_array_dev_slot_ctrl_elem(sg_p, 8, array_dev_slot_ctrl_elem);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_SEND_DIAGNOSTIC, &ses_pg_code_encl_ctrl,  sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&led_patterns_test_continue_semaphore, NULL);
    // set the completion context values accordingly
    memset(&led_patterns_test_io_completion_context, 0, sizeof(led_patterns_test_io_completion_context_t));
    led_patterns_test_io_completion_context.encl_id = 4;
    led_patterns_test_io_completion_context.drive_slot = 8;
    led_patterns_test_io_completion_context.array_dev_slot_stat.fault_requested = 1;  
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&led_patterns_test_continue_semaphore, NULL);

    // Test 3. Set the rqst fault bit to activate fault pattern.
    memset(&array_dev_slot_ctrl_elem, 0, sizeof(ses_ctrl_elem_array_dev_slot_struct));
    array_dev_slot_ctrl_elem.rqst_fault = 1;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    led_pattern_test_build_ctrl_page(sg_p);
    led_pattern_test_build_array_dev_slot_ctrl_elem(sg_p, 8, array_dev_slot_ctrl_elem);
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_SEND_DIAGNOSTIC, &ses_pg_code_encl_ctrl,  sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&led_patterns_test_continue_semaphore, NULL);
    // set the completion context values accordingly
    memset(&led_patterns_test_io_completion_context, 0, sizeof(led_patterns_test_io_completion_context_t));
    led_patterns_test_io_completion_context.encl_id = 2;
    led_patterns_test_io_completion_context.drive_slot = 8;
    led_patterns_test_io_completion_context.array_dev_slot_stat.fault_requested = 1; 
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&led_patterns_test_continue_semaphore, NULL);

    // Test 4. Set the ident bit to activate the identify pattern.
    memset(&array_dev_slot_ctrl_elem, 0, sizeof(ses_ctrl_elem_array_dev_slot_struct));
    array_dev_slot_ctrl_elem.rqst_ident = 1;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    led_pattern_test_build_ctrl_page(sg_p);
    led_pattern_test_build_array_dev_slot_ctrl_elem(sg_p, 2, array_dev_slot_ctrl_elem);
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_SEND_DIAGNOSTIC, &ses_pg_code_encl_ctrl,  sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&led_patterns_test_continue_semaphore, NULL);
    // set the completion context values accordingly
    memset(&led_patterns_test_io_completion_context, 0, sizeof(led_patterns_test_io_completion_context_t));
    led_patterns_test_io_completion_context.encl_id = 2;
    led_patterns_test_io_completion_context.drive_slot = 2;
    led_patterns_test_io_completion_context.array_dev_slot_stat.ident = 1; 
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&led_patterns_test_continue_semaphore, NULL);

    // Test 5. Local Enclosure status element test
    memset(&encl_ctrl_elem, 0, sizeof(ses_stat_elem_encl_struct));
    encl_ctrl_elem.rqst_ident = 1;
    encl_ctrl_elem.rqst_failure = 1;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    led_pattern_test_build_ctrl_page(sg_p);
    led_pattern_test_build_local_encl_ctrl_elem(sg_p, encl_ctrl_elem);
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_SEND_DIAGNOSTIC, &ses_pg_code_encl_ctrl,  sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&led_patterns_test_continue_semaphore, NULL);
    // set the completion context values accordingly
    memset(&led_patterns_test_io_completion_context, 0, sizeof(led_patterns_test_io_completion_context_t));
    led_patterns_test_io_completion_context.encl_id = 2;
    led_patterns_test_io_completion_context.encl_stat.ident = 1;
    led_patterns_test_io_completion_context.encl_stat.failure_rqsted = 1;
    led_patterns_test_io_completion_context.encl_stat.failure_indicaton = 1;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&led_patterns_test_continue_semaphore, NULL);

    // Test 6. Local Enclosure status element test
    memset(&encl_ctrl_elem, 0, sizeof(ses_stat_elem_encl_struct));
    encl_ctrl_elem.rqst_ident = 0;
    encl_ctrl_elem.rqst_failure = 0;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    led_pattern_test_build_ctrl_page(sg_p);
    led_pattern_test_build_local_encl_ctrl_elem(sg_p, encl_ctrl_elem);
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_SEND_DIAGNOSTIC, &ses_pg_code_encl_ctrl,  sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&led_patterns_test_continue_semaphore, NULL);
    // set the completion context values accordingly
    memset(&led_patterns_test_io_completion_context, 0, sizeof(led_patterns_test_io_completion_context_t));
    led_patterns_test_io_completion_context.encl_id = 2;
    led_patterns_test_io_completion_context.encl_stat.ident = 0;
    led_patterns_test_io_completion_context.encl_stat.failure_rqsted = 0;
    led_patterns_test_io_completion_context.encl_stat.failure_indicaton = 0;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&led_patterns_test_continue_semaphore, NULL);

    // Test 7. Chassis Enclosure status element test
    memset(&chassis_encl_ctrl_elem, 0, sizeof(ses_stat_elem_encl_struct));
    chassis_encl_ctrl_elem.rqst_ident = 1;
    chassis_encl_ctrl_elem.rqst_failure = 1;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    led_pattern_test_build_ctrl_page(sg_p);
    led_pattern_test_build_chassis_encl_ctrl_elem(sg_p, chassis_encl_ctrl_elem);
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_SEND_DIAGNOSTIC, &ses_pg_code_encl_ctrl,  sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&led_patterns_test_continue_semaphore, NULL);
    // set the completion context values accordingly
    memset(&led_patterns_test_io_completion_context, 0, sizeof(led_patterns_test_io_completion_context_t));
    led_patterns_test_io_completion_context.encl_id = 2;
    led_patterns_test_io_completion_context.chassis_encl_stat.ident = 1;
    led_patterns_test_io_completion_context.chassis_encl_stat.failure_rqsted = 1;
    led_patterns_test_io_completion_context.chassis_encl_stat.failure_indicaton = 1;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&led_patterns_test_continue_semaphore, NULL);

    // Test 8. Chassis Enclosure status element test
    memset(&chassis_encl_ctrl_elem, 0, sizeof(ses_stat_elem_encl_struct));
    chassis_encl_ctrl_elem.rqst_ident = 0;
    chassis_encl_ctrl_elem.rqst_warning = 1;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    led_pattern_test_build_ctrl_page(sg_p);
    led_pattern_test_build_chassis_encl_ctrl_elem(sg_p, chassis_encl_ctrl_elem);
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_SEND_DIAGNOSTIC, &ses_pg_code_encl_ctrl,  sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&led_patterns_test_continue_semaphore, NULL);
    // set the completion context values accordingly
    memset(&led_patterns_test_io_completion_context, 0, sizeof(led_patterns_test_io_completion_context_t));
    led_patterns_test_io_completion_context.encl_id = 2;
    led_patterns_test_io_completion_context.chassis_encl_stat.ident = 0;
    led_patterns_test_io_completion_context.chassis_encl_stat.warning_indicaton = 1;
    led_patterns_test_io_completion_context.chassis_encl_stat.warning_rqsted = 1;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&led_patterns_test_continue_semaphore, NULL);

    // Sleep for all previous logins to complete
    EmcutilSleep(LED_TEST_SLEEP_TIME);
    led_patterns_login_count = 0;
    led_patterns_logout_count = 0;

    // Test 9. Issue an LCC power cycle and test it.
    memset(&chassis_encl_ctrl_elem, 0, sizeof(ses_stat_elem_encl_struct));
    chassis_encl_ctrl_elem.power_cycle_rqst = 1;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    led_pattern_test_build_ctrl_page(sg_p);
    led_pattern_test_build_local_encl_ctrl_elem(sg_p, chassis_encl_ctrl_elem);
    // Issue command to port 0, enclosure 1(id:4). It has 1 drive and 1 downstream enclosure that has an other drive.
    // So we should get a total of 6 logins & 6 logouts (counting Virtual Phys in each encl)
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_SEND_DIAGNOSTIC, &ses_pg_code_encl_ctrl,  sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&led_patterns_test_continue_semaphore, NULL);

    EmcutilSleep(LED_TEST_POWER_CYCLE_TIME / 2);

    MUT_ASSERT_INT_EQUAL(6, led_patterns_logout_count);

    // Now all enclosures & drives downsream of encl 1 should have been logged out.
    // Issue a status page to encl 0, so to check the connector in encl 0 corresponding
    // to encl 1( that is logged out) is correctly updated.
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&led_patterns_test_continue_semaphore, NULL);

    EmcutilSleep(LED_TEST_POWER_CYCLE_TIME / 2);
    EmcutilSleep(LED_TEST_SLEEP_TIME);

    MUT_ASSERT_INT_EQUAL(6, led_patterns_login_count);

    status = fbe_terminator_api_destroy();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator destroyed.", __FUNCTION__);
}


void led_pattern_test_build_array_dev_slot_ctrl_elem(fbe_sg_element_t *sg_p,
                                                     fbe_u8_t slot_num,
                                                     ses_ctrl_elem_array_dev_slot_struct array_dev_slot_ctrl_elem_to_fill)
{
    ses_ctrl_elem_array_dev_slot_struct *array_dev_slot_ctrl_elem;

    array_dev_slot_ctrl_elem = (ses_ctrl_elem_array_dev_slot_struct *) (((fbe_u8_t *)fbe_sg_element_address(sg_p)) + ARRAY_DEVICE_SLOT_ELEM_OFFSET);
    array_dev_slot_ctrl_elem++;
    array_dev_slot_ctrl_elem+= slot_num;
    memcpy(array_dev_slot_ctrl_elem, &array_dev_slot_ctrl_elem_to_fill, sizeof(ses_ctrl_elem_array_dev_slot_struct));
    array_dev_slot_ctrl_elem->cmn_ctrl.select = TRUE;
}


void led_pattern_test_build_ctrl_page(fbe_sg_element_t *sg_p)
{
    ses_pg_encl_ctrl_struct *encl_ctrl_page_hdr = NULL;

    encl_ctrl_page_hdr = (ses_pg_encl_ctrl_struct *)fbe_sg_element_address(sg_p);
    encl_ctrl_page_hdr->pg_code =  SES_PG_CODE_ENCL_CTRL;
    encl_ctrl_page_hdr->pg_len = BYTE_SWAP_16(STAT_PAGE_LENGTH);
}

void led_pattern_test_build_local_encl_ctrl_elem(fbe_sg_element_t *sg_p,
                                                 ses_ctrl_elem_encl_struct encl_ctrl_elem_to_fill)
{
    ses_ctrl_elem_encl_struct *encl_ctrl_elem;

    encl_ctrl_elem = (ses_ctrl_elem_encl_struct *) (((fbe_u8_t *)fbe_sg_element_address(sg_p)) + LOCAL_LCC_ENCL_STAT_ELEM_OFFSET);
    encl_ctrl_elem++;
    memcpy(encl_ctrl_elem, &encl_ctrl_elem_to_fill, sizeof(ses_ctrl_elem_array_dev_slot_struct));
    encl_ctrl_elem->cmn_ctrl.select = TRUE;
}

void led_pattern_test_build_chassis_encl_ctrl_elem(fbe_sg_element_t *sg_p,
                                                   ses_ctrl_elem_encl_struct chassis_encl_ctrl_elem_to_fill)
{
    ses_ctrl_elem_encl_struct *chassis_encl_ctrl_elem;

    chassis_encl_ctrl_elem = (ses_ctrl_elem_encl_struct *) (((fbe_u8_t *)fbe_sg_element_address(sg_p)) + CHASSIS_ENCL_STAT_ELEM_OFFSET);
    chassis_encl_ctrl_elem++;
    memcpy(chassis_encl_ctrl_elem, &chassis_encl_ctrl_elem_to_fill, sizeof(ses_ctrl_elem_array_dev_slot_struct));
    chassis_encl_ctrl_elem->cmn_ctrl.select = TRUE;
}


static fbe_status_t terminator_eses_tests_led_patterns_io_completion (fbe_payload_ex_t * payload, fbe_payload_ex_completion_context_t context)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_cdb_operation_t *payload_cdb_operation;
    fbe_u8_t *cdb = NULL;
    fbe_sg_element_t *sg_list = NULL;
    fbe_u8_t *return_page_code;
    ses_pg_encl_stat_struct  *encl_status_diag_page_hdr = NULL;
    ses_stat_elem_array_dev_slot_struct *array_dev_slot_stat_rtn_ptr;
    ses_stat_elem_encl_struct *encl_stat_rtn_ptr = NULL;
    ses_stat_elem_encl_struct *chassis_encl_stat_rtn_ptr = NULL;
    // As all these tests are only on port 0.
    fbe_u8_t encl_id = ((led_patterns_test_io_completion_context_t *)context)->encl_id;
    fbe_u8_t drive_slot = ((led_patterns_test_io_completion_context_t *)context)->drive_slot;
    ses_stat_elem_array_dev_slot_struct array_dev_slot_stat_expected = 
        ((led_patterns_test_io_completion_context_t *)context)->array_dev_slot_stat;
    ses_stat_elem_encl_struct encl_stat_expected = 
        ((led_patterns_test_io_completion_context_t *)context)->encl_stat;
    ses_stat_elem_encl_struct chassis_encl_stat_expected = 
        ((led_patterns_test_io_completion_context_t *)context)->chassis_encl_stat;

    io_completion_call_count++;

    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    MUT_ASSERT_TRUE(payload_cdb_operation != NULL);   
    fbe_payload_cdb_operation_get_cdb(payload_cdb_operation, &cdb); 
    status = fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    if (cdb[0] == FBE_SCSI_RECEIVE_DIAGNOSTIC)
    {   
        return_page_code = (fbe_u8_t *)fbe_sg_element_address(sg_list);
        if(*return_page_code == SES_PG_CODE_ENCL_STAT)
        {
            encl_status_diag_page_hdr = (ses_pg_encl_stat_struct *)fbe_sg_element_address(sg_list); 
            array_dev_slot_stat_rtn_ptr = (ses_stat_elem_array_dev_slot_struct *) (((fbe_u8_t *)encl_status_diag_page_hdr) + ARRAY_DEVICE_SLOT_ELEM_OFFSET);
            array_dev_slot_stat_rtn_ptr++;
            array_dev_slot_stat_rtn_ptr += drive_slot;
            MUT_ASSERT_INT_EQUAL(array_dev_slot_stat_expected.ok, array_dev_slot_stat_rtn_ptr->ok);
            MUT_ASSERT_INT_EQUAL(array_dev_slot_stat_expected.ident, array_dev_slot_stat_rtn_ptr->ident);
            MUT_ASSERT_INT_EQUAL(array_dev_slot_stat_expected.fault_requested, array_dev_slot_stat_rtn_ptr->fault_requested);
            MUT_ASSERT_INT_EQUAL(array_dev_slot_stat_expected.rmv, array_dev_slot_stat_rtn_ptr->rmv);

            encl_stat_rtn_ptr = (ses_stat_elem_encl_struct *) (((fbe_u8_t *)encl_status_diag_page_hdr) + LOCAL_LCC_ENCL_STAT_ELEM_OFFSET);
            encl_stat_rtn_ptr ++;
            MUT_ASSERT_INT_EQUAL(encl_stat_expected.failure_indicaton, encl_stat_rtn_ptr->failure_indicaton);
            MUT_ASSERT_INT_EQUAL(encl_stat_expected.failure_rqsted, encl_stat_rtn_ptr->failure_rqsted);
            MUT_ASSERT_INT_EQUAL(encl_stat_expected.warning_indicaton, encl_stat_rtn_ptr->warning_indicaton);
            MUT_ASSERT_INT_EQUAL(encl_stat_expected.warning_rqsted, encl_stat_rtn_ptr->warning_rqsted);
            MUT_ASSERT_INT_EQUAL(encl_stat_expected.ident, encl_stat_rtn_ptr->ident);

            chassis_encl_stat_rtn_ptr = (ses_stat_elem_encl_struct *) (((fbe_u8_t *)encl_status_diag_page_hdr) + CHASSIS_ENCL_STAT_ELEM_OFFSET);
            chassis_encl_stat_rtn_ptr ++;
            MUT_ASSERT_INT_EQUAL(chassis_encl_stat_expected.failure_indicaton, chassis_encl_stat_rtn_ptr->failure_indicaton);
            MUT_ASSERT_INT_EQUAL(chassis_encl_stat_expected.failure_rqsted, chassis_encl_stat_rtn_ptr->failure_rqsted);
            MUT_ASSERT_INT_EQUAL(chassis_encl_stat_expected.warning_indicaton, chassis_encl_stat_rtn_ptr->warning_indicaton);
            MUT_ASSERT_INT_EQUAL(chassis_encl_stat_expected.warning_rqsted, chassis_encl_stat_rtn_ptr->warning_rqsted);
            MUT_ASSERT_INT_EQUAL(chassis_encl_stat_expected.ident, chassis_encl_stat_rtn_ptr->ident);
        }
    }

    fbe_semaphore_release(&led_patterns_test_continue_semaphore, 0, 1, FALSE);
    fbe_ldo_test_free_memory_with_sg(&sg_list);
    return(FBE_STATUS_OK);
}




