/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * home.c (cable pull and insert)
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains the Home scenario.
 * 
 *  This scenario is for a 1 port, 3 enclosure configuration, aka the naxos.xml
 *  configuration.
 * 
 *  The test run in this case is:
 *  1) Create and validate the configuration. (1-Port 3-Enclosures)
 *  2) Remove depth-0 enclosure.
 *  3) Cause the port to receive a logout for the enclosure in depth-0 on both 
 *     expander SMP address and virtual PHY SMP address.
 *  4) Verify that all the objects, except the board and port are in ACTIVATE state.
 *  5) Verify that the enclosure's discovery edge is set to NOT_PRESENT and 
 *     protocol edge to the port have the CLOSED attribute set.     
 *  6) Verify the depth-0 enclosure does not re-LOGIN and the enclosure ride-through
 *     timer expires. Also verify the child objects of the enclosure and the enclosure
 *     itself are destroyed.
 *  7) Insert a different depth-0 enclosure with a different SAS address and make sure
 *     the port receives a LOGIN.
 *  8) Verify that the new enclosure and its child topology are created and in READY state.
 *  9) Repeat steps 1 to 6 using the depth-2 enclosure and verify that this enclosure and
 *     all of its attached drives (both physical and logical) are destroyed, and new 
 *     enclosure and drives are created and in READY state. 
 * 10) Shutdown the physical package.
 *
 * HISTORY
 *   08/01/2008:  Created. ArunKumar Selvapillai
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "physical_package_tests.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_topology_interface.h"
#include "pp_utils.h"

/*************************
 *   MACRO DEFINITIONS
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/****************************************************************
 * home_insert_enclosure()
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

static fbe_status_t home_insert_enclosure(fbe_test_params_t *test, fbe_u32_t encl)
{
    fbe_u32_t                           no_of_drives, no_of_encls;
    fbe_status_t                        insert_status, status;
    fbe_terminator_sas_encl_info_t      sas_encl;
    fbe_terminator_sas_drive_info_t     sas_drive;
    fbe_u32_t                           num_handles;
    fbe_api_terminator_device_handle_t  port_handle, encl_handle, drive_handle;

    mut_printf(MUT_LOG_LOW,
               "Home Scenario: Inserting Enclosure %d (New) with different SAS address.", encl);

    /* search handle of port with given port number */
    status = fbe_api_terminator_get_port_handle(test->backend_number, &port_handle);    
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    for ( no_of_encls = encl; no_of_encls < NAXOS_MAX_ENCLS; no_of_encls++ )
    {
        /* Insert an enclosure with a different SAS address */
        sas_encl.encl_type = test->encl_type;
        sas_encl.sas_address = ((fbe_u64_t)(no_of_encls) << 40) + 0x123456789;
        sas_encl.uid[0] = (fbe_u8_t)no_of_encls; // some unique ID.
        sas_encl.backend_number = test->backend_number;
        sas_encl.encl_number = no_of_encls;
        
        /* Insert depth-no_of_encls enclosure on port 0 */
        status = fbe_test_insert_sas_enclosure(port_handle, &sas_encl, &encl_handle, &num_handles);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        /* Insert drives to the enclosure */
        sas_drive.drive_type = test->drive_type;
        sas_drive.sas_address = ((fbe_u64_t)(no_of_encls) << 40) + 0x443456780;
        sas_drive.backend_number = test->backend_number;
        sas_drive.encl_number = no_of_encls;
        sas_drive.block_size = 520;
        sas_drive.capacity = TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY;

        /* Depth-2 enclosure should contain only 12 drives starting from slot 2 */
        if ( no_of_encls != 2 )
        {
            for ( no_of_drives = 0; no_of_drives < NAXOS_MAX_DRIVES; no_of_drives++ )
            {
                insert_status = fbe_test_insert_sas_drive (encl_handle, no_of_drives, &sas_drive, &drive_handle);
                MUT_ASSERT_TRUE(insert_status == FBE_STATUS_OK);
                sas_drive.sas_address++;
            }
        }
        else
        {
            /* Insert depth-2 enclosure with only 12 drives starting from slot 2 */
            for ( no_of_drives = 2; no_of_drives < 11; no_of_drives++ )
            {
                insert_status = fbe_test_insert_sas_drive (encl_handle, no_of_drives, &sas_drive, &drive_handle);
                MUT_ASSERT_TRUE(insert_status == FBE_STATUS_OK);
                sas_drive.sas_address++;
            }
        }
        /* Wait 2 sec */
        fbe_api_sleep(2000);
        /* Check the state of the newly inserted enclosure object */
        insert_status = fbe_zrt_wait_enclosure_status(test->backend_number, no_of_encls, FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME);
        MUT_ASSERT_TRUE(insert_status == FBE_STATUS_OK);
    }/* End of for..loop */

    mut_printf(MUT_LOG_LOW,
                "Home Scenario: Inserted Enclosure %d (New) with different SAS address.", encl);

    /* Wait 2 sec. This additional 2 sec timer is needed for the enclosure/drives to stabilize
     * before enumerating the objects
     */
    fbe_api_sleep(2000);
        
    return FBE_STATUS_OK;
}

