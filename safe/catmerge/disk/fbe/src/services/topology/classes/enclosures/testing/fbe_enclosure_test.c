/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * file fbe_enclosure_test.c
 ***************************************************************************
 *
 * 
 *  This file contains testing functions for the enclosure object.
 *
 * HISTORY
 *   7/16/2008:  Created. bphilbin ---Thanks Rob!
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_topology_interface.h"

#include "fbe_trace.h"
#include "fbe_transport_memory.h"

#include "fbe_service_manager.h"
#include "fbe/fbe_physical_package.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe/fbe_transport.h"
#include "fbe_transport_memory.h"
#include "fbe_base_object.h"
#include "fbe_topology.h"
#include "fbe_enclosure_test_prototypes.h"
#include "fbe/fbe_terminator_api.h"
#include "mut.h"
#include "fbe_interface.h"
#include "fbe_cpd_shim.h"
#include "fbe_base_board.h"
#include "fbe_terminator_miniport_interface.h"


/******************************
 ** LOCAL FUNCTION PROTOTYPES *
 ******************************/


/****************************************************************
 * fbe_enclosure_setup()
 ****************************************************************
 * DESCRIPTION:
 *  This function performs generic setup for 
 *  fbe_enclosure test suite.  We setup the framework 
 *  necessary for testing the enclosure object by initting the
 *  physical package.  
 *
 * PARAMETERS:
 *  None.
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  7/16/08 - Created. bphilbin
 *
 ****************************************************************/
fbe_u32_t fbe_enclosure_setup(void) 
{
    fbe_status_t status;
    fbe_terminator_board_info_t     board_info;
    fbe_terminator_sas_port_info_t  sas_port;
    fbe_terminator_api_device_handle_t port_handle;
    fbe_terminator_miniport_interface_port_shim_sim_pointers_t miniport_pointers;

    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_LAST);

    mut_printf(MUT_LOG_MEDIUM, "%s %s ",__FILE__, __FUNCTION__);

    /*before loading the physical package we initialize the terminator*/
    status = fbe_terminator_api_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    board_info.platform_type = SPID_DREADNOUGHT_HW_TYPE;
    status  = fbe_terminator_api_insert_board (&board_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*insert sas belsi on port 0*/
    sas_port.sas_address = (fbe_u64_t)0x123456789;
    sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    sas_port.io_port_number = 0;
    sas_port.portal_number = 1;
    sas_port.backend_number = 0;
    status  = fbe_terminator_api_insert_sas_port (&sas_port, &port_handle);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    /* setup the function pointers to Terminator */
    miniport_pointers.terminator_miniport_api_port_init_function = fbe_terminator_miniport_api_port_init;
    miniport_pointers.terminator_miniport_api_port_destroy_function = fbe_terminator_miniport_api_port_destroy;
    miniport_pointers.terminator_miniport_api_register_callback_function = fbe_terminator_miniport_api_register_callback;
    miniport_pointers.terminator_miniport_api_unregister_callback_function = fbe_terminator_miniport_api_unregister_callback;
    miniport_pointers.terminator_miniport_api_register_payload_completion_function = fbe_terminator_miniport_api_register_payload_completion;
    miniport_pointers.terminator_miniport_api_unregister_payload_completion_function = fbe_terminator_miniport_api_unregister_payload_completion;
    miniport_pointers.terminator_miniport_api_enumerate_cpd_ports_function = fbe_terminator_miniport_api_enumerate_cpd_ports;

    miniport_pointers.terminator_miniport_api_get_port_type_function = fbe_terminator_miniport_api_get_port_type;
    miniport_pointers.terminator_miniport_api_remove_port_function = fbe_terminator_miniport_api_remove_port;
    miniport_pointers.terminator_miniport_api_port_inserted_function = fbe_terminator_miniport_api_port_inserted;
    miniport_pointers.terminator_miniport_api_set_speed_function = fbe_terminator_miniport_api_set_speed;
    miniport_pointers.terminator_miniport_api_get_port_info_function = fbe_terminator_miniport_api_get_port_info;
    miniport_pointers.terminator_miniport_api_send_payload_function = fbe_terminator_miniport_api_send_payload;
    miniport_pointers.terminator_miniport_api_send_fis_function = fbe_terminator_miniport_api_send_fis;
    miniport_pointers.terminator_miniport_api_reset_device_function = fbe_terminator_miniport_api_reset_device;
    miniport_pointers.terminator_miniport_api_get_port_address_function = fbe_terminator_miniport_api_get_port_address;

    fbe_cpd_shim_sim_set_terminator_miniport_pointers(&miniport_pointers);

    fbe_base_board_sim_set_terminator_api_get_board_info_function(fbe_terminator_api_get_board_info);

    fbe_base_board_sim_set_terminator_api_get_sp_id_function(fbe_terminator_api_get_sp_id);

    fbe_base_board_sim_set_terminator_api_is_single_sp_function(fbe_terminator_api_is_single_sp);

    /* Initialize the physical package.  We need the physical package
     * to be initialized so that the enclosure objects are
     * created and available for use.
     */
    status = physical_package_init();

    /* If the physical package init failed then we cannot continue.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Sleep 7 sec. to give a chance to monitor and create objects.
     */
    EmcutilSleep(7000); 

    return status;
}
/* end fbe_enclosure_setup() */

/****************************************************************
 * fbe_enclosure_teardown()
 ****************************************************************
 * DESCRIPTION:
 *  This function performs generic cleanup for 
 *  fbe_enclosure test suite.  We clean up the framework 
 *  necessary for testing the enclosure object by destroying the
 *  physical package.  
 *
 * PARAMETERS:
 *  None.
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  7/16/08 - Created. bphilbin
 *
 ****************************************************************/
