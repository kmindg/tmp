#include "terminator_drive_sas_plugin_test.h"
#include "terminator_sas_io_api.h"
#include "fbe_transport_memory.h"
#include "terminator_sas_io_neit.h"
#include "fbe/fbe_payload_cdb_operation.h"
#include "fbe/fbe_payload_operation_header.h"
#include "fbe/fbe_payload_ex.h"


/*****************************/
/*     local functions       */
/*****************************/
static fbe_sg_element_t* drive_sas_plugin_alloc_memory_with_sg(fbe_u32_t bytes);
static fbe_status_t drive_sas_plugin_payload_execute_command(fbe_payload_ex_t * payload_p, 
                                     fbe_cpd_device_id_t cpd_device_id,
                                     fbe_port_number_t port_number,
                                     fbe_payload_block_operation_opcode_t opcode,
                                     fbe_lba_t lba,
                                     fbe_block_count_t block_count,
                                     fbe_block_size_t block_size);

/* This code gets called as part of fbe_terminator_sas_drive_payload. 
 * Since this code is defined separately we need to pull it in here.
 */
fbe_status_t terminator_drive_increment_error_count(fbe_terminator_device_ptr_t handle)
{
    if (handle == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        return FBE_STATUS_OK;
    }
}

void drive_sas_plugin_error_injection_state_transition_test(void)
{
    fbe_status_t         status = FBE_STATUS_GENERIC_FAILURE;

    /* Make a new error_record */
    fbe_terminator_neit_error_record_t error_record;

    /* Initialize the error record */    
    status = fbe_error_record_init(&error_record);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Fill in error values */
    error_record.drive.port_number = 0;
    error_record.drive.enclosure_number = 0;
    error_record.drive.slot_number = 0;
    error_record.lba_start = 0x50;
    error_record.lba_end = 0x51;
    error_record.opcode = 0x28;
    error_record.error_sk = 0x01;
    error_record.error_asc = 0x15;
    error_record.error_ascq = 0x01;        
    error_record.num_of_times_to_insert = 1;   
   
    /* Remove all of the errors from error injection */
    status = fbe_neit_close();                                               
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
   
    /* Add the new error record to the drive error table
     * This should fail since error injection is not initialized
     */    
    status = fbe_neit_insert_error_record(error_record);
    MUT_ASSERT_TRUE(status != FBE_STATUS_OK);
   
    /* Start error injection
     * This should fail since error injection is not initialized    
     */
    status = fbe_neit_error_injection_start();
    MUT_ASSERT_TRUE(status != FBE_STATUS_OK);
   
    /* Stop error injection
     * This should fail since error injection is not initialized
     */
    status = fbe_neit_error_injection_stop();
    MUT_ASSERT_TRUE(status != FBE_STATUS_OK);
    
    /* Remove all of the errors from error injection
     * This should pass even if error injection is not initialized    
     */
    status = fbe_neit_close();                                               
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initialize error injection */
    status = fbe_neit_init();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);    
     
    /* Add the new error record to the drive error table */    
    status = fbe_neit_insert_error_record(error_record);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    /* Start error injection */
    status = fbe_neit_error_injection_start();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    error_record.drive.port_number = 0;
    error_record.drive.enclosure_number = 0;
    error_record.drive.slot_number = 0;

    /* Add another error record to the drive error table */    
    status = fbe_neit_insert_error_record(error_record);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);    
    
    /* Stop error injection */
    status = fbe_neit_error_injection_stop();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);    

    error_record.drive.port_number = 0;
    error_record.drive.enclosure_number = 0;
    error_record.drive.slot_number = 0;
    error_record.lba_start = 0x60;
    error_record.lba_end = 0x6F;
    /* Add another error record to the drive error table */    
    status = fbe_neit_insert_error_record(error_record);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);    

    /* Start error injection */
    status = fbe_neit_error_injection_start();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Remove all of the errors from error injection */
    status = fbe_neit_close();                                               
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    error_record.drive.port_number = 0;
    error_record.drive.enclosure_number = 0;
    error_record.drive.slot_number = 0;
    error_record.lba_start = 0x60;
    error_record.lba_end = 0x6F;
    /* Add another error record to the drive error table */    
    status = fbe_neit_insert_error_record(error_record);
    MUT_ASSERT_TRUE(status != FBE_STATUS_OK);

    /* Remove all of the errors from error injection */
    status = fbe_neit_close();                                               
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_HIGH, "%s %s test completed!",__FILE__, __FUNCTION__);
}


