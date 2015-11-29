/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file Serengeti_test.c
 ***************************************************************************
 *
 * @brief
 *   This file contains the serengeti test, which is a fbe_test for
 *  injecting errors on drive spinup /spindown via DEST.
 *
 * @version
 *   06/13/2012 - Created. kothal
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
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe_media_error_edge_tap.h"
#include "pp_utils.h"
#include "fbe/fbe_api_dest_injection_interface.h"
#include "fbe/fbe_dest_service.h"
#include "neit_utils.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_drive_configuration_interface.h"
#include "fbe/fbe_api_drive_configuration_service_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_power_saving_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "sep_utils.h"
#include "sep_tests.h"
#include "fbe_test_common_utils.h"                  // define common utility functions 
#include "fbe/fbe_api_base_config_interface.h"


/*************************
 *   TEST DESCRIPTION
 *************************/
char * serengeti_short_desc = "Inject errors on spinup/spindown";
char * serengeti_long_desc =
    "\n"
    "The serengeti scenario tests the ability to inject errors when a drive is spinning down and up.\n"
    "\n"
    "\n"
    "Starting Config:\n"
    "    [PP] armada boards\n"
    "    [PP] SAS PMC port\n"
    "    [PP] viper enclosure\n"
    "    [PP] 1 SAS drives (PDOs)\n"
    "    [PP] 1 logical drives (LDs)\n"
    "    [SEP] 1 provision drives (PVDs)\n"    
    "Last update: 06/13/12\n"
    "\n";
   
/*!*******************************************************************
 * @def SERENGETI_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects we will create.
 *        (1 board + 1 port + 1 encl + 6 pdo) 
 *********************************************************************/
#define SERENGETI_TEST_NUMBER_OF_PHYSICAL_OBJECTS 9


void
serengeti_test_fill_error_injection_record(fbe_dest_error_record_t * record_p,       
        fbe_object_id_t serengeti_pdo_id)
{    
    /* Put PDO object ID into the error record
    */
    record_p->object_id = serengeti_pdo_id;
    
    /* Fill in the fields in the error record that are common to all
     * the error cases.
     */
    record_p->dest_error_type = FBE_DEST_ERROR_TYPE_SCSI;
    record_p->dest_error.dest_scsi_error.scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION;
    record_p->dest_error.dest_scsi_error.port_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
    
    /* Inject the error
     */
    record_p->lba_start = 0;
    record_p->lba_end = 0xFFFFFF ;        
    record_p->dest_error.dest_scsi_error.scsi_command[0] = FBE_SCSI_START_STOP_UNIT;  
    record_p->dest_error.dest_scsi_error.scsi_command_count = 1;
    record_p->dest_error.dest_scsi_error.scsi_sense_key = 0x03 /* FBE_SCSI_SENSE_KEY_MEDIUM_ERROR */;
    record_p->dest_error.dest_scsi_error.scsi_additional_sense_code = ( 0x011 )/* (FBE_SCSI_ASC_READ_DATA_ERROR )*/;
    record_p->dest_error.dest_scsi_error.scsi_additional_sense_code = 0x00; 
    record_p->num_of_times_to_insert = 1;
    record_p->frequency = 0;
    record_p->times_inserting = 0;
    record_p->times_inserted = 0;
    record_p->is_active_record = FALSE;

}

/*!**************************************************************
 * @fn serengeti_inject_scsi_error_stop()
 ****************************************************************
 * @brief
 *   This function stops the protocol error injection.
 *  
 * @return - fbe_status_t - FBE_STATUS_OK on success,
 *                        any other value implies error.
 * 
 *
 ****************************************************************/
fbe_status_t serengeti_inject_scsi_error_stop(void)
{
    fbe_status_t   status = FBE_STATUS_OK;

    /*set the error using NEIT package. */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: stop error injection.", __FUNCTION__);

    status = fbe_api_dest_injection_stop(); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    return status;
}

/*!**************************************************************
 * @fn serengeti_test_inject_not_spinning_error()
 ****************************************************************
 * @brief
 *   This function explicitly sets not_spinning condition.
 *  
 * @return - nothing 
 * 
 *
 ****************************************************************/
