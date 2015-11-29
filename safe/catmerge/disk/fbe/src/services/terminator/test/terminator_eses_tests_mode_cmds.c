/***************************************************************************
 *  terminator_eses_tests_mode_cmds.c
 ***************************************************************************
 *
 *  Description
 *      ESES enclosure tests related to mode scsi commands (mode sense, mode select)
 *      
 *
 *  History:
 *     Mar-15-09   Rajesh V. Created
 *    
 ***************************************************************************/

/***************************************************************************
 *  terminator_eses_tests_mode_cmds.c
 ***************************************************************************
 *
 *  Description
 *      ESES enclosure tests for mode SCSI commands (mode sense, mode select)
 *
 *  NOTE: 
 *
 *  History:
 *    Mar-15-09   Rajesh V. Created
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

#define TERMINATOR_ESES_MODE_CMDS_TEST_EEP_PAGE_SIZE            16
#define TERMINATOR_ESES_MODE_CMDS_TEST_EENP_PAGE_SIZE           16
#define TERMINATOR_ESES_MODE_CMDS_MODE_PARAM_LIST_HEADER_SIZE    8

/**********************************/
/*        local variables        */
/**********************************/
typedef struct mode_cmds_test_io_completion_context_s
{
    fbe_u8_t encl_id;
    fbe_bool_t disable_indicator_ctrl;
    fbe_bool_t ssu_disable;
    fbe_bool_t ha_mode;
    fbe_bool_t bad_exp_recovery_enabled;
    fbe_bool_t test_mode;
}mode_cmds_test_io_completion_context_t;

static mode_cmds_test_io_completion_context_t mode_cmds_test_io_completion_context;
static fbe_semaphore_t   mode_cmds_test_continue_semaphore;
static fbe_u32_t mode_cmds_test_io_completion_call_count;

/**********************************/
/*       LOCAL FUNCTIONS          */
/**********************************/
static fbe_status_t mode_cmds_test_io_completion (fbe_payload_ex_t * payload, fbe_payload_ex_completion_context_t context);
static fbe_status_t mode_cmds_test_callback(fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context);
static void mode_cmds_test_init_io_completion_context(void);


void mode_cmds_test_init_io_completion_context(void)
{
    memset(&mode_cmds_test_io_completion_context, 0, sizeof(mode_cmds_test_io_completion_context_t));
}

static fbe_status_t mode_cmds_test_callback(fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context)
{
    return FBE_STATUS_OK;
}


