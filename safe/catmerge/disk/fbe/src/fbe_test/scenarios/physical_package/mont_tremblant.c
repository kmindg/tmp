/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!***************************************************************************
 *@file Mont_Tremblant.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains the test functions for the "Mont Tremblant" scenario.
 * 
 *  This scenario creates a 1 board, 1 port, 1 enclosure configuration with 12 drivers.

 *  The scenario:
 *  - Inject a specific check condition to a drive..
 *  - Issue a read I/O to the drive to a drive from an XML file.
 *  - Issue an I/O to the drive from MUT.
 *  - If the I/O is outside the range of error injection, 
 *    validates that the error counter inside Physical Drive Object is not incremented;
 *    If the I/O is inside the range of error injection, validates that the error counter is indeed incremented. 
 *  - In both of the above cases, the status for this I/O should always be a success. And
 *    physical Drive Object does not change state and stay in READY.
 *
 * @version
 *    9/18/2008:  Created. CLC
 *   11/19/2009:  Migrated from zeus_ready_test. Bo Gao
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "physical_package_tests.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe_test_configurations.h"
#include "fbe_test_package_config.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_sim_server.h" 
#include "pp_utils.h"
#include "fbe_test_io_api.h"


/* This is the number of drives in the scenario. 
 * 1 board 
 * 1 port 
 * 1 enclosure 
 * 12 physical drives 
 */
#define MONT_TREMBLANT_NUMBER_OF_OBJECTS 15

/* We have one enclosure with 12 520 drives.
 */

#define MONT_TREMBLANT_520_DRIVE_NUMBER_FIRST 0
#define MONT_TREMBLANT_520_DRIVE_NUMBER_LAST 11

/* This is the max number of drives we use in this test.
 */
#define MONT_TREMBLANT_MAX_DRIVES 12
 /*************************
  *   FUNCTION DEFINITIONS
  *************************/

/* This is the set of I/O test cases to be run against 520 drives in this test.
 * These ranges are typically arbitrary.  We try to cover interesting 
 * combination of lbas and block counts. 
 * The max block counts of 1024 or 0x5000 are arbitrary.
 */
fbe_api_rdgen_test_io_case_t fbe_mont_tremblant_520_io_case_array[] =
{
    /* 
     * exported imported start  end     start    end     increment exp opt   imp opt
     *  bl size block    lba    lba     blocks   blocks  blocks    bl sizes  bl size
     */
    {520,       520,    0x0,    0x3,    1,       1024,       1,       1,        1},
    {520,       520,    0x100,  0x110,   1,        50,       1,       1,        1},

    /* This is the terminator record, it always should be zero.
     */
    {FBE_API_RDGEN_TEST_IO_INVALID_FIELD, 0,         0,    0,    0,       0, 0},

};
/* end fbe_mont_tremblant_520_io_case_array global structure */



