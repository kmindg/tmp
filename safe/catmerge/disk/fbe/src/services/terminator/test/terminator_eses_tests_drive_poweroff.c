/***************************************************************************
 *  terminator_eses_tests_drive_poweroff.c
 ***************************************************************************
 *
 *  Description
 *      ESES enclosure tests related to DRIVE Poweroff.
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

#define SLEEP_TIME 4000

/**********************************/
/*        local variables        */
/**********************************/
typedef struct drive_poweroff_test_io_completion_context_s
{
    fbe_u8_t encl_id;
    fbe_u8_t drive_slot;
    ses_stat_elem_array_dev_slot_struct array_dev_slot_stat;
    ses_stat_elem_exp_phy_struct exp_phy_stat;
}drive_poweroff_test_io_completion_context_t;

static drive_poweroff_test_io_completion_context_t drive_poweroff_test_io_completion_context;
static fbe_semaphore_t   drive_poweroff_test_continue_semaphore;
static fbe_u32_t drive_poweroff_test_io_completion_call_count;
static fbe_u32_t drive_poweroff_login_count;
static fbe_u32_t drive_poweroff_logout_count;

/**********************************/
/*       EXTERNAL VARIABLES       */
/**********************************/
extern const fbe_u8_t drive_to_phy_map[MAX_SLOT_NUMBER];

/**********************************/
/*       LOCAL FUNCTIONS          */
/**********************************/
static fbe_status_t terminator_eses_tests_drive_poweroff_io_completion (fbe_payload_ex_t * payload, fbe_payload_ex_completion_context_t context);
static fbe_status_t terminator_eses_tests_drive_poweroff_callback(fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context);
static void init_drive_poweroff_test_io_completion_context(void);
void drive_poweroff_test_build_ctrl_page(fbe_sg_element_t *sg_p);
void drive_poweroff_test_build_array_dev_slot_ctrl_elem(fbe_sg_element_t *sg_p,
                                                     fbe_u8_t slot_num,
                                                     ses_ctrl_elem_array_dev_slot_struct array_dev_slot_ctrl_elem_to_fill);

void drive_poweroff_test_build_exp_phy_ctrl_elem(fbe_sg_element_t *sg_p,
                                                fbe_u8_t phy_id,
                                                ses_ctrl_elem_exp_phy_struct exp_phy_ctrl_elem_to_fill);

void init_drive_poweroff_test_io_completion_context(void)
{
    memset(&drive_poweroff_test_io_completion_context, 0, sizeof(drive_poweroff_test_io_completion_context_t));
    drive_poweroff_test_io_completion_context.exp_phy_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    drive_poweroff_test_io_completion_context.array_dev_slot_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_NOT_INSTALLED;
}

static fbe_status_t terminator_eses_tests_drive_poweroff_callback(fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context)
{
    if((callback_info->callback_type == FBE_PMC_SHIM_CALLBACK_TYPE_LOGOUT) &&
       (callback_info->info.login.device_type == FBE_PMC_SHIM_DEVICE_TYPE_SSP))
    {
        drive_poweroff_logout_count++;
    }
    else if((callback_info->callback_type == FBE_PMC_SHIM_CALLBACK_TYPE_LOGIN) &&
            (callback_info->info.login.device_type == FBE_PMC_SHIM_DEVICE_TYPE_SSP))
    {
        drive_poweroff_login_count++;
    }

    return FBE_STATUS_OK;
}