void drive_sas_plugin_error_injection_read_test(void)
{
    fbe_status_t         status = FBE_STATUS_GENERIC_FAILURE;
    //fbe_payload_cdb_operation_t* op_exe_cdb = NULL;
    fbe_u8_t* sense_info_buffer = NULL;    

    /* Command related */    
    //fbe_payload_ex_t        cmd_payload;
    fbe_cpd_device_id_t  cmd_cpd_device_id;
    fbe_port_number_t    cmd_port_number;
    fbe_lba_t            cmd_lba;
    fbe_block_count_t    cmd_num_of_blocks;
    fbe_block_size_t     cmd_block_size;
    fbe_payload_block_operation_opcode_t  cmd_opcode;    

    /* Make a new error_record */
    fbe_terminator_neit_error_record_t error_record;

    /* Setup the  payload */
    //status = fbe_payload_ex_init(&cmd_payload);
    //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    //op_exe_cdb = fbe_payload_ex_allocate_cdb_operation(&cmd_payload);
    //MUT_ASSERT_TRUE(op_exe_cdb != NULL);
    
    //status = fbe_payload_ex_increment_cdb_operation_level(&cmd_payload);
    //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initialize error injection */
    status = fbe_neit_init();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        
    
   /*
    ******************************************************
    * Check that an error is inserted
    ******************************************************
    */    
    
    /* Initialize data for a read command */
    cmd_cpd_device_id = 4;
    cmd_port_number = 0; 
    cmd_lba = 0x50;
    cmd_num_of_blocks = 1;
    cmd_block_size = 520;    
    cmd_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ;
    
    /* Initialize the error record */    
    status = fbe_error_record_init(&error_record);
   
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* Fill in error values */
    error_record.drive.port_number = 0;
    error_record.drive.enclosure_number = 0;
    error_record.drive.slot_number = 0;
    error_record.lba_start = 0x50;
    error_record.lba_end = 0x51;
    error_record.opcode = 0x28;
    error_record.error_sk = 0x01;
    error_record.error_asc = 0x15;
    error_record.error_ascq = 0x01;        
    error_record.num_of_times_to_insert = 1;
    /* Add the new error record to the drive error table */    
    status = fbe_neit_insert_error_record(error_record);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    /* Start error injection */
    status = fbe_neit_error_injection_start();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    /* Execute the command */
    //status = drive_sas_plugin_payload_execute_command(&cmd_payload, 
    //                                                  cmd_cpd_device_id,
    //                                                  cmd_port_number,
    //                                                  cmd_opcode,
    //                                                  cmd_lba,
    //                                                  cmd_num_of_blocks,
    //                                                  cmd_num_of_blocks);

    /* Check for no error on the call */
    //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);                                                   

    /* Check the results returned by the command
     */
    //op_exe_cdb = fbe_payload_ex_get_cdb_operation(&cmd_payload);
    //sense_info_buffer = &(op_exe_cdb->sense_info_buffer[0]);

    /* Check that the SCSI status is as expected */
    //MUT_ASSERT_TRUE(op_exe_cdb->payload_cdb_scsi_status ==
    //                         FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION);
    /* Check the sense buffer for proper returned sense data */                                    
    //MUT_ASSERT_TRUE(sense_info_buffer[0] == 0xF0);    /* Error Code */
    //MUT_ASSERT_TRUE(sense_info_buffer[2] == 0x01);    /* SK */
    //MUT_ASSERT_TRUE(sense_info_buffer[12] == 0x15);   /* ASC */
    //MUT_ASSERT_TRUE(sense_info_buffer[13] == 0x01);   /* ASCQ */

   /*
    ******************************************************
    * Check that an error is not injected if error
    * injection is turned off
    ******************************************************
    */  

    /* Stop error injection */
    status = fbe_neit_error_injection_stop();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

   /* Execute the command */
    //status = drive_sas_plugin_payload_execute_command(&cmd_payload, 
    //                                                  cmd_cpd_device_id,
    //                                                  cmd_port_number,
    //                                                  cmd_opcode,
    //                                                  cmd_lba,
    //                                                  cmd_num_of_blocks,
    //                                                  cmd_num_of_blocks);
    /* Check for no error on the call */
    //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);                                                   

    //op_exe_cdb = fbe_payload_ex_get_cdb_operation(&cmd_payload);
    //MUT_ASSERT_TRUE(op_exe_cdb->payload_cdb_scsi_status ==
    //                                FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD);

    //sense_info_buffer = &(op_exe_cdb->sense_info_buffer[0]);
    
    /* Check the sense buffer for proper returned sense data */                                    
    //MUT_ASSERT_TRUE(sense_info_buffer[0] == 0x00);    /* Error Code */
    //MUT_ASSERT_TRUE(sense_info_buffer[2] == 0x00);    /* SK */
    //MUT_ASSERT_TRUE(sense_info_buffer[12] == 0x00);   /* ASC */
    //MUT_ASSERT_TRUE(sense_info_buffer[13] == 0x00);   /* ASCQ */    

    /* Remove all of the errors from error injection */
    status = fbe_neit_close();                                               
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    //status = fbe_payload_ex_release_cdb_operation(&cmd_payload, op_exe_cdb);
    //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    mut_printf(MUT_LOG_HIGH, "%s %s test completed!",__FILE__, __FUNCTION__);
}