static void serengeti_test_inject_not_spinning_error(fbe_dest_error_record_t * record_p, fbe_object_id_t object_id)
{
    /*in simulation, the terminator always returns a good spinup status so we have to force it*/    
    fbe_status_t                                    status;                 // fbe stat   
    fbe_dest_record_handle_t                        record_handle = NULL;   

    /*we'll set a spinup error on PDO 10 so when in ACTIVATE it will explicitly set the not_spinning condition and then 
    spin up is successfull*/   

    record_p->object_id                = object_id; 
    record_p->lba_start                = 0;
    record_p->lba_end                  = 0xFFFFFF;
    record_p->num_of_times_to_insert   = 1;

    record_p->dest_error_type = FBE_DEST_ERROR_TYPE_SCSI;      
    record_p->dest_error.dest_scsi_error.scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION;
    record_p->dest_error.dest_scsi_error.scsi_command_count = 1;
    record_p->dest_error.dest_scsi_error.scsi_command[0] = FBE_SCSI_TEST_UNIT_READY;
    record_p->dest_error.dest_scsi_error.scsi_sense_key = FBE_SCSI_SENSE_KEY_NOT_READY;
    record_p->dest_error.dest_scsi_error.scsi_additional_sense_code = FBE_SCSI_ASC_NOT_READY;
    record_p->dest_error.dest_scsi_error.scsi_additional_sense_code = FBE_SCSI_ASCQ_NOT_SPINNING;
    record_p->num_of_times_to_insert = 1; 
    record_p->times_inserting = 0;
    record_p->times_inserted = 0;
    record_p->frequency = 0;

    /* Add the error record
    */
    mut_printf(MUT_LOG_LOW, "=== Adding an error record to set not_spinning condition ===\n");
    status = fbe_api_dest_injection_add_record(record_p, &record_handle);   
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);   
    
    return;
}

/*!****************************************************************************
 *  serengeti_test
 ******************************************************************************
 *
 * @brief
 *   This is the main entry point for the serengeti test.  
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void serengeti_test(void)
{
    fbe_system_power_saving_info_t              power_save_info;
    fbe_status_t                                status;   
    fbe_u32_t object_handle;   
    fbe_object_id_t serengeti_pvd_id, serengeti_pdo_id;    
    fbe_dest_error_record_t record_p = {0};
    fbe_dest_record_handle_t record_handle = NULL;      
  
    mut_printf(MUT_LOG_LOW, "=== Serengeti test start ===\n");    
        
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Enabling system wide power saving ===\n", __FUNCTION__);
    status  = fbe_api_enable_system_power_save();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    /*make sure it worked*/
    power_save_info.enabled = FBE_FALSE;
    status  = fbe_api_get_system_power_save_info(&power_save_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(power_save_info.enabled == FBE_TRUE);
    
    status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(0, 0, 4, &object_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);
    serengeti_pvd_id = (fbe_object_id_t)object_handle;
    
    /* Get the physical drive object id.
    */
    status = fbe_api_get_physical_drive_object_id_by_location(0, 0, 4, &object_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);
    serengeti_pdo_id = (fbe_object_id_t)object_handle;   
        
    /* Fill the error record
    */
    serengeti_test_fill_error_injection_record(&record_p, serengeti_pdo_id);    
    
    /* Add the error record
    */
    mut_printf(MUT_LOG_LOW, "=== Adding an error record ===\n");
    status = fbe_api_dest_injection_add_record(&record_p, &record_handle);  
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);   
    
    /* Start the error injection 
    */
    mut_printf(MUT_LOG_LOW, "=== Starting error injection ===\n");
    status = fbe_api_dest_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
     /* Disable background zeroing so that drives go into power saving mode*/  
     mut_printf(MUT_LOG_LOW, "=== Disabling background zeroing ===\n");    
    status = fbe_api_base_config_disable_all_bg_services();    
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    /* set PVD object idle time */
    mut_printf(MUT_LOG_LOW, "=== Setting PVD idle time ===\n"); 
    status = fbe_api_set_object_power_save_idle_time(serengeti_pvd_id, 2);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
  
    /* enable the power saving on the PVD*/
    mut_printf(MUT_LOG_LOW, "=== Enabling PVD power save ===\n");   
    status = fbe_api_enable_object_power_save(serengeti_pvd_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);   
    
    mut_printf(MUT_LOG_LOW, "=== Wait for PVD hibernation (idle time 2 sec)  ===\n");        
    
    /*let's wait for a while to let the power saving mode to be enabled*/
    fbe_api_sleep(14000);
    
     /* check that PVD lifecycle sate is HIBERNATE */
    mut_printf(MUT_LOG_LOW, "=== Check if PVD lifecycle state is HIBERNATE ===\n");
    status = fbe_api_wait_for_object_lifecycle_state(serengeti_pvd_id, FBE_LIFECYCLE_STATE_HIBERNATE, 40000, FBE_PACKAGE_ID_SEP_0);    
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);   
    mut_printf(MUT_LOG_LOW, "=== PVD lifecycle state is hibernate  ===\n");     
    
    status = fbe_api_dest_injection_get_record(&record_p, record_handle);  
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(1, record_p.times_inserted);
    mut_printf(MUT_LOG_LOW, "=== Inserted error during spindown ===\n");  
    
    /*in simulation, the terminator always returns a good spinup status so we have to force it*/
    serengeti_test_inject_not_spinning_error(&record_p, serengeti_pdo_id);   
    
    /* disable the power saving on the PVD*/
    mut_printf(MUT_LOG_LOW, "=== Disabling PVD power save ===\n");  
    status = fbe_api_disable_object_power_save(serengeti_pvd_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);      
    
    /*we want to clean and go out w/o leaving the power saving persisted on the disks for next test*/
    mut_printf(MUT_LOG_LOW, "=== Disabling system wide power saving ===\n");
    status  = fbe_api_disable_system_power_save();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);    
        
    /* check that PVD lifecycle sate is ACTIVATE */
    mut_printf(MUT_LOG_LOW, "=== Check if PVD lifecycle state is READY ===\n");      
    status = fbe_api_wait_for_object_lifecycle_state(serengeti_pvd_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "=== PVD lifecycle state is activate  ===\n");     
       
    /* check that PDO lifecycle sate is READY */
    mut_printf(MUT_LOG_LOW, "=== Check if PDO lifecycle state is READY ===\n");
    status = fbe_api_wait_for_object_lifecycle_state(serengeti_pdo_id, FBE_LIFECYCLE_STATE_READY, 40000, FBE_PACKAGE_ID_PHYSICAL);    
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);    
    mut_printf(MUT_LOG_LOW, "=== PDO lifecycle state is ready  ===\n");    
    
    status = fbe_api_dest_injection_get_record(&record_p, record_handle);  
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(1, record_p.times_inserted);
    mut_printf(MUT_LOG_LOW, "=== Inserted error during spinup ===\n");
    
    mut_printf(MUT_LOG_LOW, "=== Stopping error injection ===\n");
    status = serengeti_inject_scsi_error_stop();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);    

    mut_printf(MUT_LOG_LOW, "=== Serengeti test completed ===\n");    

    return;

}
/***************************************************************
 * end serengeti_test()
 ***************************************************************/
