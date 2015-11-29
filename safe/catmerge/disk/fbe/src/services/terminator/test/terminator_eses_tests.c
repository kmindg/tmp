#include "terminator_test.h"

#include "fbe/fbe_terminator_api.h"
#include "fbe_terminator_miniport_interface.h"
#include "fbe_terminator.h"

#include "fbe/fbe_eses.h"
#include "fbe_scsi.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_topology_interface.h"
#include "terminator_eses_test.h"

/**********************************/
/*        Global variables        */
/**********************************/
ses_stat_elem_array_dev_slot_struct array_dev_slot_stat_ptr[10][MAX_SLOT_NUMBER];
ses_stat_elem_exp_phy_struct exp_phy_stat_ptr[10][MAX_PHY_NUMBER];

/**********************************/
/*        local variables         */
/**********************************/
static fbe_u32_t fbe_cpd_shim_callback_count;
static fbe_semaphore_t      update_semaphore;
static eses_test_io_completion_context_t device_info;
static fbe_u8_t addl_stat_page_esc_elec_phy_id[24];
static fbe_u8_t addl_stat_page_esc_elec_other_elem_index[24];
static fbe_u64_t addl_elem_sas_exp_sas_addr[10]; //one for each enclosure

// For testing power supply status in stat page
ses_stat_elem_ps_struct ps_stat[10][MAX_PS_NUMBER];
ses_stat_elem_cooling_struct cooling_stat[10][MAX_COOLING_NUMBER];
ses_stat_elem_temp_sensor_struct temp_sensor_stat[10][MAX_TEMP_SENSOR_NUMBER];
ses_stat_elem_temp_sensor_struct overall_temp_sensor_stat[10][MAX_TEMP_SENSOR_NUMBER];

//End of ps status structures

/**********************************/
/*        GLOBAL VARIABLES        */
/**********************************/
//These represent the other and connector element indexes expected of the phys. THis is after using the
//phy to drive/conn mapping for viper. Also started using the configuration page related stuff here.
static fbe_u8_t addl_stat_page_sas_exp_other_elem_index[MAX_PHY_NUMBER] = {43,43,43,43,38,38,38,38,0xFF,48+14,48+13,48+12,48+11,48+10,48+9,48+8,48+7,48+6,48+5,48+4,48+0,48+3,48+1,48+2,0xFF,0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, 0xFF,};
static fbe_u8_t addl_stat_page_sas_exp_conn_elem_index[MAX_PHY_NUMBER] = {44,45,46,47, 39,40,41,42,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, 0xFF};

/**********************************/
/*        GLOBAL VARIABLES        */
/**********************************/
const fbe_u8_t drive_to_phy_map[MAX_SLOT_NUMBER] = {20, 22, 23, 21, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 14};
ses_pg_code_enum ses_pg_code_encl_stat = SES_PG_CODE_ENCL_STAT;
ses_pg_code_enum ses_pg_code_emc_encl_stat = SES_PG_CODE_EMC_ENCL_STAT;
ses_pg_code_enum ses_pg_code_config = SES_PG_CODE_CONFIG;
ses_pg_code_enum ses_pg_code_addl_elem_stat = SES_PG_CODE_ADDL_ELEM_STAT;
ses_pg_code_enum ses_pg_code_download_microcode_stat = SES_PG_CODE_DOWNLOAD_MICROCODE_STAT;
ses_pg_code_enum ses_pg_code_encl_ctrl = SES_PG_CODE_ENCL_CTRL;
ses_pg_code_enum ses_pg_code_emc_encl_ctrl = SES_PG_CODE_EMC_ENCL_CTRL;
ses_pg_code_enum ses_pg_code_str_out = SES_PG_CODE_STR_OUT;


/**********************************/
/*        local functions         */
/**********************************/
static fbe_status_t fbe_cpd_shim_callback_function_mock(fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context)
{
    fbe_cpd_shim_callback_count++;
    return FBE_STATUS_OK;
}
/* forward declaration */

fbe_status_t terminator_miniport_api_test_PHY_io_complete (fbe_payload_ex_t * payload, fbe_payload_ex_completion_context_t context);
fbe_status_t terminator_miniport_api_test_PS_io_complete (fbe_payload_ex_t * payload, fbe_payload_ex_completion_context_t context);
void init_addl_stat_page_strts(void);
void init_ps_elem_strts(void);
void init_cooling_elem_strts(void);
void init_temp_sensor_elem_strts(void);


