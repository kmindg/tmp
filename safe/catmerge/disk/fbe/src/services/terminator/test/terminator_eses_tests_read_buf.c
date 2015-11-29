/***************************************************************************
 *  terminator_eses_tests_led_patterns.c
 ***************************************************************************
 *
 *  Description
 *      ESES enclosure tests related to the "read buffer" cdb command.
 *      
 *
 *  History:
 *     Jan-12-09   Rajesh V. Created
 *    
 ***************************************************************************/

/***************************************************************************
 *  terminator_eses_tests_read_buf.c
 ***************************************************************************
 *
 *  Description
 *      ESES enclosure tests read to read buffer & write buffer cdbs.
 *
 *  NOTE: 
 *      Change the name of the file to terminator_eses_tests_buf_cmds.c as it
 *      has both read & write buffer cdb related tests (in future).
 *
 *  History:
 *     Jan-12-08   Rajesh V. Created
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

// The below definitions for buffer ids will
// change if a different configuration page
// is used.
#define BUFFER_ID_OF_LOCAL_ACTIVE_TRACE 0
#define BUFFER_ID_OF_LOCAL_SAVED_TRACE  2
#define BUFFER_ID_OF_PEER_ACTIVE_TRACE  4
#define BUFFER_ID_OF_PEER_SAVED_TRACE   6   

/**********************************/
/*        local variables        */
/**********************************/
typedef struct read_buf_test_io_completion_context_s
{
    fbe_u8_t encl_id;
    fbe_u32_t expected_buf_len;
    fbe_u8_t *expected_buf;
    fbe_bool_t scsi_error_return_set;
}read_buf_test_io_completion_context_t;

static read_buf_test_io_completion_context_t read_buf_test_io_completion_context;
static fbe_semaphore_t   buf_cmds_test_continue_semaphore;
static fbe_u32_t read_buf_test_io_completion_call_count;

/**********************************/
/*       LOCAL FUNCTIONS          */
/**********************************/
static fbe_status_t buf_cmds_test_io_completion (fbe_payload_ex_t * payload, fbe_payload_ex_completion_context_t context);
static fbe_status_t read_buf_test_callback(fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context);
static void read_buf_test_init_io_completion_context(void);
static void buf_cmds_test_build_emc_ctrl_page(fbe_sg_element_t *sg_p);
static void buf_cmds_test_build_str_out_page(fbe_sg_element_t *sg_p, fbe_u8_t *str);


void read_buf_test_init_io_completion_context(void)
{
    memset(&read_buf_test_io_completion_context, 0, sizeof(read_buf_test_io_completion_context_t));
}

static fbe_status_t read_buf_test_callback(fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context)
{
    return FBE_STATUS_OK;
}


