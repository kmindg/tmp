/***************************************************************************
 *  terminator_eses_tests_config_page.c
 ***************************************************************************
 *
 *  Description
 *      ESES enclosure configuration page related tests.
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

/**********************************/
/*        local variables        */
/**********************************/
typedef struct config_page_test_io_completion_context_s
{
    fbe_u8_t encl_id;
    fbe_u8_t unit_attention[10];
    fbe_u32_t gen_code[10];
    // used exclusively for the ver
    // descriptor test.
    ses_subencl_type_enum subencl_type;
    fbe_u8_t side;
    fbe_u8_t comp_type;
    ses_ver_desc_struct ver_desc;
}config_page_test_io_completion_context_t;

static config_page_test_io_completion_context_t config_page_test_io_completion_context;
static fbe_semaphore_t   config_page_test_continue_semaphore;

/**********************************/
/*       LOCAL FUNCTIONS          */
/**********************************/
static fbe_status_t terminator_eses_tests_config_page_gen_code_io_completion (fbe_payload_ex_t * payload, fbe_payload_ex_completion_context_t context);
static fbe_status_t terminator_eses_tests_config_page_callback(fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context);
static void init_config_page_test_io_completion_context(void);
static fbe_status_t terminator_eses_tests_config_page_get_ver_desc(fbe_u8_t *config_page,
                                                            ses_subencl_type_enum subencl_type,
                                                            fbe_u8_t side,
                                                            fbe_u8_t  comp_type,
                                                            ses_ver_desc_struct *ver_desc);

static fbe_status_t terminator_eses_tests_config_page_ver_desc_io_completion (fbe_payload_ex_t * payload, fbe_payload_ex_completion_context_t context);


void terminator_config_page_gen_code_test(void)
{
    fbe_status_t                    status;
    fbe_terminator_board_info_t     board_info;
    fbe_terminator_sas_port_info_t  sas_port;
    fbe_terminator_sas_encl_info_t  sas_encl;
    fbe_terminator_sas_drive_info_t sas_drive;
    fbe_terminator_api_device_handle_t   encl_id;
    fbe_terminator_api_device_handle_t   drive_id;
    fbe_sg_element_t *sg_p = NULL;
    fbe_u32_t  port_index;

    mut_printf(MUT_LOG_HIGH, "%s %s ",__FILE__, __FUNCTION__);

    fbe_semaphore_init(&config_page_test_continue_semaphore, 0, FBE_SEMAPHORE_MAX);
    init_config_page_test_io_completion_context();

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
                                                           terminator_eses_tests_config_page_callback, 
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



	/* now lets verify the status */
	status = fbe_terminator_miniport_api_register_payload_completion(0,
																    terminator_eses_tests_config_page_gen_code_io_completion,
																    &config_page_test_io_completion_context);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);   


    config_page_test_io_completion_context.encl_id = 4;
    // Test 1 issue request for config page
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_config, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);

    status = fbe_terminator_api_eses_increment_config_page_gen_code(0, 4);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    config_page_test_io_completion_context.gen_code[4]++;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_config, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);

    status = fbe_terminator_api_set_unit_attention(0, 4);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    config_page_test_io_completion_context.unit_attention[4] = 1;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_config, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);

    // Unit attention should only be set once
    config_page_test_io_completion_context.unit_attention[4] = 0;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);

    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_addl_elem_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);

    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_download_microcode_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);

    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_emc_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);


    // All the below tests issue ESES commands to second enclosure.
    // Test 2 issue request for config page 
    config_page_test_io_completion_context.encl_id = 2;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_config, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);

    status = fbe_terminator_api_eses_increment_config_page_gen_code(0, 2);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    config_page_test_io_completion_context.gen_code[2]++;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_config, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);

    status = fbe_terminator_api_set_unit_attention(0, 2);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    config_page_test_io_completion_context.unit_attention[2] = 1;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_config, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);

    // Unit attention should only be set once.
    config_page_test_io_completion_context.unit_attention[2] = 0;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);

    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_addl_elem_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);

    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_download_microcode_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);

    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_emc_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);

    status = fbe_terminator_api_eses_increment_config_page_gen_code(0, 2);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    config_page_test_io_completion_context.gen_code[2]++;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_config, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);

    status = fbe_terminator_api_set_unit_attention(0, 2);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    config_page_test_io_completion_context.unit_attention[2] = 1;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_download_microcode_stat, sg_p);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);

    // Unit attention should only be set once
    config_page_test_io_completion_context.unit_attention[2] = 0;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_download_microcode_stat, sg_p);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);

    status = fbe_terminator_api_destroy();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator destroyed.", __FUNCTION__);

}

