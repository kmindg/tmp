/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * republica_dominicana.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains the Republica Dominicana scenario.
 * 
 *  This scenario is for a 1 port, 3 enclosure configuration, aka the naxos.xml
 *  configuration.
 * 
 *  The test run in this case is:
 *  1) Create and validate the configuration. (1-Port 3-Enclosures)
 *  2) Cause the port to receive a logout for the enclosure in depth-1 on both 
 *     expander SMP address and virtual PHY SMP address.
 *  3) Verify that the enclosure and all its children are in ACTIVATE state.
 *  4) Verify that the enclosure's discovery edge is set to NOT_PRESENT and 
 *     protocol edge to the port have the CLOSED attribute set.     
 *  5) Cause the port to receive a LOGIN for the depth-1 enclosure, before the 
 *     enclosure ride-through timer expires. 
 *  6) Validate that the enclosure and all its children are in READY state.
 *  7) Shutdown the physical package.
 *
 * HISTORY
 *   08/01/2008:  Created. ArunKumar Selvapillai
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "physical_package_tests.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_utils.h"
#include "pp_utils.h"
/*************************
 *   MACRO DEFINITIONS
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/****************************************************************
 * rd_insert_enclosure()
 ****************************************************************
 * Description:
 *  Function to insert an enclosure. This functions also checks for
 *  - the enclosure and its drives transition to READY state.
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *  08/05/2008 - Created. Arunkumar Selvapillai
 *
 ****************************************************************/

static fbe_status_t rd_insert_enclosure(fbe_u32_t port, fbe_u32_t encl,fbe_u32_t max_drives, fbe_u64_t drive_mask, fbe_u32_t max_enclosures)
{
    fbe_status_t                insert_status;
    fbe_api_terminator_device_handle_t    encl_device_id;

    mut_printf(MUT_LOG_LOW,
                "Republica Dominicana Scenario: Inserting Enclosure %d", encl);

    /* Get the device id of the enclosure object to pass it to remove_sas_encl function */
    insert_status = fbe_api_terminator_get_enclosure_handle(port, encl, &encl_device_id);
    MUT_ASSERT_TRUE(insert_status == FBE_STATUS_OK);

    /* Force the login to occur. */
    insert_status = fbe_api_terminator_force_login_device(encl_device_id);
    MUT_ASSERT_TRUE(insert_status == FBE_STATUS_OK);

    /* Sleep 1 secs */
    fbe_api_sleep(1000);

    mut_printf(MUT_LOG_LOW,
                "Republica Dominicana Scenario: Objects in READY state after Insert");

    insert_status = naxos_verify(port, encl, FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME, max_drives, drive_mask, max_enclosures);
    MUT_ASSERT_TRUE(insert_status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW,
                "Republica Dominicana Scenario: Inserted Enclosure %d", encl);

    return FBE_STATUS_OK;
}

/****************************************************************
 * rd_remove_enclosure()
 ****************************************************************
 * Description:
 *  Function to remove an enclosure. This function calls
 * 
 *  zrt_verify_edge_info_of_removed_enclosure to check:
 *  - Edges of discovery and SSP are marked NOT_PRESENT and
 *     CLOSED respectively after the removal.
 *  
 *  naxos_verify to check:
 *  - The enclosure and all its drives transition to ACTIVATE state.
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *  08/05/2008 - Created. Arunkumar Selvapillai
 *
 ****************************************************************/

static fbe_status_t rd_remove_enclosure(fbe_u32_t port, fbe_u32_t encl,fbe_u32_t max_drives, fbe_u64_t drive_mask, fbe_u32_t max_enclosures)
{
    fbe_status_t                remove_status;
    fbe_api_terminator_device_handle_t    encl_device_id;

    mut_printf(MUT_LOG_LOW,
                "Republica Dominicana Scenario: Removing Enclosure %d", encl);

    /* Validate that the enclosure and all its drives are in READY state. */
    remove_status = fbe_zrt_wait_enclosure_status(port, encl, FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME);
    MUT_ASSERT_TRUE(remove_status == FBE_STATUS_OK);

    /* Get the device id of the enclosure object to pass it to remove_sas_encl function */
    remove_status = fbe_api_terminator_get_enclosure_handle(port, encl, &encl_device_id);
    MUT_ASSERT_TRUE(remove_status == FBE_STATUS_OK);

    /* Force the enclosure to receive a LOGOUT */
    remove_status = fbe_api_terminator_force_logout_device(encl_device_id);
    MUT_ASSERT_TRUE(remove_status == FBE_STATUS_OK);

    /* Sleep 1 sec */
    fbe_api_sleep(1000);

    /* Verify that the enclosure's discovery edge is set to NOT_PRESENT and 
     * protocol edge to the port have the CLOSED attribute set. 
     */
    remove_status = fbe_zrt_verify_edge_info_of_logged_out_enclosure(port, encl);
    MUT_ASSERT_TRUE(remove_status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW,
                "Republica Dominicana Scenario: Objects in ACTIVATE state after Removal");

    /* Get the enclosure and the drive status (Verify if they are in ACTIVATE state) */
    remove_status = naxos_verify(port, encl, FBE_LIFECYCLE_STATE_ACTIVATE, READY_STATE_WAIT_TIME, max_drives, drive_mask, max_enclosures);
    MUT_ASSERT_TRUE(remove_status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW,
                "Republica Dominicana Scenario: Removed Enclosure %d", encl);

    return FBE_STATUS_OK;
}

