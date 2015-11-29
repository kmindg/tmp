/***************************************************************************
 *  terminator_eses_tests_send_diag.c
 ***************************************************************************
 *
 *  Description
 *      ESES enclosure "send diagnostic CDB" related tests
 *      
 *
 *  History:
 *     Nov-21-08   Rajesh V. Created
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
/*       LOCAL VARIABLES          */
/**********************************/
static fbe_u32_t login_count;
static fbe_u32_t logout_count;
static eses_test_io_completion_context_t device_info;
static fbe_semaphore_t   test_continue_semaphore;



/**********************************/
/*       LOCAL FUNCTIONS          */
/**********************************/
static fbe_status_t terminator_eses_tests_drive_bypass_io_completion (fbe_payload_ex_t * payload, fbe_payload_ex_completion_context_t context);
static fbe_status_t terminator_eses_tests_drive_bypass_callback(fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context);
static void terminator_eses_tests_drive_bypass_build_ctrl_page(fbe_sg_element_t *sg_p,
                                                               fbe_u8_t slot_num,
                                                               fbe_bool_t bypass);

/**********************************/
/*       EXTERNAL VARIABLES       */
/**********************************/
extern const fbe_u8_t drive_to_phy_map[MAX_SLOT_NUMBER];
extern ses_stat_elem_array_dev_slot_struct array_dev_slot_stat_ptr[10][MAX_SLOT_NUMBER];
extern ses_stat_elem_exp_phy_struct exp_phy_stat_ptr[10][MAX_PHY_NUMBER];