void terminator_magnum_enclosure_test(void)
{
    fbe_status_t                    status;
    fbe_terminator_board_info_t     board_info;
    fbe_terminator_sas_port_info_t  sas_port;
    fbe_terminator_sas_encl_info_t  sas_encl;
    fbe_terminator_sas_drive_info_t sas_drive;
    fbe_terminator_api_device_handle_t   encl_id;
    fbe_terminator_api_device_handle_t   drive_id;
    fbe_terminator_api_device_handle_t        encl_id0, encl_id1;
    fbe_sg_element_t *sg_p = NULL;
    fbe_u32_t  port_index;

    mut_printf(MUT_LOG_HIGH, "%s %s ",__FILE__, __FUNCTION__);

    fbe_semaphore_init(&config_page_test_continue_semaphore, 0, FBE_SEMAPHORE_MAX);
    init_config_page_test_io_completion_context();

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
                                                           terminator_eses_tests_config_page_callback, 
                                                           NULL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);


    /*insert an enclosure*/
    // id 2
    sas_encl.uid[0] = 2; // some unique ID.
    sas_encl.sas_address = (fbe_u64_t)0x123456780;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_MAGNUM;
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

	status = fbe_terminator_api_get_enclosure_handle(0, 0, &encl_id0);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	status = fbe_terminator_api_get_enclosure_handle(0, 1, &encl_id1);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);


    /*insert a sas_drive to the magnum enclosure*/
    sas_drive.sas_address = (fbe_u64_t)0x987654321;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.capacity = 0x10BD0;
    sas_drive.block_size = 520;
    status = fbe_terminator_api_insert_sas_drive (0, encl_id0, 0, &sas_drive, &drive_id);/*add drive to port 0 encl 3*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);



	/* now lets verify the status */
	status = fbe_terminator_miniport_api_register_payload_completion(0,
																    terminator_eses_tests_config_page_gen_code_io_completion,
																    &config_page_test_io_completion_context);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);   


    // All the below tests issue ESES commands to second enclosure.
    // Test 2 issue request for config page 
    config_page_test_io_completion_context.encl_id = encl_id0;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, encl_id0+1, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_config, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);

    status = fbe_terminator_api_eses_increment_config_page_gen_code(0, encl_id0);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    config_page_test_io_completion_context.gen_code[encl_id0]++;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, encl_id0+1, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_config, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);

    status = fbe_terminator_api_set_unit_attention(0, encl_id0);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    config_page_test_io_completion_context.unit_attention[encl_id0] = 1;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, encl_id0+1, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_config, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);

    // Unit attention should only be set once.
    config_page_test_io_completion_context.unit_attention[encl_id0] = 0;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, encl_id0+1, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);

    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, encl_id0+1, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_addl_elem_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);

    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, encl_id0+1, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_download_microcode_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);

    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, encl_id0+1, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_emc_encl_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);

    status = fbe_terminator_api_eses_increment_config_page_gen_code(0, encl_id0);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    config_page_test_io_completion_context.gen_code[encl_id0]++;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, encl_id0+1, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_config, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);

    status = fbe_terminator_api_set_unit_attention(0, encl_id0);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    config_page_test_io_completion_context.unit_attention[encl_id0] = 1;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, encl_id0+1, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_download_microcode_stat, sg_p);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);

    // Unit attention should only be set once
    config_page_test_io_completion_context.unit_attention[encl_id0] = 0;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, encl_id0+1, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_download_microcode_stat, sg_p);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);

    status = fbe_terminator_api_destroy();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator destroyed.", __FUNCTION__);    
}

void init_config_page_test_io_completion_context(void)
{
    memset(&config_page_test_io_completion_context, 0, sizeof(config_page_test_io_completion_context));
}

