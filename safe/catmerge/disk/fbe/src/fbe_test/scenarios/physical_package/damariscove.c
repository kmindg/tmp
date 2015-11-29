/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * damariscove.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains the test functions for the "Damariscove" scenario.
 * 
 *  This scenario creates a 1 board, 1 port, 1 enclosure configuration with 12 drivers.

 *  The scenario:
 *  - Inject specific errors to a drive..
 *  - Issue an WRITE I/O to the drive from MUT.
 *  - If the I/O is inside the range of error injection, validates that the error counter is indeed incremented. 
 *
 * HISTORY
 *   01/12/2009:  Created. Chenl6
 *  11/20/2009:  Migrated from zeus_ready_test. Bo Gao
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "physical_package_tests.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe_test_configurations.h"
#include "fbe_test_package_config.h"
#include "fbe_test_io_api.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_api_sim_server.h" 
#include "neit_utils.h"

/* This is the number of drives in the scenario. 
 * 1 board 
 * 1 port 
 * 1 enclosure 
 * 12 physical drives 
 * 12 logical drives 
 */
#define DAMARISCOVE_520_DRIVE_NUMBER_FIRST 0
#define DAMARISCOVE_520_DRIVE_NUMBER_LAST 11

 /*************************
  *   FUNCTION DEFINITIONS
  *************************/

/* This is the set of I/O test cases to be run against 520 drives in this test.
 * These ranges are typically arbitrary.  We try to cover interesting 
 * combination of lbas and block counts. 
 * The max block counts of 1024 or 0x5000 are arbitrary.
 */
fbe_api_rdgen_test_io_case_t fbe_damariscove_520_io_case_array[] =
{
    /* 
     * exported imported start  end     start    end     increment exp opt   imp opt
     *  bl size block    lba    lba     blocks   blocks  blocks    bl sizes  bl size
     */
    {520,       520,    0x0,    0x20,    1,       1024,       1,       1,        1},
    {520,       520,    0x100,  0x110,   1,       1024,       1,       1,        1},

    /* This is the terminator record, it always should be zero.
     */
    {FBE_TEST_IO_INVALID_FIELD, 0,         0,    0,    0,       0, 0},

};
/* end fbe_damariscove_520_io_case_array global structure */

/*!**************************************************************
 * @fn damariscove_run()
 ****************************************************************
 * @brief
 *  This function executes the damariscove scenario.
 * 
 *  This scenario will kick off a read test and inject recoveable error, the retry and remap.
 *  
 * @param None.               
 *
 * @return None.
 *
 * HISTORY:
 *   01/12/2009:  Created. Chenl6
 ****************************************************************/