/****************************************************************
 * home_remove_enclosure()
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

static fbe_status_t home_remove_enclosure(fbe_u32_t port, fbe_u32_t encl, fbe_u32_t max_drives, fbe_u64_t drive_mask, fbe_u32_t max_enclosures)
{
    fbe_status_t                          status;
    fbe_status_t                          remove_status;
    fbe_api_terminator_device_handle_t        encl_device_id;
    fbe_object_id_t                       encl_object_id;

    mut_printf(MUT_LOG_LOW,
                "Home Scenario: Removing Enclosure %d", encl);

    /* Validate that the enclosure and all its drives are in READY state. */
    remove_status = fbe_zrt_wait_enclosure_status(port, encl, FBE_LIFECYCLE_STATE_READY, 10);
    MUT_ASSERT_TRUE(remove_status == FBE_STATUS_OK);

    /* Get the device id of the enclosure object to pass it to remove_sas_encl function */
    remove_status = fbe_api_terminator_get_enclosure_handle(port, encl, &encl_device_id);
    MUT_ASSERT_TRUE(remove_status == FBE_STATUS_OK);

    /* Remove the enclosure */
    remove_status  = fbe_api_terminator_remove_sas_enclosure (encl_device_id);
    MUT_ASSERT_TRUE(remove_status == FBE_STATUS_OK);

    /* Verify the device id of the enclosure object we removed does not exist any more 
     * by reading the remove_status as NOT FBE_STATUS_OK. */
    remove_status = fbe_api_terminator_get_enclosure_handle(port, encl, &encl_device_id);
    MUT_ASSERT_TRUE(remove_status != FBE_STATUS_OK);

    /* Wait 1 sec */
    fbe_api_sleep(3000);

    if(encl == 0)
    {
        /* The first enclosure in the discovery path. */
        /* Verify that the enclosure's discovery edge is set to NOT_PRESENT and 
        * protocol edge to the port have the CLOSED attribute set. 
        */
        remove_status = fbe_zrt_verify_edge_info_of_logged_out_enclosure(port, encl);
        MUT_ASSERT_TRUE(remove_status == FBE_STATUS_OK);

        mut_printf(MUT_LOG_LOW,
                    "Home Scenario: Objects in ACTIVATE state after Removal");

        /* Get the enclosure and its drives status (Verify if it is in ACTIVATE state) */
        remove_status = naxos_verify(port, encl, FBE_LIFECYCLE_STATE_ACTIVATE, READY_STATE_WAIT_TIME, max_drives, 0, max_enclosures);
        MUT_ASSERT_TRUE(remove_status == FBE_STATUS_OK);

        /* fbe_api_sleep for 15 secs to allow 12 sec. ride through timer expire */
        fbe_api_sleep(15000);
    }
    else
    {
        /* It is not the first enclosure in the discovery path. 
         * Verify the enclosure has been destroyed.
         */
        fbe_api_sleep(6000);
        status = fbe_api_get_enclosure_object_id_by_location(port, encl, &encl_object_id);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(encl_object_id, FBE_OBJECT_ID_INVALID);
    }

    mut_printf(MUT_LOG_LOW, 
                "Home Scenario: Removed Enclosure %d", encl);

    return FBE_STATUS_OK;
}