void terminator_read_buf_test(void)
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
    fbe_u8_t *buf;
    read_buf_test_cdb_param_t read_buf_test_param;
    int i;

    mut_printf(MUT_LOG_HIGH, "%s %s \n",__FILE__, __FUNCTION__);

    fbe_semaphore_init(&buf_cmds_test_continue_semaphore, 0, FBE_SEMAPHORE_MAX);
    read_buf_test_init_io_completion_context();

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
                                                           read_buf_test_callback, 
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
																    buf_cmds_test_io_completion,
																    &read_buf_test_io_completion_context);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 

    // allocate buffer and set the resume prom buffer using TAPI
    buf = (fbe_u8_t *)malloc(10*sizeof(fbe_u8_t));
    for(i=0; i < 10; i++)
    {
        buf[i] = i;
    }
    read_buf_test_io_completion_context.expected_buf = (fbe_u8_t *)malloc(10*sizeof(fbe_u8_t));

    // test 1
    status = fbe_terminator_api_set_resume_prom_info(0, 4, LCC_A_RESUME_PROM, buf, 10);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // Set the expected values that the IO Completion function checks for.
    read_buf_test_io_completion_context.encl_id = 4;
    read_buf_test_io_completion_context.expected_buf_len = 3;
    for(i=0; i < 3; i++)
    {
        read_buf_test_io_completion_context.expected_buf[i] = i+4;
    }
    // Send the read buffer eses command.
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    read_buf_test_param.buf_offset = 4;
    read_buf_test_param.alloc_len = 3;
    read_buf_test_param.buf_id = 3;
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_READ_BUFFER, &read_buf_test_param, sg_p);
    fbe_semaphore_wait(&buf_cmds_test_continue_semaphore, NULL);

    //test 2
    status = fbe_terminator_api_set_resume_prom_info(0, 4, LCC_B_RESUME_PROM, buf, 10);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // Set the expected values that the IO Completion function checks for.
    read_buf_test_io_completion_context.encl_id = 4;
    read_buf_test_io_completion_context.expected_buf_len = 2;
    for(i=0; i < 2; i++)
    {
        read_buf_test_io_completion_context.expected_buf[i] = i+8;
    }
    // Send the read buffer eses command.
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    read_buf_test_param.buf_offset = 8;
    read_buf_test_param.alloc_len = 9;
    read_buf_test_param.buf_id = 7;
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_READ_BUFFER, &read_buf_test_param, sg_p);
    fbe_semaphore_wait(&buf_cmds_test_continue_semaphore, NULL);

    //test 3
    status = fbe_terminator_api_set_resume_prom_info(0, 4, CHASSIS_RESUME_PROM, buf, 10);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // Set the expected values that the IO Completion function checks for.
    read_buf_test_io_completion_context.encl_id = 4;
    read_buf_test_io_completion_context.expected_buf_len = 1;
    for(i=0; i < 1; i++)
    {
        read_buf_test_io_completion_context.expected_buf[i] = i+8;
    }
    // Send the read buffer eses command.
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    read_buf_test_param.buf_offset = 8;
    read_buf_test_param.alloc_len = 1;
    read_buf_test_param.buf_id = 8;
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_READ_BUFFER, &read_buf_test_param, sg_p);
    fbe_semaphore_wait(&buf_cmds_test_continue_semaphore, NULL);

    //test 4
    status = fbe_terminator_api_set_resume_prom_info(0, 4, CHASSIS_RESUME_PROM, buf, 9);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // Set the expected values that the IO Completion function checks for.
    read_buf_test_io_completion_context.encl_id = 4;
    read_buf_test_io_completion_context.expected_buf_len = 1;
    for(i=0; i < 1; i++)
    {
        read_buf_test_io_completion_context.expected_buf[i] = i+8;
    }
    // Send the read buffer eses command.
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    read_buf_test_param.buf_offset = 8;
    read_buf_test_param.alloc_len = 1;
    read_buf_test_param.buf_id = 8;
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_READ_BUFFER, &read_buf_test_param, sg_p);
    fbe_semaphore_wait(&buf_cmds_test_continue_semaphore, NULL);


    //test 5
    status = fbe_terminator_api_set_resume_prom_info(0, 4, PS_A_RESUME_PROM, buf, 10);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // Set the expected values that the IO Completion function checks for.
    read_buf_test_io_completion_context.encl_id = 4;
    read_buf_test_io_completion_context.expected_buf_len = 6;
    for(i=0; i < 6; i++)
    {
        read_buf_test_io_completion_context.expected_buf[i] = i+4;
    }
    // Send the read buffer eses command.
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    read_buf_test_param.buf_offset = 4;
    read_buf_test_param.alloc_len = 10;
    read_buf_test_param.buf_id = 9;
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_READ_BUFFER, &read_buf_test_param, sg_p);
    fbe_semaphore_wait(&buf_cmds_test_continue_semaphore, NULL);

    //test 6
    status = fbe_terminator_api_set_resume_prom_info(0, 4, PS_B_RESUME_PROM, buf, 10);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // Set the expected values that the IO Completion function checks for.
    read_buf_test_io_completion_context.encl_id = 4;
    read_buf_test_io_completion_context.expected_buf_len = 3;
    read_buf_test_io_completion_context.scsi_error_return_set = FBE_TRUE;
    for(i=0; i < 3; i++)
    {
        read_buf_test_io_completion_context.expected_buf[i] = i+4;
    }
    // Send the read buffer eses command.
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    read_buf_test_param.buf_offset = 12;
    read_buf_test_param.alloc_len = 3;
    read_buf_test_param.buf_id = 10;
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_READ_BUFFER, &read_buf_test_param, sg_p);
    fbe_semaphore_wait(&buf_cmds_test_continue_semaphore, NULL);


    //test 7
    // Allocate 2K of memory for a resume prom.
    free(buf);
    buf = (fbe_u8_t *)malloc(2*1024*sizeof(fbe_u8_t));
    for(i=0; i < 10 ; i++)
    {
        buf[2032+i] = i;
    }
    status = fbe_terminator_api_set_resume_prom_info(0, 4, LCC_A_RESUME_PROM, buf, 2048);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    //set the expected values that the io completion function checks for.
    free(read_buf_test_io_completion_context.expected_buf);
    read_buf_test_io_completion_context.expected_buf = (fbe_u8_t *)malloc(10*sizeof(fbe_u8_t));
    for(i=0; i < 10; i++)
    {
        read_buf_test_io_completion_context.expected_buf[i] = i;
    }
    read_buf_test_io_completion_context.encl_id = 4;
    read_buf_test_io_completion_context.expected_buf_len = 10;
    read_buf_test_io_completion_context.scsi_error_return_set = FBE_FALSE;
    // send the read buffer eses command.
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    read_buf_test_param.buf_offset = 2032;
    read_buf_test_param.alloc_len = 10;
    read_buf_test_param.buf_id = 3;
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_READ_BUFFER, &read_buf_test_param, sg_p);
    fbe_semaphore_wait(&buf_cmds_test_continue_semaphore, NULL);

    //test 8
    // Allocate even greater memory to resume prom
    free(buf);
    buf = (fbe_u8_t *)malloc(0xABCDEF*sizeof(fbe_u8_t));
    for(i=0; i < 10 ; i++)
    {
        buf[0xABCDEC-20+i] = i;
    }
    status = fbe_terminator_api_set_resume_prom_info(0, 4, LCC_A_RESUME_PROM, buf, 0xABCDEF);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    //set the expected values that the io completion function checks for.
    free(read_buf_test_io_completion_context.expected_buf);
    read_buf_test_io_completion_context.expected_buf = (fbe_u8_t *)malloc(10*sizeof(fbe_u8_t));
    for(i=0; i < 10; i++)
    {
        read_buf_test_io_completion_context.expected_buf[i] = i;
    }
    read_buf_test_io_completion_context.encl_id = 4;
    read_buf_test_io_completion_context.expected_buf_len = 10;
    read_buf_test_io_completion_context.scsi_error_return_set = FBE_FALSE;
    // send the read buffer eses command.
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    read_buf_test_param.buf_offset = 0xABCDEC-20;
    read_buf_test_param.alloc_len = 10;
    read_buf_test_param.buf_id = 3;
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_READ_BUFFER, &read_buf_test_param, sg_p);
    fbe_semaphore_wait(&buf_cmds_test_continue_semaphore, NULL);

    status = fbe_terminator_api_destroy();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator destroyed.", __FUNCTION__);
}