void terminator_mode_cmds_test(void)
{
    fbe_status_t                    status;
    fbe_terminator_board_info_t     board_info;
    fbe_terminator_sas_port_info_t  sas_port;
    fbe_terminator_sas_encl_info_t  sas_encl;
    fbe_terminator_sas_drive_info_t sas_drive;
    fbe_terminator_api_device_handle_t        encl_id;
    fbe_terminator_api_device_handle_t        drive_id;
    fbe_terminator_api_device_handle_t        encl_id0, encl_id1;
    fbe_sg_element_t *sg_p = NULL;
    fbe_u32_t  port_index;
    mode_cmds_test_mode_sense_cdb_param_t mode_sense_cdb_param;
    mode_cmds_test_mode_select_cdb_param_t mode_select_cdb_param;
    fbe_eses_mode_param_list_t *mode_param_list = NULL;
    fbe_eses_pg_eep_mode_pg_t *eep_mode_page = NULL;
    fbe_eses_pg_eenp_mode_pg_t *eenp_mode_page = NULL;

    mut_printf(MUT_LOG_HIGH, "%s %s \n",__FILE__, __FUNCTION__);

    fbe_semaphore_init(&mode_cmds_test_continue_semaphore, 0, FBE_SEMAPHORE_MAX);
    mode_cmds_test_init_io_completion_context();

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
                                                           mode_cmds_test_callback, 
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

    // Register the IO completion function
	status = fbe_terminator_miniport_api_register_payload_completion(0,
																    mode_cmds_test_io_completion,
																    &mode_cmds_test_io_completion_context);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 

	status = fbe_terminator_api_get_enclosure_handle(0, 0, &encl_id0);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	status = fbe_terminator_api_get_enclosure_handle(0, 0, &encl_id1);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // Test 1. issue a mode sense command to get EEP mode page
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    mode_sense_cdb_param.pc = 0x00;
    mode_sense_cdb_param.pg_code = 0x20;
    mode_sense_cdb_param.subpg_code = 0x00;
    status = terminator_miniport_api_issue_eses_cmd(0, 
                                                    (encl_id1 + 1), 
                                                    FBE_SCSI_MODE_SENSE_10, 
                                                    &mode_sense_cdb_param, 
                                                    sg_p);
    fbe_semaphore_wait(&mode_cmds_test_continue_semaphore, NULL);

    // Test 2. issue a mode sense command to get EENP persistent mode page
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    mode_sense_cdb_param.pc = 0x00;
    mode_sense_cdb_param.pg_code = 0x21;
    mode_sense_cdb_param.subpg_code = 0x00;
    status = terminator_miniport_api_issue_eses_cmd(0, 
                                                    (encl_id1 + 1), 
                                                    FBE_SCSI_MODE_SENSE_10, 
                                                    &mode_sense_cdb_param, 
                                                    sg_p);
    fbe_semaphore_wait(&mode_cmds_test_continue_semaphore, NULL);

    // Test 3. issue a mode sense command to get both EEP & EENP pages.
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    mode_sense_cdb_param.pc = 0x00;
    mode_sense_cdb_param.pg_code = 0x3F;
    mode_sense_cdb_param.subpg_code = 0xFF;
    status = terminator_miniport_api_issue_eses_cmd(0, 
                                                    (encl_id1 + 1), 
                                                    FBE_SCSI_MODE_SENSE_10, 
                                                    &mode_sense_cdb_param, 
                                                    sg_p);
    fbe_semaphore_wait(&mode_cmds_test_continue_semaphore, NULL);

    // Test 4. 
    // Issue a mode select and set test mode by sending only EENP page.
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    mode_param_list = (fbe_eses_mode_param_list_t *)fbe_sg_element_address(sg_p);
    memset(mode_param_list, 0, sizeof(fbe_eses_mode_param_list_t));
    eenp_mode_page = (fbe_eses_pg_eenp_mode_pg_t *)(mode_param_list + 1);
    memset(eenp_mode_page, 0, sizeof(fbe_eses_pg_eenp_mode_pg_t));
    eenp_mode_page->mode_pg_common_hdr.pg_code = SES_PG_CODE_EMC_ESES_NON_PERSISTENT_MODE_PG;
    eenp_mode_page->mode_pg_common_hdr.pg_len = FBE_ESES_EMC_ESES_NON_PERSISTENT_MODE_PAGE_LEN;
    eenp_mode_page->test_mode = 1;
    mode_select_cdb_param.param_list_len = 
        TERMINATOR_ESES_MODE_CMDS_TEST_EENP_PAGE_SIZE + 
        TERMINATOR_ESES_MODE_CMDS_MODE_PARAM_LIST_HEADER_SIZE;
    status = terminator_miniport_api_issue_eses_cmd(0, 
                                                    (encl_id1 + 1), 
                                                    FBE_SCSI_MODE_SELECT_10, 
                                                    &mode_select_cdb_param, 
                                                    sg_p);    
    fbe_semaphore_wait(&mode_cmds_test_continue_semaphore, NULL);
    // issue a mode sense command to get EENP persistent mode page
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    mode_sense_cdb_param.pc = 0x00;
    mode_sense_cdb_param.pg_code = 0x21;
    mode_sense_cdb_param.subpg_code = 0x00;
    status = terminator_miniport_api_issue_eses_cmd(0, 
                                                    (encl_id1 + 1), 
                                                    FBE_SCSI_MODE_SENSE_10, 
                                                    &mode_sense_cdb_param, 
                                                    sg_p);
    fbe_semaphore_wait(&mode_cmds_test_continue_semaphore, NULL);

    // Test 5. 
    // Issue a mode select and clear test mode and set ha mode by sending
    // both EEP & EENP pages.
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    mode_param_list = (fbe_eses_mode_param_list_t *)fbe_sg_element_address(sg_p);
    memset(mode_param_list, 0, sizeof(fbe_eses_mode_param_list_t));
    eenp_mode_page = (fbe_eses_pg_eenp_mode_pg_t *)(mode_param_list + 1);
    memset(eenp_mode_page, 0, sizeof(fbe_eses_pg_eenp_mode_pg_t));
    eenp_mode_page->mode_pg_common_hdr.pg_code = SES_PG_CODE_EMC_ESES_NON_PERSISTENT_MODE_PG;
    eenp_mode_page->mode_pg_common_hdr.pg_len = FBE_ESES_EMC_ESES_NON_PERSISTENT_MODE_PAGE_LEN;
    eenp_mode_page->test_mode = 0;
    //eep mode page
    eep_mode_page = (fbe_eses_pg_eep_mode_pg_t *)(eenp_mode_page + 1);
    memset(eep_mode_page, 0, sizeof(fbe_eses_pg_eep_mode_pg_t));
    eep_mode_page->mode_pg_common_hdr.pg_code = SES_PG_CODE_EMC_ESES_PERSISTENT_MODE_PG;
    eep_mode_page->mode_pg_common_hdr.pg_len = FBE_ESES_EMC_ESES_PERSISTENT_MODE_PAGE_LEN;
    eep_mode_page->ha_mode = 1;
    mode_select_cdb_param.param_list_len = 
        TERMINATOR_ESES_MODE_CMDS_TEST_EENP_PAGE_SIZE +
        TERMINATOR_ESES_MODE_CMDS_TEST_EEP_PAGE_SIZE +
        TERMINATOR_ESES_MODE_CMDS_MODE_PARAM_LIST_HEADER_SIZE;
    status = terminator_miniport_api_issue_eses_cmd(0, 
                                                    (encl_id1 + 1), 
                                                    FBE_SCSI_MODE_SELECT_10, 
                                                    &mode_select_cdb_param, 
                                                    sg_p);    
    fbe_semaphore_wait(&mode_cmds_test_continue_semaphore, NULL);
    // issue a mode sense command to get EEP & EENP  mode pages
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    mode_sense_cdb_param.pc = 0x00;
    mode_sense_cdb_param.pg_code = 0x3F;
    mode_sense_cdb_param.subpg_code = 0x00;
    status = terminator_miniport_api_issue_eses_cmd(0, 
                                                    (encl_id1 + 1), 
                                                    FBE_SCSI_MODE_SENSE_10, 
                                                    &mode_sense_cdb_param, 
                                                    sg_p);
    fbe_semaphore_wait(&mode_cmds_test_continue_semaphore, NULL);



    // Test 6.
    // Issue a mode select and clear ha mode & set disable indicator control
    // by issuing only a EEP mode page
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    mode_param_list = (fbe_eses_mode_param_list_t *)fbe_sg_element_address(sg_p);
    memset(mode_param_list, 0, sizeof(fbe_eses_mode_param_list_t));
    eep_mode_page = (fbe_eses_pg_eep_mode_pg_t *)(mode_param_list + 1);
    memset(eep_mode_page, 0, sizeof(fbe_eses_pg_eep_mode_pg_t));
    eep_mode_page->mode_pg_common_hdr.pg_code = SES_PG_CODE_EMC_ESES_PERSISTENT_MODE_PG;
    eep_mode_page->mode_pg_common_hdr.pg_len = FBE_ESES_EMC_ESES_PERSISTENT_MODE_PAGE_LEN;
    eep_mode_page->ha_mode = 0;
    eep_mode_page->disable_indicator_ctrl = 1;
    mode_select_cdb_param.param_list_len = 
        TERMINATOR_ESES_MODE_CMDS_TEST_EEP_PAGE_SIZE +
        TERMINATOR_ESES_MODE_CMDS_MODE_PARAM_LIST_HEADER_SIZE;
    status = terminator_miniport_api_issue_eses_cmd(0, 
                                                    (encl_id1 + 1), 
                                                    FBE_SCSI_MODE_SELECT_10, 
                                                    &mode_select_cdb_param, 
                                                    sg_p);    
    fbe_semaphore_wait(&mode_cmds_test_continue_semaphore, NULL);
    // issue a mode sense command to get EEP  mode page
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    mode_sense_cdb_param.pc = 0x00;
    mode_sense_cdb_param.pg_code = 0x20;
    mode_sense_cdb_param.subpg_code = 0x00;
    status = terminator_miniport_api_issue_eses_cmd(0, 
                                                    (encl_id1 + 1), 
                                                    FBE_SCSI_MODE_SENSE_10, 
                                                    &mode_sense_cdb_param, 
                                                    sg_p);
    fbe_semaphore_wait(&mode_cmds_test_continue_semaphore, NULL);

   
    status = fbe_terminator_api_destroy();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator destroyed.", __FUNCTION__);
}