/****************************************************************
 * republica_dominicana_run()
 ****************************************************************
 * Description:
 *  This scenario tests a hard reset on an enclosure.
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *  08/01/2008 - Created. Arunkumar Selvapillai
 *
 ****************************************************************/

static fbe_status_t republica_dominicana_run(fbe_test_params_t *test)
{
    fbe_status_t    rd_status;

    mut_printf(MUT_LOG_LOW, "Republica Dominicana Scenario: Start for %s",test->title);

    /* Verify the existance of the object before performing any action on it */
    rd_status = rd_remove_enclosure(test->backend_number, test->encl_number,test->max_drives, test->drive_mask, test->max_enclosures);
    MUT_ASSERT_TRUE(rd_status == FBE_STATUS_OK);

    /* Enumerate the objects */
    rd_status = fbe_test_wait_for_term_pp_sync(5000);
    MUT_ASSERT_TRUE_MSG(rd_status == FBE_STATUS_OK, "Wait for number of objects failed!");

    /* Sleep for 10 secs. (8+2)
     * We don't want to sleep longer and should wake up and trigger the 
     * insert event before actually the ride thru timer expires.
     */
    fbe_api_sleep(2000); 

    /* Cause the port to receive a LOGIN for the depth-0 enclosure, before the 
     * enclosure ride-through timer expires.
     */
    rd_status = rd_insert_enclosure(test->backend_number, 0, test->max_drives, test->drive_mask, test->max_enclosures);
    MUT_ASSERT_TRUE(rd_status == FBE_STATUS_OK);

    /* Enumerate the objects */
    rd_status = fbe_test_wait_for_term_pp_sync(5000);

    //rd_status = fbe_api_wait_for_number_of_objects(NAXOS_NUMBER_OF_OBJ, 5000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(rd_status == FBE_STATUS_OK, "Wait for number of objects failed!");

     /* Verify the existance of the object before performing any action on it */
    if(test->encl_type != FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM)
    {
        rd_status = rd_remove_enclosure(test->backend_number, 1, test->max_drives, test->drive_mask, test->max_enclosures);
        MUT_ASSERT_TRUE(rd_status == FBE_STATUS_OK);
    
        /* Enumerate the objects */
        rd_status = fbe_test_wait_for_term_pp_sync(5000);
        MUT_ASSERT_TRUE_MSG(rd_status == FBE_STATUS_OK, "Wait for number of objects failed!");
    
        /* Sleep for 10 secs. (8+2)
         * We don't want to sleep longer and should wake up and trigger the 
         * insert event before actually the ride thru timer expires.
         */
        fbe_api_sleep(2000); 
    
        /* Cause the port to receive a LOGIN for the depth-1 enclosure, before the 
         * enclosure ride-through timer expires.
         */
        rd_status = rd_insert_enclosure(test->backend_number, 1, test->max_drives, test->drive_mask, test->max_enclosures);
        MUT_ASSERT_TRUE(rd_status == FBE_STATUS_OK);
    
        /* Enumerate the objects */
        rd_status = fbe_test_verify_term_pp_sync();
        MUT_ASSERT_TRUE_MSG(rd_status == FBE_STATUS_OK, "Wait for number of objects failed!");
    
        mut_printf(MUT_LOG_LOW, "Republica Dominicana Scenario: End for %s",test->title);
    }
    return FBE_STATUS_OK;
}

/****************************************************************
 * republica_dominicana()
 ****************************************************************
 * Description:
 *  Function to load, unload the config and run the 
 *  Republica Dominicana scenario.
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *  03/03/2010 - Updated. To support table driven approach.
 ****************************************************************/

void republica_dominicana(void)
{
    fbe_status_t run_status;
    fbe_u32_t test_num;
    fbe_test_params_t *republicana_dominaca_test;
    fbe_u32_t max_table_entries ;

    max_table_entries = fbe_test_get_naxos_num_of_tests() ;

    for(test_num = 0; test_num < max_table_entries; test_num++)
    {
        /* Load the configuration for test
         */
        republicana_dominaca_test =  fbe_test_get_naxos_test_table(test_num); 
        naxos_load_and_verify_table_driven(republicana_dominaca_test);
        run_status = republica_dominicana_run(republicana_dominaca_test);
        MUT_ASSERT_TRUE(run_status == FBE_STATUS_OK);

        /* Unload the configuration
        */
        fbe_test_physical_package_tests_config_unload();
    }

}
// *  08/01/2008 - Created. 