static fbe_status_t terminator_eses_tests_config_page_callback(fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context)
{
    return FBE_STATUS_OK;
}


static fbe_status_t terminator_eses_tests_config_page_gen_code_io_completion (fbe_payload_ex_t * payload, fbe_payload_ex_completion_context_t context)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_cdb_operation_t *payload_cdb_operation = NULL;
    fbe_u8_t *cdb = NULL;
    fbe_sg_element_t *sg_list = NULL;
    fbe_u8_t *sense_info_buffer = NULL;
    ses_common_pg_hdr_struct *page_hdr = NULL;
    fbe_u8_t encl_id = ((config_page_test_io_completion_context_t *)context)->encl_id;
    fbe_u8_t unit_attention = ((config_page_test_io_completion_context_t *)context)->unit_attention[encl_id];
    fbe_u8_t gen_code = ((config_page_test_io_completion_context_t *)context)->gen_code[encl_id];

    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    MUT_ASSERT_TRUE(payload_cdb_operation != NULL);   
    fbe_payload_cdb_operation_get_cdb(payload_cdb_operation, &cdb); 
    MUT_ASSERT_TRUE(payload_cdb_operation != NULL);
    fbe_payload_cdb_operation_get_sense_buffer(payload_cdb_operation, &sense_info_buffer);
    MUT_ASSERT_TRUE(sense_info_buffer != NULL);
    status = fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    if(unit_attention)
    {
        MUT_ASSERT_INT_EQUAL(FBE_SCSI_SENSE_KEY_UNIT_ATTENTION, (int)sense_info_buffer[2]);
        MUT_ASSERT_INT_EQUAL(FBE_SCSI_ASC_TARGET_OPERATING_CONDITIONS_HAVE_CHANGED, (int)sense_info_buffer[12]);
        MUT_ASSERT_INT_EQUAL(FBE_SCSI_ASCQ_TARGET_OPERATING_CONDITIONS_HAVE_CHANGED, (int)sense_info_buffer[13]);
        fbe_semaphore_release(&config_page_test_continue_semaphore, 0, 1, FALSE);
        fbe_ldo_test_free_memory_with_sg(&sg_list);
        return(FBE_STATUS_OK);
    }
    
    if ((cdb[0] == FBE_SCSI_RECEIVE_DIAGNOSTIC) || 
        (cdb[0] == FBE_SCSI_SEND_DIAGNOSTIC))
    {   
        page_hdr = (ses_common_pg_hdr_struct *)fbe_sg_element_address(sg_list);
        MUT_ASSERT_INT_EQUAL(gen_code, BYTE_SWAP_32(page_hdr->gen_code));
    }
    else
    {
        MUT_ASSERT_TRUE(1);
    }

    fbe_semaphore_release(&config_page_test_continue_semaphore, 0, 1, FALSE);
    fbe_ldo_test_free_memory_with_sg(&sg_list);
    return(FBE_STATUS_OK);
}