static fbe_status_t mode_cmds_test_io_completion (fbe_payload_ex_t * payload, fbe_payload_ex_completion_context_t context)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_cdb_operation_t *payload_cdb_operation;
    fbe_u8_t *cdb = NULL;
    fbe_sg_element_t *sg_list = NULL;
    fbe_u8_t encl_id = ((mode_cmds_test_io_completion_context_t *)context)->encl_id;
    static mode_cmds_test_io_completion_call_count = 0;

    mode_cmds_test_io_completion_call_count++;
    //mut_printf(MUT_LOG_HIGH, "%s: called %d th time\n ",__FUNCTION__, mode_cmds_test_io_completion_call_count);

    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    MUT_ASSERT_TRUE(payload_cdb_operation != NULL);   
    fbe_payload_cdb_operation_get_cdb(payload_cdb_operation, &cdb); 
    status = fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    if (cdb[0] == FBE_SCSI_MODE_SENSE_10)
    {   
        // For now ,  just verify the values (like test mode) in debugger.
        mut_printf(MUT_LOG_HIGH, "%s: mode sense command completed\n ",__FUNCTION__);
    }
    else if(cdb[0] == FBE_SCSI_MODE_SELECT_10)
    {
         // For now ,  just verify the values (like test mode) in debugger.
         mut_printf(MUT_LOG_HIGH, "%s: mode select command completed\n ",__FUNCTION__);
    }

    fbe_semaphore_release(&mode_cmds_test_continue_semaphore, 0, 1, FALSE);
    fbe_ldo_test_free_memory_with_sg(&sg_list);
    return(FBE_STATUS_OK);
}