/*!**************************************************************
 * serengeti_physical_config()
 ****************************************************************
 * @brief
 *  Configure the serengeti test's physical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 *
 ****************************************************************/

void serengeti_physical_config(void)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                               total_objects = 0;
    fbe_class_id_t                          class_id;
    fbe_api_terminator_device_handle_t      port0_handle;
    fbe_terminator_api_device_handle_t      encl0_0_handle;
    fbe_api_terminator_device_handle_t      drive_handle;
    fbe_u32_t                               slot = 0;
    fbe_object_id_t                     object_id;
    
    mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration ===\n");
    
    mut_printf(MUT_LOG_TEST_STATUS, "=== configuring terminator ===\n");
    /* before loading the physical package we initialize the terminator */
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    /* Insert a board
     */
    status = fbe_test_pp_util_insert_armada_board();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    /* Insert a port
     */
    fbe_test_pp_util_insert_sas_pmc_port(   1, /* io port */
                                            2, /* portal */
                                            0, /* backend number */ 
                                            &port0_handle);
    
    /* insert an enclosures to port 0
    */
    fbe_test_pp_util_insert_viper_enclosure(0, 0, port0_handle, &encl0_0_handle);
    
    /* Insert drives for enclosures.
     */
    for ( slot = 0; slot < 6; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 0, slot, 520, TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY, &drive_handle);
    }
    
    /* Init fbe api on server */
    mut_printf(MUT_LOG_TEST_STATUS, "=== starting Server FBE_API ===\n");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    mut_printf(MUT_LOG_TEST_STATUS, "=== starting physical package===\n");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    mut_printf(MUT_LOG_TEST_STATUS, "=== set physical package entries ===\n");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);    
    
    mut_printf(MUT_LOG_LOW, "=== starting system discovery ===\n");
    /* Wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(SERENGETI_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 20000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");
    
    mut_printf(MUT_LOG_TEST_STATUS, "=== verifying configuration ===\n");
    
    /* Inserted a armada board so we check for it;
     * board is always assumed to be object id 0
     */
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying board type....Passed");
    
    /* Make sure we have the expected number of objects.
     */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == SERENGETI_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");
    
    for (object_id = 0 ; object_id < SERENGETI_TEST_NUMBER_OF_PHYSICAL_OBJECTS; object_id++)
    {
        status = fbe_api_wait_for_object_lifecycle_state(object_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    
    return;
}

/*!**************************************************************
 * serengeti_test_setup()
 ****************************************************************
 * @brief
 *  This is the setup function for the serengeti test. It is responsible
 *  for loading the physical and logical configuration for this test suite.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/

void serengeti_test_setup(void)
{

    mut_printf(MUT_LOG_LOW, "=== serengeti setup ===\n");     
    
    /* Only load config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        /* Load the physical configuration 
         */
        serengeti_physical_config();
    
        /* Load the logical configuration 
         */
        sep_config_load_sep_and_neit();
    }  
    
    /* Initialize any required fields and perform cleanup if required
    */
    fbe_test_common_util_test_setup_init();

    return;

} /* End serengeti_test_setup() */

/*!****************************************************************************
 *  serengeti_cleanup
 ******************************************************************************
 *
 * @brief
 *   This function unloads the logical and physical packages and destroy the configuration.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void serengeti_cleanup(void)
{   
    /* Only destroy config in simulation  
     */
    if (fbe_test_util_is_simulation())
    {
        mut_printf(MUT_LOG_LOW, "===  serengeti destroy starts ===\n");
        fbe_test_sep_util_destroy_neit_sep_physical();
        mut_printf(MUT_LOG_LOW, "=== serengeti destroy completes ===\n");
    }
    return; 
}
/***************************************************************
 * end serengeti_cleanup()
 ***************************************************************/