void drive_sas_plugin_error_injection_reset_test(void)
{

   /*
    ******************************************************
    * Test number of times that an error will be inserted
    ******************************************************
    */

    fbe_status_t  status = FBE_STATUS_GENERIC_FAILURE;
    //fbe_payload_cdb_operation_t* op_exe_cdb = NULL;
    fbe_u8_t* sense_info_buffer = NULL;   
    fbe_u32_t cnt = 0; 

    /* Command related */    
    //fbe_payload_ex_t        cmd_payload;
    fbe_cpd_device_id_t  cmd_cpd_device_id;
    fbe_port_number_t    cmd_port_number;
    fbe_lba_t            cmd_lba;
    fbe_block_count_t    cmd_num_of_blocks;
    fbe_block_size_t     cmd_block_size;
    fbe_payload_block_operation_opcode_t  cmd_opcode;    
   
    /* Make a new error_record */
    fbe_terminator_neit_error_record_t error_record;

    /* Setup the  payload */
    //status = fbe_payload_ex_init(&cmd_payload);
    //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    //op_exe_cdb = fbe_payload_ex_allocate_cdb_operation(&cmd_payload);
    //MUT_ASSERT_TRUE(op_exe_cdb != NULL);
    
    //status = fbe_payload_ex_increment_cdb_operation_level(&cmd_payload);
    //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initialize data for a read command */
    cmd_cpd_device_id = 4;
    cmd_port_number = 0; 
    cmd_lba = 0x50;
    cmd_num_of_blocks = 1;
    cmd_block_size = 520;    
    cmd_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ;
    
    /* Initialize error injection */
    status = fbe_neit_init();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);    

    /* Initialize the error record */    
    status = fbe_error_record_init(&error_record);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    /* Fill in error values */
    error_record.drive.port_number = 0;
    error_record.drive.enclosure_number = 0;
    error_record.drive.slot_number = 0;
    error_record.lba_start = 0x50;
    error_record.lba_end = 0x51;
    error_record.opcode = 0x28;
    error_record.error_sk = 0x01;
    error_record.error_asc = 0x5D;
    error_record.error_ascq = 0x02;        
    error_record.num_of_times_to_insert = 15;
    
    /* Add the new error record to the drive error table */    
    status = fbe_neit_insert_error_record(error_record);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    /* Start error injection */
    status = fbe_neit_error_injection_start();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);                                              
    
    for (cnt = 0; cnt < error_record.num_of_times_to_insert; cnt++)
    {
        /* Execute the command */
        //status = drive_sas_plugin_payload_execute_command(&cmd_payload, 
        //                                                  cmd_cpd_device_id,
        //                                                  cmd_port_number,
        //                                                  cmd_opcode,
        //                                                  cmd_lba,
        //                                                  cmd_num_of_blocks,
        //                                                  cmd_num_of_blocks);
        /* Check for no error on the call */
        //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);                                                   

        /* Check the results returned by the command
         */
        //op_exe_cdb = fbe_payload_ex_get_cdb_operation(&cmd_payload);
        //sense_info_buffer = &(op_exe_cdb->sense_info_buffer[0]);

        /* Check that the SCSI status is as expected */
        //MUT_ASSERT_TRUE(op_exe_cdb->payload_cdb_scsi_status ==
        //                         FBE_IO_BLOCK_SCSI_STATUS_CHECK_CONDITION);
        ///* Check the sense buffer for proper returned sense data */                                    
        //MUT_ASSERT_TRUE(sense_info_buffer[0] == 0xF0);    /* Error Code */
        //MUT_ASSERT_TRUE(sense_info_buffer[2] == 0x01);    /* SK */
        //MUT_ASSERT_TRUE(sense_info_buffer[12] == 0x5D);   /* ASC */
        //MUT_ASSERT_TRUE(sense_info_buffer[13] == 0x02);   /* ASCQ */
    }

    /* Execute the command */
	//status = drive_sas_plugin_payload_execute_command(&cmd_payload, 
	//													cmd_cpd_device_id,
	//													cmd_port_number,
	//													cmd_opcode,
	//													cmd_lba,
	//													cmd_num_of_blocks,
	//													cmd_num_of_blocks);
    /* Check for no error on the call */
	//MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);                                                   

    /* Check the results returned by the command
     */
    //op_exe_cdb = fbe_payload_ex_get_cdb_operation(&cmd_payload);

    /* Check that the SCSI status is as expected */
    //MUT_ASSERT_TRUE(op_exe_cdb->payload_cdb_scsi_status ==
    //                             FBE_IO_BLOCK_SCSI_STATUS_GOOD);

	//sense_info_buffer = &(op_exe_cdb->sense_info_buffer[0]);

    /* Check the sense buffer for proper returned sense data */                                    
    //MUT_ASSERT_TRUE(sense_info_buffer[0] == 0x00);    /* Error Code */
    //MUT_ASSERT_TRUE(sense_info_buffer[2] == 0x00);    /* SK */
    //MUT_ASSERT_TRUE(sense_info_buffer[12] == 0x00);   /* ASC */
    //MUT_ASSERT_TRUE(sense_info_buffer[13] == 0x00);   /* ASCQ */

    /* Remove all of the errors from error injection */
    status = fbe_neit_close();                                               
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    //status = fbe_payload_ex_release_cdb_operation(&cmd_payload, op_exe_cdb);
    //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_HIGH, "%s %s test completed!",__FILE__, __FUNCTION__);
}                               