void terminator_drive_poweroff_test(void)
{
    fbe_status_t                    status;
    fbe_terminator_board_info_t     board_info;
    fbe_terminator_sas_port_info_t  sas_port;
    fbe_terminator_sas_encl_info_t  sas_encl;
    fbe_terminator_sas_drive_info_t sas_drive;
    fbe_terminator_api_device_handle_t        encl_id;
    fbe_terminator_api_device_handle_t        drive_id0, drive_id1;
    fbe_sg_element_t *sg_p = NULL;
    fbe_u32_t  port_index;
    ses_ctrl_elem_array_dev_slot_struct array_dev_slot_ctrl_elem;
    ses_stat_elem_exp_phy_struct exp_phy_stat_elem;
    fbe_u8_t subencl_id = 4;
    fbe_u8_t expected_drive_login_count = 0, expected_drive_logout_count = 0;

    mut_printf(MUT_LOG_HIGH, "%s %s ",__FILE__, __FUNCTION__);

    fbe_semaphore_init(&drive_poweroff_test_continue_semaphore, 0, FBE_SEMAPHORE_MAX);
    init_drive_poweroff_test_io_completion_context();

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
                                                           terminator_eses_tests_drive_poweroff_callback, 
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


    /*insert a sas_drive to the second last enclosure*/
    // inserted in encl id 4, in slot 0.
    sas_drive.sas_address = (fbe_u64_t)0x987654321;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.capacity = 0x10BD0;
    sas_drive.block_size = 520;
    status = fbe_terminator_api_insert_sas_drive (0, encl_id, 0, &sas_drive, &drive_id0);/*add drive to port 0 encl 3*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    expected_drive_login_count++;

    // Wait some time so that the login due to drive insertion
    // does not interfere with the logins generated by the
    // successive test cases.
    EmcutilSleep(SLEEP_TIME);

    // Register the IO completion function
	status = fbe_terminator_miniport_api_register_payload_completion(0,
																    terminator_eses_tests_drive_poweroff_io_completion,
																    &drive_poweroff_test_io_completion_context);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 


    // set the force disabled bit for the phy corresponding to drive slot 0
    // So that the phy ctrl element processing will not interfere with our
    // drive power off logins & logout tests.
    status = fbe_terminator_api_get_sas_enclosure_phy_eses_status(0, 4, drive_to_phy_map[0], &exp_phy_stat_elem);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    exp_phy_stat_elem.force_disabled = 1;
    status = fbe_terminator_api_set_sas_enclosure_phy_eses_status(0, 4, drive_to_phy_map[0], exp_phy_stat_elem);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);    


    // Test 1. power off the drive in slot 0. logout should be generated.
    memset(&array_dev_slot_ctrl_elem, 0, sizeof(ses_ctrl_elem_array_dev_slot_struct));
    array_dev_slot_ctrl_elem.dev_off = 1;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    drive_poweroff_test_build_ctrl_page(sg_p);
    drive_poweroff_test_build_array_dev_slot_ctrl_elem(sg_p, 0, array_dev_slot_ctrl_elem);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_SEND_DIAGNOSTIC, &ses_pg_code_encl_ctrl,  sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&drive_poweroff_test_continue_semaphore, NULL);
    memset(&drive_poweroff_test_io_completion_context, 0, sizeof(drive_poweroff_test_io_completion_context_t));
    drive_poweroff_test_io_completion_context.encl_id = 4;
    drive_poweroff_test_io_completion_context.drive_slot = 0;
    drive_poweroff_test_io_completion_context.array_dev_slot_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    drive_poweroff_test_io_completion_context.array_dev_slot_stat.dev_off = 1; 
    drive_poweroff_test_io_completion_context.exp_phy_stat.phy_rdy = 0;
    drive_poweroff_test_io_completion_context.exp_phy_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&drive_poweroff_test_continue_semaphore, NULL);
    expected_drive_logout_count++;

    // Test 2. power off the drive in slot 0 again. logout should be NOT be generated.
    memset(&array_dev_slot_ctrl_elem, 0, sizeof(ses_ctrl_elem_array_dev_slot_struct));
    array_dev_slot_ctrl_elem.dev_off = 1;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    drive_poweroff_test_build_ctrl_page(sg_p);
    drive_poweroff_test_build_array_dev_slot_ctrl_elem(sg_p, 0, array_dev_slot_ctrl_elem);
    // set the disable bit in the phy, so that p
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_SEND_DIAGNOSTIC, &ses_pg_code_encl_ctrl,  sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&drive_poweroff_test_continue_semaphore, NULL);
    memset(&drive_poweroff_test_io_completion_context, 0, sizeof(drive_poweroff_test_io_completion_context_t));
    drive_poweroff_test_io_completion_context.encl_id = 4;
    drive_poweroff_test_io_completion_context.drive_slot = 0;
    drive_poweroff_test_io_completion_context.array_dev_slot_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    drive_poweroff_test_io_completion_context.array_dev_slot_stat.dev_off = 1; 
    drive_poweroff_test_io_completion_context.exp_phy_stat.phy_rdy = 0;
    drive_poweroff_test_io_completion_context.exp_phy_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&drive_poweroff_test_continue_semaphore, NULL);

    // Test 3. power ON the drive in slot 0. login should be generated.
    memset(&array_dev_slot_ctrl_elem, 0, sizeof(ses_ctrl_elem_array_dev_slot_struct));
    array_dev_slot_ctrl_elem.dev_off = 0;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    drive_poweroff_test_build_ctrl_page(sg_p);
    drive_poweroff_test_build_array_dev_slot_ctrl_elem(sg_p, 0, array_dev_slot_ctrl_elem);
    // set the disable bit in the phy, so that p
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_SEND_DIAGNOSTIC, &ses_pg_code_encl_ctrl,  sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&drive_poweroff_test_continue_semaphore, NULL);
    memset(&drive_poweroff_test_io_completion_context, 0, sizeof(drive_poweroff_test_io_completion_context_t));
    drive_poweroff_test_io_completion_context.encl_id = 4;
    drive_poweroff_test_io_completion_context.drive_slot = 0;
    drive_poweroff_test_io_completion_context.array_dev_slot_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    drive_poweroff_test_io_completion_context.array_dev_slot_stat.dev_off = 0; 
    drive_poweroff_test_io_completion_context.exp_phy_stat.phy_rdy = 1;
    drive_poweroff_test_io_completion_context.exp_phy_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&drive_poweroff_test_continue_semaphore, NULL);
    expected_drive_login_count++;

    // Test 4. power ON the drive in slot 0. login should NOT be generated.
    memset(&array_dev_slot_ctrl_elem, 0, sizeof(ses_ctrl_elem_array_dev_slot_struct));
    array_dev_slot_ctrl_elem.dev_off = 0;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    drive_poweroff_test_build_ctrl_page(sg_p);
    drive_poweroff_test_build_array_dev_slot_ctrl_elem(sg_p, 0, array_dev_slot_ctrl_elem);
    // set the disable bit in the phy, so that p
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_SEND_DIAGNOSTIC, &ses_pg_code_encl_ctrl,  sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&drive_poweroff_test_continue_semaphore, NULL);
    memset(&drive_poweroff_test_io_completion_context, 0, sizeof(drive_poweroff_test_io_completion_context_t));
    drive_poweroff_test_io_completion_context.encl_id = 4;
    drive_poweroff_test_io_completion_context.drive_slot = 0;
    drive_poweroff_test_io_completion_context.array_dev_slot_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    drive_poweroff_test_io_completion_context.array_dev_slot_stat.dev_off = 0; 
    drive_poweroff_test_io_completion_context.exp_phy_stat.phy_rdy = 1;
    drive_poweroff_test_io_completion_context.exp_phy_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&drive_poweroff_test_continue_semaphore, NULL);

    // test 5. poweroff an empty drive slot(1) and then insert a drive in it. 
    // Login should not be generated
    memset(&array_dev_slot_ctrl_elem, 0, sizeof(ses_ctrl_elem_array_dev_slot_struct));
    array_dev_slot_ctrl_elem.dev_off = 1;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    drive_poweroff_test_build_ctrl_page(sg_p);
    drive_poweroff_test_build_array_dev_slot_ctrl_elem(sg_p, 1, array_dev_slot_ctrl_elem);
    // set the disable bit in the phy, so that p
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_SEND_DIAGNOSTIC, &ses_pg_code_encl_ctrl,  sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&drive_poweroff_test_continue_semaphore, NULL);
    memset(&drive_poweroff_test_io_completion_context, 0, sizeof(drive_poweroff_test_io_completion_context_t));
    drive_poweroff_test_io_completion_context.encl_id = 4;
    drive_poweroff_test_io_completion_context.drive_slot = 1;
    drive_poweroff_test_io_completion_context.array_dev_slot_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_NOT_INSTALLED;
    drive_poweroff_test_io_completion_context.array_dev_slot_stat.dev_off = 1; 
    drive_poweroff_test_io_completion_context.exp_phy_stat.phy_rdy = 0;
    drive_poweroff_test_io_completion_context.exp_phy_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&drive_poweroff_test_continue_semaphore, NULL);

    /*insert a sas_drive to the second last enclosure*/
    // inserted in encl id 4, in slot 1.
    sas_drive.sas_address = (fbe_u64_t)0x987654321;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.capacity = 0x10BD0;
    sas_drive.block_size = 520;
    status = fbe_terminator_api_insert_sas_drive (0, encl_id, 1, &sas_drive, &drive_id1);/*add drive to port 0 encl 3*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    memset(&drive_poweroff_test_io_completion_context, 0, sizeof(drive_poweroff_test_io_completion_context_t));
    drive_poweroff_test_io_completion_context.encl_id = 4;
    drive_poweroff_test_io_completion_context.drive_slot = 1;
    drive_poweroff_test_io_completion_context.array_dev_slot_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    drive_poweroff_test_io_completion_context.array_dev_slot_stat.dev_off = 1; 
    drive_poweroff_test_io_completion_context.exp_phy_stat.phy_rdy = 0;
    drive_poweroff_test_io_completion_context.exp_phy_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&drive_poweroff_test_continue_semaphore, NULL);

    EmcutilSleep(SLEEP_TIME);
    MUT_ASSERT_INT_EQUAL(expected_drive_login_count, drive_poweroff_login_count);
    MUT_ASSERT_INT_EQUAL(expected_drive_logout_count, drive_poweroff_logout_count);

    // test 6. powerON the drive slot 1, that has a drive inserted.
    // Login SHOULD be generated
    memset(&array_dev_slot_ctrl_elem, 0, sizeof(ses_ctrl_elem_array_dev_slot_struct));
    array_dev_slot_ctrl_elem.dev_off = 0;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    drive_poweroff_test_build_ctrl_page(sg_p);
    drive_poweroff_test_build_array_dev_slot_ctrl_elem(sg_p, 1, array_dev_slot_ctrl_elem);
    // set the disable bit in the phy, so that p
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_SEND_DIAGNOSTIC, &ses_pg_code_encl_ctrl,  sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&drive_poweroff_test_continue_semaphore, NULL);
    memset(&drive_poweroff_test_io_completion_context, 0, sizeof(drive_poweroff_test_io_completion_context_t));
    drive_poweroff_test_io_completion_context.encl_id = 4;
    drive_poweroff_test_io_completion_context.drive_slot = 1;
    drive_poweroff_test_io_completion_context.array_dev_slot_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    drive_poweroff_test_io_completion_context.array_dev_slot_stat.dev_off = 0; 
    drive_poweroff_test_io_completion_context.exp_phy_stat.phy_rdy = 1;
    drive_poweroff_test_io_completion_context.exp_phy_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&drive_poweroff_test_continue_semaphore, NULL);
    expected_drive_login_count++;

    // Test 7. power off the drive in slot 0. logout should be generated.
    memset(&array_dev_slot_ctrl_elem, 0, sizeof(ses_ctrl_elem_array_dev_slot_struct));
    array_dev_slot_ctrl_elem.dev_off = 1;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    drive_poweroff_test_build_ctrl_page(sg_p);
    drive_poweroff_test_build_array_dev_slot_ctrl_elem(sg_p, 0, array_dev_slot_ctrl_elem);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_SEND_DIAGNOSTIC, &ses_pg_code_encl_ctrl,  sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&drive_poweroff_test_continue_semaphore, NULL);
    memset(&drive_poweroff_test_io_completion_context, 0, sizeof(drive_poweroff_test_io_completion_context_t));
    drive_poweroff_test_io_completion_context.encl_id = 4;
    drive_poweroff_test_io_completion_context.drive_slot = 0;
    drive_poweroff_test_io_completion_context.array_dev_slot_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    drive_poweroff_test_io_completion_context.array_dev_slot_stat.dev_off = 1; 
    drive_poweroff_test_io_completion_context.exp_phy_stat.phy_rdy = 0;
    drive_poweroff_test_io_completion_context.exp_phy_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&drive_poweroff_test_continue_semaphore, NULL);
    expected_drive_logout_count++;

    EmcutilSleep(SLEEP_TIME);

    MUT_ASSERT_INT_EQUAL(expected_drive_login_count, drive_poweroff_login_count);
    MUT_ASSERT_INT_EQUAL(expected_drive_logout_count, drive_poweroff_logout_count);

    //test7. remove drive 0 in the powered off slot. 
    // logout should NOT be generated.
    status = fbe_terminator_api_remove_sas_drive(0, drive_id0);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    memset(&drive_poweroff_test_io_completion_context, 0, sizeof(drive_poweroff_test_io_completion_context_t));
    drive_poweroff_test_io_completion_context.encl_id = 4;
    drive_poweroff_test_io_completion_context.drive_slot = 0;
    drive_poweroff_test_io_completion_context.array_dev_slot_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_NOT_INSTALLED;
    drive_poweroff_test_io_completion_context.array_dev_slot_stat.dev_off = 1; 
    drive_poweroff_test_io_completion_context.exp_phy_stat.phy_rdy = 0;
    drive_poweroff_test_io_completion_context.exp_phy_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&drive_poweroff_test_continue_semaphore, NULL);

    EmcutilSleep(SLEEP_TIME);

    MUT_ASSERT_INT_EQUAL(expected_drive_login_count, drive_poweroff_login_count);
    MUT_ASSERT_INT_EQUAL(expected_drive_logout_count, drive_poweroff_logout_count);

    // Clear the force disable bit we have set at the beginning
    status = fbe_terminator_api_get_sas_enclosure_phy_eses_status(0, 4, drive_to_phy_map[0], &exp_phy_stat_elem);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    exp_phy_stat_elem.force_disabled = 0;
    status = fbe_terminator_api_set_sas_enclosure_phy_eses_status(0, 4, drive_to_phy_map[0], exp_phy_stat_elem);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 
   
    status = fbe_terminator_api_destroy();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator destroyed.", __FUNCTION__);
}


void drive_poweroff_test_build_array_dev_slot_ctrl_elem(fbe_sg_element_t *sg_p,
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


void drive_poweroff_test_build_ctrl_page(fbe_sg_element_t *sg_p)
{
    ses_pg_encl_ctrl_struct *encl_ctrl_page_hdr = NULL;

    encl_ctrl_page_hdr = (ses_pg_encl_ctrl_struct *)fbe_sg_element_address(sg_p);
    encl_ctrl_page_hdr->pg_code =  SES_PG_CODE_ENCL_CTRL;
    encl_ctrl_page_hdr->pg_len = BYTE_SWAP_16(STAT_PAGE_LENGTH);
}


static fbe_status_t terminator_eses_tests_drive_poweroff_io_completion (fbe_payload_ex_t * payload, fbe_payload_ex_completion_context_t context)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_cdb_operation_t *payload_cdb_operation;
    fbe_u8_t *cdb = NULL;
    fbe_sg_element_t *sg_list = NULL;
    fbe_u8_t *return_page_code;
    ses_pg_encl_stat_struct  *encl_status_diag_page_hdr = NULL;
    ses_stat_elem_array_dev_slot_struct *array_dev_slot_stat_rtn_ptr = NULL;
    ses_stat_elem_exp_phy_struct *exp_phy_stat_rtn_ptr = NULL;
    // As all these tests are only on port 0.
    fbe_u8_t encl_id = ((drive_poweroff_test_io_completion_context_t *)context)->encl_id;
    fbe_u8_t drive_slot = ((drive_poweroff_test_io_completion_context_t *)context)->drive_slot;
    ses_stat_elem_array_dev_slot_struct array_dev_slot_stat_expected;
    ses_stat_elem_exp_phy_struct exp_phy_stat_expected;

    memcpy(&array_dev_slot_stat_expected, 
           &((drive_poweroff_test_io_completion_context_t *)context)->array_dev_slot_stat,
           sizeof(ses_stat_elem_array_dev_slot_struct));
    memcpy(&exp_phy_stat_expected,
           &((drive_poweroff_test_io_completion_context_t *)context)->exp_phy_stat,
           sizeof(ses_stat_elem_exp_phy_struct));

    drive_poweroff_test_io_completion_call_count++;

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
            MUT_ASSERT_INT_EQUAL(array_dev_slot_stat_expected.dev_off, array_dev_slot_stat_rtn_ptr->dev_off);
            MUT_ASSERT_INT_EQUAL(array_dev_slot_stat_expected.cmn_stat.elem_stat_code, array_dev_slot_stat_rtn_ptr->cmn_stat.elem_stat_code);

            exp_phy_stat_rtn_ptr = (ses_stat_elem_exp_phy_struct *) (((fbe_u8_t *)encl_status_diag_page_hdr) + PHY_ELEM_OFFSET);
            exp_phy_stat_rtn_ptr ++;
            exp_phy_stat_rtn_ptr += drive_to_phy_map[drive_slot];
            MUT_ASSERT_INT_EQUAL(exp_phy_stat_expected.phy_rdy, exp_phy_stat_rtn_ptr->phy_rdy);
            MUT_ASSERT_INT_EQUAL(exp_phy_stat_expected.cmn_stat.elem_stat_code, exp_phy_stat_rtn_ptr->cmn_stat.elem_stat_code);;

        }
    }

    fbe_semaphore_release(&drive_poweroff_test_continue_semaphore, 0, 1, FALSE);
    fbe_ldo_test_free_memory_with_sg(&sg_list);
    return(FBE_STATUS_OK);
}