void terminator_config_page_version_descriptors_test(void)
{
    fbe_status_t                    status;
    fbe_terminator_board_info_t     board_info;
    fbe_terminator_sas_port_info_t  sas_port;
    fbe_terminator_sas_encl_info_t  sas_encl;
    fbe_terminator_sas_drive_info_t sas_drive;
    fbe_terminator_api_device_handle_t        encl_id;
    fbe_terminator_api_device_handle_t        drive_id;
    fbe_sg_element_t *sg_p = NULL;
    fbe_u32_t  port_index;
    ses_ver_desc_struct ver_desc;

    mut_printf(MUT_LOG_HIGH, "%s %s ",__FILE__, __FUNCTION__);

    fbe_semaphore_init(&config_page_test_continue_semaphore, 0, FBE_SEMAPHORE_MAX);
    init_config_page_test_io_completion_context();

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
                                                           terminator_eses_tests_config_page_callback, 
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



	/* now lets verify the status */
	status = fbe_terminator_miniport_api_register_payload_completion(0,
																    terminator_eses_tests_config_page_ver_desc_io_completion,
																    &config_page_test_io_completion_context);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 


    // Test 1
    // Get the version desc
    status = fbe_terminator_api_eses_get_ver_desc(0, 4, SES_SUBENCL_TYPE_LCC, 0, SES_COMP_TYPE_EXPANDER_FW_EMA, &ver_desc);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // Set the version desc
    strncpy(ver_desc.comp_rev_level, "45.1", strlen("45.1"));
    status = fbe_terminator_api_eses_set_ver_desc(0, 4, SES_SUBENCL_TYPE_LCC, 0, SES_COMP_TYPE_EXPANDER_FW_EMA, ver_desc);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    config_page_test_io_completion_context.encl_id = 4;
    config_page_test_io_completion_context.subencl_type = SES_SUBENCL_TYPE_LCC;
    config_page_test_io_completion_context.side = 0;
    config_page_test_io_completion_context.comp_type = ver_desc.comp_type;
    memcpy(&config_page_test_io_completion_context.ver_desc, &ver_desc, sizeof(ses_ver_desc_struct));
    //issue request for config page
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_config, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);

    // Test 2
    // Get the version desc
    status = fbe_terminator_api_eses_get_ver_desc(0, 4, SES_SUBENCL_TYPE_LCC, 0, SES_COMP_TYPE_EXPANDER_BOOT_LOADER_FW, &ver_desc);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // Set the version desc
    strncpy(ver_desc.comp_rev_level, "23.2", strlen("23.2"));
    status = fbe_terminator_api_eses_set_ver_desc(0, 4, SES_SUBENCL_TYPE_LCC, 0, SES_COMP_TYPE_EXPANDER_BOOT_LOADER_FW, ver_desc);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    config_page_test_io_completion_context.encl_id = 4;
    config_page_test_io_completion_context.subencl_type = SES_SUBENCL_TYPE_LCC;
    config_page_test_io_completion_context.side = 0;
    config_page_test_io_completion_context.comp_type = ver_desc.comp_type;
    memcpy(&config_page_test_io_completion_context.ver_desc, &ver_desc, sizeof(ses_ver_desc_struct));
    //issue request for config page
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_config, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);

    // Test 3
    // Get the version desc
    status = fbe_terminator_api_eses_get_ver_desc(0, 2, SES_SUBENCL_TYPE_LCC, 0, SES_COMP_TYPE_FPGA_IMAGE, &ver_desc);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // Set the version desc
    strncpy(ver_desc.comp_rev_level, "67.5", strlen("67.5"));
    status = fbe_terminator_api_eses_set_ver_desc(0, 2, SES_SUBENCL_TYPE_LCC, 0, SES_COMP_TYPE_FPGA_IMAGE, ver_desc);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    config_page_test_io_completion_context.encl_id = 2;
    config_page_test_io_completion_context.subencl_type = SES_SUBENCL_TYPE_LCC;
    config_page_test_io_completion_context.side = 0;
    config_page_test_io_completion_context.comp_type = ver_desc.comp_type;
    memcpy(&config_page_test_io_completion_context.ver_desc, &ver_desc, sizeof(ses_ver_desc_struct));
    //issue request for config page
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_config, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);

    // Test 4
    // Get the version desc
    status = fbe_terminator_api_eses_get_ver_desc(0, 2, SES_SUBENCL_TYPE_LCC, 1, SES_COMP_TYPE_FPGA_IMAGE, &ver_desc);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // Set the version desc
    strncpy(ver_desc.comp_rev_level, "55.3", strlen("67.5"));
    status = fbe_terminator_api_eses_set_ver_desc(0, 2, SES_SUBENCL_TYPE_LCC, 1, SES_COMP_TYPE_FPGA_IMAGE, ver_desc);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    config_page_test_io_completion_context.encl_id = 2;
    config_page_test_io_completion_context.subencl_type = SES_SUBENCL_TYPE_LCC;
    config_page_test_io_completion_context.side = 1;
    config_page_test_io_completion_context.comp_type = ver_desc.comp_type;
    memcpy(&config_page_test_io_completion_context.ver_desc, &ver_desc, sizeof(ses_ver_desc_struct));
    //issue request for config page
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_config, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);


    // Test 5
    // Get the version desc
    status = fbe_terminator_api_eses_get_ver_desc(0, 4, SES_SUBENCL_TYPE_LCC, 1, SES_COMP_TYPE_LCC_MAIN, &ver_desc);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // Set the version desc
    strncpy(ver_desc.comp_rev_level, "32.9", strlen("67.5"));
    status = fbe_terminator_api_eses_set_ver_desc(0, 4, SES_SUBENCL_TYPE_LCC, 1, SES_COMP_TYPE_LCC_MAIN, ver_desc);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    config_page_test_io_completion_context.encl_id = 4;
    config_page_test_io_completion_context.subencl_type = SES_SUBENCL_TYPE_LCC;
    config_page_test_io_completion_context.side = 1;
    config_page_test_io_completion_context.comp_type = ver_desc.comp_type;
    memcpy(&config_page_test_io_completion_context.ver_desc, &ver_desc, sizeof(ses_ver_desc_struct));
    //issue request for config page
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_config, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);

    // Test 6
    // Get the version desc
    status = fbe_terminator_api_eses_get_ver_desc(0, 6, SES_SUBENCL_TYPE_LCC, 1, SES_COMP_TYPE_EXPANDER_BOOT_LOADER_FW, &ver_desc);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // Set the version desc
    strncpy(ver_desc.comp_rev_level, "86.4", strlen("67.5"));
    status = fbe_terminator_api_eses_set_ver_desc(0, 6, SES_SUBENCL_TYPE_LCC, 1, SES_COMP_TYPE_EXPANDER_BOOT_LOADER_FW, ver_desc);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    config_page_test_io_completion_context.encl_id = 6;
    config_page_test_io_completion_context.subencl_type = SES_SUBENCL_TYPE_LCC;
    config_page_test_io_completion_context.side = 1;
    config_page_test_io_completion_context.comp_type = ver_desc.comp_type;
    memcpy(&config_page_test_io_completion_context.ver_desc, &ver_desc, sizeof(ses_ver_desc_struct));
    //issue request for config page
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    status = terminator_miniport_api_issue_eses_cmd(0, 7, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_config, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&config_page_test_continue_semaphore, NULL);

   //Test 7, illegal combinations not supported
    status = fbe_terminator_api_eses_get_ver_desc(0, 4, SES_SUBENCL_TYPE_LCC, 0, SES_COMP_TYPE_OTHER_HW_VER, &ver_desc);
    MUT_ASSERT_TRUE(status != FBE_STATUS_OK);
    status = fbe_terminator_api_eses_set_ver_desc(0, 4, SES_SUBENCL_TYPE_LCC, 0, SES_COMP_TYPE_PS_FW, ver_desc);
    MUT_ASSERT_TRUE(status != FBE_STATUS_OK);
    status = fbe_terminator_api_eses_get_ver_desc(0, 4, SES_SUBENCL_TYPE_CHASSIS, 0x7f, SES_COMP_TYPE_LCC_MAIN, &ver_desc);
    MUT_ASSERT_TRUE(status != FBE_STATUS_OK);
    status = fbe_terminator_api_eses_set_ver_desc(0, 4, SES_SUBENCL_TYPE_PS, 0, SES_COMP_TYPE_PS_FW, ver_desc);
    MUT_ASSERT_TRUE(status != FBE_STATUS_OK);
    status = fbe_terminator_api_eses_get_ver_desc(0, 4, SES_SUBENCL_TYPE_PS, 0, SES_COMP_TYPE_EXPANDER_FW_EMA, &ver_desc);
    MUT_ASSERT_TRUE(status != FBE_STATUS_OK);
    status = fbe_terminator_api_eses_get_ver_desc(0, 4, SES_SUBENCL_TYPE_PS, 1, SES_COMP_TYPE_EXPANDER_FW_EMA, &ver_desc);
    MUT_ASSERT_TRUE(status != FBE_STATUS_OK);

    status = fbe_terminator_api_destroy();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator destroyed.", __FUNCTION__);

}