void drive_sas_plugin_error_injection_reactivate_test(void)
{

   /*
    *********************************************************
    * Test number of times that an error will be reactivated
    *********************************************************
    */

    fbe_status_t  status = FBE_STATUS_GENERIC_FAILURE;
    //fbe_payload_cdb_operation_t* op_exe_cdb = NULL;
    //fbe_u8_t* sense_info_buffer = NULL;
    fbe_u32_t cnt_reactivate = 0;
    fbe_u32_t cnt_reset = 0; 

    /* Command related */    
    //fbe_payload_ex_t        cmd_payload;
    fbe_cpd_device_id_t  cmd_cpd_device_id;
    fbe_port_number_t    cmd_port_number;
    fbe_lba_t            cmd_lba;
    fbe_block_count_t    cmd_num_of_blocks;
    fbe_block_size_t     cmd_block_size;
    fbe_payload_block_operation_opcode_t  cmd_opcode;    
   
    /* Make a new error_record */
    fbe_terminator_neit_error_record_t error_record;

    /* Setup the  payload */
    //status = fbe_payload_ex_init(&cmd_payload);
    //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    //op_exe_cdb = fbe_payload_ex_allocate_cdb_operation(&cmd_payload);
    //MUT_ASSERT_TRUE(op_exe_cdb != NULL);
    
    //status = fbe_payload_ex_increment_cdb_operation_level(&cmd_payload);
    //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initialize data for a read command */
    cmd_cpd_device_id = 4;
    cmd_port_number = 0; 
    cmd_lba = 0x50;
    cmd_num_of_blocks = 1;
    cmd_block_size = 520;    
    cmd_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ;
    
    /* Initialize error injection */
    status = fbe_neit_init();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);    

    /* Initialize the error record */    
    status = fbe_error_record_init(&error_record);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    /* Fill in error values */
    error_record.drive.port_number = 0;
    error_record.drive.enclosure_number = 0;
    error_record.drive.slot_number = 0;
    error_record.lba_start = 0x50;
    error_record.lba_end = 0x51;
    error_record.opcode = 0x28;
    error_record.error_sk = 0x01;
    error_record.error_asc = 0x5D;
    error_record.error_ascq = 0x02;        
    error_record.num_of_times_to_insert = 2;
    error_record.secs_to_reactivate = 0;
    error_record.num_of_times_to_reactivate = 3;    
    
    /* Add the new error record to the drive error table */    
    status = fbe_neit_insert_error_record(error_record);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    /* Start error injection */
    status = fbe_neit_error_injection_start();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);                                              
    
    for (cnt_reactivate = 0;
                cnt_reactivate <= error_record.num_of_times_to_reactivate;
                cnt_reactivate++)
    {
        for (cnt_reset = 0;
                cnt_reset < error_record.num_of_times_to_insert;
                cnt_reset++)
        {
            /* Execute the command */
            //status = drive_sas_plugin_payload_execute_command(&cmd_payload, 
            //                                                  cmd_cpd_device_id,
            //                                                  cmd_port_number,
            //                                                  cmd_opcode,
            //                                                  cmd_lba,
            //                                                  cmd_num_of_blocks,
            //                                                  cmd_num_of_blocks);
            /* Check for no error on the call */
            //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);                                                   

            /* Check the results returned by the command
             */
            //op_exe_cdb = fbe_payload_ex_get_cdb_operation(&cmd_payload);

            /* Check that the SCSI status is as expected */
            //MUT_ASSERT_TRUE(op_exe_cdb->payload_cdb_scsi_status ==
            //                         FBE_IO_BLOCK_SCSI_STATUS_CHECK_CONDITION);
                                      
            //sense_info_buffer = &(op_exe_cdb->sense_info_buffer[0]);
            
            /* Check the sense buffer for proper returned sense data */                                    
            //MUT_ASSERT_TRUE(sense_info_buffer[0] == 0xF0);    /* Error Code */
            //MUT_ASSERT_TRUE(sense_info_buffer[2] == 0x01);    /* SK */
            //MUT_ASSERT_TRUE(sense_info_buffer[12] == 0x5D);   /* ASC */
            //MUT_ASSERT_TRUE(sense_info_buffer[13] == 0x02);   /* ASCQ */
        }
    }

    /* Execute the command */
    //status = drive_sas_plugin_payload_execute_command(&cmd_payload, 
    //                                                  cmd_cpd_device_id,
    //                                                  cmd_port_number,
    //                                                  cmd_opcode,
    //                                                  cmd_lba,
    //                                                  cmd_num_of_blocks,
    //                                                  cmd_num_of_blocks);
    /* Check for no error on the call */
    //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);                                                   

    /* Check the results returned by the command
     */
    //op_exe_cdb = fbe_payload_ex_get_cdb_operation(&cmd_payload);
    
    /* Check that the SCSI status is as expected */
    //MUT_ASSERT_TRUE(op_exe_cdb->payload_cdb_scsi_status ==
    //                             FBE_IO_BLOCK_SCSI_STATUS_GOOD);
                                 
    //sense_info_buffer = &(op_exe_cdb->sense_info_buffer[0]);                                 

    /* Check the sense buffer for proper returned sense data */                                    
    //MUT_ASSERT_TRUE(sense_info_buffer[0] == 0x00);    /* Error Code */
    //MUT_ASSERT_TRUE(sense_info_buffer[2] == 0x00);    /* SK */
    //MUT_ASSERT_TRUE(sense_info_buffer[12] == 0x00);   /* ASC */
    //MUT_ASSERT_TRUE(sense_info_buffer[13] == 0x00);   /* ASCQ */

    /* Remove all of the errors from error injection */
    status = fbe_neit_close();                                               
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    //status = fbe_payload_ex_release_cdb_operation(&cmd_payload, op_exe_cdb);
    //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_HIGH, "%s %s test completed!",__FILE__, __FUNCTION__);
} 