static fbe_status_t validate_physical_drive(fbe_u32_t port,
                                            fbe_u32_t enclosure,
                                            fbe_u32_t slot,
                                            fbe_lifecycle_state_t expected_lifecycle_state)
{
    fbe_status_t status;
    fbe_u32_t object_handle_p;

    /* Check for the physical drives on the enclosure.
     * If status was bad, then the drive does not exist.
     */
    status = fbe_api_get_physical_drive_object_id_by_location(port, enclosure, slot, &object_handle_p);
         
    /* If status was bad, then return error.
     */
    if (status !=  FBE_STATUS_OK)
    {
        mut_printf(MUT_LOG_LOW,
                   "%s: ERROR can't get physical drive [%u] in slot [%u], enclosure [%u] on port [%u] lifecycle state, status: 0x%X.",
                   __FUNCTION__, object_handle_p, slot, enclosure, port, status);
        return status;
    }

    /* Get the lifecycle state of the drive.
     */
    status = fbe_test_pp_util_verify_pdo_state(port, 
                                                enclosure, 
                                                slot, 
                                                expected_lifecycle_state, 
                                                READY_STATE_WAIT_TIME);
      
    /* If the lifecycle state does not match, then display and return error.
     */
    if (status !=  FBE_STATUS_OK)
    {
        mut_printf(MUT_LOG_LOW,
                   "%s: ERROR - Lifecycle state does not match!",
                   __FUNCTION__);
        return status;
    }
    
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn mont_tremblant_run()
 ****************************************************************
 * @brief
 *  This function executes the mont tremblant scenario.
 * 
 *  This scenario will kick off a read test and inject recoveable error, the retry and remap.
 *  
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  09/18/2008  Created.  CLC. 
 ****************************************************************/

static fbe_status_t mont_tremblant_run(void)
{
    fbe_u32_t object_handle;
    fbe_object_id_t object_id_520;
    fbe_status_t status;
    fbe_u32_t port_number;
    fbe_terminator_neit_error_record_t record;
    fbe_api_dieh_stats_t dieh_stats;

    status = fbe_api_get_physical_drive_object_id_by_location(0, 0, 0, &object_handle);//Test for enclosure 0
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(object_handle, FBE_OBJECT_ID_INVALID);

    status = fbe_api_physical_drive_clear_error_counts(object_handle, FBE_PACKET_FLAG_NO_ATTRIB);//need to pass error structure
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Use the handle to get port number of the drive.
     */ 
    status = fbe_api_get_object_port_number (object_handle, &port_number);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(port_number == port_number);

    object_id_520 = (fbe_object_id_t)object_handle;

    /* Setup error injection.
     */
    mut_printf(MUT_LOG_HIGH, "%s %s start error injection!",__FILE__, __FUNCTION__);
    fbe_api_terminator_drive_error_injection_init();

    fbe_api_terminator_drive_error_injection_init_error(&record);
    
    record.drive.port_number = port_number;
    record.drive.enclosure_number = 0;
    record.drive.slot_number = 0;
    record.lba_start = 0x100;
    record.lba_end = 0x110;
    record.opcode = FBE_SCSI_WRITE_10;
    record.error_sk = 0x01;
    record.error_asc = 0x15;
    record.error_ascq = 0x00; 
    record.num_of_times_to_insert = 3;
    
    status = fbe_api_terminator_drive_error_injection_add_error(record);
     
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
      
    fbe_api_terminator_drive_error_injection_start(); 

    /* Kick off I/O to the 520 bytes per block drive.
     */
    mut_printf(MUT_LOG_HIGH, "%s %s running IO!",__FILE__, __FUNCTION__);
    status = fbe_api_rdgen_test_io_run_write_only_test(fbe_mont_tremblant_520_io_case_array,
                                                   object_id_520,
                                                   FBE_TEST_IO_INVALID_CASE_COUNT);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_api_terminator_drive_error_injection_stop();
     
    fbe_api_terminator_drive_error_injection_destroy();
    mut_printf(MUT_LOG_HIGH, "%s %s Error injection completed!",__FILE__, __FUNCTION__);
    
    /* TODO: Verify if PDO is still in READY state. */
    status = validate_physical_drive(port_number, 0, 0, FBE_LIFECYCLE_STATE_READY);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
      
    /* Get error counts. */
    status = fbe_api_get_physical_drive_object_id_by_location(0, 0, 0, &object_handle);//Test for enclosure 0
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(object_handle, FBE_OBJECT_ID_INVALID);

    /*FBE_PACKET_FLAG_NO_ATTRIB is used because we go directly to the physical drive and via the logical drive. 
     need to pass error structure */
    status = fbe_api_physical_drive_get_dieh_info(object_handle, &dieh_stats, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 
    /*
    mut_printf(MUT_LOG_LOW, "%s: get recovered error counts: %d.", 
                   __FUNCTION__,
                    dieh_stats.error_counts.recoverable_error_count); 
    */
    return  status;
}

fbe_status_t mont_tremblant_load_and_verify(void)
{
    fbe_class_id_t      class_id;
    fbe_u32_t           object_handle = 0;
    fbe_u32_t           port_number = 0;
    fbe_u32_t           enclosure_number = 0;
    fbe_u32_t           drive_number;
    fbe_u32_t           total_objects = 0;
    fbe_status_t        status;

    /* There is some verbose tracing in the I/O path, so let's turn down 
     * the tracing level. 
     */
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_INFO);

    /* Before initializing the physical package, initialize the terminator. */
    status = fbe_api_terminator_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "%s: terminator initialized.", __FUNCTION__);

    /* Enable the IO mode.  This will cause the terminator to create
     * disk files as drive objects are created.
     */ 
    status = fbe_api_terminator_set_io_mode(FBE_TERMINATOR_IO_MODE_ENABLED);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Load the test config*/
    status = fbe_test_load_los_vilos_config();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "%s:  Mont Tremblant config loaded.", __FUNCTION__);

    /* Initialize the fbe_api on server. */ 
    mut_printf(MUT_LOG_LOW, "%s: starting fbe_api...", __FUNCTION__);
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initialize the physical package. */
    mut_printf(MUT_LOG_LOW, "%s: starting physical package...", __FUNCTION__);
    fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    
    fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "%s: load neit package", __FUNCTION__);
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

   /* Wait for the expected number of objects.
   */
   status = fbe_api_wait_for_number_of_objects(MONT_TREMBLANT_NUMBER_OF_OBJECTS, 20000, FBE_PACKAGE_ID_PHYSICAL);
   MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");
   mut_printf(MUT_LOG_LOW, "%s: starting configuration verification...", __FUNCTION__);

    /* We inserted a fleet board so we check for it board is always object id 0
     * ??? Maybe we need an interface to get the board object id. 
     */  
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_LOW, "Verifying board type....Passed");

    /* We should have exactly one port.
     * Get the handle of the port and validate port exists.
     */  
    status = fbe_api_get_port_object_id_by_location (port_number, &object_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

    /* Use the handle to get lifecycle state.
     */  
    status = fbe_api_wait_for_object_lifecycle_state (object_handle, 
                                                      FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME,
                                                      FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Use the handle to get port number.  Make sure the port exists and the
     * port number is zero (we only have one port). 
     */  
    status = fbe_api_get_object_port_number(object_handle, &port_number);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(port_number == 0);
    mut_printf(MUT_LOG_LOW, "Verifying port existance, state and number ...Passed");

    /* We should have exactly one enclosure on the port.
     * Get the handle of the port and validate enclosure exists.
     */
    status = fbe_api_get_enclosure_object_id_by_location (port_number, enclosure_number, &object_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

    /* Use the handle to get lifecycle state.
     */  
    status = fbe_zrt_wait_enclosure_status (port_number, enclosure_number, FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Use the handle to get port number.
     */  
    status = fbe_api_get_object_port_number (object_handle, &port_number);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(port_number == port_number);
    mut_printf(MUT_LOG_LOW, "Verifying enclosure existance and state....Passed");

    /* Check for the physical drives on the enclosure.
     */  
    for (drive_number = 0; drive_number < MONT_TREMBLANT_MAX_DRIVES; drive_number++)
    {
        /* Get the handle of the port and validate drive exists.
         */  
        status = fbe_api_get_physical_drive_object_id_by_location(port_number, enclosure_number, drive_number, &object_handle);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

        /* Use the handle to get lifecycle state.
         */  
        status = fbe_test_pp_util_verify_pdo_state(port_number, 
                                                    enclosure_number, 
                                                    drive_number, 
                                                    FBE_LIFECYCLE_STATE_READY, 
                                                    READY_STATE_WAIT_TIME);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        /* Use the handle to get port number of the drive.
         */ 
        status = fbe_api_get_object_port_number (object_handle, &port_number);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(port_number == port_number);

    }  /* End for all drives. */

    mut_printf(MUT_LOG_LOW, "Verifying physical drive existance and state....Passed");

    /* Let's see how many objects we have...
     * 1 board
     * 1 port
     * 1 enclosure
     * 12 physical drives
     * 12 logical drives
     * 27 objects
     */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == MONT_TREMBLANT_NUMBER_OF_OBJECTS);
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");
    return FBE_STATUS_OK;
}
/******************************************
 * end mont_tremblant_load_and_verify()
 ******************************************/

void mont_tremblant(void)
{
    fbe_status_t run_status;

    run_status = mont_tremblant_run();
        
    MUT_ASSERT_TRUE(run_status == FBE_STATUS_OK);
}

/*************************
 * end file mont_tremblant.c
 *************************/