static fbe_status_t terminator_eses_tests_config_page_ver_desc_io_completion (fbe_payload_ex_t * payload, fbe_payload_ex_completion_context_t context)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_cdb_operation_t *payload_cdb_operation = NULL;
    fbe_u8_t *cdb = NULL;
    fbe_sg_element_t *sg_list = NULL;
    fbe_u8_t *sense_info_buffer = NULL;
    ses_common_pg_hdr_struct *page_hdr = NULL;
    fbe_u8_t *return_page_code = NULL;
    fbe_u8_t *config_page;
    fbe_u8_t encl_id = ((config_page_test_io_completion_context_t *)context)->encl_id;
    ses_subencl_type_enum subencl_type = ((config_page_test_io_completion_context_t *)context)->subencl_type;
    fbe_u8_t side = ((config_page_test_io_completion_context_t *)context)->side;
    fbe_u8_t comp_type = ((config_page_test_io_completion_context_t *)context)->comp_type;
    ses_ver_desc_struct expected_ver_desc, actual_ver_desc;

    memcpy(&expected_ver_desc,
           &((config_page_test_io_completion_context_t *)context)->ver_desc,
           sizeof(expected_ver_desc));

    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    MUT_ASSERT_TRUE(payload_cdb_operation != NULL);   
    fbe_payload_cdb_operation_get_cdb(payload_cdb_operation, &cdb); 
    MUT_ASSERT_TRUE(payload_cdb_operation != NULL);
    fbe_payload_cdb_operation_get_sense_buffer(payload_cdb_operation, &sense_info_buffer);
    MUT_ASSERT_TRUE(sense_info_buffer != NULL);
    status = fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    if(cdb[0] == FBE_SCSI_RECEIVE_DIAGNOSTIC)
    {   
        return_page_code = (fbe_u8_t *)fbe_sg_element_address(sg_list);
        if(*return_page_code == SES_PG_CODE_CONFIG)
        {
            config_page = (fbe_u8_t *)fbe_sg_element_address(sg_list);
            status = terminator_eses_tests_config_page_get_ver_desc(config_page,
                                                                    subencl_type,
                                                                    side,
                                                                    comp_type,
                                                                    &actual_ver_desc);

            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            
            // This assert is not requried, just to "test" the "test code".
            MUT_ASSERT_INT_EQUAL(expected_ver_desc.comp_type, comp_type);

            MUT_ASSERT_TRUE(strncmp(expected_ver_desc.comp_rev_level, actual_ver_desc.comp_rev_level, 5) == 0);
        }
        else
        {
            MUT_ASSERT_TRUE(1);
        }
    }
    else
    {
        MUT_ASSERT_TRUE(1);
    }

    fbe_semaphore_release(&config_page_test_continue_semaphore, 0, 1, FALSE);
    fbe_ldo_test_free_memory_with_sg(&sg_list);
    return(FBE_STATUS_OK);
}