void terminator_control_page_drive_bypass_test(void)
{
    fbe_status_t                    status;
    fbe_terminator_board_info_t     board_info;
    fbe_terminator_sas_port_info_t  sas_port;
    fbe_terminator_sas_encl_info_t  sas_encl;
    fbe_terminator_sas_drive_info_t sas_drive;
    fbe_terminator_api_device_handle_t        encl_id;
    fbe_terminator_api_device_handle_t        drive_id, drive_id1;
    fbe_sg_element_t *sg_p = NULL;
    fbe_u32_t  port_index;

    mut_printf(MUT_LOG_HIGH, "%s %s ",__FILE__, __FUNCTION__);

    fbe_semaphore_init(&test_continue_semaphore, 0, FBE_SEMAPHORE_MAX);
    init_dev_phy_elems();

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
                                                           terminator_eses_tests_drive_bypass_callback, 
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
    status = fbe_terminator_api_insert_sas_drive (0, encl_id, 0, &sas_drive, &drive_id);/*add drive to port 0 encl 3*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // for testing that the phy/drive status got updated correctly.
    memset(&exp_phy_stat_ptr[4][drive_to_phy_map[0]], 0, sizeof(ses_stat_elem_exp_phy_struct));
    array_dev_slot_stat_ptr[4][0].cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    exp_phy_stat_ptr[4][drive_to_phy_map[0]].phy_rdy = 1;

    /*insert a sas_drive to the second last enclosure*/
    // inserted in encl id 4, slot 1
    sas_drive.sas_address = (fbe_u64_t)0x987654321;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.capacity = 0x10BD0;
    sas_drive.block_size = 520;
    status = fbe_terminator_api_insert_sas_drive (0, encl_id, 1, &sas_drive, &drive_id1);/*add drive to port 0 encl 3*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // for testing that the phy/drive status got updated correctly.
    memset(&exp_phy_stat_ptr[4][drive_to_phy_map[1]], 0, sizeof(ses_stat_elem_exp_phy_struct));
    array_dev_slot_stat_ptr[4][1].cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    exp_phy_stat_ptr[4][drive_to_phy_map[1]].phy_rdy = 1;


	/* now lets verify the status */
	status = fbe_terminator_miniport_api_register_payload_completion(0,
																    terminator_eses_tests_drive_bypass_io_completion,
																    &device_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    // Wait for some time till all the logins due to device 
    // insertions are done.
    EmcutilSleep(SLEEP_TIME);

    login_count = 0;
    logout_count = 0;
    

    //test 1. Bypass drive 0 in encl id 4. logout should be generated.
    device_info.port = 0;
    device_info.encl_id = 4;
    device_info.phy = drive_to_phy_map[0];
    // ask to bypass the drive0
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    terminator_eses_tests_drive_bypass_build_ctrl_page(sg_p, 0, 1);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_SEND_DIAGNOSTIC, &ses_pg_code_encl_ctrl, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&test_continue_semaphore, NULL);
    // for testing that the phy/drive status got updated correctly.
    memset(&exp_phy_stat_ptr[4][drive_to_phy_map[0]], 0, sizeof(ses_stat_elem_exp_phy_struct));
    exp_phy_stat_ptr[4][drive_to_phy_map[0]].cmn_stat.elem_stat_code = SES_STAT_CODE_UNAVAILABLE;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	fbe_semaphore_wait(&test_continue_semaphore, NULL);
   


    /*
    // test 2. bypass drive0 again. It should not generate a logout
    // as the drive is already loggedout.
    device_info.port = 0;
    device_info.encl_id = 4;
    // ask to bypass the drive.
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    terminator_eses_tests_drive_bypass_build_ctrl_page(sg_p, 0, 1); 
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_SEND_DIAGNOSTIC, &ses_pg_code_encl_ctrl, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    */


    // Test 3. Unbypass the drive 0 again. login should be generated.
    device_info.port = 0;
    device_info.encl_id = 4;
    // ask to Unbypass the drive.
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    terminator_eses_tests_drive_bypass_build_ctrl_page(sg_p, 0, 0); 
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_SEND_DIAGNOSTIC, &ses_pg_code_encl_ctrl,  sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&test_continue_semaphore, NULL);
    // for testing that the phy/drive status got updated correctly.
    memset(&exp_phy_stat_ptr[4][drive_to_phy_map[0]], 0, sizeof(ses_stat_elem_exp_phy_struct));
    exp_phy_stat_ptr[4][drive_to_phy_map[0]].cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    exp_phy_stat_ptr[4][drive_to_phy_map[0]].phy_rdy = 0x1;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	fbe_semaphore_wait(&test_continue_semaphore, NULL);

    //wait some time
    EmcutilSleep(SLEEP_TIME/2);
    MUT_ASSERT_INT_EQUAL(1, logout_count);
    MUT_ASSERT_INT_EQUAL(1, login_count);  
    login_count =0;
    logout_count = 0;

    // Test 4.
    // Bypass the drive0 using API. This will force bypass the drive
    // Once a drive is force bypassed, ctrl page bypass/unbypass 
    // commands are ignored. The drive can only be unbypassed using
    // the API again. 
    status = fbe_terminator_api_enclosure_bypass_drive_slot(0, drive_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_terminator_api_force_logout_device(0, drive_id);
    // for testing that the phy/drive status got updated correctly.
    memset(&array_dev_slot_stat_ptr[4][0], 0, sizeof(ses_stat_elem_array_dev_slot_struct));
    memset(&exp_phy_stat_ptr[4][drive_to_phy_map[0]], 0, sizeof(ses_stat_elem_exp_phy_struct));
    array_dev_slot_stat_ptr[4][0].cmn_stat.elem_stat_code = SES_STAT_CODE_CRITICAL;
    exp_phy_stat_ptr[4][drive_to_phy_map[0]].cmn_stat.elem_stat_code = SES_STAT_CODE_CRITICAL;
    exp_phy_stat_ptr[4][drive_to_phy_map[0]].force_disabled = 0x1;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	fbe_semaphore_wait(&test_continue_semaphore, NULL);

    // Issue ctrl page bypass/unbypass. These should have no effect
    // as the drive is force bypassed.

    // test 5. unbypass drive0 using ctrl page. 
    // This should not generate a login nor change
    // phy status.
    device_info.port = 0;
    device_info.encl_id = 4;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    terminator_eses_tests_drive_bypass_build_ctrl_page(sg_p, 0, 0);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_SEND_DIAGNOSTIC, &ses_pg_code_encl_ctrl, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&test_continue_semaphore, NULL);

    // test 6. bypass drive0 using ctrl page. 
    // This should not have any effect and
    // NOT generate a logout.
    device_info.port = 0;
    device_info.encl_id = 4;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    terminator_eses_tests_drive_bypass_build_ctrl_page(sg_p, 0, 1);
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_SEND_DIAGNOSTIC, &ses_pg_code_encl_ctrl, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&test_continue_semaphore, NULL);

    //wait some time
    EmcutilSleep(SLEEP_TIME/2);
    MUT_ASSERT_INT_EQUAL(0, login_count);
    MUT_ASSERT_INT_EQUAL(1, logout_count);   
    login_count =0;
    logout_count = 0;

    // Unbypass the drive using API. This will force Unbypass the drive 
    status = fbe_terminator_api_enclosure_unbypass_drive_slot(0, drive_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // force login the drive
    fbe_terminator_api_force_login_device (0, drive_id);
    // for testing that the phy/drive status got updated correctly.
    memset(&array_dev_slot_stat_ptr[4][0], 0, sizeof(ses_stat_elem_array_dev_slot_struct));
    memset(&exp_phy_stat_ptr[4][drive_to_phy_map[0]], 0, sizeof(ses_stat_elem_exp_phy_struct));
    array_dev_slot_stat_ptr[4][0].cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    exp_phy_stat_ptr[4][drive_to_phy_map[0]].cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    exp_phy_stat_ptr[4][drive_to_phy_map[0]].phy_rdy = 1;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	fbe_semaphore_wait(&test_continue_semaphore, NULL);


    //Now the control page bypass and unbypass should work again.

    // Test 7.
    // Unbypass drive0 again. It should not generate a login
    // as the phy status code is OK.
    device_info.port = 0;
    device_info.encl_id = 4;
    // ask to Unbypass the drive.
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    terminator_eses_tests_drive_bypass_build_ctrl_page(sg_p, 0, 0);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_SEND_DIAGNOSTIC, &ses_pg_code_encl_ctrl, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&test_continue_semaphore, NULL);

    // Test 8. 
    // bypass the drive 0 again. logout should be generated.
    device_info.port = 0;
    device_info.encl_id = 4;
    // ask to bypass the drive.
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    terminator_eses_tests_drive_bypass_build_ctrl_page(sg_p, 0, 1);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_SEND_DIAGNOSTIC, &ses_pg_code_encl_ctrl, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&test_continue_semaphore, NULL);
    // for testing that the phy/drive status got updated correctly.
    memset(&exp_phy_stat_ptr[4][drive_to_phy_map[0]], 0, sizeof(ses_stat_elem_exp_phy_struct));
    exp_phy_stat_ptr[4][drive_to_phy_map[0]].cmn_stat.elem_stat_code = SES_STAT_CODE_UNAVAILABLE;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	fbe_semaphore_wait(&test_continue_semaphore, NULL);

    // Test 9. 
    // Unbypass the drive 0 again. login should be generated.
    device_info.port = 0;
    device_info.encl_id = 4;
    // ask to bypass the drive.
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    terminator_eses_tests_drive_bypass_build_ctrl_page(sg_p, 0, 0);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_SEND_DIAGNOSTIC, &ses_pg_code_encl_ctrl, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&test_continue_semaphore, NULL);
    // for testing that the phy/drive status got updated correctly.
    memset(&exp_phy_stat_ptr[4][drive_to_phy_map[0]], 0, sizeof(ses_stat_elem_exp_phy_struct));
    exp_phy_stat_ptr[4][drive_to_phy_map[0]].cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    exp_phy_stat_ptr[4][drive_to_phy_map[0]].phy_rdy = 0x1;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	fbe_semaphore_wait(&test_continue_semaphore, NULL);


    EmcutilSleep(SLEEP_TIME/2);
    MUT_ASSERT_INT_EQUAL(2, login_count);
    MUT_ASSERT_INT_EQUAL(1, logout_count);   
    login_count =0;
    logout_count = 0;


    //test 10. Bypass drive 1 in encl id 4. logout should be generated.
    device_info.port = 0;
    device_info.encl_id = 4;
    device_info.phy = drive_to_phy_map[1];
    // ask to bypass the drive0
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    terminator_eses_tests_drive_bypass_build_ctrl_page(sg_p, 1, 1);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_SEND_DIAGNOSTIC, &ses_pg_code_encl_ctrl, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&test_continue_semaphore, NULL);
    // for testing that the phy/drive status got updated correctly.
    memset(&exp_phy_stat_ptr[4][drive_to_phy_map[1]], 0, sizeof(ses_stat_elem_exp_phy_struct));
    exp_phy_stat_ptr[4][drive_to_phy_map[1]].cmn_stat.elem_stat_code = SES_STAT_CODE_UNAVAILABLE;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	fbe_semaphore_wait(&test_continue_semaphore, NULL);
   


    // Test 11. Unbypass the drive 1 . login should be generated.
    device_info.port = 0;
    device_info.encl_id = 4;
    device_info.phy = drive_to_phy_map[1];
    // ask to Unbypass the drive.
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    terminator_eses_tests_drive_bypass_build_ctrl_page(sg_p, 1, 0); 
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_SEND_DIAGNOSTIC, &ses_pg_code_encl_ctrl,  sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&test_continue_semaphore, NULL);
    // for testing that the phy/drive status got updated correctly.
    memset(&exp_phy_stat_ptr[4][drive_to_phy_map[1]], 0, sizeof(ses_stat_elem_exp_phy_struct));
    exp_phy_stat_ptr[4][drive_to_phy_map[1]].cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    exp_phy_stat_ptr[4][drive_to_phy_map[1]].phy_rdy = 0x1;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	fbe_semaphore_wait(&test_continue_semaphore, NULL);

    //wait some time
    EmcutilSleep(SLEEP_TIME/2);
    MUT_ASSERT_INT_EQUAL(1, logout_count);
    MUT_ASSERT_INT_EQUAL(1, login_count);  
    login_count =0;
    logout_count = 0;

    status = fbe_terminator_api_destroy();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator destroyed.", __FUNCTION__);
}