void terminator_write_buf_test(void)
{
    fbe_status_t                    status;
    fbe_terminator_board_info_t     board_info;
    fbe_terminator_sas_port_info_t  sas_port;
    fbe_terminator_sas_encl_info_t  sas_encl;
    fbe_terminator_sas_drive_info_t sas_drive;
    fbe_terminator_api_device_handle_t  encl_id;
    fbe_terminator_api_device_handle_t  drive_id;
    fbe_sg_element_t *sg_p = NULL;
    fbe_u32_t  port_index;
    fbe_u8_t *buf, *write_buf;
    read_buf_test_cdb_param_t read_buf_test_param;
    write_buf_test_cdb_param_t write_buf_test_param;
    int i;

    mut_printf(MUT_LOG_HIGH, "%s %s \n",__FILE__, __FUNCTION__);

    fbe_semaphore_init(&buf_cmds_test_continue_semaphore, 0, FBE_SEMAPHORE_MAX);
    read_buf_test_init_io_completion_context();

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
                                                           read_buf_test_callback, 
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
																    buf_cmds_test_io_completion,
																    &read_buf_test_io_completion_context);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 

    // allocate buffer and set the active trace buffer using TAPI
    buf = (fbe_u8_t *)malloc(11*sizeof(fbe_u8_t));
    for(i=0; i < 10; i++)
    {
        buf[i] = i;
    }
    read_buf_test_io_completion_context.expected_buf = (fbe_u8_t *)malloc(11*sizeof(fbe_u8_t));

    // TEST 1

    // The buffer will now have 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
    status = fbe_terminator_api_set_buf_by_buf_id(0, 4, BUFFER_ID_OF_LOCAL_ACTIVE_TRACE, buf, 11);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // send the write buffer eses command to overwrite data in last 3 elements of 
    // the buffer
    write_buf_test_param.buf_id = BUFFER_ID_OF_LOCAL_ACTIVE_TRACE;
    write_buf_test_param.buf_offset = 8;
    write_buf_test_param.param_list_len = 3;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    write_buf = (fbe_u8_t *)fbe_sg_element_address(sg_p);
    for(i=0; i < 3; i++)
    {
        write_buf[i] = i + 11; // 11, 12, 13
    }
    // The buffer will now have 0, 1, 2, 3, 4, 5, 6, 7, 11, 12, 13.
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_WRITE_BUFFER, &write_buf_test_param, sg_p);
    fbe_semaphore_wait(&buf_cmds_test_continue_semaphore, NULL);

    // Set the expected values that the IO Completion function checks for
    // when a read buffer cmd is issued to the EMA buffer.
    // we want to read from offset 4 , the next 7 values.
    // So we expect 4, 5, 6, 7, 11, 12, 13
    read_buf_test_io_completion_context.encl_id = 4;
    read_buf_test_io_completion_context.expected_buf_len = 7;
    for(i=0; i < 4; i++)
    {
        read_buf_test_io_completion_context.expected_buf[i] = i + 4 ; // 4, 5, 6, 7
    }
    for(i=4; i < 7; i++)
    {
        read_buf_test_io_completion_context.expected_buf[i] = i + 7 ; // 11, 12, 13
    }
    read_buf_test_io_completion_context.scsi_error_return_set = FBE_FALSE;
    // Send the read buffer eses command.
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    // we are expecting the last 4 values in the buffer (7, 11, 12, 13)
    read_buf_test_param.buf_offset = 4;
    read_buf_test_param.alloc_len = 7;
    read_buf_test_param.buf_id = BUFFER_ID_OF_LOCAL_ACTIVE_TRACE;
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_READ_BUFFER, &read_buf_test_param, sg_p);
    fbe_semaphore_wait(&buf_cmds_test_continue_semaphore, NULL);



    // TEST 2
    // Do the same test on a different buffer (BUFFER_ID_OF_LOCAL_SAVED_TRACE)

    // The buffer will now have 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
    status = fbe_terminator_api_set_buf_by_buf_id(0, 4, BUFFER_ID_OF_LOCAL_SAVED_TRACE, buf, 11);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // send the write buffer eses command to overwrite data in last 3 elements of 
    // the buffer
    write_buf_test_param.buf_id = BUFFER_ID_OF_LOCAL_SAVED_TRACE;
    write_buf_test_param.buf_offset = 8;
    write_buf_test_param.param_list_len = 3;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    write_buf = (fbe_u8_t *)fbe_sg_element_address(sg_p);
    for(i=0; i < 3; i++)
    {
        write_buf[i] = i + 11; // 11, 12, 13
    }
    // The buffer will now have 0, 1, 2, 3, 4, 5, 6, 7, 11, 12, 13.
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_WRITE_BUFFER, &write_buf_test_param, sg_p);
    fbe_semaphore_wait(&buf_cmds_test_continue_semaphore, NULL);

    // Set the expected values that the IO Completion function checks for
    // when a read buffer cmd is issued to the EMA buffer.
    // we want to read from offset 4 , the next 7 values.
    // So we expect 4, 5, 6, 7, 11, 12, 13
    read_buf_test_io_completion_context.encl_id = 4;
    read_buf_test_io_completion_context.expected_buf_len = 7;
    for(i=0; i < 4; i++)
    {
        read_buf_test_io_completion_context.expected_buf[i] = i + 4 ; // 4, 5, 6, 7
    }
    for(i=4; i < 7; i++)
    {
        read_buf_test_io_completion_context.expected_buf[i] = i + 7 ; // 11, 12, 13
    }
    read_buf_test_io_completion_context.scsi_error_return_set = FBE_FALSE;
    // Send the read buffer eses command.
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    // we are expecting the last 4 values in the buffer (7, 11, 12, 13)
    read_buf_test_param.buf_offset = 4;
    read_buf_test_param.alloc_len = 7;
    read_buf_test_param.buf_id = BUFFER_ID_OF_LOCAL_SAVED_TRACE;
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_READ_BUFFER, &read_buf_test_param, sg_p);
    fbe_semaphore_wait(&buf_cmds_test_continue_semaphore, NULL);

    // TEST 3
    // Do the same test on a different buffer (BUFFER_ID_OF_PEER_ACTIVE_TRACE)

    // The buffer will now have 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
    status = fbe_terminator_api_set_buf_by_buf_id(0, 4, BUFFER_ID_OF_PEER_ACTIVE_TRACE, buf, 11);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // send the write buffer eses command to overwrite data in last 3 elements of 
    // the buffer
    write_buf_test_param.buf_id = BUFFER_ID_OF_PEER_ACTIVE_TRACE;
    write_buf_test_param.buf_offset = 8;
    write_buf_test_param.param_list_len = 3;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    write_buf = (fbe_u8_t *)fbe_sg_element_address(sg_p);
    for(i=0; i < 3; i++)
    {
        write_buf[i] = i + 11; // 11, 12, 13
    }
    // The buffer will now have 0, 1, 2, 3, 4, 5, 6, 7, 11, 12, 13.
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_WRITE_BUFFER, &write_buf_test_param, sg_p);
    fbe_semaphore_wait(&buf_cmds_test_continue_semaphore, NULL);

    // Set the expected values that the IO Completion function checks for
    // when a read buffer cmd is issued to the EMA buffer.
    // we want to read from offset 4 , the next 7 values.
    // So we expect 4, 5, 6, 7, 11, 12, 13
    read_buf_test_io_completion_context.encl_id = 4;
    read_buf_test_io_completion_context.expected_buf_len = 7;
    for(i=0; i < 4; i++)
    {
        read_buf_test_io_completion_context.expected_buf[i] = i + 4 ; // 4, 5, 6, 7
    }
    for(i=4; i < 7; i++)
    {
        read_buf_test_io_completion_context.expected_buf[i] = i + 7 ; // 11, 12, 13
    }
    read_buf_test_io_completion_context.scsi_error_return_set = FBE_FALSE;
    // Send the read buffer eses command.
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    // we are expecting the last 4 values in the buffer (7, 11, 12, 13)
    read_buf_test_param.buf_offset = 4;
    read_buf_test_param.alloc_len = 7;
    read_buf_test_param.buf_id = BUFFER_ID_OF_PEER_ACTIVE_TRACE;
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_READ_BUFFER, &read_buf_test_param, sg_p);
    fbe_semaphore_wait(&buf_cmds_test_continue_semaphore, NULL);


    // TEST 4
    // Do the same test on a different buffer (BUFFER_ID_OF_PEER_SAVED_TRACE)

    // The buffer will now have 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
    status = fbe_terminator_api_set_buf_by_buf_id(0, 4, BUFFER_ID_OF_PEER_SAVED_TRACE, buf, 11);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // send the write buffer eses command to overwrite data in last 3 elements of 
    // the buffer
    write_buf_test_param.buf_id = BUFFER_ID_OF_PEER_SAVED_TRACE;
    write_buf_test_param.buf_offset = 8;
    write_buf_test_param.param_list_len = 3;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    write_buf = (fbe_u8_t *)fbe_sg_element_address(sg_p);
    for(i=0; i < 3; i++)
    {
        write_buf[i] = i + 11; // 11, 12, 13
    }
    // The buffer will now have 0, 1, 2, 3, 4, 5, 6, 7, 11, 12, 13.
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_WRITE_BUFFER, &write_buf_test_param, sg_p);
    fbe_semaphore_wait(&buf_cmds_test_continue_semaphore, NULL);

    // Set the expected values that the IO Completion function checks for
    // when a read buffer cmd is issued to the EMA buffer.
    // we want to read from offset 4 , the next 7 values.
    // So we expect 4, 5, 6, 7, 11, 12, 13
    read_buf_test_io_completion_context.encl_id = 4;
    read_buf_test_io_completion_context.expected_buf_len = 7;
    for(i=0; i < 4; i++)
    {
        read_buf_test_io_completion_context.expected_buf[i] = i + 4 ; // 4, 5, 6, 7
    }
    for(i=4; i < 7; i++)
    {
        read_buf_test_io_completion_context.expected_buf[i] = i + 7 ; // 11, 12, 13
    }
    read_buf_test_io_completion_context.scsi_error_return_set = FBE_FALSE;
    // Send the read buffer eses command.
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    // we are expecting the last 4 values in the buffer (7, 11, 12, 13)
    read_buf_test_param.buf_offset = 4;
    read_buf_test_param.alloc_len = 7;
    read_buf_test_param.buf_id = BUFFER_ID_OF_PEER_SAVED_TRACE;
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_READ_BUFFER, &read_buf_test_param, sg_p);
    fbe_semaphore_wait(&buf_cmds_test_continue_semaphore, NULL);


    // TEST 5

    // Send a write buffer command that requests data over the EMA buffer size
    // This is not allowed for a write command.
    write_buf_test_param.buf_id = BUFFER_ID_OF_LOCAL_ACTIVE_TRACE;
    write_buf_test_param.buf_offset = 8;
    // 8 + 5 = 13 which is greater than size of the EMA buffer(11).
    // So this write will fail.
    write_buf_test_param.param_list_len = 5;
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    write_buf = (fbe_u8_t *)fbe_sg_element_address(sg_p);
    for(i=0; i < 5; i++)
    {
        write_buf[i] = i + 20; // 20-24
    }
    // This write command should fail.
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_WRITE_BUFFER, &write_buf_test_param, sg_p);
    fbe_semaphore_wait(&buf_cmds_test_continue_semaphore, NULL);

    // The buffer will STILL have 0, 1, 2, 3, 4, 5, 6, 7, 11, 12, 13 as above
    // write buffer command should fail.
    // Set the expected values that the IO Completion function checks for
    // when a read buffer cmd is issued to the EMA buffer.
    // we want to read from offset 4 , the next 7 values.
    // So we expect 4, 5, 6, 7, 11, 12, 13
    read_buf_test_io_completion_context.encl_id = 4;
    read_buf_test_io_completion_context.expected_buf_len = 7;
    for(i=0; i < 4; i++)
    {
        read_buf_test_io_completion_context.expected_buf[i] = i + 4 ; // 4, 5, 6, 7
    }
    for(i=4; i < 7; i++)
    {
        read_buf_test_io_completion_context.expected_buf[i] = i + 7 ; // 11, 12, 13
    }
    read_buf_test_io_completion_context.scsi_error_return_set = FBE_FALSE;
    // Send the read buffer eses command.
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    // we are expecting the last 4 values in the buffer (7, 11, 12, 13)
    read_buf_test_param.buf_offset = 4;
    read_buf_test_param.alloc_len = 7;
    read_buf_test_param.buf_id = BUFFER_ID_OF_LOCAL_ACTIVE_TRACE;
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_READ_BUFFER, &read_buf_test_param, sg_p);
    fbe_semaphore_wait(&buf_cmds_test_continue_semaphore, NULL);

    // TEST 6
    // Send a EMC control page and clear the local buffers using
    // trace buffer information elements.
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    buf_cmds_test_build_emc_ctrl_page(sg_p);
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_SEND_DIAGNOSTIC, &ses_pg_code_emc_encl_ctrl, sg_p);
    fbe_semaphore_wait(&buf_cmds_test_continue_semaphore, NULL);
    // Set the expected values the completion function checks for.
    read_buf_test_io_completion_context.encl_id = 4;
    read_buf_test_io_completion_context.expected_buf_len = 11;
    for(i=0; i < 11; i++)
    {
        read_buf_test_io_completion_context.expected_buf[i] = 0;
    }
    read_buf_test_io_completion_context.scsi_error_return_set = FBE_FALSE;

    // The EMC CTRL page should have cleared both active trace and saved
    // trace buffers of the local LCC.
    // Now issue read buffer cmds and test the buffers are properly cleared.

    // Send the read buffer eses command to BUFFER_ID_OF_LOCAL_ACTIVE_TRACE
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    read_buf_test_param.buf_offset = 0;
    read_buf_test_param.alloc_len = 11;
    read_buf_test_param.buf_id = BUFFER_ID_OF_LOCAL_ACTIVE_TRACE;
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_READ_BUFFER, &read_buf_test_param, sg_p);
    fbe_semaphore_wait(&buf_cmds_test_continue_semaphore, NULL);  
    // Send the read buffer eses command to BUFFER_ID_OF_LOCAL_SAVED_TRACE
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    read_buf_test_param.buf_offset = 0;
    read_buf_test_param.alloc_len = 11;
    read_buf_test_param.buf_id = BUFFER_ID_OF_LOCAL_SAVED_TRACE;
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_READ_BUFFER, &read_buf_test_param, sg_p);
    fbe_semaphore_wait(&buf_cmds_test_continue_semaphore, NULL); 

    // TEST 7. ISSUE A STRING OUT COMMAND to write a STRING to ACTIVE TRACE
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    buf_cmds_test_build_str_out_page(sg_p, "emc2");
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_SEND_DIAGNOSTIC, &ses_pg_code_str_out, sg_p);
    fbe_semaphore_wait(&buf_cmds_test_continue_semaphore, NULL);
    // Set the expected values the completion function checks for.
    read_buf_test_io_completion_context.encl_id = 4;
    read_buf_test_io_completion_context.expected_buf_len = 5;
    strncpy(read_buf_test_io_completion_context.expected_buf, "emc2", strlen("emc2"));
    read_buf_test_io_completion_context.scsi_error_return_set = FBE_FALSE;
    // Send the read buffer eses command to BUFFER_ID_OF_LOCAL_ACTIVE_TRACE
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    read_buf_test_param.buf_offset = 0;
    read_buf_test_param.alloc_len = 5;
    read_buf_test_param.buf_id = BUFFER_ID_OF_LOCAL_ACTIVE_TRACE;
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_READ_BUFFER, &read_buf_test_param, sg_p);
    fbe_semaphore_wait(&buf_cmds_test_continue_semaphore, NULL);  
        
    status = fbe_terminator_api_destroy();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator destroyed.", __FUNCTION__);
}