static fbe_status_t damariscove_run(void)
{
    fbe_u32_t object_handle;
    fbe_object_id_t object_id;
    fbe_status_t status;
    fbe_protocol_error_injection_error_record_t record;
    fbe_protocol_error_injection_record_handle_t record_handle1, record_handle2;
    fbe_api_dieh_stats_t old_dieh_stats, new_dieh_stats;

    status = fbe_api_get_physical_drive_object_id_by_location(0, 0, DAMARISCOVE_520_DRIVE_NUMBER_FIRST, &object_handle);//Test for enclosure 0
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(object_handle, FBE_OBJECT_ID_INVALID);
    object_id = (fbe_object_id_t)object_handle;

    /* Get error counts before running IOs. 
     */
    status = fbe_api_physical_drive_get_dieh_info(object_handle, &old_dieh_stats, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "%s: get remapped block counts: %d.", 
                     __FUNCTION__, old_dieh_stats.error_counts.physical_drive_error_counts.remap_block_count);  

    mut_printf(MUT_LOG_HIGH, "%s %s start error injection!",__FILE__, __FUNCTION__);

    // Initialize the protocl error injection record.
    fbe_test_neit_utils_init_error_injection_record(&record);

    // Put PDO object ID into the error record
    record.object_id = object_handle;

    // Fill in the fields in the error record that are common to all
    // the error cases.
    record.protocol_error_injection_error_type = FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.port_status =
        FBE_PORT_REQUEST_STATUS_SUCCESS;


    /* Case 1: inject error to WRITE command.
     */
    record.lba_start = 0x100;
    record.lba_end = 0x110;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[0] = 
        FBE_SCSI_WRITE_10;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_sense_key = 
        0x01;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code = 
        0x17;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code_qualifier = 
        0x00; 
    record.num_of_times_to_insert = 1;
    
    status = fbe_api_protocol_error_injection_add_record(&record, &record_handle1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
      
    //  Start the error injection 
    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_HIGH, "%s %s running IO!",__FILE__, __FUNCTION__);
    status = fbe_api_rdgen_test_io_run_write_only_test(fbe_damariscove_520_io_case_array,
                                                       object_id,
                                                       FBE_TEST_IO_INVALID_CASE_COUNT);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Stop the error injection 
    status = fbe_api_protocol_error_injection_stop(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Remove the error injection records
    status = fbe_api_protocol_error_injection_remove_record(record_handle1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
     

    /* Case 2: inject error to WRITE and RESERVE commands.
     */
    record.lba_start = 0x100;
    record.lba_end = 0x110;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[0] = 
        FBE_SCSI_WRITE_10;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_sense_key = 
        0x01;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code = 
        0x17;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code_qualifier = 
        0x00; 
    record.num_of_times_to_insert = 1;
    
    status = fbe_api_protocol_error_injection_add_record(&record, &record_handle1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
      

    record.lba_start = 0;
    record.lba_end = 0;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[0] = 
        FBE_SCSI_RESERVE_10;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_sense_key = 
        0x04;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code = 
        0x44;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code_qualifier = 
        0x00; 
    record.num_of_times_to_insert = 3;
    
    status = fbe_api_protocol_error_injection_add_record(&record, &record_handle2);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Start the error injection 
    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_HIGH, "%s %s running IO!",__FILE__, __FUNCTION__);
    status = fbe_api_rdgen_test_io_run_write_only_test(fbe_damariscove_520_io_case_array,
                                                       object_id,
                                                       FBE_TEST_IO_INVALID_CASE_COUNT);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Stop the error injection 
    status = fbe_api_protocol_error_injection_stop(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Remove the error injection records
    status = fbe_api_protocol_error_injection_remove_record(record_handle1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_protocol_error_injection_remove_record(record_handle2);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    /* Case 3: inject error to WRITE and REASSIGN commands.
     */
    record.lba_start = 0x100;
    record.lba_end = 0x110;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[0] = 
        FBE_SCSI_WRITE_10;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_sense_key = 
        0x01;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code = 
        0x17;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code_qualifier = 
        0x00;
    record.num_of_times_to_insert = 1;
    
    status = fbe_api_protocol_error_injection_add_record(&record, &record_handle1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
      
    record.lba_start = 0;
    record.lba_end = 0;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[0] = 
        FBE_SCSI_REASSIGN_BLOCKS;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_sense_key = 
        0x04;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code = 
        0x44;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code_qualifier = 
        0x00; 
    record.num_of_times_to_insert = 3;
    
    status = fbe_api_protocol_error_injection_add_record(&record, &record_handle2);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Start the error injection 
    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_HIGH, "%s %s running IO!",__FILE__, __FUNCTION__);
    status = fbe_api_rdgen_test_io_run_write_only_test(fbe_damariscove_520_io_case_array,
                                                       object_id,
                                                       FBE_TEST_IO_INVALID_CASE_COUNT);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Stop the error injection 
    status = fbe_api_protocol_error_injection_stop(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Remove the error injection records
    status = fbe_api_protocol_error_injection_remove_record(record_handle1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_protocol_error_injection_remove_record(record_handle2);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Case 4: inject error to WRITE and RELEASE commands.
     */
    record.lba_start = 0x100;
    record.lba_end = 0x110;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[0] = 
        FBE_SCSI_WRITE_10;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_sense_key = 
        0x01;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code = 
        0x17;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code_qualifier = 
        0x00;
    record.num_of_times_to_insert = 1;
    
    status = fbe_api_protocol_error_injection_add_record(&record, &record_handle1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
      
    record.lba_start = 0;
    record.lba_end = 0;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[0] = 
        FBE_SCSI_RELEASE_10;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_sense_key = 
        0x04;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code = 
        0x44;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code_qualifier = 
        0x00; 
    record.num_of_times_to_insert = 3;
    
    status = fbe_api_protocol_error_injection_add_record(&record, &record_handle2);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Start the error injection 
    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_HIGH, "%s %s running IO!",__FILE__, __FUNCTION__);
    status = fbe_api_rdgen_test_io_run_write_only_test(fbe_damariscove_520_io_case_array,
                                                       object_id,
                                                       FBE_TEST_IO_INVALID_CASE_COUNT);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Stop the error injection 
    status = fbe_api_protocol_error_injection_stop(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Remove the error injection records
    status = fbe_api_protocol_error_injection_remove_record(record_handle1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_protocol_error_injection_remove_record(record_handle2);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    /* Get DIEH information. */
    status = fbe_api_physical_drive_get_dieh_info(object_handle, &new_dieh_stats, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "%s: get remapped block counts: %d.", 
                      __FUNCTION__, new_dieh_stats.error_counts.physical_drive_error_counts.remap_block_count); 
    MUT_ASSERT_TRUE(old_dieh_stats.error_counts.physical_drive_error_counts.remap_block_count < new_dieh_stats.error_counts.physical_drive_error_counts.remap_block_count);
    return  status;
}


void damariscove(void)
{
    fbe_status_t run_status;

    run_status = damariscove_run();
        
    MUT_ASSERT_INT_EQUAL(run_status, FBE_STATUS_OK);
}

/*************************
 * end file damariscove.c
 *************************/