void drive_sas_plugin_error_injection_reactivate_time_test(void)
{

   /*
    ******************************************************
    * Test the reactivate time interval
    ******************************************************
    */

    fbe_status_t  status = FBE_STATUS_GENERIC_FAILURE;
    //fbe_payload_cdb_operation_t* op_exe_cdb = NULL;
    //fbe_u8_t* sense_info_buffer = NULL;
    fbe_u32_t cnt_reset = 0; 

    /* Command related */    
	//fbe_payload_ex_t        cmd_payload;
    fbe_cpd_device_id_t  cmd_cpd_device_id;
    fbe_port_number_t    cmd_port_number;
    fbe_lba_t            cmd_lba;
    fbe_block_count_t    cmd_num_of_blocks;
    fbe_block_size_t     cmd_block_size;
    fbe_payload_block_operation_opcode_t  cmd_opcode;    
   
    /* Make a new error_record */
    fbe_terminator_neit_error_record_t error_record;

    /* Setup the  payload */
    //status = fbe_payload_ex_init(&cmd_payload);
    //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    //op_exe_cdb = fbe_payload_ex_allocate_cdb_operation(&cmd_payload);
    //MUT_ASSERT_TRUE(op_exe_cdb != NULL);
    
    //status = fbe_payload_ex_increment_cdb_operation_level(&cmd_payload);
    //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initialize data for a read command */
    cmd_cpd_device_id = 4;
    cmd_port_number = 0; 
    cmd_lba = 0x50;
    cmd_num_of_blocks = 1;
    cmd_block_size = 520;    
    cmd_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ;
    
    /* Initialize error injection */
    status = fbe_neit_init();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);    

    /* Initialize the error record */    
    status = fbe_error_record_init(&error_record);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    /* Fill in error values */
    error_record.drive.port_number = 0;
    error_record.drive.enclosure_number = 0;
    error_record.drive.slot_number = 0;
    error_record.lba_start = 0x50;
    error_record.lba_end = 0x51;
    error_record.opcode = 0x28;
    error_record.error_sk = 0x01;
    error_record.error_asc = 0x5D;
    error_record.error_ascq = 0x02;        
    error_record.num_of_times_to_insert = 2;
    /* Set the reactivate time to a very large value so that
     * reactivation will not occur during the short time
     * delay in this test (the delay of a few instructions).
     */
    error_record.secs_to_reactivate = 64000;
    error_record.num_of_times_to_reactivate = 3;    
    
    /* Add the new error record to the drive error table */    
    status = fbe_neit_insert_error_record(error_record);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    /* Start error injection */
    status = fbe_neit_error_injection_start();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);                                              
    
    for (cnt_reset = 0;
                cnt_reset < error_record.num_of_times_to_insert;
                cnt_reset++)
    {
        /* Execute the command */
        //status = drive_sas_plugin_payload_execute_command(&cmd_payload, 
        //                                                  cmd_cpd_device_id,
        //                                                  cmd_port_number,
        //                                                  cmd_opcode,
        //                                                  cmd_lba,
        //                                                  cmd_num_of_blocks,
        //                                                  cmd_num_of_blocks);
        /* Check for no error on the call */
        //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);                                                   

        /* Check the results returned by the command
         */
        //op_exe_cdb = fbe_payload_ex_get_cdb_operation(&cmd_payload);

        /* Check that the SCSI status is as expected */
        //MUT_ASSERT_TRUE(op_exe_cdb->payload_cdb_scsi_status ==
        //                         FBE_IO_BLOCK_SCSI_STATUS_CHECK_CONDITION);
                                                 
        //sense_info_buffer = &(op_exe_cdb->sense_info_buffer[0]);                                 
                                 
        /* Check the sense buffer for proper returned sense data */                                    
        //MUT_ASSERT_TRUE(sense_info_buffer[0] == 0xF0);    /* Error Code */
        //MUT_ASSERT_TRUE(sense_info_buffer[2] == 0x01);    /* SK */
        //MUT_ASSERT_TRUE(sense_info_buffer[12] == 0x5D);   /* ASC */
        //MUT_ASSERT_TRUE(sense_info_buffer[13] == 0x02);   /* ASCQ */
    }

    /* Execute the command */
    //status = drive_sas_plugin_payload_execute_command(&cmd_payload, 
    //                                                  cmd_cpd_device_id,
    //                                                  cmd_port_number,
    //                                                  cmd_opcode,
    //                                                  cmd_lba,
    //                                                  cmd_num_of_blocks,
    //                                                  cmd_num_of_blocks);
    /* Check for no error on the call */
    //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);                                                   

    /* Check the results returned by the command
     */
    //op_exe_cdb = fbe_payload_ex_get_cdb_operation(&cmd_payload);
    
    //sense_info_buffer = &(op_exe_cdb->sense_info_buffer[0]);
    
    /* Check that the error did not reactivate and SCSI status
     * is good
     */
    //MUT_ASSERT_TRUE(op_exe_cdb->payload_cdb_scsi_status ==
    //                             FBE_IO_BLOCK_SCSI_STATUS_GOOD);
                                 
    /* Check the sense buffer for proper returned sense data */                                    
    //MUT_ASSERT_TRUE(sense_info_buffer[0] == 0x00);    /* Error Code */
    //MUT_ASSERT_TRUE(sense_info_buffer[2] == 0x00);    /* SK */
    //MUT_ASSERT_TRUE(sense_info_buffer[12] == 0x00);   /* ASC */
    //MUT_ASSERT_TRUE(sense_info_buffer[13] == 0x00);   /* ASCQ */

    /* Remove all of the errors from error injection */
    status = fbe_neit_close();                                               
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    //status = fbe_payload_ex_release_cdb_operation(&cmd_payload, op_exe_cdb);
    //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_HIGH, "%s %s test completed!",__FILE__, __FUNCTION__);
} 