static fbe_status_t buf_cmds_test_io_completion (fbe_payload_ex_t * payload, fbe_payload_ex_completion_context_t context)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_cdb_operation_t *payload_cdb_operation;
    fbe_u8_t *cdb = NULL;
    fbe_sg_element_t *sg_list = NULL;
    // As all these tests are only on port 0.
    fbe_u8_t encl_id = ((read_buf_test_io_completion_context_t *)context)->encl_id;
    fbe_u8_t *expected_buf = ((read_buf_test_io_completion_context_t *)context)->expected_buf;
    fbe_u32_t expected_buf_len = ((read_buf_test_io_completion_context_t *)context)->expected_buf_len;
    fbe_bool_t scsi_error_return_set = ((read_buf_test_io_completion_context_t *)context)->scsi_error_return_set;
    fbe_u8_t *returned_buf;
    fbe_u8_t *sense_info_buffer = NULL;
    static buf_cms_test_io_completion_count = 0;

    read_buf_test_io_completion_call_count++;
    //mut_printf(MUT_LOG_HIGH, "%s: called %d th time\n ",__FUNCTION__, read_buf_test_io_completion_call_count);

    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    MUT_ASSERT_TRUE(payload_cdb_operation != NULL);   
    fbe_payload_cdb_operation_get_cdb(payload_cdb_operation, &cdb); 
    status = fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    buf_cms_test_io_completion_count++;
    mut_printf(MUT_LOG_TEST_STATUS, "%s: called %d th time.", __FUNCTION__, buf_cms_test_io_completion_count);

    if (cdb[0] == FBE_SCSI_READ_BUFFER)
    {   
        if(!scsi_error_return_set)
        {
            returned_buf = (fbe_u8_t *)fbe_sg_element_address(sg_list);
            MUT_ASSERT_TRUE(memcmp(expected_buf, returned_buf, expected_buf_len) == 0);
        }
        else
        {
            // can check expected sensa data for more rigorous testing.
        }
    }
    else if(cdb[0] == FBE_SCSI_WRITE_BUFFER)
    {
        if(!scsi_error_return_set)
        {
            // Can check expected sense data for more rigorous testing.
            // Can also check the SCSI CDB return status ( should be a CHECK condition).
        }
    }

    fbe_semaphore_release(&buf_cmds_test_continue_semaphore, 0, 1, FALSE);
    fbe_ldo_test_free_memory_with_sg(&sg_list);
    return(FBE_STATUS_OK);
}