static fbe_status_t terminator_eses_tests_drive_bypass_io_completion (fbe_payload_ex_t * payload, fbe_payload_ex_completion_context_t context)
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
    fbe_u8_t encl_id = ((eses_test_io_completion_context_t *)context)->encl_id;
    fbe_u8_t phy = ((eses_test_io_completion_context_t *)context)->phy;
    fbe_u32_t i=0;

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

            for(i=0; i<MAX_SLOT_NUMBER; i++)
            {
                array_dev_slot_stat_rtn_ptr++;
                MUT_ASSERT_INT_EQUAL(array_dev_slot_stat_ptr[encl_id][i].cmn_stat.elem_stat_code, array_dev_slot_stat_rtn_ptr->cmn_stat.elem_stat_code);
            }

            exp_phy_stat_rtn_ptr = (ses_stat_elem_exp_phy_struct *) (((fbe_u8_t *)encl_status_diag_page_hdr) + PHY_ELEM_OFFSET);
            exp_phy_stat_rtn_ptr ++;
            exp_phy_stat_rtn_ptr += phy;
            MUT_ASSERT_INT_EQUAL(phy, exp_phy_stat_rtn_ptr->phy_id);
            MUT_ASSERT_INT_EQUAL(exp_phy_stat_ptr[encl_id][phy].cmn_stat.elem_stat_code, exp_phy_stat_rtn_ptr->cmn_stat.elem_stat_code);
            MUT_ASSERT_INT_EQUAL(exp_phy_stat_ptr[encl_id][phy].phy_rdy, exp_phy_stat_rtn_ptr->phy_rdy);
            MUT_ASSERT_INT_EQUAL(exp_phy_stat_ptr[encl_id][phy].link_rdy, exp_phy_stat_rtn_ptr->link_rdy);
            MUT_ASSERT_INT_EQUAL(exp_phy_stat_ptr[encl_id][phy].force_disabled, exp_phy_stat_rtn_ptr->force_disabled);
        }
    }

   
    fbe_semaphore_release(&test_continue_semaphore, 0, 1, FALSE);
    fbe_ldo_test_free_memory_with_sg(&sg_list);
    return(FBE_STATUS_OK);
}