fbe_status_t terminator_eses_tests_config_page_get_ver_desc(fbe_u8_t *config_page,
                                                            ses_subencl_type_enum subencl_type,
                                                            fbe_u8_t side,
                                                            fbe_u8_t  comp_type,
                                                            ses_ver_desc_struct *ver_desc)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    ses_subencl_desc_struct * subencl_desc_ptr = NULL;
    ses_pg_config_struct * config_pg_hdr = NULL;
    fbe_u8_t subencl = 0;
    fbe_u8_t ver_desc_index = 0;
    fbe_u8_t total_num_subencls = 0;

    config_pg_hdr = (ses_pg_config_struct *)config_page; 
    subencl_desc_ptr = (ses_subencl_desc_struct *)((fbe_u8_t *)config_page 
                                                    + FBE_ESES_PAGE_HEADER_SIZE);
    total_num_subencls =  config_pg_hdr->num_secondary_subencls + 1;

    for(subencl = 0; subencl < total_num_subencls; subencl ++ )   
    {
        if((subencl_type == subencl_desc_ptr->subencl_type) &&
           (side == subencl_desc_ptr->side) )
        {
            for(ver_desc_index = 0; ver_desc_index < subencl_desc_ptr->num_ver_descs; ver_desc_index++)
            {
                if(comp_type == subencl_desc_ptr->ver_desc[ver_desc_index].comp_type)
                {
                    memcpy(ver_desc, &subencl_desc_ptr->ver_desc[ver_desc_index], sizeof(ses_ver_desc_struct));
                    status = FBE_STATUS_OK;
                    goto DOUBLE_LOOP_BREAK;
                }
            }
        }
        subencl_desc_ptr = fbe_eses_get_next_ses_subencl_desc_p(subencl_desc_ptr);
    }
DOUBLE_LOOP_BREAK:

    return(status);
}