static void buf_cmds_test_build_emc_ctrl_page(fbe_sg_element_t *sg_p)
{
    /* Comment tempararily as EMC control page format has changed 

    ses_pg_emc_encl_ctrl_struct *emc_encl_ctrl_page_hdr = NULL;
    ses_trace_buf_info_elem_struct *trace_buffer_inf_elem_ptr = NULL;
    fbe_u8_t *num_trace_buf_inf_elems;

    emc_encl_ctrl_page_hdr = (ses_pg_emc_encl_ctrl_struct *)fbe_sg_element_address(sg_p);
    memset(emc_encl_ctrl_page_hdr, 0, sizeof(emc_encl_ctrl_page_hdr));
    emc_encl_ctrl_page_hdr->hdr.pg_code = SES_PG_CODE_EMC_ENCL_CTRL;
    emc_encl_ctrl_page_hdr->num_sas_conn_info_elems = 5;
    num_trace_buf_inf_elems = (fbe_u8_t *)(((fbe_u8_t *)(emc_encl_ctrl_page_hdr + 1)) + 
                                           (emc_encl_ctrl_page_hdr->num_sas_conn_info_elems) * (sizeof(ses_sas_conn_info_elem_struct)));
    *num_trace_buf_inf_elems = 2;
    trace_buffer_inf_elem_ptr = (ses_trace_buf_info_elem_struct *)(num_trace_buf_inf_elems + 1);


    trace_buffer_inf_elem_ptr->buf_id = BUFFER_ID_OF_LOCAL_ACTIVE_TRACE;
    trace_buffer_inf_elem_ptr->buf_action = FBE_ESES_TRACE_BUF_ACTION_CTRL_CLEAR_BUF;

    trace_buffer_inf_elem_ptr++;

    trace_buffer_inf_elem_ptr->buf_id = BUFFER_ID_OF_LOCAL_SAVED_TRACE;
    trace_buffer_inf_elem_ptr->buf_action = FBE_ESES_TRACE_BUF_ACTION_CTRL_CLEAR_BUF;  

    */

    return;
}


static void buf_cmds_test_build_str_out_page(fbe_sg_element_t *sg_p, fbe_u8_t *str)
{
    fbe_eses_pg_str_out_t *str_out_page = NULL;

    str_out_page = (fbe_eses_pg_str_out_t *)fbe_sg_element_address(sg_p);
    memset(str_out_page, 0, sizeof(fbe_eses_pg_str_out_t));
    str_out_page->page_code = SES_PG_CODE_STR_OUT;
    str_out_page->echo = 1;
    memcpy(str_out_page->str_out_data, str, (strlen((char *)str)+1));

    return;
}