static fbe_status_t drive_sas_plugin_sas_physical_drive_cdb_build_16_byte(fbe_payload_cdb_operation_t* cdb_operation,
                                                  fbe_time_t timeout,
                                                  fbe_lba_t lba,
                                                  fbe_block_count_t block_count)
{
    fbe_status_t status = FBE_STATUS_OK;
    
    /* Set common parameters */    
    cdb_operation->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cdb_operation->timeout = timeout;    
    cdb_operation->payload_sg_descriptor.repeat_count = 1;
    cdb_operation->cdb_length = 16;      
    
    /* Set the lba and blockcount */       
    cdb_operation->cdb[2] = (fbe_u32_t)((lba >> 56) & 0xFFFF); /* MSB */
    cdb_operation->cdb[3] |= (fbe_u32_t)((lba >> 48) & 0xFFFF); 
    cdb_operation->cdb[4] |= (fbe_u32_t)((lba >> 40) & 0xFFFF);
    cdb_operation->cdb[5] |= (fbe_u32_t)((lba >> 32) & 0xFFFF); 
    cdb_operation->cdb[6] |= (fbe_u32_t)((lba >> 24) & 0xFFFF); 
    cdb_operation->cdb[7] |= (fbe_u32_t)((lba >> 16) & 0xFFFF); 
    cdb_operation->cdb[8] |= (fbe_u32_t)((lba >> 8) & 0xFFFF); 
    cdb_operation->cdb[9] |= (fbe_u32_t)(lba & 0xFFFF); /* LSB */
    cdb_operation->cdb[10] = (fbe_u32_t)((block_count >> 24) & 0xFFFF); /* MSB */
    cdb_operation->cdb[11] |= (fbe_u32_t)((block_count >> 16) & 0xFFFF); 
    cdb_operation->cdb[12] |= (fbe_u32_t)((block_count >> 8) & 0xFFFF);
    cdb_operation->cdb[13] |= (fbe_u32_t)(block_count & 0xFFFF); /* LSB */       
    
    return status;
}