/****************************************************************
 * home_run()
 ****************************************************************
 * Description:
 *  Function to simulate the enclosure cable pull and insert.
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *  08/01/2008 - Created. Arunkumar Selvapillai
 *
 ****************************************************************/

static fbe_status_t home_run(fbe_test_params_t *test)
{
    fbe_status_t    home_status;

    mut_printf(MUT_LOG_LOW, "Home Scenario: Start for %s",test->title);

    /****************************************************************
    * Step 2: Case 1: Remove depth-0 enclosure
    ****************************************************************/

    /* Perform removal and insertion for depth-0 enclosure */
    home_status = home_remove_enclosure(test->backend_number, 0, test->max_drives, test->drive_mask, test->max_enclosures);
    MUT_ASSERT_TRUE(home_status == FBE_STATUS_OK);

    /* Enumerate the objects after the removal to verify 
     * if we have removed the actual no. of objects
     * (1 board + 1 port = 2)
     */
    home_status = fbe_test_wait_for_term_pp_sync(10000);
    MUT_ASSERT_TRUE_MSG(home_status == FBE_STATUS_OK, "Wait for number of objects failed!");

    /* Insert a different depth-0 enclosure with a different SAS address and make sure
       the port receives a LOGIN. */
    home_status = home_insert_enclosure(test, 0);
    MUT_ASSERT_TRUE(home_status == FBE_STATUS_OK);

    /* Enumerate the objects after the insert to verify 
     * we got back the original configuration
     * (1 board + 1 port + 3 encl + 42 pdos = 47)
     */
    home_status = fbe_test_wait_for_term_pp_sync(10000);
    MUT_ASSERT_TRUE_MSG(home_status == FBE_STATUS_OK, "Wait for number of objects failed!");

    /* Wait 1 sec */
    fbe_api_sleep(1000);

    /****************************************************************
    * Step 9: Case 2: Remove depth-2 enclosure
    ****************************************************************/

    /* Perform removal and insertion for depth-2 enclosure */
    if(test->encl_type != FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM)
    {
        home_status = home_remove_enclosure(test->backend_number, 2, test->max_drives, test->drive_mask, test->max_enclosures);
    MUT_ASSERT_TRUE(home_status == FBE_STATUS_OK);

    /* Enumerate the objects after the removal to verify 
     * if we have removed the actual no. of objects
     * (1 board + 1 port + 2 encl + 30 pdos  = 34)
     */
        home_status = fbe_test_wait_for_term_pp_sync(10000);
    MUT_ASSERT_TRUE_MSG(home_status == FBE_STATUS_OK, "Wait for number of objects failed!");

    /* Insert depth-2 enclosure with a different SAS address */
        home_status = home_insert_enclosure(test, 2);
    MUT_ASSERT_TRUE(home_status == FBE_STATUS_OK);

    /* Enumerate the objects after the insert to verify 
     * we got back the original configuration
     * (1 board + 1 port + 3 encl + 42 pdos  = 47)
     */
        home_status = fbe_test_verify_term_pp_sync();
    MUT_ASSERT_TRUE_MSG(home_status == FBE_STATUS_OK, "Wait for number of objects failed!");
    }
    mut_printf(MUT_LOG_LOW, "Home Scenario for %s",test->title);

    return FBE_STATUS_OK;
}

/****************************************************************
 * home()
 ****************************************************************
 * Description:
 *  Function to load, unload the config and run the home scenario.
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *  08/01/2008 - Created.
 *
 ****************************************************************/

void home(void)
{
    fbe_status_t run_status;
    fbe_u32_t test_num;
    fbe_test_params_t *home_test;
    fbe_u32_t max_entries ;

    max_entries = fbe_test_get_naxos_num_of_tests();

    for(test_num = 0; test_num < max_entries; test_num++)
    {
        /* Load the configuration for test */
        home_test =  fbe_test_get_naxos_test_table(test_num);
        naxos_load_and_verify_table_driven(home_test);
        
        if(home_test->encl_type != FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM)  //Temporarily disabled this test as it is failing with test suit. 
                                                                                                   //Whereas runs succesfully when executed indivdually.
        {
           run_status = home_run(home_test);
           MUT_ASSERT_TRUE(run_status == FBE_STATUS_OK);
        }
        /* Unload the configuration */
        fbe_test_physical_package_tests_config_unload();
    }
}