fbe_u32_t fbe_enclosure_teardown(void) 
{
    fbe_status_t status;

    mut_printf(MUT_LOG_MEDIUM, "%s %s ",__FILE__, __FUNCTION__);
    
    /* Simply destroy the physical package.
     */
    status = physical_package_destroy();

    /* If the physical package destroy failed then we cannot
     * continue.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_terminator_api_destroy();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return status;
}
/* end fbe_enclosure_teardown() */

/*!***************************************************************
 * @fn fbe_enclosure_unit_tests_setup(
 *                                 mut_testsuite_t * const suite_p)
 ****************************************************************
 * @brief
 *  This function adds unit tests to the input suite.
 *
 * HISTORY:
 *  7/16/08 - Created. bphilbin
 *
 ****************************************************************/
void 
fbe_enclosure_unit_tests_setup(void)
{
    /* Turn down the warning level to limit the trace to the display.
     */
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_ERROR);
    return;
}
/* end fbe_enclosure_unit_tests_setup() */

/*!***************************************************************
 * @fn fbe_enclosure_unit_tests_teardown(
 *                            mut_testsuite_t * const suite_p)
 ****************************************************************
 * @brief
 *  This function adds unit tests to the input suite.
 *
 * HISTORY:
 *  7/16/08 - Created. bphilbin
 *
 ****************************************************************/
void 
fbe_enclosure_unit_tests_teardown(void)
{
    /* Set the trace level back to the default.
     */
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_DEBUG_MEDIUM);

    return;
}
/* end fbe_enclosure_unit_tests_teardown() */

/****************************************************************
 * fbe_enclosure_add_unit_tests()
 ****************************************************************
 * DESCRIPTION:
 *  This function adds unit tests to the input suite.
 *
 * PARAMETERS:
 *  suite_p, [I] - Suite to add to.
 *
 * RETURNS:
 *  None.
 *
 * HISTORY:
 *  7/16/08 - Created. bphilbin
 *
 ****************************************************************/
void fbe_enclosure_add_unit_tests(mut_testsuite_t * const suite_p)
{
    /* Turn down the warning level to limit the trace to the display.
     */
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_WARNING);

    /* Currently this test does nothing but setup and teardown. */
    MUT_ADD_TEST(suite_p,
                 fbe_sas_viper_enclosure_test_create_object_function,
                 fbe_enclosure_setup,
                 fbe_enclosure_teardown); 

    /* Check that the viper enclosure is valid. */
    MUT_ADD_TEST(suite_p,
                 fbe_sas_viper_enclosure_test_lifecycle,
                 fbe_enclosure_setup, 
                 fbe_enclosure_teardown); 

     // Check that component data access is working (without Physical Package)
    MUT_ADD_TEST(suite_p,
                 fbe_sas_viper_enclosure_test_publicDataAccess,
                 NULL,
                 NULL);

    // Check that EDAL with component type spanning multiple blocks is working (without Physical Package)
    MUT_ADD_TEST(suite_p,
                 fbe_sas_viper_enclosure_test_edalFlexComp,
                 NULL,
                 NULL);

   /* size of the CDB and builds the CDB to send "RECEIVE_DIAGNOSTIC_RESULTS" command. */
    MUT_ADD_TEST(suite_p,
                 test_fbe_eses_build_receive_diagnostic_results_cdb,
                 NULL,
                 NULL);
   
   /* Validates size of the CDB and builds the CDB to send "SEND_DIAGNOSTIC"
    *  command which sends the control page to the EMA in the LCC.
    */
    MUT_ADD_TEST(suite_p,
                 test_fbe_eses_build_send_diagnostic_cdb,
                 NULL,
                 NULL);

    /* Function to test functions fbe_sas_viper_enclosure_ps_extract_status
        which parses the power supply status. */
    MUT_ADD_TEST(suite_p,
                 test_fbe_sas_viper_enclosure_ps_extract_status,
                 NULL,
                 NULL);

    /* Function to test functions fbe_sas_viper_enclosure_connector_extract_status
        which parses the connector status. */
    MUT_ADD_TEST(suite_p,
                 test_fbe_sas_viper_enclosure_connector_extract_status,
                 NULL,
                 NULL);
     /* Function to test function fbe_sas_viper_enclosure_array_dev_slot_extract_status
         which parses the array device slot status to fill in the drive info. */
    MUT_ADD_TEST(suite_p,
                 test_fbe_sas_viper_enclosure_array_dev_slot_extract_status,
                 NULL,
                 NULL);

     /* Function to test function fbe_sas_viper_enclosure_exp_phy_extract_status
         which parses the expander phy info. */
    MUT_ADD_TEST(suite_p,
                 test_fbe_sas_viper_enclosure_exp_phy_extract_status,
                 NULL,
                 NULL);

    /* Function to test functions for --cooling_extract_status */
    MUT_ADD_TEST(suite_p,
                 test_fbe_sas_viper_enclosure_cooling_extract_status,
                 NULL,
                 NULL);

    /* Function to test functions for --temp_sensor_extract_status */
    MUT_ADD_TEST(suite_p,
                 test_fbe_sas_viper_enclosure_temp_sensor_extract_status,
                 NULL,
                 NULL);

     MUT_ADD_TEST(suite_p,
                 test_fbe_eses_enclosure_handle_scsi_cmd_response,
                 NULL,
                 NULL);

    return;
}