static fbe_status_t drive_sas_plugin_payload_execute_command(fbe_payload_ex_t * payload_p, 
                                     fbe_cpd_device_id_t cpd_device_id,
                                     fbe_port_number_t port_number,
                                     fbe_payload_block_operation_opcode_t opcode,
                                     fbe_lba_t lba,
                                     fbe_block_count_t block_count,
                                     fbe_block_size_t block_size)
{
    fbe_sg_element_t    *sg_list = NULL;
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t        drive_handle[1]; /* get an arbitrary pointer for the drive handle.  Mock does not use the handle!!! */
	fbe_payload_cdb_operation_t * cdb_operation = NULL;

	cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload_p);

    cdb_operation->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cdb_operation->payload_cdb_scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD;
    cdb_operation->port_request_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
    /* In sumulation set timeout to 1 */
    cdb_operation->timeout = 1;
    /* cdb_operation->block_size = block_size;*/
    
    fbe_zero_memory(cdb_operation->cdb, FBE_PAYLOAD_CDB_CDB_SIZE);
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);
    
    /* Create an sg for the data and add it to the io_block*/
    sg_list = drive_sas_plugin_alloc_memory_with_sg( block_count * block_size );
    if(sg_list == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;    
    }
    
    status = fbe_payload_ex_set_sg_list(payload_p, sg_list, 1);
    if(status != FBE_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;    
    } 

    /* Construct a 16 byte CDB.
     */
    cdb_operation->cdb_length = 16;

    /* Choose the operation based on the input opcode.
     */
    switch (opcode)
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:
            cdb_operation->cdb[0] = FBE_SCSI_READ_16;
            cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_IN;

            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:
            cdb_operation->cdb[0] = FBE_SCSI_WRITE_16;
            cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_OUT;

            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME:
            cdb_operation->cdb[0] = FBE_SCSI_WRITE_SAME_16;
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY:
            cdb_operation->cdb[0] = FBE_SCSI_VERIFY_16;
            break;
        default:
            /* Unknown operation.
             */
            return FBE_STATUS_GENERIC_FAILURE;
            break;
    };

    /* Assume 16 byte cdb for now.
     * Fill out our lba, block count and block size fields.
     */
    drive_sas_plugin_sas_physical_drive_cdb_build_16_byte(cdb_operation, 0, lba, block_count);
    
    /* payload_p->cdb_operation.block_size = block_size; */
    
    
    /* Send the command to terminator */
    fbe_payload_ex_increment_cdb_operation_level(payload_p);
    status = fbe_terminator_sas_drive_payload(payload_p, drive_handle);
    if(status != FBE_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;    
    }    

	fbe_payload_ex_release_cdb_operation(payload_p, cdb_operation);

    free(sg_list);

    return FBE_STATUS_OK;
}


static fbe_sg_element_t * drive_sas_plugin_alloc_memory_with_sg( fbe_u32_t bytes )
{
    fbe_sg_element_t *sg_p;
    fbe_u8_t *sectors_p;
    fbe_u8_t *memory_p;

    /* Allocate the memory.
     * We also reserve 2 sg entries for the sg list we are using. 
     * (one for the sg entry with data and one for the null terminator).
     */
    memory_p = malloc(bytes + (sizeof(fbe_sg_element_t) * 2));
    
    if (memory_p == NULL)
    {
        return NULL;
    }
    /* Place sg at beginning of memory.
     */
    sg_p = (fbe_sg_element_t *)memory_p;

    /* Place data memory after the sg memory (2 sg entries).
     */
    sectors_p = memory_p + (sizeof(fbe_sg_element_t) * 2);

    /* Init the sgs with the memory we just allocated.
     */
    fbe_sg_element_init(&sg_p[0], bytes, sectors_p);
    fbe_sg_element_terminate(&sg_p[1]);
    
    return sg_p;
}