void terminator_set_enclosure_drive_phy_status_test(void)
{
    fbe_status_t                    status;
    fbe_terminator_board_info_t     board_info;
    fbe_terminator_sas_port_info_t  sas_port;
    fbe_terminator_sas_encl_info_t  sas_encl;
    fbe_terminator_sas_drive_info_t sas_drive;
    fbe_terminator_api_device_handle_t   encl_id;
    fbe_terminator_api_device_handle_t   drive_id;

    fbe_terminator_api_device_handle_t        device_list[MAX_DEVICES];
    fbe_u32_t                       total_devices = 0;
    fbe_cpd_shim_enumerate_backend_ports_t cpd_shim_enumerate_backend_ports;
    fbe_u32_t                       i = 0;
    fbe_u32_t                       context = 0;
    fbe_terminator_device_ptr_t handle;
    fbe_u32_t       port_index;

    ses_stat_elem_array_dev_slot_struct drive_slot_stat;
    ses_stat_elem_exp_phy_struct phy_stat;
    fbe_sg_element_t *sg_p = NULL;

    mut_printf(MUT_LOG_HIGH, "%s %s ",__FILE__, __FUNCTION__);
    fbe_semaphore_init(&update_semaphore, 0, FBE_SEMAPHORE_MAX);
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
    status = fbe_terminator_miniport_api_register_callback(port_index, fbe_cpd_shim_callback_function_mock, NULL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* enumerate the ports */
    status  = fbe_terminator_miniport_api_enumerate_pmc_ports (&cpd_shim_enumerate_backend_ports);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* enumerate the devices on ports */
    terminator_get_port_ptr_by_port_portal(0, 1, &handle);
	status = terminator_enumerate_devices (handle, device_list, &total_devices);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(0, total_devices);

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
    sas_encl.sas_address = (fbe_u64_t)0x123456783;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    sas_encl.connector_id = 0;
    status  = fbe_terminator_api_insert_sas_enclosure (0, &sas_encl, &encl_id);/*add encl on port 0*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* enumerate the devices on ports */
    terminator_get_port_ptr_by_port_portal(0, 1, &handle);
	status = terminator_enumerate_devices (handle, device_list, &total_devices);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(6, total_devices);

    /*insert a sas_drive to the second last enclosure*/
    // inserted in encl id 4
    sas_drive.sas_address = (fbe_u64_t)0x987654321;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.capacity = 0x10BD0;
    sas_drive.block_size = 520;
    status = fbe_terminator_api_insert_sas_drive (0, encl_id-2, 0, &sas_drive, &drive_id);/*add drive to port 0 encl 3*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);


    array_dev_slot_stat_ptr[4][0].cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    exp_phy_stat_ptr[4][drive_to_phy_map[0]].phy_rdy = 1;

	/* now let verify the status */
	status = fbe_terminator_miniport_api_register_payload_completion(0,
																    terminator_miniport_api_test_PHY_io_complete,
																    &device_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    // alloocate memory for the sglist
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);

    // test by issuing to the virtual phy.
    device_info.port = 0;
    device_info.encl_id = 4;
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	fbe_semaphore_wait(&update_semaphore, NULL);

    /* now let verify the status */
    // test 1
    status = fbe_terminator_api_get_sas_enclosure_drive_slot_eses_status(0, 
        2, 0, &drive_slot_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    drive_slot_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    drive_slot_stat.dev_bypassed_a = 1;
    drive_slot_stat.dev_off = 1;
    drive_slot_stat.app_client_bypassed_a = 1;
    status = fbe_terminator_api_set_sas_enclosure_drive_slot_eses_status(0, 
        2, 0, drive_slot_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // set for an other slot
    status = fbe_terminator_api_get_sas_enclosure_drive_slot_eses_status(0, 
        2, 1, &drive_slot_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    drive_slot_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_CRITICAL;
    drive_slot_stat.dev_bypassed_a = 1;
    drive_slot_stat.dev_off = 0;
    drive_slot_stat.app_client_bypassed_a = 1;
    status = fbe_terminator_api_set_sas_enclosure_drive_slot_eses_status(0, 
        2, 1, drive_slot_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    //set the expected values to check for in the io completion function.
    array_dev_slot_stat_ptr[2][0].cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    array_dev_slot_stat_ptr[2][0].dev_bypassed_a = 1;
    array_dev_slot_stat_ptr[2][0].dev_off = 1;
    array_dev_slot_stat_ptr[2][0].app_client_bypassed_a = 1;
    // expected values for other slot.
    array_dev_slot_stat_ptr[2][1].cmn_stat.elem_stat_code = SES_STAT_CODE_CRITICAL;
    array_dev_slot_stat_ptr[2][1].dev_bypassed_a = 1;
    array_dev_slot_stat_ptr[2][1].dev_off = 0;
    array_dev_slot_stat_ptr[2][1].app_client_bypassed_a = 1;
    // issue receive diagnostic page command.
    device_info.port = 0;
    device_info.encl_id = 2;
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat,  sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&update_semaphore, NULL);


    //test 2
    status = fbe_terminator_api_get_sas_enclosure_drive_slot_eses_status(0, 4, 2, &drive_slot_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 
    drive_slot_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_NOT_INSTALLED;
    drive_slot_stat.dev_bypassed_a = 0;
    drive_slot_stat.dev_off = 1;
    drive_slot_stat.app_client_bypassed_a = 0;
    status = fbe_terminator_api_set_sas_enclosure_drive_slot_eses_status(0, 
        4, 2, drive_slot_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    //set expected values.
    memcpy(&array_dev_slot_stat_ptr[4][2], &drive_slot_stat, sizeof(ses_stat_elem_array_dev_slot_struct));
    // Issue eses command.
    device_info.port = 0;
    device_info.encl_id = 4;    
    status =terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat,  sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&update_semaphore, NULL);

    //test 3
    status = fbe_terminator_api_get_sas_enclosure_drive_slot_eses_status(0, 6, 3, &drive_slot_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    drive_slot_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_UNRECOV;
    drive_slot_stat.dev_bypassed_a = 1;
    drive_slot_stat.dev_off = 1;
    drive_slot_stat.app_client_bypassed_a = 0;
    status = fbe_terminator_api_set_sas_enclosure_drive_slot_eses_status(0, 
        6, 3, drive_slot_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    memcpy(&array_dev_slot_stat_ptr[6][3], &drive_slot_stat, sizeof(ses_stat_elem_array_dev_slot_struct));
    // Issue eses command.
    device_info.port = 0;
    device_info.encl_id = 6;   
    status = terminator_miniport_api_issue_eses_cmd(0, 7, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat,  sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&update_semaphore, NULL);

    // Phy get&set API test.
    status = fbe_terminator_api_get_sas_enclosure_phy_eses_status(0, 6, 3, &phy_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    phy_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_NOT_INSTALLED;
    phy_stat.phy_rdy = 1;
    phy_stat.link_rdy = 1;
    phy_stat.spinup_enabled = 1;
    phy_stat.force_disabled = 1;
    status = fbe_terminator_api_set_sas_enclosure_phy_eses_status(0, 6, 3, phy_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    memcpy(&exp_phy_stat_ptr[6][3], &phy_stat, sizeof(ses_stat_elem_exp_phy_struct));
    // Issue eses command.
    device_info.port = 0;
    device_info.encl_id = 6; 
    status = terminator_miniport_api_issue_eses_cmd(0, 7, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat,  sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	fbe_semaphore_wait(&update_semaphore, NULL);

    /* Bypass the drive test */
    status = fbe_terminator_api_enclosure_bypass_drive_slot(0, drive_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    memset(&array_dev_slot_stat_ptr[4][0], 0, sizeof(ses_stat_elem_array_dev_slot_struct));
    memset(&exp_phy_stat_ptr[4][drive_to_phy_map[0]], 0, sizeof(ses_stat_elem_exp_phy_struct));
    array_dev_slot_stat_ptr[4][0].cmn_stat.elem_stat_code = SES_STAT_CODE_CRITICAL;
    exp_phy_stat_ptr[4][drive_to_phy_map[0]].cmn_stat.elem_stat_code = SES_STAT_CODE_CRITICAL;
    exp_phy_stat_ptr[4][drive_to_phy_map[0]].force_disabled = 0x1;
    // Issue eses command.
    device_info.port = 0;
    device_info.encl_id = 4; 
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat,  sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&update_semaphore, NULL);
    /* end of bypass test */

    /* Unbypass the drive test */
    status = fbe_terminator_api_enclosure_unbypass_drive_slot(0, drive_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    memset(&array_dev_slot_stat_ptr[4][0], 0, sizeof(ses_stat_elem_array_dev_slot_struct));
    memset(&exp_phy_stat_ptr[4][drive_to_phy_map[0]], 0, sizeof(ses_stat_elem_exp_phy_struct));
    array_dev_slot_stat_ptr[4][0].cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    exp_phy_stat_ptr[4][drive_to_phy_map[0]].cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    exp_phy_stat_ptr[4][drive_to_phy_map[0]].phy_rdy = 1;
    // Issue eses command.
    device_info.port = 0;
    device_info.encl_id = 4; 
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&update_semaphore, NULL);
    /* end of Unbypass test */

    status = fbe_terminator_api_remove_sas_drive(0, drive_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    memset(&array_dev_slot_stat_ptr[4][0], 0, sizeof(ses_stat_elem_array_dev_slot_struct));
    array_dev_slot_stat_ptr[4][0].cmn_stat.elem_stat_code = SES_STAT_CODE_NOT_INSTALLED;
    exp_phy_stat_ptr[4][drive_to_phy_map[0]].cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    exp_phy_stat_ptr[4][drive_to_phy_map[0]].phy_rdy = 0x0;
    // Issue eses command.
    device_info.port = 0;
    device_info.encl_id = 4;    
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat,  sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&update_semaphore, NULL);

    // Issue eses command.
    device_info.port = 0;
    device_info.encl_id = 4; 
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_config,  sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&update_semaphore, NULL);

    // issue to phys on enclosure ids 2,4,6.
    // This is the unit test for testing emc encl stat
    // page and the connector status elements in 
    // enclosure status diagnostic page. Just verify
    // them in debugger as it is appearing to be more
    // reliable for offset verficiation instead of Asserts()
    // in Io completion function.

    // Issue eses command.
    device_info.port = 0;
    device_info.encl_id = 2; 
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_emc_encl_stat,  sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&update_semaphore, NULL);

    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat,  sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&update_semaphore, NULL);

    // Issue eses command.
    device_info.port = 0;
    device_info.encl_id = 4; 
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_emc_encl_stat,  sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 
    fbe_semaphore_wait(&update_semaphore, NULL);

    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat,  sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&update_semaphore, NULL);

    // Issue eses command.
    device_info.port = 0;
    device_info.encl_id = 6; 
    status = terminator_miniport_api_issue_eses_cmd(0, 7, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_emc_encl_stat,  sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&update_semaphore, NULL);

    status = terminator_miniport_api_issue_eses_cmd(0, 7, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat,  sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&update_semaphore, NULL);

    // End of test Addl stat page, SAS exp elements.

    status = fbe_terminator_api_destroy();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator destroyed.", __FUNCTION__);
}

void terminator_set_enclosure_ps_cooling_tempSensor_status_test(void)
{
    fbe_status_t                    status;
    fbe_terminator_board_info_t     board_info;
    fbe_terminator_sas_port_info_t  sas_port;
    fbe_terminator_sas_encl_info_t  sas_encl;
    fbe_terminator_sas_drive_info_t sas_drive;
    fbe_terminator_api_device_handle_t        encl_id;
    fbe_terminator_api_device_handle_t        drive_id;

    fbe_terminator_api_device_handle_t        device_list[MAX_DEVICES];
    fbe_u32_t                       total_devices = 0;
    fbe_cpd_shim_enumerate_backend_ports_t cpd_shim_enumerate_backend_ports;
    fbe_u32_t                       i = 0;
    fbe_u32_t                       context = 0;
    fbe_terminator_device_ptr_t handle;
    fbe_u32_t       port_index;

    ses_stat_elem_ps_struct             terminator_ps_stat;
    ses_stat_elem_cooling_struct        terminator_cooling_stat;
    ses_stat_elem_temp_sensor_struct    terminator_temp_sensor_stat;
    fbe_sg_element_t *sg_p = NULL;
        
    mut_printf(MUT_LOG_HIGH, "%s %s ",__FILE__, __FUNCTION__);
    fbe_semaphore_init(&update_semaphore, 0, FBE_SEMAPHORE_MAX);
    init_ps_elem_strts();
    init_cooling_elem_strts();
    init_temp_sensor_elem_strts();

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

    sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    sas_port.io_port_number = 0;
    sas_port.portal_number = 1;
    sas_port.backend_number = 0;
    status  = fbe_terminator_api_insert_sas_port (0, &sas_port);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_terminator_miniport_api_port_init(0, 1, &port_index);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Register a callback to port 0, so we can destroy the object properly */
    status = fbe_terminator_miniport_api_register_callback(port_index, fbe_cpd_shim_callback_function_mock, NULL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* enumerate the ports */
    status  = fbe_terminator_miniport_api_enumerate_cpd_ports (&cpd_shim_enumerate_backend_ports);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/* Peter Puhov: I think it is wrong test
    for (i=0; i<cpd_shim_enumerate_backend_ports.number_of_io_ports; i++)
    {
        MUT_ASSERT_INT_EQUAL(i, cpd_shim_enumerate_backend_ports.io_port_array[i])
    }
	*/

    /* enumerate the devices on ports */
    terminator_get_port_ptr_by_port_portal(0, 1, &handle);
	status = terminator_enumerate_devices (handle, device_list, &total_devices);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(0, total_devices);

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
    sas_encl.sas_address = (fbe_u64_t)0x123456783;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    sas_encl.connector_id = 0;
    status  = fbe_terminator_api_insert_sas_enclosure (0, &sas_encl, &encl_id);/*add encl on port 0*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* enumerate the devices on ports */
    terminator_get_port_ptr_by_port_portal(0, 1, &handle);
	status = terminator_enumerate_devices (handle, device_list, &total_devices);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(6, total_devices);

    /*insert a sas_drive to the second last enclosure*/
    // inserted in encl id 4
    sas_drive.sas_address = (fbe_u64_t)0x987654321;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.capacity = 0x10BD0;
    sas_drive.block_size = 520;
    status = fbe_terminator_api_insert_sas_drive (0, encl_id-2, 0, &sas_drive, &drive_id);/*add drive to port 0 encl 3*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    /* now lets verify the PS status in enclosure status page */
    status =  fbe_terminator_miniport_api_register_payload_completion(0,
                                                                    terminator_miniport_api_test_PS_io_complete,
                                                                    &device_info);

    // alloocate memory for the sglist
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);

    // Issue eses command.
    device_info.port = 0;
    device_info.encl_id = 4;     
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&update_semaphore, NULL);

    // Issue eses command.
    device_info.port = 0;
    device_info.encl_id = 6;  
    status = terminator_miniport_api_issue_eses_cmd(0, 7, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&update_semaphore, NULL);

    // AC-fail API testing.

    status = fbe_terminator_api_get_ps_eses_status(0, 2, PS_0, &terminator_ps_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    terminator_ps_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    terminator_ps_stat.off = 0;
    terminator_ps_stat.dc_fail = 0;
    terminator_ps_stat.rqsted_on = 1;
    terminator_ps_stat.ac_fail = 1;
    //Ask terminator to set the values accordingly.
    status = fbe_terminator_api_set_ps_eses_status(0, 2, PS_0, terminator_ps_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // set the expected values for the completion function to check.
    ps_stat[2][PS_0].cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    ps_stat[2][PS_0].off = 0;
    ps_stat[2][PS_0].dc_fail = 0;
    ps_stat[2][PS_0].rqsted_on = 1;
    ps_stat[2][PS_0].ac_fail = 1;
    //now issue the request for the status page.
    device_info.port = 0;
    device_info.encl_id = 2;  
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&update_semaphore, NULL);

    status = fbe_terminator_api_get_ps_eses_status(0, 6, PS_1, &terminator_ps_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    terminator_ps_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    terminator_ps_stat.off = 0;
    terminator_ps_stat.dc_fail = 0;
    terminator_ps_stat.rqsted_on = 1;
    terminator_ps_stat.ac_fail = 1;
    //Ask terminator to set the values accordingly.
    status = fbe_terminator_api_set_ps_eses_status(0, 6, PS_1, terminator_ps_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // set the expected values for the completion function to check.
    ps_stat[6][PS_1].cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    ps_stat[6][PS_1].off = 0;
    ps_stat[6][PS_1].dc_fail = 0;
    ps_stat[6][PS_1].rqsted_on = 1;
    ps_stat[6][PS_1].ac_fail = 1;
    //now issue the request for the status page.
    device_info.port = 0;
    device_info.encl_id = 6; 
    status = terminator_miniport_api_issue_eses_cmd(0, 7, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&update_semaphore, NULL);

    //Test for cooling status elements

    status = fbe_terminator_api_get_cooling_eses_status(0, 4, COOLING_0, &terminator_cooling_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    terminator_cooling_stat.actual_fan_speed_high = 0x1;
    terminator_cooling_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_CRITICAL;
    terminator_cooling_stat.fail = 1;
    terminator_cooling_stat.rqsted_on = 0;
    //Ask terminator to set the values accordingly.
    status = fbe_terminator_api_set_cooling_eses_status(0, 4, COOLING_0, terminator_cooling_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // set the expected values for the completion function to check.
    cooling_stat[4][COOLING_0].actual_fan_speed_high = 0x1;
    cooling_stat[4][COOLING_0].cmn_stat.elem_stat_code = SES_STAT_CODE_CRITICAL;
    cooling_stat[4][COOLING_0].fail = 1;
    cooling_stat[4][COOLING_0].rqsted_on = 0;
    //now issue the request for the status page.
    //now issue the request for the status page.
    device_info.port = 0;
    device_info.encl_id = 4; 
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&update_semaphore, NULL);

    status = fbe_terminator_api_get_cooling_eses_status(0, 4, COOLING_1, &terminator_cooling_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    terminator_cooling_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_CRITICAL;
    terminator_cooling_stat.fail = 1;
    terminator_cooling_stat.rqsted_on = 1;
    //Ask terminator to set the values accordingly.
    status = fbe_terminator_api_set_cooling_eses_status(0, 4, COOLING_1, terminator_cooling_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // set the expected values for the completion function to check.
    cooling_stat[4][COOLING_1].cmn_stat.elem_stat_code = SES_STAT_CODE_CRITICAL;
    cooling_stat[4][COOLING_1].fail = 1;
    cooling_stat[4][COOLING_1].rqsted_on = 1;
    //now issue the request for the status page.
    device_info.port = 0;
    device_info.encl_id = 4; 
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&update_semaphore, NULL);

    status = fbe_terminator_api_get_cooling_eses_status(0, 2, COOLING_1, &terminator_cooling_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    terminator_cooling_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_CRITICAL;
    terminator_cooling_stat.fail = 1;
    terminator_cooling_stat.rqsted_on = 0;
    //Ask terminator to set the values accordingly.
    status = fbe_terminator_api_set_cooling_eses_status(0, 2, COOLING_1, terminator_cooling_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // set the expected values for the completion function to check.
    cooling_stat[2][COOLING_1].cmn_stat.elem_stat_code = SES_STAT_CODE_CRITICAL;
    cooling_stat[2][COOLING_1].fail = 1;
    cooling_stat[2][COOLING_1].rqsted_on = 0;
    //now issue the request for the status page.
    device_info.port = 0;
    device_info.encl_id = 2; 
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&update_semaphore, NULL);

    status = fbe_terminator_api_get_cooling_eses_status(0, 4, COOLING_3, &terminator_cooling_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    terminator_cooling_stat.actual_fan_speed_high = 0x2;
    terminator_cooling_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_CRITICAL;
    terminator_cooling_stat.fail = 1;
    terminator_cooling_stat.rqsted_on = 0;
    terminator_cooling_stat.actual_speed_code = 5;
    //Ask terminator to set the values accordingly.
    status = fbe_terminator_api_set_cooling_eses_status(0, 4, COOLING_3, terminator_cooling_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // set the expected values for the completion function to check.
    cooling_stat[4][COOLING_3].actual_fan_speed_high = 0x2;
    cooling_stat[4][COOLING_3].cmn_stat.elem_stat_code = SES_STAT_CODE_CRITICAL;
    cooling_stat[4][COOLING_3].fail = 1;
    cooling_stat[4][COOLING_3].rqsted_on = 0;
    cooling_stat[4][COOLING_3].actual_speed_code = 5;
    //now issue the request for the status page.
    device_info.port = 0;
    device_info.encl_id = 4; 
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&update_semaphore, NULL);

    
    // Test for individual Temperature Sensor Elements.
    
    status = fbe_terminator_api_get_temp_sensor_eses_status(0, 6, TEMP_SENSOR_0, &terminator_temp_sensor_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    terminator_temp_sensor_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_CRITICAL;
    terminator_temp_sensor_stat.ot_failure = 1;
    terminator_temp_sensor_stat.ot_warning = 1;
    terminator_temp_sensor_stat.temp = 5;
    //Ask terminator to set the values accordingly.
    status = fbe_terminator_api_set_temp_sensor_eses_status(0, 6, TEMP_SENSOR_0, terminator_temp_sensor_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // set the expected values for the completion function to check.
    temp_sensor_stat[6][TEMP_SENSOR_0].cmn_stat.elem_stat_code = SES_STAT_CODE_CRITICAL;
    temp_sensor_stat[6][TEMP_SENSOR_0].ot_failure = 1;
    temp_sensor_stat[6][TEMP_SENSOR_0].ot_warning = 1;
    temp_sensor_stat[6][TEMP_SENSOR_0].temp = 5;
    //now issue the request for the status page.
    device_info.port = 0;
    device_info.encl_id = 6; 
    status = terminator_miniport_api_issue_eses_cmd(0, 7, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&update_semaphore, NULL);

    // There is only one temperature sensor. This should return an error.
    status = fbe_terminator_api_get_temp_sensor_eses_status(0, 6, 1, &terminator_temp_sensor_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);

    status = fbe_terminator_api_get_temp_sensor_eses_status(0, 6, TEMP_SENSOR_0, &terminator_temp_sensor_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    terminator_temp_sensor_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_UNKNOWN;
    terminator_temp_sensor_stat.ot_failure = 0;
    terminator_temp_sensor_stat.ot_warning = 1;
    terminator_temp_sensor_stat.temp = 3;
    //Ask terminator to set the values accordingly.
    status = fbe_terminator_api_set_temp_sensor_eses_status(0, 6, TEMP_SENSOR_0, terminator_temp_sensor_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // set the expected values for the completion function to check.
    temp_sensor_stat[6][TEMP_SENSOR_0].cmn_stat.elem_stat_code = SES_STAT_CODE_UNKNOWN;
    temp_sensor_stat[6][TEMP_SENSOR_0].ot_failure = 0;
    temp_sensor_stat[6][TEMP_SENSOR_0].ot_warning = 1;
    temp_sensor_stat[6][TEMP_SENSOR_0].temp = 3;
    //now issue the request for the status page.
    device_info.port = 0;
    device_info.encl_id = 6; 
    status = terminator_miniport_api_issue_eses_cmd(0, 7, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&update_semaphore, NULL);

    status = fbe_terminator_api_get_temp_sensor_eses_status(0, 2, TEMP_SENSOR_0, &terminator_temp_sensor_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    terminator_temp_sensor_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_UNKNOWN;
    terminator_temp_sensor_stat.ot_failure = 0;
    terminator_temp_sensor_stat.ot_warning = 1;
    terminator_temp_sensor_stat.temp = 1;
    //Ask terminator to set the values accordingly.
    status = fbe_terminator_api_set_temp_sensor_eses_status(0, 2, TEMP_SENSOR_0, terminator_temp_sensor_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // set the expected values for the completion function to check.
    temp_sensor_stat[2][TEMP_SENSOR_0].cmn_stat.elem_stat_code = SES_STAT_CODE_UNKNOWN;
    temp_sensor_stat[2][TEMP_SENSOR_0].ot_failure = 0;
    temp_sensor_stat[2][TEMP_SENSOR_0].ot_warning = 1;
    temp_sensor_stat[2][TEMP_SENSOR_0].temp = 1;
    //now issue the request for the status page.
    device_info.port = 0;
    device_info.encl_id = 2; 
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&update_semaphore, NULL);



    // Test for Overall Temperature Sensor Elements.
    
    status = fbe_terminator_api_get_overall_temp_sensor_eses_status(0, 6, TEMP_SENSOR_0, &terminator_temp_sensor_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    terminator_temp_sensor_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_CRITICAL;
    terminator_temp_sensor_stat.ot_failure = 1;
    terminator_temp_sensor_stat.ot_warning = 1;
    terminator_temp_sensor_stat.temp = 5;
    //Ask terminator to set the values accordingly.
    status = fbe_terminator_api_set_overall_temp_sensor_eses_status(0, 6, TEMP_SENSOR_0, terminator_temp_sensor_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // set the expected values for the completion function to check.
    overall_temp_sensor_stat[6][TEMP_SENSOR_0].cmn_stat.elem_stat_code = SES_STAT_CODE_CRITICAL;
    overall_temp_sensor_stat[6][TEMP_SENSOR_0].ot_failure = 1;
    overall_temp_sensor_stat[6][TEMP_SENSOR_0].ot_warning = 1;
    overall_temp_sensor_stat[6][TEMP_SENSOR_0].temp = 5;
    //now issue the request for the status page.
    device_info.port = 0;
    device_info.encl_id = 6; 
    status = terminator_miniport_api_issue_eses_cmd(0, 7, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&update_semaphore, NULL);

    // There is only one temperature sensor. This should return an error.
    status = fbe_terminator_api_get_overall_temp_sensor_eses_status(0, 6, 1, &terminator_temp_sensor_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);

    status = fbe_terminator_api_get_overall_temp_sensor_eses_status(0, 6, TEMP_SENSOR_0, &terminator_temp_sensor_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    terminator_temp_sensor_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_UNKNOWN;
    terminator_temp_sensor_stat.ot_failure = 0;
    terminator_temp_sensor_stat.ot_warning = 1;
    terminator_temp_sensor_stat.temp = 3;
    //Ask terminator to set the values accordingly.
    status = fbe_terminator_api_set_overall_temp_sensor_eses_status(0, 6, TEMP_SENSOR_0, terminator_temp_sensor_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // set the expected values for the completion function to check.
    overall_temp_sensor_stat[6][TEMP_SENSOR_0].cmn_stat.elem_stat_code = SES_STAT_CODE_UNKNOWN;
    overall_temp_sensor_stat[6][TEMP_SENSOR_0].ot_failure = 0;
    overall_temp_sensor_stat[6][TEMP_SENSOR_0].ot_warning = 1;
    overall_temp_sensor_stat[6][TEMP_SENSOR_0].temp = 3;
    //now issue the request for the status page.
    device_info.port = 0;
    device_info.encl_id = 6; 
    status = terminator_miniport_api_issue_eses_cmd(0, 7, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&update_semaphore, NULL);

    status = fbe_terminator_api_get_overall_temp_sensor_eses_status(0, 2, TEMP_SENSOR_0, &terminator_temp_sensor_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    terminator_temp_sensor_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_UNKNOWN;
    terminator_temp_sensor_stat.ot_failure = 0;
    terminator_temp_sensor_stat.ot_warning = 1;
    terminator_temp_sensor_stat.temp = 1;
    //Ask terminator to set the values accordingly.
    status = fbe_terminator_api_set_overall_temp_sensor_eses_status(0, 2, TEMP_SENSOR_0, terminator_temp_sensor_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // set the expected values for the completion function to check.
    overall_temp_sensor_stat[2][TEMP_SENSOR_0].cmn_stat.elem_stat_code = SES_STAT_CODE_UNKNOWN;
    overall_temp_sensor_stat[2][TEMP_SENSOR_0].ot_failure = 0;
    overall_temp_sensor_stat[2][TEMP_SENSOR_0].ot_warning = 1;
    overall_temp_sensor_stat[2][TEMP_SENSOR_0].temp = 1;
    //now issue the request for the status page.
    device_info.port = 0;
    device_info.encl_id = 2; 
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&update_semaphore, NULL);


    status = fbe_terminator_api_destroy();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator destroyed.", __FUNCTION__);
}

fbe_status_t terminator_miniport_api_test_PS_io_complete (fbe_payload_ex_t * payload, fbe_payload_ex_completion_context_t context)
{
    fbe_status_t status;
    fbe_u8_t *return_page_code = NULL;
    ses_pg_encl_stat_struct  *encl_status_diag_page_hdr = NULL;
    ses_stat_elem_ps_struct *ps_stat_page_ptr, *ps_expected_ptr;
    ses_stat_elem_cooling_struct *cooling_stat_page_ptr, *cooling_expected_ptr;
    ses_stat_elem_temp_sensor_struct *temp_sensor_stat_page_ptr, *temp_sensor_expected_ptr;
    fbe_payload_cdb_operation_t *payload_cdb_operation = NULL;
    fbe_u8_t *cdb = NULL;
    fbe_sg_element_t *sg_list = NULL;
    fbe_port_request_status_t cdb_request_status;

    // As all these tests are only on port 0.
    fbe_u8_t encl_id = ((eses_test_io_completion_context_t *)context)->encl_id;
    fbe_u32_t i=0;

    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    MUT_ASSERT_TRUE(payload_cdb_operation != NULL);  

    fbe_payload_cdb_operation_get_cdb(payload_cdb_operation, &cdb); 

    status = fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_payload_cdb_get_request_status(payload_cdb_operation, &cdb_request_status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    if (cdb[0] == FBE_SCSI_RECEIVE_DIAGNOSTIC)
    {   
        return_page_code = (fbe_u8_t *)fbe_sg_element_address(sg_list);
        if(*return_page_code == SES_PG_CODE_ENCL_STAT)
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: opcode:0x%x,return_status: %d\n", __FUNCTION__, cdb[0], cdb_request_status);
            encl_status_diag_page_hdr = (ses_pg_encl_stat_struct *)fbe_sg_element_address(sg_list);    
            terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: diagnostic page code returned ->0x%x\n", __FUNCTION__, encl_status_diag_page_hdr->pg_code);

            // Test for power supply A
            ps_stat_page_ptr = (ses_stat_elem_ps_struct *) (((fbe_u8_t *)encl_status_diag_page_hdr) + PS_A_OFFSET);
            ps_stat_page_ptr++;
            ps_expected_ptr = &ps_stat[encl_id][PS_0];
            MUT_ASSERT_TRUE(ps_stat_page_ptr->cmn_stat.elem_stat_code == ps_expected_ptr->cmn_stat.elem_stat_code);
            MUT_ASSERT_TRUE(ps_stat_page_ptr->off == ps_expected_ptr->off);
            MUT_ASSERT_TRUE(ps_stat_page_ptr->dc_fail == ps_expected_ptr->dc_fail);
            MUT_ASSERT_TRUE(ps_stat_page_ptr->rqsted_on == ps_expected_ptr->rqsted_on);
            MUT_ASSERT_TRUE(ps_stat_page_ptr->ac_fail == ps_expected_ptr->ac_fail);

            // Test for power supply B
            ps_stat_page_ptr = (ses_stat_elem_ps_struct *) (((fbe_u8_t *)encl_status_diag_page_hdr) + PS_B_OFFSET);
            ps_stat_page_ptr++;
            ps_expected_ptr = &ps_stat[encl_id][PS_1];
            MUT_ASSERT_TRUE(ps_stat_page_ptr->cmn_stat.elem_stat_code == ps_expected_ptr->cmn_stat.elem_stat_code);
            MUT_ASSERT_TRUE(ps_stat_page_ptr->off == ps_expected_ptr->off);
            MUT_ASSERT_TRUE(ps_stat_page_ptr->dc_fail == ps_expected_ptr->dc_fail);
            MUT_ASSERT_TRUE(ps_stat_page_ptr->rqsted_on == ps_expected_ptr->rqsted_on);
            MUT_ASSERT_TRUE(ps_stat_page_ptr->ac_fail == ps_expected_ptr->ac_fail);

            // Test for cooling elements 0,1
            cooling_stat_page_ptr = (ses_stat_elem_cooling_struct *) (((fbe_u8_t *)encl_status_diag_page_hdr) + COOLING_0_OFFSET);
            cooling_stat_page_ptr++;
            cooling_expected_ptr = &cooling_stat[encl_id][COOLING_0];
            MUT_ASSERT_TRUE(cooling_stat_page_ptr->cmn_stat.elem_stat_code == cooling_expected_ptr->cmn_stat.elem_stat_code);
            MUT_ASSERT_TRUE(cooling_stat_page_ptr->actual_fan_speed_high == cooling_expected_ptr->actual_fan_speed_high);
            MUT_ASSERT_TRUE(cooling_stat_page_ptr->actual_fan_speed_low == cooling_expected_ptr->actual_fan_speed_low);
            MUT_ASSERT_TRUE(cooling_stat_page_ptr->rqsted_on == cooling_expected_ptr->rqsted_on);
            MUT_ASSERT_TRUE(cooling_stat_page_ptr->off == cooling_expected_ptr->off);
            MUT_ASSERT_TRUE(cooling_stat_page_ptr->actual_speed_code == cooling_expected_ptr->actual_speed_code);
            
            cooling_stat_page_ptr++;
            cooling_expected_ptr = &cooling_stat[encl_id][COOLING_1];
            MUT_ASSERT_TRUE(cooling_stat_page_ptr->cmn_stat.elem_stat_code == cooling_expected_ptr->cmn_stat.elem_stat_code);
            MUT_ASSERT_TRUE(cooling_stat_page_ptr->actual_fan_speed_high == cooling_expected_ptr->actual_fan_speed_high);
            MUT_ASSERT_TRUE(cooling_stat_page_ptr->actual_fan_speed_low == cooling_expected_ptr->actual_fan_speed_low);
            MUT_ASSERT_TRUE(cooling_stat_page_ptr->rqsted_on == cooling_expected_ptr->rqsted_on);
            MUT_ASSERT_TRUE(cooling_stat_page_ptr->off == cooling_expected_ptr->off);
            MUT_ASSERT_TRUE(cooling_stat_page_ptr->actual_speed_code == cooling_expected_ptr->actual_speed_code);
            
            // Test for cooling elements 1,2
            cooling_stat_page_ptr = (ses_stat_elem_cooling_struct *) (((fbe_u8_t *)encl_status_diag_page_hdr) + COOLING_2_OFFSET);
            cooling_stat_page_ptr++;
            cooling_expected_ptr = &cooling_stat[encl_id][COOLING_2];
            MUT_ASSERT_TRUE(cooling_stat_page_ptr->cmn_stat.elem_stat_code == cooling_expected_ptr->cmn_stat.elem_stat_code);
            MUT_ASSERT_TRUE(cooling_stat_page_ptr->actual_fan_speed_high == cooling_expected_ptr->actual_fan_speed_high);
            MUT_ASSERT_TRUE(cooling_stat_page_ptr->actual_fan_speed_low == cooling_expected_ptr->actual_fan_speed_low);
            MUT_ASSERT_TRUE(cooling_stat_page_ptr->rqsted_on == cooling_expected_ptr->rqsted_on);
            MUT_ASSERT_TRUE(cooling_stat_page_ptr->off == cooling_expected_ptr->off);
            MUT_ASSERT_TRUE(cooling_stat_page_ptr->actual_speed_code == cooling_expected_ptr->actual_speed_code);
            
            cooling_stat_page_ptr++;
            cooling_expected_ptr = &cooling_stat[encl_id][COOLING_3];
            MUT_ASSERT_TRUE(cooling_stat_page_ptr->cmn_stat.elem_stat_code == cooling_expected_ptr->cmn_stat.elem_stat_code);
            MUT_ASSERT_TRUE(cooling_stat_page_ptr->actual_fan_speed_high == cooling_expected_ptr->actual_fan_speed_high);
            MUT_ASSERT_TRUE(cooling_stat_page_ptr->actual_fan_speed_low == cooling_expected_ptr->actual_fan_speed_low);
            MUT_ASSERT_TRUE(cooling_stat_page_ptr->rqsted_on == cooling_expected_ptr->rqsted_on);
            MUT_ASSERT_TRUE(cooling_stat_page_ptr->off == cooling_expected_ptr->off);
            MUT_ASSERT_TRUE(cooling_stat_page_ptr->actual_speed_code == cooling_expected_ptr->actual_speed_code);
            
            // Test for temperature sensor elements
            temp_sensor_stat_page_ptr = (ses_stat_elem_temp_sensor_struct *) (((fbe_u8_t *)encl_status_diag_page_hdr) + TEMP_SENSOR_0_OFFSET);
            temp_sensor_stat_page_ptr++;
            temp_sensor_expected_ptr = &temp_sensor_stat[encl_id][TEMP_SENSOR_0];
            MUT_ASSERT_TRUE(temp_sensor_stat_page_ptr->cmn_stat.elem_stat_code == temp_sensor_expected_ptr->cmn_stat.elem_stat_code);
            MUT_ASSERT_TRUE(temp_sensor_stat_page_ptr->ot_failure  == temp_sensor_expected_ptr->ot_failure );
            MUT_ASSERT_TRUE(temp_sensor_stat_page_ptr->ot_warning == temp_sensor_expected_ptr->ot_warning);
            MUT_ASSERT_TRUE(temp_sensor_stat_page_ptr->ident == temp_sensor_expected_ptr->ident);
            MUT_ASSERT_TRUE(temp_sensor_stat_page_ptr->temp == temp_sensor_expected_ptr->temp);

            // Test for overall temperature sensor elements
            temp_sensor_stat_page_ptr = (ses_stat_elem_temp_sensor_struct *) (((fbe_u8_t *)encl_status_diag_page_hdr) + TEMP_SENSOR_0_OFFSET);
            temp_sensor_expected_ptr = &overall_temp_sensor_stat[encl_id][TEMP_SENSOR_0];
            MUT_ASSERT_INT_EQUAL(temp_sensor_expected_ptr->cmn_stat.elem_stat_code, temp_sensor_stat_page_ptr->cmn_stat.elem_stat_code);
            MUT_ASSERT_INT_EQUAL(temp_sensor_expected_ptr->ot_failure, temp_sensor_stat_page_ptr->ot_failure);
            MUT_ASSERT_INT_EQUAL(temp_sensor_expected_ptr->ot_warning, temp_sensor_stat_page_ptr->ot_warning);
            MUT_ASSERT_INT_EQUAL(temp_sensor_expected_ptr->ident, temp_sensor_stat_page_ptr->ident);
            MUT_ASSERT_INT_EQUAL(temp_sensor_expected_ptr->temp, temp_sensor_stat_page_ptr->temp);
        }
    }
    else
    {
        MUT_ASSERT_TRUE(1);
    }

    fbe_semaphore_release(&update_semaphore, 0, 1, FALSE);

    return FBE_STATUS_OK;
}

#if 0
/************************************************************************
fbe_status_t terminator_miniport_api_test_issue_phy_receive_diagnostic (
    fbe_u32_t port_number, 
    fbe_cpd_device_id_t device_id, 
    ses_pg_code_enum page_code)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_sg_element_t *sg_p = NULL;
    fbe_io_block_t *io_block;
    
    io_block =(fbe_io_block_t *)malloc(sizeof(fbe_io_block_t));
    //io_block = io_block_new();


    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    if (sg_p == NULL)
    {
        status = FBE_STATUS_INSUFFICIENT_RESOURCES;
        return status;
    }
    // First, setup the io block.

    io_block_build_receive_diagnostic(io_block, device_id, page_code);
    io_block->operation.execute_cdb.port_number = port_number;
    io_block->sg_list = sg_p;

    status = fbe_terminator_miniport_api_send_io(0, io_block);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    //fbe_ldo_test_free_memory_with_sg(&sg_p);
    return(status);  

}

fbe_status_t
io_block_build_receive_diagnostic(fbe_io_block_t * io_block, fbe_cpd_device_id_t cpd_device_id, ses_pg_code_enum page_code)
{
    fbe_eses_receive_diagnostic_results_cdb_t *cdb_page;

    io_block->io_block_opcode = FBE_IO_BLOCK_OPCODE_EXECUTE_CDB;
    io_block->operation.execute_cdb.cpd_device_id = cpd_device_id;
    
    cdb_page = (fbe_eses_receive_diagnostic_results_cdb_t *)(io_block->operation.execute_cdb.cdb);
    cdb_page->page_code_valid = 1;
    cdb_page->control = 0;
    cdb_page->operation_code = FBE_SCSI_RECEIVE_DIAGNOSTIC;
    cdb_page->page_code = page_code;

    return FBE_STATUS_OK;
}
******************************************************************************/

#endif


fbe_status_t terminator_miniport_api_test_PHY_io_complete (fbe_payload_ex_t * payload, fbe_payload_ex_completion_context_t context)
{
    fbe_status_t status;
    fbe_u8_t *return_page_code = NULL;
    ses_pg_encl_stat_struct  *encl_status_diag_page_hdr = NULL;
    ses_stat_elem_array_dev_slot_struct *array_dev_slot_stat_rtn_ptr = NULL;
    ses_stat_elem_exp_phy_struct *exp_phy_stat_rtn_ptr = NULL;
    ses_common_pg_hdr_struct *addl_elem_stat_page_hdr = NULL;
    ses_array_dev_phy_desc_struct *dev_phy_desc_ptr = NULL;
    ses_esc_elec_exp_phy_desc_struct *esc_elec_exp_phy_desc_ptr = NULL;
    ses_sas_exp_phy_desc_struct *sas_exp_phy_desc_ptr = NULL;
    ses_sas_exp_prot_spec_info_struct *sas_exp_prot_spec_info_ptr = NULL;
    ses_stat_elem_encl_struct *encl_stat_rtn_ptr = NULL;
    fbe_payload_cdb_operation_t *payload_cdb_operation;
    fbe_u8_t *cdb = NULL;
    fbe_sg_element_t *sg_list = NULL;
    fbe_port_request_status_t cdb_request_status;
    // As all these tests are only on port 0.
    fbe_u8_t encl_id = ((eses_test_io_completion_context_t *)context)->encl_id;
    fbe_u32_t i=0;

    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    MUT_ASSERT_TRUE(payload_cdb_operation != NULL);   

    fbe_payload_cdb_operation_get_cdb(payload_cdb_operation, &cdb); 

    status = fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_payload_cdb_get_request_status(payload_cdb_operation, &cdb_request_status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    if (cdb[0] == FBE_SCSI_RECEIVE_DIAGNOSTIC)
    {   
        return_page_code = (fbe_u8_t *)fbe_sg_element_address(sg_list);
        if(*return_page_code == SES_PG_CODE_ENCL_STAT)
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: opcode:0x%x,return_status: %d\n", __FUNCTION__, cdb[0], cdb_request_status);
            encl_status_diag_page_hdr = (ses_pg_encl_stat_struct *)fbe_sg_element_address(sg_list);    
            terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: diagnostic page code returned ->0x%x\n", __FUNCTION__, encl_status_diag_page_hdr->pg_code);
            // now this points to overall status element that we ignore
            array_dev_slot_stat_rtn_ptr = (ses_stat_elem_array_dev_slot_struct *) (((fbe_u8_t *)encl_status_diag_page_hdr) + ARRAY_DEVICE_SLOT_ELEM_OFFSET);

            terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: overall status element has ELEMENT STAT CODE: 0x%x\n", __FUNCTION__, 
                    array_dev_slot_stat_rtn_ptr->cmn_stat.elem_stat_code);
            for(i=0; i<MAX_SLOT_NUMBER; i++)
            {
                array_dev_slot_stat_rtn_ptr++;
                /* terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: Individual drive status element i.e. slot num:%d has ELEMENT STAT CODE 0x%x\n", __FUNCTION__, i, 
                    array_dev_slot_stat_ptr->cmn_stat.elem_stat_code); */
                // the return ioblock has the id of the v.phy
                MUT_ASSERT_TRUE(array_dev_slot_stat_rtn_ptr->cmn_stat.elem_stat_code == array_dev_slot_stat_ptr[encl_id][i].cmn_stat.elem_stat_code);
                MUT_ASSERT_TRUE(array_dev_slot_stat_rtn_ptr->dev_bypassed_a == array_dev_slot_stat_ptr[encl_id][i].dev_bypassed_a);
                MUT_ASSERT_TRUE(array_dev_slot_stat_rtn_ptr->dev_off == array_dev_slot_stat_ptr[encl_id][i].dev_off);
            }

            exp_phy_stat_rtn_ptr = (ses_stat_elem_exp_phy_struct *) (((fbe_u8_t *)encl_status_diag_page_hdr) + PHY_ELEM_OFFSET);
            for(i=0; i<MAX_PHY_NUMBER; i++)
            {
                exp_phy_stat_rtn_ptr ++;
                MUT_ASSERT_INT_EQUAL(exp_phy_stat_rtn_ptr->phy_id, i);
                MUT_ASSERT_INT_EQUAL( exp_phy_stat_rtn_ptr->cmn_stat.elem_stat_code, exp_phy_stat_ptr[encl_id][i].cmn_stat.elem_stat_code);
                MUT_ASSERT_INT_EQUAL(exp_phy_stat_rtn_ptr->phy_rdy, exp_phy_stat_ptr[encl_id][i].phy_rdy);
                MUT_ASSERT_INT_EQUAL(exp_phy_stat_rtn_ptr->link_rdy, exp_phy_stat_ptr[encl_id][i].link_rdy);
                MUT_ASSERT_INT_EQUAL(exp_phy_stat_rtn_ptr->spinup_enabled, exp_phy_stat_ptr[encl_id][i].spinup_enabled);
                MUT_ASSERT_INT_EQUAL(exp_phy_stat_rtn_ptr->force_disabled, exp_phy_stat_ptr[encl_id][i].force_disabled);
                /* terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: Individual PHY status element, phy slot num:%d, ELEMENT STAT CODE 0x%x, phy_id:%d\n", __FUNCTION__, i, 
                    exp_phy_stat_ptr->cmn_stat.elem_stat_code, exp_phy_stat_ptr->phy_id); */
            }

            // local lcc encl status element
            encl_stat_rtn_ptr = (ses_stat_elem_encl_struct *)(((fbe_u8_t *)encl_status_diag_page_hdr) + LOCAL_LCC_ENCL_STAT_ELEM_OFFSET);
            encl_stat_rtn_ptr++;
            MUT_ASSERT_INT_EQUAL(SES_STAT_CODE_OK, encl_stat_rtn_ptr->cmn_stat.elem_stat_code);
            MUT_ASSERT_INT_EQUAL(0, encl_stat_rtn_ptr->warning_indicaton);
            MUT_ASSERT_INT_EQUAL(0, encl_stat_rtn_ptr->failure_indicaton);
            MUT_ASSERT_INT_EQUAL(0, encl_stat_rtn_ptr->ident);

            // peer lcc encl status element
            encl_stat_rtn_ptr = (ses_stat_elem_encl_struct *)(((fbe_u8_t *)encl_status_diag_page_hdr) + PEER_LCC_ENCL_STAT_ELEM_OFFSET);
            encl_stat_rtn_ptr++;
            MUT_ASSERT_INT_EQUAL(SES_STAT_CODE_NOT_INSTALLED, encl_stat_rtn_ptr->cmn_stat.elem_stat_code);
            MUT_ASSERT_INT_EQUAL(0, encl_stat_rtn_ptr->warning_indicaton);
            MUT_ASSERT_INT_EQUAL(0, encl_stat_rtn_ptr->failure_indicaton);
            MUT_ASSERT_INT_EQUAL(0, encl_stat_rtn_ptr->ident);

            // chassis lcc encl status element
            encl_stat_rtn_ptr = (ses_stat_elem_encl_struct *)(((fbe_u8_t *)encl_status_diag_page_hdr) + CHASSIS_ENCL_STAT_ELEM_OFFSET);
            encl_stat_rtn_ptr++;
            MUT_ASSERT_INT_EQUAL(SES_STAT_CODE_OK, encl_stat_rtn_ptr->cmn_stat.elem_stat_code);
            MUT_ASSERT_INT_EQUAL(0, encl_stat_rtn_ptr->warning_indicaton);
            MUT_ASSERT_INT_EQUAL(0, encl_stat_rtn_ptr->failure_indicaton);
            MUT_ASSERT_INT_EQUAL(0, encl_stat_rtn_ptr->ident);
        }
        else if(*return_page_code == SES_PG_CODE_ADDL_ELEM_STAT)
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: opcode:0x%x,return_status: %d\n", __FUNCTION__, cdb[0], cdb_request_status);
            addl_elem_stat_page_hdr = (ses_common_pg_hdr_struct *)fbe_sg_element_address(sg_list);    
            terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: diagnostic page code returned ->0x%x\n", __FUNCTION__, addl_elem_stat_page_hdr->pg_code);
            dev_phy_desc_ptr = (ses_array_dev_phy_desc_struct *)((fbe_u8_t *)addl_elem_stat_page_hdr + FBE_ESES_PAGE_HEADER_SIZE);
            for(i=0; i<MAX_SLOT_NUMBER; i++)
            {
                dev_phy_desc_ptr = 
                    (ses_array_dev_phy_desc_struct *)
                    (((fbe_u8_t *)dev_phy_desc_ptr) +
                     FBE_ESES_ADDL_ELEM_STATUS_DESC_HDR_SIZE +
                     FBE_ESES_ARRAY_DEV_SLOT_PROT_SPEC_INFO_HEADER_SIZE );
                
                /******************************************
                if(i==0)
                {
                    MUT_ASSERT_TRUE(dev_phy_desc_ptr->sas_address == 0x987654321);
                    MUT_ASSERT_TRUE(dev_phy_desc_ptr->phy_id == i);
                }
                else if(i==4)
                {
                    MUT_ASSERT_TRUE(dev_phy_desc_ptr->sas_address == 0x987654327);
                    MUT_ASSERT_TRUE(dev_phy_desc_ptr->phy_id == i);
                }
                ********************************************/
                dev_phy_desc_ptr ++;

            }

/*******************************************************************************************************
 * Comment ESC electronic elements as we are not considering them now
 *
            esc_elec_exp_phy_desc_ptr = (ses_esc_elec_exp_phy_desc_struct *)
                ((fbe_u8_t *)(dev_phy_desc_ptr) +
                FBE_ESES_ADDL_ELEM_STATUS_DESC_HDR_SIZE +
                FBE_ESES_ESC_ELEC_PROT_SPEC_INFO_HEADER_SIZE);
            for (i=0; i<24; i++)
            {
                MUT_ASSERT_TRUE(esc_elec_exp_phy_desc_ptr->phy_id == addl_stat_page_esc_elec_phy_id[i]);
                if(i<15)
                {                  
                    //MUT_ASSERT_TRUE(esc_elec_exp_phy_desc_ptr->other_elem_index == addl_stat_page_esc_elec_other_elem_index[i]);

                }
                else
                {
                   //MUT_ASSERT_TRUE(esc_elec_exp_phy_desc_ptr->other_elem_index == 0xFF);
                }
                //MUT_ASSERT_TRUE(esc_elec_exp_phy_desc_ptr->conn_elem_index == 0xFF);

                esc_elec_exp_phy_desc_ptr++;
            }
            // SAS Expander Elements Test.
**************************************************************************************************/
            sas_exp_prot_spec_info_ptr = (ses_sas_exp_prot_spec_info_struct *)
                ((fbe_u8_t *)(dev_phy_desc_ptr) +
                 FBE_ESES_ADDL_ELEM_STATUS_DESC_HDR_SIZE);  
            MUT_ASSERT_TRUE((BYTE_SWAP_64(sas_exp_prot_spec_info_ptr->sas_address)) == 
                addl_elem_sas_exp_sas_addr[encl_id]);

            sas_exp_phy_desc_ptr  = (ses_sas_exp_phy_desc_struct *)
                ((fbe_u8_t *)(dev_phy_desc_ptr) +
                FBE_ESES_ADDL_ELEM_STATUS_DESC_HDR_SIZE +
                FBE_ESES_SAS_EXP_PROT_SPEC_INFO_HEADER_SIZE);   
            for (i=0; i<MAX_PHY_NUMBER; i++)
            {
                MUT_ASSERT_TRUE(sas_exp_phy_desc_ptr->conn_elem_index == addl_stat_page_sas_exp_conn_elem_index[i]);
                MUT_ASSERT_TRUE(sas_exp_phy_desc_ptr->other_elem_index == addl_stat_page_sas_exp_other_elem_index[i]);
                sas_exp_phy_desc_ptr++;
            }
        }
      
    }
    fbe_semaphore_release(&update_semaphore, 0, 1, FALSE);

    return FBE_STATUS_OK;
}
/****************************************************************
 * fbe_ldo_test_alloc_memory_with_sg()
 ****************************************************************
 * DESCRIPTION:
 *  This function allocates an sg list with (bytes) worth of
 *  associated memory.
 *
 * PARAMETERS:
 *  bytes, [I] - Bytes to allocate.
 *
 * RETURNS:
 *  Returns the newly allocated sg list with memory.
 *  NULL Indicates failure.
 *
 * HISTORY:
 *  11/26/07 - Created. RPF
 *
 ****************************************************************/
fbe_sg_element_t * fbe_ldo_test_alloc_memory_with_sg( fbe_u32_t bytes )
{
    fbe_sg_element_t *sg_p;
    fbe_u8_t *sectors_p;

    /* Allocate the memory.
     */
    sectors_p = malloc(bytes);
    memset(sectors_p, 0, bytes*sizeof(fbe_u8_t));
    
    if (sectors_p == NULL)
    {
        return NULL;
    }

    /* Allocate the sg.
     */
    sg_p = malloc(sizeof(fbe_sg_element_t) * 5);
    memset(sg_p, 0, sizeof(fbe_sg_element_t) * 5);

    if (sectors_p == NULL)
    {
        free(sectors_p);
        return NULL;
    }

    /* Init the sgs with the memory we just allocated.
     */
    fbe_sg_element_init(&sg_p[0], bytes, sectors_p);
    fbe_sg_element_terminate(&sg_p[1]);
    
    return sg_p;
}

/****************************************************************
 * fbe_ldo_test_free_memory_with_sg()
 ****************************************************************
 * DESCRIPTION:
 *  This function frees an sg list and associated memory.
 *
 * PARAMETERS:
 *  sg_p, [IO] - SG list with memory to free.
 *
 * RETURNS:
 *  None.
 *
 * HISTORY:
 *  11/26/07 - Created. RPF
 *
 ****************************************************************/
void fbe_ldo_test_free_memory_with_sg( fbe_sg_element_t **sg_p )
{
    /* If the sg ptr is not null, then free it.
     */
    if (*sg_p != NULL)
    {
        /* If the memory inside the sg list is not null,
         * free it as well.
         */
        if (fbe_sg_element_address(*sg_p) != NULL)
        {
            free(fbe_sg_element_address(*sg_p));
        }
        free(*sg_p);
        
        /* Set this to NULL to indicate it is already freed.
         */
        *sg_p = NULL;
    }
    return;
}
/* end fbe_ldo_test_free_memory_with_sg() */

void init_dev_phy_elems(void)
{
    fbe_u8_t i,j;

    for(j=0; j<10; j++)
    {
        for (i=0; i<MAX_SLOT_NUMBER; i++)
        {
            memset(&array_dev_slot_stat_ptr[j][i], 0, sizeof(ses_stat_elem_array_dev_slot_struct));
            array_dev_slot_stat_ptr[j][i].cmn_stat.elem_stat_code = SES_STAT_CODE_NOT_INSTALLED;
        }
        for (i=0; i<MAX_PHY_NUMBER; i++)
        {
            memset(&exp_phy_stat_ptr[j][i], 0, sizeof(ses_stat_elem_exp_phy_struct));
            exp_phy_stat_ptr[j][i].cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
        }
    }
    return;
}

void init_addl_stat_page_strts(void)
{
    int i;

    for(i=0; i<24; i++)
    {
        addl_stat_page_esc_elec_other_elem_index[i] = i;
        addl_stat_page_esc_elec_phy_id[i] = i;
    }

    for(i=0; i<10; i++)
    {
        addl_elem_sas_exp_sas_addr[i] = 0;
    }
    return;

}

void init_ps_elem_strts(void)
{
    int i;

    for(i=0;i<=10;i++)
    {
        memset(&ps_stat[i][PS_0], 0, sizeof(ses_stat_elem_ps_struct));
        ps_stat[i][PS_0].cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
        ps_stat[i][PS_0].off = 0;
        ps_stat[i][PS_0].dc_fail = 0;
        ps_stat[i][PS_0].rqsted_on = 1;
        ps_stat[i][PS_0].ac_fail = 0;

        memset(&ps_stat[i][PS_1], 0, sizeof(ses_stat_elem_ps_struct));
        ps_stat[i][PS_1].cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
        ps_stat[i][PS_1].off = 0;
        ps_stat[i][PS_1].dc_fail = 0;
        ps_stat[i][PS_1].rqsted_on = 1;
        ps_stat[i][PS_1].ac_fail = 0;
    }
}

void init_cooling_elem_strts(void)
{
    int i, j;

    for(i=0;i<=10;i++)
    {
        for(j=0;j<MAX_COOLING_NUMBER;j++)
        {
            memset(&cooling_stat[i][j], 0, sizeof(ses_stat_elem_cooling_struct));
            cooling_stat[i][j].cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
            cooling_stat[i][j].actual_fan_speed_high = 0;
            cooling_stat[i][j].actual_fan_speed_low = 0x10;
            cooling_stat[i][j].rqsted_on = 1;
            cooling_stat[i][j].off = 0;
            cooling_stat[i][j].actual_speed_code = 7;
        }
    }
}

void init_temp_sensor_elem_strts(void)
{
    int i, j;

    for(i=0;i<=10;i++)
    {
        for(j=0;j<MAX_TEMP_SENSOR_NUMBER;j++)
        {
            
            // overall temp status elem
            memset(&overall_temp_sensor_stat[i][j], 0, sizeof(ses_stat_elem_temp_sensor_struct));
            overall_temp_sensor_stat[i][j].cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
            overall_temp_sensor_stat[i][j].ot_failure = 0;
            overall_temp_sensor_stat[i][j].ot_warning = 0;
            overall_temp_sensor_stat[i][j].ident = 0;
            overall_temp_sensor_stat[i][j].temp = 64;            
            
            // individual temp status elem
            memset(&temp_sensor_stat[i][j], 0, sizeof(ses_stat_elem_temp_sensor_struct));
            temp_sensor_stat[i][j].cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
            temp_sensor_stat[i][j].ot_failure = 0;
            temp_sensor_stat[i][j].ot_warning = 0;
            temp_sensor_stat[i][j].ident = 0;
            temp_sensor_stat[i][j].temp = 64;
        }
    }
}

// only a basic test, not much for now...
void terminator_addl_status_page_test(void)
{
    fbe_status_t                    status;
    fbe_terminator_board_info_t     board_info;
    fbe_terminator_sas_port_info_t  sas_port;
    fbe_terminator_sas_encl_info_t  sas_encl;
    fbe_terminator_sas_drive_info_t sas_drive;
    fbe_terminator_api_device_handle_t        encl_id;
    fbe_terminator_api_device_handle_t        drive_id;

    fbe_terminator_api_device_handle_t        device_list[MAX_DEVICES];
    fbe_u32_t                       total_devices = 0;
    fbe_cpd_shim_enumerate_backend_ports_t cpd_shim_enumerate_backend_ports;
    fbe_u32_t                       i = 0;
    fbe_u32_t                       context = 0;
    fbe_terminator_device_ptr_t handle;
    fbe_u32_t       port_index;
    fbe_sg_element_t *sg_p = NULL;

    mut_printf(MUT_LOG_HIGH, "%s %s ",__FILE__, __FUNCTION__);

    fbe_semaphore_init(&update_semaphore, 0, FBE_SEMAPHORE_MAX);
    init_addl_stat_page_strts();

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
    sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    sas_port.io_port_number = 0;
    sas_port.portal_number = 1;
    sas_port.backend_number = 0;
    status  = fbe_terminator_api_insert_sas_port (0, &sas_port);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_terminator_miniport_api_port_init(0, 1, &port_index);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Register a callback to port 0, so we can destroy the object properly */
    status = fbe_terminator_miniport_api_register_callback(port_index, fbe_cpd_shim_callback_function_mock, NULL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* enumerate the ports */
    status  = fbe_terminator_miniport_api_enumerate_cpd_ports (&cpd_shim_enumerate_backend_ports);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/* Peter Puhov: I think it is wrong test
    for (i=0; i<cpd_shim_enumerate_backend_ports.number_of_io_ports; i++)
    {
        MUT_ASSERT_TRUE(i==cpd_shim_enumerate_backend_ports.io_port_array[i])
    }
	*/

    /* enumerate the devices on ports */
    terminator_get_port_ptr_by_port_portal(0, 1, &handle);
	status = terminator_enumerate_devices (handle, device_list, &total_devices);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(0, total_devices);

    /*insert an enclosure*/
    // id 2
    sas_encl.uid[0] = 2; // some unique ID.
    sas_encl.sas_address = (fbe_u64_t)0x123456780;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    sas_encl.connector_id = 0;
    status  = fbe_terminator_api_insert_sas_enclosure (0, &sas_encl, &encl_id);/*add encl on port 0*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    addl_elem_sas_exp_sas_addr[2] = 0x123456780;

    /*insert an enclosure*/
    // id 4
    sas_encl.uid[0] = 4; // some unique ID.
    sas_encl.sas_address = (fbe_u64_t)0x123456782;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    sas_encl.connector_id = 0;
    status  = fbe_terminator_api_insert_sas_enclosure (0, &sas_encl, &encl_id);/*add encl on port 0*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    addl_elem_sas_exp_sas_addr[4] = 0x123456782;

    /*insert an enclosure*/
    // id 6
    sas_encl.uid[0] = 6; // some unique ID.
    sas_encl.sas_address = (fbe_u64_t)0x123456783;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    sas_encl.connector_id = 0;
    status  = fbe_terminator_api_insert_sas_enclosure (0, &sas_encl, &encl_id);/*add encl on port 0*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    addl_elem_sas_exp_sas_addr[6] = 0x123456783;

    /* enumerate the devices on ports */
    terminator_get_port_ptr_by_port_portal(0, 1, &handle);
	status = terminator_enumerate_devices (handle, device_list, &total_devices);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(6, total_devices);

    /*insert a sas_drive to the second last enclosure*/
    // inserted in encl id 4
    sas_drive.sas_address = (fbe_u64_t)0x987654321;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.capacity = 0x10BD0;
    sas_drive.block_size = 520;
    status = fbe_terminator_api_insert_sas_drive (0, encl_id-2, 0, &sas_drive, &drive_id);/*add drive to port 0 encl 3*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // insert in slot 4  for encl id 4 
    sas_drive.sas_address = (fbe_u64_t)0x987654327;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.capacity = 0x10BD0;
    sas_drive.block_size = 520;
    status = fbe_terminator_api_insert_sas_drive (0, encl_id-2, 4, &sas_drive, &drive_id);/*add drive to port 0 encl 3*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status =  fbe_terminator_miniport_api_register_payload_completion(0,
                                                                    terminator_miniport_api_test_PHY_io_complete,
                                                                    &device_info);

    // allocate memory for the sglist
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);

    // test by issuing to the virtual phy.
    // Issue eses command.
    device_info.port = 0;
    device_info.encl_id = 2; 
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_addl_elem_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&update_semaphore, NULL);

    device_info.port = 0;
    device_info.encl_id = 4; 
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_addl_elem_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&update_semaphore, NULL);

    device_info.port = 0;
    device_info.encl_id = 6; 
    status = terminator_miniport_api_issue_eses_cmd(0, 7, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_addl_elem_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&update_semaphore, NULL);

    status = fbe_terminator_api_destroy();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator destroyed.", __FUNCTION__);
 }