static fbe_status_t terminator_eses_tests_drive_bypass_callback(fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context)
{
    if(callback_info->callback_type == FBE_PMC_SHIM_CALLBACK_TYPE_LOGOUT)
    {
        logout_count ++;
    }
    else if(callback_info->callback_type == FBE_PMC_SHIM_CALLBACK_TYPE_LOGIN)
    {
        login_count++;
    }

    return FBE_STATUS_OK;
}
    
    
static void terminator_eses_tests_drive_bypass_build_ctrl_page(fbe_sg_element_t *sg_p,
                                                               fbe_u8_t slot_num,
                                                               fbe_bool_t bypass)
{
    ses_pg_encl_ctrl_struct *encl_ctrl_page_hdr = NULL;
    ses_ctrl_elem_exp_phy_struct *exp_phy_ctrl_elem;

    encl_ctrl_page_hdr = (ses_pg_encl_ctrl_struct *)fbe_sg_element_address(sg_p);
    encl_ctrl_page_hdr->pg_code =  SES_PG_CODE_ENCL_CTRL;
    encl_ctrl_page_hdr->pg_len = BYTE_SWAP_16(STAT_PAGE_LENGTH);

    exp_phy_ctrl_elem = (ses_ctrl_elem_exp_phy_struct *) (((fbe_u8_t *)encl_ctrl_page_hdr) + PHY_ELEM_OFFSET);
    exp_phy_ctrl_elem++;
    exp_phy_ctrl_elem += drive_to_phy_map[slot_num];
    exp_phy_ctrl_elem->cmn_stat.disable = bypass;  

    // Set the select bit
    exp_phy_ctrl_elem->cmn_stat.select = TRUE;

    return;
}











